# Notes
## Structure of a WSP file
A WSP file (extension `wsp`) is a concatenation of multiple WEMs in a single
file. Each WEM is placed so that it starts at the next 2048-byte boundary from
the last; I suspect this is to make reading/extraction on the part of the
Audiokinetic engine a simpler implementation.

[countwsp][]

## Weird WEMS
Some WEMs seem to be formatted (chunk `fmt `) using a specific kind of wav format, or something
very like it. They have the extensible format code (`0xFFFE`), and the 'extra data'
section is 6 bytes long which seems invalid. After that, a `JUNK` chunk with
exactly four bytes of 0. Then it's that data chunk, and
the data chunk is very very strange; all of the files have what looks like `almost`
the same data repeating over and over again, evenly spaced. And between the files,
some of the data seems to be identical to, or a conjunction/modification of, other files'
data. Ww2ogg can't handle them, giving `Parse error: expected 0x42 fmt if vorb missing`. Maybe they're a single/multiple sounds encoded
across multiple files? I have no idea, more research is necessary.  
These are the WEMs in question:  
`BGM_0_000_04.wem`  
`BGM_0_000_05.wem`  
`BGM_0_000_06.wem`  
`BGM_0_000_07.wem`  
`BGM_0_000_09.wem`  
`BGM_0_000_10.wem`  
`BGM_0_000_11.wem`  
`BGM_0_000_12.wem`  
They all have the same `data` chunk size of `00 CA 08 00` (576,000)

There's also this one:  
`BGM_0_001_10.wem`  
It's data, along with a number of others, isn't formatted like the vast majority of WEMs, but
ww2ogg handles them fine so it must just be a different encoding.  

[McGill WAVE Specification][] for some reference.

[mcgill wave specification]:https://web.archive.org/web/20201228133457/http://www-mmsp.ece.mcgill.ca/documents/audioformats/wave/wave.html
[countwsp]:https://github.com/bowtoes/countwsp
