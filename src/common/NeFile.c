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

#include <stdint.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <errno.h>

#include <brrtools/brrplatform.h>
#include <brrtools/brrlib.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrmem.h>

const brrfccT WEEMCC = {.code=0x46464952};
const brrfccT BANKCC = {.code=0x44484b42};
const brrfccT OGGSCC = {.code=0x5367674f};

const char *const NeFileModeStr[] = {
	"invalid operation",
	"reading",
	"writing",
	"reading+writing",
};

static NeErrT nneopen(struct NeFile *f) {
	NeErrT err = NeERGNONE;
	if (f->mode == NeFileModeRead) {
		f->file = fopen(brrstr_cstr(&f->path), "rb");
	} else if (f->mode == NeFileModeWrite) {
		if (!f->stat.exist) {
			f->stat.isnew = 1;
			f->stat.exist = 1;
		}
		f->file = fopen(brrstr_cstr(&f->path), "wb+");
	} else if (f->mode == NeFileModeReadWrite) {
		if (!f->stat.exist) {
			f->stat.isnew = 1;
			f->stat.exist = 1;
			f->file = fopen(brrstr_cstr(&f->path), "wb+");
		} else {
			f->file = fopen(brrstr_cstr(&f->path), "rb+");
		}
	} else {
		err = NeERFMODE;
	}
	if (!f->file && err != NeERFMODE)
		err = NeERFOPEN;
	return err;
}

/* Update NeFile EOF indicator */
static brrby nneeof(struct NeFile *f) {
	return f->stat.iseof = f->position >= f->size - 1;
}

NeErrT
NeFileStat(struct NeFileStat *fs, brrsz *fsize, const char *const path)
{
	struct stat s;
	NeErrT err = NeERGNONE;
	if (!fs)
		return NeERGNONE;
	*fs = (struct NeFileStat){0};
	if (stat(path, &s) == 0) {
		fs->exist = 1;
		fs->canrd = fs->canwt = fs->isreg = 1;
		/* TODO windows version
		fs->isreg = S_ISREG(s.st_mode);
		fs->canwt = ((s.st_mode & S_IWUSR) && s.st_uid == getuid()) ||
			((s.st_mode & S_IWGRP) && s.st_gid == getgid());
		fs->canrd = ((s.st_mode & S_IRUSR) && s.st_uid == getuid()) ||
			((s.st_mode & S_IRGRP) && s.st_gid == getgid());
		*/
		if (fsize) *fsize = s.st_size;
	} else {
		// File not existing is not an error.
		if (errno != ENOENT)
			err = NeERFSTAT;
		if (fsize) *fsize = 0;
	}
	return err;
}

NeErrT
NeFileOpen(struct NeFile *const file, const char *const path,
        enum NeFileMode mode)
{
	struct NeFile f = {0};
	NeErrT err = NeERGNONE;

	if (!file || !path || !*path)
		return err;

	f.path = brrstr_new(path, -1);
	f.mode = mode;
	if ((err = NeFileStat(&f.stat, &f.size, path)) != NeERGNONE) {
		BRRLOG_ERR("Could not stat %s : %m", path);
	} else {
		if (!f.stat.isreg && f.stat.exist) {
			BRRLOG_ERR("%s is not a regular file", path);
			err = NeERFTYPE;
		} else if ((err = nneopen(&f)) != NeERGNONE) {
			if (err == NeERFMODE)
				BRRLOG_ERR("Invalid mode passed to open for %s : %i", path, mode);
			else
				BRRLOG_ERR("Failed to open %s for %s : %m", path, NeFileModeStr[mode]);
			err = NeERFOPEN;
		} else {
			nneeof(&f);
		}
	}

	if (err) {
		brrbuffer_delete(&f.path);
		f.mode = NeFileModeNone;
		f.status = 0;
	}

	*file = f;
	return err;
}

NeErrT
NeFileClose(struct NeFile *const file)
{
	NeErrT err = NeERGNONE;
	if (!file || !file->file)
		return NeERGNONE;
	if (fclose(file->file) == 0) {
		file->file = NULL;
		file->position = 0;
		file->size = 0;
		file->status = 0;
	} else {
		BRRLOG_ERR("Failed to close file %s : %m", brrstr_cstr(&file->path));
		err = NeERFFILE;
	}
	brrbuffer_delete(&file->path);
	return err;
}

void
NeFileReopen(struct NeFile *const file, enum NeFileMode newmode)
{
	brrstrT p = {0};
	if (!file)
		return;
	if (newmode == file->mode) {
		NeFileReset(file);
		return;
	}
	p = brrbuffer_copy(&file->path);
	NeFileClose(file);
	NeFileOpen(file, brrstr_cstr(&p), newmode);
	brrbuffer_delete(&p);
}

