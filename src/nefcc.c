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

#include "nefcc.h"

#include <ctype.h>
#include <string.h>

#include "nelog.h"

/* This'll work fine for 4-byte fcc's, but others that are less-than 4 bytes worry me. */
int
fcccmp(fcc_t a, fcc_t b)
{
	if (a.n != b.n) {
		Nor(,"%i != %i : '%c%c%c%c' vs '%c%c%c%c'", _fcc_print_split(a), _fcc_print_split(b));
		return a.n - b.n;
	}
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
