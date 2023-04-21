/* Copyright (c), bowtoes (bow.toes@mailfence.com)
Apache 2.0 license, http://www.apache.org/licenses/LICENSE-2.0
Full license can be found in 'license' file */

#ifndef NAeP_nefilter_h
#define NAeP_nefilter_h

#include <brrtools/brrtypes.h>

typedef brru4 nefilter_index_t;
typedef enum nefilter_type {
	nefilter_white = 0,
	nefilter_black,
} nefilter_type_t;

/* TODO Eventually index white/blackisting can be replaced by a more flexible filtering system, similar to in 'countwsp' */
typedef struct nefilter {
	nefilter_index_t *list;
	brru4 count;
	nefilter_type_t type;
} nefilter_t;

int
nefilter_init(nefilter_t *const filter, const char *const arg);

void
nefilter_clear(nefilter_t *const filter);

int
nefilter_contains(const nefilter_t *const filter, brru4 index);

#endif /* nefilter_h */
