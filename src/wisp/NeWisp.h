#ifndef NeWisp_h
#define NeWisp_h

#include "common/NeFile.h"

struct NeWem {
	NeU4 offset;
	NeU4 size;
};

struct NeWisp {
	struct NeFile file;
	struct NeWem *wems;
	NeU2 wemCount;
};

/* Initialize wisp and count weem files; returns NULL if not wisp or on error*/
int NeWispOpen(struct NeWisp *const dest, const char *const path);
/* close wisp file and free all associated memory */
int NeWispClose(struct NeWisp *const wisp);

#endif /* NeWisp_h */
