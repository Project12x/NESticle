#ifndef NESTICLE_CONFIG_FORMAT_H
#define NESTICLE_CONFIG_FORMAT_H

#define NESTICLE_CONFIG_VERSION 0x101

int nesticle_config_version_supported(int version);
int nesticle_config_size_supported(unsigned bytes, unsigned current_size);

#endif
