#ifdef __clang__
#pragma clang diagnostic ignored "-Wwritable-strings"
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

#include "dd.h"
#include "font.h"
#include "r2img.h"

#include "gui.h"
#include "guicolor.h"
#include "guimenu.h"
#include "message.h"
#include "mouse.h"

#include "keyb.h"

#include "config.h"

#include "guivol.h"

//----------------------
//  GUI messagebox
//----------------------
#define MSGYW 8 // height in lines

class GUImessage : public GUIcontents {
  class msgbuffer *m;
  GUIvscrollbar *scroll;

  struct WrapLine {
    int msg_idx;
    int offset;
    int len;
  };
  WrapLine wlines[MAXMSG * 5];
  int num_wlines;
  int last_width;
  int last_updated;

public:
  void wrap_all() {
    num_wlines = 0;
    int max_w = width() - 16;
    if (max_w < 10)
      max_w = 10;

    for (int i = 0; i < m->num; i++) {
      char *s = m->msg[i].s;
      if (!s)
        continue;
      int start = 0;
      while (s[start]) {
        int w = 0, c = start, last_space = -1;
        while (s[c] && w < max_w) {
          if (s[c] == ' ')
            last_space = c;
          w += font[(int)m->msg[i].color]->getwidth(s[c]);
          if (w >= max_w)
            break;
          c++;
        }
        if (!s[c]) {
          wlines[num_wlines++] = {i, start, c - start};
          break;
        } else {
          if (last_space > start)
            c = last_space;
          else if (c == start)
            c++;
          wlines[num_wlines++] = {i, start, c - start};
          start = c;
          if (s[start] == ' ')
            start++;
        }
        if (num_wlines >= MAXMSG * 5)
          break;
      }
      if (num_wlines >= MAXMSG * 5)
        break;
    }
    last_width = width();
    last_updated = m->updated;
  }

  void setrange() {
    wrap_all();
    int visible_lines = height() / 10;
    int yw = num_wlines - visible_lines;
    if (yw < 0)
      yw = 0;
    scroll->setrange(0, yw);
    scroll->setpos(yw);
    m->updated = 0;
  }

  GUImessage(class msgbuffer *tm) : GUIcontents(261, MSGYW * 10) {
    m = tm;
    last_width = 0;
    last_updated = 0;
    scroll = new GUIvscrollbar(this, x1 + width() - 12, y1, height());
    setrange();
  }

  virtual void resize(int xw, int yw) {
    GUIcontents::resize(xw, yw);
    if (scroll) {
      scroll->moveto(x1 + xw - 12, y1);
      scroll->resize(12, yw);
    }
  }

  virtual void draw(char *dest) {
    if (m->updated || width() != last_width) {
      setrange();
    }
    fill(CLR_BOX);

    int pos = scroll->getpos();
    int draw_y = y1;
    int visible_lines = height() / 10;

    for (int i = 0; i < visible_lines && (pos + i) < num_wlines; i++) {
      WrapLine &wl = wlines[pos + i];
      MESSAGE &msg = m->msg[wl.msg_idx];
      if (msg.s) {
        char temp[256];
        int len = wl.len;
        if (len > 255)
          len = 255;
        strncpy(temp, msg.s + wl.offset, len);
        temp[len] = 0;
        font[msg.color]->draw(temp, dest, x1 + 2, draw_y);
      }
      draw_y += 10;
    }

    GUIrect::draw(dest);
  }
  virtual char *getname() { return "message"; }
  virtual ~GUImessage() {}
};

GUIrect *newguimessage(msgbuffer *tm) { return new GUImessage(tm); }

class systemmessages : public GUImessage {
public:
  systemmessages(msgbuffer *tm) : GUImessage(tm) {}
  // open functions
  static DLGPOS pos; // saved last position of dialog
  static void open() {
    pos.open(new GUIbox(guiroot, "Messages", new systemmessages(&msg), 0, 0));
    //   cfg->set(CFG_SHOWMESSAGE,1);
  };
  virtual ~systemmessages() {
    pos.close((GUIbox *)parent);
  } // cfg->set(CFG_SHOWMESSAGE,0);}
};
DLGPOS systemmessages::pos;

//--------------------
//   about box
//----------------------
extern char appname[];
extern char buildtime[], builddate[], buildcompiler[];
extern int buildcompilerversionhigh, buildcompilerversionlow;
extern float version; // game version
class aboutdlg : public GUIcontents {
public:
  aboutdlg() : GUIcontents(500, 90) {
    char s[80];

    new GUIstaticimage(this, guivol.about, 5, 5);
    new GUIstatictext(this, 2, "Buddy says:", 140, 5);

    sprintf(s, "%s %s", appname,
#ifdef WIN95
            "Win95"
#else
            "DOS"
#endif
    );

    new GUIstatictext(this, 3, s, 140, 25);

    sprintf(s, "V%d.%02d", (unsigned)version,
            ((unsigned)(version * 100)) % 100);
    new GUIstatictext(this, 5, s, 420, 25);

    sprintf(s, "%s %d.%d %s %s", buildcompiler, buildcompilerversionhigh,
            buildcompilerversionlow, builddate, buildtime);
    new GUIstatictext(this, 3, s, 140, 40);

    new GUIstatictext(this, 3, "Copyright (C) 1997 Bloodlust Software", 140, 60);
  }

