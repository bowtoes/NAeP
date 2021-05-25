/*
Copyright 2021 BowToes (bow.toes@mailfence.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef NeFile_h
#define NeFile_h

#include <stdio.h>
#include "common/NeStr.h"
#include "common/NeErrors.h"

/* TODO some kind of buffer system? not sure, but definetely
 * little endian/big endian specification; default is system default?
 * argument to pass? bitstreams? huh? */

extern const brrfccT WEEMCC;
extern const brrfccT BANKCC;
extern const brrfccT OGGSCC;

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
	brrsz size;
	brrsz position;
	enum NeFileMode mode;
	union {
		struct NeFileStat {
			brrby exist:1;
			brrby canrd:1;
			brrby canwt:1;
			brrby isreg:1;
			brrby isnew:1;
			brrby iseof:1;
		} stat;
		brrby status;
	};
};

NeErrT NeFileStat(struct NeFileStat *stat, brrsz *fsize, const char *const path);

/* Open file for reading/writing */
/* Returns -1 on error */
NeErrT NeFileOpen(struct NeFile *const file, const char *const path,
        enum NeFileMode mode);

/* Closes file */
/* Fails gracefully*/
NeErrT NeFileClose(struct NeFile *const file);
/* Reopens given file with given mode */
void NeFileReopen(struct NeFile *const file, enum NeFileMode newmode);

/* Skips file position forward/backward by bytes, with wrap-around */
void NeFileSkip(struct NeFile *const file, brrof bytes);
/* Jumps to offset in file, no wrap-around */
void NeFileJump(struct NeFile *const file, brrsz position);
/* Flushes file and sets position to beginning of file */
void NeFileReset(struct NeFile *const file);

/* Read dstlen bytes into buffer and return bytes successfully read */
/* File position is updated by bytes read */
/* No wrap around
*/
/* If feof, output is truncated */
/* If ferror, buffer is unchanged and returns -1 */
brrof NeFileStream(struct NeFile *const file, void *dst, brrsz dstlen);
/* Same as NeFileStream, but read from start to end */
/* If start > end, read in reverse */
/* File position is unchanged */
brrof NeFileSegment(struct NeFile *const file, void *dst,
        brrsz start, brrsz end, brrsz maxlen);

/* Writes datalen bytes into file at file position */
/* Returns new file position */
brrsz NeFileWrite(struct NeFile *const file, const void *const data, brrsz datalen);

/* Closes the file and removes it from disk */
NeErrT NeFileRemove(struct NeFile *const file);
NeErrT NeFileRename(const char *const file, const char *const newpath);
#endif /* NeFile_h */
