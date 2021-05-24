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

#include "common/NeFile.h"
#include "common/NePlatform.h"

#include <stdint.h>
#include <stdlib.h>
#if defined(NePLATFORMTYPE_POSIX)
#include <sys/stat.h>
#include <errno.h>
#endif

#include "common/NeDebugging.h"
#include "common/NeLogging.h"
#include "common/NeLibrary.h"
#include "common/NeMisc.h"
#include "common/NeStr.h"

const NeFcc WEEMCC = 0x46464952;
const NeFcc BANKCC = 0x44484b42;
const NeFcc OGGSCC = 0x5367674f;

const char *const NeFileModeStr[] = {
	"invalid operation",
	"reading",
	"writing",
	"reading+writing",
};

static NeErr nneopen(struct NeFile *f) {
	NeErr err = NeERGNONE;
	if (f->mode == NeFileModeRead) {
		f->file = fopen(f->path.cstr, "rb");
	} else if (f->mode == NeFileModeWrite) {
		if (!f->stat.exist) {
			f->stat.isnew = 1;
			f->stat.exist = 1;
		}
		f->file = fopen(f->path.cstr, "wb+");
	} else if (f->mode == NeFileModeReadWrite) {
		if (!f->stat.exist) {
			f->stat.isnew = 1;
			f->stat.exist = 1;
			f->file = fopen(f->path.cstr, "wb+");
		} else {
			f->file = fopen(f->path.cstr, "rb+");
		}
	} else {
		err = NeERFMODE;
	}
	if (!f->file && err != NeERFMODE)
		err = NeERFOPEN;
	return err;
}

/* Update NeFile EOF indicator */
static NeBy nneeof(struct NeFile *f) {
	return f->stat.iseof = f->position >= f->size - 1;
}

NeErr
NeFileStat(struct NeFileStat *fs, NeSz *fsize, const char *const path)
{
	struct stat s;
	NeErr err = NeERGNONE;
	if (!fs)
		return NeERGNONE;
	*fs = (struct NeFileStat){0};
	if (stat(path, &s) == 0) {
		fs->exist = 1;
		fs->isreg = S_ISREG(s.st_mode);
		fs->canwt = ((s.st_mode & S_IWUSR) && s.st_uid == getuid()) ||
			((s.st_mode & S_IWGRP) && s.st_gid == getgid());
		fs->canrd = ((s.st_mode & S_IRUSR) && s.st_uid == getuid()) ||
			((s.st_mode & S_IRGRP) && s.st_gid == getgid());
		if (fsize) *fsize = s.st_size;
	} else {
		// File not existing is not an error.
		if (errno != ENOENT)
			err = NeERFSTAT;
		if (fsize) *fsize = 0;
	}
	return err;
}

NeErr
NeFileOpen(struct NeFile *const file, const char *const path,
        enum NeFileMode mode)
{
	struct NeFile f = {0};
	NeErr err = NeERGNONE;

	if (!file || !path || !*path)
		return err;

	NeStrNew(&f.path, path, -1);
	f.mode = mode;
	if ((err = NeFileStat(&f.stat, &f.size, path)) != NeERGNONE) {
		NeERROR("Could not stat %s : %m", path);
	} else {
		if (!f.stat.isreg && f.stat.exist) {
			NeERROR("%s is not a regular file", path);
			err = NeERFTYPE;
		} else if ((err = nneopen(&f)) != NeERGNONE) {
			if (err == NeERFMODE)
				NeERROR("Invalid mode passed to open for %s : %i", path, mode);
			else
				NeERROR("Failed to open %s for %s : %m", path, NeFileModeStr[mode]);
			err = NeERFOPEN;
		} else {
			nneeof(&f);
		}
	}

	if (err) {
		NeStrDel(&f.path);
		f.mode = NeFileModeNone;
		f.status = 0;
	}

	*file = f;
	return err;
}

NeErr
NeFileClose(struct NeFile *const file)
{
	NeErr err = NeERGNONE;
	if (!file || !file->file)
		return NeERGNONE;
	if (fclose(file->file) == 0) {
		file->file = NULL;
		file->position = 0;
		file->size = 0;
		file->status = 0;
	} else {
		NeERROR("Failed to close file %s : %m", file->path.cstr);
		err = NeERFFILE;
	}
	NeStrDel(&file->path);
	return err;
}

void
NeFileReopen(struct NeFile *const file, enum NeFileMode newmode)
{
	struct NeStr p = {0};
	if (!file)
		return;
	if (newmode == file->mode) {
		NeFileReset(file);
		return;
	}
	NeStrCopy(&p, file->path);
	NeFileClose(file);
	NeFileOpen(file, p.cstr, newmode);
	NeStrDel(&p);
}

void
NeFileSkip(struct NeFile *const file, NeOf bytes)
{
	struct NeFile *f = (struct NeFile *)file;
	NeSz newpos;
	if (!bytes || !file || !f->file)
		return;
	newpos = NeSmartMod(bytes + (NeOf)file->position, file->size, 0);
	// TODO fseek portability?
	fseek(f->file, newpos, SEEK_SET);
	f->position = newpos;
	nneeof(f);
}

