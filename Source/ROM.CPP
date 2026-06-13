// routines for handling roms

#include <malloc.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys\stat.h>
#ifdef _MSC_VER
#include <direct.h>
#endif

#include "types.h"

#include "devlog.h"
#include "file.h"
#include "message.h"
#include "config.h"
#include "save_state_format.h"

#include "m6502\m6502.h"

#include "keyb.h"

#define FREE(x)                                                                \
  if (x) {                                                                     \
    free(x);                                                                   \
    x = 0;                                                                     \
  }
#define DELETE(x)                                                              \
  if (x) {                                                                     \
    delete x;                                                                  \
    x = 0;                                                                     \
  }

#include "nes.h"
#include "nessound.h"
#include "nesvideo.h"

#include "r2img.h"

#include "mmc.h"
#include "slist.h"

// rom stuff
NESROMHEADER romhdr;
byte *trainer;
int numrom, numvrom;
ROMPAGE *ROM;
VROMPAGE *VROM;

//
byte *ram;             // CPU address space
NES_ppumemory *ppu;    // PPU address space
NES_sprite *spritemem; // Sprite memory

BATTERYMEM *batterymem; // battery backed memory

byte *expansion; // memory at 5000

int NESinitialized = 0;

int romloaded = 0;
void freerom();
void m_reset();
extern config *cfg;
extern char configfile[];

// rom filename (without extension) eg "CASTLEVA"
char romfilename[512];

static void remember_loaded_rom(char *path) {
  if (!cfg || !path || !path[0])
    return;

  char fullpath[512];
#ifdef _MSC_VER
  if (!_fullpath(fullpath, path, sizeof(fullpath)))
#endif
    strncpy(fullpath, path, sizeof(fullpath) - 1);
  fullpath[sizeof(fullpath) - 1] = 0;

  char dir[260];
  strncpy(dir, fullpath, sizeof(dir) - 1);
  dir[sizeof(dir) - 1] = 0;
  char *slash = strrchr(dir, '\\');
  char *fslash = strrchr(dir, '/');
  if (!slash || (fslash && fslash > slash))
    slash = fslash;
  if (slash) {
    *slash = 0;
    strncpy(cfg->lastromdir, dir, sizeof(cfg->lastromdir) - 1);
    cfg->lastromdir[sizeof(cfg->lastromdir) - 1] = 0;
  }

  for (int i = 0; i < MAX_RECENT_ROMS; i++) {
    if (!stricmp(cfg->recentroms[i], fullpath)) {
      for (int j = i; j > 0; j--)
        strcpy(cfg->recentroms[j], cfg->recentroms[j - 1]);
      strncpy(cfg->recentroms[0], fullpath, sizeof(cfg->recentroms[0]) - 1);
      cfg->recentroms[0][sizeof(cfg->recentroms[0]) - 1] = 0;
      cfg->save(configfile);
      return;
    }
  }

  for (int i = MAX_RECENT_ROMS - 1; i > 0; i--)
    strcpy(cfg->recentroms[i], cfg->recentroms[i - 1]);
  strncpy(cfg->recentroms[0], fullpath, sizeof(cfg->recentroms[0]) - 1);
  cfg->recentroms[0][sizeof(cfg->recentroms[0]) - 1] = 0;
  cfg->save(configfile);
}

//-----------------------------------------------------
// NES rom loader

static char *filename_part(char *path) {
  char *slash = strrchr(path, '\\');
  char *fslash = strrchr(path, '/');
  if (!slash || (fslash && fslash > slash))
    slash = fslash;
  return slash ? slash + 1 : path;
}

static void make_nes_paths(char *romfile, char *rompath, int rompathsize,
                           char *rombase, int rombasesize) {
  strncpy(rombase, romfile, rombasesize - 1);
  rombase[rombasesize - 1] = 0;

  char *name = filename_part(rombase);
  char *lastdot = strrchr(name, '.');
  if (lastdot)
    *lastdot = 0;

  strncpy(rompath, rombase, rompathsize - 5);
  rompath[rompathsize - 5] = 0;
  strcat(rompath, ".nes");
}