  virtual int sendmessage(GUIrect *c, int guimsg) { return 1; };
  virtual int acceptfocus() { return 1; }
  virtual char *getname() { return "aboutdlg"; }
  virtual void draw(char *dest) {
    fill(CLR_BOX);
    GUIrect::draw(dest);
  };

  // open functions
  static DLGPOS pos; // saved last position of dialog
  static void open() {
    pos.open(new GUIonebuttonbox(guiroot, "About", new aboutdlg(),
                                 "Thanks Buddy!", 0, 0));
  };
  virtual ~aboutdlg() { pos.close((GUIbox *)parent); }
};
DLGPOS aboutdlg::pos;

//--------------------------------------
void disablegui();

void cleardesktop();

#include "nes.h"
#include "nesvideo.h"

void setviewportscale(int scale) {
  if (!nv || !nv->parent)
    return;
  GUImaximizebox *box = (GUImaximizebox *)nv->parent;
  if (box->maximized)
    box->restore();
  int box_w = 256 * scale + 4;
  int box_h = 224 * scale + 16;
  // Clamp to screen bounds
  if (box_w > SCREENX) box_w = SCREENX;
  if (box_h > SCREENY) box_h = SCREENY;
  // Move box so it stays within screen
  int bx = box->x1, by = box->y1;
  if (bx + box_w > SCREENX) bx = SCREENX - box_w;
  if (by + box_h > SCREENY) by = SCREENY - box_h;
  if (bx < 0) bx = 0;
  if (by < 0) by = 0;
  box->moveto(bx, by);
  box->resize(box_w, box_h);
  if (nv) {
    nv->resize(box_w - 4, box_h - 16);
  }
  box->reposclosebutton();
  cleardesktop();
  msg.printf(1, "Viewport scale: %dx", scale);
}

void scale1x() { setviewportscale(1); }
void scale2x() { setviewportscale(2); }
void scale3x() { setviewportscale(3); }

void m_flipmirror() {
  if (!nv)
    return;
  nv->setmirroring(nv->mirroring ^ 1);
  msg.printf(1, "Mirroring set to %s",
             nv->mirroring ? "Vertical" : "Horizontal");
}

void m_heapcheck() {
#ifdef _MSC_VER
  int status = _heapchk();
  switch (status) {
  case _HEAPBADNODE:
    msg.printf(5, "ERROR: Bad heap node");
    return;
  case _HEAPEMPTY:
    msg.printf(5, "ERROR: Heap empty");
    return;
  case _HEAPOK:
    msg.printf(1, "Heap OK");
    return;
  };
#else
  switch (_heapset(0x69)) {
  case _HEAPBADNODE:
    msg.printf(5, "ERROR: Bad heap node");
    return;
  case _HEAPEMPTY:
    msg.printf(5, "ERROR: Heap empty");
    return;
  case _HEAPOK:
    msg.printf(1, "Heap OK");
    return;
  };
#endif
}

extern char waitvsync;
void m_waitvsync() {
  msg.printf(1, "Wait Vsync %s", (waitvsync ^= 1) ? "on" : "off");
}

void m_showfps() {
  msg.printf(1, "Show FPS %s", cfg->toggle(CFG_SHOWFPS) ? "on" : "off");
}

extern char configfile[];
void m_saveconfig() { cfg->save(configfile); }

void editinputdevice0();
void editinputdevice1();

#ifdef WIN95
void ddrawinfo();
void m_ddrawinfo() {
  systemmessages::open();
  ddrawinfo();
}
#endif

void m_quit() { quitgame(); }

GUImenu *gmenu = 0;
int guienabled = 0;
void enablegui() {
  if (guienabled)
    return;
  guienabled = 1;
  cleardesktop();
}

void disablegui() {
  if (!guienabled)
    return;
  GUIrect::setfocus(gmenu);
  guienabled = 0;
  cleardesktop();
}

void togglegui() {
  if (guienabled)
    disablegui();
  else
    enablegui();
}

void m_showmessages() {
  if (!systemmessages::pos.opened)
    systemmessages::open();
}

void m_load();

// void m_execute();
// void m_stop();
void m_reset();
void m_free();
void m_getrominfo();

void m_ramdump();
void m_romdump();
void m_pause();
void m_resume();
void m_patternview();
void m_nametableview();
void m_paletteview();
void m_spritedump();
void m_advanceframe();

void m_writeromheader();

void m_savepalette();
void m_loadstate();
void m_savestate();
void m_step();

byte showvframeinfo = 0, showcpuutilization;

