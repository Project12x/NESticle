// gui_impl.cpp - Reconstructed GUI system implementation for NESticle
// The original GUI implementation was never included in the source release.

// Define GUI_DEBUG to enable widget coordinate/lifecycle logging to stderr
#define GUI_DEBUG
#ifdef GUI_DEBUG
#define GUI_LOG(...)                                                           \
  do {                                                                         \
    fprintf(stderr, "[GUI] ");                                                 \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "\n");                                                     \
    fflush(stderr);                                                            \
  } while (0)
#else
#define GUI_LOG(...) ((void)0)
#endif
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dd.h"
#include "font.h"
#include "gui.h"
#include "guicolor.h"
#include "guimenu.h"
#include "guivol.h"
#include "keyb.h"
#include "message.h"
#include "mouse.h"
#include "r2img.h"
#include "types.h"

// ---- Externals ----
extern int SCREENX, SCREENY;
extern int PITCH;
extern class FONT *font[];

// ---- GUI definition position tracking ----
int GUIdefx = 30, GUIdefy = 30;
static int GUIdefxstart = 30;

void nextGUIdef() {
  GUIdefx += 16;
  GUIdefy += 16;
  if (GUIdefx > SCREENX - 100) {
    GUIdefx = GUIdefxstart;
    GUIdefy = 30;
  }
}

void resetGUIdef() {
  GUIdefx = 30;
  GUIdefy = 30;
}

// ---- GUIrect base class ----
GUIrect::GUIrect(GUIrect *p, int tx1, int ty1, int tx2, int ty2) {
  parent = 0;
  child = lastchild = 0;
  prev = next = 0;
  focus = 0;
  lastfocus = 0;
  x1 = tx1;
  y1 = ty1;
  x2 = tx2;
  y2 = ty2;
  if (p)
    link(p);
  GUI_LOG("new GUIrect name=%s coords=(%d,%d)-(%d,%d) parent=%s", getname(), x1,
          y1, x2, y2, p ? p->getname() : "none");
}

void GUIrect::link(GUIrect *p) {
  parent = p;
  prev = 0;
  next = p->child;
  if (p->child)
    p->child->prev = this;
  else
    p->lastchild = this;
  p->child = this;
}

void GUIrect::unlink() {
  if (!parent)
    return;
  if (prev)
    prev->next = next;
  else
    parent->child = next;
  if (next)
    next->prev = prev;
  else
    parent->lastchild = prev;
  prev = next = 0;
  parent = 0;
}

GUIrect::~GUIrect() {
  while (child)
    delete child;
  // Clear parent's lastfocus if it points to us to prevent dangling pointer
  if (parent && parent->lastfocus == this)
    parent->lastfocus = 0;
  unlink();
}

GUIrect *GUIrect::hittest(int x, int y) {
  if (x < x1 || x >= x2 || y < y1 || y >= y2)
    return 0;
  for (GUIrect *c = child; c; c = c->next) {
    GUIrect *h = c->hittest(x, y);
    if (h)
      return h;
  }
  return this;
}

void GUIrect::bringtofront() {
  if (!parent || parent->child == this)
    return;
  GUIrect *p = parent;
  unlink();
  link(p);
}

void GUIrect::sendtoback() {
  if (!parent)
    return;
  GUIrect *p = parent;
  unlink();
  // link at end
  parent = p;
  prev = p->lastchild;
  next = 0;
  if (p->lastchild)
    p->lastchild->next = this;
  else
    p->child = this;
  p->lastchild = this;
}

void GUIrect::moverel(int dx, int dy) {
  x1 += dx;
  y1 += dy;
  x2 += dx;
  y2 += dy;
  for (GUIrect *c = child; c; c = c->next)
    c->moverel(dx, dy);
}

void GUIrect::moveto(int x, int y) { moverel(x - x1, y - y1); }

void GUIrect::fill(char color) {
  extern char *screen;
  if (!screen)
    return;
  for (int y = y1; y < y2; y++) {
    if (y < 0 || y >= SCREENY)
      continue;
    for (int x = x1; x < x2; x++) {
      if (x < 0 || x >= SCREENX)
        continue;
      screen[y * PITCH + x] = color;
    }
  }
}

void GUIrect::outline(char color) {
  extern char *screen;
  if (!screen)
    return;
  drawhline(screen, color, x1, y1, x2);
  drawhline(screen, color, x1, y2 - 1, x2);
  drawvline(screen, color, x1, y1, y2);
  drawvline(screen, color, x2 - 1, y1, y2);
}

void GUIrect::reparent(GUIrect *p) {
  unlink();
  if (p)
    link(p);
}

void GUIrect::draw(char *dest) {
  for (GUIrect *c = lastchild; c; c = c->prev)
    c->draw(dest);
}

int GUIrect::keyhit(char scan, char key) {
  if (lastfocus && lastfocus->focus)
    return lastfocus->keyhit(scan, key);
  for (GUIrect *c = child; c; c = c->next)
    if (c->keyhit(scan, key))
      return 1;
  return 0;
}

GUIrect *GUIrect::modal = 0;
void GUIrect::setmodal(GUIrect *m) { modal = m; }

int GUIrect::setfocus(GUIrect *f) {
  if (!f)
    return 0;
  GUIrect *p = f;
  while (p) {
    if (p->parent) {
      if (p->parent->lastfocus && p->parent->lastfocus != p)
        p->parent->lastfocus->losefocus();
      p->parent->lastfocus = p;
      p->parent->focus = 1;
    }
    p->focus = 1;
    p = p->parent;
  }
  f->receivefocus();
  return 1;
}

void GUIrect::receivefocus() { focus = 1; }
void GUIrect::losefocus() {
  focus = 0;
  losechildfocus();
}
void GUIrect::losechildfocus() {
  for (GUIrect *c = child; c; c = c->next) {
    c->focus = 0;
    c->losechildfocus();
  }
  lastfocus = 0;
}

