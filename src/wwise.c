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

#include "wwise.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <vorbis/vorbisenc.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrnum.h>
#include <brrtools/brrpath.h>

#include "errors.h"
#include "lib.h"
#include "packer.h"
#include "print.h"

#define COMMENT_MAX 1024

static const codebook_library_t *s_used_library = NULL;
static const neinput_t *s_current_input = NULL;

const char *const vorbis_header_packet_names[3] = {
	"ID",
	"comments",
	"setup",
};

typedef struct i_packeteer {
// Bitfields to avoid padding
	brru8 payload_size:16;
	brru8 granule:32;
	brru8 unused:16;
	unsigned char *payload;
	int header_length;
	brru4 total_size;
} i_packeteer_t;

/* Initializes 'packet' from the wwise stream 'wem' with data 'data'.
 *  1 : success
 *  0 : insufficient data
 * -1 : error (input)
 * */
static inline int
i_packeteer_init(
    i_packeteer_t *const packet,
    const unsigned char *const data,
    brrsz data_size,
    wwriff_flags_t wem_flags,
	brrsz offset
)
{
	if (!packet || !data)
		return I_INIT_ERROR;

	if (data_size < 2)
		return I_INSUFFICIENT_DATA;

	i_packeteer_t pk = {0};
	pk.payload_size = *(brru2 *)data;
	if (pk.payload_size > data_size)
		return I_INSUFFICIENT_DATA;

	brru1 ofs = 2;
	if (wem_flags.granule_present) {
		if (data_size < ofs + 4)
			return I_INSUFFICIENT_DATA;

		pk.granule = *(brru4 *)(data + ofs);
		ofs += 4;
		if (wem_flags.all_headers_present) {
			if (data_size < ofs + 2)
				return I_INSUFFICIENT_DATA;

			pk.unused = *(brru2 *)(data + ofs);
			ofs += 2;
		}
	}
	pk.payload = (unsigned char *)data + ofs;
	pk.header_length = ofs;
	pk.total_size = pk.payload_size + pk.header_length;
	//NeExtraPrint(DEB, "Init packet of size %llu (%llu header, %llu payload) at offset 0x%08x", pk.total_size, pk.header_length, pk.payload_size, offset);
	*packet = pk;
	return 0;
}

/* TODO packer_transfer can error; many transfers don't check for errors. */

// TODO please explain this
typedef struct wwise_vorb_implicit {
	brru4 sample_count;
	brru4 mod_signal;
	brru1 skipped_0[8];
	brru4 header_packets_offset;
	brru4 audio_start_offset;
	brru1 skipped_1[12];
	brru4 uid;
	brru1 blocksize_0;
	brru1 blocksize_1;
} wwise_vorb_implicit_t;

// TODO please explain this
typedef struct wwise_vorb_basic {
	brru4 sample_count;
	brru1 skipped_0[20];
	brru4 header_packets_offset;
	brru4 audio_start_offset;
	brru1 unknown[12];
} wwise_vorb_basic_t;

// TODO please explain this
typedef struct wwise_vorb_extra {
	brru4 sample_count;
	brru1 skipped_0[20];
	brru4 header_packets_offset;
	brru4 audio_start_offset;
	brru1 skipped_1[12];
	brru4 uid;
	brru1 blocksize_0;
	brru1 blocksize_1;
	brru1 unknown[2];
} wwise_vorb_extra_t;

static inline void
i_init_vorb(wwriff_t *const wem, const unsigned char *const data, brru4 data_size)
{
	wwriff_t w = *wem;
	if (data_size == 42) {
		/* Implicit type */
		wwise_vorb_implicit_t i = {0};
		memcpy(&i, data, 42);

		w.vorb.sample_count = i.sample_count;
		w.vorb.mod_signal = i.mod_signal;
		w.vorb.header_packets_offset = i.header_packets_offset;
		w.vorb.audio_start_offset = i.audio_start_offset;
		w.vorb.uid = i.uid;
		w.vorb.blocksize_0 = i.blocksize_0;
		w.vorb.blocksize_1 = i.blocksize_1;

		/* from ww2ogg, no idea what 'mod_packets' is supposed to mean */
		if (i.mod_signal == 0x4a || i.mod_signal == 0x4b || i.mod_signal == 0x69 || i.mod_signal == 0x70)
			w.flags.mod_packets = 0;
		else
			w.flags.mod_packets = 1;

		w.flags.granule_present = 0;
		w.flags.all_headers_present = 0;
	} else {
		/* Explicit type */
		wwise_vorb_extra_t e = {0};
#ifdef Ne_extra_debug
		if (data_size > sizeof(e)) {
			NeExtraPrint(DEBUG,
				"Explicit vorbis initialization header size is %zu bytes (expected at most %zu).",
				data_size, sizeof(e));
		}
#endif
		memcpy(&e, data, brrnum_umin(data_size, sizeof(e))); // TODO why this min?

		w.vorb.sample_count = e.sample_count;
		w.vorb.header_packets_offset = e.header_packets_offset;
		w.vorb.audio_start_offset = e.audio_start_offset;
		w.vorb.uid = e.uid;
		w.vorb.blocksize_0 = e.blocksize_0;
		w.vorb.blocksize_1 = e.blocksize_1;

		w.flags.mod_packets = 0;
		w.flags.granule_present = 1;
		w.flags.all_headers_present = (data_size <= 44);
	}
	*wem = w;
}

static inline void
i_init_fmt(wwise_fmt_t *const fmt, const unsigned char *const data, brru4 data_size)
{
#ifdef Ne_extra_debug
	if (data_size > sizeof(*fmt)) {
		NeExtraPrint(DEBUG,
			"fmt chunk size is %zu bytes (expected at most %zu).",
			data_size, sizeof(*fmt));
	}
#endif
	memcpy(fmt, data, brrnum_umin(data_size, sizeof(*fmt)));
}

