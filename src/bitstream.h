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

#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <brrtools/brrapi.h>
#include <brrtools/brrtypes.h>

BRRCPPSTART

typedef union bitplot {
	struct {
		brru8 bit:3;
		brru8 byte:61;
	};
	struct {
		brru8 residue:3;
		brru8 floor:61;
	};
	brru8 integer;
} bitplotT;

#define bitplot_new(_b_) ((bitplotT){.byte=(_b_)>>3,.bit=(_b_)&0x7})
#define bitplot_bits(_p_) (((_p_).byte << 3) + (_p_).bit)
#define bitplot_add(_l_, _r_) bitplot_new(bitplot_bits(_l_) + bitplot_bits(_r_))
#define bitplot_sub(_l_, _r_) bitplot_new(bitplot_bits(_l_) - bitplot_bits(_r_))

typedef struct bitstream_state {
	bitplotT length;
	bitplotT position;
} bitstream_stateT;

/* -1 : Invalid stream
 *  0 : Valid stream
 * */
int BRRCALL bitstream_check(const bitstream_stateT *const bs, const void *const stream);
int BRRCALL bitstream_init(bitstream_stateT *const bs, brru8 bits);
void BRRCALL bitstream_clear(bitstream_stateT *const bs);

#define BITSTREAM_TRUNCATED -2
#define BITSTREAM_ERROR -1
#define BITSTREAM_INCOMPLETE 0
#define BITSTREAM_SUCCESS 1

/* -1 : Error (invalid input); stream unchanged
 *  0 : Input stream incomplete; # of necessary bits of additional data needed stored in 'copied'; stream unchanged
 *  1 : Success
 * */
int BRRCALL bitstream_copy_out(bitstream_stateT* const bs,
    const void *restrict const stream, void *restrict const buffer,
    brru8 bits, brru8 *const copied);
/* -2 : Destination stream truncated, # of necessary bits of additional storage needed stored in 'copied'; stream unchanged
 * -1 : Error (invalid input); stream unchanged
 *  1 : Success
 * */
int BRRCALL bitstream_copy_in(bitstream_stateT *const bs,
    void *restrict const stream, const void *restrict const buffer,
    brru8 bits, brru8 *const copied);
/*
 * -2 : Output stream truncated, # of necessary additional bits of storage stored in 'copied'; streams unchanged
 * -1 : Error (invalid input/allocation error); streams unchanged
 *  0 : Input stream incomplete; # of necessary additional bits of data stored in 'copied'; streams unchanged
 *  1 : Success
 * */
int BRRCALL bitstream_copy(bitstream_stateT *restrict const os, void *restrict const out,
    bitstream_stateT *restrict const is, const void *restrict const in,
    brru8 bits, brru8 *const copied);

int BRRCALL bitstream_write(bitstream_stateT *const bs,
    void *const stream, brru8 value, brru8 bits, brru8 *const copied);

/* -1 : Invalid stream
 *  0 : Success
 * Position unaffected.
 * */
int BRRCALL bitstream_resize(bitstream_stateT *const bs, brru8 bits);
/* -1 : Invalid stream/growth would overflow; stream unchanged
 *  0 : Success
 * Position unaffected.
 * */
int BRRCALL bitstream_grow(bitstream_stateT *const bs, brru8 bits);
/* -1 : Invalid stream/shrink would overflow; stream unchanged
 *  0 : Success
 * Position unaffected.
 * */
int BRRCALL bitstream_shrink(bitstream_stateT *const bs, brru8 bits);

/* -1 : Invalid stream; stream unchanged
 *  0 : Success
 *  1 : Past end of stream, clamped to end
 * */
int BRRCALL bitstream_seek(bitstream_stateT *const bs, brru8 bits);
/* -1 : Invalid stream; stream unchanged
 *  0 : Success
 *  1 : Past end of stream, clamped to end
 * */
int BRRCALL bitstream_seek_up(bitstream_stateT *const bs, brru8 bits);
/* -1 : Invalid stream; stream unchanged
 *  0 : Success
 *  1 : Before beginning of stream, clamped to beginning
 * */
int BRRCALL bitstream_seek_down(bitstream_stateT *const bs, brru8 bits);

BRRCPPEND

#endif /* BITSTREAM_H */
