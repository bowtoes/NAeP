.POSIX:
.SUFFIXES:
all:

include config.mk
include help.mk

info: _delim_1 _info _delim_2
build-info: _delim_1 _build_info _delim_2
output-info: _delim_1 _output_info _delim_2
all-info: _delim_1 _info _build_info _output_info _delim_2
.PHONY: info build_info output_info all_info

ass_dir ?= ass
ass_out_dir ?= $(output_directory)/$(ass_dir)
ass_out := $(addprefix $(ass_out_dir)/,$(srcs:.c=.s))

int_dir ?= int
int_out_dir ?= $(output_directory)/$(int_dir)
int_out := $(addprefix $(int_out_dir)/,$(srcs:.c=.e))

obj_dir ?= obj
obj_out_dir ?= $(output_directory)/$(obj_dir)
obj_out := $(addprefix $(obj_out_dir)/,$(srcs:.c=.o))

build_directories := $(sort $(dir $(ass_out) $(int_out) $(obj_out)))

all: info $(project)
setup:
	@$(mk_dir_tree) $(build_directories) 2>$(null) ||:
.PHONY: all setup

$(ass_out_dir)/%.s: $(src_dir)/%.c ; $(cc_custom) $(project_cppflags) $(project_cflags) -S $< -o $@
$(int_out_dir)/%.e: $(src_dir)/%.c ; $(cc_custom) $(project_cppflags) $(project_cflags) -E $< -o $@
$(obj_out_dir)/%.o: $(src_dir)/%.c ; $(cc_custom) $(project_cppflags) $(project_cflags) -c $< -o $@
$(ass_out) $(int_out) $(obj_out): $(addprefix $(src_dir)/,$(hdrs)) $(makefiles)

ass: info setup $(ass_out)
int: info setup $(int_out)
obj: info setup $(obj_out)
aio: ass int obj
.PHONY: ass int obj aio

$(output_file): vnd $(obj_out) $(makefiles) $(addprefix $(src_dir)/,$(hdrs))
	$(cc_custom) -o $@ $(obj_out) $(vnd_bins) $(project_ldflags)
$(project): setup $(output_file)

clean:
	@$(rm_file) $(output_file) $(ass_out) $(int_out) $(obj_out) 2>$(null) ||:
	@$(rm_recurse) $(build_directories) 2>$(null) ||:

again: clean $(project)

CLEAN: vnd-clean clean
AGAIN: CLEAN all

install: all
	@$(mk_dir_tree) '$(prefix)/bin'
	@$(copy_file) '$(output_file)' '$(prefix)/bin'
	@$(strip_exe) '$(prefix)/bin/$(output_name)'
uninstall:
	@$(rm_file) '$(prefix)/bin/$(output_name)'

### Vendor shit
vnd:
vnd-clean:
vnd-again: vnd-clean vnd
.PHONY: vnd vnd-clean vnd-again

## Brrtools
brrtools_dir := $(vnd_dir)/brrtools
brrtools_out_dir := $(output_directory)/$(brrtools_dir)
brrtools_bin := $(brrtools_out_dir)/lib/libbrrtools.a

vnd_includes += '$(brrtools_out_dir)/include'
vnd_bins += '$(brrtools_bin)'

brrtools_flags :=\
	prefix='$(brrtools_out_dir)'\
	PEDANTIC=1\
	DO_STRIP=1\
	DO_LDCONFIG=0\
	PIC=$(PIC)\
	HOST=$(if $(host:unix=),WINDOWS,UNIX)\
	HOST_BIT=$(host_bit)\
	TARGET=$(if $(target:unix=),WINDOWS,UNIX)\
	TARGET_BIT=$(target_bit)\
	TARGET_MODE=STATIC\
	TARGET_PART_WINDOWS_STATIC='.a'
ifneq ($(debug),0)
 brrtools_flags += DEBUG=$(debug)
 ifneq ($(memcheck),0)
  brrtools_flags += MEMCHECK=$(memcheck)
 endif
endif

$(brrtools_bin): $(makefiles)
	cd '$(brrtools_dir)' && \
		$(MAKE) install $(brrtools_flags)
brrtools: $(brrtools_bin)
brrtools-clean:
	cd '$(brrtools_dir)' && \
		$(MAKE) clean uninstall $(brrtools_flags) ||:
brrtools-again: brrtools-clean brrtools

vnd: brrtools
vnd-clean: brrtools-clean
.PHONY: brrtools brrtools-clean brrtools-again

## OGG
ogg_dir := $(vnd_dir)/ogg
ogg_out_dir := $(output_directory)/$(ogg_dir)
ogg_bin := $(ogg_out_dir)/lib/libogg.a
ogg_makefile := $(ogg_dir)/Makefile

vnd_includes += '$(ogg_out_dir)/include'
vnd_bins += '$(ogg_bin)'

