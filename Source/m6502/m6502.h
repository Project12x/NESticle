
/* General 6502 related goodies */
/* Modified for MSVC compatibility */

#ifndef _M6502_H_
#define _M6502_H_

#ifndef DWORD
#define DWORD unsigned long int
#endif

#ifndef WORD
#define WORD unsigned short int
#endif

#ifndef BYTE
#define BYTE unsigned char
#endif

/* Some bitwise defines for the m6502GetInfo call */

#define MULTI_CPU 0x00010000    /* Set if multi CPU support */
#define TIMING_INFO 0x00020000  /* Set if timing info present */
#define SYNC 0x00040000         /* Set if SYNC support present */
#define BOUNDS_CHECK 0x00080000 /* Set if bounds checking on */

#ifdef __cplusplus
extern "C" {
#endif

#define EXEC_METHOD 0x00100000 /* Set if we're counting cycles */

extern DWORD m6502clockticks;
extern BYTE *m6502rom; // used for accessing >=0x8000
extern BYTE *m6502ram; // used for accessing <0x2000
extern WORD m6502lastjsr;

extern WORD m6502pc;
extern WORD m6502af;
extern BYTE m6502x;
extern BYTE m6502y;
extern BYTE m6502s;
extern DWORD __cdecl m6502nmi(void);
extern DWORD __cdecl m6502int(void);
extern DWORD __cdecl m6502exec(DWORD);
extern void __cdecl m6502reset(void);
extern DWORD __cdecl m6502GetInfo(void);
extern DWORD cyclesRemaining;
extern BYTE syncCycle;
extern BYTE inNmi;

extern int (*m6502MemoryWrite)(WORD, BYTE);
extern WORD (*m6502MemoryRead)(WORD);

#ifdef __cplusplus
}
#endif

#endif /* _M6502_H_ */
