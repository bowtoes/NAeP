#ifndef NePlatform_h
#define NePlatform_h

#define NePLATFORM_AIX 0
#define NePLATFORM_HPUX 1
#define NePLATFORM_Sun 2
#define NePLATFORM_MinGW64 3
#define NePLATFORM_Win64 4
#define NePLATFORM_Cygwin 5
#define NePLATFORM_MinGW 6
#define NePLATFORM_Win86 7
#define NePLATFORM_PosixCygwin 8
#define NePLATFORM_DragonFly 9
#define NePLATFORM_FreeBSD 10
#define NePLATFORM_NetBSD 11
#define NePLATFORM_OpenBSD 12
#define NePLATFORM_Xcode 13
#define NePLATFORM_iOS 14
#define NePLATFORM_OSX 15
#define NePLATFORM_Linux 16

/* +++++ Specific OS detection */
#if defined(_AIX)
#  define NePLATFORMNAME "IBM AIX"
#  define NePLATFORM NePLATFORM_AIX
#elif defined(hpux) || defined(__hpux)
#  define NePLATFORMNAME "HP-UX"
#  define NePLATFORM NePLATFORM_HPUX
#elif defined(__sun) && defined(__SVR4) /* does not detect old BSD-based sol */
#  define NePLATFORMNAME "Solaris"
#  define NePLATFORM NePLATFORM_Sun
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__WIN32__)
#  if defined(WIN64) || defined(_WIN64) || defined(__WIN64) || defined(__WIN64__)
#    if defined(__MINGw64__)
#      define NePLATFORMNAME "MinGW 64"
#      define NePLATFORM NePLATFORM_MinGW64
#    else
#      define NePLATFORMNAME "Windows x64"
#      define NePLATFORM NePLATFORM_Win64
#    endif
#  elif defined(__CYGWIN__) || defined(__CYGWIN32__)
#    define NePLATFORMNAME "Cygwin"
#    define NePLATFORM NePLATFORM_Cygwin
#  elif defined(__MINGW32__)
#    define NePLATFORMNAME "MinGW"
#    define NePLATFORM NePLATFORM_MinGW
#  else
#    define NePLATFORMNAME "Windows x86"
#    define NePLATFORM NePLATFORM_Win86
#  endif
/* posix cygwin */
#elif defined(__CYGWIN__) || defined(__CYGWIN32__)
#  define NePLATFORMNAME "POSIX Cygwin"
#  define NePLATFORM NePLATFORM_PosixCygwin
#elif defined(unix) || defined(__unix) || defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#  // 'sys/param.h' for 'BSD'
#  include <sys/param.h>
#  if defined(BSD) || defined(__bsdi__)
#    if defined(__DragonFly__)
#      define NePLATFORMNAME "DragonFly"
#      define NePLATFORM NePLATFORM_DragonFly
#    elif defined(__FreeBSD__)
#      define NePLATFORMNAME "FreeBSD"
#      define NePLATFORM NePLATFORM_FreeBSD
#    elif defined(__NetBSD__)
#      define NePLATFORMNAME "NetBSD"
#      define NePLATFORM NePLATFORM_NetBSD
#    elif defined(__OpenBSD__)
#      define NePLATFORMNAME "OpenBSD"
#      define NePLATFORM NePLATFORM_OpenBSD
#    elif defined(__APPLE__) && defined(__MACH__)
#      include <TargetConditionals.h>
#      define NePLATFORMTYPE_APPLE
#      define NePLATFORMTYPE_UNIX /* Apple does not define the unix's */
#      if TARGET_IPHONE_SIMULATOR == 1
#        define NePLATFORMNAME "Xcode"
#        define NePLATFORM NePLATFORM_Xcode
#      elif TARGET_OS_IPHONE == 1
#        define NePLATFORMNAME "iOS"
#        define NePLATFORM NePLATFORM_iOS
#      elif TARGET_OS_MAC == 1
#        define NePLATFORMNAME "OSX"
#        define NePLATFORM NePLATFORM_OSX
#      else
#        error Huh?
#      endif
#    else
#      error Unknown BSD distribution
#    endif
#  // end (BSD || __bsdi__)
#  elif defined(linux) || defined(__linux) || defined(__linux__)
#    define NePLATFORMNAME "Linux"
#    define NePLATFORM NePLATFORM_Linux
#  else
#    error Unknown UNIX system
#  endif
#endif

/* +++++ Broad environment detection */
#if defined(BSD) || defined(__bsdi__)
#  define NePLATFORMTYPE_BSD
#endif
#if defined(unix) || defined(__unix) || defined(__unix__)
#  define NePLATFORMTYPE_UNIX
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__WIN32__)
#  define NePLATFORMTYPE_WINDOWS
#  define NeNOLOGCOLORS
#  if defined(__MINGW32__)
#    define NePLATFORMTYPE_MINGW
#  endif
#endif
#if defined(__CYGWIN__) || defined(__CYGWIN32__)
#  define NePLATFORMTYPE_CYGWIN
#endif

/* Get POSIX information */
#if defined(NePLATFORMTYPE_UNIX) && !defined(NePLATFORMTYPE_WINDOWS)
#  include <unistd.h>
#  if defined(_POSIX_VERSION)
#    define NePLATFORMTYPE_POSIX
#  endif
#endif

#ifndef NePLATFORM
#  error UNKNOWN PLATFORM
#endif

#if NePLATFORM != NePLATFORM_Linux
#  error Unsupported platform
#endif

/* +++++ System requirements */
#if CHAR_BIT != 8
#  error WEIRD SYSTEM
#endif

#endif /* NePlatform_h */
