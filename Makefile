.POSIX:
.SUFFIXES:

include config.mk

ASS:=$(addprefix $(OUTDIR)/$(ASSDIR)/,$(patsubst $(SRCDIR)/%,%,$(SRC:.c=.s)))
INT:=$(addprefix $(OUTDIR)/$(INTDIR)/,$(patsubst $(SRCDIR)/%,%,$(SRC:.c=.e)))
OBJ:=$(addprefix $(OUTDIR)/$(OBJDIR)/,$(patsubst $(SRCDIR)/%,%,$(SRC:.c=.o)))

all: $(PROJECT)
setup:
	@mkdir -pv $(dir $(ASS)) 2>/dev/null || :
	@mkdir -pv $(dir $(INT)) 2>/dev/null || :
	@mkdir -pv $(dir $(OBJ)) 2>/dev/null || :
options:
	@echo "<------------------------------------->"
	@echo ""
	@echo "$(PROJECT) BUILD options:"
	@echo "TARGETNAME : $(TARGETNAME)"
	@echo "HOST       : $(HOST)"
	@echo "TARGET     : $(TARGET)"
	@echo "CC         : $(CC)"
	@echo "OUTDIR     : $(OUTDIR)"
	@echo "PREFIX     : $(prefix)"
ifdef DEBUG
	@echo "DEBUG      : ON"
 ifdef MEMCHECK
	@echo "MEMCHECK   : ON"
 else
	@echo "MEMCHECK   : OFF"
 endif
else
	@echo "DEBUG      : OFF"
endif
ifdef LIBRECONFIG
	@echo "LIBRECONFIG: ON"
else
	@echo "LIBRECONFIG: OFF"
endif
	@echo ""
	@echo "$(PROJECT)_CFLAGS   =$($(PROJECT)_CFLAGS)"
	@echo "$(PROJECT)_CPPFLAGS =$($(PROJECT)_CPPFLAGS)"
	@echo "$(PROJECT)_LDFLAGS  =$($(PROJECT)_LDFLAGS)"
	@echo "CFLAGS            =$(CFLAGS)"
	@echo "CPPFLAGS          =$(CPPFLAGS)"
	@echo "LDFLAGS           =$(LDFLAGS)"
	@echo ""
	@echo "<------------------------------------->"

$(OUTDIR)/$(ASSDIR)/%.s: $(SRCDIR)/%.c
	$(CC) $($(PROJECT)_CPPFLAGS) $($(PROJECT)_CFLAGS) -S $< -o $@
$(OUTDIR)/$(INTDIR)/%.e: $(SRCDIR)/%.c
	$(CC) $($(PROJECT)_CPPFLAGS) $($(PROJECT)_CFLAGS) -E $< -o $@
$(OUTDIR)/$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $($(PROJECT)_CPPFLAGS) $($(PROJECT)_CFLAGS) -c $< -o $@

$(ASS): $(HDR) Makefile config.mk
$(INT): $(HDR) Makefile config.mk
$(OBJ): $(HDR) Makefile config.mk

$(PROJECT): libs setup options $(OBJ)
ifeq ($(TARGET),UNIX)
	$(CC) -o $(OUTDIR)/$(TARGETNAME) $(OBJ) ./vendor/brrtools/$(OUTDIR)/static/libbrrtools.a \
	    ./vendor/vorbis/lib/.libs/libvorbis.a ./vendor/ogg/src/.libs/libogg.a \
	    $($(PROJECT)_LDFLAGS)
else
	$(CC) -o $(OUTDIR)/$(TARGETNAME) $(OBJ) ./vendor/brrtools/$(OUTDIR)/static/libbrrtools.lib \
	    ./vendor/vorbis/lib/.libs/libvorbis.a ./vendor/ogg/src/.libs/libogg.a \
	    $($(PROJECT)_LDFLAGS)
endif

ass: setup options $(ASS) ;
int: setup options $(INT) ;
obj: setup options $(OBJ) ;
aio: setup options $(ASS) $(INT) $(OBJ) ;

install: all
	@cp -fuv $(OUTDIR)/$(TARGETNAME) $(prefix)/bin
uninstall:
	@rm -fv $(prefix)/bin/$(PROJECT)

brrtools:
	make -C vendor/brrtools PEDANTIC=1 SRCDIR=src HDRDIR=src MODE=STATIC \
	    UNISTANAME=libbrrtools.a WINSTANAME=libbrrtools.lib
