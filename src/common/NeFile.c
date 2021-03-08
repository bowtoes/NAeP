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

static int nneopen(struct NeFile *f) {
	int code = NeERGNONE;
	if (f->mode == NeFileModeRead) {
		f->file = fopen(f->ppp.cstr, "rb");
	} else if (f->mode == NeFileModeWrite) {
		if (!f->stat.exist) {
			f->stat.isnew = 1;
			f->stat.exist = 1;
		}
		f->file = fopen(f->ppp.cstr, "wb+");
	} else if (f->mode == NeFileModeReadWrite) {
		if (!f->stat.exist) {
			f->stat.isnew = 1;
			f->stat.exist = 1;
			f->file = fopen(f->ppp.cstr, "wb+");
		} else {
			f->file = fopen(f->ppp.cstr, "rb+");
		}
	} else {
		code = NeERFMODE;
	}
	if (!f->file && code != NeERFMODE)
		code = NeERFOPEN;
	return code;
}

static NeBy nneeof(struct NeFile *f) {
	return f->stat.iseof = f->position >= f->size - 1;
}

int
NeFileStat(struct NeFileStat *fs, NeSz *fsize, const char *const path)
{
	int code = NeERGNONE;
	struct stat s;
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
		if (errno != ENOENT)
			code = NeERFSTAT;
		if (fsize) *fsize = 0;
	}
	return code;
}

int
NeFileOpen(struct NeFile *const file, const char *const path,
        enum NeFileMode mode)
{
	struct NeFile f = {0};
	int err = NeERGNONE;

	if (!file || !path || !*path)
		return err;

	NeStrNew(&f.ppp, path, -1);
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
		NeStrDel(&f.ppp);
		f.mode = NeFileModeNone;
		f.status = 0;
	}

	*file = f;
	return err;
}

int
NeFileClose(struct NeFile *const file)
{
	int code = NeERGNONE;
	if (!file || !file->file)
		return NeERGNONE;
	if (fclose(file->file) == 0) {
		file->file = NULL;
		file->position = 0;
		file->size = 0;
		file->status = 0;
	} else {
		NeERROR("Failed to close file %s : %m", file->ppp.cstr);
		code = NeERFFILE;
	}
	NeStrDel(&file->ppp);
	return code;
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
	NeStrCopy(&p, file->ppp);
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
	fseek(file->file, position, SEEK_SET);
	file->position = position;
	nneeof(file);
}

void
NeFileReset(struct NeFile *const file)
{
	struct NeFile *f = (struct NeFile *)file;
	if (!file || !f->file)
		return;

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
	rd = fread(blk, 1, dstlen, file->file);
	if (ferror(file->file) != 0) {
		NeERROR("Failed to read %zu bytes from %s : %m", dstlen, file->ppp.cstr);
		clearerr(file->file);
		rd = NeERFREAD;
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
        NeSz maxlen, NeSz start, NeSz end)
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
	if (rd != end - start) { /* no ferror */
		if (rv) { /* reverse read bytes */
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

	if ((wt = fwrite(data, 1, datalen, f->file)) != datalen) {
		NeERROR("Failed to write %zu bytes to %s : %m", datalen, f->ppp.cstr);
	}
	f->position += wt;
	nneeof(f);
	return wt;
}

int
NeFileRemove(struct NeFile *const file)
{
	int code = NeERGNONE;
	if (!file || !file->file)
		return 0;
	if (remove(file->ppp.cstr) != 0) {
		NeERROR("Could not remove %s : %m", file->ppp.cstr);
		if (!file->stat.exist)
			code = NeERFEXIST;
		else if (!file->stat.canwt)
			code = NeERFWRITE;
		else
			code = NeERFREMOVE;
	}
	NeFileClose(file);
	return code;
}

int
NeFileRename(struct NeFile *const file, const char *const newpath)
{
	int code = NeERGNONE;
	struct NeFileStat stat;
	if (!file || !newpath)
		return 0;
	NeFileStat(&stat, NULL, newpath);
	if (stat.exist) {
		NeERROR("Cannot rename %s to existing path %s", file->ppp.cstr, newpath);
		code = NeERFEXIST;
	} else if (!file->stat.exist) {
		NeERROR("Cannot rename non-existant file %s", file->ppp.cstr);
		code = NeERFEXIST;
	} else if (!file->stat.canwt) {
		NeERROR("Lacking write permissions for rename of %s", file->ppp.cstr);
		code = NeERFWRITEP;
	} else if (!file->stat.isreg) {
		NeERROR("Will not rename non-regular file %s", file->ppp.cstr);
		code = NeERFTYPE;
	} else if (rename(file->ppp.cstr, newpath) != 0) {
		NeERROR("Error renaming %s to %s : %m", file->ppp.cstr, newpath);
		code = NeERFRENAME;
	}
	return code;
}
