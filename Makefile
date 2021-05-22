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
	@echo "MODE       : $(MODE)"
	@echo "CC         : $(CC)"
	@echo "OUTDIR     : $(OUTDIR)"
	@echo "PREFIX     : $(prefix)"
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

$(PROJECT): setup options $(OBJ)
	$(CC) -o $(OUTDIR)/$(TARGETNAME) $(OBJ) $($(PROJECT)_LDFLAGS)

ass: setup options $(ASS) ;
int: setup options $(INT) ;
obj: setup options $(OBJ) ;
aio: setup options $(ASS) $(INT) $(OBJ) ;

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

install: all
	@cp -fv $(PROJECT) $(prefix)/bin
uninstall:
	@rm -fv $(prefix)/bin/$(PROJECT)
test: $(PROJECT)
	@test/test-dryrun.py
test-ogg: $(PROJECT)
	@test/test-ogg.py

.PHONY: setup options ass int obj aio all clean again install test
