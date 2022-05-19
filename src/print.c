#include "print.h"

#include <stdio.h>
#include <stdlib.h>

#include <brrtools/brrnum.h>

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
"\n        -w, -wem, -weem . . . . . . . . . .  File(s) are WEMs to be converted to OGG." \
"\n        -W, -wsp, -wisp . . . . . . . . . .  File(s) are wisps to have their WEMs extracted." \
"\n        -b, -bnk, -bank . . . . . . . . . .  File(s) are banks to extract all referenced WEMs." \
"\n        -o, -ogg  . . . . . . . . . . . . .  File(s) are OGG files to be regranularizeed." \
"\n    OGG Processing Options:" \
"\n        -ri, -rgrn-inplace, -rvb-inplace. .  Oggs are regranularized in-place." \
"\n    WEM Processing Options:" \
"\n        -oi, -ogg-inplace . . . . . . . . .  All WEM-to-OGG conversion is done in-place;" \
"\n                                             WEMs are replaced with their converted OGGs." \
"\n        -cbl, -codebook-library . . . . . .  The following is a codebook library to use for the following WEMs;" \
"\n                                             if none are specified for a given WEM, then it is assumed the" \
"\n                                             codebooks are inline." \
"\n        -inline . . . . . . . . . . . . . .  The following WEMs have inline codebooks." \
"\n        -stripped . . . . . . . . . . . . .  The vorbis headers of the WEM are stripped." \
"\n    WSP/BNK Processing Options:" \
"\n        -w2o, -wem2ogg  . . . . . . . . . .  Convert extracted WEMs to OGGs." \
"\n        -white, -black," \
"\n        -weiss, -noir . . . . . . . . . . .  Comma-separated list of indices used to determine" \
"\n                                             what indices to process from the file(s)." \
"\n                                             Blacklists and whitelists are mutually exclusive; each " \
"\n                                             overrides the others preceding it." \
"\n                                             E.g. '-black 3,9' would process every index except 3 and 9," \
"\n                                             but  '+white 3,9' would process only indices 3 and 9." \
"\n        -rubrum . . . . . . . . . . . . . .  Toggle the list to between being a whitelist/blacklist." \
"\n    Miscellaneous options:" \
"\n        -!  . . . . . . . . . . . . . . . .  The following argument is a file path, not an option." \
"\n        --  . . . . . . . . . . . . . . . .  All following arguments are file paths, not options." \
"\n        -d, -debug  . . . . . . . . . . . .  Enable debug output, irrespective of quiet settings." \
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
int
print_report(const nestateT *const state)
{
	brrsz input_count_digits = 1 + brrnum_ndigits(state->n_inputs, 0, 10);
	brrsz total_success =
	      state->oggs_regrained
	    + state->wems_converted
	    + state->wsps_processed
	    + state->bnks_processed;
	brrsz total_failure =
	      state->oggs_failed
	    + state->wems_failed
	    + state->wsps_failed
	    + state->bnks_failed;
	BRRLOG_NORN("Successfully processed a total of ");
	BRRLOG_FORENP(LOG_COLOR_INFO, "%*i / %*i",
	    input_count_digits, total_success, input_count_digits, state->n_inputs);
	BRRLOG_NORP(" inputs");
	if (state->full_report) {
		if (state->oggs_to_regrain) {
			BRRLOG_NORN("    ");
			BRRLOG_FORENP(LOG_COLOR_INFO, "%*i / %*i",
				input_count_digits, state->oggs_regrained, input_count_digits, state->oggs_to_regrain);
			BRRLOG_MESSAGETP(gbrrlog_level_last, LOG_FORMAT_OGG, " Regrained Oggs");
		}
		if (state->wems_to_convert) {
			BRRLOG_NORN("    ");
			BRRLOG_FORENP(LOG_COLOR_INFO, "%*i / %*i",
				input_count_digits, state->wems_converted, input_count_digits, state->wems_to_convert);
			BRRLOG_MESSAGETP(gbrrlog_level_last, LOG_FORMAT_WEM, " Converted WEMs");
		}
		if (state->wsps_to_process) {
			BRRLOG_NORN("    ");
			BRRLOG_FORENP(LOG_COLOR_INFO, "%*i / %*i",
				input_count_digits, state->wsps_processed, input_count_digits, state->wsps_to_process);
			BRRLOG_MESSAGETP(gbrrlog_level_last, LOG_FORMAT_WSP, " Processed WSPs");
		}
		if (state->bnks_to_process) {
			BRRLOG_NORN("    ");
			BRRLOG_FORENP(LOG_COLOR_INFO, "%*i / %*i",
				input_count_digits, state->bnks_processed, input_count_digits, state->bnks_to_process);
			BRRLOG_MESSAGETP(gbrrlog_level_last, LOG_FORMAT_BNK, " Processed BNKs");
		}
		if (state->wems_to_extract) {
			BRRLOG_NORN("    ");
			BRRLOG_FORENP(LOG_COLOR_INFO, "%*i / %*i",
				input_count_digits, state->wems_extracted, input_count_digits, state->wems_to_extract);
			BRRLOG_MESSAGETP(gbrrlog_level_last, LOG_FORMAT_WEM, " Extracted WEMs");
		}
		if (state->wems_to_convert_extract) {
			BRRLOG_NORN("    ");
			BRRLOG_FORENP(LOG_COLOR_INFO, "%*i / %*i",
				input_count_digits, state->wems_convert_extracted, input_count_digits, state->wems_to_convert_extract);
			BRRLOG_MESSAGETP(gbrrlog_level_last, LOG_FORMAT_OGG, " Auto-converted WEMs");
		}
	}
	return 0;
}
