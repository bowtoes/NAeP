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

#include "wspbnk.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrapi.h>
#include <brrtools/brrlib.h>
#include <brrtools/brrpath.h>

#include "codebook_library.h"
#include "common_lib.h"
#include "errors.h"
#include "riff.h"
#include "wwise_riff.h"

static const input_optionsT *goptions = NULL;
static char goutput_format[BRRPATH_MAX_PATH + 1] = {0};

void BRRCALL
wsp_clear(wspT *const wsp)
{
	if (wsp->wems) {
		free(wsp->wems);
	}
	memset(wsp, 0, sizeof(*wsp));
}

static int BRRCALL
i_count_wems(FILE *const file, wspT *const wsp)
{
	fourccT cc = {0};
	wem_geomT current = {0};
	/* TODO something is horribly wrong in this; if I don't explicitly add the
	 * size AND 8 to the offset the seeking falls behind by 8 each time.
	 * It happens whether the 8 is added to size first or not at all.
	 * */
	while (4 == fread(&cc, 1, 4, file)) {
		if (!(current.byteorder = riff_cc_byteorder(cc.integer))) {
			if (fseek(file, -3, SEEK_CUR)) {
				wsp_clear(wsp);
				return I_IO_ERROR;
			}
			current.offset++;
		} else { /* Found RIFF */
			if (4 != fread(&current.size, 1, 4, file)) {
				if (feof(file)) { /* End of file, no error (maybe?) */
					break;
				} else {
					wsp_clear(wsp);
					return I_IO_ERROR;
				}
			} else if (brrlib_alloc((void **)&wsp->wems, (wsp->wem_count + 1) * sizeof(*wsp->wems), 0)) {
				wsp_clear(wsp);
				return I_BUFFER_ERROR;
			} else if (current.byteorder == riff_byteorder_RIFX || current.byteorder == riff_byteorder_FFIR) {
				brru4 tmp;
				riff_copier_data(current.byteorder)(&tmp, &current.size, 4);
				current.size = tmp;
			}
			current.size += 8;
			wsp->wems[wsp->wem_count++] = current;
			current.offset += current.size + 8; /* I'll be honest; I have no idea */
			current.byteorder = 0;
			if (fseek(file, current.size, SEEK_CUR)) {
				wsp_clear(wsp);
				return I_IO_ERROR;
			}
		}
	}
	if (ferror(file)) {
		wsp_clear(wsp);
		return I_IO_ERROR;
	}
	rewind(file);
	return 0;
}
static int BRRCALL
i_ogg_wem(const char *const output_name, codebook_libraryT *const library,
    unsigned char *const buffer, brrsz buffer_size)
{
	int err = 0;
	riffT rf = {0};
	ogg_stream_state streamer;
	if ((err = lib_read_riff_from_buffer(&rf, buffer, buffer_size))) {
		return err;
	} else if ((err = wwise_riff_process(&streamer, &rf, goptions, library))) {
		riff_clear(&rf);
		return err;
	} else {
		err = lib_write_ogg_out(&streamer, output_name);
	}
	riff_clear(&rf);
	ogg_stream_clear(&streamer);
	return err;
}
static int BRRCALL
i_write_wem(const char *const output_name, unsigned char *const buffer, brrsz buffer_size)
{
	FILE *out = NULL;
	if (!(out = fopen(output_name, "wb"))) {
		return I_IO_ERROR;
	}
	fwrite(buffer, 1, buffer_size, out);
	fclose(out);
	return I_SUCCESS;
}
static int BRRCALL
i_extract_wems(FILE *const file, wspT *const wsp, codebook_libraryT *const library,
    numbersT *const numbers)
{
	static char output_name[BRRPATH_MAX_PATH + 1] = {0};
	int err = 0;
	brru4 maxsize = 0;
	brrsz digits = brrlib_ndigits(wsp->wem_count, 0, 10);
	unsigned char *buffer = NULL;
	if (goptions->auto_ogg) {
		numbers->wems_to_convert_extract += wsp->wem_count;
	} else {
		numbers->wems_to_extract += wsp->wem_count;
	}
	for (brrsz i = 0; i < wsp->wem_count; ++i) {
		wem_geomT *const wem = &wsp->wems[i];
		if (fseek(file, wem->offset, SEEK_SET)) {
			break;
		}
		if (wem->size > maxsize) {
			if (buffer)
				free(buffer);
			if (!(buffer = malloc(wem->size)))
				return I_BUFFER_ERROR;
			maxsize = wem->size;
		}
		if (wem->size > fread(buffer, 1, wem->size, file)) {
			free(buffer);
			if (feof(file))
				return I_FILE_TRUNCATED;
			return I_IO_ERROR;
		} else if (goptions->auto_ogg) {
			snprintf(output_name, sizeof(output_name), goutput_format, digits, i, ".ogg");
			if (!(err = i_ogg_wem(output_name, library, buffer, wem->size)))
				numbers->wems_convert_extracted++;
		} else {
			snprintf(output_name, sizeof(output_name), goutput_format, digits, i, ".wem");
			if (!(err = i_write_wem(output_name, buffer, wem->size)))
				numbers->wems_extracted++;
		}
	}
	if (buffer)
		free(buffer);
	return err;
}
int BRRCALL
wspbnk_extract(numbersT *const numbers, const char *const input, brrsz input_length,
    const input_optionsT *const options, codebook_libraryT *const library)
{
	int err = 0;
	FILE *in = NULL;
	wspT wsp = {0};

	goptions = options;
	lib_replace_ext(input, input_length, goutput_format, NULL, "_%0*zu%s");
	if (!(in = fopen(input, "rb"))) {
		BRRLOG_ERRN("Failed to open wsp for extraction : %s", strerror(errno));
		return I_IO_ERROR;
	} else if ((err = i_count_wems(in, &wsp))) {
		BRRLOG_ERRN("Failed counting wems from wsp : %s", lib_strerr(err));
		fclose(in);
		return err;
	} else if ((err = i_extract_wems(in, &wsp, library, numbers))) {
		BRRLOG_ERRN("Failed extracting wems from wsp : %s", lib_strerr(err));
		fclose(in);
		wsp_clear(&wsp);
		return err;
	}
	wsp_clear(&wsp);
	fclose(in);
	return 0;
}
