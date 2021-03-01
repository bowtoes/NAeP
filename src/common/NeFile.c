#include "common/NeFile.h"

#include <stdint.h>
#include <stdlib.h>
#include "common/NePlatform.h"
#include "common/NeLibrary.h"
#include "common/NeDebugging.h"
#include "common/NeMisc.h"
#include "common/NeStr.h"

/* these will probably need platform specific impls? */
#if defined(NePLATFORMTYPE_POSIX)
#define NeFOPEN(path, mode) fopen(path, mode)
#define NeFCLOSE(file) fclose(file)
#define NeFSEEK(file, to, wh) fseeko(file, to, wh)
#define NeFTELL(file) ftello(file)
#define NeFREAD(into, count, file) fread(into, 1, count, file)
#define NeFWRITE(from, count, file) fwrite(from, 1, count, file)
#define NeFFLUSH(file) fflush(file)
#define NeFEOF(file) feof(file)
#define NeFERROR(file) ferror(file)
#else
#error Unsupported platform
#endif

static const char *nnemodestring(enum NeFileMode mode)
{
	switch (mode) {
		case NeModeRead:
			return "reading";
		case NeModeWrite:
			return "writing";
		case NeModeReadWrite:
			return "reading+writing";
		default:
			return "invalid operation";
	}
}

/* TODO SEEK_END is not universal */
static NeSz nnefilesize(FILE *file)
{
	NeSz s = 0;
	NeSz p = NeFTELL(file);
	NeFSEEK(file, 0, SEEK_END);
	s = NeFTELL(file);
	NeFSEEK(file, p, SEEK_SET);
	return s;
}
/* TODO platform specific impls */
#if defined(NePLATFORMTYPE_POSIX)
#include <sys/stat.h>
#include <errno.h>
#else
#error Unsupported platform
#endif
static int nnestat(struct NeFile *const file)
{
#if defined(NePLATFORMTYPE_POSIX)
	struct stat s;
	if (stat(file->ppp.cstr, &s) == -1) {
		if (errno != ENOENT || !file->ppp.length)
			NeTRACE("Could not stat %s : %m", file->ppp.cstr);
		file->size = 0;
		return 0;
	}
	if (!S_ISREG(s.st_mode))
		return -1;
	file->size = s.st_size;
	return 0;
#else
#error Unsupported platform
#endif
}
static int nneopen(struct NeFile *f)
{
	switch (f->mode) {
		case NeModeRead:
			f->file = NeFOPEN(f->ppp.cstr, "rb");
			break;
		case NeModeWrite:
			f->file = NeFOPEN(f->ppp.cstr, "wb+");
			break;
		case NeModeReadWrite:
			f->file = NeFOPEN(f->ppp.cstr, "rb+");
			if (!f->file)
				f->file = NeFOPEN(f->ppp.cstr, "wb+");
			break;
		default:
			NeERROR("Invalid file open mode specified %i", f->mode);
			return -1;
	}
	return 0;
}

int
NeFileOpen(struct NeFile *const file, const struct NeStr path,
        enum NeFileMode mode)
{
	struct NeFile f = {0};

	if (!file || !path.length)
		return 0;
	if (path.length > NeMAXPATH) {
		NeERROR("File path %s must have less than %u characters", path, NeMAXPATH);
		return -1;
	}
	NeStrCopy(&f.ppp, path);
	if (nnestat(&f) != 0) {
		NeERROR("Could not stat %s : %m", f.ppp.cstr);
		NeStrDel(&f.ppp);
		return -1;
	}
	f.mode = mode;
	if (nneopen(&f) == -1) {
		NeERROR("Failed to open %s for %s : %m", path, nnemodestring(mode));
		NeStrDel(&f.ppp);
		return -1;
	}
	*file = f;
	return 0;
}

void
NeFileSkip(struct NeFile *const file, NeOf bytes)
{
	struct NeFile *f = (struct NeFile *)file;
	NeSz newpos;
	if (!bytes || !file || !f->file)
		return;
	newpos = NeSmartMod(bytes + (NeOf)file->position, file->size, 0);
	NeFSEEK(f->file, newpos, SEEK_SET);
	f->position = newpos;
}

void
NeFileJump(struct NeFile *const file, NeSz position)
{
	struct NeFile *f = (struct NeFile *)file;
	if (!file || !f->file || f->position == position)
		return;
	if (position >= f->size) {
		NeWARNING("Jumped past EOF");
		NeDEBUG("POS 0x%08x SZ 0x%08x OFS 0x%08x", position, f->size, f->position);
		position = f->size - 1;
	}
	NeFSEEK(f->file, position, SEEK_SET);
	f->position = position;
}

void
NeFileReset(struct NeFile *const file)
{
	struct NeFile *f = (struct NeFile *)file;
	if (!file || !f->file)
		return;

	NeFFLUSH(file->file);
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
	rd = NeFREAD(blk, dstlen, file->file);
	if (NeFERROR(file->file)) {
		NeERROR("Failed to read %zu bytes from %s : %m", dstlen, file->ppp.cstr);
		rd = -1;
	} else { /* copy tmp into dst */
		NeCopy(dst, dstlen, blk, rd);
		file->position += rd;
	}
	blk = NeSafeAlloc(blk, 0, 0);

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
	if (!file || !f->file || !(f->mode & NeModeWrite) || !data || !datalen)
		return 0;

	if ((wt = NeFWRITE(data, datalen, f->file)) != datalen) {
		NeERROR("Failed to write %zu bytes to %s : %m", datalen, f->ppp.cstr);
	}
	f->position += wt;
	return wt;
}

int
NeFileEOF(const struct NeFile file)
{
	return file.position == file.size - 1;
}

int
NeFileClose(struct NeFile *const file)
{
	if (!file || !file->file)
		return 0;
	if (NeFCLOSE(file->file) != 0)
		NeTRACE("Failed to close file %s : %m", file->ppp.cstr);
	file->file = NULL;
	file->position = 0;
	file->size = 0;
	NeStrDel(&file->ppp);
	return 0;
}

int
NeFileReopen(struct NeFile *const file, enum NeFileMode newmode)
{
	struct NeStr p = {0};
	if (!file)
		return 0;
	if (newmode == file->mode) {
		NeFileReset(file);
		return 0;
	}
	NeStrCopy(&p, file->ppp);
	NeFileClose(file);
	NeFileOpen(file, p, newmode);
	NeStrDel(&p);
	return 0;
}