void GUIrect::cyclefocus(int dir) {
  GUIrect *start = lastfocus;
  GUIrect *c = start ? (dir > 0 ? start->next : start->prev) : child;
  if (!c)
    c = dir > 0 ? child : lastchild;
  GUIrect *first = c;
  while (c) {
    if (c->acceptfocus()) {
      setfocus(c);
      return;
    }
    c = dir > 0 ? c->next : c->prev;
    if (!c)
      c = dir > 0 ? child : lastchild;
    if (c == first)
      break;
  }
}

// ---- GUIcontents ----
void GUIcontents::resize(int xw, int yw) { GUIrect::resize(xw, yw); }
int GUIcontents::keyhit(char scan, char key) {
  return GUIrect::keyhit(scan, key);
}

// ROOT is implemented in MAIN.CPP

// ---- GUIroot (constructor/destructor) ----
extern GUIroot *guiroot;
GUIroot::GUIroot(ROOT *p) : GUIrect(p, 0, 0, SCREENX, SCREENY) {
  guiroot = this;
}
GUIroot::~GUIroot() {}
int GUIroot::keyhit(char scan, char key) { return GUIrect::keyhit(scan, key); }

// ---- GUIbar ----
void GUIbar::draw(char *d) {
  fill(color);
  GUIrect::draw(d);
}

// ---- GUIstatictext ----
GUIstatictext::GUIstatictext(GUIrect *p, int f, char *str, int x, int y)
    : GUIrect(p, x, y, x + 200, y + 10) {
  fontnum = f;
  strncpy(text, str, 79);
  text[79] = 0;
  if (font[fontnum])
    x2 = x1 + font[fontnum]->getwidth(text);
}
void GUIstatictext::draw(char *dest) {
  if (font[fontnum])
    font[fontnum]->draw(text, dest, x1, y1);
}
void GUIstatictext::settext(char *s) {
  strncpy(text, s, 79);
  text[79] = 0;
  if (font[fontnum])
    x2 = x1 + font[fontnum]->getwidth(text);
}

// ---- GUIstaticcenteredtext ----
void GUIstaticcenteredtext::draw(char *dest) {
  if (font[fontnum])
    font[fontnum]->drawcentered(text, dest, x1 + width() / 2, y1);
}

// ---- GUIstaticimage ----
GUIstaticimage::GUIstaticimage(GUIrect *p, struct IMG *i, int x, int y)
    : GUIrect(p, x, y, x + (i ? i->xw : 16), y + (i ? i->yw : 16)) {
  img = i;
}
void GUIstaticimage::draw(char *dest) {
  if (img)
    img->draw(dest, x1, y1);
}

// ---- GUIbutton ----
GUIbutton::GUIbutton(GUIrect *p, int x, int y, int xw, int yw)
    : GUIrect(p, x, y, x + xw, y + yw) {
  depressed = 0;
}
GUIrect *GUIbutton::click(mouse &m) {
  depressed = 1;
  return this;
}
int GUIbutton::release(mouse &m) {
  if (depressed) {
    depressed = 0;
    if (parent)
      parent->sendmessage(this, GUIMSG_PUSHED);
  }
  return 1;
}
void GUIbutton::draw(char *dest) {
  if (depressed) {
    drawrect(dest, CLR_BUTTONOUTERDOWN, x1, y1, width(), height());
    drawrect(dest, CLR_BUTTONINNERDOWN, x1 + 1, y1 + 1, width() - 2,
             height() - 2);
  } else {
    drawrect(dest, CLR_BUTTONOUTER, x1, y1, width(), height());
    drawrect(dest, CLR_BUTTONINNER, x1 + 1, y1 + 1, width() - 2, height() - 2);
  }
  GUIrect::draw(dest);
}

// ---- GUItextbutton ----
GUItextbutton::GUItextbutton(GUIrect *p, char *s, int x, int y)
    : GUIbutton(p, x, y, 50, 14) {
  strncpy(text, s, 79);
  text[79] = 0;
  bwidth = font[FNT_TEXTBUTTON] ? font[FNT_TEXTBUTTON]->getwidth(text) + 8 : 50;
  x2 = x1 + bwidth;
}
void GUItextbutton::settext(char *s) {
  strncpy(text, s, 79);
  text[79] = 0;
  bwidth = font[FNT_TEXTBUTTON] ? font[FNT_TEXTBUTTON]->getwidth(text) + 8 : 50;
  x2 = x1 + bwidth;
}
void GUItextbutton::draw(char *dest) {
  GUIbutton::draw(dest);
  int fn = depressed ? FNT_TEXTBUTTONDOWN : FNT_TEXTBUTTON;
  if (font[fn])
    font[fn]->draw(text, dest, x1 + 4, y1 + 3);
}

// ---- GUIimagebutton ----
GUIimagebutton::GUIimagebutton(GUIrect *p, struct IMG *i, int x, int y)
    : GUIbutton(p, x, y, i ? i->xw + 4 : 16, i ? i->yw + 4 : 16) {
  img = i;
}
void GUIimagebutton::draw(char *dest) {
  GUIbutton::draw(dest);
  if (img)
    img->draw(dest, x1 + 2, y1 + 2);
}

// ---- GUIcheckbox ----
GUIcheckbox::GUIcheckbox(GUIrect *p, char *s, int x, int y, int state)
    : GUIrect(p, x, y, x + 100, y + 10) {
  checked = state;
  depressed = 0;
  strncpy(text, s, 79);
  text[79] = 0;
}
GUIrect *GUIcheckbox::click(mouse &m) {
  depressed = 1;
  return this;
}
int GUIcheckbox::release(mouse &m) {
  if (depressed) {
    depressed = 0;
    checked ^= 1;
    if (parent)
      parent->sendmessage(this, checked ? GUIMSG_CHECKED : GUIMSG_UNCHECKED);
  }
  return 1;
}
int GUIcheckbox::keyhit(char scan, char key) {
  if (key == ' ') {
    checked ^= 1;
    if (parent)
      parent->sendmessage(this, checked ? GUIMSG_CHECKED : GUIMSG_UNCHECKED);
    return 1;
  }
  return 0;
}
void GUIcheckbox::draw(char *dest) {
  drawrect(dest, CLR_CHECKBOXOUTER, x1, y1, 9, 9);
  drawrect(dest, CLR_CHECKBOXINNER, x1 + 1, y1 + 1, 7, 7);
  if (checked && guivol.checkmark)
    guivol.checkmark->draw(dest, x1 + 1, y1 + 1);
  int fn = focus ? FNT_CHECKBOXDOWN : FNT_CHECKBOX;
  if (font[fn])
    font[fn]->draw(text, dest, x1 + 12, y1);
}