int
wwriff_init(wwriff_t *const wwriff, const riff_t *const rf)
{
	if (!wwriff || !rf)
		return I_INIT_ERROR;

	wwriff_t w = {0};
	// Iterate the RIFF chunks
	for (brru8 i = 0; i < rf->n_basics; ++i) {
		riff_basic_chunk_t basic = rf->basics[i];
		if (basic.type == riff_basic_fmt) {
			if (w.flags.fmt_initialized) {
				BRRLOG_ERR("WWRIFF has multiple 'fmt' chunks");
				return I_INIT_ERROR;
			}

			i_init_fmt(&w.fmt, basic.data, basic.size);
			w.flags.fmt_initialized = 1;

			/* Vorb init header data is contained in the fmt */
			if (basic.size == 66) {
				if (w.flags.vorb_initialized) {
					BRRLOG_ERR("WWRIFF has both explicit and implicit 'vorb' chunks.");
					return I_INIT_ERROR;
				}
				i_init_vorb(&w, basic.data + 24, basic.size - 24);
				w.flags.vorb_initialized = 1;
			}

		} else if (basic.type == riff_basic_vorb) {
			/* Vorb init header data is explicit */
			if (w.flags.vorb_initialized) {
				BRRLOG_ERR("WWRIFF has multiple 'vorb' chunks.");
				return I_INIT_ERROR;
			}
			i_init_vorb(&w, basic.data, basic.size);
			w.flags.vorb_initialized = 1;

		} else if (basic.type == riff_basic_data) {
			if (w.flags.data_initialized) {
				BRRLOG_ERR("WWRIFF has multiple 'data' chunks.");
				return I_INIT_ERROR;
			}
			if (brrlib_alloc((void**)&w.data, basic.size, 0)) {
				BRRLOG_ERR("Failed to allocated %zu bytes for WWRIFF data : %s (%d)", basic.size, strerror(errno), errno);
				return I_BUFFER_ERROR;
			}
			memcpy(w.data, basic.data, basic.size);
			w.data_size = basic.size;
			w.flags.data_initialized = 1;
		}
	}

	if (!w.flags.fmt_initialized || !w.flags.vorb_initialized || !w.flags.data_initialized) {
		int count = 0;
		BRRLOG_ERRN("WWRIFF is missing");
		if (!w.flags.fmt_initialized) {
			if (count)
				BRRLOG_ERRNP(",");
			BRRLOG_ERRNP(" 'fmt'");
			++count;
		}
		if (!w.flags.vorb_initialized) {
			if (count)
				BRRLOG_ERRNP(",");
			BRRLOG_ERRNP(" 'vorb'");
			++count;
		}
		if (!w.flags.data_initialized) {
			if (count)
				BRRLOG_ERRNP(",");
			BRRLOG_ERRNP(" 'data'");
			++count;
		}
		BRRLOG_ERRP(count==1?" chunk":" chunks.");
		return I_INSUFFICIENT_DATA;
	}
	if (w.vorb.header_packets_offset > w.data_size || w.vorb.audio_start_offset > w.data_size) {
		BRRLOG_ERRN("WWRIFF data is corrupt: ");
		if (w.vorb.header_packets_offset > w.data_size) {
			BRRLOG_ERRP("'header_packets_offset' is past end of data.");
		} else {
			BRRLOG_ERRP("'audio_start_offset' is past end of data.");
		}
		return I_CORRUPT;
	}

	*wwriff = w;
	return 0;
}

void
wwriff_clear(wwriff_t *const wem)
{
	if (wem) {
		if (wem->comments) {
			for (brru4 i = 0; i < wem->n_comments; ++i)
				brrstringr_clear(&wem->comments[i]);
			free(wem->comments);
		}
		memset(wem, 0, sizeof(*wem));
	}
}

int
wwriff_add_comment(wwriff_t *const wem, const char *const format, ...)
{
	if (!wem || !format)
		return -1;

	brrstringr_t string = {0};
	va_list lptr;
	va_start(lptr, format);
	brrsz size = brrstringr_vprint(&string, 0, COMMENT_MAX, format, lptr);
	va_end(lptr);
	if (size == BRRSZ_MAX)
		return -1;
	if (brrlib_alloc((void **)&wem->comments, (wem->n_comments + 1) * sizeof(*wem->comments), 0)) {
		brrstringr_clear(&string);
		return -1;
	}
	wem->comments[wem->n_comments++] = string;

	return 0;
}

static inline int
i_init_ogg_packet(ogg_packet *const packet, oggpack_buffer *const packer, brru8 packetno, brru8 granule, brrbl end_of_stream)
{
	*packet = (ogg_packet) {
		.packet = oggpack_get_buffer(packer),
		.bytes = oggpack_bytes(packer),
		.b_o_s = packetno == 0,
		.e_o_s = end_of_stream != 0,
		.packetno = packetno,
		.granulepos = granule,
	};
	return 0;
}
#define PRINT_PACKET(_packet_) do {\
	NeExtraPrint(DEB, "    Packetno:   %lld", (_packet_).packetno);\
	NeExtraPrint(DEB, "    Granulepos: %lld", (_packet_).granulepos);\
	NeExtraPrint(DEB, "    BOS:        %i",   (_packet_).b_o_s);\
	NeExtraPrint(DEB, "    EOS:        %i",   (_packet_).e_o_s);\
	NeExtraPrint(DEB, "    Size:       %lld", (_packet_).bytes);\
} while (0)
static inline int
i_insert_packet(ogg_stream_state *const streamer, ogg_packet *const packet)
{
#ifdef Ne_extra_debug
	if (packet->b_o_s) {
		NeExtraPrint(DEB, "Inserting BOS packet");
		PRINT_PACKET(*packet);
	}
	if (packet->e_o_s) {
		NeExtraPrint(DEB, "Inserting EOS packet");
		PRINT_PACKET(*packet);
	}
#endif
	if (ogg_stream_packetin(streamer, packet)) {
		BRRLOG_ERR("Failed to insert ogg packet %lld into output stream.", packet->packetno);
		return I_BUFFER_ERROR;
	}
	return I_SUCCESS;
}
static int
i_insert_header(ogg_stream_state *const streamer, ogg_packet *const packet, vorbis_info *const vi, vorbis_comment *const vc)
{
	int err = 0;
	NeExtraPrint(DEB, "Inserting vorbis %s header packet:", vorbis_header(packet->packetno));
	if (packet->packetno != 0)
		PRINT_PACKET(*packet);

	if ((err = i_insert_packet(streamer, packet))) {
		BRRLOG_ERR("Failed to insert vorbis %s header packet into output stream.", vorbis_header_packet_names[packet->packetno]);
		return err;
	}

	if ((err = vorbis_synthesis_headerin(vi, vc, packet))) {
		BRRLOG_ERRN("Could not synthesize header %s : ", vorbis_header_packet_names[packet->packetno]);
		if (err == OV_ENOTVORBIS)
			BRRLOG_ERRP("NOT VORBIS");
		else if (err == OV_EBADHEADER)
			BRRLOG_ERRP("BAD HEADER");
		else
			BRRLOG_ERRP("INTERNAL ERROR");
		return I_CORRUPT;
	}
	return I_SUCCESS;
}

