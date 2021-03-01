#ifndef NeWisp_h
#define NeWisp_h

#include "common/NeFile.h"

#define NeBLOCKSIZE 2048
/* what looks to be max wisp file size in data files */
#define NeMAXWEEMSIZE (1<<25)

struct NeWeem {
	struct NeStr outpath;
	NeU4 size;
	NeBy *data;
};

struct NeWisp {
	struct NeFile file;
	struct NeWem {
		NeU4 offset;
		NeU4 size;
	} *wems;
	NeU2 wemCount;
};

/* Initialize wisp and count weem files; returns NULL if not wisp or on error*/
int NeWispOpen(struct NeWisp *const dst, const struct NeStr path);
/* close wisp file and free all associated memory */
int NeWispClose(struct NeWisp *const wisp);

#endif /* NeWisp_h */
