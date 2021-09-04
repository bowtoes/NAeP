#ifndef WWISE_H
#define WWISE_H

#include <brrtools/brrapi.h>
#include <brrtools/brrtypes.h>

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
typedef union wwise_vorb {
	brru4 sample_count;
	wwise_vorb_implicitT implicit;
	wwise_vorb_basicT basic;
	wwise_vorb_extraT extra;
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
	union {
		struct {
			brru4 data1;
			brru2 data2;
			brru2 data3;
			brru1 data4[8];
		} guid;
		wwise_vorbT vorb;
	};
} wwise_fmtT;

typedef enum wwise_vorb_type {
	wwise_vorb_type_basic = 0,
	wwise_vorb_type_extra,
	wwise_vorb_type_implicit,
} wwise_vorb_typeT;
typedef struct wwise_wem {
	wwise_fmtT fmt;
	wwise_vorb_typeT vorb_type;
	wwise_vorbT vorb;
	unsigned char *data;
} wwise_wemT;

#define CODEBOOK_SUCCESS 0
#define CODEBOOK_ERROR -1
#define CODEBOOK_CORRUPT -2

typedef struct packed_codebook {
	unsigned char *codebook_data;
	brru4 codebook_size;
} packed_codebookT;
typedef struct codebook_library {
	packed_codebookT *codebooks;
	brru4 codebook_count;
} codebook_libraryT;

/* -2 : corruption
 * -1 : error (allocation/argument)
 *  0 : success
 * */
int BRRCALL codebook_library_deserialize_deprecated(codebook_libraryT *const cb,
    const void *const data, brru8 data_size);
/* -2 : corruption
 * -1 : error (allocation/argument)
 *  0 : success
 * */
int BRRCALL codebook_library_deserialize(codebook_libraryT *const cb,
    const void *const data, brru8 data_size);
/* -1 : error (allocation/argument)
 *  0 : success
 * */
int BRRCALL codebook_library_serialize_deprecated(const codebook_libraryT *const cb,
    void **const data, brru8 *const data_size);
/* -1 : error (allocation/argument)
 *  0 : success
 * */
int BRRCALL codebook_library_serialize(const codebook_libraryT *const cb,
    void **const data, brru8 *const data_size);

void BRRCALL codebook_library_clear(codebook_libraryT *const cb);

BRRCPPEND

#endif /* WWISE_H */
