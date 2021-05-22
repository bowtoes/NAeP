#include "common/NeLogging.h"

#include <stdarg.h>

const struct NeLogFmt NeFmtNormal   = {  NePrNormal,          0,       0,           0, 0, ""};
const struct NeLogFmt NeFmtWarning  = { NePrWarning, NeClYellow,       0,  NeStBright, 0, "[Warning]"};
const struct NeLogFmt NeFmtError    = {   NePrError,    NeClRed,       0,           0, 0, "[Error]"};
const struct NeLogFmt NeFmtCritical = {NePrCritical,    NeClRed,       0, NeStReverse, 0, "[CRITICAL]"};
const struct NeLogFmt NeFmtDebug    = {   NePrDebug, NeClYellow,       0, NeStReverse, 0, "[Debug]"};

const struct NeLogFmt NeFmtClear    = { NePrCount,          0,          0,        0,          0, "[clear]"};
      struct NeLogFmt NeFmtLast     = {NePrNormal, NeClNormal, NeClNormal, NeStNone, NeFnNormal, "[last]"};

static NeBy loglevel = NePrDebug;
static NeBy logdbg = 0;
#if defined(NeLOGCOLORS) && !defined(NeNOLOGCOLORS)
static NeBy logcolor = 1;
#else
static NeBy logcolor = 0;
#endif
static NeU8 logcount = 0;
#if defined(NeLOGFLUSH) && !defined(NeNOLOGFLUSH)
static FILE *lastloc = 0;
#endif

/* HIDEOUS */
/* HIDEOUS */
/* HIDEOUS */
/* HIDEOUS */
/* HIDEOUS */
/* HIDEOUS */
/* HIDEOUS */
/* HIDEOUS */
/* https://stackoverflow.com/questions/4842424/list-of-ansi-color-escape-sequences */
static int fgid(enum NeLogCl fg) {
	switch (fg) {
	case NeClBlack:      return 30; case NeClRed:          return 31;
	case NeClGreen:      return 32; case NeClYellow:       return 33;
	case NeClBlue:       return 34; case NeClMagenta:      return 35;
	case NeClCyan:       return 36; case NeClWhite:        return 37;
	case NeClDarkGrey:   return 90; case NeClLightRed:     return 91;
	case NeClLightGreen: return 92; case NeClLightYellow:  return 93;
	case NeClLightBlue:  return 94; case NeClLightMagenta: return 95;
	case NeClLightCyan:  return 96; case NeClLightWhite:   return 97;
	default: return 39;
	}
}
static int bgid(enum NeLogCl bg) {
	return 10 + fgid(bg);
}
static int stid(enum NeLogSt st) {
	switch (st) {
		case        NeStNone: return  0; case         NeStBold: return  1;
		case         NeStDim: return  2; case      NeStItalics: return  3;
		case       NeStUnder: return  4; case        NeStBlink: return  5;
		case   NeStFastBlink: return  6; case      NeStReverse: return  7;
		case     NeStConceal: return  8; case    NeStStrikeout: return  9;
		case     NeStFraktur: return 20; case       NeStNoBold: return 21;
		case    NeStNoBright: return 22; case    NeStNoItalics: return 23;
		case     NeStNoUnder: return 24; case      NeStNoBlink: return 25;
		case   NeStNoReverse: return 27; case       NeStReveal: return 28;
		case NeStNoStrikeout: return 29; case        NeStFrame: return 51;
		case      NeStCircle: return 52; case         NeStOver: return 53;
		case     NeStNoFrame: return 54; case       NeStNoOver: return 55;
		case      NeStIUnder: return 60; case NeStIDoubleUnder: return 61;
		case       NeStIOver: return 62; case  NeStIDoubleOver: return 63;
		case     NeStIStress: return 64; case         NeStIOff: return 65;
		default: return 0;
	}
}
static int fnid(enum NeLogFn fn) {
	if (fn >= 0 && fn < NeFnCount)
		return fn + 10;
	return 10;
}

char NeFmtStr[] = "\x1b[000;039;049;010m";
#if defined(NeLOGCOLORS) && !defined(NeNOLOGCOLORS)
const char *NeTextFormat(enum NeLogCl fg, enum NeLogCl bg, enum NeLogSt st, enum NeLogFn fn) {
	if (fg == NeClLast) fg = NeFmtLast.fg;
	if (bg == NeClLast) bg = NeFmtLast.bg;
	if (st == NeStLast) st = NeFmtLast.st;
	if (fn == NeFnLast) fn = NeFmtLast.fn;
	snprintf(NeFmtStr, 19, "\x1b[%03i;%03i;%03i;%03im",
			stid(st),
			fgid(fg),
			bgid(bg),
			fnid(fn));
	return NeFmtStr;
}
#endif
static const char *updatefmtstr2(struct NeLogFmt fmt) {
	return NeTextFormat(fmt.fg, fmt.bg, fmt.st, fmt.fn);
}

