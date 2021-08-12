# RIFF/WAVE `cue ` Chunk

## `cue ` Fields
| Name              | Type    | Size (bytes)                            | Description
| :---:             | :---:   | :---:                                   | :---:
| `ckID`            | `DWORD` | 4                                       | Chunk type identifier; always `cue `.
| `ckSize`          | `DWORD` | 4                                       | Chunk data size.
| `dwCuePoints`     | `DWORD` | 4                                       | Number of cue points in table.
| `tbCuePointTable` | Table   | `dwCuePoints * sizeof(struct cuePoint)` | Table of cue points.

## `struct cuePoint` Fields
| Name             | Type    | Size (bytes) | Description
| :---:            | :---:   | :---:        | :---:
| `dwName`         | `DWORD` | 4 | Unique name for the `cuePoint`.
| `dwPosition`     | `DWORD` | 4 | Ordering information? I don't know how to interpret the documentation.<br/>'Sample position of the cue point; the sequential sample number within the play order'
| `fccChunk`       | `DWORD` | 4 | `ckID` of the chunk that contains the cue point
| `dwChunkStart`   | `DWORD` | 4 | File position-offset of `fccChunk` chunk data that contains the cue point.<br/>Zero in Wwise?
| `dwBlockStart`   | `DWORD` | 4 | File position-offset of the block (?) that contains the cue point.<br/>Zero in Wwise?
| `dwSampleOffset` | `DWORD` | 4 | Sample offset of cue point relative to `dwBlockStart`.<br/>Wwise uses this.
