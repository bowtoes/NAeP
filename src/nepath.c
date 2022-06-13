#include "nepath.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int
nepath_init(nepath_t *const path, const char *const arg)
{
	if (!path || !arg)
		return -1;

	nepath_t p = {.cstr=arg, .length=brrstringr_length(arg, BRRPATH_MAX_PATH)};
	{
		if (brrpath_stat(&p.st, &(brrstringr_t){.cstr=(char*)p.cstr,.length=p.length})) {
			Err(,"Failed to stat '%s': %s (%d)", arg, strerror(errno), errno);
			return -1;
		}
	}
	*path = p;
	return 0;
}

int
nepath_read(const nepath_t *const path, void **const buffer)
{
	if (!path || !buffer)
		return -1;
	if (!path->cstr) {
		Err(,"Tried to read from unintialized path.");
		return -1;
	}
	if (path->st.type != brrpath_type_file) {
		Err(,"Can't read from '%s': Not a file");
		return -1;
	}

	void *data = malloc(path->st.size);
	if (!data) {
		Err(,"Failed to allocate %zu bytes for reading '%s': %s (%d)", path->st.size, path->cstr, strerror(errno), errno);
		return -1;
	}
	{
		FILE *file;
		if (!(file = fopen(path->cstr, "rb"))) {
			Err(,"Failed to open '%s' for reading: %s (%d)", path->cstr, strerror(errno), errno);
			free(data);
			return -1;
		}

		brrsz read;
		if (path->st.size > (read = fread(data, 1, path->st.size, file))) {
			if (feof(file)) {
				Err(,"Input file '%s' was truncated, expected %zu bytes and got %zu", path->cstr, path->st.size, read);
			} else {
				Err(,"Could not read %zu bytes from '%s': %s (%d)", path->st.size, path->cstr, strerror(errno), errno);
			}
			fclose(file);
			free(data);
			return -1;
		}
		fclose(file);
	}
	*buffer = data;
	return 0;
}

static inline brrsz
i_ext_index(const char *const str, brrsz length)
{
	const char *n = NULL;
	for (const char *i = str + length; i > str; --i) {
		char c = *(i-1);
		if (c == '.') {
			n = i - 1;
			break;
		}
		if (c == BRRPATH_SEP_CHR)
			break;
#ifdef BRRPLATFORMTYPE_Windows
		if (c == '/')
			break;
#endif
	}
	if (!n)
		return length;
	return n - str;
}

int
nepath_extension_cmp(const nepath_t *const path, const char **match, ...)
{
	if (!path || !path->cstr)
		return -1;

	brrsz dot = i_ext_index(path->cstr, path->length);
	if (dot == path->length)
		return -1;

	int idx = -1;
	{
		va_list lptr;
		va_start(lptr, match);
		int i = 0;
		const char *extension = path->cstr + dot;
		const char *a;
		while ((a = va_arg(lptr, const char *))) {
			if (0 == brrpathcmp(a, extension)) {
				if (match)
					*match = a;
				idx = i;
			}
			++i;
		}
		va_end(lptr);
	}
	if (match && idx == -1)
		*match = NULL;
	return idx;
}

int
nepath_extension_replace(
    const nepath_t *restrict const path,
    const char *restrict const newext,
    int extlen,
    char *restrict const dst
)
{
	if (!path || !path->cstr)
		return -1;
	if (!dst)
		return 0;
	brrsz dot = i_ext_index(path->cstr, path->length);
	memcpy(dst, path->cstr, dot);
	if (newext && extlen)
		memcpy(dst + dot, newext, extlen);
	dst[dot + extlen] = 0;
	return 0;
}
