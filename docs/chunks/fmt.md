# `fmt` Chunk
`WAVE` files have a `fmt ` chunk, used to detail various aspects of the audio
stored within. In Ogg/Vorbis, this information is stored in a different format
in the identification & setup `vorbis` headers.  

In WEMs, the `fmt ` chunk contains many fields; some are standard to the WAVE
specification, however many are custom. It also seems many fields are not
meaningfully standard across WEMs from different games, probably dependent on
Wwise version, so there are some bits and pieces of the chunk that remain unknown.  

See this [MGSV soundswap guide][mgsv soundswap],
[Topher Lee's page][topher lee pcm], and
[some WEM format blueprints][wem format blueprints] as some examples of what I mean.

## `fmt` Chunk Layouts According to Win10 SDK (`mmreg.h`)

* #### Common `fmt ` chunk structure (`waveformat_tag` `WAVEFORMAT`)
  |Field            |Type   |Size (bytes)|Description                                               |
  |:----            |:----  |:---:       |:----                                                     |
  |`wFormatTag`     |`WORD` |2           |Format type (see <a href="#format-types">format types</a>)|
  |`nChannels`      |`WORD` |2           |Number of channels (i.e. mono, stereo, ...)               |
  |`nSamplesPerSec` |`DWORD`|4           |Sample rate of the audio                                  |
  |`nAvgBytesPerSec`|`DWORD`|4           |For buffer estimation (whatever that means)               |
  |`nBlockAlign`    |`WORD` |2           |Block size of data (wtm)                                  |

  **Total Size:** 14 `0x0E` bytes
* #### PCM Extra format (`pcmwaveformat_tag` `PCMWAVEFORMAT`)
  |Field           |Type        |Size (bytes)|Description                                  |
  |:----           |:----       |:---:       |:----                                        |
  |`wf`            |`WAVEFORMAT`|14          |Inherits all fields of standard `fmt ` chunk.|
  |`wBitsPerSample`|`WORD`      |2           |Bits used per PCM audio sample               |

  **Total Size:** 16 `0x10` bytes
* #### Standard extended format structure (`tWAVEFORMATEX` `WAVEFORMATEX`)
  |Field            |Type   |Size (bytes)|Description                                                    |
  |:----            |:----  |:---:       |:----                                                          |
  |`wFormatTag`     |`WORD` |2           |Format type (see <a href="#format-types">format types</a>)     |
  |`nChannels`      |`WORD` |2           |Number of channels (i.e. mono, stereo, ...)                    |
  |`nSamplesPerSec` |`DWORD`|4           |Sample rate of the audio                                       |
  |`nAvgBytesPerSec`|`DWORD`|4           |For buffer estimation (whatever that means)                    |
  |`nBlockAlign`    |`WORD` |2           |Block size of data (wtm)                                       |
  |`wBitsPerSample` |`WORD` |2           |Bits-per-sample of mono data                                   |
  |`cbSize`         |`WORD` |2           |Size of the extra format information, starting after this field|

  **Total Size:**  18 `0x12` bytes
* #### New extended format structure (`WAVEFORMATEXTENSIBLE`)
  |Field                 |Type                          |Size (bytes)|Description                             |
  |:----                 |:----                         |:---:       |:----                                   |
  |`Format`              |`WAVEFORMATEX`                |18          |Standard extended information           |
  |`Samples`             |<code><b>union</b> WORD</code>|2           |Union of the following fields:          |
  |`.wValidBitsPerSample`|`WORD`                        |2           |Bits of precision per audio sample.     |
  |`.wSamplesPerBlock`   |`WORD`                        |2           |Valid if `Format.wBitsPerSample` is 0   |
  |`.wReserved`          |`WORD`                        |2           |Reserved                                |
  |`dwChannelMask`       |`DWORD`                       |4           |Which channels are present in the stream|
  |`SubFormat`           |`GUID`                        |16          |TBD                                     |

  **Total Size:** 40 `0x28` bytes
  **Size without `GUID`:** 24 `0x18` bytes

## Format Types
The format type for a given WAVE file is stored in the `wFormatTag` field of
the `fmt ` structure.  
Notable format types:

|Name                    |Value   |Description|
|:---                    |:---:   |:---       |
|`WAVE_FORMAT_EXTENSIBLE`|`0xFFFE`|Format used for some extended WAVE format not registered with Microsoft; the `GUID` field is the unique type identifier|
|`WAVE_FORMAT_DEVELOPMENT`|`0xFFFF`|Indicates the format is unofficial, or is in its development stages, to be registered with Microsoft|

[mgsv soundswap]:https://bobdoleowndu.github.io/mgsv/documentation/soundswapping.html
[wem format blueprints]:https://github.com/rickvg/Wwise-audiobanks-wem-format-blueprints/blob/master/WEM-File%20Template.bt
[topher lee pcm]:http://www.topherlee.com/software/pcm-tut-wavformat.html
[mcgill wave]:https://web.archive.org/web/20201228133457/http://www-mmsp.ece.mcgill.ca/documents/audioformats/wave/wave.html
[anders bergh old]:https://web.archive.org/web/20200621130653/https://bitbucket.org/anders/wwiseconv/wiki/WWise_format
[anders bergh new]:https://web.archive.org/web/20200621130652/https://bitbucket.org/anders/wwiseconv/wiki/New_WWise_format
