# Notes
## Structure of a WISP file
Simply put, a WISP file (extension `.wsp`) is just a concatenation on multiple
weem files (extension `.wem`) into a single file; the only caveat is that there
is some padding between the individual weems. This padding length is exactly
enough so that the next weem begins at the next multiple of 2048 bytes.  
So if one weem ends at byte 4000, then there will be 95 bytes of padding so that
the next weem begins at byte 4096 (2 * 2048). Or there would be 150 padding bytes, if
the first weem ended at byte 5993, so that the next weem would begin at the 6144th
byte (3 * 2048), etc.

[countwsp][]

## Weird WEEMS
Some weems seem to be formatted (chunk `fmt `) using a specific kind of wav format, or something
very like it. They have the extensible format code (`0xFFFE`), and the 'extra data'
section is 6 bytes long (which seems invalid) and after that, a JUNK chunk with
exactly four bytes of data which is all zeroes. Then it's that data chunk, and
the data chunk is very very strange; all of the files have what looks like `almost`
the same data repeating over and over again, evenly spaced. And between the files,
some of the data seems to be identical to, or a conjunction/modification of other files'
data. ww2ogg can't handle them. They seem to be maybe the same data strangely
encoded across multiple files? I have no idea.
These are the weems:  
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
It's data (along with others) isn't formatted like the vast majority of weems, but
ww2ogg handles them fine so it must just be a different compression.  

[McGill WAVE Specification][] for some reference.

[mcgill wave specification]:https://web.archive.org/web/20201228133457/http://www-mmsp.ece.mcgill.ca/documents/audioformats/wave/wave.html
[countwsp]:https://github.com/bowtoes/countwsp
