// platform_sdl2.cpp - SDL2 platform backend for NESticle
// Replaces WIN95/DX.CPP (DirectDraw/DirectSound) and DOS backends
// Provides: video, audio, input, timer, main loop

// WIN95 is defined globally by CMake
#include <SDL.h>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <windows.h>

#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

static volatile int g_framecount = 0;
static LONG WINAPI crash_handler(EXCEPTION_POINTERS *ep) {
  fprintf(stderr, "[CRASH] Exception 0x%08lX at addr %p, frame=%d\n",
          ep->ExceptionRecord->ExceptionCode,
          ep->ExceptionRecord->ExceptionAddress, g_framecount);

  void *stack[100];
  unsigned short frames = CaptureStackBackTrace(0, 100, stack, NULL);
  HANDLE process = GetCurrentProcess();
  SymInitialize(process, NULL, TRUE);

  char symbol_buffer[sizeof(SYMBOL_INFO) + 256 * sizeof(char)];
  SYMBOL_INFO *symbol = (SYMBOL_INFO *)symbol_buffer;
  symbol->MaxNameLen = 255;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

  for (int i = 0; i < frames; i++) {
    SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
    fprintf(stderr, "%i: %s\n", i, symbol->Name);
  }

  fflush(stderr);
  return EXCEPTION_EXECUTE_HANDLER;
}

// NESticle headers
#include "command.h"
#include "config.h"
#include "cursor_frames.h"
#include "dd.h"
#include "guivol.h"
#include "keyb.h"
#include "message.h"
#include "mouse.h"
#include "nesvideo.h"
#include "prof.h"
#include "r2img.h"
#include "sound.h"
#include "timing.h"

// Functions supplied by the game (MAIN.CPP)
int initgame();
void updatescreen();
void terminategame();
void gametimer();

// ---- Globals matching DD.H ----
extern "C" int PITCH = 0;
extern "C" int BANKED = 0;
char *video = nullptr;
char *screen = nullptr;
char errstr[80] = {0};

// Timer
volatile int timerbusy = 0;
static int timerdisabled = 0;

// Mouse
mouse m;

// Active state
static int ActiveApp = 1;

// Globals
static int sdl_scale = 2; // DPI scale factor (auto-detected)
static SDL_Window *sdl_window = nullptr;
static SDL_Renderer *sdl_renderer = nullptr;
static SDL_Texture *sdl_texture = nullptr;
static Uint32 *sdl_framebuffer = nullptr;
static unsigned char *sdl_screen = nullptr;
SDL_Color sdl_palette[256];

// Sound
extern "C" {
extern int SOUNDRATE;
}
extern class nessound *ns;
extern unsigned char soundenabled;
static SDL_AudioDeviceID sdl_audio_device = 0;

// From timing.h
extern int TIMERSPEED;

// Globals that NESticle references
int blah = 0, blah2 = 0;

// ---- Platform functions ----

void disable() { timerdisabled = 1; }
void enable() { timerdisabled = 0; }
void quitgame() { ActiveApp = 0; }
void cleanexit(int x) {
  fprintf(stderr, "[FATAL] cleanexit(%d) called: %s\n", x, errstr);
  fflush(stderr);
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "NESticle Error", errstr,
                           nullptr);
  SDL_Quit();
  exit(x);
}

void setpalette(PALETTE *pal) {
  for (int i = 0; i < 256; i++) {
    sdl_palette[i].r = pal->c[i].r;
    sdl_palette[i].g = pal->c[i].g;
    sdl_palette[i].b = pal->c[i].b;
    sdl_palette[i].a = 255;
  }
}

void setpalettenum(int index, COLOR *c) {
  sdl_palette[index].r = c->r;
  sdl_palette[index].g = c->g;
  sdl_palette[index].b = c->b;
  sdl_palette[index].a = 255;
}

// Accessible from asm_replacements.cpp without SDL_Color type
extern "C" void sdl_set_palette_entry(int index, unsigned char r,
                                      unsigned char g, unsigned char b) {
  sdl_palette[index].r = r;
  sdl_palette[index].g = g;
  sdl_palette[index].b = b;
  sdl_palette[index].a = 255;
}

