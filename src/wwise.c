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

#include "neinput.h"
#include "nelog.h"
#include "neutil.h"
#include "riff.h"

#define COMMENT_MAX 1024

#define VORBIS "vorbis"
#define CODEBOOK "BCV"

static const codebook_library_t *s_used_library = NULL;
static const neinput_t *s_current_input = NULL;

const char *const vorbishdr_names[3] = {
	"ID",
	"comments",
	"setup",
};

typedef struct i_packeteer {
// Bitfields to avoid padding
// First 4 fields are ordered as they are on disk
	brru8 payload_size:16;
	brru8 granule:32;
	brru8 unused:16;
	unsigned char *payload;
	int header_length;
	brru4 total_size;
} i_packeteer_t;

#define E_GENERIC -1
#define E_INSUFFICIENT_DATA -2
#define E_BUFFER -3
#define E_CORRUPT -4

/* Initializes 'packet' from the wwise stream 'wem' with data 'data'.
 *  0 : success
 * -1 : error (input)
 * -2 : insufficient data
 * */
static inline int
i_packeteer_init(
    i_packeteer_t *const packet,
    const unsigned char *const data,
    brrsz data_size,
    wwise_flags_t wem_flags,
    brrsz offset
)
{
	if (!packet || !data)
		return E_GENERIC;

	if (data_size < 2)
		return E_INSUFFICIENT_DATA;

	i_packeteer_t pk = {0};
	pk.payload_size = *(brru2 *)data;
	if (pk.payload_size > data_size)
		return E_INSUFFICIENT_DATA;

	brru1 ofs = 2;
	if (wem_flags.granule_present) {
		if (data_size < ofs + 4)
			return E_INSUFFICIENT_DATA;

		pk.granule = *(brru4 *)(data + ofs);
		ofs += 4;
		if (wem_flags.all_headers_present) {
			if (data_size < ofs + 2)
				return E_INSUFFICIENT_DATA;

			pk.unused = *(brru2 *)(data + ofs);
			ofs += 2;
		}
	}
	pk.payload = (unsigned char *)data + ofs;
	pk.header_length = ofs;
	pk.total_size = pk.payload_size + pk.header_length;
	ExtraDeb(,"Init packet of size %llu (%llu header, %llu payload) at offset 0x%08x", pk.total_size, pk.header_length, pk.payload_size, offset);
	*packet = pk;
	return 0;
}

/* TODO nepack_transfer can error; many transfers don't check for errors. */

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
		if (data_size > sizeof(e))
			ExtraDeb(,"Explicit vorbis initialization header size is %zu bytes (expected at most %zu).", data_size, sizeof(e));
#endif
		memcpy(&e, data, neutil_min(data_size, sizeof(e))); // TODO why this min?

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
	if (data_size > sizeof(*fmt))
		ExtraDeb(,"fmt chunk size is %zu bytes (expected at most %zu).", data_size, sizeof(*fmt));
#endif
	memcpy(fmt, data, neutil_min(data_size, sizeof(*fmt)));
}

