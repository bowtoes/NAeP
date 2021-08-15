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

#include "process_files.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include <brrtools/brrlog.h>
#include <brrtools/brrpath.h>
#include <brrtools/brrmem.h>

typedef struct wem_chunk {
	fourccT fcc;
	brru4 size;
} wem_chunkT;
#define WEM_CHUNK_HEAD(_n_) \
union { \
    struct { \
        fourccT fcc; \
        brru4 size; \
    }; \
    wem_chunkT _n_; \
}
typedef struct wem_riff_chunk {
	WEM_CHUNK_HEAD(chunk_head);
	fourccT type;
} wem_riff_chunkT;
static const fourccT riff_chunk_fcc = FCC_MAKE(RIFF);
static const fourccT rifx_chunk_fcc = FCC_MAKE(RIFX);
static const fourccT wave_type_fcc = FCC_MAKE(WAVE);

typedef struct wem_fmt_chunk {
	WEM_CHUNK_HEAD(chunk_head);
	brru2 format_tag;
	brru2 channel_count;
	brru4 sample_rate;
	brru4 avg_byte_rate;
	brru2 block_align;
	brru2 bits_per_sample;
	brru2 extra_length;
	brru2 padding;
} wem_fmt_chunkT;
static const fourccT fmt_chunk_fcc = FCC_INIT("fmt ");

static int BRRCALL
int_convert_wem(const char *const input, const char *const output)
{
	FILE *in = NULL;
	wem_riff_chunkT riff = {0};
	wem_fmt_chunkT fmt = {0};
	brrby *extra_fmt = NULL;
	int endian_need_swap = BRRENDIAN_SYSTEM != BRRENDIAN_LITTLE; /* assuming little endian riff data */
	if (!(in = fopen(input, "rb"))) {
		BRRLOG_ERRN("Failed to open WEM '%s' for conversion input : %s", input, strerror(errno));
		return -1;
	} else if (12 != fread(&riff, 1, 12, in)) {
		BRRLOG_ERRN("Failed to read 12 bytes of RIFF chunk of WEM '%s' : %s", input, strerror(errno));
		fclose(in);
		return -1;
	} else if (riff.fcc.integer != riff_chunk_fcc.integer) {
		if (riff.fcc.integer == rifx_chunk_fcc.integer) {
			endian_need_swap = BRRENDIAN_SYSTEM != BRRENDIAN_BIG;
			if (endian_need_swap)
				riff.size = SWAP_4(riff.size);
		} else {
			BRRLOG_ERRN("WEM '%s' had invalid fourcc '%s'", FCC_AS_STR(riff.fcc));
			fclose(in);
			return -1;
		}
	} else if (8 != fread(&fmt, 1, 8, in)) {
		BRRLOG_ERRN("Failed to read 8 bytes of next chunk of WEM '%s' : %s", input, strerror(errno));
		fclose(in);
		return -1;
	} else {
		if (fmt.fcc.integer == fmt_chunk_fcc.integer) {
			brrsz size_to_read = sizeof(wem_fmt_chunkT) - 8;
			if (size_to_read != read_to_offset(&fmt, 8, size_to_read, in)) {
				BRRLOG_ERRN("Failed to read %zu bytes of fmt chunk in WEM '%s' : %s", size_to_read, input, strerror(errno));
				fclose(in);
				return -1;
			}
			if (endian_need_swap) {
				fmt.size            = SWAP_4(fmt.size);
				fmt.format_tag      = SWAP_2(fmt.format_tag);
				fmt.channel_count   = SWAP_2(fmt.channel_count);
				fmt.sample_rate     = SWAP_4(fmt.sample_rate);
				fmt.avg_byte_rate   = SWAP_4(fmt.avg_byte_rate);
				fmt.block_align     = SWAP_2(fmt.block_align);
				fmt.bits_per_sample = SWAP_2(fmt.bits_per_sample);
				fmt.extra_length    = SWAP_2(fmt.extra_length);
			}
			BRRLOG_NOR("\nGot fmt chunk:");
			BRRLOG_NOR("    fcc             : %c%c%c%c 0x%02X 0x%02X 0x%02X 0x%02x", FCC_GET_BYTES(fmt.fcc), FCC_GET_BYTES(fmt.fcc));
			BRRLOG_NOR("    size            : %zu", fmt.size);
			BRRLOG_NOR("    format_tag      : 0x%04X");
			BRRLOG_NOR("    channel_count   : %zu", fmt.channel_count);
			BRRLOG_NOR("    sample_rate     : %zu", fmt.sample_rate);
			BRRLOG_NOR("    avg_byte_rate   : %zu", fmt.avg_byte_rate);
			BRRLOG_NOR("    block_align     : %zu", fmt.block_align);
			BRRLOG_NOR("    bits_per_sample : %zu", fmt.bits_per_sample);
			BRRLOG_NOR("    extra_length    : %zu", fmt.extra_length);
		} else {
			BRRLOG_WARN("Got unexpected second chunk '%s'", FCC_AS_STR(fmt.fcc));
		}
	}

	fclose(in);

	return 0;
}
int BRRCALL
convert_wem(numbersT *const numbers, int dry_run, const char *const path,
    int inplace_regranularize, int inplace_ogg)
{
	static char output[BRRPATH_MAX_PATH + 1] = {0};
	int err = 0;
	numbers->wems_to_convert++;
	if (dry_run) {
		BRRLOG_FORENP(DRY_COLOR, " Convert WEM");
	} else {
		brrsz outlen = 0, inlen = 0;
		NeTODO("Implement 'convert_wem' priority 1 ");
		BRRLOG_FORENP(WET_COLOR, " Converting WEM... ");
		replace_ext(path, &inlen, output, &outlen, ".ogg");
		err = int_convert_wem(path, output);
		if (!err) {
			if (inplace_ogg) {
				NeTODO("WEM CONVERT IN-PLACE");
				/* remove 'path' and rename 'output' to 'path' */
			}
		}
	}
	if (!err) {
		numbers->wems_converted++;
		BRRLOG_MESSAGETP(gbrrlog_level_last, SUCCESS_FORMAT, " Success!");
	} else {
		/* remove 'output' */
		NeTODO("WEM ERROR REMOVE OUTPUT");
		BRRLOG_MESSAGETP(gbrrlog_level_last, FAILURE_FORMAT, " Failure! (%d)", err);
	}
	return err;
}
