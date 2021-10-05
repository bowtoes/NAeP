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

#include <vorbis/vorbisenc.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrpath.h>

#include "errors.h"
#include "lib.h"
#include "packer.h"

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

static const codebook_libraryT *glibrary = NULL;
static const neinputT *ginput = NULL;

/* State init/input reading */
static int BRRCALL
i_init_state(ogg_stream_state *const streamer, const wwise_wemT *const wem,
    vorbis_info *const vi, vorbis_comment *const vc)
{
	int serialno = 0;
	serialno = wem->vorb.uid;
	if (STREAM_INIT_SUCCESS != ogg_stream_init(streamer, serialno)) {
		return I_INIT_ERROR;
	}
	vorbis_info_init(vi);
	vorbis_comment_init(vc);
	return I_SUCCESS;
}

static int BRRCALL
i_build_packet(ogg_packet *const packet, oggpack_buffer *const packer, brru8 packetno, brru8 granule, int end_of_stream)
{
	packet->packet = oggpack_get_buffer(packer);
	packet->bytes = oggpack_bytes(packer);
	packet->b_o_s = packetno == 0;
	packet->e_o_s = end_of_stream != 0;
	packet->granulepos = granule;
	packet->packetno = packetno;
	return 0;
}
static int BRRCALL
i_insert_packet(ogg_stream_state *const streamer, ogg_packet *const packet, vorbis_info *const vi, vorbis_comment *const vc)
{
	if (ogg_stream_packetin(streamer, packet)) {
		return I_BUFFER_ERROR;
	} else if (vi || vc) {
		int err = 0;
		if (!vi || !vc) {
			return I_BAD_ERROR;
		} else if ((err = vorbis_synthesis_headerin(vi, vc, packet))) {
#if defined(NeEXTRA_DEBUG)
			BRRLOG_ERRN("Failed to synthesize header %d : ", packet->packetno);
			if (err == OV_ENOTVORBIS)
				BRRLOG_ERRNP("NOT VORBIS");
			else if (err == OV_EBADHEADER)
				BRRLOG_ERRNP("BAD HEADER");
			else
				BRRLOG_ERRNP("INTERNAL ERROR");
#endif
			return I_CORRUPT;
		}
	}
	return I_SUCCESS;
}

