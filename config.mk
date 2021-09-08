# # # TOP CONFIG
CURDIR:=$(PWD)
VENDIR:=$(PWD)/vendor
# Name of project (or overwrite it if you prefer)
#PROJECT:=
ifndef PROJECT
PROJECT:=$(notdir $(CURDIR))
endif
ifndef UPROJECT
UPROJECT:=Ne
#$(shell echo '$(PROJECT)' | tr '[:lower:]' '[:upper:]')
endif
# Default installation prefix
ifndef prefix
prefix=/usr/local
endif

# Project version
$(PROJECT)_MAJOR=0
$(PROJECT)_MINOR=0
$(PROJECT)_REVIS=0

# Default c-standard to used
ifndef STD
 STD:=c11
endif
# Default linker options
ifndef LNK
 LNK:=-lm
 ifeq ($(TARGET),UNIX)
  LNK:=$(LNK) -lmvec
 endif
endif

# # # DIRECTORY CONFIG
### Directory locations relative to $(CURDIR).
# Top directory of all project specific source code.
ifndef SRCDIR
SRCDIR:=src
endif
# Top directory of all project headers that will be installed.
ifndef HDRDIR
HDRDIR:=
endif
# Directory where all products of compilation will be placed.
ifndef BLDDIR
BLDDIR:=build
endif
# Directory name for all unix-specific compilations
ifndef UNIDIR
UNIDIR:=uni
endif
# Directory name for all windows-specific compilations
ifndef WINDIR
WINDIR:=win
endif
# Directory name for dynamically linked binaries.
ifndef SHRDIR
SHRDIR:=shared
endif
# Directory name for statically linked binaries.
ifndef STADIR
STADIR:=static
endif
# Directory name for 32-bit binaries.
ifndef B32DIR
B32DIR:=x86
endif
# Directory name for 64-bit binaries.
ifndef B64DIR
B64DIR:=x64
endif

# Directory name where intermediates will be stored (after preprocessor)
ifndef INTDIR
INTDIR:=int
endif
# Directory name where assemblies will be stored.
ifndef ASSDIR
ASSDIR:=ass
endif
# Directory name where objects will be stored (after assembly).
ifndef OBJDIR
OBJDIR:=obj
endif

# # LIBRARY OUTPUT
# Name of final shared unix output.
ifndef UNISHRNAME
UNISHRNAME:=lib$(PROJECT).so
endif
# Name of final static unix output.
ifndef UNISTANAME
UNISTANAME:=lib$(PROJECT).a
endif
# Name of final windows dynamic library.
ifndef WINSHRNAME
WINSHRNAME:=$(PROJECT).dll
endif
# Name of windows dll definitions file.
ifndef WINDEFNAME
WINDEFNAME:=$(PROJECT).def
endif
# Name of windows dll import library.
ifndef WINIMPNAME
WINIMPNAME:=$(PROJECT).dll.lib
endif
# Name of final windows static library.
ifndef WINSTANAME
WINSTANAME:=lib$(PROJECT).lib
endif

# # EXECUTABLE OUTPUT
ifndef UNINAME
UNINAME:=$(PROJECT)
endif
ifndef WINNAME
WINNAME:=$(PROJECT).exe
endif

# # # COMPILATION SETTINGS
# Host system
ifndef HOST # Assume unix host
 HOST:=UNIX
else ifneq ($(HOST),UNIX)
 ifneq ($(HOST),WINDOWS)
  HOST:=UNIX
 else # Windows host always compiles windows target
  TARGET:=WINDOWS
 endif
endif ## HOST

# Target system
ifndef TARGET # Assume unix target
 TARGET:=UNIX
else ifneq ($(TARGET),UNIX)
 ifneq ($(TARGET),WINDOWS)
  TARGET:=UNIX
 endif
endif ## TARGET

# Bit-ness of target.
ifndef BITS # Assume 64-bit target
 BITS:=64
else ifneq ($(BITS),32)
 ifneq ($(BITS),64)
  BITS:=64
 endif
endif ## BITS

# # # TOOLCHAIN
# Compiler
ifdef CC_CUSTOM
 CC:=$(CC_CUSTOM)
else
 ifeq ($(HOST),UNIX)
  ifeq ($(TARGET),UNIX)
   CC:=gcc
  else # Windows target, must have mingw
   ifeq ($(BITS),32)
    CC:=i686-w64-mingw32-gcc
   else # Assume 64 bit
    CC:=x86_64-w64-mingw32-gcc
   endif ## BITS
  endif ## TARGET
 else # Windows host, must have mingw
  CC:=gcc
 endif ## HOST
endif ## CC

# Archiver
ifdef AR_CUSTOM
 AR:=$(AR_CUSTOM)
