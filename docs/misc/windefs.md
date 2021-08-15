# Common Windows Type Definitions
These definitions are taken from various files in the Windows 10 SDK
(specifically version 10.0.18362.0)
* `shared/intsafe.h`
  * #### Basic Types
    |Alias     |Type     |Size (bytes)|Architecture|Alias Type|
    |:---      |:---     |:---:       |:---:       |:---:     |
    |`CHAR`    |`char`   |1           |x86-64      |`typedef` |
    |`SHORT`   |`short`  |2           |x86-64      |`typedef` |
    |`INT`     |`int`    |4           |x86-64      |`typedef` |
    |`LONG`    |`long`   |4           |x86-64      |`typedef` |
    |`LONGLONG`|`__int64`|8           |x86-64      |`typedef` |
  * #### Unsigned Types
    |Alias      |Type              |Size (bytes)|Architecture|Alias Type|
    |:---       |:---              |:---:       |:---:       |:---:     |
    |`UCHAR`    |`unsigned char`   |1           |x86-64      |`typedef` |
    |`USHORT`   |`unsigned short`  |2           |x86-64      |`typedef` |
    |`UINT`     |`unsigned int`    |4           |x86-64      |`typedef` |
    |`ULONG`    |`unsigned long`   |4           |x86-64      |`typedef` |
    |`ULONGLONG`|`unsigned __int64`|8           |x86-64      |`typedef` |
  * #### Bit-width Types
    |Alias  |Type            |Size (bytes)|Architecture|Alias Type|
    |:---   |:---            |:---:       |:---:       |:---:     |
    |`INT8` |`signed char`   |1           |x86-64      |`typedef` |
    |`INT16`|`signed short`  |2           |x86-64      |`typedef` |
    |`INT32`|`signed int`    |4           |x86-64      |`typedef` |
    |`INT64`|`signed __int64`|8           |x86-64      |`typedef` |
  * #### Bit-width Unsigned Types
    |Alias   |Type              |Size (bytes)|Architecture|Alias Type|
    |:---    |:---              |:---:       |:---:       |:---:     |
    |`UINT8` |`unsigned char`   |1           |x86-64      |`typedef` |
    |`UINT16`|`unsigned short`  |2           |x86-64      |`typedef` |
    |`UINT32`|`unsigned int`    |4           |x86-64      |`typedef` |
    |`UINT64`|`unsigned __int64`|8           |x86-64      |`typedef` |
  * #### Pointer Types
    |Alias      |Type                              |Size (bytes)|Architecture|Alias Type|
    |:---       |:---                              |:---:       |:---:       |:---:     |
    |`INT_PTR`  |`int`/`__int64`                   |4/8         |x86/x64     |`typedef` |
    |`UINT_PTR` |`unsigned int`/`unsigned __int64` |4/8         |x86/x64     |`typedef` |
    |`LONG_PTR` |`long`/`__int64`                  |4/8         |x86/x64     |`typedef` |
    |`ULONG_PTR`|`unsigned long`/`unsigned __int64`|4/8         |x86/x64     |`typedef` |
  * #### Microsoft's Named Aliases
    |Alias      |Type            |Size (bytes)|Architecture|Alias Type|
    |:---       |:---            |:---:       |:---:       |:---:     |
    |`BYTE`     |`unsigned char` |1           |x86-64      |`typedef` |
    |`WORD`     |`unsigned short`|2           |x86-64      |`typedef` |
    |`DWORD`    |`unsigned long` |4           |x86-64      |`typedef` |
    |`DWORD_PTR`|`ULONG_PTR`     |4/8         |x86/x64      |`typedef` |
    |`SSIZE_T`  |`LONG_PTR`      |4/8         |x86/x64      |`typedef` |
    |`SIZE_T`   |`ULONG_PTR`     |4/8         |x86/x64      |`typedef` |
  * #### Other Aliases
    |Alias      |Type                             |Size (bytes)|Architecture|Alias Type|
    |:---       |:---                             |:---:       |:---:       |:---:     |
    |`ptrdiff_t`|`int`/`__int64`                  |4/8         |x86/x64     |`typedef` |
    |`size_t`   |`unsigned int`/`unsigned __int64`|4/8         |x86/x64     |`typedef` |
    |`DWORD64`  |`unsigned __int64`               |8           |x86-64      |`typedef` |
    |`DWORDLONG`|`unsigned __int64`               |8           |x86-64      |`typedef` |
    |`LONG64`   |`__int64`                        |8           |x86-64      |`typedef` |
    |`ULONG64`  |`unsigned __int64`               |8           |x86-64      |`typedef` |

* Miscellaneous structs:
  * `GUID` - `shared/guiddef.h`
    |Field  |Type              |Size (bytes)|Misc info|
    |:---   |:---              |:---:       |:---     |
    |`Data1`|`unsigned long`   |4           |         |
    |`Data2`|`unsigned short`  |2           |         |
    |`Data3`|`unsigned short`  |2           |         |
    |`Data4`|`unsigned char[8]`|8           |         |

    **Total Size:** 16 `0x10` bytes

