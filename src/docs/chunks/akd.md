# `akd ` Chunk
This is just a chunk storing Wwise project-specific metadata, irrelevant to decoding
and converting the actual audio data.

* Almost always present in Wwise WAVE/Vorbis  
   * NOT present in  
   `BGM_0_000_04.wem`  
   `BGM_0_000_05.wem`  
   `BGM_0_000_06.wem`  
   `BGM_0_000_07.wem`  
   `BGM_0_000_09.wem`  
   `BGM_0_000_10.wem`  
   `BGM_0_000_11.wem`  
   `BGM_0_000_12.wem`  

   Those ones are weird in their own way however. More noteworthy lack of presence in  
   `BGM_0_001_09.wem`  
   `BGM_0_007_*.wem`  
   `BGM_0_008_00.wem`  
   `BGM_0_008_01.wem`  
   `BGM_0_008_02.wem`  
   `BGM_0_008_03.wem`  
   `BGM_0_008_08.wem`  
   `BGM_0_008_09.wem`  
   `BGM_0_008_11.wem`  
   `BGM_0_008_12.wem`  
   `BGM_0_009_01.wem`  
   `BGM_0_009_03.wem`  
   `BGM_0_009_04.wem`  
   `BGM_0_009_06.wem`  
   `BGM_0_009_07.wem`  
   `BGM_0_009_09.wem`  
   `BGM_0_010_00.wem`  
   `BGM_0_010_01.wem`  
   `BGM_0_010_03.wem`  
   `BGM_0_010_04.wem`  
   `BGM_2_001_07.wem`  
   `BGM_2_001_08.wem`  

* Almost always at the same offset; always with same size of 16 bytes  
* First four bytes of data vary, but last twelve always the same `00 00 80 3F`|`00 00 00 00`|`00 00 00 00`  
   * Last of varying bytes seems to be a count of something, or an ID or type identifier. I've seen consecutive numbers `3E`, `3F`, and `40`.  
   * `40` seems rare, so instances are noted here:  
      * `BGM_0_000_21.wem`  
      * `BGM_0_001_30.wem`  
      * `BGM_0_001_31.wem`  
      * `BGM_0_008_07.wem`  
      * `BGM_1_000_3.wem`  
      * `BGM_2_000_03.wem`  
      * `BGM_3_000_00.wem`  
      * `BGM_3_000_03.wem`  
      * `BGM_3_001_1.wem`  
* Always preceded by `JUNK` chunk, with size of 2 bytes and `NULL` data. Alignment? But it isn't because the offset isn't always the same. Maybe the offset is always one of two values?  