void
NeFileSkip(struct NeFile *const file, brrof bytes)
{
	struct NeFile *f = (struct NeFile *)file;
	brrsz newpos;
	if (!bytes || !file || !f->file)
		return;
	newpos = brrlib_wrap(bytes + (brrof)file->position, file->size, 0);
	// TODO fseek portability?
	fseek(f->file, newpos, SEEK_SET);
	f->position = newpos;
	nneeof(f);
}

void
NeFileJump(struct NeFile *const file, brrsz position)
{
	if (!file || !file->file || file->position == position)
		return;
	if (position >= file->size) {
		BRRLOG_WAR("Jumped past EOF");
		BRRLOG_DEB("POS 0x%08x SZ 0x%08x OFS 0x%08x", position, file->size, file->position);
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

brrof
NeFileStream(struct NeFile *const file, void *dst, brrsz dstlen)
{
	brrof rd = 0;
	void *blk = NULL;
	if (!file || !file->file || !dst || !dstlen)
		return 0;

	if (!brrlib_alloc(&blk, dstlen, 1))
		return -1;
	// TODO are fread, ferror, clearerr cross-platform/portable?
	rd = fread(blk, 1, dstlen, file->file);
	if (ferror(file->file) != 0) {
		BRRLOG_ERR("Failed to read %zu bytes from %s : %m", dstlen, brrstr_cstr(&file->path));
		clearerr(file->file);
		rd = -1;
	} else { /* copy tmp into dst */
		brrmem_copy(dst, dstlen, blk, rd);
		file->position += rd;
	}
	brrlib_alloc(&blk, 0, 0);

	nneeof(file);
	return rd;
}

brrof
NeFileSegment(struct NeFile *const file, void *dest,
        brrsz end, brrsz start, brrsz maxlen)
{
	brrof rd = 0;
	brrsz first = 0;
	brrsz rv = 0;
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
			brrmem_static_reverse((brrby *)dest + start, rd);
		}
	}
	NeFileJump(file, first);
	return rd;
}

brrsz
NeFileWrite(struct NeFile *const file, const void *const data, brrsz datalen)
{
	brrsz wt = 0;
	struct NeFile *f = (struct NeFile *)file;
	if (!file || !f->file || !(f->mode & NeFileModeWrite) || !data || !datalen)
		return 0;

	// TODO is fwrite cross-platform/portable?
	if ((wt = fwrite(data, 1, datalen, f->file)) != datalen) {
		BRRLOG_ERR("Failed to write %zu bytes to %s : %m", datalen, brrstr_cstr(&f->path));
	}
	f->position += wt;
	nneeof(f);
	return wt;
}

NeErrT
NeFileRemove(struct NeFile *const file)
{
	brrstrT path = {0};
	NeErrT err = NeERGNONE;
	if (!file || !file->file)
		return err;
	path = brrbuffer_copy(&file->path);
	if ((err = NeFileClose(file)) == NeERGNONE) {
		// TODO remove is not cross-platform/portable
		if ((err = remove(brrstr_cstr(&path))) != 0) {
			BRRLOG_ERR("Could not remove %s : %m", brrstr_cstr(&file->path));
			err = NeERFREMOVE;
		}
	}
	brrbuffer_delete(&path);
	return err;
}

NeErrT
NeFileRename(const char *const oldpath, const char *const newpath)
{
	struct NeFileStat stat;
	NeErrT err = NeERGNONE;
	if (!oldpath || !newpath)
		return NeERFPATH;
	if ((err = NeFileStat(&stat, NULL, oldpath)) == NeERGNONE) {
		if (!stat.exist) {
			BRRLOG_ERR("Cannot rename non-existant path %s", oldpath);
			return NeERFEXIST;
		} else if (!stat.isreg) {
			BRRLOG_ERR("Will not rename non-regular file %s", newpath);
			return NeERFMODE;
		}
	} else {
		return NeERFSTAT;
	}
	if ((err = NeFileStat(&stat, NULL, newpath)) == NeERGNONE) {
		if (stat.exist) {
			BRRLOG_ERR("Cannot rename %s to existing path %s", oldpath, newpath);
			return NeERFEXIST;
		// TODO rename is not cross-platform/portable
		} else if (rename(oldpath, newpath) != 0) {
			BRRLOG_ERR("Error renaming %s to %s : %m", oldpath, newpath);
			return NeERFRENAME;
		}
	} else {
		return NeERFSTAT;
	}
	return err;
}