/****************************************
  Copy Vorbis headers; all are present and conform to spec.
****************************************/
static int
i_copy_id_header(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	int packet_type = packer_unpack(unpacker, 8); /* R Packet type, should be 1 */
	packer_pack(packer, 1, 8); /* W Packet type */

	/* R Vorbis str, should read 'vorbis' */
	char vorbis[7] = {0};
	for (int i = 0; i < 6; ++i)
		vorbis[i] = packer_unpack(unpacker, 8);
	/* W Vorbis str */
	for (int i = 0; i < 6; ++i)
		packer_pack(packer, VORBIS_STR[i], 8);

	long version        = packer_unpack(unpacker, 32); /* R Version, should be 0 */
	packer_pack(packer, 0, 32); /* W Version */
	int  audio_channels = packer_transfer(unpacker,  8, packer,  8); /* R/W Audio channels */
	long sample_rate    = packer_transfer(unpacker, 32, packer, 32); /* R/W Sample rate */
	long bitrate_max    = packer_transfer(unpacker, 32, packer, 32); /* R/W Bitrate maximum */
	long bitrate_nom    = packer_transfer(unpacker, 32, packer, 32); /* R/W Bitrate nominal */
	long bitrate_min    = packer_transfer(unpacker, 32, packer, 32); /* R/W Bitrate minimum */
	int  blocksize_0    = packer_transfer(unpacker,  4, packer,  4); /* R/W Blocksize 0 */
	int  blocksize_1    = packer_transfer(unpacker,  4, packer,  4); /* R/W Blocksize 1 */
	int  frame_flag     = packer_unpack(unpacker, 1); /* R Frame flag, should be 1 */
	packer_pack(packer, 1, 1); /* W Frame flag */

	return I_SUCCESS;
}
static int
i_copy_comment_header(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	/* See:
	 *   https://xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-820005
	 * */
	int packet_type = packer_unpack(unpacker, 8); /* R Packet type, should be 3 */
	packer_pack(packer, 3, 8); /* W Packet type */

	/* R Vorbis str, should read 'vorbis' */
	char vorbis[7] = {0};
	for (int i = 0; i < sizeof(VORBIS_STR) - 1; ++i)
		vorbis[i] = packer_unpack(unpacker, 8);
	/* W Vorbis str */
	for (int i = 0; i < sizeof(VORBIS_STR) - 1; ++i)
		packer_pack(packer, VORBIS_STR[i], 8);

	long vendor_length = packer_transfer(unpacker, 32, packer, 32); /* R/W Vendor length */
	/* R/W Vendor string */
	for (long i = 0; i < vendor_length; ++i) {
		char vendor_str = packer_transfer(unpacker, 8, packer, 8);
	}
	long comments_count = packer_transfer(unpacker, 32, packer, 32); /* R/W Comment list length */
	for (brrs8 i = 0; i < comments_count; ++i) {
		brrs8 comment_length = packer_transfer(unpacker, 32, packer, 32); /* R/W Comment length */
		/* R/W Comment string */
		for (brrs8 j = 0; j < comment_length; ++j) {
			char comment_str = packer_transfer(unpacker, 8, packer, 8);
		}
	}
	int frame_flag = packer_unpack(unpacker, 1); /* R Frame flag, should be 1 */
	packer_pack(packer, 1, 1); /* W Frame flag */
	return I_SUCCESS;
}
static int
i_copy_next_codebook(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	/* See:
	 *   https://xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-510003.2.1
	 * For an in-depth (allbeit hard to read) explanation of what this function does. */

	for (int i = 0; i < sizeof(CODEBOOK_SYNC) - 1; ++i) {
		/* R/W Codebook sync */
		if (CODEBOOK_SYNC[i] != packer_transfer(unpacker, 8, packer, 8)) {
			BRRLOG_ERR("Bad codebook sync.");
			return I_CORRUPT;
		}
	}
	long dimensions = packer_transfer(unpacker, 16, packer, 16); /* R/W Codebook dimensions */
	long entries = packer_transfer(unpacker, 24, packer, 24); /* R/W Codebook entries */
	int ordered = packer_transfer(unpacker, 1, packer, 1); /* R/W Ordered flag */
	if (ordered) {
		int current_length = 1 + packer_transfer(unpacker, 5, packer, 5); /* R/W Start length */
		long current_entry = 0;
		while (current_entry < entries) {
			int number_bits = lib_count_bits(entries - current_entry);
			long number = packer_transfer(unpacker, number_bits, packer, number_bits); /* R/W Magic number */
			current_entry += number;
			current_length++;
		}
		if (current_entry > entries) {
			BRRLOG_ERR("Corrupt ordered entries when copying codebook.");
			return I_CORRUPT;
		}
	} else {
		int sparse = packer_transfer(unpacker, 1, packer, 1); /* R/W Sparse flag */
		for (long i = 0; i < entries; ++i) {
			if (!sparse) {
				int length = 1 + packer_transfer(unpacker, 5, packer, 5); /* R/W Codeword length */
			} else {
				int used = packer_transfer(unpacker, 1, packer, 1); /* R/W Used flag */
				if (used) {
					int length = 1 + packer_transfer(unpacker, 5, packer, 5); /* R/W Codeword length */
				}
			}
		}
	}

	int lookup = packer_transfer(unpacker, 4, packer, 4); /* R/W Lookup type */
	if (lookup) {
		if (lookup > 2) {
			BRRLOG_ERR("Bad lookup value %i", lookup);
			return I_CORRUPT;
		}
		long minval_packed = packer_transfer(unpacker, 32, packer, 32); /* R/W Minimum value as uint; for real decoding, would be unpacked as a float. */
		long delval_packed = packer_transfer(unpacker, 32, packer, 32); /* R/W Delta value as uint; for real decoding, would be unpacked as a float. */
		int value_bits = 1 + packer_transfer(unpacker, 4, packer, 4); /* R/W Value bits */
		int sequence_flag = packer_transfer(unpacker, 1, packer, 1); /* R/W Sequence flag */

		long lookup_values = 0;
		if (lookup == 1)
			lookup_values = lib_lookup1_values(entries, dimensions);
		else
			lookup_values = entries * dimensions;

		for (long i = 0; i < lookup_values; ++i) {
			/* R/W Codebook multiplicands */
			long multiplicand = packer_transfer(unpacker, value_bits, packer, value_bits);
		}
	}
	return I_SUCCESS;
}
static int
i_copy_setup_header(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	/* See:
	 *    https://xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-620004.2.1
	 * and the following section on the setup header for what this function does.
	 * */
	int err = 0;
	int packet_type = packer_unpack(unpacker, 8); /* R Packet type, should be 5 */
	packer_pack(packer, 5, 8); /* W Packet type */

	brru1 vorbis[6] = {0};
	for (int i = 0; i < 6; ++i) /* R Vorbis str, should read 'vorbis' */
		vorbis[i] = packer_unpack(unpacker, 8);

	for (int i = 0; i < 6; ++i) /* W Vorbis str */
		packer_pack(packer, VORBIS_STR[i], 8);

	int codebook_count = 1 + packer_transfer(unpacker, 8, packer, 8); /* R/W Codebooks counts */
	if (!s_used_library || 1) {
		/* Inline codebooks, copy verbatim */
		/* For now, always copy verbatim */
		for (int i = 0; i < codebook_count; ++i) {
			if ((err = i_copy_next_codebook(unpacker, packer))) {
				BRRLOG_ERR("Could not copy codebook %lld.", i);
				return err;
			}
		}
	} else {
		/* Optionally external codebooks, copy from those instead */
		/* Currently unimplemented. */
		return I_BAD_ERROR;
	}

	/* Now copy the rest of it (naive copy, could probably do with some data integrity verification) */
	packer_transfer_remaining(unpacker, packer);
	return I_SUCCESS;
}
static int
i_copy_headers(ogg_stream_state *const streamer, wwriff_t *const wem, vorbis_info *const vi, vorbis_comment *const vc
)
{
	const wwriff_flags_t wem_flags = wem->flags;
	const unsigned char *packets = wem->data + wem->vorb.header_packets_offset;
	brru4 packets_length = wem->vorb.audio_start_offset - wem->vorb.header_packets_offset;

	int err = 0;
	for (int current_header = vorbis_header_packet_id; current_header < 3; ++current_header) {
		i_packeteer_t packeteer = {0};
		if ((err = i_packeteer_init(&packeteer, packets, packets_length, wem_flags, (brrsz)(packets - wem->data)))) {
			BRRLOG_ERR("Insufficient data to copy vorbis %s header packet.", vorbis_header(current_header));
			return err;
		}

		oggpack_buffer unpacker, packer;
		oggpack_writeinit(&packer);
		oggpack_readinit(&unpacker, packeteer.payload, packeteer.payload_size);
		switch (current_header) {
			case vorbis_header_packet_id: err = i_copy_id_header(&unpacker, &packer); break;
			case vorbis_header_packet_comment: err = i_copy_comment_header(&unpacker, &packer); break;
			case vorbis_header_packet_setup: err = i_copy_setup_header(&unpacker, &packer); break;
		}
		if (err) {
			BRRLOG_ERR("Could not copy vorbis %s header.", vorbis_header(current_header));
			oggpack_writeclear(&packer);
			return err;
		}

		{
			ogg_packet packet;
			i_init_ogg_packet(&packet, &packer, current_header, 0, 0);
			if ((err = i_insert_header(streamer, &packet, vi, vc))) {
				oggpack_writeclear(&packer);
				return err;
			}
		}
		oggpack_writeclear(&packer);
		packets += packeteer.total_size;
		packets_length -= packeteer.total_size;
	}
	return I_SUCCESS;
}

