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
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrlib.h>

#include "common_lib.h"
#include "errors.h"
#include "riff.h"

static const input_optionsT *goptions = NULL;
static const char *ginput_name = NULL;
static numbersT *gnumbers = NULL;
static char goutput_name[BRRPATH_MAX_PATH + 1] = {0};

typedef struct wsp_wem {
	brrsz offset;
	brrsz size;
	riff_byteorderT byteorder;
} wsp_wemT;
typedef struct wsp {
	wsp_wemT *wems;
	brrsz wem_count;
} wspT;

static void BRRCALL
i_clear_wsp(wspT *const wsp)
{
	if (wsp->wems) {
		free(wsp->wems);
	}
	memset(wsp, 0, sizeof(*wsp));
}
static int BRRCALL
i_count_wems(FILE *const file, wspT *const wsp)
{
	fourccT cc;
	wsp_wemT current = {0};
	while (4 == fread(&cc, 1, 4, file)) {
		if (!(current.byteorder = riff_cc_byteorder(cc.integer))) {
			if (fseek(file, -3, SEEK_CUR))
				return I_IO_ERROR;
		} else { /* Found RIFF */
			if (4 != fread(&current.size, 1, 4, file)) {
				if (feof(file)) {
					break;
				} else {
					return I_IO_ERROR;
				}
			} else if (brrlib_alloc((void **)&wsp->wems, (wsp->wem_count + 1) * sizeof(*wsp->wems), 0)) {
				return I_BUFFER_ERROR;
			} else if (0 == (current.byteorder & 1)) { /* Is even? */
				brru4 tmp;
				riff_copier_data(current.byteorder)(&tmp, &current.size, 4);
				current.size = tmp;
			}
			wsp->wems[wsp->wem_count++] = current;
			current.offset += current.size;
			if (fseek(file, current.size, SEEK_CUR))
				return I_IO_ERROR;
		}
	}
	if (ferror(file)) {
		return I_IO_ERROR;
	}
	rewind(file);
	return 0;
}
static int BRRCALL
i_extract_wems(FILE *const file, wspT *const wsp)
{
	return 0;
}
static int BRRCALL
int_extract_wsp(void)
{
	int err = 0;
	FILE *in = NULL;
	wspT wsp;
	if (!(in = fopen(ginput_name, "rb"))) {
		BRRLOG_ERRN("Failed to open wsp for extraction : %s", strerror(errno));
		return I_IO_ERROR;
	} else if ((err = i_count_wems(in, &wsp))) {
		BRRLOG_ERRN("Failed counting wems from wsp : %s", lib_strerr(err));
		fclose(in);
		i_clear_wsp(&wsp);
		return err;
	} else if ((err = i_extract_wems(in, &wsp))) {

	}
	fclose(in);
	return 0;
}

int BRRCALL
extract_wsp(numbersT *const numbers, const char *const input, brrsz input_length,
    const input_optionsT *const options, input_libraryT *const library)
{
	int err = 0;
	numbers->wsps_to_process++;
	if (options->dry_run) {
		BRRLOG_FOREP(LOG_COLOR_DRY, " Extract WSP (dry)");
	} else {
		BRRLOG_FOREP(LOG_COLOR_WET, " Extracting WSP...");
		goptions = options;
		ginput_name = input;
		gnumbers = numbers;
		err = int_extract_wsp();
		/* for each 'wem' in 'path':
		 *   extract 'wem' to 'path_base_%0*d.wem'
		 *   auto_ogg: convert_wem(numbers, 'path_base_%0*d.wem', ...);
		 *   appropiately increment 'wems_extracted', 'wems_extracted_converted',
		 *       'wems_extracted_failed'
		 * */
	}
	if (!err) {
		numbers->wsps_processed++;
		BRRLOG_MESSAGETP(gbrrlog_level_normal, LOG_FORMAT_SUCCESS, "Success!");
	} else {
		numbers->wsps_failed++;
		BRRLOG_MESSAGETP(gbrrlog_level_normal, LOG_FORMAT_FAILURE, "Failure! (%d)", err);
	}
	return err;
}
