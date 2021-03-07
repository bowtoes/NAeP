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
	else                   { NePREFIXNST(pr, NeClMagenta, NeClNormal, NeStBold, "AUTO"); }
	NePREFIXNFG(pr, NeClMagenta, " %-*s ", maxarg, arg.arg.cstr);
	if (arg.opt.logcolor)    { NePREFIXNST(pr, NeClBlue, NeClNormal, NeStBold, "STYLED"); }
	else                     { NePREFIXNST(pr, NeClRed,  NeClNormal, NeStNone, "SIMPLE"); }
	NePREFIXN(pr, " ");
	if (arg.opt.logoff) { NePREFIXNST(pr, NeClRed,   NeClNormal, NeStBold, "DISABLED"); }
	else                { NePREFIXNST(pr, NeClGreen, NeClNormal, NeStBold, " ENABLED"); }
	NePREFIXN(pr, " ");
	if (arg.opt.logdebug) { NePREFIXNST(pr, NeClCyan, NeClNormal, NeStBold, "DEBUG "); }
	else                  { NePREFIXNST(pr, NeClCyan, NeClNormal, NeStBold, "NORMAL"); }
	NePREFIXN(pr, " ");
	NePREFIXNST(pr, fm.fg, fm.bg, fm.st, "%8s", NeLogPrStr(arg.opt.loglevel));
	NePREFIXN(pr, " log ");
	if (arg.opt.dryrun) { NePREFIXNST(pr, NeClYellow, NeClNormal, NeStBold, "DRY RUN"); }
	else                { NePREFIXNST(pr, NeClYellow, NeClNormal, NeStNone, "PROCESS"); }
	NePREFIXN(pr, " ");
	if (arg.opt.bankrecurse) { NePREFIXNST(pr, NeClGreen,   NeClNormal, NeStBold, "FULL"); }
	else                     { NePREFIXNST(pr, NeClGreen,   NeClNormal, NeStNone, "  NO"); }
	NePREFIXN(pr, " bank recurse ");
	if (arg.opt.autoogg)     { NePREFIXNST(pr, NeClMagenta, NeClNormal, NeStBold, "AUTO"); }
	else                     { NePREFIXNST(pr, NeClMagenta, NeClNormal, NeStNone, "  NO"); }
	NePREFIXN(pr, " ");
	if (arg.opt.ogginplace)  { NePREFIXNST(pr, NeClBlue,    NeClNormal, NeStBold, "IN-PLACE"); }
	else                     { NePREFIXNST(pr, NeClBlue,    NeClNormal, NeStNone, "SEPARATE"); }
	NePREFIXN(pr, " ogg convert ");
	if (arg.opt.autorvb)     { NePREFIXNST(pr, NeClYellow,  NeClNormal, NeStBold, "AUTO"); }
	else                     { NePREFIXNST(pr, NeClYellow,  NeClNormal, NeStNone, "  NO"); }
	NePREFIXN(pr, " ");
	if (arg.opt.rvbinplace)  { NePREFIXNST(pr, NeClCyan,    NeClNormal, NeStBold, "IN-PLACE"); }
	else                     { NePREFIXNST(pr, NeClCyan,    NeClNormal, NeStNone, "SEPARATE"); }
	NePREFIXN(pr, " revorb\n");
}

void
NeDetectType(struct NeArg *arg, struct NeFile *f)
{
	NeFcc fcc;
	if (NeFileStream(f, &fcc, 4) != NeERFREAD) {
		if (fcc == WEEMCC) {
			if (NeStrEndswith(f->ppp, NeStrShallow(".wsp", 4)))
				arg->opt.wisp = 1;
			else if (NeStrEndswith(f->ppp, NeStrShallow(".wem", 4)))
				arg->opt.weem = 1;
			else
				arg->opt.wisp = 1;
			/* assume wisp because wisps are a superset of weems */
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

int
NeRevorbOgg(struct NeArg arg, struct NeFile *f)
{
	struct NeFile out;
	struct NeStr op;
	int suc = 1;
	if (!f || !f->stat.exist)
		return 0;

	if (!arg.opt.dryrun) {
		NeStrCopy(&op, f->ppp);
		NeStrMerge(&op, NeStrShallow(".tmp", 4));
		if (NeFileOpen(&out, op.cstr, NeFileModeWrite) != NeERGNONE) {
			NeERROR("Failed to open %s for revorb output : %m", op.cstr);
			suc = 0;
		} else if (revorb(f->file, out.file) != NeERGNONE) {
			NeERROR("Failed to revorb %s", f->ppp.cstr);
			suc = 0;
		}
		if (suc && arg.opt.rvbinplace)
			NeDEBUG("TODO MOVE %s -> %s", op.cstr, f->ppp.cstr);
		NeStrDel(&op);
	} else {
		NeNORMALN("Opened ");
		NePREFIXNFG(NePrNormal, NeClCyan, "%s.tmp", f->ppp.cstr);
		NePREFIX(NePrNormal, " for revorbtion output");
		NeNORMALN("Moved ");
		NePREFIXNFG(NePrNormal, NeClCyan, "%s.tmp", f->ppp.cstr);
		NeNORMALN(" -> ");
		if (arg.opt.rvbinplace)
			NePREFIXFG(NePrNormal, NeClCyan, "%s", f->ppp.cstr);
		else
			NePREFIXFG(NePrNormal, NeClCyan, "%.*s_rvb.ogg", NeStrRindex(f->ppp, NeStrShallow(".", 1), f->ppp.length), f->ppp.cstr);
	}

	return suc;
}
int
NeConvertWeem(struct NeArg arg, struct NeFile *f)
{
	return 0;
}
int
NeExtractWisp(struct NeArg arg, struct NeFile *f)
{
	return 0;
}
int
NeExtractBank(struct NeArg arg, struct NeFile *f)
{
	return 0;
}