void
NeFileJump(struct NeFile *const file, NeSz position)
{
	if (!file || !file->file || file->position == position)
		return;
	if (position >= file->size) {
		NeWARNING("Jumped past EOF");
		NeDEBUG("POS 0x%08x SZ 0x%08x OFS 0x%08x", position, file->size, file->position);
	}
	file->position = position;
	// TODO rewind portability?
	if (!position)
		rewind(file->file);
	else
		fseek(file->file, position, SEEK_SET);
	nneeof(file);
}

void
NeFileReset(struct NeFile *const file)
{
	struct NeFile *f = (struct NeFile *)file;
	if (!file || !f->file)
		return;

	// TODO fflush portability?
	fflush(file->file);
	NeFileJump(file, 0);
}

NeOf
NeFileStream(struct NeFile *const file, void *dst, NeSz dstlen)
{
	NeOf rd = 0;
	void *blk;
	if (!file || !file->file || !dst || !dstlen)
		return 0;

	blk = NeSafeAlloc(NULL, dstlen, 1);
	// TODO are fread, ferror, clearerr cross-platform/portable?
	rd = fread(blk, 1, dstlen, file->file);
	if (ferror(file->file) != 0) {
		NeERROR("Failed to read %zu bytes from %s : %m", dstlen, file->path.cstr);
		clearerr(file->file);
		rd = -1;
	} else { /* copy tmp into dst */
		NeCopy(dst, dstlen, blk, rd);
		file->position += rd;
	}
	blk = NeSafeAlloc(blk, 0, 0);

	nneeof(file);
	return rd;
}

NeOf
NeFileSegment(struct NeFile *const file, void *dest,
        NeSz end, NeSz start, NeSz maxlen)
{
	NeOf rd = 0;
	NeSz first = 0;
	int rv = 0;
	if (!file || !file->file || !dest || !maxlen || start == end)
		return 0;

	if (end < start) {
		rv = end;
		end = start;
		start = rv;
	}
	if (end - start > maxlen)
		end = start + maxlen;

	first = file->position;
	NeFileJump(file, start);
	rd = NeFileStream(file, dest, end - start);
	if (rd > 0) { // no ferror
		if (rv) { // reverse read bytes
			NeBy *const d = (NeBy *const)dest;
			for (NeSz i = 0; i < rd/2; ++i) {
				NeSWAP(d[start + i], d[start + rd - 1 - i]);
			}
		}
	}
	NeFileJump(file, first);
	return rd;
}

NeSz
NeFileWrite(struct NeFile *const file, const void *const data, NeSz datalen)
{
	NeSz wt = 0;
	struct NeFile *f = (struct NeFile *)file;
	if (!file || !f->file || !(f->mode & NeFileModeWrite) || !data || !datalen)
		return 0;

	// TODO is fwrite cross-platform/portable?
	if ((wt = fwrite(data, 1, datalen, f->file)) != datalen) {
		NeERROR("Failed to write %zu bytes to %s : %m", datalen, f->path.cstr);
	}
	f->position += wt;
	nneeof(f);
	return wt;
}

NeErr
NeFileRemove(struct NeFile *const file)
{
	struct NeStr path = {0};
	NeErr err = NeERGNONE;
	if (!file || !file->file)
		return err;
	NeStrCopy(&path, file->path);
	if ((err = NeFileClose(file)) == NeERGNONE) {
		// TODO remove is not cross-platform/portable
		if ((err = remove(path.cstr)) != 0) {
			NeERROR("Could not remove %s : %m", file->path.cstr);
			err = NeERFREMOVE;
		}
	}
	NeStrDel(&path);
	return err;
}

NeErr
NeFileRename(const char *const oldpath, const char *const newpath)
{
	struct NeFileStat stat;
	NeErr err = NeERGNONE;
	if (!oldpath || !newpath)
		return NeERFPATH;
	if ((err = NeFileStat(&stat, NULL, oldpath)) == NeERGNONE) {
		if (!stat.exist) {
			NeERROR("Cannot rename non-existant path %s", oldpath);
			return NeERFEXIST;
		} else if (!stat.isreg) {
			NeERROR("Will not rename non-regular file %s", newpath);
			return NeERFMODE;
		}
	} else {
		return NeERFSTAT;
	}
	if ((err = NeFileStat(&stat, NULL, newpath)) == NeERGNONE) {
		if (stat.exist) {
			NeERROR("Cannot rename %s to existing path %s", oldpath, newpath);
			return NeERFEXIST;
		// TODO rename is not cross-platform/portable
		} else if (rename(oldpath, newpath) != 0) {
			NeERROR("Error renaming %s to %s : %m", oldpath, newpath);
			return NeERFRENAME;
		}
	} else {
		return NeERFSTAT;
	}
	return err;
}
