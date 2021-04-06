#ifndef NeLogging_h
#define NeLogging_h

#include <stdio.h>

#include "common/NeTypes.h"
#include "common/NePlatform.h"

#ifndef NeMAXLOG
#define NeMAXLOG 2048
#endif

enum NeLogCl {
	NeClLast = -1,
	NeClNormal,

	NeClBlack,      NeClRed,            NeClGreen,      NeClYellow,
	NeClBlue,       NeClMagenta,        NeClCyan,       NeClWhite,
	NeClDarkGrey,   NeClLightRed,       NeClLightGreen, NeClLightYellow,
	NeClLightBlue,  NeClLightMagenta,   NeClLightCyan,  NeClLightWhite,

	NeClCount,
};

enum NeLogFn {
	NeFnLast = -1,
	NeFnNormal,

	NeFn1, NeFn2, NeFn3, NeFn4, NeFn5, NeFn6, NeFn7, NeFn8, NeFn9,

	NeFnCount,
};

enum NeLogSt {
	NeStLast = -1,
	NeStNone,       NeStBold,           NeStDim,            NeStItalics,
	NeStUnder,      NeStBlink,          NeStFastBlink,      NeStReverse,
	NeStConceal,    NeStStrikeout,      NeStFraktur,        NeStNoBold,
	NeStNoBright,   NeStNoItalics,      NeStNoUnder,        NeStNoBlink,
	NeStNoReverse,  NeStReveal,         NeStNoStrikeout,    NeStFrame,
	NeStCircle,     NeStOver,           NeStNoFrame,        NeStNoOver,
	NeStIUnder,     NeStIDoubleUnder,   NeStIOver,          NeStIDoubleOver,
	NeStIStress,    NeStIOff,

	NeStCount,

	NeStNormal = NeStNone,
	NeStBright = NeStBold,
	NeStDoubleUnder = NeStNoBold,
	NeStNoDim = NeStNoBright,
	NeStNoFraktur = NeStNoItalics,
	NeStNoCircle = NeStNoFrame,
};

enum NeLogPr {
	NePrLast = -1,
	NePrNone,     /* ll 0 */
	NePrCritical, /* ll 1 */
	NePrError,    /* ll 2 */
	NePrNormal,   /* ll 3 */
	NePrWarning,  /* ll 4 */

	NePrCount,
	NePrDebug,    /* ll 6 */
	NePrAll = NePrCount,
	NePrFull = NePrDebug,
};

struct NeLogFmt {
	enum NeLogPr pr;
	enum NeLogCl fg;
	enum NeLogCl bg;
	enum NeLogSt st;
	enum NeLogFn fn;
	const char *const pfx;
};

extern const struct NeLogFmt NeFmtNormal;
extern const struct NeLogFmt NeFmtWarning;
extern const struct NeLogFmt NeFmtError;
extern const struct NeLogFmt NeFmtCritical;
extern const struct NeLogFmt NeFmtDebug;

extern const struct NeLogFmt NeFmtClear;
extern       struct NeLogFmt NeFmtLast;

void NeLogLevelSet(NeBy lvl);
void NeLogColorState(NeBy cl);
void NeLogToggleColor();
NeSz NeLogCount();

NeSz NeLog(enum NeLogPr pr,
        enum NeLogCl fg, enum NeLogCl bg,
        enum NeLogSt st, enum NeLogFn fn,
        int nl, int pfx, const char *const fmt, ...);

const char *NeLogClrStr(enum NeLogCl cl);
const char *NeLogStStr(enum NeLogSt st);
const char *NeLogFnStr(enum NeLogFn fn);
const char *NeLogPrDbgStr(enum NeLogPr pr);
const char *NeLogPrPfx(enum NeLogPr pr);
struct NeLogFmt NeLogPrFmt(enum NeLogPr pr);

#define NeClrStr "\x1b[000;039;049;010m"
extern char NeFmtStr[];
#if defined(NeLOGCOLORS) && !defined(NeNOLOGCOLORS)
const char *NeTextFormat(enum NeLogCl fg, enum NeLogCl bg, enum NeLogSt st, enum NeLogFn fn);
#else
#define NeTextFormat(fg, bg, st, fn) ""
#endif

#define NeStyleFn(fg, bg, st, fn) NeTextFormat(fg,       bg,       st,       fn)
#define NeStyleSt(fg, bg, st)     NeTextFormat(fg,       bg,       st, NeFnLast)
#define NeStyleBg(fg, bg)         NeTextFormat(fg,       bg, NeStLast, NeFnLast)
#define NeStyleFg(fg)             NeTextFormat(fg, NeClLast, NeStLast, NeFnLast)

