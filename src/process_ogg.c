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

#include "process_files.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include <brrtools/brrlog.h>
#include <brrtools/brrpath.h>

#define SYNC_BUFFER_SIZE 4096
typedef enum ogg_sync_status {
	sync_wrote_success = 0,
	sync_pageout_success = 1,
	sync_pageout_desync = -1,
	sync_pageout_incomplete = 0,
} ogg_sync_statusT;
typedef enum ogg_stream_status {
	stream_init_success = 0,
	stream_pagein_success = 0,
	stream_packetin_success = 0,
	stream_packetout_desync = -1,
	stream_packetout_incomplete = 0,
	stream_packetout_success = 1,
} ogg_stream_statusT;
typedef enum ogg_get_page_error {
	ogg_get_page_success = sync_pageout_success,
	ogg_get_page_buffer_error = -1,
	ogg_get_page_io_error = -2,
	ogg_get_page_file_truncated_error = -3,
} ogg_get_page_errorT;
static int BRRCALL
ogg_get_next_page(FILE *const file, ogg_page *const page, ogg_sync_state *current_sync)
{
	int err = sync_pageout_success;
	brrsz bytes_read = 0;
	while (sync_pageout_success != (err = ogg_sync_pageout(current_sync, page)) && !feof(file)) {
		char *sync_buffer = NULL;
		if (!(sync_buffer = ogg_sync_buffer(current_sync, SYNC_BUFFER_SIZE))) {
			return ogg_get_page_buffer_error;
		}
		bytes_read = fread(sync_buffer, 1, SYNC_BUFFER_SIZE, file);
		if (ferror(file)) {
			return ogg_get_page_io_error;
		} else if (sync_wrote_success != ogg_sync_wrote(current_sync, bytes_read)) {
			return ogg_get_page_buffer_error;
		}
	}
	if (feof(file) && bytes_read != 0 && sync_pageout_success != err) {
		return ogg_get_page_file_truncated_error;
	}
	return err;
}
static int BRRCALL
int_regranularize(const char *const input, const char *const output)
{
	/* I know this function is hideous and all, but I'm not sure how to break it up nicely.
	 * TODO figure out how to break this thing up nicely. */
	int err = 0;
	FILE *in = NULL;
	FILE *out = NULL;
	if (!(in = fopen(input, "rb"))) {
		BRRLOG_ERRN("Failed to open input ogg for regranularization '%s' : %s", input, strerror(errno));
		err = -1;
	} else {
		ogg_sync_state input_sync;
		ogg_page page;
		ogg_sync_init(&input_sync);
		err = ogg_get_next_page(in, &page, &input_sync);
		if (ogg_get_page_success != err) {
			if (err == ogg_get_page_buffer_error)
				BRRLOG_ERRN("Error starting sync of ogg page buffer for '%s' : %s", input, strerror(errno));
			else if (err == ogg_get_page_io_error)
				BRRLOG_ERRN("Failed to read first ogg page from input '%s' : %s", input, strerror(errno));
			else if (err == ogg_get_page_file_truncated_error)
				BRRLOG_ERRN("Ogg page truncated '%s'", input);
			err = -1;
		} else {
			ogg_stream_state input_stream;
			ogg_stream_state output_stream;
			int stream_serial = ogg_page_serialno(&page);
			err = 0;
			if (ogg_stream_init(&input_stream, stream_serial)) {
				BRRLOG_ERRN("Could not initialize input ogg stream for '%s' : %s", input, strerror(errno));
				err = -1;
			} else if (ogg_stream_init(&output_stream, stream_serial)) {
				BRRLOG_ERRN("Could not initialize output ogg stream for '%s' : %s", input, strerror(errno));
				err = -1;
			} else if (ogg_stream_pagein(&input_stream, &page)) {
				BRRLOG_ERRN("Failed to read first page into input stream for '%s' : %s", input, strerror(errno));
				err = -1;
			} else {
				ogg_packet packet;
				/* TODO check packet errors */
				if (1 != (err = ogg_stream_packetout(&input_stream, &packet))) {
					BRRLOG_ERRN("Failed retrieving packet from input stream '%s' : %s", input, strerror(errno));
					err = -1;
				} else {
					vorbis_info vorb_info;
					vorbis_comment vorb_comment;
					vorbis_info_init(&vorb_info);
					vorbis_comment_init(&vorb_comment);
					if ((err = vorbis_synthesis_headerin(&vorb_info, &vorb_comment, &packet))) {
						if (err == OV_EFAULT)
							BRRLOG_ERRN("Failed to synthesize vorbis header info for input '%s' : %s", input, strerror(errno));
						else if (err == OV_ENOTVORBIS)
							BRRLOG_ERRN("Input ogg is not vorbis '%s'", input);
						else if (err == OV_EBADHEADER)
							BRRLOG_ERRN("Input ogg has bad vorbis header '%s'", input);
						err = -1;
					} else if (stream_packetin_success != ogg_stream_packetin(&output_stream, &packet)) {
						BRRLOG_ERRN("Failed to write first header packet to output stream '%s' : %s", output, strerror(errno));
						err = -1;
					} else {
						/* TODO What's the nicest way to avoid this endless
						 * if-else nesting? */
						int completed_headers = 0;
						while (!err && completed_headers < 2) {
							err = ogg_get_next_page(in, &page, &input_sync);
							if (ogg_get_page_success != err) {
								if (err == ogg_get_page_buffer_error)
									BRRLOG_ERRN("Error syncing ogg page buffer for '%s' : %s", input, strerror(errno));
								else if (err == ogg_get_page_io_error)
									BRRLOG_ERRN("Failed to read ogg page from input '%s' : %s", input, strerror(errno));
								else if (err == ogg_get_page_file_truncated_error)
									BRRLOG_ERRN("Ogg page truncated '%s'", input);
								err = -1;
							} else if (ogg_stream_pagein(&input_stream, &page)) {
								BRRLOG_ERRN("Failed to read page %zu into input stream for '%s' : %s",
								    ogg_page_pageno(&page), input, strerror(errno));
								err = -1;
							} else {
								err = 0;
								while (!err && completed_headers < 2) {
									if (stream_packetout_incomplete == (err = ogg_stream_packetout(&input_stream, &packet))) {
										break;
									} else if (stream_packetout_desync == err) {
										BRRLOG_ERRN("Corrupted header in ogg '%s'");
										err = -1;
									} else {
										packet.granulepos = 0;
										if ((err = vorbis_synthesis_headerin(&vorb_info, &vorb_comment, &packet))) {
											if (err == OV_EFAULT)
												BRRLOG_ERRN("Failed to synthesize vorbis header info for input '%s' : %s", input, strerror(errno));
											else if (err == OV_ENOTVORBIS)
												BRRLOG_ERRN("Input ogg is not vorbis '%s'", input);
											else if (err == OV_EBADHEADER)
												BRRLOG_ERRN("Input ogg has bad vorbis header '%s'", input);
											err = -1;
										} else if ((err = ogg_stream_packetin(&output_stream, &packet))) {
											BRRLOG_ERRN("Failed to write header packet to output stream '%s' : %s", output, strerror(errno));
											err = -1;
										} else {
											completed_headers++;
										}
									}
								}
							}
						}
						if (!err) {
							int stream_end = 0;
							int last_block = 0;
							brru8 total_granule = 0;
							brru8 total_packets = 0;
							while (!stream_end) {
								err = ogg_get_next_page(in, &page, &input_sync);
								if (err != ogg_get_page_success) {
									if (err == ogg_get_page_buffer_error)
										BRRLOG_ERRN("Error syncing ogg page buffer for '%s' : %s", input, strerror(errno));
									else if (err == ogg_get_page_io_error)
										BRRLOG_ERRN("Failed to read ogg page from input '%s' : %s", input, strerror(errno));
									else if (err == ogg_get_page_file_truncated_error)
										BRRLOG_ERRN("Ogg page truncated '%s'", input);
									err = -1;
									break;
								} else if (stream_pagein_success != ogg_stream_pagein(&input_stream, &page)) {
									BRRLOG_ERRN("Error reading page into input stream '%s' : %s", input ,strerror(errno));
									err = -1;
									break;
								} else if (ogg_page_eos(&page)) {
									stream_end = 1;
								}
								err = 0;
								while (!err) {
									int blocksize = 0;
									err = ogg_stream_packetout(&input_stream, &packet);
									if (stream_packetout_incomplete == err) {
										err = 0;
										break;
									} else if (stream_packetout_desync == err) {
										BRRLOG_WAR("Bitstream desync at input packet %lld", packet.packetno);
									} else {
										err = 0;
										/* How is this done? */
										blocksize = vorbis_packet_blocksize(&vorb_info, &packet);
										if (last_block)
											total_granule += (last_block + blocksize) / 4;
										last_block = blocksize;
										packet.granulepos = total_granule;
										packet.packetno = total_packets++;
										if (stream_packetin_success != ogg_stream_packetin(&output_stream, &packet)) {
											BRRLOG_ERROR("");
											err = -1;
										}
									}
								}
							}
						}
					}
					vorbis_comment_clear(&vorb_comment);
					vorbis_info_clear(&vorb_info);
				}
			}
			fclose(in);
			if (!err) {
				if (!(out = fopen(output, "wb"))) {
					BRRLOG_ERRN("Failed to open regranularization output '%s' : %s", input, strerror(errno));
					err = -1;
				} else {
					while (!err && (ogg_stream_pageout(&output_stream, &page) || ogg_stream_flush(&output_stream, &page))) {
						if (page.header_len != fwrite(page.header, 1, page.header_len, out)) {
							BRRLOG_ERRN("Failed writing page %zu header to output '%s' : %s", ogg_page_pageno(&page), output, strerror(errno));
							err = -1;
						} else if (page.body_len != fwrite(page.body, 1, page.body_len, out)) {
							BRRLOG_ERRN("Failed writing page %zu body to output '%s' : %s", ogg_page_pageno(&page), output, strerror(errno));
							err = -1;
						}
					}
					fclose(out);
				}
			}
			ogg_stream_clear(&output_stream);
			ogg_stream_clear(&input_stream);
		}
		ogg_sync_clear(&input_sync);
	}
	return err;
}
int BRRCALL
regranularize_ogg(numbersT *const numbers, int dry_run, const char *const path,
    int inplace_regranularize)
{
	static char output[BRRPATH_MAX_PATH + 1] = {0};
	int err = 0;
	numbers->oggs_to_regranularize++;
	if (dry_run) {
		BRRLOG_FOREP(DRY_COLOR, " Regranularize OGG");
	} else {
		brrsz inlen = 0, outlen = 0;
		BRRLOG_FORENP(WET_COLOR, " Regranularizing OGG... ");
		replace_ext(path, &inlen, output, &outlen, "_rvb.ogg");
		err = int_regranularize(path, output);
		if (!err) {
			if (inplace_regranularize) {
				NeTODO("ANTIGRAIN IN-PLACE");
				/* remove 'path' and rename 'output' to 'path' */
			}
		}
	}
	if (!err) {
		numbers->oggs_regranularized++;
		BRRLOG_MESSAGETP(gbrrlog_level_last, SUCCESS_FORMAT, " Success!");
	} else {
		/* remove 'output' */
		NeTODO("ANTIGRAIN ERROR REMOVE OUTPUT");
		BRRLOG_MESSAGETP(gbrrlog_level_last, FAILURE_FORMAT, " Failure! (%d)", err);
	}
	return err;
}
