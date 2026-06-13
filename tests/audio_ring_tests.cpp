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

  return failures ? 1 : 0;
}
