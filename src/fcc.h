#ifndef FCC_H
#define FCC_H

#include <brrtools/brrapi.h>
#include <brrtools/brrtypes.h>

BRRCPPSTART

typedef union fourcc {
	struct {
		brru1 _0;
		brru1 _1;
		brru1 _2;
		brru1 _3;
	} bytes;
	brru4 integer;
} fourccT;

#define _fcc_init(_a_, _b_, _c_, _d_) {.bytes={(_a_), (_b_), (_c_), (_d_)}}
#define _fcc_lit(_l_) _fcc_init((_l_)[0], (_l_)[1], (_l_)[2], (_l_)[3])
#define FCC_INIT(_l_) _fcc_lit(_l_)
#define FCC_CODE(_f_) ((char[5]){(_f_).bytes._0, (_f_).bytes._1, (_f_).bytes._2, (_f_).bytes._3, 0})
/* Is this wrong on big-endian systems? */
#define FCC_CODE_INT(_l_) (brru4)(\
	(brru4)((_l_)[0] <<  0) | \
	(brru4)((_l_)[1] <<  8) | \
	(brru4)((_l_)[2] << 16) | \
	(brru4)((_l_)[3] << 24)   \
)
#define FCC_INT_CODE(_i_) ((char[5]){\
	((char*)(&(_i_)))[0], \
	((char*)(&(_i_)))[1], \
	((char*)(&(_i_)))[2], \
	((char*)(&(_i_)))[3], \
	0, \
})
#define FCC_FROM_INT(_i_) _fcc_init( \
	((brru1*)(&(_i_)))[0], \
	((brru1*)(&(_i_)))[1], \
	((brru1*)(&(_i_)))[2], \
	((brru1*)(&(_i_)))[3])
#define FCC_GET_BYTES(_f_) (_f_).bytes._0, (_f_).bytes._1, (_f_).bytes._2, (_f_).bytes._3
#define FCC_REVERSED(_f_) _fcc_init((_f_).bytes._3, (_f_).bytes._2, (_f_).bytes._1, (_f_).bytes._0)
#define FCC_REVERSE(_f_) ((_f_) = FCC_REVERSED(_f_))

BRRCPPEND

#endif /* FCC_H */