// ---- GUIedit ----
GUIedit::GUIedit(GUIrect *p, char *pmpt, int x, int y, int xw)
    : GUIrect(p, x, y, x + xw, y + 12) {
  disabled = 0;
  strncpy(prompt, pmpt, 79);
  prompt[79] = 0;
  promptw = font[FNT_EDITPROMPT] ? font[FNT_EDITPROMPT]->getwidth(prompt) : 0;
  inputmaxw = xw;
}
GUIrect *GUIedit::hittest(int x, int y) {
  return disabled ? 0 : GUIrect::hittest(x, y);
}
GUIrect *GUIedit::click(mouse &m) { return this; }
void GUIedit::draw(char *dest) {
  if (font[FNT_EDITPROMPT])
    font[FNT_EDITPROMPT]->draw(prompt, dest, x1, y1);
  drawrect(dest, CLR_EDITINPUT, x1 + promptw + 2, y1, inputmaxw - promptw - 2,
           height());
  drawdata(dest, x1 + promptw + 4, y1 + 1, inputmaxw - promptw - 6);
  if (focus)
    outline((char)15);
  GUIrect::draw(dest);
}

// ---- GUItextedit ----
GUItextedit::GUItextedit(GUIrect *p, char *pmpt, char *inp, int x, int y,
                         int xw, int maxlen)
    : GUIedit(p, pmpt, x, y, xw) {
  maxinputlen = maxlen;
  strncpy(input, inp ? inp : "", 127);
  input[127] = 0;
  inputlen = (int)strlen(input);
  getinputw();
}
void GUItextedit::drawdata(char *dest, int x, int y, int xw) {
  if (font[FNT_EDITINPUT])
    font[FNT_EDITINPUT]->draw(input, dest, x, y);
}
void GUItextedit::backspace() {
  if (inputlen > 0) {
    input[--inputlen] = 0;
    getinputw();
  }
}
void GUItextedit::addchar(char c) {
  if (inputlen < maxinputlen - 1) {
    input[inputlen++] = c;
    input[inputlen] = 0;
    getinputw();
  }
}
int GUItextedit::isvalidkey(char key) { return (key >= 32 && key < 127); }
int GUItextedit::keyhit(char scan, char key) {
  if (scan == 0x0E) {
    backspace();
    if (parent)
      parent->sendmessage(this, GUIMSG_EDITCHANGED);
    return 1;
  }
  if (isvalidkey(key)) {
    addchar(key);
    if (parent)
      parent->sendmessage(this, GUIMSG_EDITCHANGED);
    return 1;
  }
  return 0;
}
void GUItextedit::paste() {}
void GUItextedit::getinputw() {
  inputw =
      font[FNT_EDITINPUT] ? font[FNT_EDITINPUT]->getwidth(input) : inputlen * 8;
}
void GUItextedit::setinput(char *inp) {
  strncpy(input, inp, 127);
  input[127] = 0;
  inputlen = (int)strlen(input);
  getinputw();
}

// ---- GUInumbertextedit ----
GUInumbertextedit::GUInumbertextedit(GUIrect *p, char *pmpt, int inp, int x,
                                     int y, int xw, int maxlen)
    : GUItextedit(p, pmpt, (char *)"", x, y, xw, maxlen) {
  sprintf(input, "%d", inp);
  inputlen = (int)strlen(input);
  getinputw();
}
int GUInumbertextedit::isvalidkey(char key) { return key >= '0' && key <= '9'; }
int GUInumbertextedit::getstate() { return atoi(input); }

// ---- GUInumberedit ----
GUInumberedit::GUInumberedit(GUIrect *p, char *pmpt, int x, int y, int xw)
    : GUIedit(p, pmpt, x, y, xw) {
  up = new GUIimagebutton(this, guivol.umark, x1 + xw - 10, y1);
  down = new GUIimagebutton(this, guivol.dmark, x1 + xw - 10, y1 + 6);
}
int GUInumberedit::keyhit(char scan, char key) {
  return GUIedit::keyhit(scan, key);
}
void GUInumberedit::draw(char *dest) { GUIedit::draw(dest); }

// ---- GUIintedit ----
GUIintedit::GUIintedit(GUIrect *p, char *pmpt, int x, int y, int xw, int num,
                       int tmin, int tmax)
    : GUInumberedit(p, pmpt, x, y, xw) {
  n = num;
  min = tmin;
  max = tmax;
  clip();
}
void GUIintedit::add(int num) {
  n += num;
  clip();
  if (parent)
    parent->sendmessage(this, GUIMSG_EDITCHANGED);
}
void GUIintedit::minus(int num) {
  n -= num;
  clip();
  if (parent)
    parent->sendmessage(this, GUIMSG_EDITCHANGED);
}
void GUIintedit::clip() {
  if (n < min)
    n = min;
  if (n > max)
    n = max;
}
void GUIintedit::clear() {
  n = 0;
  clip();
}
void GUIintedit::setmax() { n = max; }
void GUIintedit::drawdata(char *dest, int x, int y, int xw) {
  char s[32];
  sprintf(s, "%d", n);
  if (font[FNT_EDITINPUT])
    font[FNT_EDITINPUT]->draw(s, dest, x, y);
}
int GUIintedit::sendmessage(GUIrect *c, int msg) {
  if (msg == GUIMSG_PUSHED) {
    if (c == (GUIrect *)up)
      add(1);
    else if (c == (GUIrect *)down)
      minus(1);
  }
  return 1;
}

