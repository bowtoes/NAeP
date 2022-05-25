#ifndef PACKER_H
#define PACKER_H

/* Wrapper around oggpack for convenience */

#include <ogg/ogg.h>

#include <brrtools/brrapi.h>
#include <brrtools/brrtypes.h>

#define packer_unpack oggpack_read

inline long
packer_pack(oggpack_buffer *const packer, unsigned long value, int bits)
{
	oggpack_write(packer, value, bits);
	return value;
}

// Unpack 'unpack_bits' from unpacker into a signed 64-bit value, then transfer 'pack_bits' of that into packer, and return value, or -1 on error.
long packer_transfer(oggpack_buffer *const unpacker, int unpack_bits, oggpack_buffer *const packer, int pack_bits);
// Transfer all of unpacker to packer and return number of bits transferred, or -1 on error.
long packer_transfer_remaining(oggpack_buffer *const unpacker, oggpack_buffer *const packer);
// Transfer 32-bit dwords at a time and return number of bits transferred, or -1 on error.
long packer_transfer_lots(oggpack_buffer *const unpacker, oggpack_buffer *const packer, long bits);

#endif /* PACKER_H */
