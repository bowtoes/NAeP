#ifndef NeDebugging_h
#define NeDebugging_h

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
