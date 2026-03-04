// gui_vol_gen.cpp - Generates a minimal gui.vol for NESticle
// Run this once to create gui.vol with placeholder resources.
//
// Build: cl /EHsc gui_vol_gen.cpp /Fe:gui_vol_gen.exe
// Run:   gui_vol_gen.exe (produces gui.vol in current directory)

#include <cstdio>
#include <cstdlib>
#include <cstring>


// Match NESticle's VOL.H header struct exactly (NO pack pragma, same as
// NESticle uses)
struct header {
  char key[4];   // "DSL"
  char type;     // type of data
  unsigned size; // size of data
  char name[9];  // name
};

// IMG header matching NESticle's R2IMG.H
struct IMG_HDR {
  int type;   // 1 = IMG_TYPE
  int size;   // total size
  int xw, yw; // dimensions
};

// R2 palette entry
struct COLOR {
  unsigned char r, g, b;
};

// FONT header matching NESticle's FONT.H
struct FONT_HDR {
  int size;
  unsigned short indices[128];
};

static FILE *fp = nullptr;

void write_header(char type, int datasize, const char *name) {
  header h;
  memset(&h, 0, sizeof(h));
  memcpy(h.key, "DSL", 3);
  h.key[3] = 0;
  h.type = type;
  h.size = datasize;
  strncpy(h.name, name, 8);
  fwrite(&h, sizeof(h), 1, fp);
}

// Create a small RLE IMG: single color filled rectangle
unsigned char *create_img(int xw, int yw, unsigned char color, int *out_size) {
  int hdr_size = sizeof(IMG_HDR);
  int ydisp_size = yw * sizeof(int);
  int rle_per_line = 2 + xw;
  int rle_total = yw * rle_per_line;
  int total = hdr_size + ydisp_size + rle_total;

  unsigned char *buf = (unsigned char *)calloc(1, total);
  IMG_HDR *img = (IMG_HDR *)buf;
  img->type = 1;
  img->size = total;
  img->xw = xw;
  img->yw = yw;

  int *ydisp = (int *)(buf + hdr_size);
  unsigned char *rle = buf + hdr_size + ydisp_size;

  for (int y = 0; y < yw; y++) {
    ydisp[y] = (int)((rle + y * rle_per_line) - buf);
    unsigned char *line = rle + y * rle_per_line;
    line[0] = 0;
    line[1] = (unsigned char)xw;
    memset(line + 2, color, xw);
  }

  *out_size = total;
  return buf;
}

unsigned char *create_font(int *out_size) {
  int font_hdr_size = sizeof(FONT_HDR);
  int char_img_size;
  unsigned char *char_template = create_img(6, 8, 0xCA, &char_img_size);

  int num_chars = 96;
  int total = font_hdr_size + num_chars * char_img_size;
  unsigned char *buf = (unsigned char *)calloc(1, total);

  FONT_HDR *fnt = (FONT_HDR *)buf;
  fnt->size = total;
  memset(fnt->indices, 0, sizeof(fnt->indices));

  for (int c = 32; c < 128; c++) {
    int offset = font_hdr_size + (c - 32) * char_img_size;
    fnt->indices[c] = (unsigned short)offset;
    memcpy(buf + offset, char_template, char_img_size);
  }

  free(char_template);
  *out_size = total;
  return buf;
}

int main() {
  printf("sizeof(header) = %d\n", (int)sizeof(header));
  printf("sizeof(IMG_HDR) = %d\n", (int)sizeof(IMG_HDR));
  printf("sizeof(FONT_HDR) = %d\n", (int)sizeof(FONT_HDR));

  fp = fopen("gui.vol", "wb");
  if (!fp) {
    fprintf(stderr, "Cannot create gui.vol\n");
    return 1;
  }

  // 1. Volume header
  write_header(0, 0, "GUI");

  // 2. Palette (256 * 3 bytes)
  {
    COLOR pal[256];
    memset(pal, 0, sizeof(pal));
    for (int i = 0; i < 16; i++)
      pal[i].r = pal[i].g = pal[i].b = (unsigned char)(i * 4);
    pal[0x0F].r = pal[0x0F].g = pal[0x0F].b = 63;
    pal[0xCA].r = 50;
    pal[0xCA].g = 45;
    pal[0xCA].b = 40;
    pal[0xDB].r = 0;
    pal[0xDB].g = 0;
    pal[0xDB].b = 0;
    pal[0xC6].r = 50;
    pal[0xC6].g = 40;
    pal[0xC6].b = 30;
    pal[0xD8].r = 55;
    pal[0xD8].g = 50;
    pal[0xD8].b = 45;
    pal[0x9E].r = 10;
    pal[0x9E].g = 10;
    pal[0x9E].b = 20;
    pal[2].r = 0;
    pal[2].g = 0;
    pal[2].b = 40;
    pal[8].r = 20;
    pal[8].g = 20;
    pal[8].b = 20;
    pal[15].r = 63;
    pal[15].g = 63;
    pal[15].b = 63;

    int sz = sizeof(pal);
    write_header(5, sz, "PAL");
    fwrite(pal, sz, 1, fp);
  }

  // 3-15: cursor, font, checkmark, umark, dmark, lmark, rmark,
  //        about, shadowsel, play, stop, playlooped, active, xmark
  // That's 14 entries (matching GUIVOL: pal + 13 pointers after pal)
  struct {
    const char *name;
    int dim;
    unsigned char color;
  } entries[] = {
      {"CURSOR", 8, 15},
      // FONT entry handled specially below
  };

  // cursor
  {
    int sz;
    unsigned char *img = create_img(8, 8, 15, &sz);
    write_header(2, sz, "CURSOR");
    fwrite(img, sz, 1, fp);
    free(img);
  }

  // font
  {
    int sz;
    unsigned char *fnt = create_font(&sz);
    write_header(2, sz, "FONT");
    fwrite(fnt, sz, 1, fp);
    free(fnt);
  }

  // remaining 11 images
  const char *names[] = {"CHECK", "UMARK",    "DMARK",  "LMARK",
                         "RMARK", "ABOUT",    "SHADOW", "PLAY",
                         "STOP",  "PLAYLOOP", "ACTIVE", "XMARK"};
  int dims[] = {7, 8, 8, 8, 8, 32, 8, 12, 12, 12, 12, 8};
  unsigned char cols[] = {15, 15, 15, 15, 15, 8, 8, 15, 15, 15, 15, 15};

  for (int n = 0; n < 12; n++) {
    int sz;
    unsigned char *img = create_img(dims[n], dims[n], cols[n], &sz);
    write_header(2, sz, names[n]);
    fwrite(img, sz, 1, fp);
    free(img);
  }

  long pos = ftell(fp);
  fclose(fp);
  printf("gui.vol created: %ld bytes, 15 entries\n", pos);
  return 0;
}
