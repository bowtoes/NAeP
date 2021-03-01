#ifndef NeDebugging_h
#define NeDebugging_h

#include <stdio.h>
#include "common/NeTypes.h"

enum NeLogPriority {
	NeDebug,
	NeNormal,
	NeWarning,
	NeError,
	NeCritical
};

#ifndef NeMAXLOG
#define NeMAXLOG 2048
#endif

NeSz NeLog(enum NeLogPriority priority, int nl, const char *const fmt, ...);
NeSz NeLogData(enum NeLogPriority priority,
        const void *const data, NeSz len, int hx,
        const char *const fmt, ...);

#if defined(NeLOGGING) && !defined(NeNOLOGGING)
#define NeLOG(p, ...) NeLog(p, __VA_ARGS__)
#else
#define NeLOG(...)
#endif
#define NeNORMAL(...) NeLOG(NeNormal, 1, __VA_ARGS__)
#define NeWARNING(...) NeLOG(NeWarning, 1, __VA_ARGS__)
#define NeERROR(...) NeLOG(NeError, 1, __VA_ARGS__)
#define NeCRITICAL(...) NeLOG(NeCritical, 1, __VA_ARGS__)

#define NeNORMALN(...) NeLOG(NeNormal, 0, __VA_ARGS__)
#define NeWARNINGN(...) NeLOG(NeWarning, 0, __VA_ARGS__)
#define NeERRORN(...) NeLOG(NeError, 0, __VA_ARGS__)
#define NeCRITICALN(...) NeLOG(NeCritical, 0, __VA_ARGS__)

#if defined(NeDEBUGGING) && !defined(NeNODEBUGGING)
#define NeDEBUG(...) NeLOG(NeDebug, 1, __VA_ARGS__)
#define NeDEBUGN(...) NeLOG(NeDebug, 0, __VA_ARGS__)
#define NeDEBUGDATA(dt, ln, hx) NeLogData(NeDebug, dt, ln, hx, NULL)
#define NeFORMATDATA(dt, ln, hx, ...) NeLogData(NeDebug, dt, ln, hx, __VA_ARGS__)
#else
#define NeDEBUG(...)
#define NeDEBUGN(...)
#define NeDEBUGDATA(dt, ln, hx)
#define NeFORMATDATA(dt, ln, hx, ...)
#endif

/* https://github.com/nemequ/portable-snippets debug-trap.h */
#if defined(NeDEBUGGING) && !defined(NeNODEBUGGING)
#  if defined(__has_builtin) && !defined(__ibmxl__)
#    if __has_builtin(__builtin_debugtrap)
#      define NeBREAK() __builtin_debugtrap()
#    elif __has_builtin(__debugbreak)
#      define NeBREAK() __debugbreak()
#    endif
#  endif
#  if !defined(NeBREAK)
#    if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#      define NeBREAK() __debugbreak()
#    elif defined(__ARMCC_VERSION)
#      define NeBREAK() __breakpoint(42)
#    elif defined(__ibmxl__) || defined(__xlC__)
#      include <builtins.h>
#      define NeBREAK() __trap(42)
#    elif defined(__STDC_HOSTED__) && (__STDC_HOSTED__ == 0) && defined(__GNUC__)
#      define NeBREAK() __builtin_trap()
#    else
#      include <signal.h>
#      if defined(SIGTRAP)
#        define NeBREAK() raise(SIGTRAP)
#      else
#        define NeBREAK() raise(SIGABRT)
#      endif
#    endif
#  endif /* !NeBREAK */
#  define NeHASBREAK
#  define NeASSERTS
#  define NeTRACING
#
#else
#  define NeBREAK()
#endif /* NeDebugging && !NeNoDebugging */

#if defined(NeNODEBUGGING)
#  define NeNOASSERTS
#endif /* NeNoDebugging */

#if defined(NeHASBREAK)
#  define NeTRACE(...) { NeCRITICAL(__VA_ARGS__); NeBREAK(); }
#else
#  include <stdlib.h>
#  define NeTRACE(...) { NeCRITICAL(__VA_ARGS__); abort(); }
#endif

#if defined(NeASSERTS) && !defined(NeNOASSERTS)
#  define NeASSERT(n)        { if (!(n)) { NeTRACE("Assertion %s failed", #n); } }
#  define NeASSERTM(n, ...)  { if (!(n)) { NeTRACE(__VA_ARGS__); } }
#  define NeASSERTI(n)       NeASSERT(n)
#  define NeASSERTIM(n, ...) NeASSERTM(n, __VA_ARGS__)
#else
#  define NeASSERT(n)
#  define NeASSERTM(n, ...)
#  define NeASSERTI(n)       (n)
#  define NeASSERTIM(n, ...) (n)
#endif /* NeAsserts && !NeNoAsserts */

#endif /* NeDebugging_h */
