# Get name of project automatically (or overwrite it if you prefer)
CURDIR=$(PWD)
PROJECT=$(notdir $(CURDIR))

$(PROJECT)_MAJOR=0
$(PROJECT)_MINOR=0
$(PROJECT)_REVIS=0

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
WRNS=-Wall -Wextra\
	 -Werror=implicit-function-declaration\
	 -Wno-unused-function -Wno-sign-compare -Wno-misleading-indentation -Wno-comment\

# compiler 'define' arguments
DEFS=-D$(shell echo $(PROJECT) | tr a-z A-Z)_MAJOR=$($(PROJECT)_MAJOR)\
	 -D$(shell echo $(PROJECT) | tr a-z A-Z)_MINOR=$($(PROJECT)_MINOR)\
	 -D$(shell echo $(PROJECT) | tr a-z A-Z)_REVIS=$($(PROJECT)_REVIS)\
	 -DNeDEBUGGING -DNeLOGGING\
	 -D_POSIX_C_SOURCE=200112L -D_FILE_OFFSET_BITS=64

$(PROJECT)_CFLAGS=-std=c11 -g $(WRNS) $(INCS) $(CFLAGS)
$(PROJECT)_CPPFLAGS=$(DEFS) $(STCPPFLAGS) $(CPPFLAGS)
$(PROJECT)_LDFLAGS=$(LDFLAGS) -logg -lvorbis

# find source files to compile and headers to check for changes
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
prefix=/addtl