// ---- GUIfloatedit ----
GUIfloatedit::GUIfloatedit(GUIrect *p, char *pmpt, int x, int y, int xw,
                           float num, float tmin, float tmax)
    : GUInumberedit(p, pmpt, x, y, xw) {
  n = num;
  min = tmin;
  max = tmax;
  clip();
}
void GUIfloatedit::add(float num) {
  n += num;
  clip();
  if (parent)
    parent->sendmessage(this, GUIMSG_EDITCHANGED);
}
void GUIfloatedit::minus(float num) {
  n -= num;
  clip();
  if (parent)
    parent->sendmessage(this, GUIMSG_EDITCHANGED);
}
void GUIfloatedit::clip() {
  if (n < min)
    n = min;
  if (n > max)
    n = max;
}
void GUIfloatedit::clear() {
  n = 0;
  clip();
}
void GUIfloatedit::setmax() { n = max; }
void GUIfloatedit::drawdata(char *dest, int x, int y, int xw) {
  char s[32];
  sprintf(s, "%.2f", n);
  if (font[FNT_EDITINPUT])
    font[FNT_EDITINPUT]->draw(s, dest, x, y);
}
int GUIfloatedit::sendmessage(GUIrect *c, int msg) {
  if (msg == GUIMSG_PUSHED) {
    if (c == (GUIrect *)up)
      add(0.01f);
    else if (c == (GUIrect *)down)
      minus(0.01f);
  }
  return 1;
}

// ---- GUIbox ----
GUIbox::GUIbox(GUIrect *p, char *titlestr, GUIcontents *c, int x, int y)
    : GUIrect(p, x, y, x + c->width() + 4, y + c->height() + 16) {
  strncpy(title, titlestr, 79);
  title[79] = 0;
  contents = c;
  resizing = 0;
  c->reparent(this);
  c->moverel(2, 14);
  close = new GUIimagebutton(this, guivol.xmark, x1 + width() - 12, y1 + 2);
  GUI_LOG("GUIbox '%s' content at (%d,%d)-(%d,%d) close at (%d,%d)", title,
          c->x1, c->y1, c->x2, c->y2, close->x1, close->y1);
}
void GUIbox::settitle(char *s) {
  strncpy(title, s, 79);
  title[79] = 0;
}
void GUIbox::reposclosebutton() {
  if (close)
    close->moveto(x1 + width() - 12, y1 + 2);
}

GUIrect *GUIbox::click(mouse &m) {
  GUIrect *h = GUIrect::hittest(m.x, m.y);
  if (h && h != this)
    return h->click(m);

  // Hit-test the bottom right corner for resizing (12x12 active area)
  if (m.x >= x2 - 12 && m.y >= y2 - 12) {
    resizing = 1;
    return this;
  }

  return this;
}
int GUIbox::release(mouse &m) {
  resizing = 0;
  return 1;
}
int GUIbox::drag(mouse &m) {
  if (resizing) {
    int dx = m.x - m.oldx;
    int dy = m.y - m.oldy;
    int new_w = width() + dx;
    int new_h = height() + dy;

    // Minimum bounds check
    if (new_w < 64)
      new_w = 64;
    if (new_h < 64)
      new_h = 64;

    resize(new_w, new_h);
    reposclosebutton();
    if (contents) {
      contents->resize(new_w - 4, new_h - 16);
    }
  } else {
    moverel(m.x - m.oldx, m.y - m.oldy);
  }
  return 1;
}

void GUIbox::draw(char *dest) {
  drawrect(dest, CLR_BOX, x1, y1, width(), height());
  drawrect(dest, CLR_BOXTITLE, x1, y1, width(), 12);
  if (font[FNT_BOXTITLE])
    font[FNT_BOXTITLE]->draw(title, dest, x1 + 2, y1 + 2);
  outline(focus ? (char)15 : (char)0);
  GUIrect::draw(dest);
}

int GUIbox::keyhit(char scan, char key) {
  if (scan == 0x01) {
    if (parent)
      parent->sendmessage(this, GUIMSG_CLOSE);
    delete this;
    return 1;
  }
  if (contents)
    return contents->keyhit(scan, key);
  return 0;
}

int GUIbox::sendmessage(GUIrect *c, int msg) {
  if (c == (GUIrect *)close && msg == GUIMSG_PUSHED) {
    delete this;
    return 1;
  }
  if (contents)
    return contents->sendmessage(c, msg);
  return 1;
}

// ---- GUImaximizebox ----
GUImaximizebox::GUImaximizebox(GUIrect *p, char *titlestr, GUIcontents *c,
                               int x, int y)
    : GUIbox(p, titlestr, c, x, y) {
  maximized = 0;
  max = new GUIimagebutton(this, guivol.wmmark, width() - 28, 2);
  reposmaxbutton();
}
void GUImaximizebox::maximize() {
  if (maximized)
    return;
  maximized = 1;
  moveto(0, 0);
  resize(SCREENX, SCREENY);
  if (contents) {
    contents->resize(SCREENX - 4, SCREENY - 16);
    contents->maximize();
  }
  reposclosebutton();
  reposmaxbutton();
}
void GUImaximizebox::restore() {
  if (!maximized)
    return;
  maximized = 0;
  if (contents)
    contents->restore();
  reposclosebutton();
  reposmaxbutton();
}
void GUImaximizebox::reposmaxbutton() {
  if (max)
    max->moveto(x1 + width() - 28, y1 + 2);
}
void GUImaximizebox::draw(char *dest) {
  GUIbox::draw(dest);
}
GUIrect *GUImaximizebox::hittest(int x, int y) {
  return GUIrect::hittest(x, y);
}
int GUImaximizebox::sendmessage(GUIrect *c, int msg) {
  if (c == (GUIrect *)max && msg == GUIMSG_PUSHED) {
    if (maximized)
      restore();
    else
      maximize();
    return 1;
  }
  return GUIbox::sendmessage(c, msg);
}
int GUImaximizebox::keyhit(char scan, char key) {
  if (scan == 0x01 && maximized) { // Escape
    restore();
    return 1;
  }
  return GUIbox::keyhit(scan, key);
}

