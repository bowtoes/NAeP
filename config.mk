# Get name of project automatically (or overwrite it if you prefer)
CURDIR=$(PWD)
PROJECT=$(notdir $(CURDIR))

$(PROJECT)_MAJOR=0
$(PROJECT)_MINOR=0
$(PROJECT)_REVIS=1

### Directory locations relative to project root
SRCDIR=src
# Where generated/compiled output files will live, with directory structures
# matching that of $(SRCDIR)
INTDIR=$(SRCDIR)
ASSDIR=$(SRCDIR)
OBJDIR=$(SRCDIR)
# Root directory of all output directories
OUTDIR=.

### Some defaults for C
CC=cc
# compiler 'include' arguments
INCS=-I$(SRCDIR)\

# compiler 'warning' arguments
WRNS=-Wall -Wextra -Wformat-security\
	 -Werror=implicit-function-declaration\
	 -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter\
	 -Wno-sign-compare -Wno-misleading-indentation -Wno-comment\
	 -Wno-format-zero-length

# compiler 'define' arguments
DEFS=-D$(PROJECT)_MAJOR=$($(PROJECT)_MAJOR)\
	 -D$(PROJECT)_MINOR=$($(PROJECT)_MINOR)\
	 -D$(PROJECT)_REVIS=$($(PROJECT)_REVIS)\
	 -D$(PROJECT)_VERSION='$($(PROJECT)_MAJOR).$($(PROJECT)_MINOR).$($(PROJECT)_REVIS)'\
	 -DNeASSERTS -DNeLOGGING -DNeLOGCOLORS -DNeLOGFLUSH\
	 -DNeDEBUGGING\

ifdef DEBUG
PRF=-pg -no-pie
OPT=-O0
else
PRF=
OPT=-O3 -Ofast
endif
$(PROJECT)_CFLAGS=-std=c11 $(OPT) $(PRF) $(WRNS) $(INCS) $(CFLAGS)
$(PROJECT)_CPPFLAGS=$(DEFS) $(STCPPFLAGS) $(CPPFLAGS)
$(PROJECT)_LDFLAGS=$(LDFLAGS) -logg -lvorbis $(PRF)

# find source files to compile and headers to check for changes
# should be specified relative to CURDIR
SRC:=\
	src/common/NeLogging.c\
	src/common/NeLibrary.c\
	src/common/NeFile.c\
	src/common/NeMisc.c\
	src/common/NeStr.c\
	src/wisp/NeWisp.c\
	src/revorbc/revorbc.c\
	src/NeArg.c\
	src/main.c\

HDR:=\
	src/common/NePlatform.h\
	src/common/NeTypes.h\
	src/common/NeDebugging.h\
	src/common/NeLogging.h\
	src/common/NeLibrary.h\
	src/common/NeFile.h\
	src/common/NeMisc.h\
	src/common/NeStr.h\
	src/wisp/NeWisp.h\
	src/revorbc/revorbc.h\
	src/NeArg.h\

# Set to whatever, I use /addtl on for custom programs
prefix=/addtl
