#ifndef NeStr_h
#define NeStr_h

#include "common/NeTypes.h"

struct NeStr {
	char *cstr;
	NeSz length;
};

/* Return length of str excluding null-terminator, up to max */
NeSz NeStrlen(const char *const str, NeSz max);
/* Initialize a new string; can be done on top of an existing string */
/* Str MUST be initialized (specifically, str.cstr must be NULL or a valid pointer) */
/* Pass -1 for maxlen to have no maxlen */
/* Return -1 on error */
void NeStrNew(struct NeStr *const str, const char *const cstr, NeSz maxlen);
void NeStrCopy(struct NeStr *const out, const struct NeStr src);
/* Creates a NeStr without copying cstr, dangerous if cstr is supposed to be const */
struct NeStr NeStrShallow(char *cstr, NeSz maxlen);

NeOf NeStrIndexOf(const struct NeStr hay, const struct NeStr ndl, NeSz iof);
NeOf NeStrRindex(const struct NeStr hay, const struct NeStr ndl, NeSz iof);

/* printf into dst, return bytes printed, -1 if error */
NeOf NeStrPrint(struct NeStr *dst, NeOf offset, NeSz strlen, const char *const fmt, ...);

/* Slice str (similar to python) into out */
void NeStrSlice(struct NeStr *const out, const struct NeStr str, NeOf start, NeOf end);
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