// ---- GUIonebuttonbox ----
GUIonebuttonbox::GUIonebuttonbox(GUIrect *p, char *str, GUIcontents *c,
                                 char *b1name, int x, int y)
    : GUIbox(p, str, c, x, y) {
  y2 += 18;
  bar = new GUIbar(this, (char)CLR_BOX, 0, height() - 18, width(), 18);
  b1 = new GUItextbutton(bar, b1name, (width() - 50) / 2, height() - 16);
}
void GUIonebuttonbox::resize(int xw, int yw) {
  GUIbox::resize(xw, yw);
  if (bar) {
    bar->moveto(0, yw - 18);
    bar->resize(xw, 18);
    if (b1)
      b1->moveto((xw - b1->width()) / 2, yw - 16);
  }
}
int GUIonebuttonbox::sendmessage(GUIrect *c, int msg) {
  if (c == (GUIrect *)b1 && msg == GUIMSG_PUSHED) {
    if (parent)
      parent->sendmessage(this, GUIMSG_OK);
    delete this;
    return 1;
  }
  return GUIbox::sendmessage(c, msg);
}
int GUIonebuttonbox::keyhit(char scan, char key) {
  if (key == 13 || scan == 0x01) {
    if (parent)
      parent->sendmessage(this, key == 13 ? GUIMSG_OK : GUIMSG_CANCEL);
    delete this;
    return 1;
  }
  return GUIbox::keyhit(scan, key);
}

// ---- GUItwobuttonbox ----
GUItwobuttonbox::GUItwobuttonbox(GUIrect *p, char *str, GUIcontents *c,
                                 char *b1name, char *b2name, int x, int y)
    : GUIbox(p, str, c, x, y) {
  y2 += 18;
  bar = new GUIbar(this, (char)CLR_BOX, 0, height() - 18, width(), 18);
  b1 = new GUItextbutton(bar, b1name, width() / 4 - 25, height() - 16);
  b2 = new GUItextbutton(bar, b2name, 3 * width() / 4 - 25, height() - 16);
}
void GUItwobuttonbox::resize(int xw, int yw) {
  GUIbox::resize(xw, yw);
  if (bar) {
    bar->moveto(0, yw - 18);
    bar->resize(xw, 18);
    if (b1)
      b1->moveto(xw / 4 - b1->width() / 2, yw - 16);
    if (b2)
      b2->moveto(3 * xw / 4 - b2->width() / 2, yw - 16);
  }
}
int GUItwobuttonbox::sendmessage(GUIrect *c, int msg) {
  if (msg == GUIMSG_PUSHED) {
    if (c == (GUIrect *)b1) {
      if (parent)
        parent->sendmessage(this, GUIMSG_B1);
      delete this;
      return 1;
    }
    if (c == (GUIrect *)b2) {
      if (parent)
        parent->sendmessage(this, GUIMSG_B2);
      delete this;
      return 1;
    }
  }
  return GUIbox::sendmessage(c, msg);
}
int GUItwobuttonbox::keyhit(char scan, char key) {
  if (key == 13) {
    if (parent)
      parent->sendmessage(this, GUIMSG_B1);
    delete this;
    return 1;
  }
  if (scan == 0x01) {
    if (parent)
      parent->sendmessage(this, GUIMSG_B2);
    delete this;
    return 1;
  }
  return GUIbox::keyhit(scan, key);
}

// ---- GUIscrollbar ----
GUIscrollbar::GUIscrollbar(GUIrect *p, int x, int y, int xw, int yw)
    : GUIrect(p, x, y, x + xw, y + yw) {
  pos = 0;
  min = 0;
  max = 0;
  up = down = 0;
  track = 0;
}
void GUIscrollbar::clip() {
  if (pos < min)
    pos = min;
  if (pos > max)
    pos = max;
}
void GUIscrollbar::setrange(int tmin, int tmax) {
  min = tmin;
  max = tmax;
  clip();
  if (track)
    track->setthumbsize(max - min);
}
void GUIscrollbar::setpos(int p) {
  pos = p;
  clip();
  if (track)
    track->settrackfrompos(pos, min, max);
  if (parent)
    parent->sendmessage(this, GUIMSG_EDITCHANGED);
}
void GUIscrollbar::setposfromtrack(int tpos, int tlen) {
  if (tlen <= 0)
    return;
  pos = min + (tpos * (max - min)) / tlen;
  clip();
  if (parent)
    parent->sendmessage(this, GUIMSG_EDITCHANGED);
}

void GUIscrollbar::resize(int xw, int yw) { GUIrect::resize(xw, yw); }

void GUIscrollbar::draw(char *dest) {
  drawrect(dest, 0, x1, y1, width(), height());
  GUIrect::draw(dest);
}
int GUIscrollbar::sendmessage(GUIrect *c, int msg) {
  if (msg == GUIMSG_PUSHED) {
    if (c == (GUIrect *)up)
      scrollup();
    else if (c == (GUIrect *)down)
      scrolldown();
  }
  return 1;
}

// ---- GUIvscrollbar ----
GUIvscrollbar::GUIvscrollbar(GUIrect *p, int x, int y, int h)
    : GUIscrollbar(p, x, y, 12, h) {
  up = new GUIimagebutton(this, guivol.umark, x1, y1);
  down = new GUIimagebutton(this, guivol.dmark, x1, y1 + h - 12);
  track = new GUIvtrack(this);
}

void GUIvscrollbar::resize(int xw, int yw) {
  GUIscrollbar::resize(xw, yw);
  if (down)
    down->moveto(x1, y1 + yw - 12);
  if (track)
    track->resize(12, yw - 24);
}

int GUIvscrollbar::keyhit(char scan, char key) {
  if (scan == 0x48) {
    scrollup();
    return 1;
  }
  if (scan == 0x50) {
    scrolldown();
    return 1;
  }
  return 0;
}

