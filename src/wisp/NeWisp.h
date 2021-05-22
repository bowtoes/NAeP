#ifndef NeWisp_h
#define NeWisp_h

#include "common/NeFile.h"

/* Appears to be max wisp file size in Automata */
#define NeMAXWEEMSIZE (1<<25)

/* Structure for storing position information of weems contained in wisps */
struct NeWisp {
	struct NeStr path;
	struct NeWem {
		NeSz offset;
		NeSz size;
	} *wems; // Storage for weem offset and size in wisp file
	NeCt wemCount;
};

NeErr NeWispRead(struct NeWisp *dst, struct NeFile *file);
/* Initialize wisp and count weem files */
NeErr NeWispOpen(struct NeWisp *const dst, const char *const path);
/* Close wisp file and free all associated memory */
void NeWispDelete(struct NeWisp *const wisp);

/* Save out all wems stored into seperate files */
NeErr NeWispSave(const struct NeWisp *const wsp, int autoogg);

#endif /* NeWisp_h */
