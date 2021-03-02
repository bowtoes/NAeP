#ifndef NeArg_h
#define NeArg_h

#include "common/NeStr.h"

struct NeArgs {
	struct NeArg {
		struct NeStr arg; /* actually file to process */
		union {
			struct NeArgOpt {
				NeBy quiet:1;
				NeBy wisp:1;
				NeBy weem:1;
				NeBy bank:1;
				NeBy recurse:1;
				NeBy ogg:1;
				NeBy revorb:1;
				NeBy autodetect:1;
			} options;
			NeBy opt;
		};
	} *list;
	NeSz maxarg;
	int argcount;
};

struct NeArg *NeFindArg(struct NeArgs args, const char *const arg);
void NePrintArg(struct NeArg a, NeSz pad);
int NeParseArg(const char *const arg, struct NeArgOpt *opt, struct NeArgs *args);
void NeProcessArg(struct NeArg a, NeSz maxarg);

#endif /* NeArg_h */
