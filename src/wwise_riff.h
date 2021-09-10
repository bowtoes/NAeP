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

#ifndef WWISE_RIFF_H
#define WWISE_RIFF_H

#include <ogg/ogg.h>

#include <brrtools/brrapi.h>

#include "codebook_library.h"
#include "common_input.h"
#include "riff.h"
#include "wwise.h"

BRRCPPSTART

#define VORBIS_STR "vorbis"
#define CODEBOOK_SYNC "BCV"

int BRRCALL wwise_riff_process(ogg_stream_state *const streamer, riffT *const rf,
    const input_optionsT *const options, codebook_libraryT *const library);

BRRCPPEND

#endif /* WWISE_RIFF_H */
