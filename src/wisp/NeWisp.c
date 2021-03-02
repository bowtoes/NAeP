#include "wisp/NeWisp.h"

#include <stdlib.h> /* realloc, free */

#include "common/NeDebugging.h"
#include "common/NeLogging.h"
#include "common/NeLibrary.h"

/* After each RIFF data, there is guaranteed (unless it's EOF) to be one other RIFF
 * chunk within 2048 bytes, because every RIFF chunk is padded to 2048-byte
 * boundaries */
int
NeWispOpen(struct NeWisp *const dst, const struct NeStr path)
{
	struct NeWisp wsp = {0};
	int err = 0;
	NeOf rd = 0;
	void *blk;
	if (!dst || !path.length)
		return 0;

	if (NeFileOpen(&wsp.file, path, NeModeRead) != 0) {
		NeERROR("Could not open file for new wisp %s", path.cstr);
		return -1;
	}
	blk = NeSafeAlloc(NULL, NeBLOCKSIZE, 1);
	do {
		NeSz pos = 0;
		rd = NeFileStream(&wsp.file, blk, NeBLOCKSIZE);
		if (rd == -1) { /* ferror */
			NeERROR("Error reading wisp");
			err = 1;
			break;
		}
		pos = NeFind(blk, rd, "RIFF", 4, pos);
		if (pos == -1) { /* likely won't happen */
			NeERROR("Error finding weem in wisp");
			err = 1;
			break;
		}
		if (pos < rd) { /* read file offset and position into wsp.wems */
			struct NeWem w = {0};
			w.offset = wsp.file.position - rd + pos;
			if (NeFileSegment(&wsp.file, &w.size, 4, w.offset + 4, w.offset + 8) != 4) {
				if (NeFileEOF(wsp.file)) {
					NeERROR("Unexpected EOF");
				} else { /* ferror */
					NeERROR("Error reading weem size");
					err = 1;
				}
				break;
			}
			if (w.size > NeMAXWEEMSIZE) {
				NeERROR("Read weem size too large : %zu (0x%08x)", w.size, w.size);
				break;
			}
			wsp.wemCount++;
			wsp.wems = NeSafeAlloc(wsp.wems, wsp.wemCount * sizeof(*wsp.wems), 0);
			wsp.wems[wsp.wemCount - 1] = w;
			NeFileJump(&wsp.file, w.offset + w.size + 8 - 1);
		}
	} while (rd > 0);
	if (wsp.wemCount == 0) {
		NeERROR("%s contains no wems", wsp.file.ppp.cstr);
		err = 1;
	}

	blk = NeSafeAlloc(blk, 0, 0);
	NeFileReset(&wsp.file);
	if (err) {
		if (wsp.wems)
			free(wsp.wems);
		NeFileClose(&wsp.file);
		return -1;
	} else {
		*dst = wsp;
		return 0;
	}
}

int
NeWispClose(struct NeWisp *const wisp)
{
	if (!wisp)
		return 0;
	NeFileClose(&wisp->file);
	wisp->wems = NeSafeAlloc(wisp->wems, 0, 0);
	wisp->wemCount = 0;
	return 0;
}
