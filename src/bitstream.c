/*
Copyright 2021 BowToes (bow.toes@mailfence.com)

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

#include "bitstream.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrtil.h>

#define _bitflag(_i_, _n_) ((_i_) & (1 << (_n_)))
#define _bitval(_i_, _n_) (_bitflag(_i_, _n_) >> (_n_))
#define _bitset(_i_, _n_, _v_) ((_i_) = ((_i_) & ~(1 << (_n_))) | ((_v_) << (_n_)))

/* round to lowest multiple of 8 (2^3) >= _b_, aka the next whole byte */
#define _nextwhole(_b_) (((_b_) + 7) & ~7)

static brru1 BRRCALL
i_copy_bits(brru1 *out, brru1 in, brru1 ostart, brru1 istart, brru1 len)
{
	brru1 i = 0;
	brru1 o = *out;
	if (len > 8)
		len = 8;
	for (brru1 j = 1 << ostart, k = istart; i < len && k < 8 && j > 0; ++i, j <<= 1, ++k) {
		o = (o & ~j) | (_bitval(in, k) & j);
	}
	*out = o;
	return i;
}
static BRRCALL int
i_eos(const bitstream_stateT *const bs)
{
	return bitplot_bits(bs->position) >= bitplot_bits(bs->length);
}
static BRRCALL brru8
i_bits_left(const bitstream_stateT *const bs)
{
	return bitplot_bits(bs->length) - bitplot_bits(bs->position);
}

int BRRCALL
bitstream_check(const bitstream_stateT *const bs, const void *const stream)
{
	if (!bs || !stream || i_eos(bs) || !stream)
		return -1;
	return 0;
}
int BRRCALL
bitstream_init(bitstream_stateT *const bs, brru8 bits)
{
	if (!bs)
		return -1;
	bs->length = bitplot_new(bits);
	return 0;
}
void BRRCALL
bitstream_clear(bitstream_stateT *const bs)
{
	if (bs) {
		memset(bs, 0, sizeof(*bs));
	}
}

/* copy_in and copy_out are practically identical; the stream validity check and
 * actual bit copying might each be separated into static functions or the like */
