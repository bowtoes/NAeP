/* Copyright (c), bowtoes (bow.toes@mailfence.com)
Apache 2.0 license, http://www.apache.org/licenses/LICENSE-2.0
Full license can be found in 'license' file */

#include "rifflist.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrnum.h>

#include "neinput.h"
#include "neutil.h"
#include "nelog.h"
#include "wwise.h"

int
rifflist_scan(rifflist_t *const out_list, const unsigned char *const buffer, brrsz buffer_size)
{
	if (!out_list || !buffer || buffer_size < 4)
		return -1;

	rifflist_t l = {0};
	brrsz offset = 0;
	while (offset < buffer_size - 8) {
		const unsigned char *data = buffer + offset;
		riffgeometry_t current = {.buffer_offset = offset};
		/* Scan for a recognized RIFF head chunk */
		if (!(current.byteorder = riff_cc_byteorder((fcc_t)fcc_arr((char*)data)))) {
			offset++;
			continue;
		}

		/* RIFF size is next u32 */
		current.riff_size = ((brru4 *)data)[1];
		if (current.byteorder == riff_byteorder_RIFX || current.byteorder == riff_byteorder_FFIR) {
			brru4 tmp;
			riff_copier_data(current.byteorder)(&tmp, &current.riff_size, 4);
			current.riff_size = tmp;
		}

		current.riff_size += 8; /* 8 = byteorder_bytes (4) + size_bytes (4) */
		if (current.riff_size > buffer_size - current.buffer_offset) {
			War(,"Corrupted/incomplete RIFF in data, #%zu, offset %zu + size %zu > data size %zu", l.n_riffs, current.buffer_offset, current.riff_size, buffer_size);
			break;
		}

		riffgeometry_t *new = realloc(l.riffs, sizeof(*new) * (l.n_riffs + 1));
		if (!new) {
			rifflist_clear(&l);
			return -1;
		}
		new[l.n_riffs++] = current;
		l.riffs = new;
		Deb(,"Found RIFF %zu at offset 0x%016X, %lu bytes", l.n_riffs, current.buffer_offset, current.riff_size);

		offset += current.riff_size;
	}
	XDeb(,"Scanned list of %zu WwRIFFs...", l.n_riffs);

	*out_list = l;
	return 0;
}
void
rifflist_clear(rifflist_t *const list)
{
	if (list) {
		if (list->riffs)
			free(list->riffs);
		memset(list, 0, sizeof(*list));
	}
}

/* TODO for each embedded WwRIFF processed, log a symbol for success and one for failure;
 * e.g. :
 *   Processing file.wsp... . . . X X . . . . X . . . X . . X X . ... etc.
 * */

static char s_output_file[brrpath_max_path + 1] = {0};
#define OUTPUT_FORMAT "_%0*zu"
int
rifflist_convert(
    const rifflist_t *const list,
    const unsigned char *const buffer,
    nestate_t *const state,
    const neinput_t *const input,
    const codebook_library_t *const library,
    const char *const output_root
)
{
	if (!buffer || !list || !state || !input || !output_root)
		return -1;

	int digits = brrnum_udigits(list->n_riffs, 10);
	XDeb(,"Converting WwRIFF list...");
	for (brrsz i = 0; i < list->n_riffs; ++i) {
		if (input->filter.count) {
			int contained = nefilter_contains(&input->filter, i);
			if ((contained && input->filter.type) || (!contained && !input->filter.type)) {
				XDeb(,"WwRIFF %zu was filtered due to %slist", i, input->filter.type?"black":"white");
				continue;
			}
		}

		snprintf(s_output_file, sizeof(s_output_file), "%s"OUTPUT_FORMAT".ogg", output_root, digits, i);
		state->stats.wem_converts.assigned++;

		int err = 0;
		wwriff_t wwriff = {0};
		const riffgeometry_t *const geom = &list->riffs[i];
		if ((err = neutil_buffer_to_wwriff(&wwriff, buffer + geom->buffer_offset, geom->riff_size))) {
			Err(,"Could not parse WWRIFF (!f=m:#%*zu!), skipping.", digits, i);
		} else {
			if (input->cfg.add_comments) {
				if ((err = wwriff_add_comment(&wwriff, "SourceFile=%s", input->path.full))) {
					Err(,"Could not add 'SourceFile' comment.");
				} else if ((err = wwriff_add_comment(&wwriff, "OutputFile=%s", s_output_file))) {
					Err(,"Could not add 'OutputFile' comment.");
				}
			}
			if (!err) {
				ogg_stream_state streamer;
				if ((err = wwise_convert_wwriff(&wwriff, &streamer, library, input))) {
					Err(,"Could not conver WWRIFF (!f=m:#%*zu!), skipping.", digits, i);
				} else {
					if ((err = neutil_write_ogg(&streamer, s_output_file))) {
						Err(,"Could not write converted WWRIFF (!f=m:#%*zu!), skipping.", digits, i);
					}
					ogg_stream_clear(&streamer);
				}
			}
			wwriff_clear(&wwriff);
		}

		if (!err) {
			XDeb(,"Successfuly converted WwRIFF to '%s'", s_output_file);
			state->stats.wem_converts.succeeded++;
		} else {
			XDeb(,"Failed to convert WwRIFF to '%s'", s_output_file);
			state->stats.wem_converts.failed++;
		}
	}
	return 0;
}
int
rifflist_extract(
    const rifflist_t *const list,
    const unsigned char *const buffer,
    nestate_t *const state,
    const neinput_t *const input,
    const char *const output_root
)
{
	if (!buffer || !list || !state || !input || !output_root)
		return -1;

	int digits = brrnum_udigits(list->n_riffs, 10);
	XDeb(,"Xcting WwRIFF list...");
	for (brrsz i = 0; i < list->n_riffs; ++i) {
		const riffgeometry_t *const wem = &list->riffs[i];
		brrsz wrote = 0;
		FILE *output = NULL;
		if (input->filter.count) {
			int contained = nefilter_contains(&input->filter, i);
			if ((contained && input->filter.type) || (!contained && !input->filter.type)) {
				Deb(,"WwRIFF %zu was filtered due to %slist", i, input->filter.type?"black":"white");
				continue;
			}
		}

		snprintf(s_output_file, sizeof(s_output_file), "%s"OUTPUT_FORMAT".wem", output_root, digits, i);
		state->stats.wem_extracts.assigned++;

		if (!(output = fopen(s_output_file, "wb"))) {
			Err(,"Failed to open output WEM (!f=m:#%*zu!) ((!f=y:'%s'!)), skipping", digits, i, s_output_file);
			state->stats.wem_extracts.failed++;
		} else {
			if (wem->riff_size != (wrote = fwrite(buffer + wem->buffer_offset, 1, wem->riff_size, output))) {
				Err(,"Failed to write to output WEM (!f=m:#%*zu!) ((!f=y:'%s'!)), skipping", digits, i, s_output_file);

				XDeb(,"Failed to extract WwRIFF to '%s'", s_output_file);
				state->stats.wem_extracts.failed++;
			} else {
				XDeb(,"Successfuly extracted WwRIFF to '%s'", s_output_file);
				state->stats.wem_extracts.succeeded++;
			}
			fclose(output);
		}
	}
	return 0;
}
