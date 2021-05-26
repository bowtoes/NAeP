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

#include <brrtools/brrtypes.h>

#include "common/NeFile.h"

struct NeArgs {
	struct NeArg {
		/* file to process */
		brrstrT arg;
		union {
			struct NeArgOpt {
				brrby oggs:1;
				brrby weem:1;
				brrby wisp:1;
				brrby bank:1;        /* b=4 */

				brrby logdebug:1;
				brrby loglevel:3;    /* b=8 */
				brrby logoff:1;
				brrby logcolor:1;

				brrby bankrecurse:1;

				brrby autoogg:1;     /* b=12 */
				brrby ogginplace:1;
				brrby autorvb:1;
				brrby rvbinplace:1;

				brrby dryrun:1;      /* b=16 */

			} opt;
			brru2 options;
		};
	} *args;
	brrsz maxarg;
	brrct argcount;
	brrby argdigit;
};

struct NeArg *NeFindArg(struct NeArgs args, const char *const arg);
void NePrintArg(struct NeArg arg, brrsz maxarg, int newline);

void  NeDetectType(struct NeArg *arg, struct NeFile *f);
NeErrT NeConvertWeem(struct NeArg arg, struct NeFile *infile);
NeErrT NeExtractWisp(struct NeArg arg, struct NeFile *infile);
NeErrT NeExtractBank(struct NeArg arg, struct NeFile *infile);
NeErrT   NeRevorbOgg(struct NeArg arg, struct NeFile *infile);

#endif /* NeArg_h */
