# WSP

After every embedded .wem file, there appears to be a large section of padding? bytes,
or something of the sort. All are zero and currently their purpose is unknown.

## Examples
### `BGM_0_000.wsp`
Every file is aligned to at least `WORD` (4-byte) boundaries, maybe `DWORD` (8-byte)
or more? boundaries.  

Here is a table of the different .wem files, their offsets, total size, and the padding
between them (**Pad Distance** is AFTER the file, not before)

|.wem RIFF|Start offset|Size   |End offset|Pad Distance
|:-:|:-:|:-:|:-:|:-:
|1        |0           |2726725|2726725   |1211
|2        |2727936     |3057393|5785329   |271
|3        |5785600     |1494801|7280401   |239
|4        |

In theory:
```c
startOffset = previousEndOffset + previousPadDistance;
endOffset = startOffset + size;
```

After the first RIFF file, there are `1211` bytes of padding.
There are `271` padding bytes between the second RIFF file and the 3rd.
