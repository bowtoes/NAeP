#ifndef PACKER_H
#define PACKER_H

/* Wrapper around oggpack for convenience */

#include <ogg/ogg.h>

#include <brrtools/brrapi.h>
#include <brrtools/brrtypes.h>

BRRCPPSTART

#define packer_unpack oggpack_read
#define packer_pack oggpack_write

long BRRCALL packer_transfer(oggpack_buffer *const unpacker, int unpack_bits,
    oggpack_buffer *const packer, int pack_bits);
long BRRCALL packer_transfer_remaining(oggpack_buffer *const unpacker, oggpack_buffer *const packer);
long BRRCALL packer_transfer_lots(oggpack_buffer *const unpacker, oggpack_buffer *const packer, long bits);

BRRCPPEND

#endif /* PACKER_H */