else
 ifeq ($(HOST),UNIX)
  ifeq ($(TARGET),UNIX)
   AR:=gcc-ar
  else # Windows target, must have mingw
   ifeq ($(BITS),32)
    AR:=i686-w64-mingw32-gcc-ar
   else # Assume 64 bit
    AR:=x86_64-w64-mingw32-gcc-ar
   endif ## BITS
  endif ## TARGET
 else # Windows host, must have mingw
  AR:=gcc-ar
 endif ## HOST
endif ## AR

# Windows DLL tool
ifndef DLLTOOL
 ifeq ($(HOST),UNIX)
  ifeq ($(TARGET),UNIX)
   DLLTOOL:=echo There is no dlltool for linux
  else # Windows target, must have mingw
   ifeq ($(BITS),32)
    DLLTOOL:=i686-w64-mingw32-dlltool
   else
    DLLTOOL:=x86_64-w64-mingw32-dlltool
   endif ## BITS
  endif ## TARGET
 else # Windows host
  DLLTOOL:=dlltool
 endif ## HOST
endif ## DLLTOOL

ifeq ($(HOST),UNIX)
 NULL:=/dev/null
 RM:=rm -fv
 RMDIR:=rm -rfv
else
 NULL:=nul
 RM:=del /F
 RMDIR:=rmdir
endif ## HOST

# Compileds output top directory
ifeq ($(TARGET),WINDOWS)
OUTDIR:=$(BLDDIR)/$(WINDIR)
else
OUTDIR:=$(BLDDIR)/$(UNIDIR)
endif ## TARGET
ifeq ($(BITS),32)
OUTDIR:=$(OUTDIR)/$(B32DIR)
else
OUTDIR:=$(OUTDIR)/$(B64DIR)
endif ## BITS

ifeq ($(TARGET),UNIX)
 TARGETNAME:=$(UNINAME)
else
 TARGETNAME:=$(WINNAME)
endif

# # # CC ARGS
# Includes
INCS:=-I$(SRCDIR) -I./vendor/brrtools/src -I./vendor/ogg/include -I./vendor/vorbis/include
# Warnings/errors
WRNS:=-Wall -Wextra -Wpedantic -pedantic -Werror=pedantic -pedantic-errors\
      -Werror=implicit-function-declaration -Werror=missing-declarations\
      -Wno-sign-compare
# Defines
DEFS:=-D$(UPROJECT)MAJOR=$($(PROJECT)_MAJOR)\
      -D$(UPROJECT)MINOR=$($(PROJECT)_MINOR)\
      -D$(UPROJECT)REVIS=$($(PROJECT)_REVIS)\
      -D$(UPROJECT)VERSION='"$($(PROJECT)_MAJOR).$($(PROJECT)_MINOR).$($(PROJECT)_REVIS)"'\

ifdef DEBUG
 DEFS:=$(DEFS) -D$(UPROJECT)DEBUG
 ifdef MEMCHECK
  DEFS:=$(DEFS) -D$(UPROJECT)MEMCHECK
 endif
 ifdef VENDEBUG
  VENDEFS:=$(VENDEFS) -D$(UPROJECT)VENDEBUG
  # Enables debug logging in my copy of libogg/libvorbis, for my sanity
 endif
endif

# PRF: Performance options (applied at compile & link-time)
# OPT: Optimization options (applied at compile-time)
ifdef DEBUG
 OPT:=-O0 -g
 # valgrind no-like '-pg -no-pie' options
 ifdef MEMCHECK
  PRF:=
 else
  PRF:=-pg -no-pie
 endif ## MEMCHECK
else
 OPT:=-O3 -Ofast
 ifeq ($(TARGET),SHARED)
  PRF:=-s
 else
  PRF:=
 endif
endif ## DEBUG
# Binary bit-ness
ifeq ($(BITS),32)
 PRF:=$(PRF) -m32
else
 PRF:=$(PRF) -m64
endif ## BITS

$(PROJECT)_CFLAGS=-std=$(STD) $(OPT) $(PRF) $(WRNS) $(INCS) $(CFLAGS)
$(PROJECT)_CPPFLAGS=$(DEFS) $(STCPPFLAGS) $(CPPFLAGS)
$(PROJECT)_LDFLAGS=$(LNK) $(PRF) $(LDFLAGS)

# find source files to compile and headers to check for changes
# should be specified relative to CURDIR
SRC:=\
	src/main.c\
	src/codebook_library.c\
	src/common.c\
	src/riff.c\
	src/process_ogg.c\
	src/process_wem.c\
	src/process_wsp.c\
	src/process_bnk.c\
	src/wwise.c\

HDR:=\
	src/codebook_library.h\
	src/common.h\
	src/riff.h\
	src/process_files.h\
	src/wwise.h\
