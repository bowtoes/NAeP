/*
Copyright 2021 BowToes (bow.toes@mailfence.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "NeArg.h"

#include <string.h>
#include <brrtools/brrtil.h>
#include <brrtools/brrplatform.h>

#include "revorbc/revorbc.h"
/* #include "wisp/NeWisp.h" */

NeArgT *BRRCALL
NeFindArg(NeArgArrayT args, const char *const arg)
{
	for (int i = 0; i < args.arg_count; ++i) {
		if (0 == strcmp(arg, (char *)args.args[i].argument.opaque))
			return &args.args[i];
	}
	return NULL;
}
void BRRCALL
NePrintArg(NeArgT arg, int newline, brrsz max_arg_length)
{
#define fm gbrrlog_type_normal.format
#define lv gbrrlog_type_debug.level
	if      (arg.options.weem) { BRRLOG_MESSAGESNP(lv, brrlog_color_green,   0, 0, 0, "WEEM"); }
	else if (arg.options.wisp) { BRRLOG_MESSAGESNP(lv, brrlog_color_yellow,  0, 0, 0, "WISP"); }
	else if (arg.options.bank) { BRRLOG_MESSAGESNP(lv, brrlog_color_red,     0, 0, 0, "BANK"); }
	else if (arg.options.oggs) { BRRLOG_MESSAGESNP(lv, brrlog_color_blue,    0, 0, 0, "OGGS"); }
	else                       { BRRLOG_MESSAGESNP(lv, brrlog_color_magenta, 0, brrlog_style_bold, 0, "AUT"); }
	BRRLOG_MESSAGEFNP(lv, fm, "|log: ");
	if (arg.options.logEnabled) { BRRLOG_MESSAGESNP(lv, brrlog_color_red,   0, brrlog_style_bold, 0, "ENB"); }
	else                        { BRRLOG_MESSAGESNP(lv, brrlog_color_green, 0, brrlog_style_bold, 0, "DSB"); }
	BRRLOG_MESSAGEFNP(lv, fm, " ");
	if (arg.options.logColorEnabled) { BRRLOG_MESSAGESNP(lv, brrlog_color_blue,  0, brrlog_style_bold, 0, "SYL"); }
	else                             { BRRLOG_MESSAGESNP(lv, brrlog_color_red,   0, 0, 0, "SMP"); }
	BRRLOG_MESSAGEFNP(lv, fm, " ");
	if (arg.options.logDebug) { BRRLOG_MESSAGESNP(lv, brrlog_color_cyan, 0, brrlog_style_bold, 0, "DBG"); }
	else                      { BRRLOG_MESSAGESNP(lv, brrlog_color_cyan, 0, brrlog_style_bold, 0, "NRM"); }
	BRRLOG_MESSAGEFNP(lv, fm, "|operation: ");
	if (arg.options.dryRun) { BRRLOG_MESSAGESNP(lv, brrlog_color_yellow, 0, brrlog_style_bold, 0, "DRY"); }
	else                    { BRRLOG_MESSAGESNP(lv, brrlog_color_yellow, 0, 0, 0, "WET"); }
	BRRLOG_MESSAGEFNP(lv, fm, "|bank recurse: ");
	if (arg.options.recurseBanks) { BRRLOG_MESSAGESNP(lv, brrlog_color_green,   0, brrlog_style_bold, 0, "FUL"); }
	else                          { BRRLOG_MESSAGESNP(lv, brrlog_color_green,   0, 0, 0, "NON"); }
	BRRLOG_MESSAGEFNP(lv, fm, "|ogg conversion: ");
	if (arg.options.autoOgg) { BRRLOG_MESSAGESNP(lv, brrlog_color_magenta, 0, brrlog_style_bold, 0, "AUT"); }
	else                     { BRRLOG_MESSAGESNP(lv, brrlog_color_magenta, 0, 0, 0, "MAN"); }
	BRRLOG_MESSAGEFNP(lv, fm, " ");
	if (arg.options.oggInplace) { BRRLOG_MESSAGESNP(lv, brrlog_color_blue,    0, brrlog_style_bold, 0, "INP"); }
	else                        { BRRLOG_MESSAGESNP(lv, brrlog_color_blue,    0, 0, 0, "SEP"); }
	BRRLOG_MESSAGEFNP(lv, fm, "|revorption: ");
	if (arg.options.autoRevorb) { BRRLOG_MESSAGESNP(lv, brrlog_color_yellow,  0, brrlog_style_bold, 0, "AUT"); }
	else                        { BRRLOG_MESSAGESNP(lv, brrlog_color_yellow,  0, 0, 0, "MAN"); }
	BRRLOG_MESSAGEFNP(lv, fm, " ");
	if (arg.options.revorbInplace) { BRRLOG_MESSAGESNP(lv, brrlog_color_cyan,    0, brrlog_style_bold, 0, "INP"); }
	else                           { BRRLOG_MESSAGESNP(lv, brrlog_color_cyan,    0, 0, 0, "SEP"); }
	if (newline)
		BRRLOG_MESSAGEFP(lv, fm, "");
#undef fm
#undef lv
}

