#include "nelog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <brrtools/brrmacro.h>

#define ERROR_BUFFER_SIZE 4096
static char error_buffer[ERROR_BUFFER_SIZE] = {0};

#define USAGE "Usage: NAeP [[OPTION ...] FILE ...] ..." \
"\nNAeP - NieR:Automated extraction Precept_v"Ne_version"" \
"\nCompiled on "__DATE__", " __TIME__"\n"
#define HELP \
"Most options take affect on all files following and can be toggled." \
"\nSome options are global, and apply to the meta-process itself, marked with (g)." \
"\nOptions:" \
"\n        -h, -help, -v . . . . . . . . . . .  Print this help." \
"\n    File Type Specification:" \
"\n        -a, -auto, -detect  . . . . . . . .  Autodetect filetype from file header or extension." \
"\n        -w, -wem, -weem . . . . . . . . . .  File(s) are single WwRIFFs to be converted to Ogg." \
"\n        -W, -wsp, -wisp . . . . . . . . . .  File(s) are collections of WwRIFFs to be extracted/converted." \
"\n        -b, -bnk, -bank . . . . . . . . . .  The same as '-wsp'." \
"\n        -o, -ogg  . . . . . . . . . . . . .  File(s) are Ogg files to be regranularizeed." \
"\n    OGG Processing Options:" \
"\n        -ri, -rgrn-inplace, -rvb-inplace. .  Oggs are regranularized in-place." \
"\n    WEM Processing Options:" \
"\n        -oi, -ogg-inplace . . . . . . . . .  All WwRIFF-to-Ogg conversion is replaces input files." \
"\n        -cbl, -codebook-library . . . . . .  The following file is the codebook library to use for the" \
"\n                                             WwRIFFs following." \
"\n                                             If no coedbooks are specified for a WwRIFF, then it is assumed" \
"\n                                             the codebooks are inline." \
"\n        -inline . . . . . . . . . . . . . .  The following WwRIFFs have inline codebooks." \
"\n        -stripped . . . . . . . . . . . . .  The following WwRIFFs' are stripped and must be rebuilt from" \
"\n                                             a codebook library." \
"\n    WSP/BNK Processing Options:" \
"\n        -w2o, -wem2ogg  . . . . . . . . . .  Convert WwRIFFs from '-wsp' files to Oggs, rather than extracting them." \
"\n        -white, -weiss," \
"\n        -black, -noir . . . . . . . . . . .  Comma-separated list of indices used to determine" \
"\n                                             what indices to process from the file(s)." \
"\n                                             Blacklists and whitelists are mutually exclusive; each " \
"\n                                             overrides the others preceding it." \
"\n                                             E.g. '-black 3,9' would process every index except 3 and 9," \
"\n                                             but  '-white 3,9' would process only indices 3 and 9." \
"\n        -rubrum . . . . . . . . . . . . . .  Toggle the list to between being a whitelist/blacklist." \
"\n    Miscellaneous options:" \
"\n        -!  . . . . . . . . . . . . . . . .  The following argument is a file path, not an option." \
"\n        --  . . . . . . . . . . . . . . . .  All following arguments are file paths, not options." \
"\n        -d, -debug  . . . . . . . . . . . .  Enable debug output, irrespective of quiet settings." \
"\n        -co, -comments  . . . . . . . . . .  Toggles inserting of additional comments in output Oggs." \
"\n        -c, -color  . . . . . . . . . . . .  Toggle color logging." \
"\n        -C, -global-color (g) . . . . . . .  Toggle whether log styling is enabled at all." \
"\n        -r, -report-card (g)  . . . . . . .  Print a status report of all files processed after processing." \
"\n        +r, -full-report (g)  . . . . . . .  Print a more full report of all files processed." \
"\n        -q, -quiet  . . . . . . . . . . . .  Suppress one additional level of non-critical." \
"\n        +q, +quiet  . . . . . . . . . . . .  Show one additional level non-critical output." \
"\n        -Q, -qq, -too-quiet . . . . . . . .  Suppress all output, including anything critical." \
"\n        -n, -dry, -dry-run  . . . . . . . .  Don't actually do anything, just log what would happen." \
"\n        -reset (g)  . . . . . . . . . . . .  Argument options reset to default values after each file passed." \

int /* Returns non-void so that 'return print_usage()' is valid */
print_usage(void)
{
	fprintf(stdout, USAGE"\n""    -h, -help, -v . . . . . . . . . . .  Print help.""\n");
	exit(0);
	return 0;
}
int /* Returns non-void so that 'return print_help()' is valid */
print_help(void)
{
	fprintf(stdout, USAGE"\n"HELP"\n");
	exit(0);
	return 0;
}


char nemessage[nemessage_len + 1] = {0};

int
nelog_init(int style_enabled)
{
	#define _brrlog_try_pri(_lbl_, _style_, _pfx_, _dst_, _type_) do {\
		if (brrlog_priority_mod(_lbl_, (brrlog_priority_t){ .style=_style_, .pfx=_pfx_, .dst={ .dst=_dst_, .type=_type_, } })) {\
			int e = brrapi_error_code();\
			if (e == BRRAPI_E_LIBC) {\
				fprintf(stderr, "Failed to initilaize brrlog priority %i '%s': %s (%i)\n", _lbl_, _pfx_?_pfx_:"", strerror(errno), errno);\
			} else {\
				fprintf(stderr, "Failed to initilaize brrlog priority %i '%s': %s (%i)\n", _lbl_, _pfx_?_pfx_:"", brrapi_error_message(error_buffer, ERROR_BUFFER_SIZE), e);\
			}\
			brrlog_deinit();\
			return 1;\
		}\
	} while (0)

	{
		brrlog_cfg_t c = brrlog_default_cfg;
		c.style_enabled = style_enabled != 0;
		c.flush_enabled = 1;
		c.min_label = logpri_min;
		if (brrlog_init(c, "(!", "!)")) {
			fprintf(stderr, "Failed to initialize brrlog: %s (%i)\n", strerror(errno), errno);
			return 1;
		}
	}

	#define _nostyle {0}
	_brrlog_try_pri(logpri_programmer_error, _nostyle, "(!f=ms=r:[PROGRAM]:!)     ", stderr, brrlog_dst_stream);
	_brrlog_try_pri(logpri_critical,         _nostyle, "(!f=rs=r:[CRITICAL]:!)    ", stderr, brrlog_dst_stream);
	_brrlog_try_pri(logpri_error,            _nostyle, "(!f=rs=b:[ERROR]:!)       ", stderr, brrlog_dst_stream);
	_brrlog_try_pri(logpri_warning,          _nostyle, "(!f=y   :[CAUTION]:!)     ", stderr, brrlog_dst_stream);
	_brrlog_try_pri(logpri_normal,           _nostyle, NULL, stderr, brrlog_dst_stream);
	_brrlog_try_pri(logpri_info,             _nostyle, "(!f=b   :[INFORMATION]:!) ", stderr, brrlog_dst_stream);
	_brrlog_try_pri(logpri_debug,            _nostyle, "(!f=ys=b:[DEBUG]:!)       ", stderr, brrlog_dst_stream);
	#undef _nostyle

	#undef _brrlog_try_pri
	return 0;
}
