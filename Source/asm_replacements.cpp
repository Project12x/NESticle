// asm_replacements.cpp - C implementations of the original x86 ASM blitters
// Replaces: SPRITE.ASM, SPRITEBG.ASM, TILE.ASM, R2IMGASM.ASM
//
// These are functionally identical to the original TASM/MASM routines but
// written in portable C. Performance is not a concern on modern hardware.

#include <cstring>

// Globals from dd.h (defined in platform layer)
extern "C" int SCREENX, SCREENY, PITCH, BANKED;

// Forward declarations for types used
struct bitmap8x8 {
  unsigned char s[8][8];
};
struct IMG {
  int type;
  int size;
  int xw, yw;
  int *ydisp() { return (int *)(((char *)this) + sizeof(IMG)); }
  char *data() { return (char *)(ydisp() + yw); }
};
struct COLORMAP {
  char c[256];
};

extern "C" {

// -----------------------------------------------------------
// draw_tile_asm - Draw an 8x8 tile with palette offset
// -----------------------------------------------------------
void __cdecl draw_tile_asm(bitmap8x8 *b, char *dest, int x, int y,
                           unsigned char pal) {
  unsigned char colorbase = (pal << 2) | (224 + 16);

  for (int row = 0; row < 8; row++) {
    int dy = y + row;
    if (dy < 0 || dy >= SCREENY)
      continue;
    for (int col = 0; col < 8; col++) {
      int dx = x + col;
      if (dx < 0 || dx >= SCREENX)
        continue;
      unsigned char pixel = b->s[row][col];
      if (pixel == 0)
        continue; // transparent
      dest[dy * PITCH + dx] = (char)(pixel | colorbase);
    }
  }
}

// -----------------------------------------------------------
// draw_sprite_asm - Draw an 8x8 sprite with transparency,
//                   orientation (flip X/Y), and clipping
// -----------------------------------------------------------
void __cdecl draw_sprite_asm(bitmap8x8 *b, char *dest, int x, int y, int o,
                             unsigned char pal) {
  unsigned char colorbase = (pal << 2) | (224 + 16);
  int flipx = o & 1;
  int flipy = o & 2;

  for (int row = 0; row < 8; row++) {
    int sy = flipy ? (7 - row) : row;
    int dy = y + row;
    if (dy < 0 || dy >= SCREENY)
      continue;
    for (int col = 0; col < 8; col++) {
      int sx = flipx ? (7 - col) : col;
      int dx = x + col;
      if (dx < 0 || dx >= SCREENX)
        continue;
      unsigned char pixel = b->s[sy][sx];
      if (pixel == 0)
        continue; // transparent
      dest[dy * PITCH + dx] = (char)(pixel | colorbase);
    }
  }
}

// -----------------------------------------------------------
// draw_sprite_behind_asm - Same as above but only draws into
//                          transparent (zero) pixels
// -----------------------------------------------------------
void __cdecl draw_sprite_behind_asm(bitmap8x8 *b, char *dest, int x, int y,
                                    int o, unsigned char pal) {
  unsigned char colorbase = (pal << 2) | (224 + 16);
  int flipx = o & 1;
  int flipy = o & 2;

  for (int row = 0; row < 8; row++) {
    int sy = flipy ? (7 - row) : row;
    int dy = y + row;
    if (dy < 0 || dy >= SCREENY)
      continue;
    for (int col = 0; col < 8; col++) {
      int sx = flipx ? (7 - col) : col;
      int dx = x + col;
      if (dx < 0 || dx >= SCREENX)
        continue;
      unsigned char pixel = b->s[sy][sx];
      if (pixel == 0)
        continue; // transparent
      // Only draw behind - don't overwrite existing non-zero pixels
      unsigned char existing = (unsigned char)dest[dy * PITCH + dx];
      if (existing != 0 && (existing & 0x03) != 0)
        continue;
      dest[dy * PITCH + dx] = (char)(pixel | colorbase);
    }
  }
}

// -----------------------------------------------------------
// drawimager2 - Draw an RLE-compressed image
// -----------------------------------------------------------
void __cdecl drawimager2(IMG *s, char *d, int x, int y, int o) {
  if (!s || !d)
    return;
  int xw = s->xw;
  int yw = s->yw;
  int *yd = s->ydisp();
  char *data = s->data();
  int flipx = o & 2; // IMG_FLIPX
  int flipy = o & 1; // IMG_FLIPY

  for (int row = 0; row < yw; row++) {
    int srcrow = flipy ? (yw - 1 - row) : row;
    int dy = y + row;
    if (dy < 0 || dy >= SCREENY)
      continue;

    // Each scanline's RLE data starts at offset yd[srcrow]
    char *rle = (char *)s + yd[srcrow];
    int dx = flipx ? (x + xw - 1) : x;
    int xdir = flipx ? -1 : 1;

    while (1) {
      unsigned char skip = (unsigned char)*rle++;
      if (skip == 0xFF)
        break; // end of scanline
      dx += skip * xdir;

      unsigned char count = (unsigned char)*rle++;
      for (int i = 0; i < count; i++) {
        unsigned char pixel = (unsigned char)*rle++;
        int px = dx;
        if (px >= 0 && px < SCREENX) {
          d[dy * PITCH + px] = (char)pixel;
        }
        dx += xdir;
      }
    }
  }
}

// -----------------------------------------------------------
// Drawing primitives
// -----------------------------------------------------------

// Colormap rectangle
void __cdecl drawrectmap(char *d, COLORMAP *c, int x, int y, int xw, int yw) {
  int x1 = (x >= 0) ? x : 0;
  int y1 = (y >= 0) ? y : 0;
  int x2 = x + xw;
  if (x2 > SCREENX)
    x2 = SCREENX;
  int y2 = y + yw;
  if (y2 > SCREENY)
    y2 = SCREENY;

  for (int row = y1; row < y2; row++) {
    for (int col = x1; col < x2; col++) {
      unsigned char src = (unsigned char)d[row * PITCH + col];
      d[row * PITCH + col] = c->c[src];
    }
  }
}

// Horizontal line
void __cdecl drawhline(char *d, int color, int x, int y, int x2) {
  if (y < 0 || y >= SCREENY)
    return;
  if (x < 0)
    x = 0;
  if (x2 > SCREENX)
    x2 = SCREENX;
  if (x >= x2)
    return;
  memset(d + y * PITCH + x, (unsigned char)color, x2 - x);
}

// Vertical line
void __cdecl drawvline(char *d, int color, int x, int y, int y2) {
  if (x < 0 || x >= SCREENX)
    return;
  if (y < 0)
    y = 0;
  if (y2 > SCREENY)
    y2 = SCREENY;
  for (int row = y; row < y2; row++) {
    d[row * PITCH + x] = (unsigned char)color;
  }
}

// Palette loading stubs (palette is handled by SDL2 in the platform layer)
void __cdecl loadpalette8(void *c, int first, int num) {
  (void)c;
  (void)first;
  (void)num;
}
void __cdecl loadpalette6(void *c, int first, int num) {
  (void)c;
  (void)first;
  (void)num;
}

// drawscr - uncompressed bitmap blit (replaces ASM version)
void __cdecl drawscr(char *s, char *d, int xw, int yw, int pitch) {
  for (int row = 0; row < yw; row++) {
    memcpy(d, s, xw);
    s += pitch;
    d += PITCH;
  }
}

} // extern "C"

// Filled rectangle - C++ linkage (declared in R2IMG.H under #ifdef WIN95)
void drawrect(char *d, int color, int x, int y, int xw, int yw) {
  int x1 = (x >= 0) ? x : 0;
  int y1 = (y >= 0) ? y : 0;
  int x2 = x + xw;
  if (x2 > SCREENX)
    x2 = SCREENX;
  int y2 = y + yw;
  if (y2 > SCREENY)
    y2 = SCREENY;
  for (int row = y1; row < y2; row++)
    memset(d + row * PITCH + x1, (unsigned char)color, x2 - x1);
}