static void convert_screen_to_argb() {
  for (int i = 0; i < SCREENX * SCREENY; i++) {
    unsigned char idx = sdl_screen[i];
    sdl_framebuffer[i] = (255u << 24) | (sdl_palette[idx].r << 16) |
                         (sdl_palette[idx].g << 8) | sdl_palette[idx].b;
  }
}

void changeresolution(int newx, int newy) {
  SCREENX = newx;
  SCREENY = newy;
  PITCH = SCREENX;
  delete[] sdl_screen;
  delete[] sdl_framebuffer;
  sdl_screen = new unsigned char[SCREENX * SCREENY];
  sdl_framebuffer = new Uint32[SCREENX * SCREENY];
  memset(sdl_screen, 0, SCREENX * SCREENY);
  screen = video = (char *)sdl_screen;
  if (sdl_texture)
    SDL_DestroyTexture(sdl_texture);
  sdl_texture =
      SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, SCREENX, SCREENY);
}

void *loadresource(char *name) {
  (void)name;
  return nullptr;
}
void settimerspeed(int x) { TIMERSPEED = x; }

void toggle_sdl_fullscreen() {
  if (!sdl_window)
    return;
  Uint32 flags = SDL_GetWindowFlags(sdl_window);
  if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
    // Leaving fullscreen: restore 640x480 desktop buffer
    SDL_SetWindowFullscreen(sdl_window, 0);
    changeresolution(640, 480);
    SDL_RenderSetLogicalSize(sdl_renderer, 640, 480);
  } else {
    // Entering fullscreen: switch to NES native 256x224
    changeresolution(256, 224);
    SDL_RenderSetLogicalSize(sdl_renderer, 256, 224);
    SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
  }
}

