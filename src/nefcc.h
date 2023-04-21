/* Copyright (c), bowtoes (bow.toes@mailfence.com)
Apache 2.0 license, http://www.apache.org/licenses/LICENSE-2.0
Full license can be found in 'license' file */

#ifndef NAeP_nefcc_h
#define NAeP_nefcc_h

#include <brrtools/brrmacro.h>
#include <brrtools/brrtypes.h>

typedef struct fcc {
	struct {
		brru4 r:1;
		brru4 n:3;
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

#define _fcc_print_split(_f_) (isprint((_f_)._0)?(_f_)._0:' '), (isprint((_f_)._1)?(_f_)._1:' '), (isprint((_f_)._2)?(_f_)._2:' '), (isprint((_f_)._3)?(_f_)._3:' ')

#define fcc_int(_i_) {.u=(brru4)(_i_)}
#define fcc_intB(_i_) {.v={_fcc_splitB((fcc_t)fcc_int(_i_))}}

#define fcc_arr(_a_)  fcc_chr( _str_split(_a_))
#define fcc_arrB(_a_) fcc_chrB(_str_split(_a_))

#define fcc_as_str(_f_) {_fcc_split(_f_)}
#define fcc_as_strB(_f_) {_fcc_splitB(_f_)}

/* Returns 0 when equal, <0 when a < b, >0 when a > b  */
int
fcccmp(fcc_t a, fcc_t b);

#endif /* NAeP_nefcc_h */
