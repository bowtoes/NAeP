# NAeP - Nier: Automata extraction Protocol

## Table of Contents
0. [Table of Contents](#table-of-contents)
1. [GOAL](#goal)
2. [PRESENT](#present)
   1. [Usage](#usage)
3. [PLAN](#plan)
4. [BUILD](#build)
   1. [Linux](#linux)

# GOAL
Extract the media files stored within NieR: Automata's data archives into a
useable/viewable format.

Originally, this was inspired by another project called [NME2][NME2],
however I thought it could be done better and more cross platform; this is an attempt
at that.

## PRESENT
Currently, all this does is extract all `.wem` files embedded in `.wsp` files; it
doesn't do anything like convert the Wwise audio to ogg (like [ww2ogg][ww2ogg], yet)
or [revorb][revorbc] the files (yet); it is the plan to have those processes
integrated into this single program however.

### Usage
Run `NAeP` on any number of `.wsp` (wisp) files and the program will extract all
embedded `.wem` (weem) files, with an incrementing numerical suffix on the original
wisp filename. Most of these files (though not all for some reason) can be converted
to ogg with [ww2ogg][ww2ogg], and then re-encoded into a playable form with [revorb][revorbc].

## PLAN
* [x] Extract weem files embedded in wisp files.  
* [ ] Convert weem files to playable ogg files.  
* [ ] Allow to revorb unplayable ogg files.  
* [ ] Find and extract all weem files embedded and referenced in bank files.  
* [ ] Video extraction? Maybe.  

## BUILD
I only have a linux system and do not have convenient access to a Windows system, so
for now there is only a Make build system; the plan is to eventually make this at least
cross-platform and cross-compilable between linux and Windows, maybe more in the future.

### Linux
To build, simply run `make` in the top level directory; there is `config.mk` for configuring
different build parameters and output directories.

[NME2]:https://github.com/TypeA2/NME2
[ww2ogg]:https://github.com/hcs64/ww2ogg
[revorbc]:https://github.com/bowtoes/revorbc
