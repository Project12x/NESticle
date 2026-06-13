#include "audio_ring.h"

int nesticle_audio_ring_next(int pos, int size) {
  return size > 0 ? (pos + 1) % size : 0;
}

int nesticle_audio_ring_would_overrun(int write_pos, int read_pos, int size) {
  return nesticle_audio_ring_next(write_pos, size) == read_pos;
}

int nesticle_audio_ring_push(short *ring, int size, int *write_pos,
                             int read_pos, short sample) {
  if (!ring || !write_pos || size <= 0)
    return 0;
  if (nesticle_audio_ring_would_overrun(*write_pos, read_pos, size))
    return 0;
  ring[*write_pos] = sample;
  *write_pos = nesticle_audio_ring_next(*write_pos, size);
  return 1;
}
