//routines for configuration file
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#ifdef _MSC_VER
#include <windows.h>
#endif

#include "input.h"
#include "config.h"
#include "file.h"
#include "message.h"

static int validinputtype(int type) {
 return type>=ID_NONE && type<=ID_GAMEPAD2;
}

static int validpadbutton(int button) {
 return button>=0 && button<=31;
}

static int validdeadzone(int deadzone) {
 return deadzone>=GAMEPAD_MIN_DEADZONE && deadzone<=GAMEPAD_MAX_DEADZONE;
}

static int validpadmap(const padmap &pm) {
 return pm.nesStart!=0 &&
        validpadbutton(pm.nesB) &&
        validpadbutton(pm.nesA) &&
        validpadbutton(pm.nesSelect) &&
        validpadbutton(pm.nesStart);
}

static void normalizeconfig(config &c) {
 c.version=CONFIGVERSION;

 config defaults;
 for (int i=0; i<4; i++)
  if (!validinputtype(c.pinput[i]))
   c.pinput[i]=defaults.pinput[i];

 for (int i=0; i<2; i++)
  {
   if (!validpadmap(c.ids.pm[i]))
    c.ids.pm[i]=defpadmap;
   if (!validdeadzone(c.ids.gamepad_deadzone[i]))
    c.ids.gamepad_deadzone[i]=GAMEPAD_DEFAULT_DEADZONE;
  }

 c.lastromdir[sizeof(c.lastromdir)-1]=0;
 for (int i=0; i<MAX_RECENT_ROMS; i++)
  c.recentroms[i][sizeof(c.recentroms[i])-1]=0;
}

config::config()
{
 memset(this,0,sizeof(config));
 version=CONFIGVERSION;

 pinput[0]=ID_KEY1;
 pinput[1]=ID_NONE;
 pinput[2]=ID_NONE;
 pinput[3]=ID_NONE;

 ids.km[0]=defkeymap[0];
 ids.km[1]=defkeymap[1];
 ids.pm[0]=defpadmap;
 ids.pm[1]=defpadmap;
 ids.gamepad_deadzone[0]=GAMEPAD_DEFAULT_DEADZONE;
 ids.gamepad_deadzone[1]=GAMEPAD_DEFAULT_DEADZONE;
 set(CFG_FILLEDDLG,1);
}


int config::load(char *file)
{
 FILEIO f;
 if (f.open(file)) return 0;

 unsigned bytes=f.size();
 if (!nesticle_config_size_supported(bytes,sizeof(config)))
  {
   f.close();
   msg.error("Config file %s has unsupported size; using defaults",file);
   return 0;
  }

 config loaded;
 unsigned readbytes=bytes<sizeof(config) ? bytes : sizeof(config);
 if (f.read(&loaded,readbytes))
  {
   f.close();
   msg.error("Unable to read config %s; using defaults",file);
   return 0;
  }
 f.close();

 if (!nesticle_config_version_supported(loaded.version))
  {
   msg.error("Config file %s has unsupported version; using defaults",file);
   return 0;
  }

 normalizeconfig(loaded);
 memcpy(this,&loaded,sizeof(config));
 msg.printf(2,"Config loaded from %s",file);
 return 1;
}


void config::save(char *file)
{
 FILEIO f;
 char tmpfile[512];
 snprintf(tmpfile,sizeof(tmpfile),"%s.tmp",file);

 if (f.create(tmpfile))
  {
   msg.error("unable to save config %s",file);
   return;
  }
 if (f.write(this,sizeof(config)))
  {
   f.close();
   remove(tmpfile);
   msg.error("unable to write config %s",file);
   return;
  }
 f.close();

#ifdef _MSC_VER
 if (!MoveFileExA(tmpfile,file,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
  {
   remove(tmpfile);
   msg.error("unable to replace config %s",file);
   return;
  }
#else
 remove(file);
 if (rename(tmpfile,file))
  {
   remove(tmpfile);
   msg.error("unable to replace config %s",file);
   return;
  }
#endif

 msg.printf(2,"Config saved to %s",file);
}


