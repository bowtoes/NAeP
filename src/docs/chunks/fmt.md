# `fmt` Chunk
`WAVE` files have a `fmt ` chunk, used to detail various aspects of the audio
stored within. In Ogg/Vorbis, this information is stored in a different format
in a `vorb` header.  

In weems, the `fmt ` chunk contains many fields some are standard to the WAVE
specification, however many are custom. It also seems many fields are not
meaningfully standard across weems from different games, probably dependent on
Wwise version, so there are some bits and pieces of the chunk that remain unknown.  

See this [MGSV soundswap guide][mgsv soundswap],
[Topher Lee's page][topher lee pcm] and
[some weem format blueprints][wem format blueprints] as some examples of what I mean.

## `fmt` Chunk Layouts According to Win10 SDK (`mmreg.h`)

<table>
  <thead>
    <th>Field</th>
    <th>Type</th>
    <th>Description</th>
  </thead>
  <tbody>
    <th colspan=3>Universal Format Fields</th>
    <tr>
      <td><code>wFormatTag</code></td>
      <td><code>WORD</code></td>
      <td>Format type (see <a href="#woah">format types</a>)</td>
    </tr>
    <tr>
      <td><code>nChannels</code></td>
      <td><code>WORD</code></td>
      <td>Number of channels (i.e. 1=mono, 2=stereo, ...)</td>
    </tr>
    <tr>
      <td><code>nSamplesPerSec</code></td>
      <td><code>DWORD</code></td>
      <td>Sample rate</td>
    </tr>
    <tr>
      <td><code>nAvgBytesPerSec</code></td>
      <td><code>DWORD</code></td>
      <td>Average byte-rate of file (used 'for buffer estimation'?)</td>
    </tr>
    <tr>
      <td><code>nBlockAlign</code></td>
      <td><code>WORD</code></td>
      <td>Block size of data (whatever that means)</td>
    </tr>
    <th colspan=3>PCM Format Extras</th>
    <tr>
      <td><code>wBitsPerSample</code></td>
      <td><code>WORD</code></td>
      <td>Not </td>
    </tr>
  </tbody>
</table>

[mgsv soundswap]:https://bobdoleowndu.github.io/mgsv/documentation/soundswapping.html
[wem format blueprints]:https://github.com/rickvg/Wwise-audiobanks-wem-format-blueprints/blob/master/WEM-File%20Template.bt
[topher lee pcm]:http://www.topherlee.com/software/pcm-tut-wavformat.html
[mcgill wave]:https://web.archive.org/web/20201228133457/http://www-mmsp.ece.mcgill.ca/documents/audioformats/wave/wave.html
[anders bergh old]:https://web.archive.org/web/20200621130653/https://bitbucket.org/anders/wwiseconv/wiki/WWise_format
[anders bergh new]:https://web.archive.org/web/20200621130652/https://bitbucket.org/anders/wwiseconv/wiki/New_WWise_format