// ---- GUIhscrollbar ----
GUIhscrollbar::GUIhscrollbar(GUIrect *p, int x, int y, int w)
    : GUIscrollbar(p, x, y, w, 12) {
  up = new GUIimagebutton(this, guivol.lmark, x1, y1);
  down = new GUIimagebutton(this, guivol.rmark, x1 + w - 12, y1);
  track = new GUIhtrack(this);
}

// void GUIhscrollbar::resize(int xw, int yw) { ... } if needed, but only
// vertical layout fixes requested so far.

int GUIhscrollbar::keyhit(char scan, char key) {
  if (scan == 0x4B) {
    scrollup();
    return 1;
  }
  if (scan == 0x4D) {
    scrolldown();
    return 1;
  }
  return 0;
}

// ---- GUItrack ----
GUItrack::GUItrack(GUIscrollbar *p, int x, int y, int xw, int yw)
    : GUIrect(p, x, y, x + xw, y + yw) {
  thumb = 0;
  tracklen = 0;
  trackpos = 0;
  pscroll = p;
}

void GUItrack::resize(int xw, int yw) { GUIrect::resize(xw, yw); }

void GUItrack::movethumb(int tpos) {
  trackpos = tpos;
  if (trackpos < 0)
    trackpos = 0;
  if (trackpos > tracklen)
    trackpos = tracklen;
  positionthumb();
}
void GUItrack::setthumb() { positionthumb(); }
void GUItrack::settrackfrompos(int pos, int tmin, int tmax) {
  if (tmax <= tmin) {
    trackpos = 0;
  } else {
    trackpos = ((pos - tmin) * tracklen) / (tmax - tmin);
  }
  if (trackpos < 0)
    trackpos = 0;
  if (trackpos > tracklen)
    trackpos = tracklen;
  positionthumb();
}
void GUItrack::draw(char *dest) {
  drawrect(dest, 8, x1, y1, width(), height());
  GUIrect::draw(dest);
}

// ---- GUIvtrack / GUIhtrack ----
GUIvtrack::GUIvtrack(GUIvscrollbar *p)
    : GUItrack(p, p->x1, p->y1 + 12, 12, p->height() - 24) {
  tracklen = height() - 8;
  thumb = new GUIvtrackbutton(this, 8);
}

void GUIvtrack::resize(int xw, int yw) {
  GUItrack::resize(xw, yw);
  tracklen = height() - 8;
  positionthumb();
}

void GUIvtrack::positionthumb() {
  if (thumb)
    thumb->moveto(x1, y1 + trackpos);
}
void GUIvtrack::setthumbsize(int posrange) { (void)posrange; }

GUIhtrack::GUIhtrack(GUIhscrollbar *p)
    : GUItrack(p, p->x1 + 12, p->y1, p->width() - 24, 12) {
  tracklen = width() - 8;
  thumb = new GUIhtrackbutton(this, 8);
}
void GUIhtrack::positionthumb() {
  if (thumb)
    thumb->moveto(x1 + trackpos, y1);
}
void GUIhtrack::setthumbsize(int posrange) { (void)posrange; }

// ---- GUItrackbutton ----
GUItrackbutton::GUItrackbutton(GUItrack *p, int size)
    : GUIbutton(p, p->x1, p->y1, size, size) {
  ptrack = p;
  tpos = 0;
}
GUIvtrackbutton::GUIvtrackbutton(GUIvtrack *p, int size)
    : GUItrackbutton(p, size) {
  x2 = x1 + 12;
}
GUIrect *GUIvtrackbutton::click(mouse &m) {
  tpos = m.y - y1;
  return this;
}
int GUIvtrackbutton::drag(mouse &m) {
  int newpos = m.y - parent->y1 - tpos;
  ptrack->movethumb(newpos);
  GUIscrollbar *sb = (GUIscrollbar *)ptrack->parent;
  sb->setposfromtrack(ptrack->gettrackpos(),
                      ((GUItrack *)ptrack)->height() - 8);
  return 1;
}
GUIhtrackbutton::GUIhtrackbutton(GUIhtrack *p, int size)
    : GUItrackbutton(p, size) {
  y2 = y1 + 12;
}
GUIrect *GUIhtrackbutton::click(mouse &m) {
  tpos = m.x - x1;
  return this;
}
int GUIhtrackbutton::drag(mouse &m) {
  int newpos = m.x - parent->x1 - tpos;
  ptrack->movethumb(newpos);
  GUIscrollbar *sb = (GUIscrollbar *)ptrack->parent;
  sb->setposfromtrack(ptrack->gettrackpos(), ((GUItrack *)ptrack)->width() - 8);
  return 1;
}

// ---- GUIlistbox ----
GUIlistbox::GUIlistbox(GUIrect *p, int x, int y, int xw, int iy, int iheight)
    : GUIrect(p, x, y, x + xw, y + iy * iheight + 2) {
  numitems = 0;
  items = 0;
  sel = -1;
  itemheight = iheight;
  itemv = iy;
  depressed = 0;
  scroll = new GUIvscrollbar(this, x1 + xw - 12, y1, iy * iheight + 2);
  GUI_LOG("GUIlistbox at (%d,%d)-(%d,%d) scroll at (%d,%d)-(%d,%d)", x1, y1, x2,
          y2, scroll->x1, scroll->y1, scroll->x2, scroll->y2);
}
void GUIlistbox::freeitems() {
  if (items) {
    free(items);
    items = 0;
  }
  numitems = 0;
  sel = -1;
}
ITEMPTR *GUIlistbox::resizeitems(int n) {
  numitems = n;
  items = (ITEMPTR *)realloc(items, n * sizeof(ITEMPTR));
  if (n > 0 && sel < 0)
    sel = 0;
  scroll->setrange(0, n > itemv ? n - itemv : 0);
  return items;
}
void GUIlistbox::clearsel() { sel = -1; }
void GUIlistbox::setsel(int s) {
  sel = s;
  if (sel < 0)
    sel = 0;
  if (sel >= numitems)
    sel = numitems - 1;
  if (sel < scroll->getpos())
    scroll->setpos(sel);
  if (sel >= scroll->getpos() + itemv)
    scroll->setpos(sel - itemv + 1);
}