void BRRCALL
NeDetectType(NeArgT *arg, const brrpath_stat_resultT *const st, const char *const path)
{
	brrfccT fcc;
	/* TODO */
	/* Check FOURCC */
	/* If no match: */
	/* Check file extension */
	/* If still no match: */
	/* Return unknown type. */
#if 0
	if (NeFileSegment(f, &fcc, 0, 4, 4) != NeArgErr_Read) {
		if (fcc.code == WEEMCC.code) {
			if (brrstg_ends_with(&f->path, ".wsp", true))
				arg->options.wisp = 1;
			else if (brrstr_ends_with(&f->path, ".wem", true))
				arg->options.weem = 1;
			/* assume wisp because wisps are a superset of weems */
			else
				arg->options.wisp = 1;
		} else if (fcc.code == BANKCC.code) {
			arg->options.bank = 1;
		} else if (fcc.code == OGGSCC.code) {
			arg->options.oggs = 1;
		} else {
			arg->options.weem = arg->options.wisp = arg->options.bank = arg->options.oggs = 0;
		}
	} else {
		arg->options = (NeArgOptionsT){0};
	}
#endif
}
int BRRCALL
NeRevorbOgg(const NeArgT *const arg, const brrpath_stat_resultT *const st, const char *const path)
{
	/* TODO */
	int err = 0;
	BRRLOG_DEBUG("NeRevorbOgg : (arg->argument:%s) (path:%s)",
	    BRRTIL_NULSTR((char *)arg->argument.opaque), BRRTIL_NULSTR(path));
	return err;
}
int BRRCALL
NeConvertWeem(const NeArgT *const arg, const brrpath_stat_resultT *const st, const char *const path)
{
	/* TODO */
	int err = 0;
	BRRLOG_DEBUG("NeConvertWeem : (arg->argument:%s) (path:%s)",
	    BRRTIL_NULSTR((char *)arg->argument.opaque), BRRTIL_NULSTR(path));
	return err;
}
int BRRCALL
NeExtractWisp(const NeArgT *const arg, const brrpath_stat_resultT *const st, const char *const path)
{
	/*
	struct NeWisp wsp;
	NeArgErrT err = NeArgErr_None;
	if (!infile || !infile->stat.exist)
		return err;

	if (!arg.options.dryrun) {
		if ((err = NeWispRead(&wsp, infile)) != NeArgErr_None) {
			BRRLOG_ERR("Failed reading wisp file %s : %m", infile->path.cstr);
		} else {
			NeWispSave(&wsp, 0);
		}
	} else {
	}

	return err;
	*/
	/* TODO */
	int err = 0;
	BRRLOG_DEBUG("NeExtractWisp : (arg->argument:%s) (path:%s)",
	    BRRTIL_NULSTR((char *)arg->argument.opaque), BRRTIL_NULSTR(path));
	return err;
}
int BRRCALL
NeExtractBank(const NeArgT *const arg, const brrpath_stat_resultT *const st, const char *const path)
{
	/* TODO */
	int err = 0;
	BRRLOG_DEBUG("NeExtractBank : (arg->argument:%s) (path:%s)",
	    BRRTIL_NULSTR((char *)arg->argument.opaque), BRRTIL_NULSTR(path));
	return err;
}
