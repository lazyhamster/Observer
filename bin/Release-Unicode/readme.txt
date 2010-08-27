Observer
Plug-in for FAR Manager 1.75+ / 2.0+
Copyright: 2009-2010, Ariman

-------------------------------------------------------------------

1. General information.

This plug-in assists in listing and extracting
files from several types of containers.
Add/Delete/Change operations are not planned.

Supported formats:

- MSI packages for Windows Installer

- UDF images (ISO 13346).

- CD/DVD images : ISO-9660 (incl. Joliet, RockRidge), CUE/BIN, MDF/MDS

- Installation packages made by NSIS (Nullsoft Installer)

- Egosoft packages (for X-series games)

- Volition Pack V2 files (from FreeSpace 1/2/Open)

- MIME containers (.eml, .mht, etc.)

2. License and copyright.

Plug-in includes several third-party code by different authors.
It distributed under term of GNU LGPL version 3 or any higher
(excluding parts of the code that have separate license).

Ued third-party code:

- 7-zip (http://www.7-zip.org)
Plug-in uses code for UDF and NSIS support.

- Total Commander ISO plugin by Sergey Oblomov (http://wincmd.ru/plugring/iso.html)
Modified for wchar_t support in file names. Based on version 1.7.4 b1.
Didn't found license for this one, so it goes by LGPL too.

- libmspack (http://www.cabextract.org.uk/libmspack/)
libmspack is a library which provides compressors and decompressors,
archivers and dearchivers for Microsoft compression formats.

- NSIS (http://nsis.sourceforge.net/)
Code used is for bzip2 support. Unpacker version is specific for NSIS.

- X2 FileDriver (http://x-tools.doubleshadow.wz.cz/)
User for Egosoft (X2, X3, X3:TC) archives support.

3. System requirements.
Minimal FAR versions are 1.75 build 2587 / 2.0 build 1470
Windows Installer 4+ is required for MSI support.
OS: Win2k+ (tested on WinXP and Win2003, but 2k should be suported too).

4. Diclaimer.
This software is provided 'as-is', without any express or implied warranty.
In no event will the author be held liable for any damages arising from the use or not use of this software.

5. Contacts.
Send bug reports, suggeestions, etc to ariman@inbox.ru