void GUIlistbox::resize(int xw, int yw) {
  GUIrect::resize(xw, yw);
  itemv = (yw - 2) / itemheight;
  if (scroll) {
    scroll->moveto(x1 + xw - 12, y1);
    scroll->resize(12, yw);
    if (items) {
      scroll->setrange(0, numitems > itemv ? numitems - itemv : 0);
    }
  }
}

GUIrect *GUIlistbox::click(mouse &m) {
  GUIrect *h = GUIrect::hittest(m.x, m.y);
  if (h && h != this)
    return h->click(m);
  depressed = 1;
  int item = (m.y - y1) / itemheight + scroll->getpos();
  if (item >= 0 && item < numitems) {
    int oldsel = sel;
    sel = item;
    if (parent)
      parent->sendmessage(this, GUIMSG_LISTBOXSELCHANGED);
    if (oldsel == sel && !dblclick.check())
      if (parent)
        parent->sendmessage(this, GUIMSG_LISTBOXDBLCLICKED);
    dblclick.set(300);
  }
  return this;
}
int GUIlistbox::release(mouse &m) {
  depressed = 0;
  return 1;
}
int GUIlistbox::drag(mouse &m) { return 1; }
void GUIlistbox::draw(char *dest) {
  fill((char)0);
  outline((char)(focus ? 15 : 8));
  drawitems(dest, x1 + 1, y1 + 1);
  GUIrect::draw(dest);
}
int GUIlistbox::keyhit(char scan, char key) {
  if (scan == 0x48 && sel > 0) {
    setsel(sel - 1);
    if (parent)
      parent->sendmessage(this, GUIMSG_LISTBOXSELCHANGED);
    return 1;
  }
  if (scan == 0x50 && sel < numitems - 1) {
    setsel(sel + 1);
    if (parent)
      parent->sendmessage(this, GUIMSG_LISTBOXSELCHANGED);
    return 1;
  }
  if (key == 13) {
    if (parent)
      parent->sendmessage(this, GUIMSG_LISTBOXDBLCLICKED);
    return 1;
  }
  return scroll->keyhit(scan, key);
}

// ---- GUIstringlistbox ----
void GUIstringlistbox::drawitems(char *dest, int x, int y) {
  if (!font[3])
    return;
  int start = scroll->getpos();
  int maxpx = width() - 16; // available text width (minus scrollbar)
  for (int i = 0; i < itemv && (start + i) < numitems; i++) {
    char *s = (char *)items[start + i];
    if (!s)
      continue;
    if (start + i == sel)
      drawrect(dest, 2, x, y + i * itemheight, width() - 14, itemheight);
    // Truncate text to fit within listbox width
    if (font[3]->getwidth(s) > maxpx) {
      char buf[256];
      strncpy(buf, s, sizeof(buf) - 1);
      buf[sizeof(buf) - 1] = 0;
      int len = (int)strlen(buf);
      while (len > 0 && font[3]->getwidth(buf) > maxpx)
        buf[--len] = 0;
      font[3]->draw(buf, dest, x + 2, y + i * itemheight + 1);
    } else {
      font[3]->draw(s, dest, x + 2, y + i * itemheight + 1);
    }
  }
}

// ---- GUImenu ----
GUImenu::GUImenu(GUIrect *p, menu *m, int x, int y)
    : GUIrect(p, x, y, x + 100, y + 12) {
  tmenu = m;
  selmi = 0;
}
GUIrect *GUImenu::click(mouse &m) {
  int sx, sy;
  menuitem *mi = menuhittest(m.x, m.y, sx, sy);
  if (mi)
    selmi = mi;
  return this;
}
int GUImenu::release(mouse &m) {
  if (selmi && selmi->func) {
    selmi->menufunc();
    selmi = 0;
  }
  return 1;
}
int GUImenu::drag(mouse &m) {
  int sx, sy;
  menuitem *mi = menuhittest(m.x, m.y, sx, sy);
  if (mi)
    selmi = mi;
  return 1;
}
void GUImenu::losefocus() { selmi = 0; }
void GUImenu::losechildfocus() {}
int GUImenu::domenuitem(menuitem *i) {
  if (i->func)
    return i->menufunc();
  return 0;
}

// ---- menuitem ----
void menuitem::draw(int x, int y, int xw, int sel) {
  extern char *screen;
  if (!text)
    return;
  if (text[0] == '-') {
    drawhline(screen, 8, x, y + 4, x + xw);
    return;
  }
  if (sel)
    drawrect(screen, CLR_MENUSEL, x, y, xw, height());
  int fn = sel ? FNT_MENUSEL : FNT_MENU;
  if (font[fn])
    font[fn]->draw(text, screen, x + 2, y + 1);
}
int menuitem::width() {
  return font[FNT_MENU] ? font[FNT_MENU]->getwidth(text) + 4 : 50;
}
int menuitem::height() { return (text && text[0] == '-') ? 8 : 10; }
int menuitem::menufunc() {
  if (func) {
    func();
    return 1;
  }
  return 0;
}

// ---- menu ----
int menu::getnumitems() {
  int n = 0;
  while (m[n].text)
    n++;
  return n;
}
int menu::getmaxwidth() {
  int mw = 0;
  for (int i = 0; m[i].text; i++) {
    int w = m[i].width();
    if (w > mw)
      mw = w;
  }
  return mw;
}
int menu::gettotalwidth() {
  int tw = 0;
  for (int i = 0; m[i].text; i++)
    tw += m[i].width();
  return tw;
}
int menu::getmaxheight() {
  int mh = 0;
  for (int i = 0; m[i].text; i++) {
    int h = m[i].height();
    if (h > mh)
      mh = h;
  }
  return mh;
}
int menu::gettotalheight() {
  int th = 0;
  for (int i = 0; m[i].text; i++)
    th += m[i].height();
  return th;
}
int menu::keyhit(char scan, char key) {
  for (int i = 0; m[i].text; i++)
    if (m[i].key && (m[i].key == key ||
                     ((m[i].key & 0x80) && (m[i].key & 0x7F) == (scan & 0x7F))))
      return m[i].menufunc();
  return 0;
}

