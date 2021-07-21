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

brrtools: ; make -C vendor/brrtools PEDANTIC=1 SRCDIR=src HDRDIR=src MODE=STATIC UNISTANAME=libbrrtools.a WINSTANAME=libbrrtools.lib

$(PROJECT): brrtools setup options $(OBJ)
ifeq ($(TARGET),UNIX)
	$(CC) -o $(OUTDIR)/$(TARGETNAME) $(OBJ) ./vendor/brrtools/$(OUTDIR)/static/libbrrtools.a $($(PROJECT)_LDFLAGS)
else
	$(CC) -o $(OUTDIR)/$(TARGETNAME) $(OBJ) ./vendor/brrtools/$(OUTDIR)/static/libbrrtools.lib $($(PROJECT)_LDFLAGS)
endif

ass: setup options $(ASS) ;
int: setup options $(INT) ;
obj: setup options $(OBJ) ;
aio: setup options $(ASS) $(INT) $(OBJ) ;

clean:
	make -C vendor/brrtools clean
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

install-lib: brrtools ; make -C vendor/brrtools install
install: all
	@cp -fv $(PROJECT) $(prefix)/bin
uninstall:
	@rm -fv $(prefix)/bin/$(PROJECT)
test: $(PROJECT)
	@test/test-dryrun.py
test-ogg: $(PROJECT)
	@test/test-ogg.py

.PHONY: setup options ass int obj aio all clean again install test brrtools
