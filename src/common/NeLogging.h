#ifndef NeLogging_h
#define NeLogging_h

#include <stdio.h>
#include "common/NeTypes.h"

#ifndef NeMAXLOG
#define NeMAXLOG 2048
#endif

enum NeLogColor {
	NeColorDefault = 0,
	NeColorNormal = NeColorDefault,
	NeColorBlack,
	NeColorRed,
	NeColorGreen,
	NeColorYellow,
	NeColorBlue,
	NeColorMagenta,
	NeColorCyan,
	NeColorWhite,
	NeColorDarkGrey,
	NeColorLightRed,
	NeColorLightGreen,
	NeColorLightYellow,
	NeColorLightBlue,
	NeColorLightMagenta,
	NeColorLightCyan,
	NeColorLightWhite,

	NeColorCount
};
const char *NeLogColorStr(enum NeLogColor cl);

enum NeLogStyle {
	NeStyleNone = 0,
	NeStyleNormal = NeStyleNone,
	NeStyleBold,
	NeStyleBright = NeStyleBold,
	NeStyleDim,
	NeStyleItalics,
	NeStyleUnderline,
	NeStyleBlink,
	NeStyleFastBlink,
	NeStyleReverse,
	NeStyleConceal,
	NeStyleStrikeout,
	NeStyleFraktur,
	NeStyleNoBold,
	NeStyleDoubleUnderline = NeStyleNoBold,
	NeStyleNoBright,
	NeStyleNoDim = NeStyleNoBright,
	NeStyleNoItalics,
	NeStyleNoFraktur = NeStyleNoItalics,
	NeStyleNoUnderline,
	NeStyleNoBlink,
	NeStyleNoReverse,
	NeStyleReveal,
	NeStyleNoStrikeout,
	NeStyleFrame,
	NeStyleCircle,
	NeStyleOverline,
	NeStyleNoFrame,
	NeStyleNoCircle = NeStyleNoFrame,
	NeStyleNoOverline,
	NeStyleIdeoUnderline,
	NeStyleIdeoDoubleUnderline,
	NeStyleIdeoOverline,
	NeStyleIdeoDoubleOverline,
	NeStyleIdeoStress,
	NeStyleIdeoOff,

	NeStyleCount
};
const char *NeLogStyleStr(enum NeLogStyle st);

enum NeLogFont {
	NeFontDefault = 0,
	NeFontNormal = NeFontDefault,
	NeFont1,
	NeFont2,
	NeFont3,
	NeFont4,
	NeFont5,
	NeFont6,
	NeFont7,
	NeFont8,
	NeFont9,

	NeFontCount
};
const char *NeLogFontStr(enum NeLogFont fn);

enum NeLogPriority {
	NePriorityNormal,
	NePriorityDebug,
	NePriorityWarning,
	NePriorityError,
	NePriorityCritical,

	NePriorityCount
};
const char *NeLogPriorityStr(enum NeLogPriority pr);

struct NeLogFormat {
	enum NeLogColor foreground;
	enum NeLogColor background;
	enum NeLogStyle style;
	enum NeLogFont font;
};
int NeForegroundIndex(struct NeLogFormat fmt);
int NeBackgroundIndex(struct NeLogFormat fmt);
int NeStyleIndex(struct NeLogFormat fmt);
int NeFontIndex(struct NeLogFormat fmt);

NeSz NeLog(enum NeLogPriority priority,
        enum NeLogColor fg, enum NeLogColor bg,
        enum NeLogStyle style, enum NeLogFont font,
        int nl, const char *const fmt, ...);
#if 0
NeSz NeLogData(enum NeLogPriority priority,
        enum NeLogColor fg, enum NeLogColor bg,
        enum NeLogStyle style, enum NeLogFont font,
        const void *const data, NeSz len, int hx,
        const char *const fmt, ...);
#endif

#if defined(NeLOGGING) && !defined(NeNOLOGGING)
#define NeLOGFM(p, fg, bg, st, fn, nl, ...) NeLog(p, fg, bg, st, fn, nl, __VA_ARGS__)
#else
#define NeLOGFM(p, fg, bg, st, fn, nl, ...)
#endif

