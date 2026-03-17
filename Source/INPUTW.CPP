#ifdef __clang__
#pragma clang diagnostic ignored "-Wwritable-strings"
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
// allows for changing of input devices
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dd.h"
#include "font.h"
#include "gui.h"
#include "input.h"
#include "r2img.h"

// #include "object.h"
// #include "objdef.h"

#include "keyb.h"

#include "config.h"

// input device settings.
extern inputdevicesettings *ids;

#ifdef SDL2
#include <SDL.h>
#endif

// Dynamic device list entry: maps list index to device type + controller index
struct deviceentry {
  int type;      // ID_NONE, ID_KEY1, ID_KEY2, ID_GAMEPAD1, etc.
  int sdlIndex;  // SDL joystick index (for gamepads)
  char name[64]; // Display name
};

#define MAX_DEVICES 16
static deviceentry devices[MAX_DEVICES];
static int numdevices = 0;

static void enumerateDevices() {
  numdevices = 0;
  // Fixed entries
  devices[numdevices] = {ID_NONE, -1, "None"};
  numdevices++;
  devices[numdevices] = {ID_KEY1, -1, "Keyboard 1"};
  numdevices++;
  devices[numdevices] = {ID_KEY2, -1, "Keyboard 2"};
  numdevices++;

#ifdef SDL2
  // Enumerate connected SDL GameControllers
  for (int i = 0; i < SDL_NumJoysticks() && numdevices < MAX_DEVICES; i++) {
    if (SDL_IsGameController(i)) {
      const char *cname = SDL_GameControllerNameForIndex(i);
      devices[numdevices].type = ID_GAMEPAD1;
      devices[numdevices].sdlIndex = i;
      if (cname)
        snprintf(devices[numdevices].name, 64, "%s", cname);
      else
        snprintf(devices[numdevices].name, 64, "Controller %d", i);
      numdevices++;
    }
  }
#endif

  fprintf(stderr, "[INPUT] enumerateDevices: found %d devices\\n", numdevices);
  for (int i = 0; i < numdevices; i++)
    fprintf(stderr, "[INPUT]   [%d] type=%d sdl=%d name='%s'\\n", i,
            devices[i].type, devices[i].sdlIndex, devices[i].name);
  fflush(stderr);
}

// Legacy names kept for compatibility but not used in device list
static char *inputnames[] = {
    "None",          "Gravis Gamepad", "Keyboard 1",  "Keyboard 2",
    "Joystick 1",    "Joystick 2",     "GrIP Slot 1", "GrIP Slot 2",
    "SDL Gamepad 1", "SDL Gamepad 2",
};

class keyedit : public GUIedit {
  char *keyname;
  char &keyscan; // scancode to be modified
public:
  keyedit(GUIrect *p, char *pmpt, int x2, int y, char &scan)
      : GUIedit(p, pmpt, x2, y, 70), keyscan(scan) {
    moverel(-width(), 0);
    keyname = getkeyname(keyscan);
  };

  virtual GUIrect *click(mouse &m) {
    if (modal)
      setmodal(0);
    else
      setmodal(this);
    setfocus(this);
    return 0;
  };

  virtual int release(mouse &m) { return 1; }

  virtual int keyhit(char kbscan, char key) {
    if (modal != this) {
      if (kbscan == KB_ENTER) {
        setmodal(this);
        return 1;
      }
      setmodal(0);
      return 0;
    }
    if (kbscan != KB_ESC) {
      keyscan = kbscan;
      keyname = getkeyname(keyscan);
      parent->cyclefocus(0);
    }
    setmodal(0); // parent->parent);
    return 1;
  }

  virtual void drawdata(char *dest, int x, int y, int xw) {
    if (modal == this && (uu & 16))
      drawrect(dest, 2, x, y, xw, 10);
    font[1]->draw(keyname, dest, x, y);
  }

  virtual char *getname() { return "keyedit"; }
};

char *mapnames[] = {
    "Up-left", // 0
    "Up",        "Up-right",   "Left", "Right",
    "Down-left", // 5
    "Down",      "Down-Right",
    "B", // 8
    "A",         "Select",
    "Start", // 11
};

