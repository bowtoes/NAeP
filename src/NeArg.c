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

#include <brrtools/brrlog.h>
#include <brrtools/brrdebug.h>

#include "revorbc/revorbc.h"
#include "wisp/NeWisp.h"

struct NeArg *
NeFindArg(struct NeArgs args, const char *const arg)
{
	for (int i = 0; i < args.argcount; ++i) {
		if (NeStrCmp(arg, 0, args.args[i].arg.cstr, NULL))
			return &args.args[i];
	}
	return NULL;
}

void
NePrintArg(struct NeArg arg, brrsz maxarg, int newline)
{
	brrlog_formatT fm = brrlog_format_debug;
	brrlog_levelT lv = fm.level;
	if      (arg.opt.weem) { BRRLOG_MESSAGE_FGNP(lv, brrlog_color_green,  "WEEM"); }
	else if (arg.opt.wisp) { BRRLOG_MESSAGE_FGNP(lv, brrlog_color_yellow, "WISP"); }
	else if (arg.opt.bank) { BRRLOG_MESSAGE_FGNP(lv, brrlog_color_red,    "BANK"); }
	else if (arg.opt.oggs) { BRRLOG_MESSAGE_FGNP(lv, brrlog_color_blue,   "OGGS"); }
	else                   { BRRLOG_MESSAGE_STNP(lv, brrlog_color_magenta, 0, brrlog_style_bold, "AUT"); }
	BRRLOG_MESSAGE_EM(lv, " ");
	if (arg.opt.logcolor)    { BRRLOG_MESSAGE_STNP(lv, brrlog_color_blue, brrlog_color_normal, brrlog_style_bold, "SYL"); }
	else                     { BRRLOG_MESSAGE_STNP(lv, brrlog_color_red,  brrlog_color_normal, brrlog_style_none, "SMP"); }
	BRRLOG_MESSAGE_EMNP(lv, " ");
	if (arg.opt.logoff) { BRRLOG_MESSAGE_STNP(lv, brrlog_color_red,   brrlog_color_normal, brrlog_style_bold, "DSB"); }
	else                { BRRLOG_MESSAGE_STNP(lv, brrlog_color_green, brrlog_color_normal, brrlog_style_bold, "ENB"); }
	BRRLOG_MESSAGE_EMNP(lv, " ");
	if (arg.opt.logdebug) { BRRLOG_MESSAGE_STNP(lv, brrlog_color_cyan, brrlog_color_normal, brrlog_style_bold, "DBG"); }
	else                  { BRRLOG_MESSAGE_STNP(lv, brrlog_color_cyan, brrlog_color_normal, brrlog_style_bold, "NRM"); }
	BRRLOG_MESSAGE_EMNP(lv, " ");
	BRRLOG_MESSAGEFNP(lv, fm, "%s", brrlog_priority_dbgstr(arg.opt.loglevel));
	BRRLOG_MESSAGE_EMNP(lv, " log ");
	if (arg.opt.dryrun) { BRRLOG_MESSAGE_STNP(lv, brrlog_color_yellow, brrlog_color_normal, brrlog_style_bold, "DRY"); }
	else                { BRRLOG_MESSAGE_STNP(lv, brrlog_color_yellow, brrlog_color_normal, brrlog_style_none, "WET"); }
	BRRLOG_MESSAGE_EMNP(lv, " ");
	if (arg.opt.bankrecurse) { BRRLOG_MESSAGE_STNP(lv, brrlog_color_green,   brrlog_color_normal, brrlog_style_bold, "FLL"); }
	else                     { BRRLOG_MESSAGE_STNP(lv, brrlog_color_green,   brrlog_color_normal, brrlog_style_none, "NON"); }
	BRRLOG_MESSAGE_EMNP(lv, " bnk rcs ");
	if (arg.opt.autoogg)     { BRRLOG_MESSAGE_STNP(lv, brrlog_color_magenta, brrlog_color_normal, brrlog_style_bold, "AUT"); }
	else                     { BRRLOG_MESSAGE_STNP(lv, brrlog_color_magenta, brrlog_color_normal, brrlog_style_none, "NON"); }
	BRRLOG_MESSAGE_EMNP(lv, " ");
	if (arg.opt.ogginplace)  { BRRLOG_MESSAGE_STNP(lv, brrlog_color_blue,    brrlog_color_normal, brrlog_style_bold, "RPL"); }
	else                     { BRRLOG_MESSAGE_STNP(lv, brrlog_color_blue,    brrlog_color_normal, brrlog_style_none, "SEP"); }
	BRRLOG_MESSAGE_EMNP(lv, " ogg cnv ");
	if (arg.opt.autorvb)     { BRRLOG_MESSAGE_STNP(lv, brrlog_color_yellow,  brrlog_color_normal, brrlog_style_bold, "AUT"); }
	else                     { BRRLOG_MESSAGE_STNP(lv, brrlog_color_yellow,  brrlog_color_normal, brrlog_style_none, "NON"); }
	BRRLOG_MESSAGE_EMNP(lv, " ");
	if (arg.opt.rvbinplace)  { BRRLOG_MESSAGE_STNP(lv, brrlog_color_cyan,    brrlog_color_normal, brrlog_style_bold, "RPL"); }
	else                     { BRRLOG_MESSAGE_STNP(lv, brrlog_color_cyan,    brrlog_color_normal, brrlog_style_none, "SEP"); }
	BRRLOG_MESSAGE_EMNP(lv, " rvb%s", newline?"\n":" ");
}

