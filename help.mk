help:
	# In order to build NAeP, there are various configuration variables that
	# can/must be enabled for correct results; a list of the more-common variables
	# you may wish to change is given below, and examples of valid commands are
	# given below those.
	#
	# One variable important to cross-compilation is "LIBRECONFIG"; you MUST define
	# it, in any way, in order to compile for a specific target system.
	#
	# More specifically, it needs to be defined when you compile for a different
	# target than you did before (if you HAVE already compiled it); not doing so
	# will leave Ogg and Vorbis to be configured for the wrong target.
	#
	# You don't have to set it for first-time compilation from GitHub, library
	# reconfiguration will be done automatically in that case.
	#
	# Here is the list of available make variables:
	#   Host/target configuration
	#     host:
	#       Default: $(def_host); allowed: $(def_host), $(ndef_host)
	#       Tells the scripts the kind of platform they are building on.
	#       Used to determine the correct toolchain.
	#     host_bit:
	#       Default: $(def_host_bit); allowed: $(def_host_bit), $(ndef_host_bit)
	#       Tells the scripts the architecture of the system they are building on.
	#       Used to determine the correct toolchain.
	#     target:
	#       Default $$(host); allowed: $(def_target), $(ndef_target)
	#       Tells the scripts the kind of platform to build for.
	#       Used to determine the correct toolchain.
	#     target_bit:
	#       Default: $$(host_bit); allowed: $(def_target_bit), $(ndef_target_bit)
	#       Tells the scripts the cpu-architecture to build for.
	#       Used to determine the correct toolchain.
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
	#       The combination of $$(build_root) and the subdirs; can be changed
	#       manually if desired.
	#     output_directory:
	#       Default: $$(build_root)/$$(build_tree)
	#       The directory where all compiled files will go, including local vendor
	#       installs.
	#   Build configuration:
	#     std:
	#       Default: c11
	#       C standard to compile for; best to just leave it where it is.
	#     pedantic:
	#       Default: 1
	#       Whether to be pedantic about standards compliance; set to 0 to disable.
	#     do_strip:
	#       Default: 1
	#       Enabled stripping of output executables when linking & installing; set
	#       to 0 to disable
	#     debug:
	#       Default: 0
	#       Debug build; changes compilation flags to be debug-friendly, and allows
	#       the use of the 'memcheck' variable; set to anything other than 0 to
	#       enable.
	#     memcheck:
	#       Default: 0
	#       Changes 'debug' compilation flags to be friendly with valgrind's
	#       'memcheck' tool; set to anythin other than 0 to enable.
	#   Toolchain configuration:
	#   (note that this is not well-tested, and generally shouldn't be changed from
	#   the defaults)
	#     See 'platform.mk', starting at '## Vendor Strings', to see the variables
	#     used.
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
