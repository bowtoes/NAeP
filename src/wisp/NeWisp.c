#include "wisp/NeWisp.h"

/* After each RIFF data, there is guaranteed (unless it's EOF) to be one other RIFF
 * chunk within 2048 bytes, because every RIFF chunk is padded to 2048-byte
 * boundaries */

#include <stdlib.h> /* realloc, free */

#include "common/NeDebugging.h"
#include "common/NeLogging.h"
#include "common/NeLibrary.h"
#include "common/NeErrors.h"

int
NeWispOpen(struct NeWisp *const dst, const char *const path)
{
	struct NeWisp wsp = {0};
	int err = 0;
	NeOf rd = 0;
	void *blk;
	if (!dst || !path || !*path)
		return 0;

	if (NeFileOpen(&wsp.file, path, NeFileModeRead) != 0) {
		NeERROR("Could not open file for new wisp %s", path);
		return -1;
	}
	blk = NeSafeAlloc(NULL, NeBLOCKSIZE, 1);
	do {
		NeSz pos = 0;
		rd = NeFileStream(&wsp.file, blk, NeBLOCKSIZE);
		if (rd == NeERFREAD) { /* ferror */
			NeERROR("Error reading wisp");
			err = NeERWREAD;
		} else if ((pos = NeFind(blk, rd, "RIFF", 4, pos)) != NeERGINVALID) {
			if (pos < rd) { /* read file offset and position into wsp.wems */
				struct NeWem w = {0};
				w.offset = wsp.file.position - rd + pos;
				if (NeFileSegment(&wsp.file, &w.size, 4, w.offset + 4, w.offset + 8) != 4) {
					if (wsp.file.stat.iseof)
						NeWARNING("Unexpected EOF");
					else
						NeERROR("Error reading weem size");
					err = NeERWREAD;
				} else if (w.size > NeMAXWEEMSIZE) {
					NeERROR("Read weem size too large : %zu (0x%08x)", w.size, w.size);
					err = NeERWSIZE;
				} else {
					wsp.wemCount++;
					wsp.wems = NeSafeAlloc(wsp.wems, wsp.wemCount * sizeof(*wsp.wems), 0);
					wsp.wems[wsp.wemCount - 1] = w;
					NeFileJump(&wsp.file, w.offset + w.size + 8 - 1);
				}
			}
		} else {
			err = NeERWREAD;
		}
	} while (rd > 0 && !err);
	if (wsp.wemCount == 0) {
		NeERROR("%s contains no wems", wsp.file.ppp.cstr);
		err = NeERWEMPTY;
	}

	free(blk);
	NeFileReset(&wsp.file);
	if (err) {
		if (wsp.wems)
			free(wsp.wems);
		NeFileClose(&wsp.file);
	} else {
		*dst = wsp;
	}
	return err;
}

void
NeWispClose(struct NeWisp *const wisp)
{
	if (!wisp)
		return;
	NeFileClose(&wisp->file);
	wisp->wems = NeSafeAlloc(wisp->wems, 0, 0);
	wisp->wemCount = 0;
}
