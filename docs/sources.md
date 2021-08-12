# Sources
Primary references for the construction of this program.

* ## [aoTuV][aoTuV] ([Github][aoTuV github])
  A libvorbis encoder/decoder library with the aim of better quality encoding  
  [Original location][aoTuV original]
* ## [oggenc][oggenc]
  An encoder implementation with versions using aoTuV library.
* ## [EVE Online Forum Thread][eve online]
	The link is dead and no usable archives exist on the Wayback machine, no idea what was said in that thread.
* ## [XeNTaX Forum Thread][xentax forum]
	Seemingly, the progenination point of [ww2ogg][ww2ogg].
* ## [XeNTaX Wiki Page on SoundBanks][xentax wiki on bnk]
	A wiki page detailing the known structure of Wwise SoundBank files.

# Known
It seems to me that a SoundBank is just a specific version/modification of the [RIFF][mcgill riff] file format specification.
Each SoundBank stores a number of Sections (what would be called RIFF chunks), that are layed out similarly:
* 4    bytes - Chunk header (ckID aka fourcc)
* 4    bytes - Chunk size (ckSZ)
* ckSZ bytes - Chunk data (ckData)

SoundBank sections known to exist <sup>*citation needed*</sup> :
* [BKHD - Bank Header][bkhd]
* [DIDX - Data Index ][didx]
* [DATA - Bank Data  ][data]
* [ENVS - Unkown     ][envs]
* [FXPR - Uknown     ][fxpr]
* [HIRC - ???][hirc]  
  Probably the most complicated chunk; possibly **`Header Information Resource Chunk`** ?
* [STID - ???][stid]
* [STMG - ???][stmg]

[aoTuV]:https://ao-yumi.github.io/aotuv_web
[aoTuV github]:https://github.com/AO-Yumi/vorbis_aotuv
[aoTuV original]:http://www.geocities.jp/aoyoume/aotuv/index.html
[oggenc]:https://rarewares.org/ogg-oggenc.php#oggenc-aotuv
[eve online]:http://www.eveonline.com/ingameboard.asp?a=topic&threadID=1018956
[xentax forum]:https://forum.xentax.com/viewtopic.php?f=17&t=3477
[ww2ogg]:https://github.com/hcs64/ww2ogg
[xentax wiki on bnk]:https://wiki.xentax.com/index.php/Wwise_SoundBank_(*.bnk)
[mcgill riff]:http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/Docs/riffmci.pdf

[bkhd]:https://wiki.xentax.com/index.php/Wwise_SoundBank_(*.bnk)#BKHD_section
[didx]:https://wiki.xentax.com/index.php/Wwise_SoundBank_(*.bnk)#DIDX_section
[data]:https://wiki.xentax.com/index.php/Wwise_SoundBank_(*.bnk)#DATA_section
[envs]:https://wiki.xentax.com/index.php/Wwise_SoundBank_(*.bnk)#ENVS_section
[fxpr]:https://wiki.xentax.com/index.php/Wwise_SoundBank_(*.bnk)#FXPR_section
[hirc]:https://wiki.xentax.com/index.php/Wwise_SoundBank_(*.bnk)#HIRC_section
[stid]:https://wiki.xentax.com/index.php/Wwise_SoundBank_(*.bnk)#STID_section
[stmg]:https://wiki.xentax.com/index.php/Wwise_SoundBank_(*.bnk)#STMG_section
