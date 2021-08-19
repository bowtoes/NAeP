Going off of ww2ogg, a packed codebooks binary file:
* The number of codebooks stored in the file is:  
  `(file_size - offset_table_start) / 4`  
  where `offset_table_start` is stored as the last 4 bytes of the file
  interpreted as a little-endian integer.


**WHAT THE HELL ARE THE PACKED CODEBOOKS?!**  
I've seen no mention of anything similar across any documentation I've read, no
source code other than ww2oog; how were they generated? What is their source?
