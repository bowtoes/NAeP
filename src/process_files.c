#include "process_files.h"

#include <stdio.h>

int BRRCALL
revorb_ogg(numbersT *const numbers, int dry_run, const char *const path,
    int inplace_revorb)
{
	int err = 0;
	numbers->oggs_to_revorb++;
	if (dry_run) {
		BRRLOG_FOREP(DRY_COLOR, "Revorb OGG");
	} else {
		NeTODO("Implement 'revorb_ogg' priority 2 ");
		BRRLOG_FOREP(WET_COLOR, "Revorbing OGG...");
		/* revorb to 'path_base.rvb.ogg'
		 * inplace_revorb: replace 'path' with 'path_base.rvb.ogg'
		 * */
	}
	if (!err) {
		numbers->oggs_revorbed++;
	}
	return err;
}

int BRRCALL
convert_wem(numbersT *const numbers, int dry_run, const char *const path,
    int inplace_revorb, int auto_revorb,
	int inplace_ogg)
{
	int err = 0;
	numbers->wems_to_convert++;
	if (dry_run) {
		BRRLOG_FOREP(DRY_COLOR, " Convert WEM");
	} else {
		NeTODO("Implement 'convert_wem' priority 3 ");
		BRRLOG_FOREP(WET_COLOR, "Converting WEM...");
		/* convert to 'path_base.ogg'
		 * inplace_ogg: replace 'path' with 'path_base.ogg'
		 * auto_revorb: revorb_ogg(numbers, 'path_base.ogg' or 'path', ...);
		 * */
	}
	if (!err) {
		numbers->wems_converted++;
	}
	return err;
}

int BRRCALL
extract_wsp(numbersT *const numbers, int dry_run, const char *const path,
    int inplace_revorb, int auto_revorb,
	int inplace_ogg, int auto_ogg)
{
	int err = 0;
	numbers->wsps_to_process++;
	if (dry_run) {
		BRRLOG_FOREP(DRY_COLOR, " Extract WSP");
	} else {
		NeTODO("Implement 'extract_wsp' priority 1 ");
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

int BRRCALL
extract_bnk(numbersT *const numbers, int dry_run, const char *const path,
    int inplace_revorb, int auto_revorb,
	int inplace_ogg, int auto_ogg,
	int bnk_recurse)
{
	int err = 0;
	numbers->bnks_to_process++;
	if (dry_run) {
		BRRLOG_FOREP(DRY_COLOR, " Extract BNK");
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
