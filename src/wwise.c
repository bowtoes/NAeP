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

#include "wwise.h"

#include <string.h>

#include <brrtools/brrlib.h>

typedef struct wwise_vorb_implicit {
	brru4 sample_count;
	brru4 mod_signal;
	brru1 skipped_0[8];
	brru4 header_packets_offset;
	brru4 audio_start_offset;
	brru1 skipped_1[12];
	brru4 uid;
	brru1 blocksize[2];
} wwise_vorb_implicitT;
typedef struct wwise_vorb_basic {
	brru4 sample_count;
	brru1 skipped_0[20];
	brru4 header_packets_offset;
	brru4 audio_start_offset;
	brru1 unknown[12];
} wwise_vorb_basicT;
typedef struct wwise_vorb_extra {
	brru4 sample_count;
	brru1 skipped_0[20];
	brru4 header_packets_offset;
	brru4 audio_start_offset;
	brru1 skipped_1[12];
	brru4 uid;
	brru1 blocksize[2];
	brru1 unknown[2];
} wwise_vorb_extraT;

static void BRRCALL
i_init_vorb(wwise_wemT *const wem, const unsigned char *const data, brru4 data_size)
{
	if (data_size == 42) { /* Implicit type */
		wwise_vorb_implicitT i = {0};
		memcpy(&i, data, 42);
		wem->vorb.sample_count = i.sample_count;
		wem->vorb.mod_signal = i.mod_signal;
		wem->vorb.header_packets_offset = i.header_packets_offset;
		wem->vorb.audio_start_offset = i.audio_start_offset;
		wem->vorb.uid = i.uid;
		wem->vorb.blocksize[0] = i.blocksize[0];
		wem->vorb.blocksize[1] = i.blocksize[1];

		/* from ww2ogg, no idea what they mean */
		if (i.mod_signal == 0x4a || i.mod_signal == 0x4b || i.mod_signal == 0x69 || i.mod_signal == 0x70) {
			wem->mod_packets = 0;
		} else {
			wem->mod_packets = 1;
		}
		wem->granule_present = 0;
		wem->all_headers_present = 0;
	} else { /* Explicit type */
		wwise_vorb_extraT e = {0};
		memcpy(&e, data, brrlib_umin(data_size, sizeof(e)));
		wem->vorb.sample_count = e.sample_count;
		wem->vorb.header_packets_offset = e.header_packets_offset;
		wem->vorb.audio_start_offset = e.audio_start_offset;
		wem->vorb.uid = e.uid;
		wem->vorb.blocksize[0] = e.blocksize[0];
		wem->vorb.blocksize[1] = e.blocksize[1];

		wem->mod_packets = 0;
		wem->granule_present = 1;
		wem->all_headers_present = (data_size <= 44);
	}
}
static void BRRCALL
i_init_fmt(wwise_fmtT *const fmt, const unsigned char *const data, brru4 data_size)
{
	memcpy(fmt, data, brrlib_umin(data_size, sizeof(*fmt)));
}
int BRRCALL
wwise_wem_init(wwise_wemT *const wem, const riffT *const rf)
{
	if (!wem || !rf)
		return WWISE_ERROR;
	memset(wem, 0, sizeof(*wem));
	for (brru8 i = 0; i < rf->n_basics; ++i) {
		riff_basic_chunkT *basic = &rf->basics[i];
		if (basic->type == riff_basic_fmt) {
			if (wem->fmt_initialized)
				return WWISE_DUPLICATE;
			i_init_fmt(&wem->fmt, basic->data, basic->size);
			wem->fmt_initialized = 1;
			if (basic->size == 66) { /* Vorb implicit in the fmt */
				if (wem->vorb_initialized)
					return WWISE_DUPLICATE;
				i_init_vorb(wem, basic->data + 24, basic->size - 24);
				wem->vorb_initialized = 1;
			}
		} else if (basic->type == riff_basic_vorb) { /* Vorb explicit */
			if (wem->vorb_initialized)
				return WWISE_DUPLICATE;
			i_init_vorb(wem, basic->data, basic->size);
			wem->vorb_initialized = 1;
		} else if (basic->type == riff_basic_data) {
			if (wem->data_initialized)
				return WWISE_DUPLICATE;
			wem->data = basic->data;
			wem->data_size = basic->size;
			wem->data_initialized = 1;
		}
	}
	if (!wem->fmt_initialized || !wem->vorb_initialized || !wem->data_initialized)
		return WWISE_INCOMPLETE;
	if (wem->vorb.header_packets_offset > wem->data_size ||
	    wem->vorb.audio_start_offset > wem->data_size)
		return WWISE_CORRUPT;
	return WWISE_SUCCESS;
}
void BRRCALL
wwise_wem_clear(wwise_wemT *const wem)
{
	if (wem) {
		memset(wem, 0, sizeof(*wem));
	}
}

int BRRCALL
wwise_packet_init(wwise_packetT *const packet,
    const wwise_wemT *const wem, const unsigned char *const data, brrsz data_size)
{
	brru1 ofs = 2;
	if (!packet || !wem || !data)
		return WWISE_ERROR;

	if (data_size < 2)
		return WWISE_INCOMPLETE;

	packet->payload_size = *(brru2 *)data;
	if (packet->payload_size > data_size)
		return WWISE_INCOMPLETE;

	if (wem->granule_present) {
		if (data_size < ofs + 4)
			return WWISE_INCOMPLETE;

		packet->granule = *(brru4 *)(data + ofs);
		ofs += 4;
		if (wem->all_headers_present) {
			if (data_size < ofs + 2)
				return WWISE_INCOMPLETE;

			packet->unused = *(brru2 *)(data + ofs);
			ofs += 2;
		}
	}
	packet->payload = (unsigned char *)data + ofs;
	packet->header_length = ofs;
	return WWISE_SUCCESS;
}
void BRRCALL
wwise_packet_clear(wwise_packetT *const packet)
{
	if (packet) {
		memset(packet, 0, sizeof(*packet));
	}
}
