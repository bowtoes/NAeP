#ifndef NAeP_typedefs_h
#define NAeP_typedefs_h

#include <brrtools/brrtypes.h>
#include <brrtools/brrstringr.h>

#define _tdef(_struct_) typedef struct _struct_ _struct_##_t

_tdef(packed_codebook);
_tdef(codebook_library);

_tdef(wwise_vorb);
_tdef(wwise_fmt);
_tdef(wwise_flags);
_tdef(wwriff);

typedef brru4 nefilter_index;
_tdef(nefilter);
_tdef(neinput);
_tdef(neinput_library);
_tdef(nestate_stat);
_tdef(nestate);

_tdef(nepath);

typedef void *(*riff_copier_t)(void *const, const void *const, size_t);
_tdef(riff_datasync);
_tdef(riff_chunkstate);
_tdef(riff_basic_chunk);
_tdef(riff_list_chunk);
_tdef(riff);

_tdef(riffgeometry);
_tdef(rifflist);

#undef _tdef

#endif /* NAeP_typedefs_h */
