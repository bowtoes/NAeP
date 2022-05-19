/*
Copyright 2021-2022 BowToes (bow.toes@mailfence.com)

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
