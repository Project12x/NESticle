#include "save_state_format.h"

#include <cstring>
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

  nesticle_state_header hdr;
  nesticle_init_state_header(hdr, 1234, 4);

  failures += expect_true(
      "valid header with exact file size is accepted",
      nesticle_valid_state_header(hdr, sizeof(nesticle_state_header) + 1234,
                                  4, 1234));
  failures += expect_false(
      "valid header with trailing bytes is rejected",
      nesticle_valid_state_header(hdr, sizeof(nesticle_state_header) + 1235,
                                  4, 1234));

  hdr.payload_size = 1233;
  failures += expect_false("wrong payload size is rejected",
                           nesticle_valid_state_header(
                               hdr, sizeof(nesticle_state_header) + 1233, 4,
                               1234));

  char path[64];
  failures += expect_true("state path fits normal ROM names",
                          nesticle_make_state_path("mario", path,
                                                   sizeof(path)));
  failures += expect_true("state path appends sta extension",
                          std::strcmp(path, "mario.sta") == 0);

  char long_name[80];
  std::memset(long_name, 'A', sizeof(long_name));
  long_name[sizeof(long_name) - 1] = 0;
  failures += expect_false("state path rejects overflowing ROM names",
                           nesticle_make_state_path(long_name, path,
                                                    sizeof(path)));

  return failures ? 1 : 0;
}
