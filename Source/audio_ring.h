#ifndef NESTICLE_AUDIO_RING_H
#define NESTICLE_AUDIO_RING_H

int nesticle_audio_ring_next(int pos, int size);
int nesticle_audio_ring_would_overrun(int write_pos, int read_pos, int size);

#endif