ogg:
ifdef LIBRECONFIG
	cd vendor/ogg && make maintainer-clean 2>/dev/null || :
	cd vendor/ogg && ./autogen.sh
 ifeq ($(TARGET),UNIX)
	# Unix-compile probably needs 32-,64-bit compilation options as well.
  ifeq ($(BITS),32)
	cd vendor/ogg && ./configure --disable-shared --build=i686-pc-linux
  else
	cd vendor/ogg && ./configure --disable-shared --build=x86_64-pc-linux
  endif
 else
  ifeq ($(BITS),32)
	cd vendor/ogg && ./configure --disable-shared \
	    --host=i686-w64-mingw32 --target=i686-w64-mingw32 --build=i686-pc-linux
  else
	cd vendor/ogg && ./configure --disable-shared \
	    --host=x86_64-w64-mingw32 --target=x86_64-w64-mingw32 --build=x86_64-pc-linux
  endif
 endif
endif
	make -C vendor/ogg
vorbis:
ifdef LIBRECONFIG
	cd vendor/vorbis && make maintainer-clean 2>/dev/null || :
	cd vendor/vorbis && ./autogen.sh
 ifeq ($(TARGET),UNIX)
	# Unix-compile probably needs 32-,64-bit compilation options as well.
	cd vendor/vorbis && ./configure --with-ogg-libs=../ogg/src/.libs \
	    --with-ogg-includes=../ogg/include --disable-shared --disable-docs --disable-examples \
	    --disable-oggtest
 else
  ifeq ($(BITS),32)
	cd vendor/vorbis && ./configure --with-ogg-libs=../ogg/src/.libs \
	    --with-ogg-includes=../ogg/include --disable-shared --disable-docs --disable-examples \
	    --disable-oggtest \
	    --host=i686-w64-mingw32 --target=i686-w64-mingw32 --build=i686-linux
  else
	cd vendor/vorbis && ./configure --with-ogg-libs=../ogg/src/.libs \
	    --with-ogg-includes=../ogg/include --disable-shared --disable-docs --disable-examples \
	    --disable-oggtest \
	    --host=x86_64-w64-mingw32 --target=x86_64-w64-mingw32 --build=x86_64-linux

  endif
 endif
endif
	make -C vendor/vorbis CFLAGS="$$CFLAGS -I$(VENDIR)/brrtools/src $(VENDEFS)"
libs: brrtools ogg vorbis

clean-brrtools: ; make -C vendor/brrtools clean
clean-ogg: ; make -C vendor/ogg distclean
clean-vorbis: ; make -C vendor/vorbis distclean
clean-libs: clean-brrtools clean-ogg clean-vorbis
clean-all: clean-libs clean
clean:
	$(RM) $(ASS)
	$(RM) $(INT)
	$(RM) $(OBJ)
	$(RM) $(OUTDIR)/$(TARGETNAME)
ifeq ($(TARGET),WINDOWS)
	$(RM) $(OUTDIR)/$(WINNAME)
else
	$(RM) $(OUTDIR)/$(UNINAME)
endif
ifeq ($(HOST),UNIX)
	@find $(OUTDIR)/$(ASSDIR) -type d -exec rmdir -v --ignore-fail-on-non-empty {} + 2>/dev/null || :
	@find $(OUTDIR)/$(INTDIR) -type d -exec rmdir -v --ignore-fail-on-non-empty {} + 2>/dev/null || :
	@find $(OUTDIR)/$(OBJDIR) -type d -exec rmdir -v --ignore-fail-on-non-empty {} + 2>/dev/null || :
	@find $(OUTDIR) -type d -exec rmdir -v --ignore-fail-on-non-empty {} + 2>/dev/null || :
else
	$(RMDIR) $(OUTDIR)/$(ASSDIR) 2>$(NULL) || :
	$(RMDIR) $(OUTDIR)/$(OBJDIR) 2>$(NULL) || :
	$(RMDIR) $(OUTDIR)/$(INTDIR) 2>$(NULL) || :
	$(RMDIR) $(OUTDIR) 2>$(NULL) || :
endif

again: clean all
again-brrtools: clean-brrtools brrtools
again-ogg: clean-ogg ogg
again-vorbis: clean-vorbis vorbis
again-libs: again-brrtools
again-all: again-libs again

.PHONY: brrtools ogg vorbis libs
.PHONY: clean clean-brrtools clean-ogg clean-vorbis clean-libs clean-all
.PHONY: again again-brrtools again-ogg again-vorbis again-libs again-all
.PHONY: setup options ass int obj aio install uninstall
