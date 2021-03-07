#ifndef NeFile_h
#define NeFile_h

#include <stdio.h>
#include "common/NeTypes.h"
#include "common/NeStr.h"

/* file errors */
/* no error */
#define NeFENONE 0
/* file doesn't exist */
#define NeFEEXIST -1
/* file isn't regular */
#define NeFEREG -2
/* generic stat error */
#define NeFESTAT -3
/* invalid mode */
#define NeFEMODE -4
/* could not open */
#define NeFEOPEN -5
/* generic read error */
#define NeFEREAD -6
/* read permission error */
#define NeFERPERM -7
/* write permission error */
#define NeFEWPERM -8

extern const NeFcc WEEMCC;
extern const NeFcc BANKCC;
extern const NeFcc OGGSCC;

/* Modes with which to open files (always in binary) */
enum NeFileMode {
	/* Passing to open functions will error */
	NeFileModeNone = 0 << 0,
	/* Readonly mode */
	NeFileModeRead = 1 << 0,
	/* Read-write mode; file truncated */
	NeFileModeWrite = 1 << 1,
	/* Read-write mode; file unchanged */
	NeFileModeReadWrite = NeFileModeRead | NeFileModeWrite,
};
extern const char *const NeFileModeStr[];

/* Structure for more portalizable file operations */
struct NeFile {
	struct NeStr ppp;
	FILE *file;
	NeSz size;
	NeSz position;
	enum NeFileMode mode;
	union {
		struct NeFileStat {
			NeBy exist:1;
			NeBy canrd:1;
			NeBy canwt:1;
			NeBy isreg:1;
			NeBy isnew:1;
			NeBy iseof:1;
		} stat;
		NeBy status;
	};
};

int NeFileStat(struct NeFileStat *stat, NeSz *fsize, const char *const path);

/* Open file for reading/writing */
/* Returns -1 on error */
int NeFileOpen(struct NeFile *const file, const char *const path,
        enum NeFileMode mode);

/* Closes file */
/* Fails gracefully*/
void NeFileClose(struct NeFile *const file);
/* Reopens given file with given mode */
void NeFileReopen(struct NeFile *const file, enum NeFileMode newmode);

/* Skips file position forward/backward by bytes, with wrap-around */
void NeFileSkip(struct NeFile *const file, NeOf bytes);
/* Jumps to offset in file, no wrap-around */
void NeFileJump(struct NeFile *const file, NeSz position);
/* Flushes file and sets position to beginning of file */
void NeFileReset(struct NeFile *const file);

/* Read dstlen bytes into buffer and return bytes successfully read */
/* File position is updated by bytes read */
/* No wrap around
*/
/* If feof, output is truncated */
/* If ferror, buffer is unchanged and returns -1 */
NeOf NeFileStream(struct NeFile *const file, void *dst, NeSz dstlen);
/* Same as NeFileStream, but read from start to end */
/* If start > end, read in reverse */
/* File position is unchanged */
NeOf NeFileSegment(struct NeFile *const file, void *dst,
        NeSz maxlen, NeSz start, NeSz end);

/* Writes datalen bytes into file at file position */
/* Returns new file position */
NeSz NeFileWrite(struct NeFile *const file, const void *const data, NeSz datalen);
#endif /* NeFile_h */
