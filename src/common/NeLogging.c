#include "common/NeLogging.h"

#include <stdarg.h>

/* https://stackoverflow.com/questions/4842424/list-of-ansi-color-escape-sequences */
const char *NeLogColorStr(enum NeLogColor cl)
{
	switch (cl) {
		case NeColorBlack: return "Black";
		case NeColorRed: return "Red";
		case NeColorGreen: return "Green";
		case NeColorYellow: return "Yellow";
		case NeColorBlue: return "Blue";
		case NeColorMagenta: return "Magenta";
		case NeColorCyan: return "Cyan";
		case NeColorWhite: return "White";
		case NeColorNormal: return "Normal";
		case NeColorDarkGrey: return "Dark Grey";
		case NeColorLightRed: return "Light Red";
		case NeColorLightGreen: return "Light Green";
		case NeColorLightYellow: return "Light Yellow";
		case NeColorLightBlue: return "Light Blue";
		case NeColorLightMagenta: return "Light Magenta";
		case NeColorLightCyan: return "Light Cyan";
		case NeColorLightWhite: return "Light White";
		default: return "Invalid";
	}
}
const char *NeLogStyleStr(enum NeLogStyle st)
{
	switch (st) {
		case NeStyleNormal: return "Normal";
		case NeStyleBold: return "Bold";
		case NeStyleDim: return "Dim";
		case NeStyleItalics: return "Italics";
		case NeStyleUnderline: return "Underline";
		case NeStyleBlink: return "Blink";
		case NeStyleFastBlink: return "Fast Blink";
		case NeStyleReverse: return "Reverse";
		case NeStyleConceal: return "Conceal";
		case NeStyleStrikeout: return "Strikeout";
		case NeStyleFraktur: return "Fraktur";
		case NeStyleNoBold: return "No Bold";
		case NeStyleNoBright: return "No Bright";
		case NeStyleNoItalics: return "No Italics";
		case NeStyleNoUnderline: return "No Underline";
		case NeStyleNoBlink: return "No Blink";
		case NeStyleNoReverse: return "No Reverse";
		case NeStyleReveal: return "Reveal";
		case NeStyleNoStrikeout: return "No Strikeout";
		case NeStyleFrame: return "Frame";
		case NeStyleCircle: return "Circle";
		case NeStyleOverline: return "Overline";
		case NeStyleNoFrame: return "No Frame";
		case NeStyleNoOverline: return "No Overline";
		case NeStyleIdeoUnderline: return "Ideogram Underline";
		case NeStyleIdeoDoubleUnderline: return "Ideogram Double Underline";
		case NeStyleIdeoOverline: return "Ideogram Overline";
		case NeStyleIdeoDoubleOverline: return "Ideogram Double Overline";
		case NeStyleIdeoStress: return "Ideogram Stress";
		case NeStyleIdeoOff: return "Ideogram Off";
		default: return "Invalid";
	}
}
const char *NeLogFontStr(enum NeLogFont fn)
{
	switch (fn) {
		case NeFontNormal: return "Normal";
		case NeFont1: return "Alternate 1";
		case NeFont2: return "Alternate 2";
		case NeFont3: return "Alternate 3";
		case NeFont4: return "Alternate 4";
		case NeFont5: return "Alternate 5";
		case NeFont6: return "Alternate 6";
		case NeFont7: return "Alternate 7";
		case NeFont8: return "Alternate 8";
		case NeFont9: return "Alternate 9";
		default: return "Invalid";
	}
}
const char *NeLogPriorityStr(enum NeLogPriority pr)
{
	switch (pr) {
		case NePriorityDebug: return "Debug";
		case NePriorityNormal: return "Normal";
		case NePriorityWarning: return "Warning";
		case NePriorityError: return "Error";
		case NePriorityCritical: return "Critical";
		default: return "Invalid";
	}
}

static const struct NeLogFormat    debug = {NeColorYellow,          0,             0, 0};
static const struct NeLogFormat   normal = {            0,          0,             0, 0};
static const struct NeLogFormat  warning = {NeColorYellow,          0, NeStyleBright, 0};
static const struct NeLogFormat    error = {   NeColorRed,          0,             0, 0};
static const struct NeLogFormat critical = { NeColorBlack, NeColorRed, NeStyleBright, 0};
static const struct NeLogFormat    clear = {            0,          0,             0, 0};

