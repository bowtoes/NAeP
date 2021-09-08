[Up](.)

# `vorb` Chunk
Wwise RIFF/Vorbis files store their Vorbis data in a `vorb` chunk, which may
be a separate chunk or depending on Wwise version, stored in the extra space
of the `fmt` chunk.

## Table of Contents
1. [`vorb` Chunk Data Structure(s)](#vorb-chunk-data-structure)
0. [References](#references)

## `vorb` Chunk Data Structure(s)
These fields I took from reading the source code of [ww2ogg][ww2ogg gh] and
piecing things together from how they decoded the data.

![img](vorbchunk.png)

|Field                 |Bytes|Description|
|:---                  |:---:|:---       |
|`sample_count`        |4    |Self-explanatory.|
|`mod_signal`          |4    |Mostly unknown, ww2ogg tests it to decide how to read the audio packets.|
|`header_packet_offset`|4    |The offset into the `data` chunk, of the first Vorbis header packet.|
|`audio_packets_offset`|4    |The offset into the `data` chunk, of the first Vorbis audio packet.|
|`uid`                 |4    |Unknown, ww2ogg doesn't use the value.|
|`blocksize_0_bits`    |1    |Same as in a normal Vorbis identification header, except in a whole byte.|
|`blocksize_1_bits`    |1    |Same as in a normal Vorbis identification header, except in a whole byte.|
|`skipped`             |-    |Skipped data.|
|`unknown`             |-    |Unknown/unused data.|

If the `vorb` chunk is `Type A`:  
* The Wwise [RIFF/Vorbis packet][riffvorbpacket] headers will be `Type A`.  

Else if the `vorb` chunk is `Type B` or `Type C`:
* The Wwise [RIFF/Vorbis packet][riffvorbpacket] headers will be `Type C`.  

Else if the `vorb` chunk is `Type D` or `Type E`:  
* The Wwise [RIFF/Vorbis packet][riffvorbpacket] headers will be `Type B`.  


## References
* [ww2ogg Github][ww2ogg gh]

[riffvorbpacket]:../riffvorbispacket.md
[ww2ogg gh]:https://github.com/hcs64/ww2ogg
