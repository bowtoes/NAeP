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

#ifndef NeWisp_h
#define NeWisp_h

#include "common/NeFile.h"

/* Appears to be max wisp file size in Automata */
#define NeMAXWEEMSIZE (1<<25)

/* Structure for storing position information of weems contained in wisps */
struct NeWisp {
	struct NeStr path;
	struct NeWem {
		brrsz offset;
		brrsz size;
	} *wems; // Storage for weem offset and size in wisp file
	brrct wemCount;
};

NeErrT NeWispRead(struct NeWisp *dst, struct NeFile *file);
/* Initialize wisp and count weem files */
NeErrT NeWispOpen(struct NeWisp *const dst, const char *const path);
/* Close wisp file and free all associated memory */
void NeWispDelete(struct NeWisp *const wisp);

/* Save out all wems stored into seperate files */
NeErrT NeWispSave(const struct NeWisp *const wsp, int autoogg);

#endif /* NeWisp_h */
