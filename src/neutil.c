#include "neutil.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wwise.h"
#include "riff.h"

#define USAGE "Usage: NAeP [[OPTION ...] FILE ...] ..." \
"\nNAeP - NieR:Automated extraction Precept_v"Ne_version"" \
"\nCompiled on "__DATE__", " __TIME__"\n"
#define HELP \
"Most options take affect on all files following and can be toggled." \
"\nSome options are global, and apply to the meta-process itself, marked with (g)." \
"\nOptions:" \
"\n        -h, -help, -v . . . . . . . . . . .  Print this help." \
"\n    File Type Specification:" \
"\n        -a, -auto, -detect  . . . . . . . .  Autodetect filetype from file header or extension." \
"\n        -w, -wem, -weem . . . . . . . . . .  File(s) are single WwRIFFs to be converted to Ogg." \
"\n        -W, -wsp, -wisp . . . . . . . . . .  File(s) are collections of WwRIFFs to be extracted/converted." \
"\n        -b, -bnk, -bank . . . . . . . . . .  The same as '-wsp'." \
"\n        -o, -ogg  . . . . . . . . . . . . .  File(s) are Ogg files to be regranularizeed." \
"\n    OGG Processing Options:" \
"\n        -ri, -rgrn-inplace, -rvb-inplace. .  Oggs are regranularized in-place." \
"\n    WEM Processing Options:" \
"\n        -oi, -ogg-inplace . . . . . . . . .  All WwRIFF-to-Ogg conversion is replaces input files." \
"\n        -cbl, -codebook-library . . . . . .  The following file is the codebook library to use for the" \
"\n                                             WwRIFFs following." \
"\n                                             If no coedbooks are specified for a WwRIFF, then it is assumed" \
"\n                                             the codebooks are inline." \
"\n        -inline . . . . . . . . . . . . . .  The following WwRIFFs have inline codebooks." \
"\n        -stripped . . . . . . . . . . . . .  The following WwRIFFs' are stripped and must be rebuilt from" \
"\n                                             a codebook library." \
"\n    WSP/BNK Processing Options:" \
"\n        -w2o, -wem2ogg  . . . . . . . . . .  Convert WwRIFFs from '-wsp' files to Oggs, rather than extracting them." \
"\n        -white, -weiss," \
"\n        -black, -noir . . . . . . . . . . .  Comma-separated list of indices used to determine" \
"\n                                             what indices to process from the file(s)." \
"\n                                             Blacklists and whitelists are mutually exclusive; each " \
"\n                                             overrides the others preceding it." \
"\n                                             E.g. '-black 3,9' would process every index except 3 and 9," \
"\n                                             but  '-white 3,9' would process only indices 3 and 9." \
"\n        -rubrum . . . . . . . . . . . . . .  Toggle the list to between being a whitelist/blacklist." \
"\n    Miscellaneous options:" \
"\n        -!  . . . . . . . . . . . . . . . .  The following argument is a file path, not an option." \
"\n        --  . . . . . . . . . . . . . . . .  All following arguments are file paths, not options." \
"\n        -d, -debug  . . . . . . . . . . . .  Enable debug output, irrespective of quiet settings." \
"\n        -co, -comments  . . . . . . . . . .  Toggles inserting of additional comments in output Oggs." \
"\n        -c, -color  . . . . . . . . . . . .  Toggle color logging." \
"\n        -C, -global-color (g) . . . . . . .  Toggle whether log styling is enabled at all." \
"\n        -r, -report-card (g)  . . . . . . .  Print a status report of all files processed after processing." \
"\n        +r, -full-report (g)  . . . . . . .  Print a more full report of all files processed." \
"\n        -q, -quiet  . . . . . . . . . . . .  Suppress one additional level of non-critical." \
"\n        +q, +quiet  . . . . . . . . . . . .  Show one additional level non-critical output." \
"\n        -Q, -qq, -too-quiet . . . . . . . .  Suppress all output, including anything critical." \
"\n        -n, -dry, -dry-run  . . . . . . . .  Don't actually do anything, just log what would happen." \
"\n        -reset (g)  . . . . . . . . . . . .  Argument options reset to default values after each file passed." \

