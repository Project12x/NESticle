#include "save_state_format.h"

#include <stdio.h>

void nesticle_init_state_header(nesticle_state_header &hdr,
                                unsigned payload_size, unsigned mapper) {
  hdr.magic[0] = 'N';
  hdr.magic[1] = 'S';
  hdr.magic[2] = 'T';
  hdr.magic[3] = 'A';
  hdr.version = NESTICLE_STATE_VERSION;
  hdr.payload_size = payload_size;
  hdr.mapper = mapper;
  hdr.flags = 0;
}

int nesticle_valid_state_header(const nesticle_state_header &hdr,
                                unsigned filesize, unsigned current_mapper,
                                unsigned expected_payload_size) {
  if (hdr.magic[0] != 'N' || hdr.magic[1] != 'S' || hdr.magic[2] != 'T' ||
      hdr.magic[3] != 'A')
    return 0;
  if (hdr.version != NESTICLE_STATE_VERSION)
    return 0;
  if (hdr.payload_size != expected_payload_size)
    return 0;
  if (hdr.mapper != current_mapper)
    return 0;
  return filesize == sizeof(nesticle_state_header) + hdr.payload_size;
}

int nesticle_make_state_path(const char *romfilename, char *out,
                             unsigned out_size) {
  if (!romfilename || !out || !out_size)
    return 0;
  int written = snprintf(out, out_size, "%s.sta", romfilename);
  return written > 0 && (unsigned)written < out_size;
}