int is_sdl_fullscreen() {
  if (!sdl_window)
    return 0;
  return (SDL_GetWindowFlags(sdl_window) & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 1 : 0;
}

// ---- Surface class (offscreen rendering) ----
struct SurfaceData {
  unsigned char *pixels;
  int w, h;
};

surface::surface(int txw, int tyw) : oldscrx(0), oldscry(0), oldpitch(0) {
  xw = txw;
  yw = tyw;
  auto *sd = new SurfaceData();
  sd->pixels = new unsigned char[txw * tyw];
  sd->w = txw;
  sd->h = tyw;
  memset(sd->pixels, 0, txw * tyw);
  dds = (IDirectDrawSurface *)sd;
}

surface::~surface() {
  if (dds) {
    auto *sd = (SurfaceData *)dds;
    delete[] sd->pixels;
    delete sd;
  }
}

char *surface::lock() {
  if (!dds)
    return nullptr;
  auto *sd = (SurfaceData *)dds;
  oldscrx = SCREENX;
  oldscry = SCREENY;
  oldpitch = PITCH;
  SCREENX = sd->w;
  SCREENY = sd->h;
  PITCH = sd->w;
  return (char *)sd->pixels;
}

void surface::unlock() {
  SCREENX = oldscrx;
  SCREENY = oldscry;
  PITCH = oldpitch;
}

int surface::blt(char *dest, int x, int y) {
  if (!dds)
    return 0;
  auto *sd = (SurfaceData *)dds;
  for (int row = 0; row < sd->h; row++) {
    int dy = y + row;
    if (dy < 0 || dy >= SCREENY)
      continue;
    for (int col = 0; col < sd->w; col++) {
      int dx = x + col;
      if (dx < 0 || dx >= SCREENX)
        continue;
      dest[dy * PITCH + dx] = (char)sd->pixels[row * sd->w + col];
    }
  }
  return 1;
}

int surface::blt(int sx, int sy, int sx2, int sy2, char *dest, int x, int y) {
  if (!dds)
    return 0;
  auto *sd = (SurfaceData *)dds;
  int width = sx2 - sx;
  int height = sy2 - sy;
  for (int row = 0; row < height; row++) {
    int srow = sy + row;
    int dy = y + row;
    if (dy < 0 || dy >= SCREENY || srow < 0 || srow >= sd->h)
      continue;
    for (int col = 0; col < width; col++) {
      int scol = sx + col;
      int dx = x + col;
      if (dx < 0 || dx >= SCREENX || scol < 0 || scol >= sd->w)
        continue;
      dest[dy * PITCH + dx] = (char)sd->pixels[srow * sd->w + scol];
    }
  }
  return 1;
}

// ---- Keyboard translation ----
static char sdl_to_nesticle_scancode(SDL_Scancode sc) {
  switch (sc) {
  case SDL_SCANCODE_ESCAPE:
    return 0x01;
  case SDL_SCANCODE_1:
    return 0x02;
  case SDL_SCANCODE_2:
    return 0x03;
  case SDL_SCANCODE_3:
    return 0x04;
  case SDL_SCANCODE_4:
    return 0x05;
  case SDL_SCANCODE_5:
    return 0x06;
  case SDL_SCANCODE_6:
    return 0x07;
  case SDL_SCANCODE_7:
    return 0x08;
  case SDL_SCANCODE_8:
    return 0x09;
  case SDL_SCANCODE_9:
    return 0x0A;
  case SDL_SCANCODE_0:
    return 0x0B;
  case SDL_SCANCODE_MINUS:
    return 0x0C;
  case SDL_SCANCODE_EQUALS:
    return 0x0D;
  case SDL_SCANCODE_BACKSPACE:
    return 0x0E;
  case SDL_SCANCODE_TAB:
    return 0x0F;
  case SDL_SCANCODE_Q:
    return 0x10;
  case SDL_SCANCODE_W:
    return 0x11;
  case SDL_SCANCODE_E:
    return 0x12;
  case SDL_SCANCODE_R:
    return 0x13;
  case SDL_SCANCODE_T:
    return 0x14;
  case SDL_SCANCODE_Y:
    return 0x15;
  case SDL_SCANCODE_U:
    return 0x16;
  case SDL_SCANCODE_I:
    return 0x17;
  case SDL_SCANCODE_O:
    return 0x18;
  case SDL_SCANCODE_P:
    return 0x19;
  case SDL_SCANCODE_LEFTBRACKET:
    return 0x1A;
  case SDL_SCANCODE_RIGHTBRACKET:
    return 0x1B;
  case SDL_SCANCODE_RETURN:
    return 0x1C;
  case SDL_SCANCODE_LCTRL:
    return 0x1D;
  case SDL_SCANCODE_A:
    return 0x1E;
  case SDL_SCANCODE_S:
    return 0x1F;
  case SDL_SCANCODE_D:
    return 0x20;
  case SDL_SCANCODE_F:
    return 0x21;
  case SDL_SCANCODE_G:
    return 0x22;
  case SDL_SCANCODE_H:
    return 0x23;
  case SDL_SCANCODE_J:
    return 0x24;
  case SDL_SCANCODE_K:
    return 0x25;
  case SDL_SCANCODE_L:
    return 0x26;
  case SDL_SCANCODE_SEMICOLON:
    return 0x27;
  case SDL_SCANCODE_APOSTROPHE:
    return 0x28;
  case SDL_SCANCODE_LSHIFT:
    return 0x2A;
  case SDL_SCANCODE_Z:
    return 0x2C;
  case SDL_SCANCODE_X:
    return 0x2D;
  case SDL_SCANCODE_C:
    return 0x2E;
  case SDL_SCANCODE_V:
    return 0x2F;
  case SDL_SCANCODE_B:
    return 0x30;
  case SDL_SCANCODE_N:
    return 0x31;
  case SDL_SCANCODE_M:
    return 0x32;
  case SDL_SCANCODE_SPACE:
    return 0x39;
  case SDL_SCANCODE_F1:
    return 0x3B;
  case SDL_SCANCODE_F2:
    return 0x3C;
  case SDL_SCANCODE_F3:
    return 0x3D;
  case SDL_SCANCODE_F4:
    return 0x3E;
  case SDL_SCANCODE_F5:
    return 0x3F;
  case SDL_SCANCODE_F6:
    return 0x40;
  case SDL_SCANCODE_F7:
    return 0x41;
  case SDL_SCANCODE_F8:
    return 0x42;
  case SDL_SCANCODE_F9:
    return 0x43;
  case SDL_SCANCODE_F10:
    return 0x44;
  case SDL_SCANCODE_HOME:
    return 0x47;
  case SDL_SCANCODE_UP:
    return 0x48;
  case SDL_SCANCODE_PAGEUP:
    return 0x49;
  case SDL_SCANCODE_LEFT:
    return 0x4B;
  case SDL_SCANCODE_RIGHT:
    return 0x4D;
  case SDL_SCANCODE_END:
    return 0x4F;
  case SDL_SCANCODE_DOWN:
    return 0x50;
  case SDL_SCANCODE_PAGEDOWN:
    return 0x51;
  case SDL_SCANCODE_INSERT:
    return 0x52;
  case SDL_SCANCODE_DELETE:
    return 0x53;
  case SDL_SCANCODE_F11:
    return (char)0x85;
  case SDL_SCANCODE_F12:
    return (char)0x86;
  default:
    return 0;
  }
}

// ---- Keyboard state ----
volatile char kbscan = 0;
volatile char kbstat = 0;
volatile char keydown[128] = {0};
static int (*keyboard_func)() = nullptr;

void set_keyboard_func(int (*kfunc)()) { keyboard_func = kfunc; }

void wm_keydown(char k) {
  if (k <= 0)
    return;
  kbscan = k;
  keydown[k & 0x7F] = 1;
  kbstat = 0;
  const Uint8 *state = SDL_GetKeyboardState(nullptr);
  if (state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT])
    kbstat |= 1;
  if (state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL])
    kbstat |= 2;
  if (state[SDL_SCANCODE_LALT] || state[SDL_SCANCODE_RALT])
    kbstat |= 4;
  if (keyboard_func)
    keyboard_func();
}

