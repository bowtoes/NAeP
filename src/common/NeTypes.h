#ifndef NeTypes_h
#define NeTypes_h

#include <limits.h>
#include <stdint.h>

/* To be used when representing numbers of the given byte-count */
typedef int8_t      NeS1;
typedef int16_t     NeS2;
typedef int32_t     NeS4;
typedef int64_t     NeS8;
typedef uint8_t     NeU1;
typedef uint16_t    NeU2;
typedef uint32_t    NeU4;
typedef uint64_t    NeU8;

/* To be used when representing arbitrary data */
typedef NeU1        NeBy;

/* Max supported filesize ~ 4GiB */
typedef NeU4        NeSz;
#define NeSZMAX     UINT32_MAX
#define NeSZMIN		0

/* +- Offsets */
typedef NeS8        NeOf;
#define NeOFMAX     INT64_MAX
#define NeOFMIN     INT64_MIN

/* array lengths */
typedef NeU2        NeCt;
/* RIFF chunk fourcc */
typedef NeU4 NeFcc;

/* Neither are guaranteed, screw you floating points */
typedef float       NeF4;
typedef double      NeF8;

#endif /* NeTypes_h */