int
wwriff_init(wwriff_t *const wwriff, const riff_t *const rf)
{
	if (!wwriff || !rf)
		return E_GENERIC;

	wwriff_t w = {0};
	// Iterate the RIFF chunks
	for (brru8 i = 0; i < rf->n_basics; ++i) {
		riff_basic_chunk_t basic = rf->basics[i];
		if (basic.type == riff_basic_fmt) {
			if (w.flags.fmt_initialized) {
				Err(,"WwRIFF has multiple 'fmt ' chunks");
				return E_GENERIC;
			}

			i_init_fmt(&w.fmt, basic.data, basic.size);
			w.flags.fmt_initialized = 1;

			/* Vorb init header data is contained in the fmt */
			if (basic.size == 66) {
				ExtraDeb(,"'vorb' chunk is implicit");
				if (w.flags.vorb_initialized) {
					Err(,"WwRIFF has both explicit and implicit 'vorb' chunks.");
					return E_GENERIC;
				}
				i_init_vorb(&w, basic.data + 24, basic.size - 24);
				w.flags.vorb_initialized = 1;
			}

		} else if (basic.type == riff_basic_vorb) {
			/* Vorb init header data is explicit */
			if (w.flags.vorb_initialized) {
				Err(,"WwRIFF has multiple 'vorb' chunks.");
				return E_GENERIC;
			}
			i_init_vorb(&w, basic.data, basic.size);
			w.flags.vorb_initialized = 1;

		} else if (basic.type == riff_basic_data) {
			if (w.flags.data_initialized) {
				Err(,"WwRIFF has multiple 'data' chunks.");
				return E_GENERIC;
			}

			unsigned char *new = realloc(w.data, basic.size);
			if (!new) {
				Err(,"Failed to allocated %zu bytes for WwRIFF data: %s (%d)", basic.size, strerror(errno), errno);
				return E_BUFFER;
			}
			w.data = new;

			memcpy(w.data, basic.data, basic.size);
			w.data_size = basic.size;
			w.flags.data_initialized = 1;
		}
	}

	if (!w.flags.fmt_initialized || !w.flags.vorb_initialized || !w.flags.data_initialized) {
		if (!w.flags.fmt_initialized)
			Err(,"WwRIFF is missing 'fmt ' chunk");
		if (!w.flags.vorb_initialized)
			Err(,"WwRIFF is missing 'vorb' chunk");
		if (!w.flags.data_initialized)
			Err(,"WwRIFF is missing 'data' chunk");
		return E_INSUFFICIENT_DATA;
	}
	if (w.vorb.header_packets_offset > w.data_size) {
		Err(,"WwRIFF data is corrupt: 'header_packets_offset' is past end of data.");
		return E_CORRUPT;
	} else if (w.vorb.audio_start_offset > w.data_size) {
		Err(,"WwRIFF data is corrupt: 'audio_start_offset' is past end of data.");
		return E_CORRUPT;
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

	brrstringr_t *new = realloc(wem->comments, sizeof(*new) * (wem->n_comments + 1));
	if (!new) {
		Err(,"Failed to allocate space for vorbis comment #%zu: %s (%d)", wem->n_comments, strerror(errno), errno);
		brrstringr_clear(&string);
		return -1;
	}
	new[wem->n_comments++] = string;
	wem->comments = new;

	return 0;
}

#ifdef Ne_extra_debug
#define PRINT_PACKET(_variant_,_packet_) do {\
	ExtraDeb(_variant_,\
		"ogg_packet #%lld : Size %lld | Granulepos %lld | BOS %i | EOS %i",\
		(_packet_).packetno, (_packet_).bytes, (_packet_).granulepos, (_packet_).b_o_s, (_packet_).e_o_s);\
} while (0)
#else
#define PRINT_PACKET(...)
#endif
static inline int
i_init_ogg_packet(ogg_packet *const packet, oggpack_buffer *const packer, brru8 packetno, brru8 granule, brrbl end_of_stream)
{
#ifdef Ne_extra_debug
	ogg_packet p =
#endif
	*packet = (ogg_packet) {
		.packet = oggpack_get_buffer(packer),
		.bytes = oggpack_bytes(packer),
		.b_o_s = packetno == 0,
		.e_o_s = end_of_stream != 0,
		.packetno = packetno,
		.granulepos = granule,
	};
	PRINT_PACKET(,p);
	return 0;
}
static inline int
i_insert_packet(ogg_stream_state *const streamer, ogg_packet *const packet)
{
#ifdef Ne_extra_debug
	if (packet->b_o_s || packet->e_o_s) {
		ExtraDeb(,"Inserting %s packet", packet->b_o_s?(packet->e_o_s?"BOS/EOS":"BOS    "):"    EOS");
	}
#endif
	if (ogg_stream_packetin(streamer, packet)) {
		Err(,"Failed to insert ogg packet %lld into output stream.", packet->packetno);
		return E_BUFFER;
	}
	return 0;
}
static int
i_insert_header(ogg_stream_state *const streamer, ogg_packet *const packet, vorbis_info *const vi, vorbis_comment *const vc)
{
	int err = 0;
	ExtraDeb(,"Inserting vorbis %8s packet", vorbishdr(packet->packetno));

	if ((err = i_insert_packet(streamer, packet))) {
		Err(,"Failed to insert vorbis %s header packet into output stream.", vorbishdr_names[packet->packetno]);
		return err;
	}

	if ((err = vorbis_synthesis_headerin(vi, vc, packet))) {
		Err(n,"Could not synthesize header %s : ", vorbishdr_names[packet->packetno]);
		if (err == OV_ENOTVORBIS)
			Err(p,"NOT VORBIS");
		else if (err == OV_EBADHEADER)
			Err(p,"BAD HEADER");
		else
			Err(p,"INTERNAL ERROR");
		return E_CORRUPT;
	}
	return 0;
}

