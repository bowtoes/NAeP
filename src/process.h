/* Copyright (c), bowtoes (bow.toes@mailfence.com)
Apache 2.0 license, http://www.apache.org/licenses/LICENSE-2.0
Full license can be found in 'license' file */

#ifndef NAeP_neprocess_h
#define NAeP_neprocess_h

#include "nefcc.h"

extern const fcc_t fcc_OggS;
extern const fcc_t fcc_BKHD;

typedef struct neinput neinput_t;
typedef struct nestate nestate_t;
int neprocess_inputs(nestate_t *const state);

int neprocess_ogg(nestate_t *const state, const neinput_t *const input);
int neprocess_wem(nestate_t *const state, const neinput_t *const input);
int neprocess_wsp(nestate_t *const state, const neinput_t *const input);
int neprocess_bnk(nestate_t *const state, const neinput_t *const input);
int neprocess_arc(nestate_t *const state, const neinput_t *const input);

#endif /* NAeP_neprocess_h */
