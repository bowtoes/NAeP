#ifndef NeFile_h
#define NeFile_h

#include <stdio.h>
#include "common/NeTypes.h"
#include "common/NeStr.h"
#include "common/NeErrors.h"

/* TODO some kind of buffer system? not sure, but definetely
 * little endian/big endian specification; default is system default?
 * argument to pass? bitstreams? huh? */

extern const NeFcc WEEMCC;
extern const NeFcc BANKCC;
extern const NeFcc OGGSCC;

/* Modes with which to open files (always in binary) */
enum NeFileMode {
	NeFileModeNone      = 0, // Passing to open functions will error
	NeFileModeRead      = 1, // Readonly mode
	NeFileModeWrite     = 2, // Read-write mode; file truncated
	NeFileModeReadWrite = 3, // Read-write mode; file unchanged
};
extern const char *const NeFileModeStr[];

/* Structure for more portalizable file operations */
struct NeFile {
	struct NeStr path;
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

NeErr NeFileStat(struct NeFileStat *stat, NeSz *fsize, const char *const path);

/* Open file for reading/writing */
/* Returns -1 on error */
NeErr NeFileOpen(struct NeFile *const file, const char *const path,
        enum NeFileMode mode);

/* Closes file */
/* Fails gracefully*/
NeErr NeFileClose(struct NeFile *const file);
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
        NeSz start, NeSz end, NeSz maxlen);

/* Writes datalen bytes into file at file position */
/* Returns new file position */
NeSz NeFileWrite(struct NeFile *const file, const void *const data, NeSz datalen);

/* Closes the file and removes it from disk */
NeErr NeFileRemove(struct NeFile *const file);
NeErr NeFileRename(const char *const file, const char *const newpath);
#endif /* NeFile_h */
