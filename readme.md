# NAeP - NieR: Automata extraction Protocol

## Table of Contents
0. [Table of Contents](#table-of-contents)
1. [Goal](#goal)
2. [Present](#present)
   1. [Usage](#usage)
3. [Plan](#plan)
4. [Build](#build)
   1. [Linux](#linux)

# Goal
Extract the media files stored within NieR: Automata's data archives into a
useable/viewable format.

Originally, this was inspired by another project called [NME2][NME2],
however I thought it could be done better and more cross platform; this is an attempt
at that.

## Present
Currently, all this does is extract all `.wem` files embedded in `.wsp` files; it
doesn't do anything like convert the Wwise audio to ogg (like [ww2ogg][ww2ogg], yet)
or [revorb][revorbc] the files (yet); it is the plan to have those processes
integrated into this single program however.

### Usage
Run `NAeP` on any number of `.wsp` (wisp) files and the program will extract all
embedded `.wem` (weem) files, with an incrementing numerical suffix on the original
wisp filename. Most of these files (though not all for some reason) can be converted
to ogg with [ww2ogg][ww2ogg], and then re-encoded into a playable form with [revorb][revorbc].

* [ ] Video extraction? Maybe.  

## Capabilities
What the program can/will be able to do:
1. [ ] Wisp Extraction:  
   * [ ] Extract all weems embedded in arguments to separate, individual files.  
   * [ ] Extract all weems AND convert those weems to oggs.  
   * [ ] Convert all embedded weems to ogg files directly, without extracting them.  
2. [ ] Bank Extraction:  
    Same capabilities as wisp extraction, with one addition:  
   * [ ] Recursive extraction: search all passed bank files for weems referenced in
     other bank/wisp files passed on command-line.
3. [ ] Weem-to-ogg Conversion:  
   * [ ] All passed weems (and extracted weems, if specified) are converted to ogg,
     either in-place or to separate files.
4. [ ] Ogg Revorption:  
    * [ ] All passed oggs (and generated oggs, if specified) are revorbed, either in-place
     or to separate files (`..._revorb.ogg`).
5. Videos:  
    * Maybe? If so then:
      * [ ] Video extraction.
      * [ ] Audio rip from videos, either embedded or from extracted.

## Build
I only have a linux system and do not have convenient access to a Windows system, so
for now there is only a Make build system; the plan is to eventually make this at least
cross-platform and cross-compilable between linux and Windows, maybe more in the future.

### Linux
To build, simply run `make` in the top level directory; there is `config.mk` for configuring
different build parameters and output directories.

## Todo
Eventually, some (hopefully) neat and understandable documentation on the different formats
of data storage (wisp, weem, bank, ...) used in NieR: Automata.

[NME2]:https://github.com/TypeA2/NME2
[ww2ogg]:https://github.com/hcs64/ww2ogg
[revorbc]:https://github.com/bowtoes/revorbc
