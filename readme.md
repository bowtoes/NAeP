# NAeP - NieR: Automata extraction Protocol

## Table of Contents
0. [Table of Contents](#table-of-contents)
1. [Goal](#goal)
2. [Present](#present)
   1. [Usage](#usage)
3. [Capabilities](#capabilities)
4. [Build](#build)
   1. [Linux](#linux)
5. [Todo](#todo)

# Goal
Extract the media files stored within NieR: Automata's data archives into a
useable/viewable format.

Originally, this was inspired by another project called [NME2][NME2],
however I thought it could be done better and more cross platform; this is an attempt
at that.

## Present
List of features, planned and implemented, is at [Capabilities](#capabilities).
Revorption is done using modified code of [revorb][revorb] (custom version at [revorbc][revorbc]).
Weem-to-ogg conversion is not implemented yet; may or may not make use of similar code
from [ww2ogg][ww2ogg], to be seen.

### Usage
All command-line options are viewable by passing any of `-h`, `-help`, or `-version`.
Most options are toggleable, meaning, for example, passing `-w` before a bunch of files,
all those files will be parsed as weems. Passing `-w` again means any files after
will be not be parsed as weems and instead will be parsed differently, depending
on the `state` of other options.

## Capabilities
What the program can/will be able to do:
1. [ ] Wisp Extraction:  
   * [ ] Extract all weems embedded in arguments to separate, individual files.  
   * [ ] Extract all weems AND convert those weems to separate oggs.  
   * [ ] Convert all embedded weems to ogg files directly, without extracting them
   to separate files.  
2. [ ] Bank Extraction:  
    Same capabilities as wisp extraction, with one addition:  
   * [ ] Recursive extraction: search all passed bank files for weems referenced in
     other bank/wisp files passed on command-line.  
3. [ ] Weem-to-ogg Conversion:  
   * [ ] All passed weems are converted to ogg, either in-place or to separate files.  
   * [ ] All extracted weems can be similarly converted automatically.  
4. [x] Ogg Revorption:  
    * [x] All passed oggs are revorbed, either in-place or to separate files (`..._rvb.ogg`).  
    * [ ] All converted weems can be similarly revorbed automatically.  
5. [x] Logging:  
   Not really a focused `feature` of the program, but logging options.  
    * [x] Option for successively quieter output.  
    * [x] Option for completely silent output, no errors or critical messages at all.  
    * [x] Option for debug output, turning on all possible logs.  
    * [x] Option for turning off color output.  
* [ ] Videos:  
    Maybe? If so then:
   * [ ] Video extraction.
   * [ ] Audio rip from videos, either embedded or from extracted.

## Build
I only have a linux system and do not have convenient access to a Windows system, so
for now there is only a Make build system; the plan is to eventually make this at least
cross-platform and eventually cross-compilable between linux and Windows, maybe more in the future.

### Linux
Assumes GNU Make. To build, simply run `make` in the top level directory; there is `config.mk` for configuring
different build parameters and output directories.

## Todo
Eventually, some (hopefully) neat and understandable documentation on the different formats
of data storage (wisp, weem, bank, ...) used in NieR: Automata.

[NME2]:https://github.com/TypeA2/NME2
[ww2ogg]:https://github.com/hcs64/ww2ogg
[revorbc]:https://github.com/bowtoes/revorbc
[revorb]:http://yirkha.fud.cz/progs/foobar2000/revorb.cpp
