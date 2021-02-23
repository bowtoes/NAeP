#include "common/NeDebugging.h"

#include <stdarg.h>

#if defined(NeLOGGING) && !defined(NeNOLOGGING)
NeSz
NeLog(enum NeLogPriority priority, int nl, const char *const fmt, ...)
{
	char msg[NeMAXLOG];
	va_list lptr;
	NeSz print = 0;
	FILE *location = stdout;

	if (!fmt)
		return 0;

	switch(priority) {
		case NeDebug:
			location = stderr;
			break;
		case NeNormal:
			break;
		case NeWarning:
			location = stderr;
			print += snprintf(msg + print, NeMAXLOG - print, "Warning: ");
			break;
		case NeError:
			location = stderr;
			print += snprintf(msg + print, NeMAXLOG - print, "Error: ");
			break;
		case NeCritical:
			location = stderr;
			print += snprintf(msg + print, NeMAXLOG - print, "CRITICAL: ");
			break;
		default: fprintf(stderr, "Unknown log priority %i\n", priority);
	};

	va_start(lptr, fmt);
	print += vsnprintf(msg + print, NeMAXLOG - print, fmt, lptr);
	va_end(lptr);

	if (nl)
		fprintf(location, "%s\n", msg);
	else
		fprintf(location, "%s", msg);
	return print;
}

NeSz
NeLogData(enum NeLogPriority priority,
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
		case NeDebug:
			location = stderr;
			break;
		case NeNormal:
			break;
		case NeWarning:
			location = stderr;
			print += snprintf(msg + print, NeMAXLOG - print, "Warning: ");
			break;
		case NeError:
			location = stderr;
			print += snprintf(msg + print, NeMAXLOG - print, "Error: ");
			break;
		case NeCritical:
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
