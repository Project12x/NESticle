#include "volbatch.h"

// data from guivol
struct GUIVOL : volbatch {
  struct PALETTE *pal;   // default palette
  struct IMG *cursor;    // cursor
  struct FONT *font;     // first font
  struct FONT *sfont;    // second (small) font -- sfont.fnt in gui.vol
  struct IMG *checkmark; // checkmark
  struct IMG *umark, *dmark, *lmark, *rmark; // arrows for scroll bar
  struct IMG *sumark, *sdmark;               // small up/down arrows
  struct IMG *xmark;                         // close button (wx.r2)
  struct IMG *wmmark;                        // maximize button (wm.r2)
  struct IMG *mcheck;                        // menu checkmark
  struct IMG *menur;                         // menu right arrow
  struct IMG *nesicon;                       // nes icon
  struct IMG *diricon;                       // directory icon
  struct IMG *driveicon;                     // drive icon
  struct IMG *bc1, *bc2, *bc3, *bc4;         // button corner images
  struct IMG *about;                         // buddy's head (shitman.r)

  virtual int size();
};
extern GUIVOL guivol;
