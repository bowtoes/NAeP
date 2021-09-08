# 0. Table of Contents
0. [Table of Contents](#0-table-of-contents)
1. [WEM](#1-wem)
2. [The RIFF file format](#2-the-riff-file-format)
   1. [Generic Chunk Layout](#2i-generic-chunk-layout)
   2. [RIFF/LIST Chunk Layout](#2ii-rifflist-chunk-layout)
3. [Structure of a NieR:Automata .wem file](#3-structure-of-a-nier-automata-wem-file)
   1. [How the Tables are Layed Out](#3i-how-the-tables-are-layed-out)
   2. [.wem File Structure](#3ii-wem-file-structure)
4. [Sources Referenced](#4-sources-referenced)
   1. [RIFF/WAVE Documentation](#4i-riffwave-documentation)
   1. [WEM References](#4ii-wem-references)

# 1. WEM
My attempt at documenting the .wem audio storage format used specifically for NieR:Automata.

**Note**: The .wem format and it's containing types (.bnk, .wsp) do not have official, public
specifications, so the understanding of their structure and layout is incomplete.
There is enough known to be able to use and extract them, however many details remain unclear.  

It also seems to be the case, from my investigation, that there are multiple versions
of the .wem format. Whether this difference in layout is on a per-game basis or
a different version of the tools specifically remains unknown to me.

# 2. The RIFF file format
A RIFF file is a container of seperate 'chunks', each of which contain certain data
called fields. These fields are `ckId` (chunk ID), `ckSz` (chunk size), and `ckData` (chunk data).
* `ckId` - The chunk ID is a four-character-code (fourcc) that specifies what type of chunk follows.
* `ckSz` - The chunk size is the size in bytes of `ckData`, the chunk data.
* `ckData` - The chunk data: All of the actual data the chunk stores.

Depending on the type of chunk (`ckId`), there may also be another specific field
at the beginning of `ckData` which stores the `formType` for the chunk. Chunks
that have this field are the `RIFF` and `LIST` chunks; chunks that have a `formType`
field can store subchunks.

Each file formatted using RIFF starts with a `RIFF` chunk. This chunk stores all the data in the file,
in the form of subchunks. The format of `ckData` in each subchunk depends on the kind of chunk
that it is. See [this page][msdn avi] or [this one][john loomis riff] for some more detailed
documentation of the file format. Custom chunk types can be defined by simply creating a new
fourcc. It should be noted, that if `ckSz` is odd, then a pad byte will be inserted/should
be present at the end of `ckData`. The pad byte is not counted in `ckSz`; this is true for
all chunks. However, if a chunk has subchunks, then that chunk's `ckSz` WILL count the pad
byte in each of it's subchunks that have it.

## 2i. Generic Chunk Layout
How any chunk in a given RIFF file will be formatted.

|Offset|Type|Size|Sample|Description|
|:-:|:-:|:-:|:-:|-|
|`0x00`|`ckId`  |`4`   |`fmt `<br>`0x666D7420`|Four-character-code (fourcc), representing the type of chunk this is.
|`0x04`|`ckSz`  |`4`   |66                    |Size of the chunk, excluding `ckId` and `ckSz` (8 bytes).
|`0x08`|`ckData`|`ckSz`|-                     |All of the data belonging to the chunk, `ckSz` bytes long.
## 2ii. RIFF/LIST Chunk Layout
The universal layout of the RIFF/LIST chunks specifically.

|Offset|Type|Size|Sample|Description|
|:-:|:-:|:-:|:-:|-|
|`0x00`|`ckId`    |`4`   |`RIFF`<br>`0x52494646`|Chunk fourcc
|`0x04`|`ckSz`    |`4`   |277574                |Size of file, excluding the first 8 bytes (`ckId` and `ckSz`).
|`0x08`|`formType`|`4`   |`WAVE`<br>`0x57415645`|Formtype of chunk, also part of `ckData`.
|`0x08`|`ckData`  |`ckSz`|-                     |Data of chunk, storing subchunks. `formType` is at the start of this data.

In order to get the byte after the last byte of the chunk as an offset from the
start of the file (that offset being called the 'end offset'), take the offset for the
start of the chunk (the start offset), add `ckSz`, then add 8 bytes for `ckId` and `ckSz`.
The result is the end offset, the index to the first byte AFTER the chunk. The byte
immediately preceding that one is the LAST byte of `ckData`.

_**NOTE**_: Despite the research I've done and all documentation I've found saying that `ckSz` does NOT count
the pad byte, going in and looking at RIFF files in a hex editor reveals that they DO count the pad byte. Or else
I'm just having a very hard time finding any examples where the chunk data does NOT end in a `0x00` byte.
**NOTE** also, that it seems the .wem files may not include a pad byte period; given the documentation and all
things considered, `RIFF` chunks should universally have an even length, yet they don't always in these .wem files.
Other places I've looked (standard .wav files, .ani cursor files) universally have an even length.

# 3. Structure of a NieR:Automata .wem File
Generally, a .wem file is a file 'conforming' to the [RIFF][mcgill riff] file format standard.

## 3i. How the Tables are Layed Out
These tables are an attempt by me to assemble what I've been able to find out about
the structure of the .wem format into one location.

|Table Header|Description|
|---|---|
|**Offset**   |Byte offset of the field from beginning of file (which starts at 0).
|**Type**     |The type of the field; also how I will refer to the field. Certain specifications/sources may call them differently.
|**Size**     |Size of field in bytes.
|**Sample**   |An example of a valid value for the given field, taken from a specific source file ().
|**&equest;** |Whether the field is the same for all valid .wem files ('**C**', constant) or not ('**V**', varying). There may be some endianness specificities that are still valid, need to work that out. For now, assume little-endian).
|**??**|How certain I am that the field is what the documentations say, compared to with what I'm seeing in some example files. '**1**' the documentation agrees, '**-**' unsure, '**0**' the documentation disagrees.
## 3ii. .wem File Structure
|Offset|Type|Size|Sample|&equest;|??|Description|
|:-:|:-:|:-:|:-:|:-:|:-:|-|
|`0x00`|`ckId`    |`4`  |`RIFF`<br>`0x52494646`|**C**|**1**|RIFF chunk fourcc
|`0x04`|`ckSz`    |`4`  |277574                |**V**|**1**|Size of file, excluding the first 8 bytes (`ckId` and `ckSz`).
|`0x08`|`formType`|`4`  |`WAVE`<br>`0x57415645`|**C**|**1**|Formtype fourcc signalling that this is a WAVE format file.
|`0x0C`|`ckId`    |`4`  |`fmt `<br>`0x666D7420`|**C**|**1**|Fourcc of the format chunk, detailing how the audio is stored. See [here][fmt chunk] for details.
|`0x10`|`ckSz`    |`4`  |66                    |**-**|**1**|Size of the format chunk.
|`0x14`|`audioFmt`|`2`  |`0xFFFF`              |**-**|**1**|Audio format, `0xFFFF` means experimental.
|`0x16`|`channels`|`2`  |6                     |**V**|**1**|Number of audio channels.
|`0x18`|`samples` |`4`  |48000                 |**V**|**1**|Sample rate of audio.
|`0x1C`|`avgBrate`|`4`  |44865                 |**V**|**-**|Average byte data rate<br>`channels * sample rate * bitsPerSample/8`. Seems smaller than it should be
|`0x20`|`blkAlign`|`2`  |0                     |**-**|**0**|'Block align'; I have no idea what this means.
|`0x20`|`blkAlign`|`2`  |0                     |**-**|**0**|Bits per sample, 0 in wem files?

The samples in this table are taken from a specific file in the NieR:Automata game directory:  
* `data/sound/stream/5731267.wem`

# 4. Sources referenced 
## 4i. RIFF/WAVE Documentation
[McGill University RIFF technical document][mcgill riff]  
[MSDN AVI file specification][msdn avi]  
[John Loomis' short description of the RIFF format][john loomis riff]  
[Topher Lee's slightly more in-depth description of RIFF][topher lee pcm]  

## 4ii. WEM references
[MGSV Soundswapping guide][mgsv soundswap]

[mgsv soundswap]:https://bobdoleowndu.github.io/mgsv/documentation/soundswapping.html
[topher lee pcm]:http://www.topherlee.com/software/pcm-tut-wavformat.html
[mcgill riff]:http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/Docs/riffmci.pdf
[msdn avi]:https://docs.microsoft.com/en-us/windows/win32/directshow/avi-riff-file-reference#riff-file-format
[john loomis riff]:https://johnloomis.org/cpe102/asgn/asgn1/riff.html

[fmt chunk]:chunks.md#fmt-chunk
