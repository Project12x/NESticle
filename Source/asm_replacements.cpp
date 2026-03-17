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
// BG tiles use CBASE (224) + pal*4. NO transparency check —
// all pixels are drawn including pixel value 0.
// Original ASM: 'or dl,224' then DWORD writes, no per-pixel check.
// -----------------------------------------------------------
void __cdecl draw_tile_asm(bitmap8x8 *b, char *dest, int x, int y,
                           unsigned char pal) {
  unsigned char colorbase = 224 + (pal << 2);

  // Fast path: entirely on-screen, no clipping needed
  if (x >= 0 && x + 8 <= SCREENX && y >= 0 && y + 8 <= SCREENY) {
    char *dp = dest + y * PITCH + x;
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        dp[col] = (char)(b->s[row][col] | colorbase);
      }
      dp += PITCH;
    }
    return;
  }

  // Slow path: clipped
  for (int row = 0; row < 8; row++) {
    int dy = y + row;
    if (dy < 0 || dy >= SCREENY)
      continue;
    for (int col = 0; col < 8; col++) {
      int dx = x + col;
      if (dx < 0 || dx >= SCREENX)
        continue;
      dest[dy * PITCH + dx] = (char)(b->s[row][col] | colorbase);
    }
  }
}

// -----------------------------------------------------------
// draw_sprite_asm - Draw an 8x8 sprite with transparency,
//                   orientation (flip X/Y), and clipping
// Sprites use CBASE+16 (240) + pal*4
// -----------------------------------------------------------
void __cdecl draw_sprite_asm(bitmap8x8 *b, char *dest, int x, int y, int o,
                             unsigned char pal) {
  unsigned char colorbase = 240 + (pal << 2);
  int flipx = o & 1;
  int flipy = o & 2;

  // Fast path: entirely on-screen
  if (x >= 0 && x + 8 <= SCREENX && y >= 0 && y + 8 <= SCREENY) {
    char *dp = dest + y * PITCH + x;
    for (int row = 0; row < 8; row++) {
      int sy = flipy ? (7 - row) : row;
      for (int col = 0; col < 8; col++) {
        int sx = flipx ? (7 - col) : col;
        unsigned char pixel = b->s[sy][sx];
        if (pixel != 0)
          dp[col] = (char)(pixel | colorbase);
      }
      dp += PITCH;
    }
    return;
  }

  // Clipped path
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
      if (pixel != 0)
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
  unsigned char colorbase = 240 + (pal << 2);
  int flipx = o & 1;
  int flipy = o & 2;

  // Fast path: entirely on-screen
  if (x >= 0 && x + 8 <= SCREENX && y >= 0 && y + 8 <= SCREENY) {
    char *dp = dest + y * PITCH + x;
    for (int row = 0; row < 8; row++) {
      int sy = flipy ? (7 - row) : row;
      for (int col = 0; col < 8; col++) {
        int sx = flipx ? (7 - col) : col;
        unsigned char pixel = b->s[sy][sx];
        if (pixel != 0) {
          unsigned char existing = (unsigned char)dp[col];
          if (existing == 0 || (existing & 0x03) == 0)
            dp[col] = (char)(pixel | colorbase);
        }
      }
      dp += PITCH;
    }
    return;
  }

  // Clipped path
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
      if (pixel != 0) {
        unsigned char existing = (unsigned char)dest[dy * PITCH + dx];
        if (existing == 0 || (existing & 0x03) == 0)
          dest[dy * PITCH + dx] = (char)(pixel | colorbase);
      }
    }
  }
}

// -----------------------------------------------------------
// drawimager2 - Draw an RLE-compressed image
// Matches original algorithm from R2IMG.CPP exactly.
// RLE format: each scanline is pairs of (skip, count, pixels...)
// terminated when remaining width x <= 0.
// -----------------------------------------------------------
void __cdecl drawimager2(IMG *s, char *d, int x, int y, int o) {
  if (!s || !d)
    return;
  (void)o; // orientation not implemented in original either

  int imgxw = s->xw;
  int imgyw = s->yw;
  int *yd = s->ydisp();

  // Clip: skip if entirely off-screen
  if (x >= SCREENX || y >= SCREENY || x + imgxw <= 0 || y + imgyw <= 0)
    return;

  d += x + y * PITCH;

  for (int row = 0; row < imgyw; row++) {
    if (y + row >= 0 && y + row < SCREENY) {
      unsigned char *src = ((unsigned char *)s) + yd[row];
      char *dp = d;

      for (int remaining = imgxw; remaining > 0;) {
        // Read skip (transparent) count
        unsigned int skip = *src++;
        dp += skip;
        remaining -= skip;
        if (remaining <= 0)
          break;

        // Read opaque run count + pixel data
        unsigned int count = *src++;
        // Clip-aware copy
        int destoff = (int)(dp - d) + x; // absolute x position
        if (destoff >= 0 && destoff + (int)count <= SCREENX) {
          // Fast path: entirely on-screen
          memcpy(dp, src, count);
        } else {
          // Slow path: per-pixel clip
          for (unsigned int i = 0; i < count; i++) {
            int px = destoff + (int)i;
            if (px >= 0 && px < SCREENX)
              dp[i] = src[i];
          }
        }
        src += count;
        dp += count;
        remaining -= count;
      }
    }
    d += PITCH;
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

// Palette loading - update sdl_palette via extern C function in
// platform_sdl2.cpp
extern "C" void sdl_set_palette_entry(int index, unsigned char r,
                                      unsigned char g, unsigned char b);

// loadpalette8: 8-bit palette data (0-255 per component)
void __cdecl loadpalette8(void *c, int first, int num) {
  if (!c)
    return;
  unsigned char *p = (unsigned char *)c;
  for (int i = 0; i < num; i++) {
    sdl_set_palette_entry(first + i, p[i * 3 + 0], p[i * 3 + 1], p[i * 3 + 2]);
  }
}

// loadpalette6: 6-bit VGA palette data (0-63 per component, scale to 0-255)
void __cdecl loadpalette6(void *c, int first, int num) {
  if (!c)
    return;
  unsigned char *p = (unsigned char *)c;
  for (int i = 0; i < num; i++) {
    sdl_set_palette_entry(first + i, (p[i * 3 + 0] & 63) * 255 / 63,
                          (p[i * 3 + 1] & 63) * 255 / 63,
                          (p[i * 3 + 2] & 63) * 255 / 63);
  }
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
  if (xw <= 0 || yw <= 0)
    return;
  int x1 = (x >= 0) ? x : 0;
  int y1 = (y >= 0) ? y : 0;
  int x2 = x + xw;
  if (x2 > SCREENX)
    x2 = SCREENX;
  int y2 = y + yw;
  if (y2 > SCREENY)
    y2 = SCREENY;
  if (x1 >= x2 || y1 >= y2)
    return;
  for (int row = y1; row < y2; row++)
    memset(d + row * PITCH + x1, (unsigned char)color, x2 - x1);
}
