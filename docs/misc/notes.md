# Notes
## Structure of a WSP file
A WSP file (extension `wsp`) is a concatenation of multiple WEMs in a single
file. Each WEM is placed so that it starts at the next 2048-byte boundary from
the last; I suspect this is to make reading/extraction on the part of the
Audiokinetic engine a simpler implementation.

[countwsp][]

## Weird WEMS
Some WEMs are unlike most others in NieR's files, being 16-bit PCM data. These
are the files:
`BGM_0_000_04.wem`  
`BGM_0_000_05.wem`  
`BGM_0_000_06.wem`  
`BGM_0_000_07.wem`  
`BGM_0_000_09.wem`  
`BGM_0_000_10.wem`  
`BGM_0_000_11.wem`  
`BGM_0_000_12.wem`  
They are all the same audio; a simple sine wave (but in a full 6 channels!); I
have no idea why they're here.

There's also this one:  
`BGM_0_001_10.wem`  
It's data, along with a number of others, isn't formatted like the vast majority of WEMs, but
ww2ogg handles them fine so it must just be a different encoding.  

[McGill WAVE Specification][] for some reference.

[mcgill wave specification]:https://web.archive.org/web/20201228133457/http://www-mmsp.ece.mcgill.ca/documents/audioformats/wave/wave.html
[countwsp]:https://github.com/bowtoes/countwsp
