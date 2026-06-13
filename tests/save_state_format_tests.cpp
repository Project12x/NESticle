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

  hdr.payload_size = 1234;
  hdr.mapper = 5;
  failures += expect_false("mapper mismatch is rejected",
                           nesticle_valid_state_header(
                               hdr, sizeof(nesticle_state_header) + 1234, 4,
                               1234));

  nesticle_init_state_header(hdr, 1234, 4);
  hdr.magic[0] = 0;
  failures += expect_false("legacy raw state without NSTA header is rejected",
                           nesticle_valid_state_header(hdr, 1234, 4, 1234));

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

  char max_name[512];
  std::memset(max_name, 'B', sizeof(max_name));
  max_name[sizeof(max_name) - 1] = 0;
  char max_path[516];
  failures += expect_true("state path accepts full romfilename buffer",
                          nesticle_make_state_path(max_name, max_path,
                                                   sizeof(max_path)));

  return failures ? 1 : 0;
}
