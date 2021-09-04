# Packed Codebooks

Wwise RIFF/Vorbis files, like standard Ogg/Vorbis files, use a collection of
what's called codebooks to store various encoding information. In standard
Vorbis, these codebooks are stored directly in the file's headers, however in
specific version of the Wwise Audiokinetic engine, they may be stored and
referenced externally.  

When processing files, [ww2ogg][ww2ogg] makes use of two different external
codebook files `packed_codebooks` and `packed_codebooks_aoTuV`.  
How these codebooks were generated or what their source is, I have no clue; my
assumption is that they are distributed with the Wwise Audiokinetic engine.

## Structure
The structure of the packed codebooks provided by [ww2ogg][ww2ogg] is simple
enough; starting at offset 0 of a given file, the data is laid out like so:

* Table of Codebooks  
  The data of each codebook is stored directly, one after another.
* Table of Codebook Offsets  
  After all the codebook data is a table of codebook offsets; each set of 4
  bytes specifies, in order, the ending byte of its codebook.

So for instance, the last 4 bytes of a given file specify the ending offset of
the last codebook in the file; this can be used to determine how many codebooks
are stored in the file and how many offsets to read.

[ww2ogg]:https://github.com/hcs64/ww2ogg
