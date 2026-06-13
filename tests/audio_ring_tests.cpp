#include "audio_ring.h"

#include <cstdio>

static int expect_true(const char *name, bool value) {
  if (!value) {
    std::fprintf(stderr, "FAIL: %s\n", name);
    return 1;
  }
  return 0;
}

static int expect_false(const char *name, bool value) {
  return expect_true(name, !value);
}

int main() {
  int failures = 0;

  failures += expect_true("next position wraps at ring size",
                          nesticle_audio_ring_next(3, 4) == 0);
  failures += expect_false("writer with room does not overrun",
                           nesticle_audio_ring_would_overrun(1, 0, 4));
  failures += expect_true("writer one slot behind reader would overrun",
                          nesticle_audio_ring_would_overrun(3, 0, 4));

  short ring[4] = {11, 22, 33, 44};
  int write_pos = 3;
  failures += expect_false("push rejects full ring without advancing writer",
                           nesticle_audio_ring_push(ring, 4, &write_pos, 0,
                                                    99));
  failures += expect_true("full-ring push preserves writer position",
                          write_pos == 3);
  failures += expect_true("full-ring push preserves queued sample",
                          ring[3] == 44);

  write_pos = 1;
  failures += expect_true("push writes when ring has room",
                          nesticle_audio_ring_push(ring, 4, &write_pos, 3,
                                                   55));
  failures += expect_true("successful push advances writer", write_pos == 2);
  failures += expect_true("successful push stores sample", ring[1] == 55);

  return failures ? 1 : 0;
}
