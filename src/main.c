#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <alloca.h>

#include "common/NeDebugging.h"
#include "common/NeLibrary.h"
#include "common/NeMisc.h"
#include "wisp/NeWisp.h"

#if 0
int main (int argc, char **argv)
{
	char a[] = "ABCDEF";
	NeDEBUG("%s", a);
	NeReverse((NeBy *)a, 6);
	NeDEBUG("%s", a);

	return 0;
}
#else
int main(int argc, char **argv)
{
	struct NeWisp *wisps = NULL;
	int count = 0;
	for (int i = 1; i < argc; ++i) {
		struct NeWisp w;
		if (NeWispOpen(&w, argv[i]) == 0) {
			count++;
			wisps = NeSafeAlloc(wisps, count * sizeof(*wisps), 0);
			wisps[count - 1] = w;
		}
	}

	NeNORMAL("Open %i files", count);
	for (int i = 0, err = 0; i < count && !err; ++i) {
		struct NeWisp wsp = wisps[i];
		NeSz flen = NeStrlen(wsp.file.path, NeMAXPATH);
		NeSz dgt = NeDigitCount(wsp.wemCount);
		char template[NeMAXPATH];
		void *blk = NeSafeAlloc(NULL, NeBLOCKSIZE, 1);
		NeCopy((NeBy *)template, NeRfind(wsp.file.path, flen, ".", 1, 0), (NeBy *)wsp.file.path, flen, 0, -1);

		for (int j = 0; j < wsp.wemCount; ++j) {
			struct NeWem w = wsp.wems[j];
			struct NeFile output;
			NeSz wrt = 0;
			char name[NeMAXPATH];
			snprintf(name, NeMAXPATH, "%s_%0*u.wem", template, dgt, j);
			/* time to write */
			if (NeFileOpen(&output, name, NeModeWrite) != 0) {
				NeERROR("Could not open wem for writing");
				continue;
			}
			while (wrt < w.size + 8) {
				NeSz rd = NeFileSegment(&wsp.file, blk, NeBLOCKSIZE, w.offset + wrt, w.offset + w.size + 8);
				if (rd == -1) {
					NeFileClose(&output);
					err = 1;
					break;
				}
				wrt += NeFileWrite(&output, blk, rd);
			}
			NeFileClose(&output);
			if (!err)
				NeNORMAL("Extracted %s", name);
		}
		free(blk);
		NeWispClose(&wisps[i]);
	}
	return 0;
}
#endif