ogg_build_str := $(target_architecture)-pc-linux
ifeq ($(target),windows)
 ogg_host_str := $(target_string)
 ogg_target_str := $(target_string)
endif
ogg_configure_flags :=\
	--disable-dependency-tracking\
	--prefix='$(ogg_out_dir)'\
	--disable-shared\
	$(if $(ogg_build_str),--build=$(ogg_build_str))\
	$(if $(ogg_host_str),--host=$(ogg_host_str))\
	$(if $(ogg_target_str),--target=$(ogg_target_str))\

ifdef LIBRECONFIG
 ogg_reconfig_target := ogg_reconfig
.PHONY: ogg_reconfig
else
 ogg_reconfig_target := $(ogg_makefile)
endif

$(ogg_reconfig_target):
	@$(echo) '################################################################################'
	@$(echo) 'Ogg RECONFIGURE'
	@$(echo) '################################################################################'
	cd '$(ogg_dir)' && $(MAKE) maintainer-clean 2>$(null)||:
	cd '$(ogg_dir)' && ./autogen.sh
	cd '$(ogg_dir)' && ./configure $(ogg_configure_flags)
$(ogg_bin): $(ogg_reconfig_target)
	@$(echo) '################################################################################'
	@$(echo) 'Ogg INSTALL'
	@$(echo) '################################################################################'
	cd '$(ogg_dir)' && $(MAKE) install prefix='$(ogg_out_dir)'
ogg: $(ogg_bin)
ogg-clean:
	@$(echo) '################################################################################'
	@$(echo) 'Ogg CLEAN'
	@$(echo) '################################################################################'
	cd '$(ogg_dir)' && $(MAKE) uninstall prefix='$(ogg_out_dir)' clean 2>$(null)||:
ifdef LIBRECONFIG
	cd '$(ogg_dir)' && $(MAKE) maintainer-clean 2>$(null)||:
endif
	$(rm_recurse) '$(ogg_out_dir)' 2>$(null)||:
ogg-again: ogg-clean ogg

vnd: ogg
vnd-clean: ogg-clean
.PHONY: ogg ogg-clean ogg-again

## Vorbis
vnd_links += -lm
ifeq ($(target),unix)
 vnd_links += -lmvec
endif

vorbis_dir := $(vnd_dir)/vorbis
vorbis_out_dir := $(output_directory)/$(vorbis_dir)
vorbis_bin := $(vorbis_out_dir)/lib/libvorbis.a
vorbis_makefile := $(vorbis_dir)/Makefile

vnd_includes += '$(vorbis_out_dir)/include'
vnd_bins += '$(vorbis_bin)'

ifeq ($(target),windows)
 vorbis_build_str := $(target_architecture)-linux
 vorbis_host_str := $(target_string)
 vorbis_target_str := $(target_string)
endif
vorbis_configure_flags :=\
	--disable-dependency-tracking\
	--enable-silent-rules\
	--prefix='$(vorbis_out_dir)'\
	--with-ogg='$(abspath $(ogg_out_dir))'\
	--disable-shared --disable-docs --disable-examples --disable-oggtest\
	$(if $(vorbis_host_str),--host=$(vorbis_host_str))\
	$(if $(vorbis_host_str),--target=$(vorbis_target_str))\
	$(if $(vorbis_host_str),--build=$(vorbis_build_str))\

ifdef LIBRECONFIG
 vorbis_reconfig_target := vorbis_reconfig
.PHONY: vorbis_reconfig
else
 vorbis_reconfig_target := $(vorbis_makefile)
endif
$(vorbis_reconfig_target):
	@$(echo) '################################################################################'
	@$(echo) 'Vorbis RECONFIGURE'
	@$(echo) '################################################################################'
	cd '$(vorbis_dir)' && $(MAKE) maintainer-clean 2>$(null)||:
	cd '$(vorbis_dir)' && ./autogen.sh
	cd '$(vorbis_dir)' && ./configure $(vorbis_configure_flags)

$(vorbis_bin): $(ogg_bin) $(vorbis_reconfig_target)
	@$(echo) '################################################################################'
	@$(echo) 'Vorbis INSTALL'
	@$(echo) '################################################################################'
	cd '$(vorbis_dir)' && $(MAKE) install prefix='$(vorbis_out_dir)'
vorbis: $(vorbis_bin)
vorbis-clean:
	@$(echo) '################################################################################'
	@$(echo) 'Vorbis CLEAN'
	@$(echo) '################################################################################'
	cd '$(vorbis_dir)' && $(MAKE) uninstall prefix='$(vorbis_out_dir)' clean 2>$(null)||:
ifdef LIBRECONFIG
	cd '$(vorbis_dir)' && $(MAKE) maintainer-clean 2>$(null)||:
endif
	$(rm_recurse) '$(vorbis_out_dir)' 2>$(null)||:
vorbis-again: vorbis-clean vorbis

vnd: vorbis
vnd-clean: vorbis-clean
.PHONY: vorbis vorbis-clean vorbis-again
