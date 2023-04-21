/* Copyright (c), bowtoes (bow.toes@mailfence.com)
Apache 2.0 license, http://www.apache.org/licenses/LICENSE-2.0
Full license can be found in 'license' file */

#include "nefilter.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

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
						nefilter_index_t *new = realloc(f.list, sizeof(*new) * (f.count + 1));
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