int /* Returns non-void so that 'return print_usage()' is valid */
print_usage(void)
{
	fprintf(stdout, USAGE"\n""    -h, -help, -v . . . . . . . . . . .  Print help.""\n");
	exit(0);
	return 0;
}
int /* Returns non-void so that 'return print_help()' is valid */
print_help(void)
{
	fprintf(stdout, USAGE"\n"HELP"\n");
	exit(0);
	return 0;
}

const fcc_t fcc_OggS = fcc_str(,"OggS");
const fcc_t fcc_BKHD = fcc_str(,"BKHD");
const fcc_t fcc__z   = fcc_chr(0x28, 0xb5, 0x2f, 0xfd);

const fcc_t fcc_RIFF = fcc_str(,"RIFF");
const fcc_t fcc_RIFX = fcc_str(,"RIFX");
const fcc_t fcc_XFIR = fcc_str(,"XFIR");
const fcc_t fcc_FFIR = fcc_str(,"FFIR");

/* This'll work fine for 4-byte fcc's, it's just others that are less-than 4 that worry me. */
int
fcccmp(fcc_t a, fcc_t b)
{
	if (a.n != b.n)
		return a.n - b.n;
	if (a.r != b.r) {
		brru1 temp = b._0;
		b._0 = b._3;
		b._3 = temp;
		temp = b._1;
		b._1 = b._2;
		b._2 = temp;
	}
	if (!a.r)
		return memcmp(&a.v, &b.v, a.n);
	return memcmp((brru1*)&a.v + (4-a.n), (brru1*)&b.v + (4-b.n), a.n);
}

int
nefilter_init(nefilter_t *const filter, const char *const arg)
{
	if (!filter)
		return -1;
	int arglen = strlen(arg);
	if (!arg || !arglen)
		return 0;

	nefilter_t f = {0};
	char *const _arg = (char*const)arg;
	int offset = 0;
	while (offset < arglen) {
		int comma = offset;
		for (;comma < arglen && arg[comma] != ','; ++comma);
		if (comma > offset) {
			int digit = offset;
			for (;digit < comma && !isdigit(arg[digit]); ++digit);
			if (digit < comma) {
				char *start = _arg + digit;
				char *error = NULL;
				unsigned long long val = 0;
				_arg[comma] = 0; // Is setting the comma to null then back necessary?
				val = strtoull(start, &error, 0);
				_arg[comma] = ',';
				if (start[0] && error[0] == ',') { /*  Complete success */
					if (!nefilter_contains(&f, val)) {
						nefilter_index *new = realloc(f.list, sizeof(*new) * (f.count + 1));
						if (!new)
							return -1;
						new[f.count++] = val;
						f.list = new;
					}
				}
			}
		}
		offset = comma + 1;
	}
	*filter = f;
	return 0;
}

void
nefilter_clear(nefilter_t *const filter)
{
	if (filter) {
		if (filter->list)
			free(filter->list);
		memset(filter, 0, sizeof(*filter));
	}
}

int
nefilter_contains(const nefilter_t *const filter, brru4 index)
{
	if (!filter || !filter->list)
		return 0;
	for (brru4 i = 0; i < filter->count; ++i) {
		if (filter->list[i] == index)
			return 1;
	}
	return 0;
}

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
	brru4 total = unpacker->storage - unpacker->endbyte;
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
	while (1) {
		while (riff_consume_chunk(&rf, &cs, &ds)) {
			switch (ds.status) {
				case riff_status_chunk_unrecognized:
					/* Seek forward a single byte to skip unrecognized/broken chunk */
					riff_datasync_seek(&ds, 1);
				case riff_status_consume_again:
					continue;
				case riff_status_chunk_incomplete:
					*riff = rf;
					return 0;

				case riff_status_system_error:
				case riff_status_not_riff:
				case riff_status_corrupt:
				default:
					riff_clear(&rf);
					return -1;
			}
		}
	}
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