/****************************************
  Build headers; they must be rebuilt from modified forms present in the wem.
****************************************/
static int
i_build_id_header(oggpack_buffer *const packer, const wwriff_t *const wem)
{
	const wwise_fmt_t fmt = wem->fmt;
	const wwise_vorb_t vorb = wem->vorb;
	int packet_type = packer_pack(packer, 1, 8); /* W Packet type */
	/* W Vorbis string */
	for (int i = 0; i < sizeof(VORBIS_STR)-1; ++i)
		packer_pack(packer, VORBIS_STR[i], 8);
	long version     = packer_pack(packer,                     0, 32); /* W Vorbis version */
	int  n_channels  = packer_pack(packer,        fmt.n_channels,  8); /* W Audio channels */
	long sample_rate = packer_pack(packer,   fmt.samples_per_sec, 32); /* W Sample rate */
	long bitrate_max = packer_pack(packer,                     0, 32); /* W Bitrate maximum */
	long bitrate_nom = packer_pack(packer, 8 * fmt.avg_byte_rate, 32); /* W Bitrate nominal */
	long bitrate_min = packer_pack(packer,                     0, 32); /* W Bitrate minimum */
	int  blocksize_0 = packer_pack(packer,      vorb.blocksize_0,  4); /* W Blocksize 0 */
	int  blocksize_1 = packer_pack(packer,      vorb.blocksize_1,  4); /* W Blocksize 1 */
	int  frame_flag  = packer_pack(packer,                     1,  1); /* W Frame flag */
	/* TODO those bitrates I think could be more accurate, though I'm not sure how best I'd improve them. */
	return I_SUCCESS;
}
static int
i_build_comments_header(oggpack_buffer *const packer, const wwriff_t *const wem)
{
	static const char vendor_string[] = "NieR:Automated extraction Precept_v"Ne_version;
	static long vendor_len = sizeof(vendor_string);

	packer_pack(packer, 3, 8); /* W Packet type */
	/* W Vorbis string */
	for (int i = 0; i < 6; ++i)
		packer_pack(packer, VORBIS_STR[i], 8);

	packer_pack(packer, vendor_len, 32); /* W Vendor string length */
	/* W Vendor string */
	for (long i = 0; i < vendor_len; ++i)
		packer_pack(packer, vendor_string[i], 8);

	packer_pack(packer, wem->n_comments, 32); /* W Comment list length */
	if (wem->comments) {
		for (brru4 i = 0; i < wem->n_comments; ++i) {
			brrstringr_t comment = wem->comments[i];
			packer_pack(packer, comment.length, 32);
			for (brru4 i = 0; i < comment.length; ++i)
				packer_pack(packer, comment.cstr[i], 8);
		}
	}

	packer_pack(packer, 1, 1); /* W Framing flag */
	return I_SUCCESS;
}

