#ifndef PROCESS_FILES_H
#define PROCESS_FILES_H

#include <brrtools/brrapi.h>

#include "common.h"

BRRCPPSTART

int BRRCALL revorb_ogg(numbersT *const numbers, int dry_run, const char *const path,
    int inplace_revorb);

int BRRCALL convert_wem(numbersT *const numbers, int dry_run, const char *const path,
    int inplace_revorb, int auto_revorb,
	int inplace_ogg);

int BRRCALL extract_wsp(numbersT *const numbers, int dry_run, const char *const path,
    int inplace_revorb, int auto_revorb,
	int inplace_ogg, int auto_ogg);

int BRRCALL extract_bnk(numbersT *const numbers, int dry_run, const char *const path,
    int inplace_revorb, int auto_revorb,
	int inplace_ogg, int auto_ogg,
	int bnk_recurse);

BRRCPPEND

#endif /* PROCESS_FILES_H */