int NeForegroundIndex(struct NeLogFormat fmt) {
	switch (fmt.foreground) {
		case NeColorBlack: return 30;
		case NeColorRed: return 31;
		case NeColorGreen: return 32;
		case NeColorYellow: return 33;
		case NeColorBlue: return 34;
		case NeColorMagenta: return 35;
		case NeColorCyan: return 36;
		case NeColorWhite: return 37;
		case NeColorNormal: return 39;
		case NeColorDarkGrey: return 90;
		case NeColorLightRed: return 91;
		case NeColorLightGreen: return 92;
		case NeColorLightYellow: return 93;
		case NeColorLightBlue: return 94;
		case NeColorLightMagenta: return 95;
		case NeColorLightCyan: return 96;
		case NeColorLightWhite: return 97;
		default: return 39;
	}
}
int NeBackgroundIndex(struct NeLogFormat fmt) {
	struct NeLogFormat t = {fmt.background, 0, 0, 0};
	return NeForegroundIndex(t) + 10;
}
int NeStyleIndex(struct NeLogFormat fmt) {
	switch (fmt.style) {
		case NeStyleNormal: return 0;
		case NeStyleBold: return 1;
		case NeStyleDim: return 2;
		case NeStyleItalics: return 3;
		case NeStyleUnderline: return 4;
		case NeStyleBlink: return 5;
		case NeStyleFastBlink: return 6;
		case NeStyleReverse: return 7;
		case NeStyleConceal: return 8;
		case NeStyleStrikeout: return 9;
		case NeStyleFraktur: return 20;
		case NeStyleNoBold: return 21;
		case NeStyleNoBright: return 22;
		case NeStyleNoItalics: return 23;
		case NeStyleNoUnderline: return 24;
		case NeStyleNoBlink: return 25;
		case NeStyleNoReverse: return 27;
		case NeStyleReveal: return 28;
		case NeStyleNoStrikeout: return 29;
		case NeStyleFrame: return 51;
		case NeStyleCircle: return 52;
		case NeStyleOverline: return 53;
		case NeStyleNoFrame: return 54;
		case NeStyleNoOverline: return 55;
		case NeStyleIdeoUnderline: return 60;
		case NeStyleIdeoDoubleUnderline: return 61;
		case NeStyleIdeoOverline: return 62;
		case NeStyleIdeoDoubleOverline: return 63;
		case NeStyleIdeoStress: return 64;
		case NeStyleIdeoOff: return 65;
		default: return 0;
	}
}
int NeFontIndex(struct NeLogFormat fmt) {
	switch (fmt.font) {
		case NeFontNormal: return 10;
		case NeFont1: return 11;
		case NeFont2: return 12;
		case NeFont3: return 13;
		case NeFont4: return 14;
		case NeFont5: return 15;
		case NeFont6: return 16;
		case NeFont7: return 17;
		case NeFont8: return 18;
		case NeFont9: return 19;
		default: return 10;
	}
}
#if defined(NePLATFORMTYPE_WINDOWS)
#define clrstr ""
#define fmtstr ""
#define updatefmtstr(...) ""
#else
static const char *const clrstr = "\x1b[000;039;049;010m";
static char fmtstr[] = "\x1b[000;039;049;010m";
static const char *updatefmtstr(struct NeLogFormat format) {
	return "";
	snprintf(fmtstr, 19, "\x1b[%03i;%03i;%03i;%03im",
			NeStyleIndex(format),
			NeForegroundIndex(format),
			NeBackgroundIndex(format),
			NeFontIndex(format));
	return fmtstr;
#endif
}