/* COPY */
static int BRRCALL
i_copy_id_header(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	brrs4 packet_type = 0, vorbis[6] = {0}, audio_channels, blocksize_0, blocksize_1, frame_flag;
	brrs8 version = 0, sample_rate, bitrate_max, bitrate_nom, bitrate_min;

	packet_type = packer_unpack(unpacker, 8);        /* IN Packet type, should be 1 */
	for (int i = 0; i < 6; ++i)
		vorbis[i] = packer_unpack(unpacker, 8);      /* IN Vorbis str, should read 'vorbis' */
	version = packer_unpack(unpacker, 32);           /* IN Version, should be 0 */
	audio_channels = packer_unpack(unpacker, 8);     /* IN Audio channels */
	sample_rate = packer_unpack(unpacker, 32);       /* IN Sample rate */
	bitrate_max = packer_unpack(unpacker, 32);       /* IN Bitrate maximum */
	bitrate_nom = packer_unpack(unpacker, 32);       /* IN Bitrate nominal */
	bitrate_min = packer_unpack(unpacker, 32);       /* IN Bitrate minimum */
	blocksize_0 = packer_unpack(unpacker, 4);        /* IN Blocksize 0 */
	blocksize_1 = packer_unpack(unpacker, 4);        /* IN Blocksize 1 */
	frame_flag = packer_unpack(unpacker, 1);         /* IN Frame flag, should be 1 */

	packer_pack(packer, 1, 8);                    /* OUT Packet type */
	for (int i = 0; i < 6; ++i)                     /* OUT Vorbis str */
		packer_pack(packer, VORBIS_STR[i], 8);
	packer_pack(packer, 0, 32);                   /* OUT Version */
	packer_pack(packer, audio_channels, 8);       /* OUT Audio channels */
	packer_pack(packer, sample_rate, 32);         /* OUT Sample rate */
	packer_pack(packer, bitrate_max, 32);         /* OUT Bitrate maximum */
	packer_pack(packer, bitrate_nom, 32);         /* OUT Bitrate nominal */
	packer_pack(packer, bitrate_min, 32);         /* OUT Bitrate minimum */
	packer_pack(packer, blocksize_0, 4);          /* OUT Blocksize 0 */
	packer_pack(packer, blocksize_1, 4);          /* OUT Blocksize 1 */
	packer_pack(packer, 1, 1);                    /* OUT Frame flag */
	return I_SUCCESS;
}
static int BRRCALL
i_copy_comment_header(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	brrs4 packet_type = 0, vorbis[6] = {0}, frame_flag = 0;
	brrs8 vendor_length = 0, comments_count = 0;

	packet_type = packer_unpack(unpacker, 8);        /* IN Packet type, should be 3 */
	packer_pack(packer, 3, 8);                    /* OUT Packet type */

	for (int i = 0; i < 6; ++i)                     /* IN Vorbis str, should read 'vorbis' */
		vorbis[i] = packer_unpack(unpacker, 8);
	for (int i = 0; i < 6; ++i)                     /* OUT Vorbis str */
		packer_pack(packer, VORBIS_STR[i], 8);

	vendor_length = packer_transfer(unpacker, 32, packer, 32);      /* IN/OUT Vendor length */
	for (brrs8 i = 0; i < vendor_length; ++i) {                       /* IN/OUT Vendor string */
		char vendor_str = packer_transfer(unpacker, 32, packer, 8);
	}
	comments_count = packer_transfer(unpacker, 32, packer, 32);     /* IN/OUT Comment list length */
	for (brrs8 i = 0; i < comments_count; ++i) {
		brrs8 comment_length = packer_transfer(unpacker, 32, packer, 32); /* IN/OUT Comment length */
		for (brrs8 j = 0; j < comment_length; ++j) {                        /* IN/OUT Comment string */
			char comment_str = packer_transfer(unpacker, 8, packer, 8);
		}
	}
	frame_flag = packer_unpack(unpacker, 1);         /* IN Frame flag, should be 1 */
	packer_pack(packer, 1, 1);                    /* OUT Frame flag */
	return I_SUCCESS;
}
static int BRRCALL
i_copy_next_codebook(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	long dimensions, entries;
	int ordered, lookup;
	for (int i = 0; i < 3; ++i) { /* IN/OUT Codebook sync */
		if (CODEBOOK_SYNC[i] != packer_transfer(unpacker, 8, packer, 8))
			return I_CORRUPT;
	}
	dimensions = packer_transfer(unpacker, 16, packer, 16); /* IN/OUT Codebook dimensions */
	entries = packer_transfer(unpacker, 24, packer, 24);    /* IN/OUT Codebook entries */
	ordered = packer_transfer(unpacker, 1, packer, 1);      /* IN/OUT Ordered flag */
	if (ordered) {
		int current_length = 1 + packer_transfer(unpacker, 5, packer, 5); /* IN/OUT Start length */
		long current_entry = 0;
		while (current_entry < entries) {
			int number_bits = lib_count_bits(entries - current_entry);
			long number = packer_transfer(unpacker, number_bits, packer, number_bits); /* IN/OUT Magic number */
			current_entry += number;
			current_length++;
		}
		if (current_entry > entries)
			return I_CORRUPT;
	} else {
		int sparse = packer_transfer(unpacker, 1, packer, 1); /* IN/OUT Sparse flag */
		for (long i = 0; i < entries; ++i) {
			if (!sparse) {
				int length = 1 + packer_transfer(unpacker, 5, packer, 5); /* IN/OUT Codeword length */
			} else {
				int used = packer_transfer(unpacker, 1, packer, 1); /* IN/OUT Used flag */
				if (used) {
					int length = 1 + packer_transfer(unpacker, 5, packer, 5); /* IN/OUT Codeword length */
				}
			}
		}
	}
	lookup = packer_transfer(unpacker, 4, packer, 4); /* IN/OUT Lookup type */
	if (lookup) {
		long minval_packed = 0, delval_packed = 0;
		int value_bits = 0, sequence_flag = 0;
		long lookup_values = 0;
		if (lookup > 2)
			return I_CORRUPT;
		minval_packed = packer_transfer(unpacker, 32, packer, 32); /* IN/OUT Minimum value */
		delval_packed = packer_transfer(unpacker, 32, packer, 32); /* IN/OUT Delta value */
		value_bits = 1 + packer_transfer(unpacker, 4, packer, 4);  /* IN/OUT Value bits */
		sequence_flag = packer_transfer(unpacker, 1, packer, 1);   /* IN/OUT Sequence flag */
		if (lookup == 1)
			lookup_values = lib_lookup1_values(entries, dimensions);
		else
			lookup_values = entries * dimensions;

		for (long i = 0; i < lookup_values; ++i) { /* IN/OUT Codebook multiplicands */
			long multiplicand = packer_transfer(unpacker, value_bits, packer, value_bits);
		}
	}
	return I_SUCCESS;
}
static int BRRCALL
i_copy_setup_header(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	int packet_type = 0, vorbis[6] = {0}, codebook_count = 0;

	packet_type = packer_unpack(unpacker, 8);        /* IN Packet type, should be 5 */
	packer_pack(packer, 5, 8);                    /* OUT Packet type */

	for (int i = 0; i < 6; ++i)                     /* IN Vorbis str, should read 'vorbis' */
		vorbis[i] = packer_unpack(unpacker, 8);
	for (int i = 0; i < 6; ++i)                     /* OUT Vorbis str */
		packer_pack(packer, VORBIS_STR[i], 8);

	codebook_count = 1 + packer_transfer(unpacker, 8, packer, 8); /* IN/OUT Codebooks counts */
	if (!glibrary || 1) { /* Inline codebooks, copy verbatim */
		// For now, copy verbatim
		for (int err = 0, i = 0; i < codebook_count; ++i) {
			if ((err = i_copy_next_codebook(unpacker, packer)))
				return err;
		}
	} else { /* Optionally external codebooks, copy from those instead */
		return I_BAD_ERROR;
	}
	/* Now copy the rest of it (naive shallow copy) */
	packer_transfer_remaining(unpacker, packer);
	return I_SUCCESS;
}
static int BRRCALL
i_copy_headers(ogg_stream_state *const streamer, wwise_wemT *const wem,
    vorbis_info *const vi, vorbis_comment *const vc)
{
	int err = 0;
	unsigned char *packets_start = wem->data + wem->vorb.header_packets_offset;
	brru4 packets_size = wem->vorb.audio_start_offset - wem->vorb.header_packets_offset;
	wwise_packetT packeteer = {0};

	/* Should check for WWISE_INCOMPLETE? */
	for (int current_header = 0; current_header < 3; ++current_header) {
		ogg_packet packet;
		oggpack_buffer unpacker, packer;
		if (WWISE_SUCCESS != wwise_packet_init(&packeteer, wem, packets_start, packets_size))
			return I_INSUFFICIENT_DATA;
		oggpack_readinit(&unpacker, packeteer.payload, packeteer.payload_size);
		oggpack_writeinit(&packer);
		switch (current_header) {
			case 0: err = i_copy_id_header(&unpacker, &packer); break;
			case 1: err = i_copy_comment_header(&unpacker, &packer); break;
			case 2: err = i_copy_setup_header(&unpacker, &packer); break;
		}
		if (err) {
			oggpack_writeclear(&packer);
			return err;
		} else if ((err = i_build_packet(&packet, &packer, current_header, 0, 0))) {
			oggpack_writeclear(&packer);
			return err;
		} else if ((err = i_insert_packet(streamer, &packet, vi, vc))) {
			oggpack_writeclear(&packer);
			return err;
		}
		oggpack_writeclear(&packer);
		packets_start += packeteer.header_length + packeteer.payload_size;
		packets_size -= packeteer.header_length + packeteer.payload_size;
	}
	return I_SUCCESS;
}
/* BUILD */
static int BRRCALL
i_build_id_header(oggpack_buffer *const packer, const wwise_wemT *const wem)
{
	packer_pack(packer, 1, 8);                           /* OUT Packet type */
	for (int i = 0; i < 6; ++i)                            /* OUT Vorbis string */
		packer_pack(packer, VORBIS_STR[i], 8);
	packer_pack(packer, 0, 32);                          /* OUT Vorbis version */
	packer_pack(packer, wem->fmt.n_channels, 8);         /* OUT Audio channels */
	packer_pack(packer, wem->fmt.samples_per_sec, 32);   /* OUT Sample rate */
	packer_pack(packer, 0, 32);                          /* OUT Bitrate maximum */
	packer_pack(packer, wem->fmt.avg_byte_rate * 8, 32); /* OUT Bitrate nominal */
	packer_pack(packer, 0, 32);                          /* OUT Bitrate minimum */
	packer_pack(packer, wem->vorb.blocksize[0], 4);      /* OUT Blocksize 0 */
	packer_pack(packer, wem->vorb.blocksize[1], 4);      /* OUT Blocksize 1 */
	packer_pack(packer, 1, 1);                           /* OUT Frame flag */
	return I_SUCCESS;
}
static int BRRCALL
i_build_comments_header(oggpack_buffer *const packer)
{
	static const char vendor_format[] = "NieR:Automated extraction Precept_v"NeVERSION" - Generator file '%s'";
	static char vendor_string[sizeof(vendor_format) + BRRPATH_MAX_PATH + 1] = "";
	static long vendor_len = 0;

	packer_pack(packer, 3, 8);                            /* OUT Packet type */
	for (int i = 0; i < 6; ++i)                           /* OUT Vorbis string */
		packer_pack(packer, VORBIS_STR[i], 8);

	vendor_len = snprintf(vendor_string, sizeof(vendor_string), vendor_format, ginput->path);
	packer_pack(packer, vendor_len, 32); /* OUT Vendor string length */
	for (long i = 0; i < vendor_len; ++i)  /* OUT Vendor string */
		packer_pack(packer, vendor_string[i], 8);
	packer_pack(packer, 0, 32);                           /* OUT Comment list length */
	packer_pack(packer, 1, 1);                            /* OUT Frame flag */
	return I_SUCCESS;
}
static int BRRCALL
i_build_codebook(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	int dimensions, entries, ordered, lookup;

	for (int i = 0; i < 3; ++i) /* OUT Codebook sync */
		packer_pack(packer, CODEBOOK_SYNC[i], 8);

	dimensions = packer_transfer(unpacker,  4, packer, 16); /* IN/OUT Dimensions */
	entries = packer_transfer(unpacker, 14, packer, 24);    /* IN/OUT Entries */
	ordered = packer_transfer(unpacker, 1, packer, 1);      /* IN/OUT Ordered flag */
	if (ordered) { /* Ordered codeword decode identical to spec */
		int current_length = 1 + packer_transfer(unpacker, 5, packer, 5); /* IN/OUT Start length */
		long current_entry = 0;
		while (current_entry < entries) {
			int number_bits = lib_count_bits(entries - current_entry);
			long number = packer_transfer(unpacker, number_bits, packer, number_bits); /* IN/OUT Magic number */
			current_entry += number;
			current_length++;
		}
		if (current_entry > entries)
			return I_CORRUPT;
	} else {
		int codeword_length_bits, sparse;
		codeword_length_bits = packer_unpack(unpacker, 3);     /* IN Codeword length bits */
		if (codeword_length_bits < 0 || codeword_length_bits > 5)
			return I_CORRUPT;
		sparse = packer_transfer(unpacker, 1, packer, 1);   /* IN/OUT Sparse flag */
		if (!sparse) { /* IN/OUT Nonsparse codeword lengths */
			for (int i = 0; i < entries; ++i) {
				int length = packer_transfer(unpacker, codeword_length_bits, packer, 5);
			}
		} else { /* IN/OUT Sparse codeword lengths */
			for (int i = 0; i < entries; ++i) {
				int used = packer_transfer(unpacker, 1, packer, 1); /* IN/OUT Used flag */
				if (used) {
					int length = packer_transfer(unpacker, codeword_length_bits, packer, 5); /* IN/OUT Codeword length */
				}
			}
		}
	}

	lookup = packer_transfer(unpacker, 1, packer, 4); /* IN/OUT Lookup type */
	if (lookup == 1) { /* Lookup 1 decode identical to spec */
		long minval_packed = packer_transfer(unpacker, 32, packer, 32); /* IN/OUT Minimum value */
		long delval_packed = packer_transfer(unpacker, 32, packer, 32); /* IN/OUT Delta value */
		int value_bits = 1 + packer_transfer(unpacker, 4, packer, 4);  /* IN/OUT Value bits */
		int sequence_flag = packer_transfer(unpacker, 1, packer, 1);   /* IN/OUT Sequence flag */

		long lookup_values = lib_lookup1_values(entries, dimensions);

		for (long i = 0; i < lookup_values; ++i) { /* IN/OUT Codebook multiplicands */
			long multiplicand = packer_transfer(unpacker, value_bits, packer, value_bits);
		}
#if defined(NeEXTRA_DEBUG)
	} else {
		BRRLOG_ERR("LOOKUP FAILED");
#endif
	}
	return I_SUCCESS;
}
static int BRRCALL
i_build_floors(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	/* Floor 1 decode mostly identical to spec, except floor type is absent from
	 * each floor (because there is only a single floor type) */
	int floor_count = 1 + packer_transfer(unpacker, 6, packer, 6); /* IN/OUT Floor count */
	for (int i = 0; i < floor_count; ++i) {
		int partitions, partition_classes[31], max_class = -1;
		int class_dims[16], class_subs[16], class_books[16], sub_books[16][16];
		int multiplier, rangebits;
		packer_pack(packer, 1, 16);                            /* OUT Floor type, always 1 */
		partitions = packer_transfer(unpacker, 5, packer, 5);  /* IN/OUT Floor partitions */
		for (int j = 0; j < partitions; ++j) {                   /* IN/OUT Partition classes */
			int class = partition_classes[j] = packer_transfer(unpacker, 4, packer, 4);
			if (class > max_class)
				max_class = class;
		}
		for (int j = 0; j <= max_class; ++j) {
			int dim = class_dims[j] = 1 + packer_transfer(unpacker, 3, packer, 3); /* IN/OUT Class dimensions */
			int sub = class_subs[j] = packer_transfer(unpacker, 2, packer, 2);     /* IN/OUT Class subclasses */
			int limit_break = 1 << sub;
			if (sub) {                                                               /* IN/OUT Class books */
				int master = class_books[j] = packer_transfer(unpacker, 8, packer, 8);
			}
			for (int k = 0; k < limit_break; ++k) {                                  /* IN/OUT Subclass books */
				sub_books[j][k] = -1 + packer_transfer(unpacker, 8, packer, 8);
			}
		}
		multiplier = 1 + packer_transfer(unpacker, 2, packer, 2); /* IN/OUT Floor multiplier */
		rangebits = packer_transfer(unpacker, 4, packer, 4);      /* IN/OUT Floor rangebits */
		for (int j = 0; j < partitions; ++j) {
			int dims = class_dims[partition_classes[j]];
			for (int k = 0; k < dims; ++k) {           /* IN/OUT Floor X list */
				long X = packer_transfer(unpacker, rangebits, packer, rangebits);
			}
		}
	}
	return I_SUCCESS;
}
static int BRRCALL
i_build_residues(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	/* As far as I can tell, residue decode is identical to spec */
	int residue_count = 1 + packer_transfer(unpacker, 6, packer, 6); /* IN/OUT Residue count */
	for (int i = 0; i < residue_count; ++i) {
		long start, end, partition_size;
		int classes, classbook;
		int type;
		int cascades[64];
		int acc = 0; /* ??????? */
		type = packer_transfer(unpacker, 2, packer, 16);  /* IN/OUT Residue type */
		if (type > 2)
			return I_CORRUPT;

		start = packer_transfer(unpacker, 24, packer, 24);              /* IN/OUT Residue begin */
		end = packer_transfer(unpacker, 24, packer, 24);                /* IN/OUT Residue end */
		partition_size = 1 + packer_transfer(unpacker, 24, packer, 24); /* IN/OUT Partition size */
		classes = 1 + packer_transfer(unpacker, 6, packer, 6);          /* IN/OUT Residue classes */
		classbook = packer_transfer(unpacker, 8, packer, 8);            /* IN/OUT Residue classbook */

		for (int j = 0; j < classes; ++j) {                               /* IN/OUT Residue cascades */
			int bitflag;
			cascades[j] = packer_transfer(unpacker, 3, packer, 3);          /* IN/OUT Cascade low-bits */
			bitflag = packer_transfer(unpacker, 1, packer, 1);              /* IN/OUT Cascade bitflag */
			if (bitflag)
				cascades[j] += 8 * packer_transfer(unpacker, 5, packer, 5); /* IN/OUT Cascade high-bits */

			acc += lib_count_set(cascades[j]);
		}
		for (int j = 0; j < acc; ++j) {/* IN/OUT Residue books */
			int residue_book_index_jb = packer_transfer(unpacker, 8, packer, 8);
		}
	}
	return I_SUCCESS;
}
static int BRRCALL
i_build_mappings(oggpack_buffer *const unpacker, oggpack_buffer *const packer, int n_channels)
{
	int mapping_count = 1 + packer_transfer(unpacker, 6, packer, 6); /* IN/OUT Mapping count */
	for (int i = 0; i < mapping_count; ++i) {
		int n_channel_bits = lib_count_bits(n_channels - 1);
		long mapping_type = 0; /* Mapping type always 0 */
		int submaps_flag, submaps = 1;
		int square_mapping, coupling_steps = 0;
		int reserved;
		packer_pack(packer, mapping_type, 16); /* OUT Mapping type */
		submaps_flag = packer_transfer(unpacker, 1, packer, 1); /* IN/OUT Submaps flag */

		if (submaps_flag)
			submaps = 1 + packer_transfer(unpacker, 4, packer, 4); /* IN/OUT Submaps */

		square_mapping = packer_transfer(unpacker, 1, packer, 1);  /* IN/OUT Square mapping flag */
		if (square_mapping) {
			coupling_steps = 1 + packer_transfer(unpacker, 8, packer, 8); /* IN/OUT Coupling steps */
			for (int j = 0; j < coupling_steps; ++j) { /* IN/OUT Mapping vectors */
				long mapping_magnitude = packer_transfer(unpacker, n_channel_bits, packer, n_channel_bits);
				long mapping_angle = packer_transfer(unpacker, n_channel_bits, packer, n_channel_bits);
			}
		}

		reserved = packer_unpack(unpacker, 2); /* IN Reserved */
		packer_pack(packer, 0, 2);          /* OUT Reserved */

		if (submaps > 1) {                    /* IN/OUT Mapping channel multiplexes */
			for (int j = 0; j < n_channels; ++j) {
				int mapping_mux = packer_transfer(unpacker, 4, packer, 4);
			}
		}
		for (int i = 0; i < submaps; ++i) { /* IN/OUT Submap configurations */
			int discarded = packer_transfer(unpacker, 8, packer, 8);
			int floor = packer_transfer(unpacker, 8, packer, 8);
			int residue = packer_transfer(unpacker, 8, packer, 8);
		}
	}
	return I_SUCCESS;
}
static int BRRCALL
i_build_modes(oggpack_buffer *const unpacker, oggpack_buffer *const packer,
    brru1 *const mode_blockflags, int *const mode_count)
{
	int md_count = 1 + packer_transfer(unpacker, 6, packer, 6); /* IN/OUT Mode count */
	*mode_count = md_count;
	for (int i = 0; i < md_count; ++i) {
		int blockflag, mapping;
		long window = 0, transform = 0;
		blockflag = packer_transfer(unpacker, 1, packer, 1); /* IN/OUT Blockflag */
		packer_pack(packer, window, 16);                     /* OUT Window type */
		packer_pack(packer, transform, 16);                  /* OUT Transform type */
		mapping = packer_transfer(unpacker, 8, packer, 8);   /* IN/OUT Mode mapping */

		mode_blockflags[i] = blockflag;
	}
	return I_SUCCESS;
}
static int BRRCALL
i_build_setup_header(oggpack_buffer *const unpacker, oggpack_buffer *const packer,
    wwise_wemT *const wem, int stripped)
{
	int codebook_count, err = 0;
	packer_pack(packer, 5, 8);                            /* OUT Packet type */
	for (int i = 0; i < 6; ++i)                             /* OUT Vorbis string */
		packer_pack(packer, VORBIS_STR[i], 8);

	codebook_count = 1 + packer_transfer(unpacker, 8, packer, 8); /* IN/OUT Codebook count */
	if (!glibrary) { /* Internal codebooks */
		if (!stripped) { /* Full codebooks, can be copied directly */
			for (int err = 0, i = 0; i < codebook_count; ++i) {
#if defined(NeEXTRA_DEBUG)
				BRRLOG_DEBUG("Copying internal codebook %d", i);
#endif
				if ((err = i_copy_next_codebook(unpacker, packer))) {
#if defined(NeEXTRA_DEBUG)
					BRRLOG_ERR("Failed to copy codebook %d", i);
#endif
					return err;
				}
			}
		} else {
			for (int err = 0, i = 0; i < codebook_count; ++i) {
#if defined(NeEXTRA_DEBUG)
				BRRLOG_DEBUG("Rebuilding internal codebook %d", i);
#endif
				if ((err = packed_codebook_unpack_raw(unpacker, packer))) {
#if defined(NeEXTRA_DEBUG)
					BRRLOG_ERR("Failed to build codebook %d", i);
#endif
					return err;
				}
			}
		}
	} else { /* External codebooks */
		for (int i = 0, err = 0; i < codebook_count; ++i) {
			packed_codebookT *cb = NULL;
			int cbidx = 1 + packer_unpack(unpacker, 10);      /* IN Codebook index */
			/* I don't know why it's off by 1; ww2ogg just sorta rolls with it
			 * without too much checking (specifically in get_codebook_size) and
			 * I can't figure out why it works there */
			if (cbidx > glibrary->codebook_count) {
				/* This bit ripped from ww2ogg, no idea what it means */
				if (cbidx == 0x342) {
					cbidx = packer_unpack(unpacker, 14);      /* IN Codebook id */
					if (cbidx == 0x1590) {
						/* ??? */
					}
				}
#if defined(NeEXTRA_DEBUG)
				BRRLOG_ERR("Codebook index too large %d", cbidx);
#endif
				return I_CORRUPT;
			}
#if defined(NeEXTRA_DEBUG)
			BRRLOG_DEBUG("Building external codebook %3d: ", cbidx);
#endif
			cb = &glibrary->codebooks[cbidx];
			if (CODEBOOK_SUCCESS != (err = packed_codebook_unpack(cb))) { /* Copy from external */
				if (err == CODEBOOK_ERROR)
					err = I_BUFFER_ERROR;
				else if (err == CODEBOOK_CORRUPT)
					err = I_CORRUPT;
#if defined(NeEXTRA_DEBUG)
				BRRLOG_ERR("Failed to copy external codebook %d : %s", cbidx, lib_strerr(err));
#endif
				return err;
			} else {
				oggpack_buffer cb_unpacker;
				oggpack_readinit(&cb_unpacker, cb->unpacked_data, (cb->unpacked_bits + 7) / 8);
				if (-1 == packer_transfer_lots(&cb_unpacker, packer, cb->unpacked_bits))
					return I_BUFFER_ERROR;
			}
		}
	}

	packer_pack(packer, 0, 6);  /* OUT Time count - 1 */
	packer_pack(packer, 0, 16); /* OUT Vorbis time-domain stuff */

	if (!stripped) { /* Rest of the header in-spec, copy verbatim */
		if (-1 == (err = packer_transfer_remaining(unpacker, packer))) {
#if defined(NeEXTRA_DEBUG)
			BRRLOG_ERR("Failed to copy the rest of setup packet");
#endif
			err = I_CORRUPT;
			return err;
		}
	} else { /* Need to rebuild the information */
		if ((err = i_build_floors(unpacker, packer))) {
#if defined(NeEXTRA_DEBUG)
			BRRLOG_ERR("Failed building floors");
#endif
			return err;
		} else if ((err = i_build_residues(unpacker, packer))) {
#if defined(NeEXTRA_DEBUG)
			BRRLOG_ERR("Failed building residues");
#endif
			return err;
		} else if ((err = i_build_mappings(unpacker, packer, wem->fmt.n_channels))) {
#if defined(NeEXTRA_DEBUG)
			BRRLOG_ERR("Failed building mappings");
#endif
			return err;
		} else if ((err = i_build_modes(unpacker, packer, wem->mode_blockflags, &wem->mode_count))) {
#if defined(NeEXTRA_DEBUG)
			BRRLOG_ERR("Failed building modes");
#endif
			return err;
		}
	}

	packer_pack(packer, 1, 1);                           /* OUT Frame flag */
	return I_SUCCESS;
}
static int BRRCALL
i_build_headers(ogg_stream_state *const streamer, wwise_wemT *const wem,
    vorbis_info *const vi, vorbis_comment *const vc)
{
	int err = 0;
	for (int current_header = 0; current_header < 3; ++current_header) {
		ogg_packet packet;
		oggpack_buffer packer;
		oggpack_writeinit(&packer);
		if (current_header == 0) {
			err = i_build_id_header(&packer, wem);
		} else if (current_header == 1) {
			err = i_build_comments_header(&packer);
		} else {
			oggpack_buffer unpacker;
			unsigned char *packets_start = wem->data + wem->vorb.header_packets_offset;
			brru4 packets_size = wem->vorb.audio_start_offset - wem->vorb.header_packets_offset;
			wwise_packetT packeteer = {0};
#if defined (NeEXTRA_DEBUG)
			BRRLOG_DEBUG("Building setup header");
#endif
			if (WWISE_SUCCESS != (err = wwise_packet_init(&packeteer, wem, packets_start, packets_size))) {
#if defined(NeEXTRA_DEBUG)
				BRRLOG_ERRN("Failed to init setup header packet (%d)", err);
#endif
				err = I_INSUFFICIENT_DATA;
			} else {
				oggpack_readinit(&unpacker, packeteer.payload, packeteer.payload_size);
				err = i_build_setup_header(&unpacker, &packer, wem, ginput->stripped_headers);
			}
		}
		if (err) {
			oggpack_writeclear(&packer);
			return err;
		} else if ((err = i_build_packet(&packet, &packer, current_header, 0, 0))) {
			oggpack_writeclear(&packer);
			return err;
		} else if ((err = i_insert_packet(streamer, &packet, vi, vc))) {
			oggpack_writeclear(&packer);
			return err;
		}
		oggpack_writeclear(&packer);
	}
	return I_SUCCESS;
}