static int
i_build_codebook(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	for (int i = 0; i < sizeof(CODEBOOK_SYNC)-1; ++i) /* W Codebook sync */
		packer_pack(packer, CODEBOOK_SYNC[i], 8);

	int dimensions = packer_transfer(unpacker,  4, packer, 16); /* R/W Dimensions */
	int entries    = packer_transfer(unpacker, 14, packer, 24); /* R/W Entries */
	int ordered    = packer_transfer(unpacker,  1, packer,  1); /* R/W Ordered flag */
	if (ordered) { /* Ordered codeword decode identical to spec */
		int current_length = 1 + packer_transfer(unpacker, 5, packer, 5); /* R/W Start length */
		long current_entry = 0;
		while (current_entry < entries) {
			int number_bits = lib_count_bits(entries - current_entry);
			long number = packer_transfer(unpacker, number_bits, packer, number_bits); /* R/W Magic number */
			current_entry += number;
			current_length++;
		}
		if (current_entry > entries) {
			BRRLOG_ERR("Corrupt ordered entries when rebuilding codebook");
			return I_CORRUPT;
		}
	} else {
		int codeword_length_bits, sparse;
		codeword_length_bits = packer_unpack(unpacker, 3);     /* R Codeword length bits */
		if (codeword_length_bits < 0 || codeword_length_bits > 5) {
			BRRLOG_ERR("Bad codeword length bits %i; must be (0,5]", codeword_length_bits);
			return I_CORRUPT;
		}
		sparse = packer_transfer(unpacker, 1, packer, 1);   /* R/W Sparse flag */
		if (!sparse) { /* R/W Nonsparse codeword lengths */
			for (int i = 0; i < entries; ++i) {
				int length = packer_transfer(unpacker, codeword_length_bits, packer, 5);
			}
		} else { /* R/W Sparse codeword lengths */
			for (int i = 0; i < entries; ++i) {
				int used = packer_transfer(unpacker, 1, packer, 1); /* R/W Used flag */
				if (used) {
					int length = packer_transfer(unpacker, codeword_length_bits, packer, 5); /* R/W Codeword length */
				}
			}
		}
	}

	int lookup = packer_transfer(unpacker, 1, packer, 4); /* R/W Lookup type */
	/* Lookup 1 decode identical to spec */
	if (lookup == 1) {
		long minval_packed =     packer_transfer(unpacker, 32, packer, 32); /* R/W Minimum value */
		long delval_packed =     packer_transfer(unpacker, 32, packer, 32); /* R/W Delta value */
		int  value_bits    = 1 + packer_transfer(unpacker,  4, packer,  4); /* R/W Value bits */
		int  sequence_flag =     packer_transfer(unpacker,  1, packer,  1); /* R/W Sequence flag */

		long lookup_values = lib_lookup1_values(entries, dimensions);

		/* R/W Codebook multiplicands */
		for (long i = 0; i < lookup_values; ++i) {
			long multiplicand = packer_transfer(unpacker, value_bits, packer, value_bits);
		}
	} else {
		BRRLOG_ERR("LOOKUP FAILED");
	}
	return I_SUCCESS;
}
static int
i_build_floors(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	/* Floor 1 decode mostly identical to spec, except floor type is absent from
	 * each floor (because there is only a single used floor type) */

	int floor_count = 1 + packer_transfer(unpacker, 6, packer, 6); /* R/W Floor count */
	for (int i = 0; i < floor_count; ++i) {
		long floor_type = packer_pack(packer, 1, 16); /* W Floor type */
		int  partitions = packer_transfer(unpacker, 5, packer, 5); /* R/W Floor partitions */
		int  partition_classes[31];
		int  max_class = -1;
		/* R/W Partition classes */
		for (int j = 0; j < partitions; ++j) {
			int class = partition_classes[j] = packer_transfer(unpacker, 4, packer, 4);
			if (class > max_class)
				max_class = class;
		}

		int class_dims[16], class_subs[16], class_books[16];
		int sub_books[16][16];
		for (int j = 0; j <= max_class; ++j) {
			int n_dims = class_dims[j] = 1 + packer_transfer(unpacker, 3, packer, 3); /* R/W Class dimensions */
			int n_subs = class_subs[j] =     packer_transfer(unpacker, 2, packer, 2); /* R/W Class subclasses */
			/* R/W Class books */
			if (n_subs) {
				int master = class_books[j] = packer_transfer(unpacker, 8, packer, 8);
			}

			int limit_break = 1 << n_subs;
			/* R/W Subclass books */
			for (int k = 0; k < limit_break; ++k) {
				sub_books[j][k] = -1 + packer_transfer(unpacker, 8, packer, 8);
			}
		}

		int multiplier = 1 + packer_transfer(unpacker, 2, packer, 2); /* R/W Floor multiplier */
		int rangebits  =     packer_transfer(unpacker, 4, packer, 4); /* R/W Floor rangebits */
		for (int j = 0; j < partitions; ++j) {
			int dims = class_dims[partition_classes[j]];
			/* R/W Floor X list */
			for (int k = 0; k < dims; ++k) {
				long X = packer_transfer(unpacker, rangebits, packer, rangebits);
			}
		}
	}
	return I_SUCCESS;
}
static int
i_build_residues(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	/* As far as I can tell, residue decode is identical to spec */
	int residue_count = 1 + packer_transfer(unpacker, 6, packer, 6); /* R/W Residue count */
	for (int i = 0; i < residue_count; ++i) {
		int type = packer_transfer(unpacker, 2, packer, 16);  /* R/W Residue type */
		if (type > 2) {
			BRRLOG_ERR("Bad residue type %i", type);
			return I_CORRUPT;
		}

		long start          =     packer_transfer(unpacker, 24, packer, 24); /* R/W Residue begin */
		long end            =     packer_transfer(unpacker, 24, packer, 24); /* R/W Residue end */
		long partition_size = 1 + packer_transfer(unpacker, 24, packer, 24); /* R/W Partition size */
		int  classes        = 1 + packer_transfer(unpacker,  6, packer,  6); /* R/W Residue classes */
		int  classbook      =     packer_transfer(unpacker,  8, packer,  8); /* R/W Residue classbook */

		int cascades[64];
		int acc = 0; /* ??????? */
		/* R/W Residue cascades */
		for (int j = 0; j < classes; ++j) {
			int bitflag;
			cascades[j] = packer_transfer(unpacker, 3, packer, 3);          /* R/W Cascade low-bits */
			bitflag = packer_transfer(unpacker, 1, packer, 1);              /* R/W Cascade bitflag */
			if (bitflag)
				cascades[j] += 8 * packer_transfer(unpacker, 5, packer, 5); /* R/W Cascade high-bits */

			acc += lib_count_ones(cascades[j]);
		}
		/* R/W Residue books */
		for (int j = 0; j < acc; ++j) {
			int residue_book_index_jb = packer_transfer(unpacker, 8, packer, 8);
		}
	}
	return I_SUCCESS;
}
static int
i_build_mappings(oggpack_buffer *const unpacker, oggpack_buffer *const packer, int n_channels)
{
	const int n_channel_bits = lib_count_bits(n_channels - 1);
	const int mapping_count = 1 + packer_transfer(unpacker, 6, packer, 6); /* R/W Mapping count */
	for (int i = 0; i < mapping_count; ++i) {
		long mapping_type = packer_pack(packer, 0, 16); /* W Mapping type */
		int submaps_flag = packer_transfer(unpacker, 1, packer, 1); /* R/W Submaps flag */
		int submaps = 1;
		if (submaps_flag)
			submaps = 1 + packer_transfer(unpacker, 4, packer, 4); /* R/W Submaps */

		int square_mapping = packer_transfer(unpacker, 1, packer, 1);  /* R/W Square mapping flag */
		if (square_mapping) {
			int coupling_steps = 1 + packer_transfer(unpacker, 8, packer, 8); /* R/W Coupling steps */
			/* R/W Mapping vectors */
			for (int j = 0; j < coupling_steps; ++j) {
				long mapping_magnitude = packer_transfer(unpacker, n_channel_bits, packer, n_channel_bits);
				long mapping_angle = packer_transfer(unpacker, n_channel_bits, packer, n_channel_bits);
			}
		}

		int reserved = packer_unpack(unpacker, 2); /* R Reserved */
		packer_pack(packer, 0, 2); /* W Reserved */
		/* R/W Mapping channel multiplexes */
		if (submaps > 1) {
			for (int j = 0; j < n_channels; ++j) {
				int mapping_mux = packer_transfer(unpacker, 4, packer, 4);
			}
		}
		/* R/W Submap configurations */
		for (int i = 0; i < submaps; ++i) {
			int discarded = packer_transfer(unpacker, 8, packer, 8);
			int floor = packer_transfer(unpacker, 8, packer, 8);
			int residue = packer_transfer(unpacker, 8, packer, 8);
		}
	}
	return I_SUCCESS;
}
static int
i_build_modes(oggpack_buffer *const unpacker, oggpack_buffer *const packer, brru1 *const mode_blockflags, int *const mode_count)
{
	int md_count = 1 + packer_transfer(unpacker, 6, packer, 6); /* R/W Mode count */
	for (int i = 0; i < md_count; ++i) {
		int  blockflag      = packer_transfer(unpacker, 1, packer, 1); /* R/W Blockflag */
		long window         = packer_pack(packer, 0, 16); /* W Window type */
		long transform_type = packer_pack(packer, 0, 16); /* W Transform type */
		int  mapping        = packer_transfer(unpacker, 8, packer, 8); /* R/W Mode mapping */
		mode_blockflags[i] = blockflag;
	}
	*mode_count = md_count;
	return I_SUCCESS;
}
/* This is easily the most complicated function in this entire project; it's even split up
 * amongst 4 other functions! */
