#include "config_format.h"

int nesticle_config_version_supported(int version) {
  return version > 0 && version <= NESTICLE_CONFIG_VERSION;
}

int nesticle_config_size_supported(unsigned bytes, unsigned current_size) {
  return bytes >= sizeof(int) && bytes <= current_size;
}