int BRRCALL
bitstream_copy_out(bitstream_stateT *const bs,
    const void *restrict const stream, void *restrict const buffer,
    brru8 bits, brru8 *const copied)
{
	const brru1 *st = (const brru1 *)stream;
	brru1 *buff = (brru1 *)buffer;
	brru8 did_copy = 0;
	if (bitstream_check(bs, stream) || !buffer) {
		return BITSTREAM_ERROR;
	} else if (!bits) {
		if (copied)
			*copied = 0;
		return BITSTREAM_SUCCESS;
	} else {
		brru8 left = i_bits_left(bs);
		if (left < bits) {
			if (copied)
				*copied = bits - left;
			return BITSTREAM_INCOMPLETE;
		}
	}
	for (brru8 bydx = 0; did_copy < bits; ++bydx) {
		brru1 tb = 0, pb = bs->position.bit;
		for (;pb < 8 && tb < 8 && did_copy < bits; ++pb, ++tb, ++did_copy) {
			_bitset(buff[bydx], tb, _bitval(st[bs->position.byte], pb));
		}
		if (pb == 8) {
			pb = 0;
			bs->position.byte++;
		}
		if (did_copy < bits) {
			for (;pb < 8 && tb < 8 && did_copy < bits; ++pb, ++tb, ++did_copy) {
				_bitset(buff[bydx], tb, _bitval(st[bs->position.byte], pb));
			}
		}
		bs->position.bit = pb;
	}
	if (copied)
		*copied = did_copy;
	return BITSTREAM_SUCCESS;
}
int BRRCALL
bitstream_copy_in(bitstream_stateT *const bs,
    void *restrict const stream, const void *restrict const buffer,
    brru8 bits, brru8 *const copied)
{
	const brru1 *buff = (const brru1 *)buffer;
	brru1 *st = (brru1 *)stream;
	brru8 did_copy = 0;
	if (!bs || !stream || i_eos(bs) || !buffer) {
		return BITSTREAM_ERROR;
	} else if (!bits) {
		if (copied)
			*copied = 0;
		return BITSTREAM_SUCCESS;
	} else {
		brru8 left = i_bits_left(bs);
		if (left < bits) {
			if (copied)
				*copied = bits - left;
			return BITSTREAM_TRUNCATED;
		}
	}
	for (brru8 bydx = 0; did_copy < bits; ++bydx) {
		brru1 tb = 0, pb = bs->position.bit;
		for (;pb < 8 && tb < 8 && did_copy < bits; ++pb, ++tb, ++did_copy) {
			_bitset(st[bs->position.byte], pb, _bitval(buff[bydx], tb));
		}
		if (pb == 8) {
			pb = 0;
			bs->position.byte++;
		}
		if (did_copy < bits) {
			for (;pb < 8 && tb < 8 && did_copy < bits; ++pb, ++tb, ++did_copy) {
				_bitset(st[bs->position.byte], pb, _bitval(buff[bydx], tb));
			}
		}
		bs->position.bit = pb;
	}
	if (copied)
		*copied = did_copy;
	return BITSTREAM_SUCCESS;
}
int BRRCALL
bitstream_copy(bitstream_stateT *restrict const os, void *restrict const out,
    bitstream_stateT *restrict const is, const void *restrict const in,
    brru8 bits, brru8 *const copied)
{
	brru1 *buff = NULL;
	brru8 buflen = 0, did_copy = 0;
	int err = BITSTREAM_SUCCESS;
	if (!os || !out || !is || !in || i_eos(is) || i_eos(os)) {
		return BITSTREAM_ERROR;
	} else if (!bits) {
		if (copied)
			*copied = 0;
		return BITSTREAM_SUCCESS;
	}
	buflen = _nextwhole(bits) >> 3;
	if (brrlib_alloc((void **)&buff, buflen, 1))
		return BITSTREAM_ERROR;

	/* copy in to buff */
	if (BITSTREAM_SUCCESS == (err = bitstream_copy_out(is, in, buff, bits, &did_copy))) {
		/* copy buff to out */
		err = bitstream_copy_in(os, out, buff, bits, &did_copy);
	}

	brrlib_alloc((void **)&buff, 0, 0);
	if (copied)
		*copied = did_copy;
	return err;
}

int BRRCALL
bitstream_resize(bitstream_stateT *const bs, brru8 bits)
{
	if (!bs)
		return -1;
	bs->length = bitplot_new(bits);
	return 0;
}
int BRRCALL
bitstream_grow(bitstream_stateT *const bs, brru8 bits)
{
	if (!bs) {
		return -1;
	} else {
		brru8 n = bitplot_bits(bs->length);
		if (BRRTOOLS_BRRU8_MAX - n < bits) /* overflow */
			return -1;
		return bitstream_resize(bs, n + bits);
	}
}
int BRRCALL
bitstream_shrink(bitstream_stateT *const bs, brru8 bits)
{
	if (!bs) {
		return -1;
	} else {
		brru8 n = bitplot_bits(bs->length);
		if (n < bits) /* overflow */
			return -1;
		return bitstream_resize(bs, n - bits);
	}
}

int BRRCALL
bitstream_seek(bitstream_stateT *const bs, brru8 bits)
{
	if (bs) {
		brru8 n = bits;
		if (n > bitplot_bits(bs->length)) {
			bs->position = bs->length;
			return 1;
		} else {
			bs->position = bitplot_new(n);
			return 0;
		}
	}
	return -1;
}
int BRRCALL
bitstream_seek_up(bitstream_stateT *const bs, brru8 bits)
{
	/* Note: this doesn't do much to check for overflow */
	if (bs) {
		brru8 n = bitplot_bits(bs->position);
		if (bits > bitplot_bits(bs->length) - n) {
			bs->position = bs->length;
			return 1;
		} else {
			bs->position = bitplot_new(bits + n);
			return 0;
		}
	}
	return -1;
}
int BRRCALL
bitstream_seek_down(bitstream_stateT *const bs, brru8 bits)
{
	/* Note: this doesn't do much to check for overflow */
	if (bs) {
		brru8 n = bitplot_bits(bs->position);
		if (bits > n) {
			bs->position = (bitplotT){0};
			return 1;
		} else {
			bs->position = bitplot_new(n - bits);
			return 0;
		}
	}
	return -1;
}
