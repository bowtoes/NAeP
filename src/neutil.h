#ifndef NAeP_neutil_h
#define NAeP_neutil_h

#include <brrtools/brrlog.h>

#include "typedefs.h"

extern int neerror; // todo

int print_usage(void);
int print_help(void);

#define nestylefg_e(_type_) nestylefg_##_type_
#define nestylebg_e(_type_) nestylebg_##_type_
#define nestylest_e(_type_) nestylest_##_type_
#define nestylefn_e(_type_) nestylefn_##_type_
#define nestylefg(_type_) ((brrlog_color_t)nestylefg_e(_type_))
#define nestylebg(_type_) ((brrlog_color_t)nestylebg_e(_type_))
#define nestylest(_type_) ((brrlog_style_t)nestylest_e(_type_))
#define nestylefn(_type_) ((brrlog_font_t)nestylefn_e(_type_))
#define nestyleprm(_type_) nestylefg(_type_), nestylebg(_type_), nestylest(_type_), nestylefn(_type_)
#define nestylefmt(_type_) ((brrlog_format_t){nestyleprm(_type_)})

#define _style_def(_type_, _fg_, _bg_, _st_, _fn_) \
typedef enum nestyle_##_type_ {\
	nestylefg_e(_type_) = brrlog_color_##_fg_,\
	nestylebg_e(_type_) = brrlog_color_##_bg_,\
	nestylest_e(_type_) = brrlog_style_##_st_,\
	nestylefn_e(_type_) = brrlog_font_##_fn_,\
} nestyle_##_type_##_t

_style_def(normal, normal, normal, normal, normal);
_style_def(last,   last,   last,   last,   last);

_style_def(ft_ogg,  blue,    normal, normal, normal);
_style_def(ft_wem,  green,   normal, normal, normal);
_style_def(ft_wsp,  yellow,  normal, normal, normal);
_style_def(ft_bnk,  red,     normal, normal, normal);
_style_def(ft_auto, magenta, normal, bold,   normal);

_style_def(meta_success, green,  normal, bold,   normal);
_style_def(meta_failure, red,    normal, bold,   normal);
_style_def(meta_dry,     yellow, normal, bold,   normal);
_style_def(meta_wet,     cyan,   normal, normal, normal);

_style_def(extra_manual,   red,     normal, normal, normal);
_style_def(extra_inplace,  yellow,  normal, normal, normal);
_style_def(extra_separate, cyan,    normal, normal, normal);

_style_def(extra_enabled,  green,   normal, normal, normal);
_style_def(extra_disabled, red,     normal, normal, normal);
_style_def(extra_path,     cyan,    normal, normal, normal);
_style_def(extra_info,     magenta, normal, normal, normal);

#define _log(_level_, _style_, ...)   BRRLOG_MESSAGET(gbrrlog_level(_level_), nestylefmt(_style_), __VA_ARGS__)
#define _logn(_level_, _style_, ...)  BRRLOG_MESSAGETN(gbrrlog_level(_level_), nestylefmt(_style_), __VA_ARGS__)
#define _logp(_level_, _style_, ...)  BRRLOG_MESSAGETP(gbrrlog_level(_level_), nestylefmt(_style_), __VA_ARGS__)
#define _lognp(_level_, _style_, ...) BRRLOG_MESSAGETNP(gbrrlog_level(_level_), nestylefmt(_style_), __VA_ARGS__)

#define Style(_variant_, _style_, ...)   _log##_variant_(last, _style_, __VA_ARGS__)
#define Log(_variant_, _level_, ...)     _log##_variant_(_level_, last, __VA_ARGS__)

#define Cri(_variant_, ...) Log(_variant_, critical, __VA_ARGS__)
#define Err(_variant_, ...) Log(_variant_, error,    __VA_ARGS__)
#define War(_variant_, ...) Log(_variant_, warning,  __VA_ARGS__)
#define Nor(_variant_, ...) Log(_variant_, normal,   __VA_ARGS__)
#define Deb(_variant_, ...) Log(_variant_, debug,    __VA_ARGS__)
#define Lst(_variant_, ...) Log(_variant_, last,     __VA_ARGS__)

#ifdef NAeP_extra_debug
#define ExtraCri(_variant_, ...) Log(_variant_, critical, __VA_ARGS__)
#define ExtraErr(_variant_, ...) Log(_variant_, error,    __VA_ARGS__)
#define ExtraWar(_variant_, ...) Log(_variant_, warning,  __VA_ARGS__)
#define ExtraNor(_variant_, ...) Log(_variant_, normal,   __VA_ARGS__)
#define ExtraDeb(_variant_, ...) Log(_variant_, debug,    __VA_ARGS__)
#define ExtraLst(_variant_, ...) Log(_variant_, last,     __VA_ARGS__)
#else
#define ExtraCri(_variant_, ...)
#define ExtraErr(_variant_, ...)
#define ExtraWar(_variant_, ...)
#define ExtraNor(_variant_, ...)
#define ExtraDeb(_variant_, ...)
#define ExtraLst(_variant_, ...)
#endif

#define ExtraPlace() do { ExtraDeb(,"Function '%s', in '%s' @ %i", __func__, __FILE__, __LINE__); } while (0)

typedef struct fcc {
	struct {
		unsigned r:1;
		unsigned n:3;
	};
	union {
		struct {
			brru1 _0;
			brru1 _1;
			brru1 _2;
			brru1 _3;
		};
		struct {
			brru1 _0;
			brru1 _1;
			brru1 _2;
			brru1 _3;
		} v;
		brru4 u;
		brrs4 s;
	};
} fcc_t;

