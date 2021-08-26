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
#include <stdlib.h>
#include <string.h>

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrpath.h>
#include <brrtools/brrplatform.h>
#include <brrtools/brrmem.h>

#include "riff.h"

#define RIFF_BUFFER_APPLY_SUCCESS 0
#define RIFF_BUFFER_APPLY_FAILURE -1

#define RIFF_BUFFER_INCREMENT 4096

static int BRRCALL
i_consume_next_chunk(FILE *const file, riffT *const rf, riff_chunkinfoT *const fo)
{
	int err = 0;
	brrsz bytes_read = 0;
	char *buffer = NULL;
	while (RIFF_CHUNK_CONSUMED != (err = riff_consume_chunk(rf, fo)) && !feof(file)) {
		if (err == RIFF_CONSUME_MORE) {
			continue;
		} else if (err == RIFF_CHUNK_UNRECOGNIZED) {
			BRRLOG_WAR("Skipping unrecognized chunk '%s' (%zu)", FCC_INT_CODE(fo->chunkcc), fo->chunksize);
			fseek(file, fo->chunksize, SEEK_CUR);
		} else if (err != RIFF_CHUNK_INCOMPLETE) {
			if (err == RIFF_ERROR)
				return I_BUFFER_ERROR;
			else if (err == RIFF_NOT_RIFF)
				return I_NOT_RIFF;
			else if (err == RIFF_CORRUPTED)
				return I_CORRUPT;
			else
				return I_BAD_ERROR - err;
		}
		if (!(buffer = riff_buffer(rf, RIFF_BUFFER_INCREMENT))) {
			return I_BUFFER_ERROR;
		}
		bytes_read = fread(buffer, 1, RIFF_BUFFER_INCREMENT, file);
		if (ferror(file)) {
			return I_IO_ERROR;
		} else if (RIFF_BUFFER_APPLY_SUCCESS != riff_apply_buffer(rf, bytes_read)) {
			return I_BUFFER_ERROR;
		}
	}
	return I_SUCCESS;
}
static int BRRCALL
int_convert_wem(const char *const input, const char *const output)
{
	int err = 0;
	FILE *in, *out;
	riff_chunkinfoT sync_info = {0};
	riffT rf;

	if (!(in = fopen(input, "rb"))) {
		BRRLOG_ERRN("Failed to open wem for conversion input '%s' : %s", input, strerror(errno));
		return I_IO_ERROR;
	}

	riff_init(&rf);
	while (I_SUCCESS == (err = i_consume_next_chunk(in, &rf, &sync_info))) {
		if (sync_info.is_basic) {
			riff_basic_chunkinfoT *basic = &rf.basics[sync_info.chunkinfo_index];
			BRRLOG_NOR("Got basic chunk : (%zu) '%s'", basic->type, FCC_INT_CODE(riff_basictypes[basic->type - 1]));
			BRRLOG_NOR("    Size : %zu", basic->size);
		} else if (sync_info.is_list) {
			riff_list_chunkinfoT *list = &rf.lists[sync_info.chunkinfo_index];
			BRRLOG_NOR("Got list chunk : (%zu) '%s'", list->type, FCC_INT_CODE(riff_listtypes[list->type - 1]));
			BRRLOG_NOR("    Type : '%s'", FCC_INT_CODE(riff_subtypes[list->subtype - 1]));
			BRRLOG_NOR("    Size : %zu", list->size);
			/* etc ... */
		} else {
			BRRLOG_NOR("WAH");
		}
		riff_chunkinfo_clear(&sync_info);
	}
	if (err != I_SUCCESS) {
		BRRLOG_ERRN("Failed to consume RIFF chunk from '%s' : %s", input, i_strerr(err));
	}
	riff_clear(&rf);
	fclose(in);

	return I_SUCCESS;
}

int BRRCALL
convert_wem(numbersT *const numbers, int dry_run, const char *const path,
    int inplace_regranularize, int inplace_ogg)
{
	static char output[BRRPATH_MAX_PATH + 1] = {0};
	int err = 0;
	numbers->wems_to_convert++;
	if (dry_run) {
		BRRLOG_FORENP(DRY_COLOR, " Convert WEM");
	} else {
		brrsz outlen = 0, inlen = 0;
		BRRLOG_FOREP(WET_COLOR, " Converting WEM... ");
		replace_ext(path, &inlen, output, &outlen, ".txt");
		err = int_convert_wem(path, output);
		if (!err) {
			if (inplace_ogg) {
				NeTODO("WEM CONVERT IN-PLACE");
				/* remove 'path' and rename 'output' to 'path' */
			}
		}
	}
	if (!err) {
		numbers->wems_converted++;
		BRRLOG_MESSAGETP(gbrrlog_level_last, SUCCESS_FORMAT, " Success!");
	} else {
		/* remove 'output' */
		BRRLOG_MESSAGETP(gbrrlog_level_last, FAILURE_FORMAT, " Failure! (%d)", err);
	}
	return err;
}
