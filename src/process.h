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

#ifndef NAeP_neprocess_h
#define NAeP_neprocess_h

#include "neutil.h"

int neprocess_inputs(nestate_t *const state);

int neprocess_ogg(nestate_t *const state, const neinput_t *const input);
int neprocess_wem(nestate_t *const state, const neinput_t *const input);
int neprocess_wsp(nestate_t *const state, const neinput_t *const input);
int neprocess_bnk(nestate_t *const state, const neinput_t *const input);
int neprocess_arc(nestate_t *const state, const neinput_t *const input);

#endif /* NAeP_neprocess_h */
