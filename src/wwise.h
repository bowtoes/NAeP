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

#ifndef WWISE_H
#define WWISE_H

#include <ogg/ogg.h>

#include <brrtools/brrtypes.h>
#include <brrtools/brrstringr.h>

#include "input.h"
#include "riff.h"

#define VORBIS_STR "vorbis"
#define CODEBOOK_SYNC "BCV"

typedef struct wwise_vorb {
	brru4 sample_count;
	brru4 mod_signal;
	brru4 header_packets_offset;
	brru4 audio_start_offset;
	brru4 uid;
	brru1 blocksize_0;
	brru1 blocksize_1;
} wwise_vorb_t;

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
} wwise_fmt_t;

typedef struct wwise_wem_flags {
	brru1 fmt_initialized:1;
	brru1 vorb_initialized:1;
	brru1 data_initialized:1;
	brru1 mod_packets:1;         /* No idea what this means */
	brru1 granule_present:1;     /* Whether data packets have 4-bytes for granule */
	brru1 all_headers_present:1; /* If all vorbis headers are present at header_packets_offset or it's just the setup header */
} wwriff_flags_t;

typedef struct wwise_packeteer {
// Bitfields to avoid padding
	brru8 payload_size:16;
	brru8 granule:32;
	brru8 unused:16;
	unsigned char *payload;
	int header_length;
	brru4 total_size;
} wwise_packeteer_t;

/* Initializes 'packet' from the wwise stream 'wem' with data 'data'.
 *  1 : success
 *  0 : insufficient data
 * -1 : error (input)
 * */
int wwise_packeteer_init(
    wwise_packeteer_t *const packeteer,
    const unsigned char *const data,
    brrsz data_size,
    wwriff_flags_t wem_flags
);
void wwise_packeteer_zero(wwise_packeteer_t *const packet);

typedef struct wwriff {
	wwriff_flags_t flags;
	int mode_count;              /* Storage for audio packet decode */
	brru1 mode_blockflags[32];   /* Storage for audio packet decode */
	unsigned char *data;
	brru4 data_size;
	wwise_vorb_t vorb;
	wwise_fmt_t fmt;
	brru4 n_comments;
	brrstringr_t *comments;
} wwriff_t;

/* Consumes the riff data 'rf', and parses it as WWRIFF data.
 * 'rf' is free to be cleared after initialization.
 * Returns:
 *  1 : success
 *  0 : insufficient data/missing chunks
 * -1 : error (input)
 * -2 : duplicate data
 * -3 : corrupted stream
 * */
int wwriff_init(wwriff_t *const wwriff, const riff_t *const rf);

/* Frees memory associated with 'wem', and clears it's data to 0. */
void wwriff_clear(wwriff_t *const wwriff);

int wwriff_add_comment(wwriff_t *const wwriff, const char *const format, ...);

/* Converts the wwriff data 'in_riff' to an ogg stream in 'out_stream', using codebooks from 'library'.
 * 'input' is for output stream metadata (like which file the output is converted from, etc.).
 * */
int wwise_convert_wwriff(
    wwriff_t *const in_wwriff,
    ogg_stream_state *const out_stream,
    const codebook_library_t *const library,
    const neinput_t *const input
);

#endif /* WWISE_H */
