#include "NeArg.h"

#include <stdlib.h>

#include "common/NeDebugging.h"
#include "common/NeLogging.h"
#include "common/NeLibrary.h"
#include "wisp/NeWisp.h"

static const char *getoptstr(struct NeArgOpt opt) {
	if (opt.wisp)
		return "Wisp";
	else if (opt.weem)
		return "Weem";
	else if (opt.bank)
		return "Bank";
	else
		return "UNKNOWN";
}
static int extractwisp(struct NeArg a, struct NeWeem **dst) {
	struct NeWeem *d = *dst;
	struct NeWisp wsp = {0};
	struct NeStr wispPath, tempPath = {0};
	NeSz dgt = 0;
	int wemcount = 0, err = 0;

	if (!NeWispOpen(&wsp, a.arg)) {
		NeERROR("Failed to open %s for wisp extraction");
		dst = NULL;
		return 0;

	}
	wispPath = wsp.file.ppp;
	NeStrSlice(&tempPath, wispPath, 0, NeStrRindex(wispPath, NeStrShallow(".", 1), wispPath.length));

	d = NeSafeAlloc(d, wsp.wemCount * sizeof(*d), 1);
	for (int i = 0; i < wsp.wemCount; ++i) {
		struct NeWem wem = wsp.wems[i];
		struct NeWeem w = {0};
		NeSz wrt = 0;

		NeStrPrint(&w.outpath, 0, NeMAXPATH - 4, "%s_%0*u", tempPath.cstr, dgt, i);
		NeStrPrint(&w.outpath, w.outpath.length, NeMAXPATH, "%s", ".wem");
		w.size = wem.size;

		while (wrt < wem.size + 8) {
			NeSz rd = 0;
			w.data = NeSafeAlloc(w.data, wrt + NeBLOCKSIZE, 0);
			rd = NeFileSegment(&wsp.file, w.data + wrt, NeBLOCKSIZE, wem.offset + wrt, wem.offset + wem.size + 8);
			if (rd == -1) {
				err = 1;
				break;
			}
			wrt += rd;
		}
		wemcount++;
	}
	NeWispClose(&wsp);
	return wemcount;
}

struct NeArg *NeFindArg(struct NeArgs args, const char *const arg)
{
	for (int i = 0; i < args.argcount; ++i) {
		if (NeStrCmp(arg, 0, args.list[i].arg.cstr, NULL)) {
			return &args.list[i];
		}
	}
	return NULL;
}
static int argoptions(const char *const arg, struct NeArgOpt *opt) {
	struct NeArgOpt o = *opt;
	int c = 0;
	if (NeStrCmp(arg, 1, "-v", "-version", "--version", "-h", "-help", "--help", NULL)) {
		c = -1;
	} else if (NeStrCmp(arg, 0, "-q", "-quiet", NULL)) {
		o.quiet = !o.quiet; c = 1;
	} else if (NeStrCmp(arg, 0, "-a", "-auto", NULL)) {
		o.wisp = 0; o.weem = 0; o.bank = 0;
		o.autodetect = 1;
		c = 1;
	} else if (NeStrCmp(arg, 0, "-W", "-wisp", NULL)) {
		o.wisp = 1; o.weem = 0; o.bank = 0;
		o.autodetect = 0;
		c = 1;
	} else if (NeStrCmp(arg, 0, "-w", "-weem", NULL)) {
		o.wisp = 0; o.weem = 1; o.bank = 0;
		o.autodetect = 0;
		c = 1;
	} else if (NeStrCmp(arg, 0, "-b", "-bank", NULL)) {
		o.wisp = 0; o.weem = 0; o.bank = 1;
		o.autodetect = 0;
		c = 1;
	} else if (NeStrCmp(arg, 0, "-R", "-recurse", NULL)) {
		o.recurse = !o.recurse; c = 1;
	} else if (NeStrCmp(arg, 0, "-o", "-ogg", NULL)) {
		o.ogg = !o.ogg; c = 1;
	} else if (NeStrCmp(arg, 0, "-r", "-revorb", NULL)) {
		o.revorb = !o.revorb; c = 1;
	}
	*opt = o;
	return c;
}

void NePrintArg(struct NeArg a, NeSz pad) {
	NeNORMALN("%-7s arg '%s'%*s", getoptstr(a.options), a.arg.cstr, a.arg.length - pad, "");
	NeNORMALN(" with ");
	NeNORMALN("%-5s logging, ", a.options.quiet ? "quiet" : "");
	NeNORMALN("%-4s recursion, ", a.options.recurse ? "BANK" : "NO");
	NeNORMALN("%-6s ogging, ", a.options.ogg ? "AUTO" : "MANUAL");
	NeNORMALN("%-6s revorbing, ", a.options.revorb ? "AUTO" : "MANUAL");
	NeNORMALN(" and ");
	NeNORMALN("%-6s type detecting.", a.options.autodetect ? "AUTO" : "MANUAL");
	NeNORMAL("");
}

#define MAXFILES 1024
int NeParseArg(const char *const arg, struct NeArgOpt *opt, struct NeArgs *args)
{
	struct NeArgs gs = *args;
	struct NeArg ag = {0};
	{	int c = 0;
		if ((c = argoptions(arg, opt)))
			return c == 1 ? 0 : -2;
		/* If file previously passed, update settings */
		struct NeArg *t;
		if ((t = NeFindArg(gs, arg))) {
			t->options = *opt;
			return 0;
		}
	}

	NeStrNew(&ag.arg, arg, -1);
	ag.options = *opt;

	if (gs.argcount > MAXFILES)
		return -1;
	if (ag.arg.length > gs.maxarg)
		gs.maxarg = ag.arg.length;

	gs.argcount++;
	gs.list = NeSafeAlloc(gs.list, gs.argcount * sizeof(struct NeArg), 0);
	gs.list[gs.argcount - 1] = ag;
	*args = gs;
	return 0;
}

void NeProcessArg(struct NeArg a, NeSz maxarg) {
	/* with these, build list of ogg files to extract? */
	if (a.options.weem) {
		 /* extract ogg */
	//	NeNORMAL("Converting %s to ogg", a.arg.cstr);
	} else if (a.options.wisp) {
		/* do wisp extract */
	//	NeNORMAL("Extracting weems from wisp %s", a.arg.cstr);
	} else if (a.options.bank) {
		/* do bank extract */
	//	NeNORMALN("Extracting weems from bank %s", a.arg.cstr);
		if (a.options.recurse) {
			/* yada yada */
	//		NeNORMALN(" : Doing recursive extraction", a.arg.cstr);
		}
	//	NeNORMAL("");
	} else if (a.options.autodetect) {
	//	NeNORMAL("Autodetect %s", a.arg.cstr);
	} else {
		NeTRACE("FUCK");
	}
	NePrintArg(a, maxarg);
}
