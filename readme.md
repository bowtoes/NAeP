# NieR:Automated extraction Precept_v0.0.2
C11 device with the goal of extracting and converting NieR:Automata and
Replicant audio data into useable [Ogg][oggvorbis] files.

# Table of Contents
1. [Goal](#goal)
   1. [Guide](#guide)
   1. [Capabilities](#capabilities)
   1. [Usage](#usage)
1. [Build](#build)
1. [Todo](#todo)
1. [References](#references)

## Goal
Extract the media files stored within NieR:Automata's data archives into a
useable/viewable format.  

### Guide
In NieR:Automata's sound directories are multiple files of a few different
types; these types by file-suffix and what they are is given:

* `.wem`:  
  &emsp;These files are Ogg/Vorbis audio data stored in a custom format, namely a
  version one of AudioKinetic's Wwise sound format. This format (herein called
  WwRIFF) is a modified form of Microsoft's RIFF storage format, and this
  precept can convert such files into usable Ogg/Vorbis files that can then be
  played/converted as normal.

* `.wsp`:  
  &emsp;Another format common to Wwise, is the `.wsp` format (*wisp*). These
  files are simple, each being little more than the concatenation of multiple
  WwRIFFs into one `.wsp`. There is one caveat, and that's that each
  WwRIFF in the `.wsp` is padded at the end with zeros to the nearest
  multiple of 2048 bytes (found with [countwsp][countwsp]).

* `.bnk`:  
  &emsp;`.bnk` (*bank*) files are a little tougher, though. Right now, this precept treats them
  no differently than `.wsp`, as they can also contain whole WwRIFFs, though
  not in as simple simple a way.

NieR Replicant is a little different than Automata, storing most of its data in
`.pck` files; however this precept can still convert such files (though it
doesn't know how to automatically). To convert these files, they must be treated
as `.wsp` files (specifiable on the command-line with `-W`/`-wsp`).

### Capabilities
* &#9746; WwRIFF-to-Ogg Conversion:  
   * &#9746; All passed WwRIFFs (`.wem`) are converted to Ogg, either in-place
     (overwriting input) or to separate files (`[wem_name].ogg`).  
* &#9746; `.wsp`/`.bnk` Extraction:  
   * &#9746; Extract all WwRIFFs embedded in arguments to separate,
     individual files (`[wsp_name]_XX.wem`).  
   * &#9746; All WwRIFFs extracted this way can be directly converted to Ogg
     by toggling a command-line argument.  
* &#9746; Ogg Regranularization:  
    * &#9746; All passed oggs are regranularized, either in-place or to
      separate files (`[ogg_name]_rvb.ogg`).  
* &#9746; Logging:  
   Not a primary focus, but there are various settings for logging:
    * &#9746; `-q` for quieter output; can be passed multiple times.  
    Pass `+q` to undo this.
    * &#9746; `-Q` or `-qq` to completely disable output logging.  
    * &#9746; `-d` to enable debug (and all other) output, irrespective of the
      above settings (only works in debug builds).  
    * &#9746; `-c` to disable colored logging for given inputs, or `-C` to
      disable all log styling.  
      Windows has no log styling.

### Usage
`NAeP [ARGUMENTS ... [FILES ...]] ...`

&emsp;All command-line options are viewable by passing any of `-h`, `-help`, or
`-version`, or by looking in `src/print.c`.  
Most options are toggleable; for example, by passing `-w` before a bunch of
files, all those files will be processed as if they're `.wem`s.  Passing `-w`
again will revert to the default processing model, where the type of file is
determined automatically.  
A few options are global, meaning that their order doesn't matter and they do
not apply to any individual files; these are marked in the helptext with a
`(g)`.

In order to  convert WwRIFF to Ogg files, codebooks are needed. For some
WwRIFFs, these are provided in the file itself (`inline`); for others, they
are provided but in a stripped-down form (`stripped`) and must be rebuilt; and
for others still, they aren't provided at all.  
For those last two, external codebook libraries must be used, and these are
provided in `codebooks.zip`. In this are two folders:
```
codebooks.zip/
├── codebooks/
│   ├── codebooks_aoTuV_603.cbl
│   └── codebooks_vanilla.cbl
└── codebooks_alt/
    ├── codebooks_aoTuV_603.ocbl
    └── codebooks_vanilla.ocbl
```
`.cbl` and `.ocbl` files differ only in the technical details, which are
unimportant to their use.  
In either case, these files are sourced from [`ww2ogg`][ww2ogg]; where they
originally came from or how they were created is undocumented.

To specify what codebook library to use for some given WwRIFFs, use `-cbl
[codebook_library].cbl` prior to the input files.

*Note:* If you are encountering trouble converting some WwRIFFs, it's worth
specifying different combinations of the command-line arguments
`-stripped`/`-inline`; these settings currently aren't (or maybe can't be)
automatically determined.

## Build
*Note:* For a more in-depth list and explanation, run `make help` or check
`help.mk`.

&emsp;This precept requires [`libogg`][libogg] and [`libvorbis`][libvorbis], and
makes use of [`brrtools`][brrtools]; these are provided as submodules, and
after cloning, run `git submodule init` and `git submodule update` to
initialize them.  
*Note:* There is currently no way to build with external versions of these
libraries.

In order to build, you'll need GNU Make, and to build `libogg`/`libvorbis`, GNU
Autotools is also needed.

If you look through the various `.mk` files, you'll see a plethora of various
settings that can be customized (they're assigned with `?=`); here are only the
primary ones for build customization:

| Host    | Target  | Command                                |
| :---    |  ---:   | :---                                   |
| Unix    | Unix    | `make [target=unix] [host=unix] ...`   |
| Unix    | Windows | `make target=windows [host=unix] ...`  |
| Windows | Unix    | Not implemented                        |
| Windows | Windows | `make target=windows host=windows ...` |

*Note:* When first compiling, or when cross-compiling, `libogg` and `libvorbis`
will need to be configured for the correct host/target pair; this is done
automatically the first time, but must be specified manually when
cross-compiling. This is done by defining the Make variable `LIBRECONFIG`.  
Be aware that library reconfiguring will take some time.

Host and target architectures can be specified with `host_bit` and `target_bit`
respectively, either `32` or `64`; choosing target systems/architectures
different from your host will require the appropriate toolchain to be installed
(32-bit toolchain for 32-bit targets built on 64-bit machines and vice-versa,
Windows toolchain for Windows targets built on unix machines, etc.).

Changing targets between builds necessitates setting `LIBRECONFIG` to properly
configure `libogg` and `libvorbis`.

**Disclaimer:** I only have a Linux distribution, so I can't/am too lazy to
test building on Windows or other Unixes (don't be surprised if it doesn't
work).  

## Todo
* Bug testing, (Buster) crash testing, log testing, all testing.
* Better output/process logging.
* More consistent application of logging settings.
* Improved processed output filtering feature(s).
* Better documentation, everywhere.
* Options to specify system-installed [`libogg`][libogg] & [`libvorbis`][libvorbis], so they need not
  be compiled manually; will be a headache to get working for Windows
  systems.

## References
* Ogg regranularization was initially done using a wrapper around [revorb][revorb]
  (wrapper is [revorbc][revorbc]).
* WwRIFF-to-Ogg conversion was first implemented with heavy reference to [ww2ogg][ww2ogg].
* Of course, the official [Ogg][libogg]/[Vorbis][libvorbis] documentation was also
  referenced (a custom-edited (prettified) version of the HTML docs, to be
  precise).

This precept was initially inspired by another called [NME2][NME2]; it could be
done better and more cross platform. This is an attempt at that.

[NME2]:https://github.com/TypeA2/NME2
[ww2ogg]:https://github.com/hcs64/ww2ogg
[revorb]:http://yirkha.fud.cz/progs/foobar2000/revorb.cpp
[revorbc]:https://github.com/bowtoes/revorbc
[countwsp]:https://github.com/bowtoes/countwsp
[brrtools]:https://github.com/bowtoes/brrtools
[libogg]:https://xiph.org/ogg/doc/
[libvorbis]:https://xiph.org/vorbis/doc/
