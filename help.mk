help:
	# In order to build NAeP, there are various configuration variables that can/must be enabled for correct
	# results; a list of the more-common variables you may wish to change is given below, and examples of
	# valid commands are given below those.
	#
	# One variable important to cross-compilation is "LIBRECONFIG"; you MUST define it, in any way, in order
	# to compile for a specific target system that is not your host.
	#
	# More specifically, it needs to be defined when you compile for a different target than you did before
	# (if you have already compiled it, it need not be defined the first time); this variable tells the
	# system to completely clean any vendor libraries and rebuild them for the specific target. Not defining
	# it when changing targets will leave Ogg and Vorbis to configured for the wrong target and will result
	# in a great many linkage errors.
	#
	# You don't have to set it for first-time compilation after cloning, library configuration will be done
	# automatically in that case.
	#
	# Here is the list of available make variables:
	#   Host/target configuration
	#     host:
	#       Default: $(def_host); allowed: $(def_host), $(ndef_host)
	#       Tells the scripts the kind of platform they are building on.
	#       Used to determine the appropriate toolchain.
	#     host_bit:
	#       Default: $(def_host_bit); allowed: $(def_host_bit), $(ndef_host_bit)
	#       Tells the scripts the x86 architecture of the system they are building on.
	#       Used to determine the appropriate toolchain.
	#     target:
	#       Default $$(host); allowed: $(def_target), $(ndef_target)
	#       Tells the scripts the kind of platform to build for.
	#       Used to determine the appropriate toolchain.
	#     target_bit:
	#       Default: $$(host_bit); allowed: $(def_target_bit), $(ndef_target_bit)
	#       Tells the scripts the x86 CPU-architecture to build for.
	#       Used to determine the appropriate toolchain.
	#   Output configuration:
	#     prefix:
	#       Default: $(prefix_$(def_host)), $(prefix_$(ndef_host))
	#       The prefix to where output files will be installed.
	#     build_root:
	#       Default: ./build
	#       The root directory for compilation output.
	#     build_subdir_target:
	#       Default: $(build_subdir_target_$(def_host)),
	#                $(build_subdir_target_$(ndef_host))
	#       The subdirectory of $$(build_root) where unix/windows build output will
	#       be placed.
	#     build_subdir_bit:
	#       Default: $(build_subdir_bit_$(def_host_bit)),
	#                $(build_subdir_bit_$(ndef_host_bit))
	#       The subdirectory of $$(build_subdir_target) where 32/64-bit build
	#       output will be placed.
	#     build_tree:
	#       Default: $$(build_root)/$$(build_subdir_target)/$$(build_subdir_bit)
	#       The combination of $$(build_root) and the subdirs; can be overwritten manually if desired.
	#     output_directory:
	#       Default: $$(build_root)/$$(build_tree)
	#       The directory where all compiled files will go, including local vendor installs.
	#   Build configuration:
	#     std:
	#       Default: c11
	#       C standard to compile for; best to just leave it where it is.
	#     pedantic:
	#       Default: 1
	#       Whether to be pedantic about standards compliance; set to 0 to disable.
	#     do_strip:
	#       Default: 1
	#       Enabled stripping of output executables when linking & installing; set to 0 to disable
	#     debug:
	#       Default: 0
	#       Debug build; changes compilation flags to be debug-friendly, and allows the use of the 'memcheck'
	#       variable; set to something other than 0 to enable.
	#     memcheck:
	#       Default: 0
	#       Changes 'debug' compilation flags to be friendly with Valgrind's 'memcheck' tool; set to
	#       something other than 0 to enable.
	#   Toolchain configuration: (note that this is not well-tested, and generally shouldn't be changed from
	#   the defaults, unless you (unlike me) know what you're doing)
	#     See 'platform.mk', starting at '## Vendor Strings', to see the variables used.
	#
	# See config.mk and platform.mk for other variables not mentioned that may be
	# customized.
	#
	# Some examples:
	#   Compiling on 64-bit Linux for 64-bit Linux:
	#     make
	#
	#   Compiling on 64-bit Linux for 32-bit Linux:
	#     make target_bit=32
	#
	#   Compiling on 64-bit Linux for 64-bit Windows:
	#     make target=windows
	#
	#   Compiling on 64-bit Windows for 64-bit Windows:
	#     make host=windows target=windows
	#
	#   Compiling on 32-bit Windows for 32-bit Windows:
	#     make host=windows host_bit=32 target=windows target_bit=32
	#
	# Examples of LIBRECONFIG use:
	#   Compiling on 64-bit Linux for 64-bit Windows, after already compiling for
	#   Linux, or 32-bit Windows:
	#     make target=windows LIBRECONFIG=1
	#
	#   Compiling on 64-bit Windows for 32-bit Windows, after already compiling for
	#   64-bit Windows, or any Linux:
	#     make host=windows target=windows target_bit=32 LIBRECONFIG=1