#define NeLOGST(p, fg, bg, st, ...) NeLOGFM(p, fg, bg, st, NeFontDefault, __VA_ARGS__)
#define NeLOGBG(p, fg, bg, ...)     NeLOGST(p, fg, bg, NeStyleNone, __VA_ARGS__)
#define NeLOGFG(p, fg, ...)         NeLOGBG(p, fg, NeColorNormal, __VA_ARGS__)
#define NeLOG(p, ...)               NeLOGFG(p, NeColorNormal, __VA_ARGS__)

#define NeNORMALFM(fg, bg, st, fn, ...) NeLOGFM(NePriorityNormal, fg, bg, st, fn, 1, __VA_ARGS__)
#define NeNORMALST(fg, bg, st, ...)     NeLOGST(NePriorityNormal, fg, bg, st, 1, __VA_ARGS__)
#define NeNORMALBG(fg, bg, ...)         NeLOGBG(NePriorityNormal, fg, bg, 1, __VA_ARGS__)
#define NeNORMALFG(fg, ...)             NeLOGFG(NePriorityNormal, fg, 1, __VA_ARGS__)
#define NeNORMAL(...)                   NeLOG(NePriorityNormal, 1, __VA_ARGS__)

#define NeWARNINGFM(fg, bg, st, fn, ...) NeLOGFM(NePriorityWarning, fg, bg, st, fn, 1, __VA_ARGS__)
#define NeWARNINGST(fg, bg, st, ...)     NeLOGST(NePriorityWarning, fg, bg, st, 1, __VA_ARGS__)
#define NeWARNINGBG(fg, bg, ...)         NeLOGBG(NePriorityWarning, fg, bg, 1, __VA_ARGS__)
#define NeWARNINGFG(fg, ...)             NeLOGFG(NePriorityWarning, fg, 1, __VA_ARGS__)
#define NeWARNING(...)                   NeLOG(NePriorityWarning, 1, __VA_ARGS__)

#define NeERRORFM(fg, bg, st, fn, ...) NeLOGFM(NePriorityError, fg, bg, st, fn, 1, __VA_ARGS__)
#define NeERRORST(fg, bg, st, ...)     NeLOGST(NePriorityError, fg, bg, st, 1, __VA_ARGS__)
#define NeERRORBG(fg, bg, ...)         NeLOGBG(NePriorityError, fg, bg, 1, __VA_ARGS__)
#define NeERRORFG(fg, ...)             NeLOGFG(NePriorityError, fg, 1, __VA_ARGS__)
#define NeERROR(...)                   NeLOG(NePriorityError, 1, __VA_ARGS__)

#define NeCRITICALFM(fg, bg, st, fn, ...) NeLOGFM(NePriorityCritical, fg, bg, st, fn, 1, __VA_ARGS__)
#define NeCRITICALST(fg, bg, st, ...)     NeLOGST(NePriorityCritical, fg, bg, st, 1, __VA_ARGS__)
#define NeCRITICALBG(fg, bg, ...)         NeLOGBG(NePriorityCritical, fg, bg, 1, __VA_ARGS__)
#define NeCRITICALFG(fg, ...)             NeLOGFG(NePriorityCritical, fg, 1, __VA_ARGS__)
#define NeCRITICAL(...)                   NeLOG(NePriorityCritical, 1, __VA_ARGS__)

#define NeNORMALNFM(fg, bg, st, fn, ...) NeLOGFM(NePriorityNormal, fg, bg, st, fn, 0, __VA_ARGS__)
#define NeNORMALNST(fg, bg, st, ...)     NeLOGST(NePriorityNormal, fg, bg, st, 0, __VA_ARGS__)
#define NeNORMALNBG(fg, bg, ...)         NeLOGBG(NePriorityNormal, fg, bg, 0, __VA_ARGS__)
#define NeNORMALNFG(fg, ...)             NeLOGFG(NePriorityNormal, fg, 0, __VA_ARGS__)
#define NeNORMALN(...)                   NeLOG(NePriorityNormal, 0, __VA_ARGS__)