static int
i_build_setup_header(oggpack_buffer *const packer, wwriff_t *const wem)
{
	unsigned char *packets_start = wem->data + wem->vorb.header_packets_offset;
	brru4 packets_size = wem->vorb.audio_start_offset - wem->vorb.header_packets_offset;
	i_packeteer_t packeteer = {0};
	int err = 0;
	if ((err = i_packeteer_init(&packeteer, packets_start, packets_size, wem->flags, (brrsz)(packets_start - wem->data)))) {
		BRRLOG_ERR("Failed to initialize vorbis setup header packet.");
		return err;
	}

	oggpack_buffer unpacker;
	oggpack_readinit(&unpacker, packeteer.payload, packeteer.payload_size);

	int stripped = s_current_input->flag.stripped_headers;

	packer_pack(packer, 5, 8); /* W Packet type (setup header = 5) */
	for (int i = 0; i < 6; ++i) /* W Vorbis string */
		packer_pack(packer, VORBIS_STR[i], 8);

	int codebook_count = 1 + packer_transfer(&unpacker, 8, packer, 8); /* R/W Codebook count */
	if (!s_used_library) {
		/* Internal codebooks */
		if (!stripped) {
			/* Full codebooks, can be copied from header directly */
			for (int i = 0; i < codebook_count; ++i) {
				NeExtraPrint(DEBUG, "Copying internal codebook %d", i);
				if ((err = i_copy_next_codebook(&unpacker, packer))) {
					BRRLOG_ERR("Failed to copy codebook %d", i);
					return err;
				}
			}

		} else {
			/* Stripped codebooks, need to be unpacked/rebuilt to spec */
			for (int i = 0; i < codebook_count; ++i) {
				if ((err = packed_codebook_unpack_raw(&unpacker, packer))) {
					BRRLOG_ERR("Failed to build codebook %d", i);
					return err;
				}
			}
		}

	} else {
		/* TODO this part I understand the least */
		/* External codebooks */
		for (int i = 0; i < codebook_count; ++i) {
			packed_codebook_t *cb = NULL;
			int cbidx = 1 + packer_unpack(&unpacker, 10); /* R Codebook index */
			/* I don't know why it's off by 1; ww2ogg just sorta rolls with it
			 * without too much checking (specifically in get_codebook_size) and
			 * I can't figure out why it works there */
			if (cbidx > s_used_library->codebook_count) {
				/* This bit ripped from ww2ogg, no idea what it means */
				if (cbidx == 0x342) {
					cbidx = packer_unpack(&unpacker, 14);      /* R Codebook id */
					if (cbidx == 0x1590) {
						/* ??? */
					}
				}
				BRRLOG_ERR("Codebook index too large %d", cbidx);
				return I_CORRUPT;
			}

			cb = &s_used_library->codebooks[cbidx];
			if (CODEBOOK_SUCCESS != (err = packed_codebook_unpack(cb))) { /* Copy from external */
				if (err == CODEBOOK_ERROR)
					err = I_BUFFER_ERROR;
				else if (err == CODEBOOK_CORRUPT)
					err = I_CORRUPT;
				BRRLOG_ERR("Failed to copy external codebook %d : %s", cbidx, lib_strerr(err));
				return err;
			} else {
				oggpack_buffer cb_unpacker;
				oggpack_readinit(&cb_unpacker, cb->unpacked_data, (cb->unpacked_bits + 7) / 8);
				if (-1 == packer_transfer_lots(&cb_unpacker, packer, cb->unpacked_bits))
					return I_BUFFER_ERROR;
			}
		}
	}

	packer_pack(packer, 0, 6);  /* W Time count - 1 */
	packer_pack(packer, 0, 16); /* W Vorbis time-domain stuff */

	if (!stripped) {
		/* Rest of the header in-spec, copy verbatim */
		if (-1 == (err = packer_transfer_remaining(&unpacker, packer))) {
			BRRLOG_ERR("Failed to copy the rest of vorbis setup packet");
			return I_CORRUPT;
		}

	} else {
		/* Need to rebuild the setup header */
		if ((err = i_build_floors(&unpacker, packer))) {
			BRRLOG_ERR("Failed to rebuild floors");
			return err;
		}
		if ((err = i_build_residues(&unpacker, packer))) {
			BRRLOG_ERR("Failed to rebuild residues");
			return err;
		}
		if ((err = i_build_mappings(&unpacker, packer, wem->fmt.n_channels))) {
			BRRLOG_ERR("Failed to rebuild mappings");
			return err;
		}
		if ((err = i_build_modes(&unpacker, packer, wem->mode_blockflags, &wem->mode_count))) {
			BRRLOG_ERR("Failed to rebuild modes");
			return err;
		}
	}

	packer_pack(packer, 1, 1); /* W Frame flag */
	return I_SUCCESS;
}
static int
i_build_headers(ogg_stream_state *const streamer, wwriff_t *const wem, vorbis_info *const vi, vorbis_comment *const vc)
{
	int err = 0;
	for (int current_header = 0; current_header < 3; ++current_header) {
		oggpack_buffer packer;
		oggpack_writeinit(&packer);
		switch (current_header) {
			case 0: err = i_build_id_header(&packer, wem); break;
			case 1: err = i_build_comments_header(&packer, wem); break;
			case 2: err = i_build_setup_header(&packer, wem); break;
		}
		if (err) {
			BRRLOG_ERRN("Failed to build vorbis %s header", vorbis_header(current_header));
			oggpack_writeclear(&packer);
			return err;
		}

		{
			ogg_packet packet;
			i_init_ogg_packet(&packet, &packer, current_header, 0, 0);
			if ((err = i_insert_header(streamer, &packet, vi, vc))) {
				oggpack_writeclear(&packer);
				return err;
			}
		}
		oggpack_writeclear(&packer);
	}
	return I_SUCCESS;
}