#if defined(NeLOGGING) && !defined(NeNOLOGGING)
#define NeLOGFM(p, fg, bg, st, fn, nl, pf, ...) NeLog(p, fg, bg, st, fn, nl, pf, __VA_ARGS__)
#else
#define NeLOGFM(p, fg, bg, st, fn, nl, pf, ...)
#endif

#define NeLOGST(p, fg, bg, st, ...) NeLOGFM(p, fg, bg, st, NeFnNormal, __VA_ARGS__)
#define NeLOGBG(p, fg, bg, ...)     NeLOGST(p,     fg, bg, NeStNormal, __VA_ARGS__)
#define NeLOGFG(p, fg, ...)         NeLOGBG(p,         fg, NeClNormal, __VA_ARGS__)
#define NeLOG(p, ...)               NeLOGFG(p,             NeClNormal, __VA_ARGS__)

/* default */
#define NeNORMALFM(fg, bg, st, fn, ...) NeLOGFM(NePrNormal, fg, bg, st, fn, 1, 1, __VA_ARGS__)
#define NeNORMALST(fg, bg, st, ...)     NeLOGST(NePrNormal,     fg, bg, st, 1, 1, __VA_ARGS__)
#define NeNORMALBG(fg, bg, ...)         NeLOGBG(NePrNormal,         fg, bg, 1, 1, __VA_ARGS__)
#define NeNORMALFG(fg, ...)             NeLOGFG(NePrNormal,             fg, 1, 1, __VA_ARGS__)
#define NeNORMAL(...)                     NeLOG(NePrNormal,                 1, 1, __VA_ARGS__)

#define NeWARNINGFM(fg, bg, st, fn, ...) NeLOGFM(NePrWarning, fg, bg, st, fn, 1, 1, __VA_ARGS__)
#define NeWARNINGST(fg, bg, st, ...)     NeLOGST(NePrWarning,     fg, bg, st, 1, 1, __VA_ARGS__)
#define NeWARNINGBG(fg, bg, ...)         NeLOGBG(NePrWarning,         fg, bg, 1, 1, __VA_ARGS__)
#define NeWARNINGFG(fg, ...)             NeLOGFG(NePrWarning,             fg, 1, 1, __VA_ARGS__)
#define NeWARNING(...)                     NeLOG(NePrWarning,                 1, 1, __VA_ARGS__)

#define NeERRORFM(fg, bg, st, fn, ...) NeLOGFM(NePrError, fg, bg, st, fn, 1, 1, __VA_ARGS__)
#define NeERRORST(fg, bg, st, ...)     NeLOGST(NePrError,     fg, bg, st, 1, 1, __VA_ARGS__)
#define NeERRORBG(fg, bg, ...)         NeLOGBG(NePrError,         fg, bg, 1, 1, __VA_ARGS__)
#define NeERRORFG(fg, ...)             NeLOGFG(NePrError,             fg, 1, 1, __VA_ARGS__)
#define NeERROR(...)                     NeLOG(NePrError,                 1, 1, __VA_ARGS__)

#define NeCRITICALFM(fg, bg, st, fn, ...) NeLOGFM(NePrCritical, fg, bg, st, fn, 1, 1, __VA_ARGS__)
#define NeCRITICALST(fg, bg, st, ...)     NeLOGST(NePrCritical,     fg, bg, st, 1, 1, __VA_ARGS__)
#define NeCRITICALBG(fg, bg, ...)         NeLOGBG(NePrCritical,         fg, bg, 1, 1, __VA_ARGS__)
#define NeCRITICALFG(fg, ...)             NeLOGFG(NePrCritical,             fg, 1, 1, __VA_ARGS__)
#define NeCRITICAL(...)                     NeLOG(NePrCritical,                 1, 1, __VA_ARGS__)

#define NeDEBUGFM(fg, bg, st, fn, ...) NeLOGFM(NePrDebug, fg, bg, st, fn, 1, 1, __VA_ARGS__)
#define NeDEBUGST(fg, bg, st, ...)     NeLOGST(NePrDebug,     fg, bg, st, 1, 1, __VA_ARGS__)
#define NeDEBUGBG(fg, bg, ...)         NeLOGBG(NePrDebug,         fg, bg, 1, 1, __VA_ARGS__)
#define NeDEBUGFG(fg, ...)             NeLOGFG(NePrDebug,             fg, 1, 1, __VA_ARGS__)
#define NeDEBUG(...)                     NeLOG(NePrDebug,                 1, 1, __VA_ARGS__)

/* no newline */
#define NeNORMALNFM(fg, bg, st, fn, ...) NeLOGFM(NePrNormal, fg, bg, st, fn, 0, 1, __VA_ARGS__)
#define NeNORMALNST(fg, bg, st, ...)     NeLOGST(NePrNormal,     fg, bg, st, 0, 1, __VA_ARGS__)
#define NeNORMALNBG(fg, bg, ...)         NeLOGBG(NePrNormal,         fg, bg, 0, 1, __VA_ARGS__)
#define NeNORMALNFG(fg, ...)             NeLOGFG(NePrNormal,             fg, 0, 1, __VA_ARGS__)
#define NeNORMALN(...)                     NeLOG(NePrNormal,                 0, 1, __VA_ARGS__)

