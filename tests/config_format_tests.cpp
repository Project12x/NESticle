#include "config_format.h"

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

  failures += expect_true("current config version is accepted",
                          nesticle_config_version_supported(0x101));
  failures += expect_false("zero config version is rejected",
                           nesticle_config_version_supported(0));
  failures += expect_false("future config version is rejected",
                           nesticle_config_version_supported(0x7fffffff));

  failures += expect_false("zero-byte config is rejected",
                           nesticle_config_size_supported(0, 1024));
  failures += expect_true("small old config payload is accepted for defaults",
                          nesticle_config_size_supported(sizeof(int), 1024));
  failures += expect_false("oversized config payload is rejected",
                           nesticle_config_size_supported(1025, 1024));

  return failures ? 1 : 0;
}
