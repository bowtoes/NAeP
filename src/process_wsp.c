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
extract_wsp(numbersT *const numbers, const processed_inputT *const input, input_libraryT *const libraries)
{
	int err = 0;
	numbers->wsps_to_process++;
	if (input->options.dry_run) {
		BRRLOG_FOREP(DRY_COLOR, " Extract WSP (dry)");
	} else {
		NeTODO("Implement 'extract_wsp' priority 2 ");
		BRRLOG_FOREP(WET_COLOR, " Extracting WSP...");
		/* for each 'wem' in 'path':
		 *   extract 'wem' to 'path_base_%0*d.wem'
		 *   auto_ogg: convert_wem(numbers, 'path_base_%0*d.wem', ...);
		 * */
	}
	if (!err) {
		numbers->wsps_processed++;
	}
	return err;
}
