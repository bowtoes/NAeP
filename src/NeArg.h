#ifndef NeArg_h
#define NeArg_h

#include "common/NeTypes.h"
#include "common/NeLogging.h"
#include "common/NeFile.h"

struct NeArgs {
	struct NeArg {
		/* file to process */
		struct NeStr arg;
		union {
			struct NeArgOpt {
				NeBy oggs:1;
				NeBy weem:1;
				NeBy wisp:1;
				NeBy bank:1;        /* b=4 */

				NeBy logdebug:1;
				NeBy loglevel:3;    /* b=8 */
				NeBy logoff:1;
				NeBy logcolor:1;

				NeBy bankrecurse:1;

				NeBy autoogg:1;     /* b=12 */
				NeBy ogginplace:1;
				NeBy autorvb:1;
				NeBy rvbinplace:1;

				NeBy dryrun:1;      /* b=16 */

			} opt;
			NeU2 options;
		};
	} *args;
	NeSz maxarg;
	NeCt argcount;
	NeBy argdigit;
};

struct NeArg *NeFindArg(struct NeArgs args, const char *const arg);
void NePrintArg(struct NeArg arg, NeSz maxarg);

void  NeDetectType(struct NeArg *arg, struct NeFile *f);
NeErr NeConvertWeem(struct NeArg arg, struct NeFile *f);
NeErr NeExtractWisp(struct NeArg arg, struct NeFile *f);
NeErr NeExtractBank(struct NeArg arg, struct NeFile *f);
NeErr   NeRevorbOgg(struct NeArg arg, struct NeFile *f);

#endif /* NeArg_h */
