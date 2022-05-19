# This file provides/verifies variables used for identifying the host platform
# and compilation target platform, as well as ensuring the correct toolchain
# (unless explicitly overwritten) is used.

# Verify that the variable to give is one of two allowed values, called OPT_1 and OPT_2.
# $1: variable you want to verify.
# $2 [optional]: Whatever allowed value $1 takes, the variable with this name will take the other.
#                So if $1 takes OPT_1, $4 will be set to OPT_2, and vice-versa.
# $3: first allowed value, OPT_1.
# $4: second allowed value, OPT_2; if blank, OPT_1 will be the only allowed value.
define dVerifySetting
ifndef $1
 $1 := $3
 $(and $4,$2,override $2 := $4)
else ifeq ($$($1),$3)
 $(and $4,$2,override $2 := $4)
else ifneq ($4,)
 ifeq ($1,$4)
  $(if $2,override $2 := $4)
 else
  $1 := $3
  $(if $2,override $2 := $4)
 endif
else
 $1 := $3
 $(and $4,$2,override $2 := $4)
endif
endef

define dNewline


endef

## Platform configuration settings
# Defaults
$(eval $(call dVerifySetting,def_host,ndef_host,unix,windows))
$(eval $(call dVerifySetting,def_host_bit,ndef_host_bit,64,32))
$(eval $(call dVerifySetting,def_target,ndef_target,unix,windows))
$(eval $(call dVerifySetting,def_target_bit,ndef_target_bit,64,32))
$(eval $(call dVerifySetting,def_target_mode,ndef_target_mode,static,shared))
# Actuals
$(eval $(call dVerifySetting,host,,$(def_host),$(ndef_host)))
$(eval $(call dVerifySetting,host_bit,,$(def_host_bit),$(ndef_host_bit)))
$(eval $(call dVerifySetting,target,,$(def_target),$(ndef_target)))
$(eval $(call dVerifySetting,target_bit,,$(def_target_bit),$(ndef_target_bit)))
$(eval $(call dVerifySetting,target_mode,,$(def_target_mode),$(ndef_target_mode)))

ifneq ($(host)$(host_bit),$(target)$(target_bit))
 override cross_compilation := 1
else
 override cross_compilation :=
endif

### Platform-dependent toolchain setup
ifeq ($(host),windows)
 ifeq ($(target),unix)
  $(error Windows cross-compilation to unix unsupported)
 endif
endif

## Vendor strings
# Variables
architecture_32 ?= i686
architecture_64 ?= x86_64
override host_architecture := $(architecture_$(host_bit))
override target_architecture := $(architecture_$(target_bit))
vendor_unix ?= pc
vendor_windows ?= w64
override host_vendor := $(vendor_$(host))
override target_vendor := $(vendor_$(target))
os_unix ?= linux-gnu
os_windows ?= mingw32
override host_os := $(os_$(host))
override target_os := $(os_$(target))
# The strings
override host_string := $(host_architecture)-$(host_vendor)-$(host_os)
override target_string := $(target_architecture)-$(target_vendor)-$(target_os)

## Tool chain executables
# $1 = tool name
# $2 = make variable (CC, AR, LD, ...)
# $3 = default executable (for unix)
define dGenExe
ifneq ($3,)
 $1_def ?= $3
else
 $1_def ?= $($2)
endif
ifndef $1_custom
 $1_custom = $$($1_def)
 ifneq ($(host), $(target))
  ifeq ($(host), unix)
   $1_custom = $(target_string)-$$($1_def)
  endif
 endif
endif
endef

$(eval $(call dGenExe,cc,CC,gcc))
$(eval $(call dGenExe,ar,AR,gcc-ar))
$(eval $(call dGenExe,dlltool,,dlltool))

# $1 = output variable name
# $2 = unix value
# $3 = windows value
# $4 = host or target
define dUnixWindowsVar
$1_unix ?= $2
$1_windows ?= $3
$1 ?= $$($1_$($4))
endef

$(eval $(call dUnixWindowsVar,ldconfig,,target))

## System commands
$(eval $(call dUnixWindowsVar,null,/dev/null,null,host))
$(eval $(call dUnixWindowsVar,rm_file,rm -v,del,host))
$(eval $(call dUnixWindowsVar,rm_dir,rmdir -v --ignore-fail-on-non-empty,rmdir,host))
$(eval $(call dUnixWindowsVar,rm_recurse,rm -rv,rmdir /s /q,host))
$(eval $(call dUnixWindowsVar,mk_dir_tree,mkdir -pv,helpers/win/mkdir_tree.bat,host))
$(eval $(call dUnixWindowsVar,copy_file,cp -fuv,copy /y,host))
$(eval $(call dUnixWindowsVar,strip_exe,strip -s -M -v -x,:,host))
$(eval $(call dUnixWindowsVar,echo,echo,echo,host))