#include <brrtools/brrmacro.h>

#define _fcc0()                 {0}
#define _fcc1(_0_)              {._0=(_0_)}
#define _fcc2(_0_,_1_)          {._0=(_0_),._1=(_1_)}
#define _fcc3(_0_,_1_,_2_)      {._0=(_0_),._1=(_1_),._2=(_2_)}
#define _fcc4(_0_,_1_,_2_,_3_)  {._0=(_0_),._1=(_1_),._2=(_2_),._3=(_3_)}
#define _fcc0B()                {0}
#define _fcc1B(_0_)             {._3=(_0_)}
#define _fcc2B(_0_,_1_)         {._3=(_0_),._2=(_1_)}
#define _fcc3B(_0_,_1_,_2_)     {._3=(_0_),._2=(_1_),._1=(_2_)}
#define _fcc4B(_0_,_1_,_2_,_3_) {._3=(_0_),._2=(_1_),._1=(_2_),._0=(_3_)}

#define _fcc(_B_, ...) {.n=brr_narg(__VA_ARGS__),.r=brr_narg(_B_),.v=brr_join(brr_join(_fcc,brr_narg(__VA_ARGS__)),_B_)(__VA_ARGS__)}

#define fcc_chr(...)  _fcc( ,__VA_ARGS__)
#define fcc_chrB(...) _fcc(B,__VA_ARGS__)

#define _str_split(_s_) (_s_)[0], (_s_)[1], (_s_)[2], (_s_)[3]

#define _selector0(_x_)
#define _selector1(_x_) _x_
#define _select(_x_) brr_join(_selector,brr_narg(_x_))(_x_)

#define fcc_str(_f_,_s_)  fcc_chr( _str_split(_s_ _select(_f_) "\0\0\0\0"))
#define fcc_strB(_f_,_s_) fcc_chrB(_str_split(_s_ _select(_f_) "\0\0\0\0"))

#define _fcc_split(_f_) (_f_)._0, (_f_)._1, (_f_)._2, (_f_)._3
#define _fcc_splitB(_f_) (_f_)._3, (_f_)._2, (_f_)._1, (_f_)._0

#define fcc_int(_i_) {.u=(brru4)(_i_)}
#define fcc_intB(_i_) {.v={_fcc_splitB((fcc_t)fcc_int(_i_))}}

#define fcc_arr(_a_)  fcc_chr( _str_split(_a_))
#define fcc_arrB(_a_) fcc_chrB(_str_split(_a_))

extern const fcc_t fcc_OggS;
extern const fcc_t fcc_BKHD;
extern const fcc_t fcc__z;

extern const fcc_t fcc_RIFF;
extern const fcc_t fcc_RIFX;
extern const fcc_t fcc_XFIR;
extern const fcc_t fcc_FFIR;

/* Returns 0 when equal, <0 when a < b, >0 when a > b  */
int
fcccmp(fcc_t a, fcc_t b);

typedef enum nefilter_type {
	nefilter_white = 0,
	nefilter_black,
} nefilter_type_t;

/* TODO Eventually index white/blackisting can be replaced by a more flexible filtering system, similar to in 'countwsp' */
struct nefilter {
	nefilter_index *list;
	brru4 count;
	nefilter_type_t type;
};

int
nefilter_init(nefilter_t *const filter, const char *const arg);

void
nefilter_clear(nefilter_t *const filter);

int
nefilter_contains(const nefilter_t *const filter, brru4 index);

#include <ogg/ogg.h>

#define E_OGG_SUCCESS        ( 0)
#define E_OGG_FAILURE        (-1)
#define E_OGG_OUT_SUCCESS    ( 1)
#define E_OGG_OUT_INCOMPLETE ( 0)
#define E_OGG_OUT_DESYNC     (-1)
#define E_VORBIS_HEADER_SUCCESS   (0)
#define E_VORBIS_HEADER_FAULT     (OV_EFAULT)
#define E_VORBIS_HEADER_NOTVORBIS (OV_ENOTVORBIS)
#define E_VORBIS_HEADER_BADHEADER (OV_EBADHEADER)

#define nepack_unpack oggpack_read

inline brru4
nepack_pack(oggpack_buffer *const packer, brru4 value, int bits)
{
	oggpack_write(packer, value, bits);
	return value;
}

/* Read 'unpack' bits from 'unpacker', and place 'pack' bits from that into 'packer'.
 * Returns the value unpacked, or -1 on error. */
long long
nepack_transfer(oggpack_buffer *const unpacker, int unpack, oggpack_buffer *const packer, int pack);

/* Transfer what data remains in 'unpacker' into 'packer'.
 * Returns the number of bits transferred, or -1 on error.*/
long long
nepack_transfer_remaining(oggpack_buffer *const unpacker, oggpack_buffer *const packer);

/* Transfer 'bits' bits from 'unpacker' into 'packer' in 32-bit word chunks.
 * Returns the number of bits transferred, or -1 on error.*/
long long
nepack_transfer_lots(oggpack_buffer *const unpacker, oggpack_buffer *const packer, unsigned long bits);

int
neutil_count_ones(unsigned long x);

int
neutil_count_bits(unsigned long x);

long
neutil_lookup1(long entries, long dimensions);

int
neutil_write_ogg(ogg_stream_state *const stream, const char *const file);

int
neutil_buffer_to_riff(riff_t *const wwriff, const void *const buffer, brrsz buffer_size);

int
neutil_buffer_to_wwriff(wwriff_t *const wwriff, const void *const buffer, brrsz buffer_size);

#endif /* NAeP_neutil_h */
