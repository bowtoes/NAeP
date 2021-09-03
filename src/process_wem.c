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

#include <brrtools/brrlib.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrpath.h>
#include <brrtools/brrplatform.h>
#include <brrtools/brrmem.h>

#include "riff.h"

typedef struct wwise_vorb_implicit {
	brru4 sample_count;
	brru4 mod_signal;
	brru1 skipped_0[8];
	brru4 setup_packet_offset;
	brru4 audio_start_offset;
	brru1 skipped_1[12];
	brru4 uid;
	brru1 blocksize_bits[2];
	brru1 unknown[2];
} wwise_vorb_implicitT;
typedef struct wwise_vorb_basic {
	brru4 sample_count;
	brru1 skipped_0[20];
	brru4 setup_packet_offset;
	brru4 audio_start_offset;
	brru1 unknown[12];
} wwise_vorb_basicT;
typedef struct wwise_vorb_extra {
	brru4 sample_count;
	brru1 skipped_0[20];
	brru4 setup_packet_offset;
	brru4 audio_start_offset;
	brru1 skipped_1[12];
	brru4 uid;
	brru1 blocksize_bits[2];
	brru1 unknown[2];
} wwise_vorb_extraT;
typedef union wwise_vorb {
	brru4 sample_count;
	wwise_vorb_implicitT implicit;
	wwise_vorb_basicT basic;
	wwise_vorb_extraT extra;
} wwise_vorbT;

typedef struct wwise_fmt {
	brru2 format_tag;
	brru2 n_channels;
	brru4 samples_per_sec;
	brru4 avg_byte_rate;
	brru2 block_align;
	brru2 bits_per_sample;
	brru2 extra_size;
	union {
		brru2 valid_bits_per_sample;
		brru2 samples_per_block;
		brru2 reserved;
	};
	brru4 channel_mask;
	union {
		struct {
			brru4 data1;
			brru2 data2;
			brru2 data3;
			brru1 data4[8];
		} guid;
		wwise_vorbT vorb;
	};
} wwise_fmtT;

typedef enum wwise_vorb_type {
	wwise_vorb_type_basic = 0,
	wwise_vorb_type_extra,
	wwise_vorb_type_implicit,
} wwise_vorb_typeT;
typedef struct wwise_wem {
	wwise_fmtT fmt;
	wwise_vorb_typeT vorb_type;
	wwise_vorbT vorb;
} wwise_wemT;

static void BRRCALL
i_print_wem(const wwise_wemT *const wem)
{
	if (!wem)
		return;
	BRRLOG_NOR("fmt data:");
	BRRLOG_NOR("fmt>     format_tag : 0x%04X", wem->fmt.format_tag);
	BRRLOG_NOR("fmt>     n_channels : %u", wem->fmt.n_channels);
	BRRLOG_NOR("fmt>samples_per_sec : %lu", wem->fmt.samples_per_sec);
	BRRLOG_NOR("fmt>  avg_byte_rate : %lu", wem->fmt.avg_byte_rate);
	BRRLOG_NOR("fmt>    block_align : %u", wem->fmt.block_align);
	BRRLOG_NOR("fmt>bits_per_sample : %u", wem->fmt.bits_per_sample);
	BRRLOG_NOR("fmt>     extra_size : %u", wem->fmt.extra_size);
	if (wem->fmt.extra_size) {
		BRRLOG_NOR("fmt>extra>    reserved : %u", wem->fmt.reserved);
		BRRLOG_NOR("fmt>extra>channel_mask : %lu", wem->fmt.channel_mask);
		if (wem->fmt.extra_size == 22) {
			BRRLOG_NOR("fmt>extra>        guid>data1 : %lu", wem->fmt.guid.data1);
			BRRLOG_NOR("fmt>extra>        guid>data2 : %u", wem->fmt.guid.data2);
			BRRLOG_NOR("fmt>extra>        guid>data3 : %u", wem->fmt.guid.data3);
			BRRLOG_NOR("fmt>extra>        guid>data4 : 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
			    wem->fmt.guid.data4[0], wem->fmt.guid.data4[1], wem->fmt.guid.data4[2], wem->fmt.guid.data4[3],
			    wem->fmt.guid.data4[4], wem->fmt.guid.data4[5], wem->fmt.guid.data4[6], wem->fmt.guid.data4[7]);
		} else if (wem->fmt.extra_size > 22) {
			BRRLOG_NOR("fmt>extra == vorb");
		}
	}
	BRRLOG_NOR("vorb data:");
	BRRLOG_NOR("vorb>       sample_count : %lu", wem->vorb.sample_count);
	if (wem->vorb_type == wwise_vorb_type_implicit) {
	BRRLOG_NOR("vorb>         mod_signal : %lu", wem->vorb.implicit.mod_signal);
	}
	if (wem->vorb_type == wwise_vorb_type_implicit) {
	BRRLOG_NOR("vorb>setup_packet_offset : 0x%08X", wem->vorb.implicit.setup_packet_offset);
	BRRLOG_NOR("vorb> audio_start_offset : 0x%08X", wem->vorb.implicit.audio_start_offset);
	} else {
	BRRLOG_NOR("vorb>setup_packet_offset : 0x%08X", wem->vorb.extra.setup_packet_offset);
	BRRLOG_NOR("vorb> audio_start_offset : 0x%08X", wem->vorb.extra.audio_start_offset);
	}
	if (wem->vorb_type == wwise_vorb_type_implicit) {
	BRRLOG_NOR("vorb>                uid : 0x%08X", wem->vorb.implicit.uid);
	BRRLOG_NOR("vorb>   blocksize_0_bits : %u", wem->vorb.implicit.blocksize_bits[0]);
	BRRLOG_NOR("vorb>   blocksize_1_bits : %u", wem->vorb.implicit.blocksize_bits[1]);
	} else if (wem->vorb_type == wwise_vorb_type_extra) {
	BRRLOG_NOR("vorb>                uid : 0x%08X", wem->vorb.extra.uid);
	BRRLOG_NOR("vorb>   blocksize_0_bits : %u", wem->vorb.extra.blocksize_bits[0]);
	BRRLOG_NOR("vorb>   blocksize_1_bits : %u", wem->vorb.extra.blocksize_bits[1]);
	}
}

