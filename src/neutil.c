/*
Copyright 2021-2022 BowToes (bow.toes@mailfence.com)

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

#include "neutil.h"

#include <errno.h>
#include <string.h>

#include "nelog.h"
#include "wwise.h"
#include "riff.h"

extern inline brru4 nepack_pack(oggpack_buffer *const packer, brru4 value, int bits);

long long
nepack_transfer(oggpack_buffer *const unpacker, int unpack, oggpack_buffer *const packer, int pack)
{
	brrs8 r = 0;
	if (unpack)
		if (-1 == (r = oggpack_read(unpacker, unpack)))
			return -1;
	oggpack_write(packer, r, pack);
	return r;
}

long long
nepack_transfer_remaining(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	brru4 total = 8 * (unpacker->storage - unpacker->endbyte);
	if (unpacker->endbit)
		total += 8 - unpacker->endbit;
	return nepack_transfer_lots(unpacker, packer, total);
}

long long
nepack_transfer_lots(oggpack_buffer *const unpacker, oggpack_buffer *const packer, unsigned long bits)
{
	if (!packer || !unpacker)
		return -1;
	if (!bits)
		return 0;
	for (brru4 i = 0; i < bits >> 5; ++i) {
		if (-1 == nepack_transfer(unpacker, 32, packer, 32))
			return -1;
	}
	if (-1 == nepack_transfer(unpacker, bits & 31, packer, bits & 31))
		return -1;
	return bits;
}

int
neutil_count_ones(unsigned long x)
{
	int r = 0;
	while (x) {
		r += x & 1;
		x >>= 1;
	}
	return r;
}

int
neutil_count_bits(unsigned long x)
{
	int r = 0;
	while (x) {
		++r;
		x >>= 1;
	}
	return r;
}

long
neutil_lookup1(long entries, long dimensions)
{
	int bits = neutil_count_bits(entries);
	long vals = entries >> ((bits - 1) * (dimensions - 1) / dimensions);

	while(1) {
		long acc = 1;
		long acc1 = 1;
		for(long i = 0; i < dimensions; ++i) {
			acc *= vals;
			acc1 *= vals + 1;
		}

		if(acc <= entries && acc1 > entries)
			return vals;
		else if(acc > entries)
			 vals--;
		else
			 vals++;
	}
}

int
neutil_write_ogg(ogg_stream_state *const stream, const char *const file)
{
	if (!stream || !file)
		return -1;
	FILE *f = fopen(file, "wb");
	if (!f) {
		Err(,"Could not open Ogg stream destination '%s': %s (%d)", file, strerror(errno), errno);
		return -1;
	}
	ogg_page pager;
	while (ogg_stream_pageout(stream, &pager) || ogg_stream_flush(stream, &pager)) {
		if (pager.header_len != fwrite(pager.header, 1, pager.header_len, f)) {
			fclose(f);
			Err(,"Failed to write Ogg page header to '%s': %s (%d)", file, strerror(errno), errno);
			return -1;
		}
		if (pager.body_len != fwrite(pager.body, 1, pager.body_len, f)) {
			fclose(f);
			Err(,"Failed to write Ogg page body to '%s': %s (%d)", file, strerror(errno), errno);
			return -1;
		}
	}
	fclose(f);
	return 0;
}

static inline int
i_consume_next_buffer_chunk(riff_t *const riff, riff_chunkstate_t *const chunkstate, riff_datasync_t *const datasync)
{
	while (riff_consume_chunk(riff, chunkstate, datasync)) {
		switch (datasync->status) {
			case riff_status_chunk_unrecognized:
				/* Seek forward a single byte to skip unrecognized/broken chunk */
				riff_datasync_seek(datasync, 1);
			case riff_status_consume_again:
				continue;
			case riff_status_chunk_incomplete:
				return 1;
			case riff_status_system_error:
			case riff_status_not_riff:
			case riff_status_corrupt:
			default: return -1;
		}
	}
	return 0;
}
int
neutil_buffer_to_riff(riff_t *const riff, const void *const buffer, brrsz buffer_size)
{
	if (!riff || !buffer || !buffer_size)
		return -1;

	riff_datasync_t ds = {0};
	if (riff_datasync_from_buffer(&ds, (void*)buffer, buffer_size))
		return -1;

	riff_chunkstate_t cs = {0};
	riff_t rf = {0};
	int e = 0;
	while (0 == (e = i_consume_next_buffer_chunk(&rf, &cs, &ds))) {
		riff_chunkstate_zero(&cs);
	}
	if (e == -1) {
		riff_clear(&rf);
		return -1;
	}
	*riff = rf;
	return 0;
}

int
neutil_buffer_to_wwriff(wwriff_t *const wwriff, const void *const buffer, brrsz buffer_size)
{
	if (!wwriff || !buffer || !buffer_size)
		return -1;

	riff_t rf = {0};
	if (neutil_buffer_to_riff(&rf, buffer, buffer_size))
		return -1;
	wwriff_t wf = {0};
	if (wwriff_init(&wf, &rf)) {
		riff_clear(&rf);
		return -1;
	}
	riff_clear(&rf);
	*wwriff = wf;
	return 0;
}

extern inline brru8 nemath_umin(brru8, brru8);
extern inline brrs8 nemath_smin(brrs8, brrs8);
