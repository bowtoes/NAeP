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

#include "common/NeDebugging.h"
#include "revorbc/revorbc.h"
#include "wisp/NeWisp.h"

struct NeArg *
NeFindArg(struct NeArgs args, const char *const arg)
{
	for (int i = 0; i < args.argcount; ++i) {
		if (NeStrCmp(arg, 0, args.args[i].arg.cstr, NULL)) {
			return &args.args[i];
		}
	}
	return NULL;
}

void
NePrintArg(struct NeArg arg, NeSz maxarg, int newline)
{
	struct NeLogFmt fm = NeLogPrFmt(arg.opt.loglevel);
	enum NeLogPr pr = NePrDebug;
	if      (arg.opt.weem) { NePREFIXNFG(pr, NeClGreen,  "WEEM"); }
	else if (arg.opt.wisp) { NePREFIXNFG(pr, NeClYellow, "WISP"); }
	else if (arg.opt.bank) { NePREFIXNFG(pr, NeClRed,    "BANK"); }
	else if (arg.opt.oggs) { NePREFIXNFG(pr, NeClBlue,   "OGGS"); }
	else                   { NePREFIXNST(pr, NeClMagenta, NeClNormal, NeStBold, "AUT"); }
	NePREFIXN(pr, " ");
	if (arg.opt.logcolor)    { NePREFIXNST(pr, NeClBlue, NeClNormal, NeStBold, "SYL"); }
	else                     { NePREFIXNST(pr, NeClRed,  NeClNormal, NeStNone, "SMP"); }
	NePREFIXN(pr, " ");
	if (arg.opt.logoff) { NePREFIXNST(pr, NeClRed,   NeClNormal, NeStBold, "DSB"); }
	else                { NePREFIXNST(pr, NeClGreen, NeClNormal, NeStBold, "ENB"); }
	NePREFIXN(pr, " ");
	if (arg.opt.logdebug) { NePREFIXNST(pr, NeClCyan, NeClNormal, NeStBold, "DBG"); }
	else                  { NePREFIXNST(pr, NeClCyan, NeClNormal, NeStBold, "NRM"); }
	NePREFIXN(pr, " ");
	NePREFIXNST(pr, fm.fg, fm.bg, fm.st, "%s", NeLogPrDbgStr(arg.opt.loglevel));
	NePREFIXN(pr, " log ");
	if (arg.opt.dryrun) { NePREFIXNST(pr, NeClYellow, NeClNormal, NeStBold, "DRY"); }
	else                { NePREFIXNST(pr, NeClYellow, NeClNormal, NeStNone, "WET"); }
	NePREFIXN(pr, " ");
	if (arg.opt.bankrecurse) { NePREFIXNST(pr, NeClGreen,   NeClNormal, NeStBold, "FLL"); }
	else                     { NePREFIXNST(pr, NeClGreen,   NeClNormal, NeStNone, "NON"); }
	NePREFIXN(pr, " bnk rcs ");
	if (arg.opt.autoogg)     { NePREFIXNST(pr, NeClMagenta, NeClNormal, NeStBold, "AUT"); }
	else                     { NePREFIXNST(pr, NeClMagenta, NeClNormal, NeStNone, "NON"); }
	NePREFIXN(pr, " ");
	if (arg.opt.ogginplace)  { NePREFIXNST(pr, NeClBlue,    NeClNormal, NeStBold, "RPL"); }
	else                     { NePREFIXNST(pr, NeClBlue,    NeClNormal, NeStNone, "SEP"); }
	NePREFIXN(pr, " ogg cnv ");
	if (arg.opt.autorvb)     { NePREFIXNST(pr, NeClYellow,  NeClNormal, NeStBold, "AUT"); }
	else                     { NePREFIXNST(pr, NeClYellow,  NeClNormal, NeStNone, "NON"); }
	NePREFIXN(pr, " ");
	if (arg.opt.rvbinplace)  { NePREFIXNST(pr, NeClCyan,    NeClNormal, NeStBold, "RPL"); }
	else                     { NePREFIXNST(pr, NeClCyan,    NeClNormal, NeStNone, "SEP"); }
	NePREFIXN(pr, " rvb%s", newline?"\n":" ");
}

void
NeDetectType(struct NeArg *arg, struct NeFile *f)
{
	NeFcc fcc;
	if (NeFileSegment(f, &fcc, 0, 4, 4) != NeERFREAD) {
		if (fcc == WEEMCC) {
			if (NeStrEndswith(f->path, NeStrShallow(".wsp", 4)))
				arg->opt.wisp = 1;
			else if (NeStrEndswith(f->path, NeStrShallow(".wem", 4)))
				arg->opt.weem = 1;
			/* assume wisp because wisps are a superset of weems */
			else
				arg->opt.wisp = 1;
		} else if (fcc == BANKCC) {
			arg->opt.bank = 1;
		} else if (fcc == OGGSCC) {
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
		NeDEBUGN("Open ");
		NePREFIXNFG(NePrDebug, NeClCyan, "%s", opath.cstr);
		NePREFIX(NePrDebug, " for revorbtion output");
		if (NeFileOpen(&out, opath.cstr, NeFileModeWrite) != NeERGNONE) {
			NeERROR("Failed to open %s for revorb output : %m", opath.cstr);
			err = NeERFFILE;
		} else if (revorb(infile->file, out.file) != NeERGNONE) {
			NeERROR("Failed to revorb %s", infile->path.cstr);
			err = NeERRREVORB;
		}
		NeASSERTM(NeFileClose(&out) == NeERGNONE, "Failed to close output file %s : %m", out.path.cstr);
		if (arg.opt.rvbinplace) {
			struct NeStr ipath = {0};
			NeStrCopy(&ipath, infile->path);
			NeDEBUGN("Remove ");
			NePREFIXFG(NePrDebug, NeClCyan, "%s", infile->path.cstr);
			if ((err = NeFileRemove(infile)) == NeERGNONE) {
				NeDEBUGN("Move ");
				NePREFIXNFG(NePrDebug, NeClCyan, "%s", opath.cstr);
				NePREFIXN(NePrDebug, " -> ");
				NePREFIXFG(NePrDebug, NeClCyan, "%s", ipath.cstr);
				if ((err = NeFileRename(opath.cstr, ipath.cstr)) != NeERGNONE) {
					NeERROR("Failed to rename %s to %s : %m", err, opath.cstr, ipath.cstr);
					err = NeERFFILE;
				}
			} else {
				NeERROR("Failed to remove %s when renaming : %m", ipath.cstr);
				err = NeERFFILE;
			}
			NeStrDel(&ipath);
		}
	} else {
		NeDEBUGN("Open ");
		NePREFIXNFG(NePrDebug, NeClCyan, "%s", opath.cstr);
		NePREFIX(NePrDebug, " for revorbtion output");
		if (arg.opt.rvbinplace) {
			NeDEBUGN("Move ");
			NePREFIXNFG(NePrDebug, NeClCyan, "%s", opath.cstr);
			NePREFIXN(NePrDebug, " -> ");
			NePREFIXFG(NePrDebug, NeClCyan, "%s", infile->path.cstr);
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
			NeERROR("Failed reading wisp file %s : %m", infile->path.cstr);
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
