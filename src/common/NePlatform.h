#ifndef NePlatform_h
#define NePlatform_h

enum NePlatforms {
	NeAIX = 0,
	NeHPUX,
	NeSun,
	NeMinGW64,
	NeWin64,
	NeCygwin,
	NeMinGW,
	NeWin86,
	NePosixCygwin,
	NeDragonFly,
	NeFreeBSD,
	NeNetBSD,
	NeOpenBSD,
	NeXcode,
	NeiOS,
	NeOSX,
	NeLinux
};

/* +++++ Specific OS detection */
#if defined(_AIX)
#  define NePLATFORMNAME "IBM AIX"
#  define NePLATFORM NeAIX
#elif defined(hpux) || defined(__hpux)
#  define NePLATFORMNAME "HP-UX"
#  define NePLATFORM NeHPUX
#elif defined(__sun) && defined(__SVR4) /* does not detect old BSD-based sol */
#  define NePLATFORMNAME "Solaris"
#  define NePLATFORM NeSun
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__WIN32__)
#  if defined(WIN64) || defined(_WIN64) || defined(__WIN64) || defined(__WIN64__)
#    if defined(__MINGw64__)
#      define NePLATFORMNAME "MinGW 64"
#      define NePLATFORM NeMinGW64
#    else
#      define NePLATFORMNAME "Windows x64"
#      define NePLATFORM NeWin64
#    endif
#  elif defined(__CYGWIN__) || defined(__CYGWIN32__)
#    define NePLATFORMNAME "Cygwin"
#    define NePLATFORM NeCygwin
#  elif defined(__MINGW32__)
#    define NePLATFORMNAME "MinGW"
#    define NePLATFORM NeMinGW
#  else
#    define NePLATFORMNAME "Windows x86"
#    define NePLATFORM NeWin86
#  endif
/* posix cygwin */
#elif defined(__CYGWIN__) || defined(__CYGWIN32__)
#  define NePLATFORMNAME "POSIX Cygwin"
#  define NePLATFORM NePosixCygwin
#elif defined(unix) || defined(__unix) || defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#  include <sys/param.h>
#  if defined(BSD) || defined(__bsdi__)
#    if defined(__DragonFly__)
#      define NePLATFORMNAME "DragonFly"
#      define NePLATFORM NeDragonFly
#    elif defined(__FreeBSD__)
#      define NePLATFORMNAME "FreeBSD"
#      define NePLATFORM NeFreeBSD
#    elif defined(__NetBSD__)
#      define NePLATFORMNAME "NetBSD"
#      define NePLATFORM NeNetBSD
#    elif defined(__OpenBSD__)
#      define NePLATFORMNAME "OpenBSD"
#      define NePLATFORM NeOpenBSD
#    elif defined(__APPLE__) && defined(__MACH__)
#      include <TargetConditionals.h>
#      define NePLATFORMTYPE_APPLE
#      define NePLATFORMTYPE_UNIX /* Apple does not define the unix's */
#      if TARGET_IPHONE_SIMULATOR == 1
#        define NePLATFORMNAME "Xcode"
#        define NePLATFORM NeXcode
#      elif TARGET_OS_IPHONE == 1
#        define NePLATFORMNAME "iOS"
#        define NePLATFORM NeiOS
#      elif TARGET_OS_MAC == 1
#        define NePLATFORMNAME "OSX"
#        define NePLATFORM NeOSX
#      else
#        error Huh?
#      endif
#    else
#      error Unknown BSD distribution
#    endif
#  elif defined(linux) || defined(__linux) || defined(__linux__)
#    define NePLATFORMNAME "Linux"
#    define NePLATFORM NeLinux
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

#if NePLATFORM != NeLinux
#  error Unsupported platform
#endif

/* +++++ System requirements */
#if CHAR_BIT != 8
#  error WEIRD SYSTEM
#endif

#if CHAR_MIN == 0
#  define NeUNSIGNEDCHAR
#elif CHAR_MIN < 0
#  define NeSIGNEDCHAR
#endif

#endif /* NePlatform_h */