class redefinekeys : public GUIcontents {
  keymap &km;

public:
  redefinekeys(keymap &tkm) : GUIcontents(220, 150), km(tkm) {
    for (int i = 0; i <= 11; i++) {
      char s[30];
      strcpy(s, mapnames[i]);
      strcat(s, ": ");
      new keyedit(this, s, width() - 5, i * 12 + 3, ((char *)&km)[i]);
    }
  };

  virtual ~redefinekeys() {
    pos.close((GUIbox *)parent);
    losefocus();
  }

  virtual int acceptfocus() { return 1; }

  virtual void draw(char *dest) {
    fill(1);
    GUIrect::draw(dest);
  }

  virtual char *getname() { return "redefinekeys"; }

  // open functions
  static DLGPOS pos; // saved last position of dialog
  static void open(keymap *km) {
    pos.open(new GUIbox(guiroot, "Redefine keys", new redefinekeys(*km), 0, 0));
    //   setmodal(pos.opened);
  };
};
DLGPOS redefinekeys::pos;

typedef input *INPUTPTR;
class inputtestbutton : public GUIrect {
  input *in;
  int button; // button number
public:
  void setinput(input *tin) { in = tin; }
  void setbutton(int tbut) { button = 1 << (tbut + 4); }

  inputtestbutton(GUIrect *p, int x1, int y1, int xw, int yw, input *tin,
                  int tbut)
      : GUIrect(p, x1, y1, x1 + xw, y1 + yw) {
    setinput(tin);
    setbutton(tbut);
  }
  virtual void draw(char *dest) {
    if (in && in->but & button)
      fill(4);
    else
      fill(9 * 16 + 14);
    outline(4);
  }
  virtual char *getname() { return "inputtestbutton"; }
};

class inputtestarea : public GUIrect {
  input *in; // input to test
public:
  void setinput(input *tin) { in = tin; }
  inputtestarea(GUIrect *p, int x1, int y1, int width, input *tin)
      : GUIrect(p, x1, y1, x1 + width, y1 + width) {
    setinput(tin);
  }

  virtual void draw(char *dest) {
    // drawbox(dest,4,x1-1,y1-1,x2+1,y2+1);
    fill(9 * 16 + 14);
    outline(4);
    if (!in)
      return;

    int x, y;
    if (in->stat & ID_RIGHT)
      x = width() - 10;
    else if (in->stat & ID_LEFT)
      x = 0;
    else
      x = width() / 2 - 5;
    if (in->stat & ID_DOWN)
      y = height() - 10;
    else if (in->stat & ID_UP)
      y = 0;
    else
      y = height() / 2 - 5;

    drawrect(dest, 2, x1 + x, y1 + y, 10, 10);
  }
  virtual char *getname() { return "inputtestarea"; }
};

#include "message.h"

class changeinputdevice : public GUIcontents {
protected:
  int lastintype;
  int inputnum;
  INPUTPTR &in; // pointer to input device being changed
  GUIstringlistbox *list;
  inputtestarea *test;
  inputtestbutton *b[4];

  GUItextbutton *edit, *save;

  object *lasto; // object that was bound to input

public:
  changeinputdevice(int num, INPUTPTR &tin)
      : GUIcontents(280, 145), in(tin), inputnum(num) {
    fprintf(stderr, "[INPUT] changeinputdevice(%d) in=%p\n", num, (void *)in);
    fflush(stderr);

    // Enumerate available devices
    enumerateDevices();

    new GUIstatictext(this, 1, "Input type:", 5, 3);
    int visibleItems = numdevices < 10 ? numdevices : 10;
    list = new GUIstringlistbox(this, 5, 15, 110, visibleItems, 10);
    edit = save = 0;

    // Fill list box from dynamic device list
    ITEMPTR *a = list->resizeitems(numdevices);
    for (int i = 0; i < numdevices; i++)
      a[i] = (ITEMPTR)devices[i].name;

    new GUIstatictext(this, 1, "Test area:", 125, 3);
    test = new inputtestarea(this, 125, 15, 91, in);
    for (int j = 0; j < 4; j++)
      b[j] = new inputtestbutton(this, 127 + j * 23, 110, 18, 10, in, j);
    lasto = 0;
    if (in) {
      lasto = in->o;
      // Find current device in list by matching type
      for (int k = 0; k < numdevices; k++) {
        if (devices[k].type == in->type) {
          list->setsel(k);
          break;
        }
      }
      lastintype = in->type;
    }

    // Create initial buttons for the current device
    if (in) {
      if (in->getjoythreshold())
        edit = new GUItextbutton(this, "Calibrate...", 60, 90);
      else if (in->getkeymap())
        edit = new GUItextbutton(this, "Redefine keys...", 60, 90);
    }
    save = new GUItextbutton(this, "Save settings", 60, 105);
  }

