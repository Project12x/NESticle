#include <stdlib.h>
#include <string.h>

#include "message.h"
#include "nessound.h"

#include "dd.h"
#include "font.h"
#include "r2img.h"

#include "nes.h"

#include "sound.h"

#include "timing.h"

#include "prof.h"

// primary output functions
void clearprimary();
void primaryoutput(short *buf, int nums);
void primaryskip(int nums);

extern byte soundinstalled;

// global nessound device
nessound *ns;

//---------------------------------
// sound enable stuff
byte soundenabled = 1;
void disablesound() {
  if (!soundenabled)
    return;
  clearprimary(); // clear the output buffer
  msg.printf(1, "Sound disabled");
  soundenabled = 0;
}

void enablesound() {
  if (soundenabled)
    return;
  if (!soundinstalled) {
    msg.error("Sound not initialized");
    return;
  }
  soundenabled = 1;
  msg.printf(1, "Sound enabled");
}

void togglesound() {
  if (soundenabled)
    disablesound();
  else
    enablesound();
}

//============================================
// channel i/o

void neschannel::print(int x, int y) {
  font[(enabled && noteon) ? 2 : 1]->printf(
      x, y, "%02X %cf=%03X t=%02X %X %d %X", r[0],
      //   "pbr=%X dt=%X %c%c%c f=%03X t=%02X",
      //   sr.cr1.playbackrate,
      //   sr.cr1.dutycycle,
      //   sr.cr1.holdnote ? 'H' : ' ' ,
      //   sr.cr1.envelopefixed ? ' ' : 'E ' ,
      sr.cr2.freqvariable ? 'V' : 'F', sr.fr.getfreq(), sr.fr.gettime(), r[1],
      deltaK, K);
}

//------------------
// square wave

// duty cycles as % of period (0x10000)
int duty[4] = {87.5 * 0x10000 / 100, 75.0 * 0x10000 / 100, 58.5 * 0x10000 / 100,
               25.0 * 0x10000 / 100};

// int
// duty[4]={77.5*0x10000/100,65.0*0x10000/100,50.0*0x10000/100,35.0*0x10000/100};
// int
// duty[4]={12.5*0x10000/100,25.0*0x10000/100,42.0*0x10000/100,75.0*0x10000/100};
// int duty[4]={50*0x10000/100,50*0x10000/100,50*0x10000/100,50*0x10000/100};

int vols[16] = {
    //  0, 0x200, 0x400, 0x600, 0x800, 0xA00, 0xC00, 0xE00, 0xF00,
    //  0x1000, 0x1100, 0x1200, 0x1300, 0x1400, 0x1500, 0x15FF
    0,     0x200, 0x300, 0x380, 0x400, 0x480, 0x500, 0x580, 0x600,
    0x640, 0x680, 0x6C0, 0x700, 0x740, 0x780, 0x7C0
    //      0,0x200, 0x300, 0x380, 0x400, 0x420, 0x500, 0x580,
    //  0x600,0x610, 0x620, 0x630, 0x640, 0x650, 0x660, 0x670
};

void squarewave::setvolume() {
  vol = vols[sr.cr1.playbackrate]; // set volume
  // if (!sr.cr1.envelopefixed) vol<<=1;
}

void squarewave::mix16(short *b, int nums) {
  if (!noteon)
    return;
  if (vol <= 0) {
    dur -= nums;
    if (dur < 0)
      noteon = 0;
    return;
  } // ignore sound

  int d = duty[sr.cr1.dutycycle]; // get duty cycles
  for (int i = 0; i < nums && dur > 0; i++, dur--) {
    b[i] += (count < d) ? vol : -vol;
    count += K;
    count &= 0xFFFF;

    // adjust freq/volume
    if (!(dur & 255)) {
      if (!sr.cr1.envelopefixed)
        if (--vol < 0)
          return;
      // if (!(dur&127))
      K += deltaK;
    }
  }
  if (dur < 0) {
    noteon = 0;
    return;
  }
}

//----------------------

void trianglewave::setvolume() {
  vol = 0x600; // vols[(r[0]&0x1F)/2];
}

void trianglewave::mix16(short *b, int nums) {
  if (!noteon)
    return;
  if (!r[0]) {
    dur -= nums;
    return;
  } // ignore sound

  for (int i = 0; i < nums && dur > 0; i++, dur--) {
    count += K;
    int x;
    if (!(count & 0x8000))
      x = -0x4000 + (count & 0x7FFF); // positive slope
    else
      x = 0x4000 - (count & 0x7FFF); // negative slope
    b[i] += x * vol / 0x4000;
    //   if (!(dur&31) && !sr.cr1.envelopefixed)  vol--;
  }
  if (dur < 0) {
    noteon = 0;
    return;
  }
}
//=============================================

void neschannel::startnote() {
  if (!sr.fr.getfreq()) {
    noteon = 0;
    return;
  }
  freq = 0x20000 / sr.fr.getfreq(); // set frequency
  K = (freq << 16) / SOUNDRATE;     // set freq period counter

  if (!sr.cr1.holdnote)
    dur = SOUNDRATE * (sr.fr.gettime() + 1) / 16;
  else
    dur = SOUNDRATE / 2;
  if (enabled)
    noteon = 1;
}

void neschannel::setdeltaK() {
  if (!sr.cr2.freqvariable || !sr.cr2.freqrange || !sr.cr2.freqchangespeed) {
    deltaK = 0;
    return;
  }
  deltaK = (sr.cr2.freqrange << 16) / SOUNDRATE;
  if (sr.cr2.freqselect)
    deltaK = -deltaK;
}