int loadrom(char *romfile) {
  if (!romfile)
    return 0;

  // get rom name without extension
  char newromfilename[512];
  char s[512];
  make_nes_paths(romfile, s, sizeof(s), newromfilename,
                 sizeof(newromfilename));

  msg.printf(2, "Loading rom %s...", s);
  FILEIO f;
  if (f.open(s)) {
    msg.error("Unable to open file %s", s);
    fprintf(stderr, "[ROM] open failed: '%s'\n", s);
    fflush(stderr);
    return 0;
  }

  // read header
  NESROMHEADER newhdr;
  if (f.read(&newhdr, sizeof(newhdr))) {
    msg.error("Unable to read ROM header from %s", s);
    fprintf(stderr, "[ROM] short or unreadable header: '%s'\n", s);
    fflush(stderr);
    f.close();
    return 0;
  }

  // see if header file exists ?
  FILEIO g;
  char hdrfile[512];
  strncpy(hdrfile, newromfilename, sizeof(hdrfile) - 5);
  hdrfile[sizeof(hdrfile) - 5] = 0;
  strcat(hdrfile, ".hdr");
  if (!g.open(hdrfile)) // does header file exist?
  {
    if (g.read(&newhdr, sizeof(newhdr))) {
      msg.error("Unable to read header file %s", hdrfile);
      fprintf(stderr, "[ROM] short or unreadable header override: '%s'\n",
              hdrfile);
      fflush(stderr);
      g.close();
      f.close();
      return 0;
    }
    g.close();
    msg.printf(1, "Header read from %s", hdrfile);
  }

  int newnumrom = (unsigned char)newhdr.num16k;
  int newnumvrom = (unsigned char)newhdr.num8k;
  if (!newhdr.validate() || newnumrom <= 0) {
    msg.error("Bad ROM header in %s", s);
    fprintf(stderr, "[ROM] bad header: '%s'\n", s);
    fflush(stderr);
    f.close();
    return 0;
  }

  // load trainer
  byte *newtrainer = 0;
  if (newhdr.trainer) {
    newtrainer = (byte *)f.readalloc(512);
    if (!newtrainer) {
      msg.error("Unable to read trainer from %s", s);
      fprintf(stderr, "[ROM] trainer read failed: '%s'\n", s);
      fflush(stderr);
      f.close();
      return 0;
    }
  }

  // read ROM
  ROMPAGE *newROM = (ROMPAGE *)f.readalloc(newnumrom * sizeof(ROMPAGE));
  if (!newROM) {
    msg.error("Unable to read PRG ROM from %s", s);
    fprintf(stderr, "[ROM] PRG read failed: '%s'\n", s);
    fflush(stderr);
    FREE(newtrainer);
    f.close();
    return 0;
  }

  // read VROM
  VROMPAGE *newVROM = 0;
  if (newnumvrom > 0) {
    newVROM = (VROMPAGE *)f.readalloc(newnumvrom * sizeof(VROMPAGE));
    if (!newVROM) {
      msg.error("Unable to read CHR ROM from %s", s);
      fprintf(stderr, "[ROM] CHR read failed: '%s'\n", s);
      fflush(stderr);
      FREE(newtrainer);
      FREE(newROM);
      f.close();
      return 0;
    }
  }

  int bytesleft = f.size() - f.getpos();
  if (bytesleft)
    msg.printf(1, "%d bytes left at end of .NES file", bytesleft);

  f.close();

  // The new ROM is fully staged. Now it is safe to drop the old running state.
  freerom();
  remember_loaded_rom(s);
  strncpy(romfilename, newromfilename, sizeof(romfilename) - 1);
  romfilename[sizeof(romfilename) - 1] = 0;
  romhdr = newhdr;
  trainer = newtrainer;
  numrom = newnumrom;
  ROM = newROM;
  numvrom = newnumvrom;
  VROM = newVROM;
  msg.printf(1, "%dK ROM - %dK VROM", numrom * sizeof(ROMPAGE) / 1024,
             numvrom * sizeof(VROMPAGE) / 1024);
  // msg.printf(2,"%s loaded",romfile);
  romloaded = 1;

  if (NESTICLE_DEVLOG_ENABLED) {
    fprintf(stderr, "[ROM] Loaded: '%s'\n", s);
    fprintf(stderr, "[ROM] PRG banks: %d (%dK), CHR banks: %d (%dK)\n", numrom,
            numrom * 16, numvrom, numvrom * 8);
    fprintf(stderr,
            "[ROM] Mapper: %d, Mirroring: %d, Battery: %d, Trainer: %d\n",
            (int)romhdr.banktype, (int)romhdr.mirroring, (int)romhdr.battery,
            (int)romhdr.trainer);
    fprintf(stderr,
            "[ROM] Header bytes: %02X %02X %02X %02X %02X %02X %02X %02X\n",
            ((unsigned char *)&romhdr)[0], ((unsigned char *)&romhdr)[1],
            ((unsigned char *)&romhdr)[2], ((unsigned char *)&romhdr)[3],
            ((unsigned char *)&romhdr)[4], ((unsigned char *)&romhdr)[5],
            ((unsigned char *)&romhdr)[6], ((unsigned char *)&romhdr)[7]);
    fflush(stderr);
  }

  if (romhdr.battery) {
    batterymem = new BATTERYMEM;
    batterymem->read(romfilename); // read battery backed rom
  } else
    batterymem = 0;

  /*
 char expfile[64];
 strcpy(expfile,romfilename);
 strcat(expfile,".500");
 if (!g.open(expfile)) //exp file exists?
  {
   expansion=(char *)g.readalloc(0x1000);
   g.close();
   msg.printf(1,"Expansion mem read from %s",expfile);
  } */

  // initialize nes hardware....
  initNEShardware();

  // create video space
  new nesvideo(romfilename);

  // create nes sound device
  ns = new nessound;

  // auto-start emulation
  m_reset();
  return 1;
}