void wm_keyup(char k) {
  if (k <= 0)
    return;
  kbscan = k | (char)0x80;
  keydown[k & 0x7F] = 0;
  kbstat = 0;
  const Uint8 *state = SDL_GetKeyboardState(nullptr);
  if (state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT])
    kbstat |= 1;
  if (state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL])
    kbstat |= 2;
  if (state[SDL_SCANCODE_LALT] || state[SDL_SCANCODE_RALT])
    kbstat |= 4;
  if (keyboard_func)
    keyboard_func();
}

void pushkey(char k) { (void)k; }
char waitkey() { return 0; }
char getkey() { return 0; }
int keyhit() { return 0; }

char scan2ascii(char s) {
  static const char map[] = {0,    27,  '1', '2',  '3', '4', '5', '6', '7', '8',
                             '9',  '0', '-', '=',  8,   9,   'q', 'w', 'e', 'r',
                             't',  'y', 'u', 'i',  'o', 'p', '[', ']', 13,  0,
                             'a',  's', 'd', 'f',  'g', 'h', 'j', 'k', 'l', ';',
                             '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n',
                             'm',  ',', '.', '/',  0,   '*', 0,   ' '};
  int idx = s & 0x7F;
  if (idx < (int)sizeof(map))
    return map[idx];
  return 0;
}

char *getkeyname(char key) {
  static char name[16];
  int k = key & 0x7F;

  // Complete DOS scancode to name mapping
  static const char *dos_names[128] = {
      0,       "Esc",   "1",    "2",     "3",    "4",    "5",    "6",
      "7",     "8",     "9",    "0",     "-",    "=",    "BkSp", "Tab",
      "Q",     "W",     "E",    "R",     "T",    "Y",    "U",    "I",
      "O",     "P",     "[",    "]",     "Enter","Ctrl", "A",    "S",
      "D",     "F",     "G",    "H",     "J",    "K",    "L",    ";",
      "'",     "`",     "Shift","\\",    "Z",    "X",    "C",    "V",
      "B",     "N",     "M",    ",",     ".",    "/",    "RShft","Num*",
      "Alt",   "Space", "Caps", "F1",    "F2",   "F3",   "F4",   "F5",
      "F6",    "F7",    "F8",   "F9",    "F10",  "Num",  "Scrl", "Num7",
      "Up",    "Num9",  "Num-", "Left",  "Num5", "Right","Num+", "Num1",
      "Down",  "Num3",  "Num.", 0,       0,      0,      "F11",  "F12",
  };

  if (k > 0 && k < 88 && dos_names[k])
    return (char *)dos_names[k];

  sprintf(name, "Key%02X", k);
  return name;
}

