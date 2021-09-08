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

#ifndef PROCESS_FILES_H
#define PROCESS_FILES_H

#include <brrtools/brrapi.h>

#include "common.h"

BRRCPPSTART

int BRRCALL regrain_ogg(numbersT *const numbers, const char *const input, brrsz input_length,
    const input_optionsT *const options);
int BRRCALL convert_wem(numbersT *const numbers, const char *const input, brrsz input_length,
    const input_optionsT *const options, input_libraryT *const library);
int BRRCALL extract_wsp(numbersT *const numbers, const char *const input, brrsz input_length,
    const input_optionsT *const options, input_libraryT *const library);
int BRRCALL extract_bnk(numbersT *const numbers, const char *const input, brrsz input_length,
    const input_optionsT *const options, input_libraryT *const library);

BRRCPPEND

#endif /* PROCESS_FILES_H */
