.POSIX:
.SUFFIXES:

include config.mk

# Makefiles are stupidly finicky good god
ASS:=$(addprefix $(ASSDIR)/,$(patsubst $(SRCDIR)/%,%,$(SRC:.c=.s)))
INT:=$(addprefix $(INTDIR)/,$(patsubst $(SRCDIR)/%,%,$(SRC:.c=.e)))
OBJ:=$(addprefix $(OBJDIR)/,$(patsubst $(SRCDIR)/%,%,$(SRC:.c=.o)))

all: options setup $(PROJECT)
setup:
	@mkdir -pv $(dir $(ASS)) 2>/dev/null || printf ""
	@mkdir -pv $(dir $(INT)) 2>/dev/null || printf ""
	@mkdir -pv $(dir $(OBJ)) 2>/dev/null || printf ""
options:
	@echo "$(PROJECT) Build options:"
	@echo "CFLAGS        = $(CFLAGS)"
	@echo "CPPFLAGS      = $(CPPFLAGS)"
	@echo "STCPPFLAGS    = $(STCPPFLAGS)"
	@echo "$(PROJECT)_CFLAGS   = $($(PROJECT)_CFLAGS)"
	@echo "$(PROJECT)_CPPFLAGS = $($(PROJECT)_CPPFLAGS)"
	@echo "$(PROJECT)_LDFLAGS  = $($(PROJECT)_LDFLAGS)"
	@echo "CC            = $(CC)"

$(ASSDIR)/%.s: $(SRCDIR)/%.c
	$(CC) $($(PROJECT)_CPPFLAGS) $($(PROJECT)_CFLAGS) -S $< -o $@
$(INTDIR)/%.e: $(SRCDIR)/%.c
	$(CC) $($(PROJECT)_CPPFLAGS) $($(PROJECT)_CFLAGS) -E $< -o $@
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $($(PROJECT)_CPPFLAGS) $($(PROJECT)_CFLAGS) -c $< -o $@

$(ASS): $(HDR) Makefile config.mk
$(INT): $(HDR) Makefile config.mk
$(OBJ): $(HDR) Makefile config.mk

$(PROJECT): $(OBJ)
	$(CC) $^ -o $(OUTDIR)/$@ $($(PROJECT)_LDFLAGS)

ass: setup options $(ASS) ;
int: setup options $(INT) ;
obj: setup options $(OBJ) ;

clean:
	@rm -fv $(ASS)
	@rm -fv $(INT)
	@rm -fv $(OBJ)
	@rmdir -pv $(ASSDIR) > /dev/null 2>&1 || printf ""
	@rmdir -pv $(INTDIR) > /dev/null 2>&1 || printf ""
	@rmdir -pv $(OBJDIR) > /dev/null 2>&1 || printf ""
	@rm -fv $(OUTDIR)/$(PROJECT)

again: clean all

install: all
	@cp -fv $(PROJECT) $(prefix)/bin
uninstall:
	@rm -fv $(prefix)/bin/$(PROJECT)
test: $(PROJECT)
	./T.py

.PHONY: setup options ass int obj all clean again install test