  void changedevice(int listIdx) {
    if (listIdx < 0 || listIdx >= numdevices)
      return;
    deviceentry &dev = devices[listIdx];
    fprintf(stderr,
            "[INPUT] changedevice: listIdx=%d type=%d sdl=%d name='%s'\n",
            listIdx, dev.type, dev.sdlIndex, dev.name);
    fflush(stderr);
    disable();
    if (in)
      delete in;
    if (dev.sdlIndex >= 0)
      in = newinputdevice(dev.type, dev.sdlIndex);
    else
      in = newinputdevice(dev.type);
    fprintf(stderr, "[INPUT] newinputdevice returned %p\n", (void *)in);
    fflush(stderr);
    if (in)
      in->bind(lasto);
    enable();
  }

  virtual ~changeinputdevice() {
    pos.close((GUIbox *)parent);
    if (in) {
      changedevice(lastintype); // reset input device from last settings
      //     in->bind(lasto);
    }
  }

  virtual void losefocus() { GUIrect::losefocus(); }

  virtual void receivefocus() {
    if (in)
      in->testing = 1;
    GUIrect::receivefocus();
  }

  virtual int sendmessage(GUIrect *c, int guimsg) {
    if (c == edit)
      if (guimsg == GUIMSG_PUSHED)
        if (in) {
          if (in->getkeymap()) {
            redefinekeys::open(in->getkeymap());
          }
          // else  if (in->getjoythreshold())
          // calibratejoy::open(in->getjoythreshold());
          in->testing = 0;
        }
    if (c == save)
      if (guimsg == GUIMSG_PUSHED) {
        if (in) {
          in->savesettings(); // save settings to ids
          lastintype = in->type;
          cfg->pinput[inputnum] = in->type;
          cfg->save(configfile);
        }
        delete parent;
        return 1;
      }

    if (guimsg == GUIMSG_LISTBOXSELCHANGED) // they changed input devices
    {
      int x = list->getselnum();
      if (x < 0)
        return 1;
      changedevice(x);
      if (in)
        in->testing = 1;

      if (test) {
        test->setinput(in);
        for (int i = 0; i < 4; i++)
          b[i]->setinput(in);
      }

      if (edit) {
        delete edit;
        edit = 0;
      }
      if (save) {
        delete save;
        save = 0;
      }
      if (in) {
        if (in->getjoythreshold())
          edit = new GUItextbutton(this, "Calibrate...", 60, 90);
        else if (in->getkeymap())
          edit = new GUItextbutton(this, "Redefine keys...", 60, 90);
      }
      // if (edit)
      save = new GUItextbutton(this, "Save settings", 60, 105);
    }
    return 1;
  };

  virtual int acceptfocus() { return 1; }
  virtual char *getname() { return "changeinputdevice"; }
  virtual void draw(char *dest) {
    fill(1);
    /*   if (lasto) //is bound?
       {
        char s[80];
        sprintf(s,"Bound to: %s",lasto->od->name);
        font[3]->drawcentered(s,dest,x1+width()/2,y1+125);
       }*/

    font[3]->draw("B", dest, x1 + 133, y1 + 122);
    font[3]->draw("A", dest, x1 + 134 + 23, y1 + 122);
    font[3]->draw("Sl", dest, x1 + 132 + 23 * 2, y1 + 122);
    font[3]->draw("St", dest, x1 + 132 + 23 * 3, y1 + 122);

    GUIrect::draw(dest);
  };

  // open functions
  static DLGPOS pos; // saved last position of dialog
  static void open(int inputdevicenum) {
    if (pos.opened)
      return;
    char s[80];
    sprintf(s, "Input device %d", inputdevicenum + 1);
    pos.open(new GUIbox(
        guiroot, s,
        new changeinputdevice(inputdevicenum, inputdevice[inputdevicenum]), 0,
        0));
  };
};
DLGPOS changeinputdevice::pos;

void editinputdevice0() { changeinputdevice::open(0); }
void editinputdevice1() { changeinputdevice::open(1); }
