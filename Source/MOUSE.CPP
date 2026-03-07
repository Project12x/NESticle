
#include "gui.h"
#include "mouse.h"

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
    // Blood drip animation: cycle palette indices 34->38->42->34
    // on the cursor's RLE pixel data every 20 ticks
    static uutimer drip;
    static bool drip_initialized = false;
    if (!drip_initialized) {
      drip.set(20);
      drip_initialized = true;
    }
    if (drip.check()) {
      // Rotate blood colors: 34->38->42->34
      // Use a temp index (255) to avoid overwrite conflicts
      guivol.cursor->convertcolor(42, 255); // 42 -> temp
      guivol.cursor->convertcolor(38, 42);  // 38 -> 42
      guivol.cursor->convertcolor(34, 38);  // 34 -> 38
      guivol.cursor->convertcolor(255, 34); // temp -> 34
      drip.set(20);
    }
    guivol.cursor->draw(dest, x, y);
  }
}