/* PROCESS */
static inline int
i_process_headers(ogg_stream_state *const streamer, wwriff_t *const wem, vorbis_info *const vi, vorbis_comment *const vc)
{
	if (wem->flags.all_headers_present) {
		return i_copy_headers(streamer, wem, vi, vc);
	} else {
		return i_build_headers(streamer, wem, vi, vc);
	}
}

static int
i_process_audio(ogg_stream_state *const streamer, wwriff_t *const wem, vorbis_info *const vi, vorbis_comment *const vc)
{
	const int mode_count_bits = lib_count_bits(wem->mode_count - 1);
	brru4 packets_start = wem->vorb.audio_start_offset;
	brru4 packets_size = wem->data_size - wem->vorb.audio_start_offset;
	i_packeteer_t packeteer = {0};

	int prev_blockflag = 0;
	brru8 last_block = 0;
	brru8 total_block = 0;
	brru8 packetno = 0;

	int err = 0;
	NeExtraPrint(DEB, "Stream mod packets");
	while (packets_start < wem->data_size) {
		int eos = 0;
		if ((err = i_packeteer_init(&packeteer, wem->data + packets_start, packets_size, wem->flags, packets_start))) {
			BRRLOG_ERR("Insufficient data to build next audio packet %lld", packetno);
			return err;
		}

		brru4 next_start = packets_start + packeteer.total_size;

		oggpack_buffer unpacker, packer;
		oggpack_readinit(&unpacker, packeteer.payload, packeteer.payload_size);
		oggpack_writeinit(&packer);

		if (wem->flags.mod_packets) {
			int packet_type = packer_pack(&packer, 0, 1); /* W Packet type */
			int mode_number = packer_transfer(&unpacker, mode_count_bits, &packer, mode_count_bits); /* R/W Mode number */
			int remainder = packer_unpack(&unpacker, 8 - mode_count_bits); /* R Remainder bits */
			if (wem->mode_blockflags[mode_number]) {
				/* Long window */
				int next_blockflag = 0;
				brru4 next_size  = packets_size  + packeteer.total_size;
				i_packeteer_t next_packeteer;
				if ((err = i_packeteer_init(&next_packeteer, wem->data + next_start, next_size, wem->flags, next_start))) {
					eos = 1;
				} else if (next_packeteer.payload_size) {
					oggpack_buffer next_unpacker;
					oggpack_readinit(&next_unpacker, next_packeteer.payload, next_packeteer.payload_size);
					int next_number = packer_unpack(&next_unpacker, mode_count_bits); /* R Next number */
					next_blockflag = wem->mode_blockflags[next_number];
				}
				packer_pack(&packer, prev_blockflag, 1); /* W Previous window type */
				packer_pack(&packer, next_blockflag, 1); /* W Next window type */
			}
			packer_pack(&packer, remainder, 8 - mode_count_bits); /* W Remainder of read-in first byte */
			prev_blockflag = wem->mode_blockflags[mode_number];
		}

		packer_transfer_remaining(&unpacker, &packer);

		/* This granule calculation is from revorb, not sure its source though; probably somewhere in vorbis docs, haven't found it */
		/* I'll be honest; I really don't understand this at all. */
		ogg_packet packet;
		i_init_ogg_packet(&packet, &packer, packetno + 3, 0, eos || next_start >= wem->data_size);

		long current_block = vorbis_packet_blocksize(vi, &packet);

		/* This line goes after the 'if' in original revorb, however putting it before incrementing total_block
		 * gets rid of one error from ogginfo, the ".. headers incorrectly framed, terminal header page has non-zero granpos."
		 * Honestly, it's probably just a fluke with this whole algorithm and that one test I did it on; */
		packet.granulepos = total_block;

		if (last_block)
			total_block += (last_block + current_block) / 4;
		last_block = current_block;
		NeExtraPrint(DEB, "Granulepos: %llu | Block: %llu | Total block: %llu", packet.granulepos, current_block, total_block);

		if ((err = i_insert_packet(streamer, &packet))) {
			oggpack_writeclear(&packer);
			return err;
		}

		oggpack_writeclear(&packer);
		packets_start += packeteer.total_size;
		packets_size  -= packeteer.total_size;
		++packetno;
	}
	NeExtraPrint(DEBUG, "Total packets: %lld", 3 + packetno);
	return I_SUCCESS;
}

int
wwise_convert_wwriff(
    wwriff_t *const in_wwriff,
    ogg_stream_state *const out_stream,
    const codebook_library_t *const library,
    const neinput_t *const input
)
{
	int err = 0;
	/* Setup ogg/vorbis stuff  */
	vorbis_info vi;
	vorbis_comment vc;
	{
		int serialno = in_wwriff->vorb.uid;
		if (STREAM_INIT_SUCCESS != ogg_stream_init(out_stream, serialno)) {
			BRRLOG_ERR("Failed to initialize ogg stream for output.");
			return I_INIT_ERROR;
		}
		vorbis_info_init(&vi);
		vorbis_comment_init(&vc);
	}

	/* Convert */
	s_current_input = input;
	s_used_library = library;

	if (!(err = i_process_headers(out_stream, in_wwriff, &vi, &vc)))
		err = i_process_audio(out_stream, in_wwriff, &vi, &vc);

	if (err)
		ogg_stream_clear(out_stream);

	vorbis_info_clear(&vi);
	vorbis_comment_clear(&vc);

	s_current_input = NULL;
	s_used_library = NULL;
	return err;
}
