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

#include "wisp/NeWisp.h"

/* After each RIFF data, there is guaranteed (unless it's EOF) to be one other RIFF
 * chunk within 2048 bytes, because every RIFF chunk is padded to 2048-byte
 * boundaries */

#include <stdlib.h> /* realloc, free */

#include "common/NeDebugging.h"
#include "common/NeLogging.h"
#include "common/NeMisc.h"
#include "common/NeLibrary.h"
#include "common/NeErrors.h"

NeErr
NeWispRead(struct NeWisp *dst, struct NeFile *file) {
	void *blk;
	NeErr err = NeERGNONE;
	NeOf rd = 0;
	if (!dst || !file || !file->stat.exist)
		return err;

	blk = NeSafeAlloc(NULL, NeBLOCKSIZE, 1);
	do {
		NeOf pos = 0;
		if ((rd = NeFileStream(file, blk, NeBLOCKSIZE)) != NeERGNONE) {
			NeERROR("Error reading wisp");
			err = NeERWREAD;
		} else if ((pos = NeFind(blk, rd, "RIFF", 4, pos)) < 0) {
			err = NeERWREAD;
		} else if (pos < rd) {
			struct NeWem wem = {0};
			// read file offset and position into wsp.wems
			wem.offset = file->position - rd + pos;
			if (NeFileSegment(file, &wem.size, wem.offset + 4, wem.offset + 8, 4) != 4) {
				if (file->stat.iseof)
					NeWARNING("Unexpected EOF");
				else
					NeERROR("Error reading weem size : %m");
				err = NeERWREAD;
			} else if (wem.size > NeMAXWEEMSIZE) {
				NeERROR("Read weem size too large : %zu (0x%08x)", wem.size, wem.size);
				err = NeERWSIZE;
			} else {
				dst->wemCount++;
				dst->wems = NeSafeAlloc(dst->wems, dst->wemCount * sizeof(*dst->wems), 0);
				dst->wems[dst->wemCount - 1] = wem;
				NeFileJump(file, wem.offset + wem.size + 8 - 1);
			}
		}
	} while (rd > 0 && err == NeERGNONE);
	free(blk);
	if (err) {
		if (dst->wems)
			dst->wems = NeSafeAlloc(dst->wems, 0, 0);
	}
	return err;
}

NeErr
NeWispOpen(struct NeWisp *const dst, const char *const path)
{
	struct NeWisp wsp = {0};
	struct NeFile file = {0};
	NeErr err = NeERGNONE;
	NeOf rd = 0;
	void *blk;
	if (!dst || !path || !*path)
		return err;

	NeStrCopy(&wsp.path, NeStrShallow((char *const)path, -1));
	if (NeFileOpen(&file, path, NeFileModeRead) != 0) {
		NeERROR("Could not open file for new wisp %s", path);
		return NeERFFILE;
	}
	err = NeWispRead(&wsp, &file);
	NeFileClose(&file);
	return err;
}

void
NeWispDelete(struct NeWisp *const wisp)
{
	if (!wisp)
		return;
	NeStrDel(&wisp->path);
	wisp->wems = NeSafeAlloc(wisp->wems, 0, 0);
	wisp->wemCount = 0;
}

static NeErr savewem(struct NeFile *file, struct NeWem *wem, NeCt idx, NeCt maxidx, int autoogg) {
	NeErr err = NeERGNONE;
	NeBy *block = NULL;
	NeBy *data = NULL;
	NeOf rd = 0;
	struct NeStr outpath = {0};

	NeStrPrint(&outpath,
			NeStrRindex(file->path, NeStrShallow(".",1), file->path.length), -1,
			"_%*0u", NeDigitCount(maxidx),file->path.cstr, idx);
	NeStrMerge(&outpath, NeStrShallow(".wem", -1));
	// Allocate space for file data
	data = NeSafeAlloc(data, wem->size, 1);
	rd = 0;
	// Read data
	NeFileJump(file, wem->offset);
	rd = NeFileStream(file, data, wem->size);
	if (rd == wem->size) {
		// Extract/convert data
		struct NeFile output = {0};
		if ((err = NeFileOpen(&output, outpath.cstr, NeFileModeWrite)) == NeERGNONE) {
			NeFileWrite(&output, data, wem->size);
			NeFileClose(&output);
		}
	} else {
		err = NeERFFILE;
	}
	if (autoogg) {
		// ConvertWEM(outpath, inplace, yada);
	}

	block = NeSafeAlloc(block, 0, 0);
	data = NeSafeAlloc(data, 0, 0);
	NeStrDel(&outpath);
	return err;
}

NeErr
NeWispSave(const struct NeWisp *const wsp, int autoogg)
{
	NeErr err = NeERGNONE;
	struct NeFile file = {0};
	if (!wsp || !wsp->wemCount)
		return err;

	if ((err = NeFileOpen(&file, wsp->path.cstr, NeFileModeRead)) != NeERGNONE)
		return NeERFFILE;
	for (NeCt i = 0; i < wsp->wemCount; ++i) {
		savewem(&file, &wsp->wems[i], i, wsp->wemCount, autoogg);
	}
	NeFileClose(&file);
	return err;
}
