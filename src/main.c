#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <alloca.h>

#include "common/NeLogging.h"
#include "common/NeLibrary.h"
#include "common/NeMisc.h"
#include "common/NeStr.h"
#include "wisp/NeWisp.h"
#include "NeArg.h"

static const char *const usg =
"Usage: NAeP [[OPTIONS] FILE]..."
"\n Options:"
"\n     Options take affect on all files placed1after, and can be overwritten per file."
"\n         -h, -help, -v   Print this help."
"\n         -q, -quiet      Turn off superfluous logging."
"\n         -a, -auto       Filetype is automatically determined."
"\n         -W, -wisp       File is a wisp (.wsp) file; extract all weems from it."
"\n         -w, -weem       File is a weem (.wem) file; convert to ogg."
"\n         -b, -bank       File is a bank (.bnk) file; extract all weems found in it."
"\n         -R, -recurse    Recursively extract all weems referenced in each bank file,"
"\n                             if they're found in the given bank files."
"\n         -o, -ogg        Automatically convert extracted weem files into ogg/vorbis."
"\n         -r, -revorb     Automatically revorb converted ogg files."
;
static void printhelp()
{
	NeNORMAL("NAeP - NieR: Automata extraction Protocol");
	NeNORMAL("Compiled on %s %s", __DATE__, __TIME__);
	fprintf(stdout, "%s\n", usg);
	exit(0);
}

/* How work:
 * First, process all cmd arguments and build list of files, each with different,
 * specific options. Then go through the list and yada yada, do the extractions/conversions.
 * */

#if 1
int main(int argc, char **argv)
{
	for (int p = 0; p < NePriorityCount; ++p) {
		for (int f = 0; f < NeColorCount; ++f) {
			for (int b = 0; b < NeColorCount; ++b) {
				for (int s = 0; s < NeStyleCount; ++s) {
					for (int n = 0; n < NeFontCount; ++n) {
						struct NeLogFormat fmt = {f, b, s, n};
						NeLOGFM(p, f, b, s, n, 1, "Pr:%02i %s\tFg:%02i %s\tBg:%02i %s\tSt:%02i %s\tFn:%02i %s\t",
								p, NeLogPriorityStr(p),
								NeForegroundIndex(fmt), NeLogColorStr(f),
								NeBackgroundIndex(fmt), NeLogColorStr(b),
								NeStyleIndex(fmt), NeLogStyleStr(s),
								NeFontIndex(fmt), NeLogFontStr(n));
					}
				}
			}
		}
	}
}
#else
int main(int argc, char **argv)
{
	struct NeArgOpt opt = {0};
	struct NeArgs args = {0};
	char *arg = argv[1];

	opt.wisp = 1;
	if (argc == 1)
		printhelp();
	for (int i = 1; i < argc; ++i, arg = argv[i]) {
		NeSz t = NeParseArg(arg, &opt, &args);
		if (args.maxarg == -1) {
			NeERROR("WOah there, too many files!");
			break;
		} else if (args.maxarg == -2) {
			printhelp();
		}
	}
	if (args.argcount) {
		for (int i = 0; i < args.argcount; ++i) {
			NeProcessArg(args.list[i], args.maxarg);
		}
	} else {
		if (!opt.quiet) {
			NeERROR("No files passed");
		}
	}

	return 0;
}
#endif
