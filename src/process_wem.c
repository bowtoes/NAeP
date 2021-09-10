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
#include <string.h>

#include <ogg/ogg.h>

#include "common_lib.h"
#include "errors.h"
#include "wwise_riff.h"

static void BRRCALL
i_print_wem(const wwise_wemT *const wem)
{
	if (!wem)
		return;
	BRRLOG_NOR("fmt data:");
	BRRLOG_NOR("fmt>     format_tag : 0x%04X", wem->fmt.format_tag);
	BRRLOG_NOR("fmt>     n_channels : %u", wem->fmt.n_channels);
	BRRLOG_NOR("fmt>samples_per_sec : %lu", wem->fmt.samples_per_sec);
	BRRLOG_NOR("fmt>  avg_byte_rate : %lu", wem->fmt.avg_byte_rate);
	BRRLOG_NOR("fmt>    block_align : %u", wem->fmt.block_align);
	BRRLOG_NOR("fmt>bits_per_sample : %u", wem->fmt.bits_per_sample);
	BRRLOG_NOR("fmt>     extra_size : %u", wem->fmt.extra_size);
	if (wem->fmt.extra_size) {
		BRRLOG_NOR("fmt>extra>    reserved : %u", wem->fmt.reserved);
		BRRLOG_NOR("fmt>extra>channel_mask : %lu", wem->fmt.channel_mask);
		if (wem->fmt.extra_size == 22) {
			BRRLOG_NOR("fmt>extra>        guid>data1 : %lu", wem->fmt.guid.data1);
			BRRLOG_NOR("fmt>extra>        guid>data2 : %u", wem->fmt.guid.data2);
			BRRLOG_NOR("fmt>extra>        guid>data3 : %u", wem->fmt.guid.data3);
			BRRLOG_NOR("fmt>extra>        guid>data4 : 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
			    wem->fmt.guid.data4[0], wem->fmt.guid.data4[1], wem->fmt.guid.data4[2], wem->fmt.guid.data4[3],
			    wem->fmt.guid.data4[4], wem->fmt.guid.data4[5], wem->fmt.guid.data4[6], wem->fmt.guid.data4[7]);
		} else if (wem->fmt.extra_size > 22) {
			BRRLOG_NOR("fmt>extra == vorb");
		}
	}
	BRRLOG_NOR("vorb data:");
	BRRLOG_NOR("vorb>         sample_count : %lu", wem->vorb.sample_count);
	BRRLOG_NOR("vorb>           mod_signal : %lu 0x%08X", wem->vorb.mod_signal, wem->vorb.mod_signal);
	BRRLOG_NOR("vorb>header_packets_offset : 0x%08X", wem->vorb.header_packets_offset);
	BRRLOG_NOR("vorb>   audio_start_offset : 0x%08X", wem->vorb.audio_start_offset);
	BRRLOG_NOR("vorb>                  uid : 0x%08X", wem->vorb.uid);
	BRRLOG_NOR("vorb>          blocksize_0 : %u", wem->vorb.blocksize[0]);
	BRRLOG_NOR("vorb>          blocksize_1 : %u", wem->vorb.blocksize[1]);
}

/* Output writing */
static int BRRCALL
int_convert_wem(const char *const input, const char *const output,
    const input_optionsT *const options, input_libraryT *const library)
{
	int err = 0;
	FILE *in;
	riffT rf;
	ogg_stream_state streamer;
	codebook_libraryT *cbl = NULL;

	if (library) {
		if ((err = input_library_load(library))) {
			BRRLOG_ERRN("Failed to load codebook library '%s' : %s",
			    (char *)library->library_path.opaque, lib_strerr(err));
			return err;
		}
		cbl = &library->library;
	}
	if (!(in = fopen(input, "rb"))) {
		BRRLOG_ERRN("Failed to open wem for conversion input : %s", strerror(errno));
		return I_IO_ERROR;
	}

	riff_init(&rf);
	if (I_SUCCESS != (err = lib_read_riff_chunks(in, &rf))) {
		BRRLOG_ERRN("Failed to consume RIFF chunk : %s", lib_strerr(err));
		riff_clear(&rf);
		fclose(in);
		return err;
	}
	fclose(in);
	if (rf.format != riff_format_WAVE) {
		BRRLOG_ERRN("RIFF is not WAVE");
		riff_clear(&rf);
		return I_UNRECOGNIZED_DATA;
	}
	if (!(err = wwise_riff_process(&streamer, &rf, options, cbl)))
		err = lib_write_ogg_out(&streamer, output);

	riff_clear(&rf);
	return err;
}

int BRRCALL
convert_wem(numbersT *const numbers, const char *const input, brrsz input_length,
    const input_optionsT *const options, input_libraryT *const library)
{
	static char output[BRRPATH_MAX_PATH + 1] = {0};
	int err = 0;
	numbers->wems_to_convert++;
	if (options->dry_run) {
		BRRLOG_FORENP(LOG_COLOR_DRY, "Convert WEM (dry) ");
	} else {
		BRRLOG_FORENP(LOG_COLOR_WET, "Converting WEM... ");
		if (options->inplace_ogg) {
			snprintf(output, sizeof(output), "%s", input);
		} else {
			lib_replace_ext(input, input_length, output, NULL, ".ogg");
		}
		err = int_convert_wem(input, output, options, library);
	}
	if (!err) {
		numbers->wems_converted++;
		BRRLOG_MESSAGETP(gbrrlog_level_normal, LOG_FORMAT_SUCCESS, "Success!");
	} else {
		numbers->wems_failed++;
		BRRLOG_MESSAGETP(gbrrlog_level_normal, LOG_FORMAT_FAILURE, " Failure! (%d)", err);
	}
	return err;
}
