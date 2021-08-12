# Common Windows Type Definitions
These definitions are taken from `shared/intsafe.h` from the Windows 10 SDK.  

<table align=center>
  <thead>
    <th>Alias</th>
    <th>Type</th>
    <th>Size (bytes)</th>
    <th>Architecture</th>
    <th>Alias Method</th>
  </thead>
  <tbody align=center>
    <th colspan=5>Standard Types</th>
    <tr><td><code>     CHAR</code></td><td><code>            char</code></td><td>1</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>    SHORT</code></td><td><code>           short</code></td><td>2</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>      INT</code></td><td><code>             int</code></td><td>4</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>     LONG</code></td><td><code>            long</code></td><td>4</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code> LONGLONG</code></td><td><code>         __int64</code></td><td>8</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <th colspan=5>Unsigned Types</th>
    <tr><td><code>    UCHAR</code></td><td><code>   unsigned char</code></td><td>1</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>   USHORT</code></td><td><code>  unsigned short</code></td><td>2</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>     UINT</code></td><td><code>    unsigned int</code></td><td>4</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>    ULONG</code></td><td><code>   unsigned long</code></td><td>4</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>ULONGLONG</code></td><td><code>unsigned __int64</code></td><td>8</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <th colspan=5>Bit-width Types</th>
    <tr><td><code>     INT8</code></td><td><code>     signed char</code></td><td>1</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>    INT16</code></td><td><code>    signed short</code></td><td>2</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>    INT32</code></td><td><code>      signed int</code></td><td>4</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>    INT64</code></td><td><code>  signed __int64</code></td><td>8</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <th colspan=5>Bit-width Unsigned Types</th>
    <tr><td><code>    UINT8</code></td><td><code>   unsigned char</code></td><td>1</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>   UINT16</code></td><td><code>  unsigned short</code></td><td>2</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>   UINT32</code></td><td><code>    unsigned int</code></td><td>4</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>   UINT64</code></td><td><code>unsigned __int64</code></td><td>8</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <th colspan=5>Pointer Types</th>
    <tr><td><code>  INT_PTR</code></td><td><code>             int</code></td><td>4</td><td>x86</td><td><code>typedef</code></td></tr>
    <tr><td><code>  INT_PTR</code></td><td><code>         __int64</code></td><td>8</td><td>x64</td><td><code>typedef</code></td></tr>
    <tr><td><code> UINT_PTR</code></td><td><code>unsigned     int</code></td><td>4</td><td>x86</td><td><code>typedef</code></td></tr>
    <tr><td><code> UINT_PTR</code></td><td><code>unsigned __int64</code></td><td>8</td><td>x64</td><td><code>typedef</code></td></tr>
    <tr><td><code> LONG_PTR</code></td><td><code>            long</code></td><td>4</td><td>x86</td><td><code>typedef</code></td></tr>
    <tr><td><code> LONG_PTR</code></td><td><code>         __int64</code></td><td>8</td><td>x64</td><td><code>typedef</code></td></tr>
    <tr><td><code>ULONG_PTR</code></td><td><code>unsigned    long</code></td><td>4</td><td>x86</td><td><code>typedef</code></td></tr>
    <tr><td><code>ULONG_PTR</code></td><td><code>unsigned __int64</code></td><td>8</td><td>x64</td><td><code>typedef</code></td></tr>
    <th colspan=5>Microsoft Aliases</th>
    <tr><td><code>     BYTE</code></td><td><code>   unsigned char</code></td><td>1</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>     WORD</code></td><td><code>  unsigned short</code></td><td>2</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>    DWORD</code></td><td><code>   unsigned long</code></td><td>4</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>DWORD_PTR</code></td><td><code>       ULONG_PTR</code></td><td>4/8</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>  SSIZE_T</code></td><td><code>        LONG_PTR</code></td><td>4/8</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>   SIZE_T</code></td><td><code>       ULONG_PTR</code></td><td>4/8</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <th colspan=5>Other Aliases</th>
    <tr><td><code>ptrdiff_t</code></td><td><code>             int</code></td><td>4</td><td>x86</td><td><code>typedef</code></td></tr>
    <tr><td><code>ptrdiff_t</code></td><td><code>         __int64</code></td><td>8</td><td>x64</td><td><code>typedef</code></td></tr>
    <tr><td><code>   size_t</code></td><td><code>unsigned     int</code></td><td>4</td><td>x86</td><td><code>typedef</code></td></tr>
    <tr><td><code>   size_t</code></td><td><code>unsigned __int64</code></td><td>8</td><td>x64</td><td><code>typedef</code></td></tr>
    <tr><td><code>  DWORD64</code></td><td><code>unsigned __int64</code></td><td>8</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>DWORDLONG</code></td><td><code>unsigned __int64</code></td><td>8</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>   LONG64</code></td><td><code>         __int64</code></td><td>8</td><td>x86-64</td><td><code>typedef</code></td></tr>
    <tr><td><code>  ULONG64</code></td><td><code>unsigned __int64</code></td><td>8</td><td>x86-64</td><td><code>typedef</code></td></tr>
  </tbody>
</table>

