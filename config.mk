# This file is the primary configuration point for the build process, including
# various compilation flags, output binary/ies name/s, project version, etc.

include platform.mk

project ?= NAeP
# prefix for project-specific macro defines
uproject ?= Ne

# used as a prerequisite in some recipes, to make sure that updating the makefiles
# triggers a rebuild
override makefiles := platform.mk config.mk Makefile

override project_major := 0
override project_minor := 0
override project_revis := 2
override project_letter := b
override project_version := $(project_major).$(project_minor).$(project_revis)$(project_letter)
override project_date := $(shell git show -s --date=format:'%Y/%m/%d %l:%M%p' --format=%ad)

src_dir := src
vnd_dir := vnd

srcs :=\
	main.c\
	codebook_library.c\
	input.c\
	lib.c\
	packer.c\
	print.c\
	process.c\
	process/bnk.c\
	process/ogg.c\
	process/wem.c\
	process/wsp.c\
	riff.c\
	wsp_meta.c\
	wwise.c\

hdrs :=\
	codebook_library.h\
	errors.h\
	input.h\
	lib.h\
	packer.h\
	print.h\
	process.h\
	riff.h\
	riff_extension.h\
	wsp_meta.h\
	wwise.h\

## These variables must be set to exclusively 0 to disable them
# Be pedantic about the source files when compiling
pedantic ?= 1
# Strip executable(s) when installing
do_strip ?= 1
# Enable debug compiler flags
debug ?= 0
# Enable valgrind memcheck-compitable debug flags (only takes effect when debug != 0)
memcheck ?= 0

## Toolchain flags
# c standard to use
std ?= c11

c_links =
c_includes = '$(src_dir)'
c_warnings =\
	-Wall\
	-Wextra\
	-Wno-unused-function\
	-Wno-sign-compare\
	-Wno-unused-parameter\
	-Wno-unused-variable\
	-Werror=implicit-function-declaration\
	-Werror=missing-declarations\

c_defines := \
	-D$(uproject)_major=$(project_major)\
	-D$(uproject)_minor=$(project_minor)\
	-D$(uproject)_revis=$(project_revis)\
	-D$(uproject)_version='"$(project_version)"'\
	-D$(uproject)_host_$(host)\
	-D$(uproject)_host_bit=$(host_bit)\
	-D$(uproject)_target_$(target)\
	-D$(uproject)_target_bit=$(target_bit)\
	-D$(uproject)_target_mode_$(target_mode)\
	-DRIFF_EXTENDED='"riff_extension.h"'\

# These values are filled in in the Makefile, where vendor build stuffs is done.
vnd_bins :=
vnd_links :=
vnd_includes :=
vnd_defines :=
vnd_cflags :=

ifeq ($(target),unix)
 c_defines := -D_XOPEN_SOURCE=700 -D_POSIX_C_SOURCE=200809L $(c_defines)
else
 c_defines := -DWIN32_LEAN_AND_MEAN $(c_defines)
endif

ifneq ($(PEDANTIC),0)
 c_warnings := -pedantic -pedantic-errors -Wpedantic $(c_warnings)
 c_defines += -D$(uproject)_pedantic
endif

c_optimization :=
c_performance :=
ifeq ($(debug),0)
 c_optimization += -O3 -Ofast
 c_links += -Wl,--strip-all
 #ifeq ($(target_mode),shared)
 # c_performance += -s
 #else
 # c_performance += -static-pie
 #endif
else
 c_optimization += -O0 -g
 c_defines += -D$(uproject)_debug
 ifeq ($(memcheck),0)
  c_performance += -pg -no-pie
 else
  c_defines += -D$(uproject)_memcheck
 endif
endif
c_performance += -m$(target_bit)

#ifeq ($(target_mode),shared)
# c_links += -shared
# ifeq ($(target),windows)
#  c_links += -Wl,--subsystem,windows
#  c_defines += -D$(uproject)_exports
# endif
#else
# c_links += -static-pie
#endif

ifdef PIC
 ifneq ($(PIC),pic)
  ifneq ($(PIC),PIC)
   ifneq ($(PIC),pie)
    ifneq ($(PIC),PIE)
     override PIC:=PIC
    endif
   endif
  endif
 endif
endif
ifneq ($(PIC),)
 c_performance += -f$(PIC)
endif

project_cflags = \
	-std=$(std)\
	$(c_warnings)\
	$(c_optimization)\
	$(c_performance)\
	$(vnd_cflags)\
	$(CFLAGS)
project_cppflags = \
	$(c_defines)\
	$(vnd_defines)\
	$(addprefix -I,$(c_includes) $(vnd_includes))\
	$(STCPPFLAGS)\
	$(CPPFLAGS)
project_ldflags = \
	$(c_performance)\
	$(c_links)\
	$(vnd_links)\
	$(LDFLAGS)

### Build-output variables
$(eval $(call dUnixWindowsVar,prefix,/usr/local,$(CURDIR)/install,host))

build_subdir_target_unix ?= uni
build_subdir_target_windows ?= win
build_subdir_target ?= $(build_subdir_target_$(target))

build_subdir_mode_shared ?= shared
build_subdir_mode_static ?= static
build_subdir_mode ?= $(build_subdir_mode_$(target_mode))

build_subdir_bit_64 ?= 64
build_subdir_bit_32 ?= 32
build_subdir_bit ?= $(build_subdir_bit_$(target_bit))

build_root ?= $(CURDIR)/build
build_tree ?= $(build_subdir_target)/$(build_subdir_bit)
output_directory ?= $(build_root)/$(build_tree)

output_ext_unix_shared ?=
output_ext_unix_static ?=
output_ext_windows_shared ?= .exe
output_ext_windows_static ?= .exe
output_ext ?= $(output_ext_$(target)_$(target_mode))

output_base_name ?= $(project)
output_name ?= $(output_base_name)$(output_ext)
output_file ?= $(output_directory)/$(output_name)

# windows libraries only
build_defines ?= $(output_base_name).def
output_defines ?= $(output_directory)/$(build_defines)
build_imports ?= $(output_base_name).dll.lib
output_imports ?= $(output_directory)/$(build_imports)

override dVerifySetting :=
override dGenExe :=
override dUnixWindowsVar :=
