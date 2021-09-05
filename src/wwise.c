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

static void BRRCALL
i_init_vorb(wwise_vorbT *const vorb, const unsigned char *const data, brru4 data_size)
{
	if (data_size == 42) { /* Implicit type */
		wwise_vorb_implicitT i = {0};
		memcpy(&i, data, 42);
		vorb->sample_count = i.sample_count;
		vorb->mod_signal = i.mod_signal;
		vorb->setup_packet_offset = i.setup_packet_offset;
		vorb->audio_start_offset = i.audio_start_offset;
		vorb->uid = i.uid;
		vorb->blocksize[0] = i.blocksize[0];
		vorb->blocksize[1] = i.blocksize[1];
	} else { /* Explicit type */
		wwise_vorb_extraT e = {0};
		memcpy(&e, data, brrlib_umin(data_size, sizeof(e)));
		vorb->sample_count = e.sample_count;
		vorb->setup_packet_offset = e.setup_packet_offset;
		vorb->audio_start_offset = e.audio_start_offset;
		vorb->uid = e.uid;
		vorb->blocksize[0] = e.blocksize[0];
		vorb->blocksize[1] = e.blocksize[1];
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
	brru1 init_fmt = 0, init_vorb = 0, init_data = 0;
	if (!wem || !rf)
		return WEM_ERROR;
	for (brru8 i = 0; i < rf->n_basics; ++i) {
		riff_basic_chunkT *basic = &rf->basics[i];
		if (basic->type == riff_basic_type_fmt) {
			if (init_fmt)
				return WEM_DUPLICATE;
			i_init_fmt(&wem->fmt, basic->data, basic->size);
			init_fmt = 1;
			if (basic->size >= 66) {
				if (init_vorb)
					return WEM_DUPLICATE;
				i_init_vorb(&wem->vorb, basic->data + 24, 42);
				init_vorb = 1;
			}
		} else if (basic->type == riff_basic_type_vorb) {
			if (init_vorb)
				return WEM_DUPLICATE;
			i_init_vorb(&wem->vorb, basic->data, basic->size);
			init_vorb = 1;
		} else if (basic->type == riff_basic_type_data) {
			if (init_data)
				return WEM_DUPLICATE;
			wem->data = basic->data;
			wem->data_size = basic->size;
			init_data = 1;
		}
	}
	if (!init_fmt || !init_data || !init_vorb)
		return WEM_INCOMPLETE;
	return WEM_SUCCESS;
}
void BRRCALL
wwise_wem_clear(wwise_wemT *const wem)
{
	if (wem) {
		memset(wem, 0, sizeof(*wem));
	}
}