#define NeWARNINGNFM(fg, bg, st, fn, ...) NeLOGFM(NePriorityWarning, fg, bg, st, fn, 0, __VA_ARGS__)
#define NeWARNINGNST(fg, bg, st, ...)     NeLOGST(NePriorityWarning, fg, bg, st, 0, __VA_ARGS__)
#define NeWARNINGNBG(fg, bg, ...)         NeLOGBG(NePriorityWarning, fg, bg, 0, __VA_ARGS__)
#define NeWARNINGNFG(fg, ...)             NeLOGFG(NePriorityWarning, fg, 0, __VA_ARGS__)
#define NeWARNINGN(...)                   NeLOG(NePriorityWarning, 0, __VA_ARGS__)

#define NeERRORNFM(fg, bg, st, fn, ...) NeLOGFM(NePriorityError, fg, bg, st, fn, 0, __VA_ARGS__)
#define NeERRORNST(fg, bg, st, ...)     NeLOGST(NePriorityError, fg, bg, st, 0, __VA_ARGS__)
#define NeERRORNBG(fg, bg, ...)         NeLOGBG(NePriorityError, fg, bg, 0, __VA_ARGS__)
#define NeERRORNFG(fg, ...)             NeLOGFG(NePriorityError, fg, 0, __VA_ARGS__)
#define NeERRORN(...)                   NeLOG(NePriorityError, 0, __VA_ARGS__)

#define NeCRITICALNFM(fg, bg, st, fn, ...) NeLOGFM(NePriorityCritical, fg, bg, st, fn, 0, __VA_ARGS__)
#define NeCRITICALNST(fg, bg, st, ...)     NeLOGST(NePriorityCritical, fg, bg, st, 0, __VA_ARGS__)
#define NeCRITICALNBG(fg, bg, ...)         NeLOGBG(NePriorityCritical, fg, bg, 0, __VA_ARGS__)
#define NeCRITICALNFG(fg, ...)             NeLOGFG(NePriorityCritical, fg, 0, __VA_ARGS__)
#define NeCRITICALN(...)                   NeLOG(NePriorityCritical, 0, __VA_ARGS__)

#if defined(NeDEBUGGING) && !defined(NeNODEBUGGING)
#define NeDEBUGFM(fg, bg, st, fn, ...) NeLOGFM(NePriorityDebug, fg, bg, st, fn, 1, __VA_ARGS__)
#define NeDEBUGST(fg, bg, st, ...)     NeLOGST(NePriorityDebug, fg, bg, st, 1, __VA_ARGS__)
#define NeDEBUGBG(fg, bg, ...)         NeLOGBG(NePriorityDebug, fg, bg, 1, __VA_ARGS__)
#define NeDEBUGFG(fg, ...)             NeLOGFG(NePriorityDebug, fg, 1, __VA_ARGS__)
#define NeDEBUG(...)                   NeLOG(NePriorityDebug, 1, __VA_ARGS__)

#define NeDEBUGNFM(fg, bg, st, fn, ...) NeLOGFM(NePriorityDebug, fg, bg, st, fn, 0, __VA_ARGS__)
#define NeDEBUGNST(fg, bg, st, ...)     NeLOGST(NePriorityDebug, fg, bg, st, 0, __VA_ARGS__)
#define NeDEBUGNBG(fg, bg, ...)         NeLOGBG(NePriorityDebug, fg, bg, 0, __VA_ARGS__)
#define NeDEBUGNFG(fg, ...)             NeLOGFG(NePriorityDebug, fg, 0, __VA_ARGS__)
#define NeDEBUGN(...)                   NeLOG(NePriorityDebug, 0, __VA_ARGS__)
#else
#define NeDEBUGFM(fg, bg, st, fn, ...)
#define NeDEBUGST(fg, bg, st, ...)
#define NeDEBUGBG(fg, bg, ...)
#define NeDEBUGFG(fg, ...)
#define NeDEBUG(...)

#define NeDEBUGNFM(fg, bg, st, fn, ...)
#define NeDEBUGNST(fg, bg, st, ...)
#define NeDEBUGNBG(fg, bg, ...)
#define NeDEBUGNFG(fg, ...)
#define NeDEBUGN(...)
#endif

#endif /* NeLogging_h */
