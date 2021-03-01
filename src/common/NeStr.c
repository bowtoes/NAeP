#include "common/NeStr.h"
#include "common/NePlatform.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#if defined(NePLATFORMTYPE_POSIX) || defined(NePLATFORMTYPE_BSD)
#include <strings.h>
#endif

#include "common/NeLibrary.h"
#include "common/NeDebugging.h"
#include "common/NeMisc.h"

NeSz
NeStrlen(const char *const str, NeSz max)
{
	NeSz i = 0;
	if (!str || !max)
		return 0;
	for (char a = str[0]; i < max && a != 0; ++i, a = str[i]);
	return i;
}

void
NeStrNew(struct NeStr *const str, const char *const cstr, NeSz maxlen)
{
	if (!str)
		return;
	NeASSERTM(str->cstr != cstr, "cstr and str->cstr must not overlap");
	/* !cstr || !maxlen produces valid string that can be deleted with NeStrDel */

	str->length = NeStrlen(cstr, maxlen);
	str->cstr = NeSafeAlloc(str->cstr, str->length + 1, 1);
	for (NeSz i = 0; i < str->length; ++i)
		str->cstr[i] = cstr[i];
}

void
NeStrCopy(struct NeStr *const out, const struct NeStr src)
{
	return NeStrNew(out, src.cstr, src.length);
}

struct NeStr
NeStrShallow(char *cstr, NeSz maxlen)
{
	struct NeStr s = {0};
	s.cstr = cstr;
	s.length = NeStrlen(s.cstr, maxlen);
	return s;
}

NeOf
NeStrIndexOf(const struct NeStr hay, const struct NeStr ndl, NeSz iof)
{
	return NeFind(hay.cstr, hay.length, ndl.cstr, ndl.length, iof);
}

NeOf
NeStrRindex(const struct NeStr hay, const struct NeStr ndl, NeSz iof)
{
	return NeRfind(hay.cstr, hay.length, ndl.cstr, ndl.length, iof);
}

NeOf
NeStrPrint(struct NeStr *dst, NeOf offset, NeSz strlen, const char *const fmt, ...)
{
	va_list lptr;
	NeOf prt = 0;
	if (!dst || !fmt)
		return 0;
	if (!strlen) {
		dst->cstr = NeSafeAlloc(dst->cstr, 1, 1);
		dst->cstr[0] = 0;
		dst->length = 0;
		return 0;
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
NeStrSlice(struct NeStr *const out, const struct NeStr str, NeOf start, NeOf end)
{
	NeOf rv = 0;
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
		for (NeOf k = end - 1, rv = 0; k >= start; --k, ++rv) {
			out->cstr[rv] = str.cstr[k];
		}
	} else {
		for (NeOf k = start, rv = 0; k < end; ++k, ++rv)
			out->cstr[rv] = str.cstr[k];
	}
}

void
NeStrJoin(struct NeStr *const out, const struct NeStr a, const struct NeStr b)
{
	if (!out)
		return;
	else if (!b.length)
		return NeStrNew(out, a.cstr, a.length);
	else if (!a.length)
		return NeStrNew(out, b.cstr, b.length);

	out->length = a.length + b.length;
	out->cstr = NeSafeAlloc(out->cstr, out->length + 1, 1);
	for (NeSz i = 0; i < a.length; ++i)
		out->cstr[i] = a.cstr[i];
	for (NeSz i = a.length; i < out->length; ++i)
		out->cstr[i] = b.cstr[i - a.length];
}

void
NeStrMerge(struct NeStr *const str, const struct NeStr mg)
{
	if (!str || !mg.length)
		return;
	str->length += mg.length;
	str->cstr = NeSafeAlloc(str->cstr, str->length + 1, 0);
	for (NeSz i = str->length - mg.length; i < str->length; ++i)
		str->cstr[i] = mg.cstr[i - str->length + mg.length];
}

void
NeStrDel(struct NeStr *const str)
{
	if (!str)
		return;
	str->cstr = NeSafeAlloc(str->cstr, 0, 0);
	str->length = 0;
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
#if defined(NePLATFORMTYPE_WINDOWS)
			if (_stricmp(cmp, a) == 0)
#elif defined(NePLATFORMTYPE_POSIX) || defined(NePLATFORMTYPE_BSD)
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