// ---- Sound ----
#define PRIMARY_BUF_SIZE 8192
static short primary_buf[PRIMARY_BUF_SIZE];
static int primary_write_pos = 0;
static int primary_read_pos = 0;
unsigned char soundinstalled = 0;

void clearprimary() {
  memset(primary_buf, 0, sizeof(primary_buf));
  primary_write_pos = primary_read_pos = 0;
}

void primaryoutput(short *buf, int nums) {
  for (int i = 0; i < nums; i++) {
    primary_buf[primary_write_pos] = buf[i];
    primary_write_pos = (primary_write_pos + 1) % PRIMARY_BUF_SIZE;
  }
}

void primaryskip(int nums) {
  primary_write_pos = (primary_write_pos + nums) % PRIMARY_BUF_SIZE;
}

static void sdl_audio_callback(void *userdata, Uint8 *stream, int len) {
  (void)userdata;
  short *out = (short *)stream;
  int samples = len / 2;
  for (int i = 0; i < samples; i++) {
    if (primary_read_pos != primary_write_pos) {
      out[i] = primary_buf[primary_read_pos];
      primary_read_pos = (primary_read_pos + 1) % PRIMARY_BUF_SIZE;
    } else {
      out[i] = 0;
    }
  }
}

// DirectSound stubs
struct IDirectSoundBuffer;
IDirectSoundBuffer *createdsoundbuffer(SOUND *s) {
  (void)s;
  return nullptr;
}
void playsound(IDirectSoundBuffer *dsb, int x) {
  (void)dsb;
  (void)x;
}
void playsoundlooped(SOUND *s) { (void)s; }
void freesound(IDirectSoundBuffer *dsb) { (void)dsb; }
int getsoundsize(IDirectSoundBuffer *dsb) {
  (void)dsb;
  return 0;
}

// ---- Drawing helpers ----
void line(char *dest, int x1, int y1, int x2, int y2, int Color) {
  int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
  int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
  int err = dx + dy, e2;
  while (1) {
    if (x1 >= 0 && x1 < SCREENX && y1 >= 0 && y1 < SCREENY)
      dest[y1 * PITCH + x1] = (char)Color;
    if (x1 == x2 && y1 == y2)
      break;
    e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x1 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y1 += sy;
    }
  }
}

// drawbox is in R2IMG.CPP

int random(int x) {
  if (x <= 0)
    return 0;
  return rand() % x;
}

// ddrawinfo stub
void ddrawinfo() {}

// ---- MAIN ----
int main(int argc, char *argv[]) {
#ifdef _WIN32
  // Prevent Windows from DPI-scaling the window (we handle scaling ourselves)
  SetProcessDPIAware();
#endif

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER |
               SDL_INIT_GAMECONTROLLER) != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

#ifdef _WIN32
  // Set Windows timer resolution to 1ms for accurate 60Hz frame timing
  timeBeginPeriod(1);