#define NeSNPRINTF(txt, pos, len, ...) {\
	NeOf tmp = snprintf((txt), (len), __VA_ARGS__);\
	if (tmp > 0) pos += tmp < (len) ? tmp : (len) - 1;\
}
#define NeVSNPRINTF(txt, pos, len, fmt, lptr) {\
	NeOf tmp = vsnprintf((txt), (len), (fmt), lptr);\
	if (tmp > 0) pos += tmp < (len) ? tmp : (len) - 1;\
}

/* TODO all of this is really hideous, and will be difficult (if at all possible)
 * to port to windows */
static NeSz getlogprefix(char *txt, NeSz maxlen, enum NeLogPr pr, FILE **loc, int pfx) {
	NeSz wrt = 0;
	*loc = stdout;
	/* HIDEOUS */
#if defined(NeLOGCOLORS) && !defined(NeNOLOGCOLORS)
if (logcolor) {
	if      (pr == NePrDebug)    { *loc = stderr; if (pfx) NeSNPRINTF(txt, wrt, maxlen, "%s%s"NeClrStr" ", updatefmtstr2(NeLogPrFmt(pr)), NeLogPrPfx(pr)); }
	else if (pr == NePrWarning)  { *loc = stderr; if (pfx) NeSNPRINTF(txt, wrt, maxlen, "%s%s"NeClrStr" ", updatefmtstr2(NeLogPrFmt(pr)), NeLogPrPfx(pr)); }
	else if (pr == NePrError)    { *loc = stderr; if (pfx) NeSNPRINTF(txt, wrt, maxlen, "%s%s"NeClrStr" ", updatefmtstr2(NeLogPrFmt(pr)), NeLogPrPfx(pr)); }
	else if (pr == NePrCritical) { *loc = stderr; if (pfx) NeSNPRINTF(txt, wrt, maxlen, "%s%s"NeClrStr" ", updatefmtstr2(NeLogPrFmt(pr)), NeLogPrPfx(pr)); }
} else {
#endif
	if      (pr == NePrDebug)    { *loc = stderr; if (pfx) NeSNPRINTF(txt, wrt, maxlen, "%s ", NeLogPrPfx(pr)); }
	else if (pr == NePrWarning)  { *loc = stderr; if (pfx) NeSNPRINTF(txt, wrt, maxlen, "%s ", NeLogPrPfx(pr)); }
	else if (pr == NePrError)    { *loc = stderr; if (pfx) NeSNPRINTF(txt, wrt, maxlen, "%s ", NeLogPrPfx(pr)); }
	else if (pr == NePrCritical) { *loc = stderr; if (pfx) NeSNPRINTF(txt, wrt, maxlen, "%s ", NeLogPrPfx(pr)); }
#if defined(NeLOGCOLORS) && !defined(NeNOLOGCOLORS)
}
#endif
	return wrt;
}

void NeLogLevelSet(NeBy lvl) { loglevel = lvl; }
void NeLogColorState(NeBy cl) { logcolor = cl; }
void NeLogSetDebug(NeBy dbg) { logdbg = dbg; }
void NeLogToggleColor() { logcolor = !logcolor; }
NeU8 NeLogCount() { return logcount; }

NeSz
NeLog(enum NeLogPr pr,
        enum NeLogCl fg, enum NeLogCl bg,
        enum NeLogSt st, enum NeLogFn fn,
        int nl, int pf, const char *const fmt, ...)
{
	va_list lptr;
	NeSz msgw = 0;
	char msg[NeMAXLOG];
	FILE *location = stdout;

	if (pr == NePrLast) pr = NeFmtLast.pr;
	if (fg == NeClLast) fg = NeFmtLast.fg;
	if (bg == NeClLast) bg = NeFmtLast.bg;
	if (st == NeStLast) st = NeFmtLast.st;
	if (fn == NeFnLast) fn = NeFmtLast.fn;

	if ((!fmt || pr > loglevel) && !(pr == NePrDebug && logdbg))
		return 0;

	/* HIDEOUS */
	/* HIDEOUS */
	/* HIDEOUS */
	msgw += getlogprefix(msg + msgw, NeMAXLOG - msgw, pr, &location, pf);

	#if defined(NeLOGCOLORS) && !defined(NeNOLOGCOLORS)
	if (logcolor)
		NeSNPRINTF(msg + msgw, msgw, NeMAXLOG - msgw, "%s", NeTextFormat(fg, bg, st, fn));
	#endif
	va_start(lptr, fmt);
	NeVSNPRINTF(msg + msgw, msgw, NeMAXLOG - msgw, fmt, lptr);
	va_end(lptr);
	#if defined(NeLOGCOLORS) && !defined(NeNOLOGCOLORS)
	if (logcolor)
		NeSNPRINTF(msg + msgw, msgw, NeMAXLOG - msgw, NeClrStr);
	#endif

	#if defined(NeLOGFLUSH) && !defined(NeNOLOGFLUSH)
	if (location != lastloc) // location changed, flush to keep sequential order
		fflush(lastloc);
	lastloc = location;
	#endif
	if (nl)
		fprintf(location, "%s\n", msg);
	else
		fprintf(location, "%s", msg);

	NeFmtLast.pr = pr;
	NeFmtLast.fg = fg;
	NeFmtLast.bg = bg;
	NeFmtLast.st = st;
	NeFmtLast.fn = fn;
	logcount++;
	return msgw;
}

