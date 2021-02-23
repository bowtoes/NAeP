.POSIX:
.SUFFIXES:
# Makefiles are stupidly finicky good god

include config.mk

# put output build files into defined directories
ASS:=$(addprefix $(ASSDIR)/,$(patsubst $(SRCDIR)/%,%,$(SRC:.c=.s)))
INT:=$(addprefix $(INTDIR)/,$(patsubst $(SRCDIR)/%,%,$(SRC:.c=.e)))
OBJ:=$(addprefix $(OBJDIR)/,$(patsubst $(SRCDIR)/%,%,$(SRC:.c=.o)))

all: options setup NAeP
setup:
	@mkdir -pv $(dir $(ASS)) 2>/dev/null || printf ""
	@mkdir -pv $(dir $(INT)) 2>/dev/null || printf ""
	@mkdir -pv $(dir $(OBJ)) 2>/dev/null || printf ""
options:
	@echo "NAeP Build options:"
	@echo "CFLAGS        = $(CFLAGS)"
	@echo "CPPFLAGS      = $(CPPFLAGS)"
	@echo "STCPPFLAGS    = $(STCPPFLAGS)"
	@echo "NAeP_CFLAGS   = $(NAeP_CFLAGS)"
	@echo "NAeP_CPPFLAGS = $(NAeP_CPPFLAGS)"
	@echo "NAeP_LDFLAGS  = $(NAeP_LDFLAGS)"
	@echo "CC            = $(CC)"

$(ASSDIR)/%.s: %.c Makefile config.mk
	$(CC) $(NAeP_CPPFLAGS) $(NAeP_CFLAGS) -S $< -o $@
$(INTDIR)/%.e: %.c Makefile config.mk
	$(CC) $(NAeP_CPPFLAGS) $(NAeP_CFLAGS) -E $< -o $@
$(OBJDIR)/%.o: $(SRCDIR)/%.c Makefile config.mk
	$(CC) $(NAeP_CPPFLAGS) $(NAeP_CFLAGS) -c $< -o $@

$(ASS): $(HDR)
$(INT): $(HDR)
$(OBJ): $(HDR)

NAeP: $(OBJ)
	$(CC) $^ -o $(OUTDIR)/$@ $(NAeP_LDFLAGS)

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
	@rm -fv $(OUTDIR)/NAeP

install: all
	@cp -fv NAeP $(prefix)/bin
uninstall:
	@rm -fv $(prefix)/bin/NAeP

.PHONY: setup options ass int obj all clean install