#endif

  // Auto-detect DPI scale: use 3x for 4K+, 2x otherwise
  float ddpi = 0;
  if (SDL_GetDisplayDPI(0, &ddpi, nullptr, nullptr) == 0 && ddpi > 144.0f)
    sdl_scale = 3;
  else
    sdl_scale = 2;
  // Allow --scale N override
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--scale") == 0 && i + 1 < argc) {
      int s = atoi(argv[i + 1]);
      if (s >= 1 && s <= 5)
        sdl_scale = s;
      i++;
    }
  }
  fprintf(stderr, "[DPI] scale=%dx (DPI=%.0f)\n", sdl_scale, ddpi);
  fflush(stderr);

  sdl_window =
      SDL_CreateWindow("NESticle", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 640 * sdl_scale, 480 * sdl_scale,
                       SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!sdl_window) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  // Hide OS cursor — NESticle draws its own ARGB cursor overlay
  SDL_ShowCursor(SDL_DISABLE);

  sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
  if (!sdl_renderer)
    sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_SOFTWARE);
  // Use nearest-neighbor for crisp pixel art scaling
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
  // Let SDL handle scaling/letterboxing to any window/fullscreen size
  SDL_RenderSetLogicalSize(sdl_renderer, SCREENX, SCREENY);

  // Allocate 8-bit indexed screen buffer
  PITCH = SCREENX;
  sdl_screen = new unsigned char[SCREENX * SCREENY];
  sdl_framebuffer = new Uint32[SCREENX * SCREENY];
  memset(sdl_screen, 0, SCREENX * SCREENY);
  screen = video = (char *)sdl_screen;

  sdl_texture =
      SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, SCREENX, SCREENY);

  // Audio
  SDL_AudioSpec want, have;
  memset(&want, 0, sizeof(want));
  want.freq = SOUNDRATE;
  want.format = AUDIO_S16;
  want.channels = 1;
  want.samples = 512;
  want.callback = sdl_audio_callback;
  sdl_audio_device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
  if (sdl_audio_device > 0) {
    soundinstalled = 1;
    SOUNDRATE = have.freq;
    SDL_PauseAudioDevice(sdl_audio_device, 0);
  }

  // Config
  extern config *cfg;
  extern char configfile[];
  cfg = new config();
  cfg->load(configfile);

  // Init game
  fprintf(stderr, "[LOG] Calling initgame()...\n");
  fflush(stderr);
  if (initgame() != 0) {
    fprintf(stderr, "[FATAL] initgame() returned error\n");
    fflush(stderr);
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR, "NESticle",
        "initgame() failed. Make sure gui.vol is in the working directory.",
        sdl_window);
    SDL_Quit();
    return 1;
  }
  fprintf(stderr, "[LOG] initgame() succeeded\n");
  fflush(stderr);

  // Command line ROM
  if (argc > 1) {
    extern int cmd_runrom(char *p);
    cmd_runrom(argv[1]);
  }

  // Main loop
  fprintf(stderr, "[LOG] Entering main loop\n");
  fflush(stderr);
  Uint32 lasttime = SDL_GetTicks();
  int done = 0;
  int framecount = 0;
  SetUnhandledExceptionFilter(crash_handler);
  while (!done && ActiveApp) {
    {
      m.reset();
      SDL_Event ev;
      while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT:
          done = 1;
          break;
        case SDL_KEYDOWN: {
          if (ev.key.keysym.sym == SDLK_RETURN &&
              (ev.key.keysym.mod & KMOD_ALT)) {
            extern void toggle_emulator_fullscreen();
            toggle_emulator_fullscreen();
            break;
          }
          char sc = sdl_to_nesticle_scancode(ev.key.keysym.scancode);
          if (sc)
            wm_keydown(sc);
          break;
        }
        case SDL_KEYUP: {
          char sc = sdl_to_nesticle_scancode(ev.key.keysym.scancode);
          if (sc)
            wm_keyup(sc);
          break;
        }
        case SDL_MOUSEMOTION: {
          // SDL_RenderSetLogicalSize maps coordinates to logical space
          m.updatexy(ev.motion.x, ev.motion.y);
          break;
        }
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
          int mb = 0;
          Uint32 state = SDL_GetMouseState(nullptr, nullptr);
          if (state & SDL_BUTTON(1))
            mb |= 1;
          if (state & SDL_BUTTON(3))
            mb |= 2;
          m.updatebut(mb);
          break;
        }
        }
      }

      // Timer
      Uint32 now = SDL_GetTicks();
      int numtimer = 0;
      while (now >= lasttime + (Uint32)(1000 / TIMERSPEED)) {
        if (++numtimer < 5)
          gametimer();
        lasttime += (1000 / TIMERSPEED);
      }

      // Only render if we actually ticked (avoid redundant rendering)
      if (numtimer > 0) {
        if (nv)
          nv->refreshpalette();
        memset(sdl_screen, 0, SCREENX * SCREENY);
        screen = video = (char *)sdl_screen;
        updatescreen();

        convert_screen_to_argb();

        // Nearest-neighbour scale NES content to fill windowlet if larger than native
        if (nv) {
          int src_x = nv->x1, src_y = nv->y1;
          int src_w = nv->xw,  src_h = nv->yw; // NES native (256x224)
          int dst_w = nv->width(), dst_h = nv->height();
          // Clamp source region to framebuffer
          if (src_x < 0) src_x = 0;
          if (src_y < 0) src_y = 0;
          if (src_x + src_w > SCREENX) src_w = SCREENX - src_x;
          if (src_y + src_h > SCREENY) src_h = SCREENY - src_y;
          // Clamp dest region to framebuffer
          if (src_x + dst_w > SCREENX) dst_w = SCREENX - src_x;
          if (src_y + dst_h > SCREENY) dst_h = SCREENY - src_y;
          if (dst_w > src_w && dst_h > src_h && src_w > 0 && src_h > 0) {
            // Copy native region to temp buffer
            static Uint32 nes_temp[256 * 240];
            for (int y = 0; y < src_h; y++)
              for (int x = 0; x < src_w; x++)
                nes_temp[y * src_w + x] =
                    sdl_framebuffer[(src_y + y) * SCREENX + src_x + x];
            // Scale temp into windowlet region (write bottom-up to avoid overwriting source)
            for (int y = dst_h - 1; y >= 0; y--) {
              int sy = y * src_h / dst_h;
              for (int x = dst_w - 1; x >= 0; x--) {
                int sx = x * src_w / dst_w;
                sdl_framebuffer[(src_y + y) * SCREENX + src_x + x] =
                    nes_temp[sy * src_w + sx];
              }
            }
          }
        }

        // Draw animated cursor overlay directly to ARGB framebuffer
        // This replaces the static guivol.cursor with a 4-frame animation
        // from the original NESticle cursor
        if (!m.hidden) {
          static int cursor_frame = 0;
          static unsigned last_uu = 0;
          if (uu - last_uu >= 12) {
            cursor_frame = (cursor_frame + 1) % CURSOR_FRAME_COUNT;
            last_uu = uu;
          }
          const unsigned int *frame = cursor_frames[cursor_frame];
          for (int cy = 0; cy < CURSOR_FRAME_H; cy++) {
            int sy = m.y + cy;
            if (sy < 0 || sy >= SCREENY)
              continue;
            for (int cx = 0; cx < CURSOR_FRAME_W; cx++) {
              int sx = m.x + cx;
              if (sx < 0 || sx >= SCREENX)
                continue;
              unsigned int pixel = frame[cy * CURSOR_FRAME_W + cx];
              if (pixel & 0xFF000000) { // non-transparent
                sdl_framebuffer[sy * SCREENX + sx] = pixel;
              }
            }
          }
        }

        SDL_UpdateTexture(sdl_texture, nullptr, sdl_framebuffer,
                          SCREENX * sizeof(Uint32));
        SDL_RenderClear(sdl_renderer);
        SDL_RenderCopy(sdl_renderer, sdl_texture, nullptr, nullptr);
        SDL_RenderPresent(sdl_renderer);
        framecount++;
        g_framecount = framecount;
      }
    }
  } // end while loop

  terminategame();
  if (sdl_audio_device > 0)
    SDL_CloseAudioDevice(sdl_audio_device);
  if (sdl_texture)
    SDL_DestroyTexture(sdl_texture);
  if (sdl_renderer)
    SDL_DestroyRenderer(sdl_renderer);
  if (sdl_window)
    SDL_DestroyWindow(sdl_window);
  delete[] sdl_screen;
  delete[] sdl_framebuffer;
  SDL_Quit();
  return 0;
}
