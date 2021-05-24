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
void NePrintArg(struct NeArg arg, NeSz maxarg, int newline);

void  NeDetectType(struct NeArg *arg, struct NeFile *f);
NeErr NeConvertWeem(struct NeArg arg, struct NeFile *infile);
NeErr NeExtractWisp(struct NeArg arg, struct NeFile *infile);
NeErr NeExtractBank(struct NeArg arg, struct NeFile *infile);
NeErr   NeRevorbOgg(struct NeArg arg, struct NeFile *infile);

#endif /* NeArg_h */
