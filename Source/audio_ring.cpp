#include "audio_ring.h"

int nesticle_audio_ring_next(int pos, int size) {
  return size > 0 ? (pos + 1) % size : 0;
}

int nesticle_audio_ring_would_overrun(int write_pos, int read_pos, int size) {
  return nesticle_audio_ring_next(write_pos, size) == read_pos;
}
