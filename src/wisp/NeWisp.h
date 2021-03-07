#ifndef NeWisp_h
#define NeWisp_h

#include "common/NeFile.h"

/* wisp errors */
/* no error */
#define NeWENONE NeFENONE
/* empty wisp */
#define NeWEEMPTY -1
/* generic wisp read error */
#define NeWEREAD -2
/* weem size error */
#define NeWESIZE -3

/* Appears to be max wisp file size in Automata */
#define NeMAXWEEMSIZE (1<<25)

/* Structure for storing weem data */
struct NeWeem {
	/* Where data is/will be stored on disk */
	struct NeStr outpath;
	/* Size of data */
	NeU4 size;
	/* Data of weem */
	NeBy *data;
};

/* Structure for storing position information of weems contained in wisps */
struct NeWisp {
	struct NeFile file;
	/* Storage for weem offset and size in wisp file */
	struct NeWem {
		NeU4 offset;
		/* Weem data size, excluding ckId and ckSz */
		NeU4 size;
	} *wems;
	NeCt wemCount;
};

/* Initialize wisp and count weem files */
/* Returns NULL if path not wisp or on error */
int NeWispOpen(struct NeWisp *const dst, const char *const path);
/* Close wisp file and free all associated memory */
void NeWispClose(struct NeWisp *const wisp);

#endif /* NeWisp_h */
