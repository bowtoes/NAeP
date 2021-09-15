#ifndef PROCESS_H
#define PROCESS_H

#include "input.h"

int neprocess_inputs(nestateT *const state, neinput_libraryT *const libraries, neinputT *const inputs);

/* Implemented in 'process/' directory */
int neregrain_ogg(nestateT *const state, const neinputT *const input);
int neconvert_wem(nestateT *const state, neinput_libraryT *const libraries, const neinputT *const input);
int neextract_wsp(nestateT *const state, neinput_libraryT *const libraries, const neinputT *const input);
int neextract_bnk(nestateT *const state, neinput_libraryT *const libraries, const neinputT *const input);

#endif /* PROCESS_H */
