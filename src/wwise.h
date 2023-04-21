/* Copyright (c), bowtoes (bow.toes@mailfence.com)
Apache 2.0 license, http://www.apache.org/licenses/LICENSE-2.0
Full license can be found in 'license' file */

#ifndef NAeP_newwise_h
#define NAeP_newwise_h

#include <ogg/ogg.h>

#include <brrtools/brrstr.h>

typedef enum vorbishdr {
	vorbishdr_id = 0,
	vorbishdr_comment = 1,
	vorbishdr_setup = 2,
	vorbishdr_count = 3,
} vorbishdr_t;

extern const char *const vorbishdr_names[3];
#define vorbishdr(_index_) (vorbishdr_names[_index_])

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

#define wwise_flags union {\
	brru1 _;\
	struct {\
		brru1 fmt_initialized:1;\
		brru1 vorb_initialized:1;\
		brru1 data_initialized:1;\
		brru1 mod_packets:1;         /* No idea what this means */\
		brru1 granule_present:1;     /* Whether data packets have 4-bytes for granule */\
		brru1 all_headers_present:1; /* If all vorbis headers are present at header_packets_offset or it's just the setup header */\
	};\
}

typedef wwise_flags wwise_flags_t;

typedef struct wwriff {
	wwise_vorb_t vorb;
	wwise_fmt_t fmt;
	unsigned char *data;
	brrstr_t *comments;
	brru4 data_size;
	brru4 n_comments;
	int mode_count;              /* Storage for audio packet decode */
	brru1 mode_blockflags[32];   /* Storage for audio packet decode */
	union {
		wwise_flags;
		wwise_flags_t flags;
	};
} wwriff_t;

typedef struct riff riff_t;
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

typedef struct neinput neinput_t;
typedef struct codebook_library codebook_library_t;
/* Converts the wwriff data 'in_riff' to an ogg stream in 'out_stream', using codebooks from 'library'.
 * 'input' is for output stream metadata (like which file the output is converted from, etc.).
 * */
int wwise_convert_wwriff(
    wwriff_t *const in_wwriff,
    ogg_stream_state *const out_stream,
    const codebook_library_t *const library,
    const neinput_t *const input
);

#endif /* NAeP_newwise_h */
