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

#include "process_files.h"

#include <brrtools/brrlog.h>

int BRRCALL
extract_bnk(numbersT *const numbers, const char *const input, brrsz input_length,
    const input_optionsT *const options, input_libraryT *const library)
{
	int err = 0;
	numbers->bnks_to_process++;
	if (options->dry_run) {
		BRRLOG_FOREP(DRY_COLOR, " Extract BNK (dry)");
	} else {
		NeTODO("Implement 'extract_bnk' priority ZZZ (sleeping) ");
		BRRLOG_FOREP(WET_COLOR, " Extracting BNK...");
		/* Very similar to 'extract_wsp', however banks may reference other banks and wsps.
		 * TODO: How should this be done?
		 * Hold off until the rest are done.
		 * */
	}
	if (!err) {
		numbers->bnks_processed++;
	}
	return err;
}
