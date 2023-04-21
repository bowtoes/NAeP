/* Copyright (c), bowtoes (bow.toes@mailfence.com)
Apache 2.0 license, http://www.apache.org/licenses/LICENSE-2.0
Full license can be found in 'license' file */

#include "wwise.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <vorbis/vorbisenc.h>
#include <brrtools/brrnum.h>

#include "neinput.h"
#include "neutil.h"
#include "nelog.h"
#include "riff.h"

#include "_wwise_internal.h"

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
			if (w.fmt_initialized) {
				Err(,"WwRIFF has multiple 'fmt ' chunks");
				return E_GENERIC;
			}

			i_init_fmt(&w.fmt, basic.data, basic.size);
			w.fmt_initialized = 1;

			/* Vorb init header data is contained in the fmt */
			if (basic.size == 66) {
				XDeb(,"'vorb' chunk is implicit");
				if (w.vorb_initialized) {
					Err(,"WwRIFF has both explicit and implicit 'vorb' chunks.");
					return E_GENERIC;
				}
				i_init_vorb(&w, basic.data + 24, basic.size - 24);
				w.vorb_initialized = 1;
			}

		} else if (basic.type == riff_basic_vorb) {
			/* Vorb init header data is explicit */
			if (w.vorb_initialized) {
				Err(,"WwRIFF has multiple 'vorb' chunks.");
				return E_GENERIC;
			}
			i_init_vorb(&w, basic.data, basic.size);
			w.vorb_initialized = 1;

		} else if (basic.type == riff_basic_data) {
			if (w.data_initialized) {
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
			w.data_initialized = 1;
		}
	}

	if (!w.fmt_initialized || !w.vorb_initialized || !w.data_initialized) {
		if (!w.fmt_initialized)
			Err(,"WwRIFF is missing 'fmt ' chunk");
		if (!w.vorb_initialized)
			Err(,"WwRIFF is missing 'vorb' chunk");
		if (!w.data_initialized)
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
				brrstr_free(&wem->comments[i]);
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

	va_list lptr;
	va_start(lptr, format);
	int size = vsnprintf(NULL, 0, format, lptr);
	va_end(lptr);
	if (size < 0) {
		Err(,"Couldn't determine size of wwriff comment string, format was '%s'", format);
		return -1;
	}
	if (size > COMMENT_MAX - 1)
		size = COMMENT_MAX - 1;

	brrstr_t string = {0};
	if (!(string.s = malloc(size + 1))) {
		Err(,"Couldn't allocate %s bytes for wwriff comment string, format was '%s'", size + 1, format);
		return -1;
	}
	va_start(lptr, format);
	vsnprintf(string.s, size + 1, format, lptr);
	va_end(lptr);
	string.l = size;

	brrstr_t *new = realloc(wem->comments, sizeof(*new) * (wem->n_comments + 1));
	if (!new) {
		Err(,"Failed to allocate space for vorbis comment #%zu: %s (%d)", wem->n_comments, strerror(errno), errno);
		brrstr_free(&string);
		return -1;
	}
	new[wem->n_comments++] = string;
	wem->comments = new;

	return 0;
}

/* PROCESS */
static inline int
i_process_headers(ogg_stream_state *const streamer, wwriff_t *const wem, vorbis_info *const vi, vorbis_comment *const vc)
{
	return wem->all_headers_present ? i_copy_headers(streamer, wem, vi, vc) : i_build_headers(streamer, wem, vi, vc);
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
	XDeb(,"Stream mod packets");
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

		if (wem->mod_packets) {
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
		XDeb(,"Transferrined %lld remaining bytes", a);

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
		XDeb(,"Block %lld | Total Block %lld", current_block, total_block);

		if ((err = i_insert_packet(streamer, &packet))) {
			oggpack_writeclear(&packer);
			return err;
		}

		oggpack_writeclear(&packer);
		packets_start += packeteer.total_size;
		packets_size  -= packeteer.total_size;
		++packetno;
	}
	XDeb(,"Total packets: %lld", 3 + packetno);
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