#define RIFF_BUFFER_APPLY_SUCCESS 0
#define RIFF_BUFFER_APPLY_FAILURE -1

#define RIFF_BUFFER_INCREMENT 4096

static void BRRCALL
i_clear(FILE **restrict const in, FILE **restrict const out, riff_data_syncT *const sync_data,
    riffT *const rf)
{
	if (in) {
		if (*in)
			fclose(*in);
		*in = NULL;
	}
	if (out) {
		if (*out)
			fclose(*out);
		*out = NULL;
	}
	if (sync_data)
		riff_data_sync_clear(sync_data);
	if (rf)
		riff_clear(rf);
}
static int BRRCALL
i_consume_next_chunk(FILE *const file, riffT *const rf, riff_chunk_infoT *const sc, riff_data_syncT *const ds)
{
	int err = 0;
	brrsz bytes_read = 0;
	char *buffer = NULL;
	while (RIFF_CHUNK_CONSUMED != (err = riff_consume_chunk(rf, sc, ds))) {
		if (err == RIFF_CONSUME_MORE) {
			continue;
		} else if (err == RIFF_CHUNK_UNRECOGNIZED) {
			//BRRLOG_WAR("Skipping unrecognized chunk '%s' (%zu)", FCC_INT_CODE(sc->chunkcc), sc->chunksize);
			continue;
		} else if (err != RIFF_CHUNK_INCOMPLETE) {
			if (err == RIFF_ERROR)
				return I_BUFFER_ERROR;
			else if (err == RIFF_NOT_RIFF)
				return I_NOT_RIFF;
			else if (err == RIFF_CORRUPTED)
				return I_CORRUPT;
			else
				return I_BAD_ERROR - err;
		} else if (feof(file)) {
			if (sc->chunksize)
				return I_FILE_TRUNCATED;
			break;
		} else if (!(buffer = riff_data_sync_buffer(ds, RIFF_BUFFER_INCREMENT))) {
			return I_BUFFER_ERROR;
		}
		bytes_read = fread(buffer, 1, RIFF_BUFFER_INCREMENT, file);
		if (ferror(file)) {
			return I_IO_ERROR;
		} else if (RIFF_BUFFER_APPLY_SUCCESS != riff_data_sync_apply(ds, bytes_read)) {
			return I_BUFFER_ERROR;
		}
	}
	return I_SUCCESS;
}
static int BRRCALL
int_convert_wem(const char *const input, const char *const output)
{
	int err = 0;
	FILE *in, *out;
	riff_chunk_infoT sync_chunk = {0};
	riff_data_syncT sync_data = {0};
	riffT rf;
	wwise_wemT wem;

	if (!(in = fopen(input, "rb"))) {
		BRRLOG_ERRN("Failed to open wem for conversion input '%s' : %s", input, strerror(errno));
		return I_IO_ERROR;
	}

	riff_init(&rf);
	while (I_SUCCESS == (err = i_consume_next_chunk(in, &rf, &sync_chunk, &sync_data)) && (sync_chunk.is_basic || sync_chunk.is_list)) {
		riff_chunk_info_clear(&sync_chunk);
	}
	if (err != I_SUCCESS) {
		BRRLOG_ERRN("Failed to consume RIFF chunk : %s ", i_strerr(err));
		i_clear(&in, NULL, &sync_data, &rf);
		return err;
	}
	i_clear(&in, NULL, &sync_data, NULL);
	for (brru8 i = 0; i < rf.n_basics; ++i) {
		riff_basic_chunkT *basic = &rf.basics[i];
		if (basic->type == riff_basic_type_fmt) {
			wem.fmt = *(wwise_fmtT *)basic->data;
			if (basic->size >= 66) {
				wem.vorb = wem.fmt.vorb;
				wem.vorb_type = wwise_vorb_type_implicit;
			}
		} else if (basic->type == riff_basic_type_vorb) {
			wem.vorb = *(wwise_vorbT *)basic->data;
			if (basic->size < 50)
				wem.vorb_type = wwise_vorb_type_basic;
			else
				wem.vorb_type = wwise_vorb_type_extra;
		}
	}

	i_print_wem(&wem);

	riff_clear(&rf);
	return err;
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
		BRRLOG_FORENP(WET_COLOR, " Converting WEM... ");
		replace_ext(path, &inlen, output, &outlen, ".txt");
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
		BRRLOG_MESSAGETP(gbrrlog_level_last, SUCCESS_FORMAT, "Success!");
	} else {
		/* remove 'output' */
		BRRLOG_MESSAGETP(gbrrlog_level_last, FAILURE_FORMAT, "Failure! (%d)", err);
	}
	return err;
}