/****************************************
  Copy Vorbis headers; all are present and conform to spec.
****************************************/
static int
i_copy_id_header(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	int packet_type = nepack_unpack(unpacker, 8); /* R Packet type, should be 1 */
	nepack_pack(packer, 1, 8); /* W Packet type */

	/* R Vorbis str, should read 'vorbis' */
	char vorbis[7] = {0};
	for (int i = 0; i < 6; ++i)
		vorbis[i] = nepack_unpack(unpacker, 8);
	/* W Vorbis str */
	for (int i = 0; i < 6; ++i)
		nepack_pack(packer, VORBIS[i], 8);

	long version        = nepack_unpack(unpacker, 32); /* R Version, should be 0 */
	nepack_pack(packer, 0, 32); /* W Version */
	int  audio_channels = nepack_transfer(unpacker,  8, packer,  8); /* R/W Audio channels */
	long sample_rate    = nepack_transfer(unpacker, 32, packer, 32); /* R/W Sample rate */
	long bitrate_max    = nepack_transfer(unpacker, 32, packer, 32); /* R/W Bitrate maximum */
	long bitrate_nom    = nepack_transfer(unpacker, 32, packer, 32); /* R/W Bitrate nominal */
	long bitrate_min    = nepack_transfer(unpacker, 32, packer, 32); /* R/W Bitrate minimum */
	int  blocksize_0    = nepack_transfer(unpacker,  4, packer,  4); /* R/W Blocksize 0 */
	int  blocksize_1    = nepack_transfer(unpacker,  4, packer,  4); /* R/W Blocksize 1 */
	int  frame_flag     = nepack_unpack(unpacker, 1); /* R Frame flag, should be 1 */
	nepack_pack(packer, 1, 1); /* W Frame flag */

	return 0;
}
static int
i_copy_comment_header(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	/* See:
	 *   https://xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-820005
	 * */
	int packet_type = nepack_unpack(unpacker, 8); /* R Packet type, should be 3 */
	nepack_pack(packer, 3, 8); /* W Packet type */

	/* R Vorbis str, should read 'vorbis' */
	char vorbis[6] = {0};
	for (int i = 0; i < sizeof(VORBIS) - 1; ++i)
		vorbis[i] = nepack_unpack(unpacker, 8);
	/* W Vorbis str */
	for (int i = 0; i < sizeof(VORBIS) - 1; ++i)
		nepack_pack(packer, VORBIS[i], 8);

	long vendor_length = nepack_transfer(unpacker, 32, packer, 32); /* R/W Vendor length */
	/* R/W Vendor string */
	for (long i = 0; i < vendor_length; ++i) {
		char vendor_str = nepack_transfer(unpacker, 8, packer, 8);
	}
	long comments_count = nepack_transfer(unpacker, 32, packer, 32); /* R/W Comment list length */
	for (brrs8 i = 0; i < comments_count; ++i) {
		brrs8 comment_length = nepack_transfer(unpacker, 32, packer, 32); /* R/W Comment length */
		/* R/W Comment string */
		for (brrs8 j = 0; j < comment_length; ++j) {
			char comment_str = nepack_transfer(unpacker, 8, packer, 8);
		}
	}
	int frame_flag = nepack_unpack(unpacker, 1); /* R Frame flag, should be 1 */
	nepack_pack(packer, 1, 1); /* W Frame flag */
	return 0;
}
static int
i_copy_next_codebook(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	/* See:
	 *   https://xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-510003.2.1
	 * For an in-depth (allbeit hard to read) explanation of what this function does. */

	for (int i = 0; i < sizeof(CODEBOOK) - 1; ++i) {
		/* R/W Codebook sync */
		if (CODEBOOK[i] != nepack_transfer(unpacker, 8, packer, 8)) {
			Err(,"Bad codebook sync.");
			return E_CORRUPT;
		}
	}
	long dimensions = nepack_transfer(unpacker, 16, packer, 16); /* R/W Codebook dimensions */
	long entries = nepack_transfer(unpacker, 24, packer, 24); /* R/W Codebook entries */
	int ordered = nepack_transfer(unpacker, 1, packer, 1); /* R/W Ordered flag */
	if (ordered) {
		int current_length = 1 + nepack_transfer(unpacker, 5, packer, 5); /* R/W Start length */
		long current_entry = 0;
		while (current_entry < entries) {
			int number_bits = neutil_count_bits(entries - current_entry);
			long number = nepack_transfer(unpacker, number_bits, packer, number_bits); /* R/W Magic number */
			current_entry += number;
			current_length++;
		}
		if (current_entry > entries) {
			Err(,"Corrupt ordered entries when copying codebook.");
			return E_CORRUPT;
		}
	} else {
		int sparse = nepack_transfer(unpacker, 1, packer, 1); /* R/W Sparse flag */
		for (long i = 0; i < entries; ++i) {
			if (!sparse) {
				int length = 1 + nepack_transfer(unpacker, 5, packer, 5); /* R/W Codeword length */
			} else {
				int used = nepack_transfer(unpacker, 1, packer, 1); /* R/W Used flag */
				if (used) {
					int length = 1 + nepack_transfer(unpacker, 5, packer, 5); /* R/W Codeword length */
				}
			}
		}
	}

	int lookup = nepack_transfer(unpacker, 4, packer, 4); /* R/W Lookup type */
	if (lookup) {
		if (lookup > 2) {
			Err(,"Bad lookup value %i", lookup);
			return E_CORRUPT;
		}
		long minval_packed = nepack_transfer(unpacker, 32, packer, 32); /* R/W Minimum value as uint; for real decoding, would be unpacked as a float. */
		long delval_packed = nepack_transfer(unpacker, 32, packer, 32); /* R/W Delta value as uint; for real decoding, would be unpacked as a float. */
		int value_bits = 1 + nepack_transfer(unpacker, 4, packer, 4); /* R/W Value bits */
		int sequence_flag = nepack_transfer(unpacker, 1, packer, 1); /* R/W Sequence flag */

		long lookup_values = 0;
		if (lookup == 1)
			lookup_values = neutil_lookup1(entries, dimensions);
		else
			lookup_values = entries * dimensions;

		for (long i = 0; i < lookup_values; ++i) {
			/* R/W Codebook multiplicands */
			long multiplicand = nepack_transfer(unpacker, value_bits, packer, value_bits);
		}
	}
	return 0;
}
static int
i_copy_setup_header(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	/* See:
	 *    https://xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-620004.2.1
	 * and the following section on the setup header for what this function does.
	 * */
	int err = 0;
	int packet_type = nepack_unpack(unpacker, 8); /* R Packet type, should be 5 */
	nepack_pack(packer, 5, 8); /* W Packet type */

	brru1 vorbis[6] = {0};
	for (int i = 0; i < 6; ++i) /* R Vorbis str, should read 'vorbis' */
		vorbis[i] = nepack_unpack(unpacker, 8);

	for (int i = 0; i < 6; ++i) /* W Vorbis str */
		nepack_pack(packer, VORBIS[i], 8);

	int codebook_count = 1 + nepack_transfer(unpacker, 8, packer, 8); /* R/W Codebooks counts */
	if (!s_used_library || 1) {
		/* Inline codebooks, copy verbatim */
		/* For now, always copy verbatim */
		for (int i = 0; i < codebook_count; ++i) {
			if ((err = i_copy_next_codebook(unpacker, packer))) {
				Err(,"Could not copy codebook %lld.", i);
				return err;
			}
		}
	} else {
		/* Optionally external codebooks, copy from those instead */
		/* Currently unimplemented. */
		return -1;
	}

	/* Now copy the rest of it (naive copy, could probably do with some data integrity verification) */
	nepack_transfer_remaining(unpacker, packer);
	return 0;
}
static int
i_copy_headers(ogg_stream_state *const streamer, wwriff_t *const wem, vorbis_info *const vi, vorbis_comment *const vc
)
{
	const wwise_flags_t wem_flags = wem->flags;
	const unsigned char *packets = wem->data + wem->vorb.header_packets_offset;
	brru4 packets_length = wem->vorb.audio_start_offset - wem->vorb.header_packets_offset;

	int err = 0;
	for (int current_header = vorbishdr_id; current_header < 3; ++current_header) {
		i_packeteer_t packeteer = {0};
		if ((err = i_packeteer_init(&packeteer, packets, packets_length, wem_flags, (brrsz)(packets - wem->data)))) {
			Err(,"Insufficient data to copy vorbis %s header packet.", vorbishdr(current_header));
			return err;
		}

		oggpack_buffer unpacker, packer;
		oggpack_writeinit(&packer);
		oggpack_readinit(&unpacker, packeteer.payload, packeteer.payload_size);
		switch (current_header) {
			case vorbishdr_id: err = i_copy_id_header(&unpacker, &packer); break;
			case vorbishdr_comment: err = i_copy_comment_header(&unpacker, &packer); break;
			case vorbishdr_setup: err = i_copy_setup_header(&unpacker, &packer); break;
		}
		if (err) {
			Err(,"Could not copy vorbis %s header.", vorbishdr(current_header));
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
	return 0;
}

/****************************************
  Build headers; they must be rebuilt from modified forms present in the wem.
****************************************/
static int
i_build_id_header(oggpack_buffer *const packer, const wwriff_t *const wem)
{
	const wwise_fmt_t fmt = wem->fmt;
	const wwise_vorb_t vorb = wem->vorb;
	int packet_type = nepack_pack(packer, 1, 8); /* W Packet type */
	/* W Vorbis string */
	for (int i = 0; i < sizeof(VORBIS)-1; ++i)
		nepack_pack(packer, VORBIS[i], 8);
	long version     = nepack_pack(packer,                     0, 32); /* W Vorbis version */
	int  n_channels  = nepack_pack(packer,        fmt.n_channels,  8); /* W Audio channels */
	long sample_rate = nepack_pack(packer,   fmt.samples_per_sec, 32); /* W Sample rate */
	long bitrate_max = nepack_pack(packer,                     0, 32); /* W Bitrate maximum */
	long bitrate_nom = nepack_pack(packer, 8 * fmt.avg_byte_rate, 32); /* W Bitrate nominal */
	long bitrate_min = nepack_pack(packer,                     0, 32); /* W Bitrate minimum */
	int  blocksize_0 = nepack_pack(packer,      vorb.blocksize_0,  4); /* W Blocksize 0 */
	int  blocksize_1 = nepack_pack(packer,      vorb.blocksize_1,  4); /* W Blocksize 1 */
	int  frame_flag  = nepack_pack(packer,                     1,  1); /* W Frame flag */
	/* TODO those bitrates I think could be more accurate, though I'm not sure how best I'd improve them. */
	return 0;
}
static int
i_build_comments_header(oggpack_buffer *const packer, const wwriff_t *const wem)
{
	static const char vendor_string[] = "NieR:Automated extraction Precept_v"Ne_version;
	static long vendor_len = sizeof(vendor_string) - 1;

	nepack_pack(packer, 3, 8); /* W Packet type */
	/* W Vorbis string */
	for (int i = 0; i < 6; ++i)
		nepack_pack(packer, VORBIS[i], 8);

	nepack_pack(packer, vendor_len, 32); /* W Vendor string length */
	/* W Vendor string */
	for (long i = 0; i < vendor_len; ++i)
		nepack_pack(packer, vendor_string[i], 8);

	nepack_pack(packer, wem->n_comments, 32); /* W Comment list length */
	if (wem->comments) {
		for (brru4 i = 0; i < wem->n_comments; ++i) {
			brrstringr_t comment = wem->comments[i];
			nepack_pack(packer, comment.length, 32);
			for (brru4 i = 0; i < comment.length; ++i)
				nepack_pack(packer, comment.cstr[i], 8);
		}
	}

	nepack_pack(packer, 1, 1); /* W Framing flag */
	return 0;
}

static int
i_build_codebook(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	for (int i = 0; i < sizeof(CODEBOOK)-1; ++i) /* W Codebook sync */
		nepack_pack(packer, CODEBOOK[i], 8);

	int dimensions = nepack_transfer(unpacker,  4, packer, 16); /* R/W Dimensions */
	int entries    = nepack_transfer(unpacker, 14, packer, 24); /* R/W Entries */
	int ordered    = nepack_transfer(unpacker,  1, packer,  1); /* R/W Ordered flag */
	if (ordered) { /* Ordered codeword decode identical to spec */
		int current_length = 1 + nepack_transfer(unpacker, 5, packer, 5); /* R/W Start length */
		long current_entry = 0;
		while (current_entry < entries) {
			int number_bits = neutil_count_bits(entries - current_entry);
			long number = nepack_transfer(unpacker, number_bits, packer, number_bits); /* R/W Magic number */
			current_entry += number;
			current_length++;
		}
		if (current_entry > entries) {
			Err(,"Corrupt ordered entries when rebuilding codebook");
			return E_CORRUPT;
		}
	} else {
		int codeword_length_bits, sparse;
		codeword_length_bits = nepack_unpack(unpacker, 3);     /* R Codeword length bits */
		if (codeword_length_bits < 0 || codeword_length_bits > 5) {
			Err(,"Bad codeword length bits %i; must be (0,5]", codeword_length_bits);
			return E_CORRUPT;
		}
		sparse = nepack_transfer(unpacker, 1, packer, 1);   /* R/W Sparse flag */
		if (!sparse) { /* R/W Nonsparse codeword lengths */
			for (int i = 0; i < entries; ++i) {
				int length = nepack_transfer(unpacker, codeword_length_bits, packer, 5);
			}
		} else { /* R/W Sparse codeword lengths */
			for (int i = 0; i < entries; ++i) {
				int used = nepack_transfer(unpacker, 1, packer, 1); /* R/W Used flag */
				if (used) {
					int length = nepack_transfer(unpacker, codeword_length_bits, packer, 5); /* R/W Codeword length */
				}
			}
		}
	}

	int lookup = nepack_transfer(unpacker, 1, packer, 4); /* R/W Lookup type */
	/* Lookup 1 decode identical to spec */
	if (lookup == 1) {
		long minval_packed =     nepack_transfer(unpacker, 32, packer, 32); /* R/W Minimum value */
		long delval_packed =     nepack_transfer(unpacker, 32, packer, 32); /* R/W Delta value */
		int  value_bits    = 1 + nepack_transfer(unpacker,  4, packer,  4); /* R/W Value bits */
		int  sequence_flag =     nepack_transfer(unpacker,  1, packer,  1); /* R/W Sequence flag */

		long lookup_values = neutil_lookup1(entries, dimensions);

		/* R/W Codebook multiplicands */
		for (long i = 0; i < lookup_values; ++i) {
			long multiplicand = nepack_transfer(unpacker, value_bits, packer, value_bits);
		}
	} else {
		Err(,"LOOKUP FAILED");
	}
	return 0;
}
static int
i_build_floors(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	/* Floor 1 decode mostly identical to spec, except floor type is absent from
	 * each floor (because there is only a single used floor type) */

	int floor_count = 1 + nepack_transfer(unpacker, 6, packer, 6); /* R/W Floor count */
	for (int i = 0; i < floor_count; ++i) {
		long floor_type = nepack_pack(packer, 1, 16); /* W Floor type */
		int  partitions = nepack_transfer(unpacker, 5, packer, 5); /* R/W Floor partitions */
		int  partition_classes[31];
		int  max_class = -1;
		/* R/W Partition classes */
		for (int j = 0; j < partitions; ++j) {
			int class = partition_classes[j] = nepack_transfer(unpacker, 4, packer, 4);
			if (class > max_class)
				max_class = class;
		}

		int class_dims[16], class_subs[16], class_books[16];
		int sub_books[16][16];
		for (int j = 0; j <= max_class; ++j) {
			int n_dims = class_dims[j] = 1 + nepack_transfer(unpacker, 3, packer, 3); /* R/W Class dimensions */
			int n_subs = class_subs[j] =     nepack_transfer(unpacker, 2, packer, 2); /* R/W Class subclasses */
			/* R/W Class books */
			if (n_subs) {
				int master = class_books[j] = nepack_transfer(unpacker, 8, packer, 8);
			}

			int limit_break = 1 << n_subs;
			/* R/W Subclass books */
			for (int k = 0; k < limit_break; ++k) {
				sub_books[j][k] = -1 + nepack_transfer(unpacker, 8, packer, 8);
			}
		}

		int multiplier = 1 + nepack_transfer(unpacker, 2, packer, 2); /* R/W Floor multiplier */
		int rangebits  =     nepack_transfer(unpacker, 4, packer, 4); /* R/W Floor rangebits */
		for (int j = 0; j < partitions; ++j) {
			int dims = class_dims[partition_classes[j]];
			/* R/W Floor X list */
			for (int k = 0; k < dims; ++k) {
				long X = nepack_transfer(unpacker, rangebits, packer, rangebits);
			}
		}
	}
	return 0;
}
static int
i_build_residues(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	/* As far as I can tell, residue decode is identical to spec */
	int residue_count = 1 + nepack_transfer(unpacker, 6, packer, 6); /* R/W Residue count */
	for (int i = 0; i < residue_count; ++i) {
		int type = nepack_transfer(unpacker, 2, packer, 16);  /* R/W Residue type */
		if (type > 2) {
			Err(,"Bad residue type %i", type);
			return E_CORRUPT;
		}

		long start          =     nepack_transfer(unpacker, 24, packer, 24); /* R/W Residue begin */
		long end            =     nepack_transfer(unpacker, 24, packer, 24); /* R/W Residue end */
		long partition_size = 1 + nepack_transfer(unpacker, 24, packer, 24); /* R/W Partition size */
		int  classes        = 1 + nepack_transfer(unpacker,  6, packer,  6); /* R/W Residue classes */
		int  classbook      =     nepack_transfer(unpacker,  8, packer,  8); /* R/W Residue classbook */

		int cascades[64];
		int acc = 0; /* ??????? */
		/* R/W Residue cascades */
		for (int j = 0; j < classes; ++j) {
			int bitflag;
			cascades[j] = nepack_transfer(unpacker, 3, packer, 3);          /* R/W Cascade low-bits */
			bitflag = nepack_transfer(unpacker, 1, packer, 1);              /* R/W Cascade bitflag */
			if (bitflag)
				cascades[j] += 8 * nepack_transfer(unpacker, 5, packer, 5); /* R/W Cascade high-bits */

			acc += neutil_count_ones(cascades[j]);
		}
		/* R/W Residue books */
		for (int j = 0; j < acc; ++j) {
			int residue_book_index_jb = nepack_transfer(unpacker, 8, packer, 8);
		}
	}
	return 0;
}
static int
i_build_mappings(oggpack_buffer *const unpacker, oggpack_buffer *const packer, int n_channels)
{
	const int n_channel_bits = neutil_count_bits(n_channels - 1);
	const int mapping_count = 1 + nepack_transfer(unpacker, 6, packer, 6); /* R/W Mapping count */
	for (int i = 0; i < mapping_count; ++i) {
		long mapping_type = nepack_pack(packer, 0, 16); /* W Mapping type */
		int submaps_flag = nepack_transfer(unpacker, 1, packer, 1); /* R/W Submaps flag */
		int submaps = 1;
		if (submaps_flag)
			submaps = 1 + nepack_transfer(unpacker, 4, packer, 4); /* R/W Submaps */

		int square_mapping = nepack_transfer(unpacker, 1, packer, 1);  /* R/W Square mapping flag */
		if (square_mapping) {
			int coupling_steps = 1 + nepack_transfer(unpacker, 8, packer, 8); /* R/W Coupling steps */
			/* R/W Mapping vectors */
			for (int j = 0; j < coupling_steps; ++j) {
				long mapping_magnitude = nepack_transfer(unpacker, n_channel_bits, packer, n_channel_bits);
				long mapping_angle = nepack_transfer(unpacker, n_channel_bits, packer, n_channel_bits);
			}
		}

		int reserved = nepack_unpack(unpacker, 2); /* R Reserved */
		nepack_pack(packer, 0, 2); /* W Reserved */
		/* R/W Mapping channel multiplexes */
		if (submaps > 1) {
			for (int j = 0; j < n_channels; ++j) {
				int mapping_mux = nepack_transfer(unpacker, 4, packer, 4);
			}
		}
		/* R/W Submap configurations */
		for (int i = 0; i < submaps; ++i) {
			int discarded = nepack_transfer(unpacker, 8, packer, 8);
			int floor = nepack_transfer(unpacker, 8, packer, 8);
			int residue = nepack_transfer(unpacker, 8, packer, 8);
		}
	}
	return 0;
}
static int
i_build_modes(oggpack_buffer *const unpacker, oggpack_buffer *const packer, brru1 *const mode_blockflags, int *const mode_count)
{
	int md_count = 1 + nepack_transfer(unpacker, 6, packer, 6); /* R/W Mode count */
	for (int i = 0; i < md_count; ++i) {
		int  blockflag      = nepack_transfer(unpacker, 1, packer, 1); /* R/W Blockflag */
		long window         = nepack_pack(packer, 0, 16); /* W Window type */
		long transform_type = nepack_pack(packer, 0, 16); /* W Transform type */
		int  mapping        = nepack_transfer(unpacker, 8, packer, 8); /* R/W Mode mapping */
		mode_blockflags[i] = blockflag;
	}
	*mode_count = md_count;
	return 0;
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
		Err(,"Failed to initialize vorbis setup header packet.");
		return err;
	}

	oggpack_buffer unpacker;
	oggpack_readinit(&unpacker, packeteer.payload, packeteer.payload_size);

	int stripped = s_current_input->flag.stripped_headers;

	nepack_pack(packer, 5, 8); /* W Packet type (setup header = 5) */
	for (int i = 0; i < 6; ++i) /* W Vorbis string */
		nepack_pack(packer, VORBIS[i], 8);

	int codebook_count = 1 + nepack_transfer(&unpacker, 8, packer, 8); /* R/W Codebook count */
	if (!s_used_library) {
		/* Internal codebooks */
		if (!stripped) {
			/* Full codebooks, can be copied from header directly */
			for (int i = 0; i < codebook_count; ++i) {
				ExtraDeb(,"Copying internal codebook %d", i);
				if ((err = i_copy_next_codebook(&unpacker, packer))) {
					Err(,"Failed to copy codebook %d", i);
					return err;
				}
			}

		} else {
			/* Stripped codebooks, need to be unpacked/rebuilt to spec */
			for (int i = 0; i < codebook_count; ++i) {
				if ((err = packed_codebook_unpack_raw(&unpacker, packer))) {
					Err(,"Failed to build codebook %d", i);
					return err;
				}
			}
		}

	} else {
		/* TODO this part I understand the least */
		/* External codebooks */
		for (int i = 0; i < codebook_count; ++i) {
			packed_codebook_t *cb = NULL;
			int cbidx = 1 + nepack_unpack(&unpacker, 10); /* R Codebook index */
			/* I don't know why it's off by 1; ww2ogg just sorta rolls with it
			 * without too much checking (specifically in get_codebook_size) and
			 * I can't figure out why it works there */
			if (cbidx > s_used_library->codebook_count) {
				/* This bit ripped from ww2ogg, no idea what it means */
				if (cbidx == 0x342) {
					cbidx = nepack_unpack(&unpacker, 14);      /* R Codebook id */
					if (cbidx == 0x1590) {
						/* ??? */
					}
				}
				Err(,"Codebook index too large %d", cbidx);
				return E_CORRUPT;
			}

			cb = &s_used_library->codebooks[cbidx];
			if ((err = packed_codebook_unpack(cb))) { /* Copy from external */
				if (err == CODEBOOK_ERROR)
					err = E_BUFFER;
				else if (err == CODEBOOK_CORRUPT)
					err = E_CORRUPT;
				Err(,"Failed to copy external codebook %d", cbidx);
				return err;
			} else {
				oggpack_buffer cb_unpacker;
				oggpack_readinit(&cb_unpacker, cb->unpacked_data, (cb->unpacked_bits + 7) / 8);
				if (-1 == nepack_transfer_lots(&cb_unpacker, packer, cb->unpacked_bits))
					return E_BUFFER;
			}
		}
	}

	nepack_pack(packer, 0, 6);  /* W Time count - 1 */
	nepack_pack(packer, 0, 16); /* W Vorbis time-domain stuff */

	if (!stripped) {
		/* Rest of the header in-spec, copy verbatim */
		if (-1 == (err = nepack_transfer_remaining(&unpacker, packer))) {
			Err(,"Failed to copy the rest of vorbis setup packet");
			return E_CORRUPT;
		}

	} else {
		/* Need to rebuild the setup header */
		if ((err = i_build_floors(&unpacker, packer))) {
			Err(,"Failed to rebuild floors");
			return err;
		}
		if ((err = i_build_residues(&unpacker, packer))) {
			Err(,"Failed to rebuild residues");
			return err;
		}
		if ((err = i_build_mappings(&unpacker, packer, wem->fmt.n_channels))) {
			Err(,"Failed to rebuild mappings");
			return err;
		}
		if ((err = i_build_modes(&unpacker, packer, wem->mode_blockflags, &wem->mode_count))) {
			Err(,"Failed to rebuild modes");
			return err;
		}
	}

	nepack_pack(packer, 1, 1); /* W Frame flag */
	return 0;
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
			Err(n,"Failed to build vorbis %s header", vorbishdr(current_header));
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
	return 0;
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
	const int mode_count_bits = neutil_count_bits(wem->mode_count - 1);
	brru4 packets_start = wem->vorb.audio_start_offset;
	brru4 packets_size = wem->data_size - wem->vorb.audio_start_offset;
	i_packeteer_t packeteer = {0};

	int prev_blockflag = 0;
	brru8 last_block = 0;
	brru8 total_block = 0;
	brru8 packetno = 0;

	int err = 0;
	SExtraDeb(,normal,"Stream mod packets");
	while (packets_start < wem->data_size) {
		int eos = 0;
		if ((err = i_packeteer_init(&packeteer, wem->data + packets_start, packets_size, wem->flags, packets_start))) {
			Err(,"Insufficient data to build next audio packet %lld", packetno);
			return err;
		}

		brru4 next_start = packets_start + packeteer.total_size;

		oggpack_buffer unpacker, packer;
		oggpack_readinit(&unpacker, packeteer.payload, packeteer.payload_size);
		oggpack_writeinit(&packer);

		if (wem->flags.mod_packets) {
			int packet_type = nepack_pack(&packer, 0, 1); /* W Packet type */
			int mode_number = nepack_transfer(&unpacker, mode_count_bits, &packer, mode_count_bits); /* R/W Mode number */
			int remainder = nepack_unpack(&unpacker, 8 - mode_count_bits); /* R Remainder bits */
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
					int next_number = nepack_unpack(&next_unpacker, mode_count_bits); /* R Next number */
					next_blockflag = wem->mode_blockflags[next_number];
				}
				nepack_pack(&packer, prev_blockflag, 1); /* W Previous window type */
				nepack_pack(&packer, next_blockflag, 1); /* W Next window type */
			}
			nepack_pack(&packer, remainder, 8 - mode_count_bits); /* W Remainder of read-in first byte */
			prev_blockflag = wem->mode_blockflags[mode_number];
		}

		long a = nepack_transfer_remaining(&unpacker, &packer);
		ExtraDeb(,"Transferrined %lld remaining bytes", a);

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
		ExtraDeb(,"Block %lld | Total Block %lld", current_block, total_block);

		if ((err = i_insert_packet(streamer, &packet))) {
			oggpack_writeclear(&packer);
			return err;
		}

		oggpack_writeclear(&packer);
		packets_start += packeteer.total_size;
		packets_size  -= packeteer.total_size;
		++packetno;
	}
	ExtraDeb(,"Total packets: %lld", 3 + packetno);
	return 0;
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
		if (E_OGG_SUCCESS != ogg_stream_init(out_stream, serialno)) {
			Err(,"Failed to initialize ogg stream for output.");
			return -1;
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
