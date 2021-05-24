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

#include "common/NeStr.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <brrtools/brrplatform.h>
#include <brrtools/brrdebug.h>
#include <brrtools/brrlog.h>

#include "common/NeLibrary.h"
#include "common/NeMisc.h"

#if defined(BRRPLATFORMTYPE_UNIX)
#include <strings.h>
#endif

brrsz
NeStrlen(const char *const str, brrsz max)
{
	brrsz i = 0;
	if (!str || !max)
		return 0;
	for (char a = str[0]; i < max && a != 0; ++i, a = str[i]);
	return i;
}

void
NeStrNew(struct NeStr *const str, const char *const cstr, brrsz maxlen)
{
	if (!str)
		return;
	BRRDEBUG_ASSERTM(str->cstr != cstr, "cstr and str->cstr must not overlap");
	/* !cstr || !maxlen produces valid string that can be deleted with NeStrDel */

	str->length = NeStrlen(cstr, maxlen);
	str->cstr = NeSafeAlloc(str->cstr, str->length + 1, 1);
	for (brrsz i = 0; i < str->length; ++i)
		str->cstr[i] = cstr[i];
}

void
NeStrDel(struct NeStr *const str)
{
	if (!str)
		return;
	str->cstr = NeSafeAlloc(str->cstr, 0, 0);
	str->length = 0;
}

void
NeStrCopy(struct NeStr *const out, const struct NeStr src)
{
	NeStrNew(out, src.cstr, src.length);
}

struct NeStr
NeStrShallow(char *cstr, brrsz maxlen)
{
	struct NeStr s = {0};
	s.cstr = cstr;
	s.length = NeStrlen(s.cstr, maxlen);
	return s;
}

brrof
NeStrIndex(const struct NeStr hay, const struct NeStr ndl, brrsz iof)
{
	return NeFind(hay.cstr, hay.length, ndl.cstr, ndl.length, iof);
}

brrof
NeStrRindex(const struct NeStr hay, const struct NeStr ndl, brrsz iof)
{
	return NeRfind(hay.cstr, hay.length, ndl.cstr, ndl.length, iof);
}

brrof
NeStrPrint(struct NeStr *dst, brrof offset, brrsz strlen, const char *const fmt, ...)
{
	va_list lptr;
	brrof prt = 0;
	if (!dst || !fmt)
		return 0;
	if (!strlen) {
		dst->cstr = NeSafeAlloc(dst->cstr, 1, 1);
		dst->cstr[0] = 0;
		dst->length = 0;
		return 0;
	} else if (strlen > NeBLOCKSIZE - 1) {
		strlen = NeBLOCKSIZE - 1;
	}

	dst->cstr = NeSafeAlloc(dst->cstr, strlen + 1, 0);
	va_start(lptr, fmt);
	prt = vsnprintf(dst->cstr + offset, strlen + 1 - offset, fmt, lptr);
	va_end(lptr);
	if (prt > 0) { /* no error printing */
		if (prt > strlen - offset) /* print was truncated */
			prt = strlen - offset;
		dst->length = prt + offset;
	}
	/* total len */
	dst->cstr = NeSafeAlloc(dst->cstr, dst->length + 1, 0);

	return prt;
}

void
NeStrSlice(struct NeStr *const out, const struct NeStr str, brrof start, brrof end)
{
	brrof rv = 0;
	if (!out || !str.length)
		return;

	start = NeSmartMod(start, str.length, 1);
	end = NeSmartMod(end, str.length, 1);

	if (start == end)
		return;

	if (start > end) {
		rv = start;
		start = end;
		end = rv;
		rv = 1;
	}

	out->length = end - start;
	out->cstr = NeSafeAlloc(out->cstr, out->length + 1, 1);

	if (rv) {
		for (brrof k = end - 1, rv = 0; k >= start; --k, ++rv) {
			out->cstr[rv] = str.cstr[k];
		}
	} else {
		for (brrof k = start, rv = 0; k < end; ++k, ++rv)
			out->cstr[rv] = str.cstr[k];
	}
}

void
NeStrJoin(struct NeStr *const out, const struct NeStr a, const struct NeStr b)
{
	if (!out)
		return;
	else if (!b.length) {
		NeStrNew(out, a.cstr, a.length);
		return;
	}
	else if (!a.length) {
		NeStrNew(out, b.cstr, b.length);
		return;
	}

	out->length = a.length + b.length;
	out->cstr = NeSafeAlloc(out->cstr, out->length + 1, 1);
	for (brrsz i = 0; i < a.length; ++i)
		out->cstr[i] = a.cstr[i];
	for (brrsz i = a.length; i < out->length; ++i)
		out->cstr[i] = b.cstr[i - a.length];
}

void
NeStrMerge(struct NeStr *const str, const struct NeStr mg)
{
	if (!str || !mg.length)
		return;
	str->length += mg.length;
	str->cstr = NeSafeAlloc(str->cstr, str->length + 1, 0);
	for (brrsz i = str->length - mg.length; i < str->length; ++i)
		str->cstr[i] = mg.cstr[i - str->length + mg.length];
}

int
NeStrCmp(const char *const cmp, int cse, ...)
{
	va_list lptr;
	char *a = NULL;
	int i = 0;
	va_start(lptr, cse);
	if (cse) {
		while (1) {
			a = va_arg(lptr, char *);
			if (!a || !*a)
				break;
#if defined(BRRPLATFORMTYPE_WINDOWS)
			if (_stricmp(cmp, a) == 0)
#elif defined(BRRPLATFORMTYPE_UNIX)
			if (strcasecmp(cmp, a) == 0)
#else
#error How get strcasecmp?
#endif
			{
				i = 1;
				break;
			}
		}
	} else {
		while (1) {
			a = va_arg(lptr, char *);
			if (!a || !*a)
				break;
			if (strcmp(cmp, a) == 0) {
				i = 1;
				break;
			}
		}
	}
	va_end(lptr);
	return i;
}

int
NeStrEndswith(const struct NeStr str, const struct NeStr cmp)
{
	int c = 1;
	if (!str.length || !cmp.length || cmp.length > str.length)
		return 0;
	for (brrsz i = 0; i < cmp.length && c; ++i) {
		c &= cmp.cstr[i] == str.cstr[str.length - cmp.length + i];
	}
	return c;
}
