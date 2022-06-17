/*
Copyright 2021-2022 BowToes (bow.toes@mailfence.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef NAeP_neutil_h
#define NAeP_neutil_h

#include <ogg/ogg.h>

#include <brrtools/brrtypes.h>

#define E_OGG_SUCCESS        ( 0)
#define E_OGG_FAILURE        (-1)
#define E_OGG_OUT_SUCCESS    ( 1)
#define E_OGG_OUT_INCOMPLETE ( 0)
#define E_OGG_OUT_DESYNC     (-1)
#define E_VORBIS_HEADER_SUCCESS   (0)
#define E_VORBIS_HEADER_FAULT     (OV_EFAULT)
#define E_VORBIS_HEADER_NOTVORBIS (OV_ENOTVORBIS)
#define E_VORBIS_HEADER_BADHEADER (OV_EBADHEADER)

#define nepack_unpack oggpack_read
inline brru4
nepack_pack(oggpack_buffer *const packer, brru4 value, int bits)
{
	oggpack_write(packer, value, bits);
	return value;
}

/* Read 'unpack' bits from 'unpacker', and place 'pack' bits from that into 'packer'.
 * Returns the value unpacked, or -1 on error. */
long long
nepack_transfer(oggpack_buffer *const unpacker, int unpack, oggpack_buffer *const packer, int pack);

/* Transfer what data remains in 'unpacker' into 'packer'.
 * Returns the number of bits transferred, or -1 on error.*/
long long
nepack_transfer_remaining(oggpack_buffer *const unpacker, oggpack_buffer *const packer);

/* Transfer 'bits' bits from 'unpacker' into 'packer' in 32-bit word chunks.
 * Returns the number of bits transferred, or -1 on error.*/
long long
nepack_transfer_lots(oggpack_buffer *const unpacker, oggpack_buffer *const packer, unsigned long bits);

long
neutil_lookup1(long entries, long dimensions);

/* Count how many bits of 'x' are set. */
int
neutil_count_ones(unsigned long x);

/* Count the maximum bit set in 'x'. */
int
neutil_count_bits(unsigned long x);

int
neutil_write_ogg(ogg_stream_state *const stream, const char *const file);

typedef struct riff riff_t;
int
neutil_buffer_to_riff(riff_t *const wwriff, const void *const buffer, brrsz buffer_size);

typedef struct wwriff wwriff_t;
int
neutil_buffer_to_wwriff(wwriff_t *const wwriff, const void *const buffer, brrsz buffer_size);

#endif /* NAeP_neutil_h */