void freerom() {
  if (batterymem) {
    batterymem->copyfromRAM();
    batterymem->write(romfilename); // write battery memory!
  }

  terminateNEShardware();
  FREE(trainer);
  FREE(ROM);
  FREE(VROM);
  FREE(batterymem);
  FREE(expansion);

  DELETE(ns);

  if (romloaded)
    msg.printf(2, "ROM freed");
  romloaded = 0;

  // delete video space
  if (nv && nv->parent)
    delete nv->parent;
}

void m_showmessages();
void m_getrominfo() {
  if (!romloaded)
    msg.error("ROM not loaded");
  else
    romhdr.printinfo(); // print info about rom
  m_showmessages();
}

void m_free() {
  if (!romloaded)
    msg.error("ROM not loaded");
  else
    freerom();
}

void m_ramdump() {
  if (!romloaded) {
    msg.error("ROM not loaded");
    return;
  }

  char s[64];

  for (int i = 0; i < 99; i++) {
    sprintf(s, "%s.d%02d", romfilename, i);
    struct stat sb;
    if (stat(s, &sb) != 0)
      break;
  }

  FILEIO f;
  if (f.create(s)) {
    msg.error("Unable to create RAM dump %s", s);
    fprintf(stderr, "[DUMP] create failed: '%s'\n", s);
    fflush(stderr);
    return;
  }
  if (f.write(ram, 0x8000) || f.write(m6502rom + 0x8000, 0x8000)) {
    f.close();
    msg.error("Unable to write RAM dump %s", s);
    fprintf(stderr, "[DUMP] write failed: '%s'\n", s);
    fflush(stderr);
    return;
  }
  f.close();

  msg.printf(1, "RAM dumped to %s", s);
}

void m_romdump() {
  if (!romloaded) {
    msg.error("ROM not loaded");
    return;
  }
  char s[64];
  for (int i = 0; i < numrom; i++) {
    sprintf(s, "%s.R%02X", romfilename, i);
    FILEIO f;
    if (f.create(s)) {
      msg.error("Unable to create ROM dump %s", s);
      fprintf(stderr, "[DUMP] create failed: '%s'\n", s);
      fflush(stderr);
      return;
    }
    if (f.write(ROM[i], sizeof(ROMPAGE))) {
      f.close();
      msg.error("Unable to write ROM dump %s", s);
      fprintf(stderr, "[DUMP] write failed: '%s'\n", s);
      fflush(stderr);
      return;
    }
    f.close();
  }
  msg.printf(1, "ROM dumped to %s.Rxx", romfilename);
}

//---------------------------------------------------------
// Battery backed memory

BATTERYMEM::BATTERYMEM() { memset(data, 0, 0x2000); }

void BATTERYMEM::read(char *romfilename) {
  char s[64];
  strcpy(s, romfilename);
  strcat(s, ".sav");

  // load savegame
  static FILEIO f;
  if (!f.open(s)) // try and open savegame...
  {
    if (f.read(data, 0x2000)) {
      f.close();
      msg.error("Unable to read save game %s", s);
      fprintf(stderr, "[SAVE] read failed: '%s'\n", s);
      fflush(stderr);
      return;
    }
    f.close();
    msg.printf(1, "Save game loaded from %s", s);
  }
}

