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

#ifndef WWISE_H
#define WWISE_H

#include <brrtools/brrapi.h>
#include <brrtools/brrtypes.h>

#include "riff.h"

BRRCPPSTART

typedef struct wwise_vorb_implicit {
	brru4 sample_count;
	brru4 mod_signal;
	brru1 skipped_0[8];
	brru4 setup_packet_offset;
	brru4 audio_start_offset;
	brru1 skipped_1[12];
	brru4 uid;
	brru1 blocksize[2];
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
	brru1 blocksize[2];
	brru1 unknown[2];
} wwise_vorb_extraT;
typedef struct wwise_vorb {
	brru4 sample_count;
	brru4 mod_signal;
	brru4 setup_packet_offset;
	brru4 audio_start_offset;
	brru4 uid;
	brru1 blocksize[2];
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
	struct {
		brru4 data1;
		brru2 data2;
		brru2 data3;
		brru1 data4[8];
	} guid;
} wwise_fmtT;

typedef struct wwise_wem {
	unsigned char *data;
	brru4 data_size;
	wwise_vorbT vorb;
	wwise_fmtT fmt;
} wwise_wemT;

#define WEM_SUCCESS 1
#define WEM_INCOMPLETE 0
#define WEM_ERROR -1
#define WEM_DUPLICATE -2
#define WEM_NOT_VORBIS -3

/* -2 : duplicate chunks
 * -1 : error (input)
 *  0 : insufficient data/missing chunks
 *  1 : success
 * */
int BRRCALL wwise_wem_init(wwise_wemT *const wem, const riffT *const rf);
void BRRCALL wwise_wem_clear(wwise_wemT *const wem);

BRRCPPEND

#endif /* WWISE_H */