/* PROCESS */
static int BRRCALL
i_process_headers(ogg_stream_state *const streamer, wwise_wemT *const wem,
	vorbis_info *const vi, vorbis_comment *const vc)
{
	if (wem->all_headers_present) {
		return i_copy_headers(streamer, wem, vi, vc);
	} else {
		return i_build_headers(streamer, wem, vi, vc);
	}
}
static int BRRCALL
i_process_audio(ogg_stream_state *const streamer, wwise_wemT *const wem,
	vorbis_info *const vi, vorbis_comment *const vc)
{
	int err = 0;
	brru4 packets_start = wem->vorb.audio_start_offset,
	      packets_size = wem->data_size - wem->vorb.audio_start_offset;
	wwise_packetT packeteer = {0};
	int prev_blockflag = 0,
	    mode_count_bits = lib_count_bits(wem->mode_count - 1);
	brru8 packetno = 0, last_block = 0, total_block = 0;
	for (; packets_start < wem->data_size; ++packetno) {
		int eos = 0;
		ogg_packet packet;
		oggpack_buffer unpacker, packer;
		if (WWISE_SUCCESS != wwise_packet_init(&packeteer, wem, wem->data + packets_start, packets_size))
			return I_INSUFFICIENT_DATA;
		oggpack_readinit(&unpacker, packeteer.payload, packeteer.payload_size);
		oggpack_writeinit(&packer);

		if (wem->mod_packets) {
			int packet_type = 0;
			int mode_number, remainder;
			packer_pack(&packer, packet_type, 1); /* OUT Packet type */
			mode_number = packer_transfer(&unpacker, mode_count_bits, &packer, mode_count_bits); /* IN/OUT Mode number */
			remainder = packer_unpack(&unpacker, 8 - mode_count_bits); /* IN Remainder bits */

			if (wem->mode_blockflags[mode_number]) {
				/* Long window */
				wwise_packetT next_packeteer;
				brru4 next_start = packets_start + packeteer.header_length + packeteer.payload_size,
				      next_size = packets_size + packeteer.header_length + packeteer.payload_size;
				int next_blockflag = 0;
				if (WWISE_SUCCESS != wwise_packet_init(&next_packeteer, wem, wem->data + next_start, next_size)) {
					eos = 1;
				} else if (next_packeteer.payload_size) {
					int next_number;
					oggpack_buffer next_unpacker;
					oggpack_readinit(&next_unpacker, next_packeteer.payload, next_packeteer.payload_size);
					next_number = packer_unpack(&next_unpacker, mode_count_bits); /* IN Next number */
					next_blockflag = wem->mode_blockflags[next_number];
				}
				packer_pack(&packer, prev_blockflag, 1); /* OUT Previous window type */
				packer_pack(&packer, next_blockflag, 1); /* OUT Next window type */
			}

			packer_pack(&packer, remainder, 8 - mode_count_bits); /* OUT Remainder of read-in first byte */
			prev_blockflag = wem->mode_blockflags[mode_number];
		} else {
			int transferred = packer_transfer(&unpacker, 8, &packer, 8); /* Unmodified first byte */
		}
		packer_transfer_remaining(&unpacker, &packer);
		if ((err = i_build_packet(&packet, &packer, packetno + 3, 0, eos))) {
			oggpack_writeclear(&packer);
			return err;
		} else {
			/* This granule calculation from revorb, not sure its source though
			 * Probably somewhere in vorbis docs, haven't found it */
			long current_block = vorbis_packet_blocksize(vi, &packet);
			if (last_block)
				total_block += (last_block + current_block) / 4;
			last_block = current_block;
			packet.granulepos = total_block;
			if ((err = i_insert_packet(streamer, &packet, NULL, NULL))) {
				oggpack_writeclear(&packer);
				return err;
			}
		}
		oggpack_writeclear(&packer);
		packets_start += packeteer.header_length + packeteer.payload_size;
		packets_size -= packeteer.header_length + packeteer.payload_size;
	}
#if defined(NeEXTRA_DEBUG)
	BRRLOG_DEBUG("Packetno : %lld", 3 + packetno);
#endif
	return I_SUCCESS;
}
int
wwise_convert_wwriff(riffT *const rf, ogg_stream_state *const streamer,
    const codebook_libraryT *const library, const neinputT *const input)
{
	int err = 0;
	wwise_wemT wem;
	vorbis_info vi;
	vorbis_comment vc;

	if (WWISE_SUCCESS != (err = wwise_wem_init(&wem, rf))) {
#if defined(NeEXTRA_DEBUG)
		if (err == WWISE_INCOMPLETE) {
			BRRLOG_ERRN("WEM missing");
			if (!wem.fmt_initialized)
				BRRLOG_ERRNP(" 'fmt'");
			if (!wem.vorb_initialized) {
				if (!wem.fmt_initialized)
					BRRLOG_ERRNP(",");
				BRRLOG_ERRNP(" 'vorb'");
			}
			if (!wem.data_initialized) {
				if (!wem.fmt_initialized || !wem.vorb_initialized)
					BRRLOG_ERRNP(",");
				BRRLOG_ERRNP(" 'data'");
			}
			BRRLOG_ERRNP(" chunks");
		} else if (err == WWISE_DUPLICATE) {
			BRRLOG_ERRN("WEM has duplicate 'fmt', 'data', or 'vorb' chunk(s)");
		} else
#endif
			if (err == WWISE_CORRUPT) {
#if defined(NeEXTRA_DEBUG)
			BRRLOG_ERRN("WEM is corrupted or does not contain vorbis data");
#endif
			return I_CORRUPT;
		}
		return I_INIT_ERROR;
	}
	glibrary = library;
	ginput = input;
	if (!(err = i_init_state(streamer, &wem, &vi, &vc))) {
		if (!(err = i_process_headers(streamer, &wem, &vi, &vc))) {
			err = i_process_audio(streamer, &wem, &vi, &vc);
			vorbis_info_clear(&vi);
			vorbis_comment_clear(&vc);
		}
		if (err)
			ogg_stream_clear(streamer);
	}
	return err;
}