void neschannel::write(byte a, byte d) {
  r[a] = d;
  switch (a) {
  case 0:
    setvolume();
    break;
  case 1:
    setdeltaK();
    break;
  case 2:
  case 3:
    startnote();
    break;
  }
}

void neschannel::stopnote() { noteon = 0; }
void neschannel::reset() {
  r[0] = r[1] = r[2] = r[3] = 0;
  deltaK = 0;
  stopnote();
  enabled = 0;
}
void neschannel::setenable(byte x) {
  enabled = x ? 1 : 0;
  if (!enabled)
    stopnote();
}

//=============================================
// Noise channel - 15-bit LFSR
// NES $400C-$400F: r[0]=volume/envelope, r[2]=mode+period, r[3]=length

// NTSC noise period lookup table (in CPU cycles)
static const int noise_period_table[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068};

void noisewave::setvolume() { vol = vols[sr.cr1.playbackrate]; }

void noisewave::startnote() {
  // r[2] bits 0-3 = period index, bit 7 = mode (short/long)
  period = noise_period_table[r[2] & 0x0F];

  if (!sr.cr1.holdnote)
    dur = SOUNDRATE * (sr.fr.gettime() + 1) / 16;
  else
    dur = SOUNDRATE / 2;

  if (enabled)
    noteon = 1;
}

void noisewave::mix16(short *b, int nums) {
  if (!noteon)
    return;
  if (vol <= 0) {
    dur -= nums;
    if (dur < 0)
      noteon = 0;
    return;
  }

  int mode_short = r[2] & 0x80; // bit 7: 0=long (32767), 1=short (93)

  // CPU cycles per audio sample: NTSC CPU 1789773 Hz / SOUNDRATE
  // Use fixed-point: cycles_per_sample = 1789773 / SOUNDRATE
  int cycles_per_sample = 1789773 / SOUNDRATE; // ~81 for 22050 Hz

  for (int i = 0; i < nums && dur > 0; i++, dur--) {
    // Output based on bit 0 of LFSR
    b[i] += (lfsr & 1) ? vol : -vol;

    // Accumulate CPU cycles for this sample and clock LFSR accordingly
    timer += cycles_per_sample;
    while (timer >= period) {
      timer -= period;
      // Feedback: XOR bit 0 with bit 6 (short) or bit 1 (long)
      int feedback;
      if (mode_short)
        feedback = (lfsr & 1) ^ ((lfsr >> 6) & 1);
      else
        feedback = (lfsr & 1) ^ ((lfsr >> 1) & 1);
      lfsr >>= 1;
      lfsr |= (feedback << 14); // feed back into bit 14
    }

    // Envelope decay
    if (!(dur & 255)) {
      if (!sr.cr1.envelopefixed)
        if (--vol < 0)
          return;
    }
  }
  if (dur < 0) {
    noteon = 0;
    return;
  }
}

//----------------------
// DMC channel - Delta Modulation
// NES $4010-$4013: simplified (no ROM sample fetching in this implementation)
// Just outputs a level based on register writes

static const int dmc_rate_table[16] = {428, 380, 340, 320, 286, 254, 226, 214,
                                       190, 160, 142, 128, 106, 84,  72,  54};

void dmcwave::setvolume() {
  // DMC doesn't use the standard volume system
  // $4011 (r[1]) directly sets the output level (7-bit: 0-127)
  output_level = r[1] & 0x7F;
}

void dmcwave::mix16(short *b, int nums) {
  if (!noteon)
    return;

  // DMC outputs a constant level set by $4011
  // Without ROM sample fetching, this provides the direct DAC output
  // which games use for PCM playback via rapid $4011 writes
  short out = (short)((output_level - 64) * 0x10); // center and scale

  for (int i = 0; i < nums; i++) {
    b[i] += out;
  }
}

//===================================================
// main sound device
void nessound::write(byte a, byte d) { ch[a / 4]->write(a & 3, d); }

nessound::nessound() {
  ch[0] = new squarewave;
  ch[1] = new squarewave;
  ch[2] = new trianglewave;
  ch[3] = new noisewave;
  ch[4] = new dmcwave;
  clearprimary();
}

nessound::~nessound() {
  clearprimary();
  for (int i = 0; i < 5; i++)
    delete ch[i];
}

void nessound::reset() {
  for (int i = 0; i < 5; i++)
    ch[i]->reset();
  memset(mixingbuf, 0, 4096 * 2);
}

// decode enable bit for each channel
void nessound::setenable(byte d) {
  enablebits = d;
  for (int i = 0; i < 5; i++)
    ch[i]->setenable(d & (1 << i));
}

// print all channels
void nessound::print() {
  // for (int i=0; i<5; i++)
  //  ch[i]->print(50,50+i*10);
}

//------
// mixing

// mixes nums 16 samples to *b
void nessound::mix16(short *b, int nums) {
  // clear buffer
  memset(b, 0, nums * 2);
  if (!CPURunning)
    return;

  // mix all sound channels
  for (int i = 0; i < 5; i++)
    if (ch[i]->enabled)
      ch[i]->mix16(b, nums);
}

short nessound::mixingbuf[4096];

void nessound::update(int nums) {
  if (!soundinstalled)
    return;
  if (!soundenabled) {
    primaryskip(nums);
    return;
  }
  pf.sound_timer.enter();

  mix16(mixingbuf, nums);         // do 16bit mixing
  primaryoutput(mixingbuf, nums); // output it to primary buffer
  pf.sound_timer.leave();
}