void
NeDetectType(struct NeArg *arg, struct NeFile *f)
{
	brrfccT fcc;
	if (NeFileSegment(f, &fcc, 0, 4, 4) != NeERFREAD) {
		if (fcc.code == WEEMCC.code) {
			if (NeStrEndswith(f->path, NeStrShallow(".wsp", 4)))
				arg->opt.wisp = 1;
			else if (NeStrEndswith(f->path, NeStrShallow(".wem", 4)))
				arg->opt.weem = 1;
			/* assume wisp because wisps are a superset of weems */
			else
				arg->opt.wisp = 1;
		} else if (fcc.code == BANKCC.code) {
			arg->opt.bank = 1;
		} else if (fcc.code == OGGSCC.code) {
			arg->opt.oggs = 1;
		} else {
			arg->opt.weem = arg->opt.wisp = arg->opt.bank = arg->opt.oggs = 0;
		}
	} else {
		arg->options = 0;
	}
}

NeErr
NeRevorbOgg(struct NeArg arg, struct NeFile *infile)
{
	struct NeFile out;
	struct NeStr opath = {0};
	NeErr err = NeERGNONE;
	if (!infile || !infile->stat.exist)
		return err;

	NeStrCopy(&opath, infile->path);
	NeStrPrint(&opath,
			NeStrRindex(infile->path, NeStrShallow(".", 1), infile->path.length),
			infile->path.length + 10, "_rvb.ogg");
	if (!arg.opt.dryrun) {
		BRRLOG_DEBN("Open ");
		BRRLOG_MESSAGE_FGNP(brrlog_format_debug.level, brrlog_color_cyan, "%s", opath.cstr);
		BRRLOG_DEBP(" for revorbtion output");
		if (NeFileOpen(&out, opath.cstr, NeFileModeWrite) != NeERGNONE) {
			BRRLOG_ERR("Failed to open %s for revorb output : %m", opath.cstr);
			err = NeERFFILE;
		} else if (revorb(infile->file, out.file) != NeERGNONE) {
			BRRLOG_ERR("Failed to revorb %s", infile->path.cstr);
			err = NeERRREVORB;
		}
		BRRDEBUG_ASSERTM(NeFileClose(&out) == NeERGNONE, "Failed to close output file %s : %m", out.path.cstr);
		if (arg.opt.rvbinplace) {
			struct NeStr ipath = {0};
			NeStrCopy(&ipath, infile->path);
			BRRLOG_DEBN("Remove ");
			BRRLOG_MESSAGE_FGNP(brrlog_format_debug.level, brrlog_color_cyan, "%s", infile->path.cstr);
			if ((err = NeFileRemove(infile)) == NeERGNONE) {
				BRRLOG_DEBN("Move ");
				BRRLOG_MESSAGE_FGNP(brrlog_format_debug.level, brrlog_color_cyan, "%s", opath.cstr);
				BRRLOG_DEBUGNP(" -> ");
				BRRLOG_MESSAGE_FGP(brrlog_format_debug.level, brrlog_color_cyan, "%s", ipath.cstr);
				if ((err = NeFileRename(opath.cstr, ipath.cstr)) != NeERGNONE) {
					BRRLOG_ERR("Failed to rename %s to %s : %m", err, opath.cstr, ipath.cstr);
					err = NeERFFILE;
				}
			} else {
				BRRLOG_ERR("Failed to remove %s when renaming : %m", ipath.cstr);
				err = NeERFFILE;
			}
			NeStrDel(&ipath);
		}
	} else {
		BRRLOG_DEBN("Open ");
		BRRLOG_MESSAGE_FGNP(brrlog_format_debug.level, brrlog_color_cyan, "%s", opath.cstr);
		BRRLOG_DEBP(" for revorbtion output");
		if (arg.opt.rvbinplace) {
			BRRLOG_DEBN("Move ");
			BRRLOG_MESSAGE_FGNP(brrlog_format_debug.level, brrlog_color_cyan, "%s", opath.cstr);
			BRRLOG_DEBNP(" -> ");
			BRRLOG_MESSAGE_FGP(brrlog_format_debug.level, brrlog_color_cyan, "%s", infile->path.cstr);
		}
	}
	NeStrDel(&opath);

	return err;
}
NeErr
NeConvertWeem(struct NeArg arg, struct NeFile *infile)
{
	NeErr err = NeERGNONE;
	if (!infile || !infile->stat.exist)
		return err;
	return err;
}
NeErr
NeExtractWisp(struct NeArg arg, struct NeFile *infile)
{
	struct NeWisp wsp;
	NeErr err = NeERGNONE;
	if (!infile || !infile->stat.exist)
		return err;

	if (!arg.opt.dryrun) {
		if ((err = NeWispRead(&wsp, infile)) != NeERGNONE) {
			BRRLOG_ERR("Failed reading wisp file %s : %m", infile->path.cstr);
		} else {
			NeWispSave(&wsp, 0);
		}
	} else {
	}

	return err;
}
NeErr
NeExtractBank(struct NeArg arg, struct NeFile *infile)
{
	NeErr err = NeERGNONE;
	if (!infile || !infile->stat.exist)
		return err;
	return err;
}
