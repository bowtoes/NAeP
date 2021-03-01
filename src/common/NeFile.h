#ifndef NeFile_h
#define NeFile_h

#include <stdio.h>
#include "common/NeTypes.h"
#include "common/NeStr.h"

/* Files always in binary mode */
/* ModeRead: Read-only; if file doesn't exist, error */
/* ModeCreate: Same as mode 'w' */
/* ModeReadWrite: Same as mode 'r+'; if file doesn't exist, open with 'w+' */
enum NeFileMode {
	NeModeNone      = 0 << 0,
	NeModeRead      = 1 << 0,
	NeModeWrite     = 1 << 1,
	NeModeReadWrite = NeModeRead | NeModeWrite,
};

#define NeMAXPATH 260
struct NeFile {
	struct NeStr ppp;
	FILE *file;
	NeSz size;
	NeSz position;
	enum NeFileMode mode;
};

/* Open file for reading/writing; get filesize */
int NeFileOpen(struct NeFile *const file, const struct NeStr path,
        enum NeFileMode mode);

/* Skips file position forward/backward by bytes, with wrap-around */
void NeFileSkip(struct NeFile *const file, NeOf bytes);
/* Jumps to offset in file, no wrap-around */
void NeFileJump(struct NeFile *const file, NeSz position);
/* Flushes file and sets position to beginning of file */
void NeFileReset(struct NeFile *const file);

/* Read dstlen bytes into buffer and return bytes successfully read
 * File position is updated by bytes read
 * No wrap around
 *
 * If feof, output is truncated
 * If ferror, buffer is unchanged and returns -1 */
NeOf NeFileStream(struct NeFile *const file, void *dst, NeSz dstlen);
/* Same as NeFileStream, but read from start to end
 * If end < start, read in reverse
 * File position is unchanged */
NeOf NeFileSegment(struct NeFile *const file, void *dst,
        NeSz maxlen, NeSz start, NeSz end);

/* Writes datalen bytes into file at file position; returns new file position */
NeSz NeFileWrite(struct NeFile *const file, const void *const data, NeSz datalen);

/* Checks if file position is at end of file */
int NeFileEOF(struct NeFile file);

/* Closes file */
int NeFileClose(struct NeFile *const file);
int NeFileReopen(struct NeFile *const file, enum NeFileMode newmode);
#endif /* NeFile_h */