#define NeWARNINGNFM(fg, bg, st, fn, ...) NeLOGFM(NePrWarning, fg, bg, st, fn, 0, 1, __VA_ARGS__)
#define NeWARNINGNST(fg, bg, st, ...)     NeLOGST(NePrWarning,     fg, bg, st, 0, 1, __VA_ARGS__)
#define NeWARNINGNBG(fg, bg, ...)         NeLOGBG(NePrWarning,         fg, bg, 0, 1, __VA_ARGS__)
#define NeWARNINGNFG(fg, ...)             NeLOGFG(NePrWarning,             fg, 0, 1, __VA_ARGS__)
#define NeWARNINGN(...)                     NeLOG(NePrWarning,                 0, 1, __VA_ARGS__)

#define NeERRORNFM(fg, bg, st, fn, ...) NeLOGFM(NePrError, fg, bg, st, fn, 0, 1, __VA_ARGS__)
#define NeERRORNST(fg, bg, st, ...)     NeLOGST(NePrError,     fg, bg, st, 0, 1, __VA_ARGS__)
#define NeERRORNBG(fg, bg, ...)         NeLOGBG(NePrError,         fg, bg, 0, 1, __VA_ARGS__)
#define NeERRORNFG(fg, ...)             NeLOGFG(NePrError,             fg, 0, 1, __VA_ARGS__)
#define NeERRORN(...)                     NeLOG(NePrError,                 0, 1, __VA_ARGS__)

#define NeCRITICALNFM(fg, bg, st, fn, ...) NeLOGFM(NePrCritical, fg, bg, st, fn, 0, 1, __VA_ARGS__)
#define NeCRITICALNST(fg, bg, st, ...)     NeLOGST(NePrCritical,     fg, bg, st, 0, 1, __VA_ARGS__)
#define NeCRITICALNBG(fg, bg, ...)         NeLOGBG(NePrCritical,         fg, bg, 0, 1, __VA_ARGS__)
#define NeCRITICALNFG(fg, ...)             NeLOGFG(NePrCritical,             fg, 0, 1, __VA_ARGS__)
#define NeCRITICALN(...)                     NeLOG(NePrCritical,                 0, 1, __VA_ARGS__)

#define NeDEBUGNFM(fg, bg, st, fn, ...) NeLOGFM(NePrDebug, fg, bg, st, fn, 0, 1, __VA_ARGS__)
#define NeDEBUGNST(fg, bg, st, ...)     NeLOGST(NePrDebug,     fg, bg, st, 0, 1, __VA_ARGS__)
#define NeDEBUGNBG(fg, bg, ...)         NeLOGBG(NePrDebug,         fg, bg, 0, 1, __VA_ARGS__)
#define NeDEBUGNFG(fg, ...)             NeLOGFG(NePrDebug,             fg, 0, 1, __VA_ARGS__)
#define NeDEBUGN(...)                     NeLOG(NePrDebug,                 0, 1, __VA_ARGS__)

/* no prefix */
#define NePREFIXFM(p, fg, bg, st, fn, ...)  NeLOGFM(p, fg, bg, st, fn, 1, 0, __VA_ARGS__)
#define NePREFIXST(p, fg, bg, st, ...)      NeLOGST(p,     fg, bg, st, 1, 0, __VA_ARGS__)
#define NePREFIXBG(p, fg, bg, ...)          NeLOGBG(p,         fg, bg, 1, 0, __VA_ARGS__)
#define NePREFIXFG(p, fg, ...)              NeLOGFG(p,             fg, 1, 0, __VA_ARGS__)
#define NePREFIX(p, ...)                      NeLOG(p,                 1, 0, __VA_ARGS__)

/* no prefix or newline */
#define NePREFIXNFM(p, fg, bg, st, fn, ...) NeLOGFM(p, fg, bg, st, fn, 0, 0, __VA_ARGS__)
#define NePREFIXNST(p, fg, bg, st, ...)     NeLOGST(p,     fg, bg, st, 0, 0, __VA_ARGS__)
#define NePREFIXNBG(p, fg, bg, ...)         NeLOGBG(p,         fg, bg, 0, 0, __VA_ARGS__)
#define NePREFIXNFG(p, fg, ...)             NeLOGFG(p,             fg, 0, 0, __VA_ARGS__)
#define NePREFIXN(p, ...)                     NeLOG(p,                 0, 0, __VA_ARGS__)

#endif /* NeLogging_h */
