#ifndef NESTICLE_SAVE_STATE_FORMAT_H
#define NESTICLE_SAVE_STATE_FORMAT_H

struct nesticle_state_header {
  char magic[4];
  unsigned version;
  unsigned payload_size;
  unsigned mapper;
  unsigned flags;
};

#define NESTICLE_STATE_VERSION 1

void nesticle_init_state_header(nesticle_state_header &hdr,
                                unsigned payload_size, unsigned mapper);
int nesticle_valid_state_header(const nesticle_state_header &hdr,
                                unsigned filesize, unsigned current_mapper,
                                unsigned expected_payload_size);
int nesticle_make_state_path(const char *romfilename, char *out,
                             unsigned out_size);

#endif
