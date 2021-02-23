NAeP_MAJOR=0
NAeP_MINOR=0
NAeP_REVIS=0

### Directory locations relative to project root
SRCDIR=src
# Where generated/compiled output files will live, with directory trees
# matching $(SRCDIR)
INTDIR=$(SRCDIR)
ASSDIR=$(SRCDIR)
OBJDIR=$(SRCDIR)
# Where to put final executable
OUTDIR=.

### Some defaults for C
CC=cc
# compiler 'include' arguments
INCS=-I$(SRCDIR)\

# compiler 'warning' arguments
WRNS=-Wall -Wextra\
	 -Werror=implicit-function-declaration\
	 -Wno-unused-function -Wno-sign-compare -Wno-misleading-indentation -Wno-comment\

# compiler 'define' arguments
DEFS= -DNAeP_MAJOR=$(NAeP_MAJOR)\
			-DNAeP_MINOR=$(NAeP_MINOR)\
			-DNAeP_REVIS=$(NAeP_REVIS)\
			-DNeDEBUGGING -DNeLOGGING\
			-D_POSIX_C_SOURCE=200112L -D_FILE_OFFSET_BITS=64

NAeP_CFLAGS=-std=c11 -g $(WRNS) $(INCS) $(CFLAGS)
NAeP_CPPFLAGS=$(DEFS) $(STCPPFLAGS) $(CPPFLAGS)
NAeP_LDFLAGS=$(LDFLAGS) -logg -lvorbis

# which source files to compile and headers to check for changes
# should be specified relative to CURDIR
SRC:=\
	src/common/NeDebugging.c\
	src/common/NeLibrary.c\
	src/common/NeMisc.c\
	src/common/NeFile.c\
	src/main.c\
	src/revorbc/revorbc.c\
	src/wisp/NeWisp.c\

HDR:=\
	src/common/NePlatform.h\
	src/common/NeTypes.h\
	src/common/NeDebugging.h\
	src/common/NeLibrary.h\
	src/common/NeMisc.h\
	src/common/NeFile.h\
	src/revorbc/revorbc.h\
	src/wisp/NeWisp.h\

# Set to whatever, I use /addtl on for custom programs
# Default location is usually /usr or /usr/local
prefix=/addtl
