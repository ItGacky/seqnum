# seqnum
Rename all files in a folder to zero-padded sequential numbers

## License
GNU General Public License, version 3 (GPL-3.0)

## Notice
Only files with the following extensions are renamed:
- .jpg, .jpeg, .jfif
- .png
- .gif

## Registory
Recommended to add an entiry to the context menu of folder backgrounds.

``` seqnum.reg
Windows Registry Editor Version 5.00

[HKEY_CLASSES_ROOT\Directory\Background\shell\seqnum]
@="‰æ‘œ‚ð˜A”Ô‰»"

[HKEY_CLASSES_ROOT\Directory\Background\shell\seqnum\command]
@="\"C:\\bin\\seqnum.exe\" \"%V\""
```
