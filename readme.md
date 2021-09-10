# NieR:Automata extraction Precept0.0.0

## Table of Contents
0. [Table of Contents](#table-of-contents)
1. [Goal](#goal)
2. [Present](#present)
   1. [Usage](#usage)
3. [Capabilities](#capabilities)
4. [Build](#build)
5. [Todo](#todo)
6. [References](#references)

# Goal  
Extract the media files stored within NieR:Automata's data archives into a
useable/viewable format.  
Originally, this was inspired by another precept called [NME2][NME2], however I
thought it could be done better and more cross platform; this is an attempt at
that.

## Present  
List of features, planned and implemented, is at [Capabilities](#capabilities).

### Usage  
All command-line options are viewable by passing any of `-h`, `-help`, or
`-version`.  Most options are toggleable; for example, passing `-w` before a
bunch of files, all those files will be parsed as WEMs. Passing `-w` again
means any files after will be not be parsed as WEMs and instead will be parsed
differently, depending on the state of what other options are passed.

## Capabilities
What the program can/will be able to do:
1. &#9746; WSP Extraction:  
   * &#9746; Extract all WEMs embedded in arguments to separate, individual
     files (`..._XX.wem`).  
   * &#9746; Option to convert all embedded WEMs to ogg files directly.  
2. &#9746; BNK Extraction:  
    Same capabilities as WSP extraction, with one addition:  
   * &#9744; Recursive extraction: search each passed BNK for WEMs
     referenced in other BNK/WSP files passed on command-line.  
3. &#9746; WEM-to-ogg Conversion:  
   * &#9746; All passed WEMs are converted to ogg, either in-place or to
     separate files (`.ogg`).  
   * &#9746; All WEMs extracted from BNKs or WSPs can be similarly converted.  
4. &#9746; Ogg Regranularization:  
    * &#9746; All passed oggs are regranularized, either in-place or to
      separate files (`..._rvb.ogg`).  
5. &#9746; Logging:  
   Not really a focused 'feature' of the program, but logging options.  
    * &#9746; Option for successively quieter output.  
    * &#9746; Option for completely silent output, no errors or critical
      messages at all.  
    * &#9746; Option for debug output, turning on all possible logs.  
    * &#9746; Option for turning off color/stylized output.  

### Maybe
* &#9744; Videos:  
   * &#9744; Video extraction.  
   * &#9744; Audio rip from videos, either embedded or from extracted.  

## Build  
After cloning, be sure to run `git submodule init` and `git submodule update`.  
Then issue one of the following commands (take note of the notice after the
table):  
(arguments in brackets `[]` are optional).

|Host   |Target |Requirements                        |Command|
|:---:  |:---:  |:---                                |:---|
|\*NIX  |\*NIX  |GNU `make` and toolchain            |`make [HOST=UNIX] [TARGET=UNIX] ...`|
|\*NIX  |Windows|GNU `make` and `mingw-w64` toolchain|`make [HOST=UNIX] TARGET=WINDOWS ...`|
|Windows|\*NIX  |N/A|N/A|
|Windows|Windows|Cygwin with above or mingw          |`make HOST=WINDOWS TARGET=WINDOWS ...`|

**NOTE:** When building for the first time (or if building again for a
different platform than previously), the environment variable `LIBRECONFIG`
must be set to something, anything, or else the `libogg` and `libvorbis`
submodules will be incorrectly configured and almost certainly fail
compilation.  
Be aware that library reconfiguring will take some time.

32/64-bit compilation can be specified by setting the environment variable
`BITS` to either `32` or `64`; defaults to `64`.  
Other environment variables can be set; most ones that are safe to change and
their defaults are listed in `config.mk`.  

**Note:** `libogg` and `libvorbis` are provided and built as submodules; they
need-not be installed on the host/target system to compile/run.  

**DISCLAIMER:** I only have a linux distribution, so I can't/am too lazy to
test building on Windows.  

## Todo  
Eventually, some (hopefully) neat and understandable documentation on the
different formats of data storage (WSP, WEM, BNK, ...) used in NieR:Automata.

## References  
* Ogg regranularization is done as a custom re-implementation of
  [revorb][revorb]; previously revorb was used directly.
* WEM-to-ogg conversion is currently being implemented, with heavy reference
  (though not duplication) from [ww2ogg][ww2ogg].

[NME2]:https://github.com/TypeA2/NME2
[ww2ogg]:https://github.com/hcs64/ww2ogg
[revorbc]:https://github.com/bowtoes/revorbc
[revorb]:http://yirkha.fud.cz/progs/foobar2000/revorb.cpp
