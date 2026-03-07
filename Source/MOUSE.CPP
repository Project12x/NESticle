
#include "gui.h"
#include "mouse.h"

#include "dd.h"
#include "guivol.h"
#include "message.h"
#include "r2img.h"
#include "uutimer.h"

mouse::mouse() {
  oldb = 0;
  x = y = oldx = oldy = 0;
  click = rel = b = 0;
  capture = 0;
  showcursor();
}

void mouse::updatexy(int newx, int newy) { // set new x,y
  x = newx;
  y = newy;
  // if (GUIrect::modal) capture=GUIrect::modal;

  if (x != oldx || y != oldy) {
    if (capture)
      capture->drag(*this);
  }
}

void enablegui();
void mouse::updatebut(int newb) {
  oldb = b; // save old button state

  b = newb;

  click = b & (oldb ^ 7); // press triggers
  rel = oldb & (b ^ 7);   // release triggers

  // do hit testing
  if (click) // button was clicked
  {
    if (click & 1)
      enablegui();
    if (GUIrect::modal) {
      GUIrect::modal->click(*this);
    } else if (!capture) // not captured
    {
      capture = root->hittest(x, y);
      if (capture) {
        capture = capture->click(*this);
        if (capture)
          capture->drag(*this);
      }
    } else
      capture = capture->click(*this);
  }
  if (rel) // button was released
    if (capture) {
      if (capture->release(*this))
        capture = 0;
      if (GUIrect::modal)
        capture = GUIrect::modal;
    }
}

void mouse::draw(char *dest) {
  if (!hidden && guivol.cursor) {
    // Blood drip animation: rotate COLOR VALUES at palette indices 34, 38, 42
    // The cursor has blood drip shapes at different positions, each using a
    // different palette index. By rotating which index holds the "bright red"
    // vs "dark red" color, different drip positions light up in sequence,
    // creating the downward dripping illusion.
    static uutimer drip;
    static bool drip_initialized = false;
    static int drip_phase = 0;
    if (!drip_initialized) {
      drip.set(20);
      drip_initialized = true;
    }
    if (drip.check()) {
      drip_phase = (drip_phase + 1) % 3;
      // Original blood colors (after <<2 fixup from gui.vol):
      // pal[34] = (224,0,0) bright red
      // pal[38] = (176,0,0) medium red
      // pal[42] = (124,0,0) dark red
      static COLOR blood_colors[3] = {
          {(unsigned char)224, 0, 0}, // bright
          {(unsigned char)176, 0, 0}, // medium
          {(unsigned char)124, 0, 0}  // dark
      };
      // Rotate which color goes to which index
      setpalettenum(34, &blood_colors[(0 + drip_phase) % 3]);
      setpalettenum(38, &blood_colors[(1 + drip_phase) % 3]);
      setpalettenum(42, &blood_colors[(2 + drip_phase) % 3]);
      drip.set(20);
    }
    guivol.cursor->draw(dest, x, y);
  }
}
