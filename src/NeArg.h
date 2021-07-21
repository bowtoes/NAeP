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

#include <brrtools/brrapi.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrstg.h>
#include <brrtools/brrpath.h>

BRRCPPSTART

typedef struct NeArgOptions {
	union {
		struct {
			brrby oggs:1;
			brrby weem:1;
			brrby wisp:1;
			brrby bank:1;        /* b=4 */

			brrby autoOgg:1;
			brrby oggInplace:1;
			brrby autoRevorb:1;
			brrby revorbInplace:1;  /* b=8 */

			brrby recurseBanks:1;
			brrby dryRun:1;

			brrby logDebug:1;
			brrby logEnabled:1;
			brrby logColorEnabled:1;
		};
		brru2 optionsInt;
	};
	brrlog_priorityT logPriority;
} NeArgOptionsT;
typedef struct NeArg {
	brrstgT argument;
	NeArgOptionsT options;
} NeArgT;

typedef struct NeArgArray {
	NeArgT *args;
	brrct arg_count;
	brrby arg_digit; /* ??? */
} NeArgArrayT;

NeArgT *BRRCALL NeFindArg(NeArgArrayT args, const char *const arg);
void BRRCALL NePrintArg(NeArgT arg, int newline, brrsz max_arg_length);

/* TODO */
void BRRCALL NeDetectType(NeArgT *arg, const brrpath_stat_resultT *const st, const char *const path);
int BRRCALL NeConvertWeem(const NeArgT *const arg, const brrpath_stat_resultT *const st, const char *const path);
int BRRCALL NeExtractWisp(const NeArgT *const arg, const brrpath_stat_resultT *const st, const char *const path);
int BRRCALL NeExtractBank(const NeArgT *const arg, const brrpath_stat_resultT *const st, const char *const path);
int BRRCALL NeRevorbOgg(const NeArgT *const arg, const brrpath_stat_resultT *const st, const char *const path);
/* TODO */

BRRCPPEND

#endif /* NeArg_h */
