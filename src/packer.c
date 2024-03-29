#include "packer.h"

long packer_pack(oggpack_buffer *const, unsigned long, int);

long
packer_transfer(oggpack_buffer *const unpacker, int unpack_bits,
    oggpack_buffer *const packer, int pack_bits)
{
	brrs8 val = -1;
	if (-1 == (val = packer_unpack(unpacker, unpack_bits)))
		return val;
	packer_pack(packer, val, pack_bits);
	return val;
}
long
packer_transfer_remaining(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	long dwords = 0;
	long transferred = 0;
	long left = 0;
	if (unpacker->endbit) {
		int bits = 8 - unpacker->endbit;
		if (-1 == packer_transfer(unpacker, bits, packer, bits))
			return -1;
		transferred += bits;
	}
	dwords = (unpacker->storage - unpacker->endbyte) >> 2;
	for (long i = 0; i < dwords; ++i) {
		if (-1 == packer_transfer(unpacker, 32, packer, 32))
			return -1;
		transferred += 32;
	}
	left = 8 * (unpacker->storage - unpacker->endbyte);
	if (left) {
		if (-1 == packer_transfer(unpacker, left, packer, left))
			return -1;
		transferred += left;
	}
	return transferred;
}
long
packer_transfer_lots(oggpack_buffer *const unpacker, oggpack_buffer *const packer, long bits)
{
	long dwords = 0;
	if (!packer || !unpacker)
		return -1;
	dwords = bits >> 5;
	for (long i = 0; i < dwords; ++i) {
		if (-1 == packer_transfer(unpacker, 32, packer, 32))
			return -1;
	}
	if (-1 == packer_transfer(unpacker, bits & 31, packer, bits & 31))
		return -1;
	return bits;
}