void m_showvframeinfo() { showvframeinfo ^= 1; }
void m_showcpuuse() { showcpuutilization ^= 1; }

void m_nestimingview();
void m_snapshot() {
  if (nv)
    nv->dotakesnapshot = 1;
};

void togglesound();
void m_waveoutputview();

//------------

//*************************************
//           menu definition
//*************************************

menu filemenu = {{
    {"Load ROM...", m_load, 'l', 0},
    {"Free ROM", m_free, 0, 0},
    {"-----", 0, 0, 0},
    {"Get ROM info", m_getrominfo, 0, 0},
    {"Write NES header", m_writeromheader, 0, 0},
    {"-----", 0, 0, 0},
    {"Save PCX snapshot", m_snapshot, (char)(KB_F9 | 0x80), 0},
    {"Save Palette", m_savepalette, 0, 0},
    {"-----", 0, 0, 0},
    {"Exit", m_quit, 'q', 0},
    {0, 0, 0, 0},
}};

menu NESmenu = {{
    //  {"Execute",m_execute,'e',0},
    //  {"Stop",m_stop,'s',0},
    {"Reset", m_reset, 'e', 0},
    {"-----", 0, 0, 0},
    {"Pause", m_pause, 'p', 0},
    {"Resume", m_resume, 'r', 0},
    {"Advance frame", m_advanceframe, 'a', 0},
    {"Trace", m_step, 't', 0},
    {"-----", 0, 0, 0},
    {"Save NES state", m_savestate, (char)(KB_F5 | 0x80), 0},
    {"Restore NES state", m_loadstate, (char)(KB_F7 | 0x80), 0},
    {0, NULL, 0, 0},
}};

menu viewmenu = {{
    {"Messages", m_showmessages, (char)(KB_F1 | 0x80), 0},
    {"-----", 0, 0, 0},
    {"Pattern tables", m_patternview, (char)(KB_F2 | 0x80), 0},
    {"Name tables", m_nametableview, (char)(KB_F3 | 0x80), 0},
    {"Palette", m_paletteview, (char)(KB_F4 | 0x80), 0},
    {"Wave output", m_waveoutputview, 0, 0},
    {0, NULL, 0, 0},
}};

menu resmenu = {{
    {"1x (256x224)", scale1x, 0, 0},
    {"2x (512x448)", scale2x, 0, 0},
    {"3x (768x672)", scale3x, 0, 0},
    {0, NULL, 0, 0},
}};

menu redefineinputmenu = {{
    {"Device 1", editinputdevice0, 0, 0},
    {"Device 2", editinputdevice1, 0, 0},
    {0, NULL, 0, 0},
}};

menu settingsmenu = {{
    {"Redefine input", 0, 0, &redefineinputmenu},
    {"Resolution", 0, 0, &resmenu},
    {"Toggle sound", togglesound, (char)(KB_F8 | 0x80), 0},
    {"Flip mirroring", m_flipmirror, 'f', 0},
    {"NES Timing...", m_nestimingview, 0, 0},

    {"-----", 0, 0, 0},
    {"Save config", m_saveconfig, 0, 0},
    {0, NULL, 0, 0},
}};

menu miscmenu = {{
    {"Hide GUI", disablegui, 0, 0},
    {"Show FPS", m_showfps, 0, 0},
    {"Show CPU use", m_showcpuuse, 0, 0},
    {"Wait Vsync", m_waitvsync, 0, 0},
    {"Heap check", m_heapcheck, 0, 0},
    {"Dump RAM", m_ramdump, 0, 0},
    {"Dump ROMs", m_romdump, 0, 0},
    {"Show Vframe info", m_showvframeinfo, 0, 0},
#ifdef WIN95
    {"Directdraw Info", m_ddrawinfo, 0, 0},
#endif
    {"-----", 0, 0, 0},
    {"About...", aboutdlg::open, 0, 0},
    {0, NULL, 0, 0},
}};

menu mainmenu = {{
    {"File", 0, 0, &filemenu},
    {"CPU", 0, 0, &NESmenu},
    {"View", 0, 0, &viewmenu},
    {"Settings", 0, 0, &settingsmenu},
    {"Misc", 0, 0, &miscmenu},
    {0, NULL, 0, 0},
}};

void initdefaultgui() {
  gmenu = new GUIhmenu(guiroot, &mainmenu, 0, 0);
  fprintf(stderr, "[GUI] root=%p guiroot=%p gmenu=%p\n", (void *)root,
          (void *)guiroot, (void *)gmenu);
  fprintf(stderr, "[GUI] guiroot bounds: (%d,%d)-(%d,%d)\n", guiroot->x1,
          guiroot->y1, guiroot->x2, guiroot->y2);
  fprintf(stderr, "[GUI] gmenu bounds: (%d,%d)-(%d,%d)\n", gmenu->x1, gmenu->y1,
          gmenu->x2, gmenu->y2);
  fflush(stderr);
  enablegui();
}