#define NeSNPRINTF(txt, pos, len, ...) {\
	NeOf tmp = snprintf((txt), (len), __VA_ARGS__);\
	if (tmp > 0) pos += tmp < (len) ? tmp : (len) - 1;\
}
#define NeVSNPRINTF(txt, pos, len, fmt, lptr) {\
	NeOf tmp = vsnprintf((txt), (len), (fmt), lptr);\
	if (tmp > 0) pos += tmp < (len) ? tmp : (len) - 1;\
}
static NeSz getlogprefix(enum NeLogPriority pri, char *txt, NeSz maxlen, FILE **loc) {
	NeSz wrt = 0;
	switch (pri) {
		case NePriorityDebug:
			NeSNPRINTF(txt, wrt, maxlen, "%s[Debug]", updatefmtstr(debug));
			*loc = stderr;
			break;
		case NePriorityWarning:
			NeSNPRINTF(txt, wrt, maxlen, "%s[Warning]", updatefmtstr(warning));
			*loc = stderr;
			break;
		case NePriorityError:
			NeSNPRINTF(txt, wrt, maxlen, "%s[Error]", updatefmtstr(error));
			*loc = stderr;
			break;
		case NePriorityCritical:
			NeSNPRINTF(txt, wrt, maxlen, "%s[CRITICAL]", updatefmtstr(critical));
			*loc = stderr;
			break;
		default:
			*loc = stdout;
			break;
	}
	NeSNPRINTF(txt + wrt, wrt, maxlen - wrt, "%s", clrstr);
	return wrt;
}

NeSz
NeLog(enum NeLogPriority priority,
        enum NeLogColor fg, enum NeLogColor bg,
        enum NeLogStyle style, enum NeLogFont font,
        int nl, const char *const fmt, ...)
{
	NeSz wrt = 0;
	va_list lptr;
	char msg[NeMAXLOG];
	FILE *location;

	if (!fmt)
		return 0;

	wrt += getlogprefix(priority, msg, NeMAXLOG, &location);
	NeSNPRINTF(msg + wrt, wrt, NeMAXLOG - wrt, "%s", updatefmtstr((struct NeLogFormat){fg, bg, style, font}));
	va_start(lptr, fmt);
	NeVSNPRINTF(msg + wrt, wrt, NeMAXLOG - wrt, fmt, lptr);
	va_end(lptr);
	NeSNPRINTF(msg + wrt, wrt, NeMAXLOG - wrt, "%s", clrstr);

	if (nl)
		fprintf(location, "%s\n", msg);
	else
		fprintf(location, "%s", msg);
	return wrt;
}

#if 0
NeSz
NeLogData(enum NeLogPriority priority,
        enum NeLogColor fg, enum NeLogColor bg,
        enum NeLogStyle style, enum NeLogFont font,
        const void *const data, NeSz len, int hx,
        const char *const fmt, ...)
{
	char msg[NeMAXLOG];
	NeSz print = 0;
	NeBy *d = (NeBy *)data;
	va_list lptr;
	FILE *location = stdout;

	if (!data || !len)
		return 0;

	switch(priority) {
		case NePriorityDebug:
			location = stderr;
			break;
		case NePriorityNormal:
			break;
		case NePriorityWarning:
			location = stderr;
			print += snprintf(msg + print, NeMAXLOG - print, "Warning: ");
			break;
		case NePriorityError:
			location = stderr;
			print += snprintf(msg + print, NeMAXLOG - print, "Error: ");
			break;
		case NePriorityCritical:
			location = stderr;
			print += snprintf(msg + print, NeMAXLOG - print, "CRITICAL: ");
			break;
		default: fprintf(stderr, "Unknown log priority %i\n", priority);
	};

	if (fmt) {
		va_start(lptr, fmt);
		print += vsnprintf(msg + print, NeMAXLOG - print, fmt, lptr);
		va_end(lptr);
	}

	for (NeSz i = 0; i < len - 1; ++i) {
		if (hx) {
			print += snprintf(msg + print, NeMAXLOG - print, "%02x ", d[i]);
		} else {
			print += snprintf(msg + print, NeMAXLOG - print, "%c", d[i]);
		}
	}
	if (hx) {
		print += snprintf(msg + print, NeMAXLOG - print, "%02x", d[len - 1]);
	} else {
		print += snprintf(msg + print, NeMAXLOG - print, "%c", d[len - 1]);
	}

	if (fmt)
		fprintf(location, "%s", msg);
	else
		fprintf(location, "%s\n", msg);
	return print;
}
#endif
