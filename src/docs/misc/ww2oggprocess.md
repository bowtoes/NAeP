Things in **BOLD** are things I should really look into what the hell ww2ogg
means by that, or give some proper documentation of what they are/represent.
# ww2ogg `Wwise_RIFF_Vorbis` class:
## Members:

| Field                     | Type         | Description
| :---:                     | :---:        | :---:
|                           |              | _**Command Line Options**_
| `file_name`               | `string`     | Input weem filepath.
| `codebooks_name`          | `string`     | Filepath to **packed codebooks**.
|                           |              | _**File Information**_
| `file_size`               | `long`       | Total size of input file, read by seeking to `eof`.
| `riff_size`               | `long`       | Size of `RIFF` chunk, read from file + 8 bytes.
|                           |              | _**Chunk Data Offsets and Sizes**_
| `fmt_offset`/`fmt_size`   | `long`       | **`fmt `** chunk.
| `cue_offset`/`cue_size`   | `long`       | **`cue `** chunk.
| `LIST_offset`/`LIST_size` | `long`       | **`LIST`** chunk.
| `smpl_offset`/`smpl_size` | `long`       | **`smpl`** chunk.
| `vorb_offset`/`vorb_size` | `long`       | **`vorb`** chunk.
| `data_offset`/`data_size` | `long`       | **`data`** chunk.
|                           |              | _**`fmt ` Chunk Info**_
| `channels`                | `uint16_t`   | Wwise (WAVE) audio channel count.
| `sample_rate`             | `uint32_t`   | Wwise (WAVE) audio sample rate
| `avg_bytes_per_second`    | `uint32_t`   | Average data-rate in bytes
| `ext_unk`                 | `uint16_t`   | Wwise (WAVE) format tag, always set to `0xFFFF` or `0xFEFF` for experimental/extensible.<br/>Unused by ww2ogg.
| `subtype`                 | `uint32_t`   | Not sure yet, something Vorbis-specific.
|                           |              | _**`cue ` Chunk Info**_
| `cue_count`               | `uint32_t`   | _To Be Written_.
|                           |              | _**`smpl` Chunk Info**_
| `loop_count`              | `uint32_t`   | _To Be Written_.
| `loop_start`              | `uint32_t`   | _To Be Written_.
| `loop_end`                | `uint32_t`   | _To Be Written_.
|                           |              | _**`vorb` Chunk Info**_
| `sample_count`            | `uint32_t`   | _To Be Written_.
| `setup_packet_offset`     | `uint32_t`   | _To Be Written_.
| `first_audio_packet`      | `uint32_t`   | _To Be Written_.
| `uid`                     | `uint32_t`   | _To Be Written_.
| `blocksize_0_pow`         | `uint8_t`    | _To Be Written_.
| `blocksize_1_pow`         | `uint8_t`    | _To Be Written_.
|                           |              | _**Encode/Decode specific Information**_
| `inline_codebooks`        | `const bool` | _To Be Written_.
| `full_setup`              | `const bool` | _To Be Written_.
| `header_triad_present`    | `bool`       | _To Be Written_.
| `old_packet_headers`      | `bool`       | _To Be Written_.
| `no_granule`              | `bool`       | _To Be Written_.
| `mod_packets`             | `bool`       | _To Be Written_.
| `little_endian`           | `bool`       | Whether Wwise RIFF input format is little- or big-endian.

# 0.
Constructor, given input file name, **packed codebooks** filepath, whether to do
**inline codebooks**, and whether **full setup**