void BATTERYMEM::write(char *romfilename) {
  char s[64];
  strcpy(s, romfilename);
  strcat(s, ".sav");

  // save savegame
  FILEIO f;
  if (f.create(s)) {
    msg.error("Unable to create save game %s", s);
    fprintf(stderr, "[SAVE] create failed: '%s'\n", s);
    fflush(stderr);
    return;
  }
  if (f.write(data, 0x2000)) {
    f.close();
    msg.error("Unable to write save game %s", s);
    fprintf(stderr, "[SAVE] write failed: '%s'\n", s);
    fflush(stderr);
    return;
  }
  f.close();
  msg.printf(1, "Save game saved to %s", s);
}

void BATTERYMEM::copyfromRAM() {
  if (!ram)
    return;
  memcpy(data, &ram[0x6000], 0x2000);
}

void BATTERYMEM::copytoRAM() {
  if (!ram)
    return;
  memcpy(&ram[0x6000], data, 0x2000);
}

//---------------------------------------------------------
// NES rom header

char *getmmctypestr(int type);

int NESROMHEADER::validate() {
  return (str[0] == 'N' && str[1] == 'E' && str[2] == 'S');
}
void NESROMHEADER::printinfo() {
  msg.printf(1, "16K ROM banks: %d", num16k);
  msg.printf(1, "8K VROM banks: %d", num8k);

  msg.printf(1, "%s mirroring", mirroring ? "Vertical" : "Horizontal");
  if (battery)
    msg.printf(1, "Battery backed RAM");
  if (trainer)
    msg.printf(1, "Trainer");
  if (fourscreen)
    msg.printf(1, "4 Screen VRAM");

  msg.printf(1, "Mapper#%d: %s", banktype, getmmctypestr(banktype));
}

void m_writeromheader() {
  if (!romloaded) {
    msg.error("ROM not loaded");
    return;
  }
  if (!nv)
    return;

  // put mirroring in rom
  romhdr.mirroring = nv->mirroring;

  char s[64];
  strcpy(s, romfilename);
  strcat(s, ".nes");

  FILEIO f;
  if (f.open(s)) {
    msg.error("Unable to open ROM header target %s", s);
    return;
  }
  if (f.write(&romhdr, sizeof(romhdr))) {
    f.close();
    msg.error("Unable to write ROM header %s", s);
    return;
  }
  f.close();
  msg.printf(1, "NES ROM header written to %s", s);
}

extern COLOR nespal[256];

void m_savepalette() {
  if (!romloaded) {
    msg.error("ROM not loaded");
    return;
  }
  if (!nv)
    return;
  FILEIO f;
  if (f.create("aNES.pal")) {
    msg.error("Unable to create aNES.pal");
    return;
  }
  if (f.write(nespal, 256 * sizeof(COLOR))) {
    f.close();
    msg.error("Unable to write aNES.pal");
    return;
  }
  f.close();
  msg.printf(1, "NES Palette saved to aNES.pal");
}

//----------------------------------------------------
// loading and saving game state

struct m6502regs {
  word pc;
  word af;
  byte x;
  byte y;
  byte s;
  void save() {
    pc = m6502pc;
    af = m6502af;
    x = m6502x;
    y = m6502y;
    s = m6502s;
  }
  void restore() {
    m6502pc = pc;
    m6502af = af;
    m6502x = x;
    m6502y = y;
    m6502s = s;
  }
};

class nesstate {
  // cpu stuff
  byte ram6000[0x2000]; // battery backed ram
  byte lowram[0x800];   // low ram
  m6502regs regs;       // processor regs

  // ppu stuff
  NES_sprite spmem[64];   // sprite memory
  NES_patterntable pt[2]; // 2 pattern tables
  NES_natable nat[4];     // 4 name/attribute tables
  NES_palette bgpal;
  NES_palette spritepal;

  // ports and stuff
  byte port2000[0x10];
  byte port4000[0x20];

  // mmc stuff
  char mmcstate[32]; // memory to store mmc info

