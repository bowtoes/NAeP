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

#ifndef NeStr_h
#define NeStr_h

#include <brrtools/brrtypes.h>

struct NeStr {
	char *cstr;
	brrsz length;
};

/* Return length of str excluding null-terminator, up to max */
brrsz NeStrlen(const char *const str, brrsz max);

/* Str MUST MUST MUST be initialized (specifically, str.cstr must be NULL or a
 * valid pointer) */
/* Initialize a new string; can be done on top of an existing string */
/* Pass -1 for maxlen to have no maxlen */
void NeStrNew(struct NeStr *const str, const char *const cstr, brrsz maxlen);
void NeStrCopy(struct NeStr *const out, const struct NeStr src);
/* Creates a NeStr without copying cstr, dangerous if cstr is supposed to be const */
struct NeStr NeStrShallow(char *cstr, brrsz maxlen);

brrof NeStrIndex(const struct NeStr hay, const struct NeStr ndl, brrsz iof);
brrof NeStrRindex(const struct NeStr hay, const struct NeStr ndl, brrsz iof);

/* printf into dst, return bytes printed, -1 if error */
brrof NeStrPrint(struct NeStr *dst, brrof offset, brrsz strlen, const char *const fmt, ...);

/* Slice str (similar to python) into out */
void NeStrSlice(struct NeStr *const out, const struct NeStr str, brrof start, brrof end);
/* Join b onto a and store output in out */
void NeStrJoin(struct NeStr *const out, const struct NeStr a, struct NeStr b);
/* Same as NeStrJoin, but does so in-place (on str) */
void NeStrMerge(struct NeStr *const str, const struct NeStr mg);

/* Frees associated memory */
void NeStrDel(struct NeStr *const str);

/* Last string must be "" or NULL */
int NeStrCmp(const char *const cmp, int cse, ...);

int NeStrEndswith(const struct NeStr str, const struct NeStr cmp);

#endif /* NeStr_h */
