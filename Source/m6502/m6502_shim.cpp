// m6502_shim.cpp - Bridges fake6502 API to NESticle's m6502.h interface
// This shim wraps the public domain fake6502 CPU core to match the API
// that NESticle's code expects (originally backed by x86 ASM).

#include <cstdint>
#include <cstring>

// NESticle's m6502.h types
#ifndef DWORD
#define DWORD unsigned long int
#endif
#ifndef WORD
#define WORD unsigned short int
#endif
#ifndef BYTE
#define BYTE unsigned char
#endif

// ---- Exported state matching m6502.h ----
extern "C" {

DWORD m6502clockticks = 0;
BYTE *m6502rom = nullptr; // used for accessing >= 0x8000
BYTE *m6502ram = nullptr; // used for accessing < 0x2000
WORD m6502lastjsr = 0;

WORD m6502pc = 0;
WORD m6502af = 0; // A in high byte, flags in low byte
BYTE m6502x = 0;
BYTE m6502y = 0;
BYTE m6502s = 0;
DWORD cyclesRemaining = 0;
BYTE syncCycle = 0;
BYTE inNmi = 0;

// Memory access callbacks - NESticle sets these
int (*m6502MemoryWrite)(WORD, BYTE) = nullptr;
WORD (*m6502MemoryRead)(WORD) = nullptr;

} // extern "C"

// ---- fake6502 interface ----
// fake6502.c expects these two functions to be defined externally
extern "C" {
// fake6502 globals we need access to
extern uint16_t pc;
extern uint8_t sp, a, x, y, status;
extern uint32_t clockticks6502, clockgoal6502;

// fake6502 functions
extern void reset6502(void);
extern void exec6502(uint32_t tickcount);
extern void step6502(void);
extern void nmi6502(void);
extern void irq6502(void);
}

// read6502/write6502 - called by fake6502.c, route to NESticle callbacks
extern "C" uint8_t read6502(uint16_t address) {
  m6502clockticks = clockticks6502; // Sync current cycle for getscanline()

  if (address < 0x2000) {
    // Direct RAM access (mirrored every 0x800 bytes)
    if (m6502ram)
      return m6502ram[address & 0x7FF];
    return 0;
  }
  if (address >= 0x8000) {
    // ROM access
    if (m6502rom)
      return m6502rom[address];
    return 0;
  }
  // I/O registers - use callback
  if (m6502MemoryRead)
    return (uint8_t)m6502MemoryRead(address);
  return 0;
}

extern "C" void write6502(uint16_t address, uint8_t value) {
  m6502clockticks = clockticks6502; // Sync current cycle for getscanline()

  if (address < 0x2000) {
    // Direct RAM write (mirrored)
    if (m6502ram)
      m6502ram[address & 0x7FF] = value;
    return;
  }
  if (m6502MemoryWrite)
    m6502MemoryWrite(address, value);
}

// Sync fake6502 state -> m6502 exported vars
static void sync_from_fake6502() {
  m6502pc = (WORD)pc;
  m6502x = (BYTE)x;
  m6502y = (BYTE)y;
  m6502s = (BYTE)sp;
  m6502af = ((WORD)a << 8) | (WORD)status;
  m6502clockticks = (DWORD)clockticks6502;
}

// Sync m6502 exported vars -> fake6502 state (including clock)
static void sync_clock_to_fake6502() {
  clockticks6502 = (uint32_t)m6502clockticks;
  clockgoal6502 = clockticks6502;
}

// Sync m6502 exported vars -> fake6502 state
static void sync_to_fake6502() {
  pc = (uint16_t)m6502pc;
  x = (uint8_t)m6502x;
  y = (uint8_t)m6502y;
  sp = (uint8_t)m6502s;
  a = (uint8_t)(m6502af >> 8);
  status = (uint8_t)(m6502af & 0xFF);
  sync_clock_to_fake6502();
}

// ---- m6502.h API implementation ----
extern "C" {

void __cdecl m6502reset(void) {
  clockticks6502 = 0;
  clockgoal6502 = 0;
  reset6502();
  sync_from_fake6502();
}

DWORD __cdecl m6502exec(DWORD cycles) {
  sync_to_fake6502();
  exec6502((uint32_t)cycles);
  sync_from_fake6502();
  cyclesRemaining = 0;
  // NESticle expects 0x10000 return on success (no abort)
  return 0x10000;
}

DWORD __cdecl m6502nmi(void) {
  sync_to_fake6502();
  inNmi = 1;
  nmi6502();
  sync_from_fake6502();
  inNmi = 0;
  return 0;
}

DWORD __cdecl m6502int(void) {
  sync_to_fake6502();
  irq6502();
  sync_from_fake6502();
  return 0;
}

DWORD __cdecl m6502GetInfo(void) { return 0; }

} // extern "C"
