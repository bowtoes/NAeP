#include "nelog.h"

#include <stdio.h>
#include <stdlib.h>

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
