# `fmt` Chunk
`WAVE` files have a `fmt ` chunk, used to detail various aspects of the audio
stored within. In Ogg/Vorbis, some of this information is stored in a different
format in the identification & setup `vorbis` headers.  

In WEMs, the `fmt ` chunk contains many fields; some are standard to the WAVE
specification, however many are custom. It also seems many fields are not
meaningfully standard across WEMs from different games, likely dependent on
Wwise version, so there are some bits and pieces of the chunk that are inconsistent.  

See this [MGSV soundswap guide][mgsv soundswap],
[Topher Lee's page][topher lee pcm], and
[some WEM format blueprints][wem format blueprints] as some examples of what I mean
by _inconsistency_.

## Table of Contents
1. [Standard `fmt ` Chunk Layouts](#standard-fmt--chunk-layouts)
   1. [Common `fmt ` chunk structure (c-tag `WAVEFORMAT`)](#common-fmt--chunk-structure-tag-waveformat)
   2. [PCM Extra format (c-tag `PCMWAVEFORMAT`)](#pcm-extra-format-tag-pcmwaveformat)
   2. [Standard Extended Format Structure (c-tag `tWAVEFORMATEX`, `WAVEFORMATEX`)](#standard-extended-format-structure-tag-twaveformatex-waveformatex)
   2. [New Extended Format Structure (c-tag `WAVEFORMATEXTENSIBLE`)](#new-extended-format-structure-tag-waveformatextensible)
2. [Wwise-specific Format Structure(s)](#wwise-specific-format-structures)
   1. [Fields common to all formats](#fields-common-to-all-formats)
   1. [Format A, size `0x12` bytes](#format-a)
   2. [Format B, size `0x18` bytes](#format-b)
   3. [Format C, size `0x28` bytes](#format-c)
   4. [Format D, size `0x42` bytes](#format-d)
3. [Format Tags](#format-tags)
4. [References](#references)

## Standard `fmt ` Chunk Structure
These structure layouts are taken from the [Windows 10 SDK][win10 sdk] file `mmreg.h`.

* #### Common `fmt ` chunk structure (tag `WAVEFORMAT`)
  |Field            |Type   |Size (bytes)|Description                                |
  |:----            |:----  |:---:       |:----                                      |
  |`wFormatTag`     |`WORD` |2           |[Format tag](#format-tags)                 |
  |`nChannels`      |`WORD` |2           |Number of channels (i.e. mono, stereo, ...)|
  |`nSamplesPerSec` |`DWORD`|4           |Sample rate of the audio                   |
  |`nAvgBytesPerSec`|`DWORD`|4           |For buffer estimation (whatever that means)|
  |`nBlockAlign`    |`WORD` |2           |Block size of data (wdtm)                  |

  **Total Size:** 14 `0x0E` bytes  
* #### PCM Extra format (tag `PCMWAVEFORMAT`)
  |Field           |Type        |Size (bytes)|Description                                  |
  |:----           |:----       |:---:       |:----                                        |
  |`wf`            |`WAVEFORMAT`|14          |Inherits all fields of standard `fmt ` chunk.|
  |`wBitsPerSample`|`WORD`      |2           |Bits used per PCM audio sample               |

  **Total Size:** 16 `0x10` bytes  
* #### Standard extended format structure (tag `tWAVEFORMATEX`, `WAVEFORMATEX`)
  |Field            |Type   |Size (bytes)|Description                                                    |
  |:----            |:----  |:---:       |:----                                                          |
  |`wFormatTag`     |`WORD` |2           |[Format tag](#format-tags)                                     |
  |`nChannels`      |`WORD` |2           |Number of channels (i.e. mono, stereo, ...)                    |
  |`nSamplesPerSec` |`DWORD`|4           |Sample rate of the audio                                       |
  |`nAvgBytesPerSec`|`DWORD`|4           |For buffer estimation (whatever that means)                    |
  |`nBlockAlign`    |`WORD` |2           |Block size of data (wdtm)                                      |
  |`wBitsPerSample` |`WORD` |2           |Bits-per-sample of mono data                                   |
  |`cbSize`         |`WORD` |2           |Size of the extra format information, starting after this field|

  **Total Size:**  18 `0x12` bytes  
* #### New extended format structure (tag `WAVEFORMATEXTENSIBLE`)
  |Field                  |Type                          |Size (bytes)|Description                             |
  |:----                  |:----                         |:---:       |:----                                   |
  |`Format`               |`WAVEFORMATEX`                |18          |Standard extended information           |
  |`Samples`              |<code><b>union</b> WORD</code>|2           |Union of the following fields:          |
  |`->wValidBitsPerSample`|`WORD`                        |_0_         |Bits of precision per audio sample.     |
  |`->wSamplesPerBlock`   |`WORD`                        |_0_         |Valid if `Format.wBitsPerSample` is 0   |
  |`->wReserved`          |`WORD`                        |_0_         |Reserved                                |
  |`dwChannelMask`        |`DWORD`                       |4           |Which channels are present in the stream|
  |`SubFormat`            |`GUID`                        |16          |TBW                                     |

  **Total Size:** 40 `0x28` bytes  
  **Size without `GUID`:** 24 `0x18` bytes  

## Wwise-specific format structure(s)
These fields I took from reading the source code of [ww2ogg][ww2ogg gh] and
piecing together how things were structured from there how they decoded the
data.

* #### Fields common to all formats
  |Field            |Type   |Size (bytes)|Description                                                                            |
  |:----            |:----  |:---:       |:----                                                                                  |
  |`wFormatTag`     |`WORD` |2           |[Format tag](#format-tags), same as standard formats (one of `0xFFFE`/`0xFFFF`)        |
  |`nChannels`      |`WORD` |2           |Number of channels, same as standard formats                                           |
  |`nSamplesPerSec` |`DWORD`|4           |Sample rate of the audio, same as standard formats                                     |
  |`nAvgBytesPerSec`|`DWORD`|4           |Average byte rate of the audio, same as standard formats                               |
  |`nBlockAlign`    |`WORD` |2           |Block size of data, same as standard formats (expected 0)                              |
  |`wBitsPerSample` |`WORD` |2           |Bits-per-sample/second of mono, same as standard formats (expected 0)                  |
  |`cbSize`         |`WORD` |2           |Size of extra format information, same as standard formats (expected to be `size - 18`)|

* #### Format A
  **Total Size:** 18 `0x12` bytes  
  **Expected `cbSize`:** 0 `0x00` bytes  
* #### Format B
  |Field|Type |Size (bytes)|Description|
  |:----|:----|:---:       |:----      |
  |`Samples`      |<code><b>union</b> WORD</code>|2 |Same as in the extended format structure|
  |`dwChannelMask`|`DWORD`                       |4 |Same as in the extended format structure|

  **Total Size:** 24 `0x18` bytes  
  **Expected `cbSize`:** 6 `0x06` bytes  
* #### Format C
  |Field|Type |Size (bytes)|Description|
  |:----|:----|:---:       |:----      |
  |`Samples`      |<code><b>union</b> WORD</code>|2 |Same as in the extended format structure|
  |`dwChannelMask`|`DWORD`                       |4 |Same as in the extended format structure|
  |`SubFormat`    |`GUID`                        |16|Same as in the extended format structure|

  **Total Size:** 40 `0x28` bytes  
  **Expected `cbSize`:** 22 `0x16` bytes  
* #### Format D
  |Field          |Type                          |Size (bytes)|Description                                              |
  |:----          |:----                         |:---:       |:----                                                    |
  |`Samples`      |<code><b>union</b> WORD</code>|2           |Same as in the extended format structure                 |
  |`dwChannelMask`|`DWORD`                       |4           |Same as in the extended format structure                 |
  |`Vorbis`       |`vorb`                        |42          |Vorbis specific information; normally in a separate chunk|

  **Total Size:** 66 `0x42` bytes  
  **Expected `cbSize`:** 48 `0x30` bytes  

## Format Tags
The format type for a given WAVE file is stored in the `wFormatTag` field of
the `fmt ` structure.  
Notable format types:

|Name                     |Value   |Description                                                                                                          |
|:----                    |:---:   |:----                                                                                                                |
|`WAVE_FORMAT_EXTENSIBLE` |`0xFFFE`|Format used for an extended WAVE format not registered with Microsoft; the `GUID` field is the unique type identifier|
|`WAVE_FORMAT_DEVELOPMENT`|`0xFFFF`|Indicates the format is unofficial, or is in its development stages and to be registered with Microsoft              |

## References
* [ww2ogg Github][ww2ogg gh]
* [Windows 10 SDK][win10 sdk]
* [MGSV Soundswap guide][mgsv soundswap]
* [Wem Format Blueprints][wem format blueprints]
* [Topher Lee's guide on PCM WAVE][topher lee pcm]

[ww2ogg gh]:https://github.com/hcs64/ww2ogg
[win10 sdk]:https://developer.microsoft.com/en-US/windows/downloads/windows-10-sdk/
[mgsv soundswap]:https://bobdoleowndu.github.io/mgsv/documentation/soundswapping.html
[wem format blueprints]:https://github.com/rickvg/Wwise-audiobanks-wem-format-blueprints/blob/master/WEM-File%20Template.bt
[topher lee pcm]:http://www.topherlee.com/software/pcm-tut-wavformat.html
[mcgill wave]:https://web.archive.org/web/20201228133457/http://www-mmsp.ece.mcgill.ca/documents/audioformats/wave/wave.html
[anders bergh old]:https://web.archive.org/web/20200621130653/https://bitbucket.org/anders/wwiseconv/wiki/WWise_format
[anders bergh new]:https://web.archive.org/web/20200621130652/https://bitbucket.org/anders/wwiseconv/wiki/New_WWise_format
