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

typedef struct wwise_vorb {
	brru4 sample_count;
	brru4 mod_signal;
	brru4 header_packets_offset;
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
typedef struct wwise_packet {
	brru8 payload_size:16;
	brru8 granule:32;
	brru8 unused:16;
	unsigned char *payload;
	int header_length;
} wwise_packetT;
typedef struct wwise_wem {
	brru1 fmt_initialized:1;
	brru1 vorb_initialized:1;
	brru1 data_initialized:1;
	brru1 mod_packets:1; /* No idea what this means */
	brru1 granule_present:1; /* Whether data packets have 4-bytes for granule */
	brru1 all_headers_present:1; /* If all vorbis headers are present at
									header_packets_offset or it's just the
									setup header */
	brru1 mode_blockflags[32]; /* Storage for audio packet decode */
	int mode_count; /* Storage for audio packet decode */
	unsigned char *data;
	brru4 data_size;
	wwise_vorbT vorb;
	wwise_fmtT fmt;
} wwise_wemT;

#define WWISE_SUCCESS 1
#define WWISE_INCOMPLETE 0
#define WWISE_ERROR -1
#define WWISE_DUPLICATE -2
#define WWISE_CORRUPT -3

/* -3 : corrupted stream
 * -2 : duplicate data
 * -1 : error (input)
 *  0 : insufficient data/missing chunks
 *  1 : success
 * */
int BRRCALL wwise_wem_init(wwise_wemT *const wem, const riffT *const rf);
void BRRCALL wwise_wem_clear(wwise_wemT *const wem);

/* -1 : error (input)
 *  0 : insufficient data
 *  1 : success
 * */
int BRRCALL wwise_packet_init(wwise_packetT *const packet,
    const wwise_wemT *const wem, const unsigned char *const data, brrsz data_size);
void BRRCALL wwise_packet_clear(wwise_packetT *const packet);

BRRCPPEND

#endif /* WWISE_H */