_delim_1:
	@$(echo) '@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@'
	@$(echo) ''
_delim_2:
	@$(echo) ''
	@$(echo) '@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@'
_info:
	@$(echo) '$(project) v$(project_version)$(if $(project_date), - last commit $(project_date))'
	@$(echo) 'Build Name  . . . . . . .  $(build_name)'
	@$(echo) 'Final Destination . . . .  $(output_file)'
	@$(echo) ''
	@$(echo) 'Build Host  . . . . . . .  $(host_bit)-bit $(host)$(if $(cross_compilation),; $(host_string))'
	@$(echo) 'Build Target  . . . . . .  $(target_bit)-bit $(target)$(if $(cross_compilation),; $(target_string))'
#@$(echo) 'Build Mode         :$(target_mode)'
	@$(echo) 'Build Tree. . . . . . . .  $(build_tree)'
	@$(echo) 'Debug Build . . . . . . .  $(if $(debug:0=),On,Off)'
ifneq ($(debug),0)
	@$(echo) 'Extra Debug Information .  $(if $(extra_debug:0=),On,Off)'
endif
	@$(echo) 'Memcheck Flags  . . . . .  $(if $(debug:0=),$(if $(memcheck:0=),On,Off),Off)'
_build_info:
	@$(echo) 'C Compiler (CC) . . . . .  $(cc_custom)'
	@$(echo) 'Archiver (AR) . . . . . .  $(ar_custom)'
	@$(echo) $(if $(target:unix=),\
	         'DllTool . . . . . . . . .  $(dlltool_custom)','')
	@$(echo) 'Includes  . . . . . . . .  $(c_includes) $(vnd_includes)'
	@$(echo) 'Warnings  . . . . . . . .  $(c_warnings) $(vnd_warnings)'
	@$(echo) 'Defines . . . . . . . . .  $(c_defines) $(vnd_defines)'
	@$(echo) 'Links . . . . . . . . . .  $(c_links) $(vnd_links)'
	@$(echo) ''
	@$(echo) 'Optimization Flags  . . .  $(c_optimization)'
	@$(echo) 'Performance Flags . . . .  $(c_performance)'
	@$(echo) ''
	@$(echo) 'C Flags   . . . . . . . .  $(project_cflags)'
	@$(echo) 'CPP Flags . . . . . . . .  $(project_cppflags)'
	@$(echo) 'LD Flags  . . . . . . . .  $(project_ldflags)'
	@$(echo) ''
	@$(echo) 'Vendor Binaries . . . . .  $(vnd_bins)'
	@$(echo) 'libbrrtools . . . . . . .  $(brrtools_flags)'
	@$(echo) 'libogg Configure  . . . .  $(ogg_configure_flags)'
	@$(echo) 'libvorbis Configure . . .  $(vorbis_configure_flags)'
_output_info:
	@$(echo) 'Output Base Name  . . . . $(output_base_name)'
	@$(echo) 'Output Extension  . . . . $(output_ext)'
	@$(echo) ''
	@$(echo) 'Build Root  . . . . . . . $(build_root)'
	@$(echo) 'Build Tree  . . . . . . . $(build_tree)'
	@$(echo) 'Output Directory  . . . . $(output_directory)'
	@$(echo) 'Output File . . . . . . . $(output_file)'
	@$(echo) ''
	@$(echo) 'Build Directories . . . . $(build_directories)'
	@$(echo) ''
	@$(echo) 'Sources:'
	@$(echo) '    $(srcs)'
	@$(echo) 'Headers:'
	@$(echo) '    $(hdrs)'
	@$(echo) ''
	@$(echo) 'Assemblies:'
	@$(echo) '    $(ass_out)'
	@$(echo) 'Intermediates:'
	@$(echo) '    $(int_out)'
	@$(echo) 'Objects:'
	@$(echo) '    $(obj_out)'
.PHONY: help _delim_1 _delim_2 _info _build_info _output_info