// ---- GUIvmenu ----
GUIvmenu::GUIvmenu(GUImenu *p, menu *m, int x, int y) : GUImenu(p, m, x, y) {
  resize(getmenuwidth(), getmenuheight());
}
void GUIvmenu::draw(char *dest) {
  drawrect(dest, CLR_MENU, x1, y1, width(), height());
  outline((char)0);
  int dy = y1;
  for (int i = 0; tmenu->m[i].text; i++) {
    tmenu->m[i].draw(x1, dy, width(), &tmenu->m[i] == selmi);
    dy += tmenu->m[i].height();
  }
  GUIrect::draw(dest);
}
menuitem *GUIvmenu::menuhittest(int x, int y, int &sx, int &sy) {
  if (x < x1 || x >= x2 || y < y1 || y >= y2)
    return 0;
  int dy = y1;
  for (int i = 0; tmenu->m[i].text; i++) {
    if (y >= dy && y < dy + tmenu->m[i].height()) {
      sx = x1;
      sy = dy;
      return &tmenu->m[i];
    }
    dy += tmenu->m[i].height();
  }
  return 0;
}

// ---- GUIhmenu ----
GUIhmenu::GUIhmenu(GUIrect *p, menu *m, int x, int y) : GUImenu(p, m, x, y) {
  resize(getmenuwidth(), getmenuheight());
}
void GUIhmenu::draw(char *dest) {
  drawrect(dest, CLR_MENU, x1, y1, width(), height());
  int dx = x1;
  for (int i = 0; tmenu->m[i].text; i++) {
    tmenu->m[i].draw(dx, y1, tmenu->m[i].width(), &tmenu->m[i] == selmi);
    dx += tmenu->m[i].width();
  }
  GUIrect::draw(dest);
}
menuitem *GUIhmenu::menuhittest(int x, int y, int &sx, int &sy) {
  if (y < y1 || y >= y2)
    return 0;
  int dx = x1;
  for (int i = 0; tmenu->m[i].text; i++) {
    int w = tmenu->m[i].width();
    if (x >= dx && x < dx + w) {
      sx = dx;
      sy = y1;
      return &tmenu->m[i];
    }
    dx += w;
  }
  return 0;
}
int GUIhmenu::keyhit(char scan, char key) { return tmenu->keyhit(scan, key); }

GUIpopupmenu *current_hmenu_popup = 0;

GUIrect *GUIhmenu::click(mouse &m) {
  int sx, sy;
  menuitem *mi = menuhittest(m.x, m.y, sx, sy);
  if (mi)
    selmi = mi;

  // If the item has a submenu, open it as a popup
  if (selmi && selmi->submenu) {
    // Delete any existing popup first
    if (current_hmenu_popup) {
      if (m.capture == current_hmenu_popup)
        m.capture = 0;
      delete current_hmenu_popup;
      current_hmenu_popup = 0;
    }
    int sx2, sy2;
    menuhittest(m.x, m.y, sx2, sy2);
    // Parent to guiroot so popup is hittable (hmenu bounds are too small)
    extern GUIroot *guiroot;
    current_hmenu_popup = new GUIpopupmenu(guiroot, selmi->submenu, sx2, y2);
    selmi = 0;
    return current_hmenu_popup; // Transfer capture to popup
  }
  return this;
}

int GUIhmenu::release(mouse &m) {
  // Close any open popup when releasing on the menu bar
  if (current_hmenu_popup) {
    if (m.capture == current_hmenu_popup)
      m.capture = 0;
    delete current_hmenu_popup;
    current_hmenu_popup = 0;
  }
  if (selmi && selmi->func) {
    selmi->menufunc();
  }
  selmi = 0;
  return 1;
}

// ---- GUIpopupmenu ----
GUIpopupmenu::GUIpopupmenu(GUIrect *treport, menu *m, int x, int y)
    : GUIvmenu((GUImenu *)treport, m, x, y) {
  report = treport;
}
GUIpopupmenu::~GUIpopupmenu() {
  extern GUIpopupmenu *current_hmenu_popup;
  if (current_hmenu_popup == this)
    current_hmenu_popup = 0;
}
int GUIpopupmenu::domenuitem(menuitem *t) {
  GUI_LOG("popup.domenuitem: text='%s' submenu=%p func=%p",
          t->text ? t->text : "(null)", (void *)t->submenu, (void *)t->func);
  if (t->submenu) {
    // Open submenu as a new standalone popup, replacing this one
    extern GUIroot *guiroot;
    extern GUIpopupmenu *current_hmenu_popup;
    extern mouse m;
    int popup_x = x2;
    int popup_y = y1;
    menu *sub = t->submenu;
    // Clear capture before self-delete to prevent dangling pointer
    if (m.capture == this)
      m.capture = 0;
    // Delete ourselves first (sets current_hmenu_popup to 0 via destructor)
    delete this;
    // Create new popup as the current one
    current_hmenu_popup = new GUIpopupmenu(guiroot, sub, popup_x, popup_y);
    return 0;
  }
  int r = GUImenu::domenuitem(t);
  // Clear capture before self-delete to prevent dangling pointer
  extern mouse m;
  if (m.capture == this)
    m.capture = 0;
  delete this;
  return r;
}
int GUIpopupmenu::release(mouse &m) {
  GUI_LOG("popup.release: selmi=%p text='%s'", (void *)selmi,
          selmi && selmi->text ? selmi->text : "(none)");
  if (selmi) {
    menuitem *saved = selmi;
    selmi = 0;
    return domenuitem(saved);
  }
  // Click outside any item — close the popup
  return 1;
}
void GUIpopupmenu::draw(char *dest) { GUIvmenu::draw(dest); }

// SCR::draw is implemented in R2IMG.CPP