/* HIDEOUS */
const char *NeLogClrStr(enum NeLogCl cl)
{
	switch (cl) {
		case NeClBlack:        return "Black";
		case NeClRed:          return "Red";
		case NeClGreen:        return "Green";
		case NeClYellow:       return "Yellow";
		case NeClBlue:         return "Blue";
		case NeClMagenta:      return "Magenta";
		case NeClCyan:         return "Cyan";
		case NeClWhite:        return "White";
		case NeClNormal:       return "Normal";
		case NeClDarkGrey:     return "Dark Grey";
		case NeClLightRed:     return "Light Red";
		case NeClLightGreen:   return "Light Green";
		case NeClLightYellow:  return "Light Yellow";
		case NeClLightBlue:    return "Light Blue";
		case NeClLightMagenta: return "Light Magenta";
		case NeClLightCyan:    return "Light Cyan";
		case NeClLightWhite:   return "Light White";
		default: return "Invalid";
	}
}
/* HIDEOUS */
const char *NeLogStStr(enum NeLogSt st)
{
	switch (st) {
		case NeStNone:         return "Normal";
		case NeStBold:         return "Bold";
		case NeStDim:          return "Dim";
		case NeStItalics:      return "Italics";
		case NeStUnder:        return "Underline";
		case NeStBlink:        return "Blink";
		case NeStFastBlink:    return "Fast Blink";
		case NeStReverse:      return "Reverse";
		case NeStConceal:      return "Conceal";
		case NeStStrikeout:    return "Strikeout";
		case NeStFraktur:      return "Fraktur";
		case NeStNoBold:       return "No Bold";
		case NeStNoBright:     return "No Bright";
		case NeStNoItalics:    return "No Italics";
		case NeStNoUnder:      return "No Underline";
		case NeStNoBlink:      return "No Blink";
		case NeStNoReverse:    return "No Reverse";
		case NeStReveal:       return "Reveal";
		case NeStNoStrikeout:  return "No Strikeout";
		case NeStFrame:        return "Frame";
		case NeStCircle:       return "Circle";
		case NeStOver:         return "Overline";
		case NeStNoFrame:      return "No Frame";
		case NeStNoOver:       return "No Overline";
		case NeStIUnder:       return "Ideogram Underline";
		case NeStIDoubleUnder: return "Ideogram Double Underline";
		case NeStIOver:        return "Ideogram Overline";
		case NeStIDoubleOver:  return "Ideogram Double Overline";
		case NeStIStress:      return "Ideogram Stress";
		case NeStIOff:         return "Ideogram Off";
		default: return "Invalid";
	}
}
/* HIDEOUS */
const char *NeLogFnStr(enum NeLogFn fn)
{
	switch (fn) {
		case NeFnNormal: return "Normal";
		case NeFn1: return "Alternate 1";
		case NeFn2: return "Alternate 2";
		case NeFn3: return "Alternate 3";
		case NeFn4: return "Alternate 4";
		case NeFn5: return "Alternate 5";
		case NeFn6: return "Alternate 6";
		case NeFn7: return "Alternate 7";
		case NeFn8: return "Alternate 8";
		case NeFn9: return "Alternate 9";
		default: return "Invalid";
	}
}
/* HIDEOUS */
const char *NeLogPrDbgStr(enum NeLogPr pr)
{
	switch (pr) {
		case NePrLast:     return "LST";
		case NePrNone:     return "NON";
		case NePrNormal:   return "NRM";
		case NePrWarning:  return "WRN";
		case NePrError:    return "ERR";
		case NePrCritical: return "CRT";
		case NePrAll:      return "ALL";
		case NePrDebug:    return "DBG";
		default: return "INV";
	}
}
/* HIDEOUS */
const char *NeLogPrPfx(enum NeLogPr pr)
{
	switch (pr) {
		case NePrNormal:   return NeFmtNormal.pfx;
		case NePrWarning:  return NeFmtWarning.pfx;
		case NePrError:    return NeFmtError.pfx;
		case NePrCritical: return NeFmtCritical.pfx;
		case NePrDebug:    return NeFmtDebug.pfx;
		default: return NeFmtClear.pfx;
	}
}
/* HIDEOUS */
struct NeLogFmt NeLogPrFmt(enum NeLogPr pr)
{
	switch (pr) {
		case NePrNormal:   return NeFmtNormal;
		case NePrWarning:  return NeFmtWarning;
		case NePrError:    return NeFmtError;
		case NePrCritical: return NeFmtCritical;
		case NePrDebug:    return NeFmtDebug;
		default: return NeFmtClear;
	}
}
