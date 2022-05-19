#ifndef PACKER_H
#define PACKER_H

/* Wrapper around oggpack for convenience */

#include <ogg/ogg.h>

#include <brrtools/brrapi.h>
#include <brrtools/brrtypes.h>

#define packer_unpack oggpack_read
#define packer_pack oggpack_write

long packer_transfer(oggpack_buffer *const unpacker, int unpack_bits,
    oggpack_buffer *const packer, int pack_bits);
long packer_transfer_remaining(oggpack_buffer *const unpacker, oggpack_buffer *const packer);
long packer_transfer_lots(oggpack_buffer *const unpacker, oggpack_buffer *const packer, long bits);

#endif /* PACKER_H */
