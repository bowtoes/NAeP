#include "NeArg.h"

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
NePrintArg(struct NeArg arg, NeSz maxarg)
{
	struct NeLogFmt fm = NeLogPrFmt(arg.opt.loglevel);
	enum NeLogPr pr = NePrDebug;
	if      (arg.opt.weem) { NePREFIXNFG(pr, NeClGreen,  "WEEM"); }
	else if (arg.opt.wisp) { NePREFIXNFG(pr, NeClYellow, "WISP"); }
	else if (arg.opt.bank) { NePREFIXNFG(pr, NeClRed,    "BANK"); }
	else if (arg.opt.oggs) { NePREFIXNFG(pr, NeClBlue,   "OGGS"); }
	else                   { NePREFIXNST(pr, NeClMagenta, NeClNormal, NeStBold, "AUT"); }
	NePREFIXNFG(pr, NeClMagenta, " %-*s ", maxarg, arg.arg.cstr);
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
	NePREFIXN(pr, " rvb\n");
}

void
NeDetectType(struct NeArg *arg, struct NeFile *f)
{
	NeFcc fcc;
	if (NeFileSegment(f, &fcc, 4, 0, 4) != NeERFREAD) {
		if (fcc == WEEMCC) {
			if (NeStrEndswith(f->ppp, NeStrShallow(".wsp", 4)))
				arg->opt.wisp = 1;
			else if (NeStrEndswith(f->ppp, NeStrShallow(".wem", 4)))
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
NeRevorbOgg(struct NeArg arg, struct NeFile *f)
{
	struct NeFile out;
	struct NeStr op = {0};
	NeErr err = NeERGNONE;
	if (!f || !f->stat.exist)
		return err;

	NeStrCopy(&op, f->ppp);
	NeStrPrint(&op, NeStrRindex(f->ppp, NeStrShallow(".", 1), f->ppp.length), f->ppp.length + 10, "_rvb.ogg");
	if (!arg.opt.dryrun) {
		NeDEBUGN("Open ");
		NePREFIXNFG(NePrDebug, NeClCyan, "%s", op.cstr);
		NePREFIX(NePrDebug, " for revorbtion output");
		if (NeFileOpen(&out, op.cstr, NeFileModeWrite) != NeERGNONE) {
			NeERROR("Failed to open %s for revorb output : %m", op.cstr);
			err = NeERFFILE;
		} else if (revorb(f->file, out.file) != NeERGNONE) {
			NeERROR("Failed to revorb %s", f->ppp.cstr);
			err = NeERRREVORB;
		}
		if ((err = NeFileClose(&out)) == NeERGNONE) {
			if (err == NeERGNONE && arg.opt.rvbinplace) {
				NeDEBUGN("Move ");
				NePREFIXNFG(NePrDebug, NeClCyan, "%s", op.cstr);
				NePREFIXN(NePrDebug, " -> ");
				NePREFIXFG(NePrDebug, NeClCyan, "%s", f->ppp.cstr);
				if ((err = NeFileRename(out.ppp.cstr, f->ppp.cstr)) != NeERGNONE) {
					NeERROR("Failed to rename %s to %s : %m", op.cstr, f->ppp.cstr);
					err = NeERFFILE;
				}
			}
		}
	} else {
		NeDEBUGN("Open ");
		NePREFIXNFG(NePrDebug, NeClCyan, "%s", op.cstr);
		NePREFIX(NePrDebug, " for revorbtion output");
		if (arg.opt.rvbinplace) {
			NeDEBUGN("Move ");
			NePREFIXNFG(NePrDebug, NeClCyan, "%s", op.cstr);
			NePREFIXN(NePrDebug, " -> ");
			NePREFIXFG(NePrDebug, NeClCyan, "%s", f->ppp.cstr);
		}
	}
	NeStrDel(&op);

	return err;
}
NeErr
NeConvertWeem(struct NeArg arg, struct NeFile *f)
{
	return 0;
}
NeErr
NeExtractWisp(struct NeArg arg, struct NeFile *f)
{
	return 0;
}
NeErr
NeExtractBank(struct NeArg arg, struct NeFile *f)
{
	return 0;
}