  // last screen context
  scontext lastsc;
  byte mirroring; // last mirroring

public:
  void save() {
    memcpy(ram6000, ram + 0x6000, 0x2000); // copy battery mem
    memcpy(lowram, ram, 0x800);            // copy lowram
    regs.save();
    memcpy(spmem, spritemem, 64 * sizeof(NES_sprite));
    memcpy(pt, ppu->pt, 2 * sizeof(NES_patterntable));
    bgpal = ppu->bgpal;
    spritepal = ppu->spritepal;
    memcpy(port2000, ram + 0x2000, 0x10);
    memcpy(port4000, ram + 0x4000, 0x20);
    port4000[0x15] = ns->enablebits;
    mmc->save(mmcstate);

    mirroring = nv->mirroring;
    lastsc = nv->sc.c; // store last screen context
    for (int i = 0; i < 4; i++)
      nat[i] = nv->ntc[i]->nt;
  };
  void restore() {
    memcpy(ram + 0x6000, ram6000, 0x2000); // copy battery mem
    memcpy(ram, lowram, 0x800);            // copy lowram
    regs.restore();
    memcpy(spritemem, spmem, 64 * sizeof(NES_sprite));
    memcpy(ppu->pt, pt, 2 * sizeof(NES_patterntable));
    for (int i = 0; i < 8; i++)
      nv->pt1kupdated[i] = 1;
    ppu->bgpal = bgpal;
    ppu->spritepal = spritepal;
    memcpy(ram + 0x2000, port2000, 0x10);
    memcpy(ram + 0x4000, port4000, 0x20);
    ns->setenable(port4000[0x15]);
    mmc->restore(mmcstate);

    nv->setmirroring(mirroring);
    for (i = 0; i < 4; i++)
      nv->ntc[i]->nt = nat[i];
    nv->sc.c = lastsc;
    nv->sc.c.ptn[0].restore();
    nv->sc.c.ptn[1].restore();
  };

  // read/write state to file
  void write(FILEIO &f) { f.write(this, sizeof(nesstate)); }
  void read(FILEIO &f) { f.read(this, sizeof(nesstate)); }
};

void m_savestate() {
  if (!romloaded) {
    msg.error("ROM not loaded");
    return;
  }
  if (!nv)
    return;

  char s[sizeof(romfilename) + 4];
  if (!nesticle_make_state_path(romfilename, s, sizeof(s))) {
    msg.error("Unable to create state filename for %s", romfilename);
    return;
  }

  FILEIO f;
  if (f.create(s)) {
    msg.error("Unable to create %s", s);
    return;
  }
  nesstate *state = new nesstate; // create nes state
  state->save();                  // record nes state
  nesticle_state_header hdr;
  nesticle_init_state_header(hdr, sizeof(nesstate),
                             (unsigned char)romhdr.banktype);
  if (f.write(&hdr, sizeof(hdr)) || f.write(state, sizeof(nesstate))) {
    delete state;
    f.close();
    msg.error("Unable to write game state %s", s);
    return;
  }
  delete state;                   // free state
  f.close();

  msg.printf(1, "Game state saved to %s", s);
}

void m_resume();
void m_loadstate() {
  if (!romloaded) {
    msg.error("ROM not loaded");
    return;
  }
  if (!nv)
    return;

  char s[sizeof(romfilename) + 4];
  if (!nesticle_make_state_path(romfilename, s, sizeof(s))) {
    msg.error("Unable to open state filename for %s", romfilename);
    return;
  }

  FILEIO f;
  if (f.open(s)) {
    msg.error("Unable to open %s", s);
    return;
  }

  nesticle_state_header hdr;
  memset(&hdr, 0, sizeof(hdr));
  unsigned filesize = f.size();
  if (f.read(&hdr, sizeof(hdr)) ||
      !nesticle_valid_state_header(hdr, filesize,
                                   (unsigned char)romhdr.banktype,
                                   sizeof(nesstate))) {
    msg.error("Incompatible or corrupt state file %s", s);
    fprintf(stderr,
            "[STATE] rejected '%s': size=%u version=%u payload=%u mapper=%u "
            "current_mapper=%u\n",
            s, filesize, hdr.version, hdr.payload_size, hdr.mapper,
            (unsigned)(unsigned char)romhdr.banktype);
    fflush(stderr);
    f.close();
    return;
  }

  nesstate *state = new nesstate; // create nes state
  if (f.read(state, sizeof(nesstate))) {
    delete state;
    f.close();
    msg.error("Unable to read game state %s", s);
    return;
  }
  msg.printf(1, "Game state restored from %s", s);

  resetNEShardware();
  state->restore(); // restore nes state!
  nv->reset();      // reset nes video
  CPURunning = 0;
  CPUpaused = 1;
  m_resume();

  delete state; // free state
  f.close();
}
