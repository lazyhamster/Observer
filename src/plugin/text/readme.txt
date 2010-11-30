Observer
Plug-in for FAR Manager 1.75+ / 2.0+
Copyright: 2009-2010, Ariman

-------------------------------------------------------------------

1. General information.

This plug-in assists in listing and extracting
files from several types of containers.
Add/Delete/Change operations are not planned.

1.1. Supported formats:

- MSI packages for Windows Installer
- UDF images (ISO 13346).
- CD/DVD images : ISO-9660 (incl. Joliet, RockRidge), CUE/BIN, MDF/MDS
- Installation packages made by NSIS (Nullsoft Installer)
- Egosoft packages (for X-series games)
- Volition Pack V2 files (from FreeSpace 1/2/Open)
- MIME containers (.eml, .mht, etc.)
- Containers from games made by Relic (Homeworld 1/2, CoH, WH40k DoW 1/2)
- MS Outlook databases (*.pst).
- WISE Installer packages.

1.2. Settings.

Some plugin options are not available in configuration dialog.
They can be configured in file observer.ini which resides in plugin directory.

Mail plugin options are in [General] section.
Settings for each individual module will reside with section with module name.

Note: For per-user plugin settings you can manualy create value in registry
branch with plugin settings. It must have same name as in .ini file.
Value in registry have priority over same value in file.
All of this is actual for [General] section only.

Currently you can configure following options:

[General] -> PanelHeaderPrefix
Here you can set string that will be shown in file panel header before module name.
Works for all modules simultaneously. Maximum value size in 32 symbols.
Values larger then that will be simply ignored.

[General] -> ExtendedCurDir
This value controls format of current directory value reported back to FAR.
Extended format allows usage of "Folders History" feature to navigate inside containers.
Possible values: 1 (extended mode is on) or 0 (standard format is used).
WARNING:
This feature uses undocumented specifics of curent directory processing by FAR,
so it can have unpredictabe side-effects or stop working at all after next update.

[Filters]
This section sets file extention masks for modules to speed up files processing.
Filters are used when entering file by Enter or PgDn keys.
When plugins menu (F11) or prefix is used filters are ignored.
Values are set in following format: ModuleName=.ext1;.ext2;.ext3
Module names are from [Modules] section and case sensitive. Extentions in list are seprated
by semicolon and have dot in front. They are case-insensitive.
If any module don't have filters set or extentions list is empty, then it is considered
that module accepts all files.

2. License and copyright.

Plug-in includes several third-party code by different authors.
It is distributed under term of GNU LGPL version 3 or any higher
(excluding third-party code that have separate license).

Used third-party code:

- 7-zip (http://www.7-zip.org)
Plug-in uses code for UDF and NSIS support.

- Total Commander ISO plugin by Sergey Oblomov (http://wincmd.ru/plugring/iso.html)
Modified for wchar_t support in file names. Based on version 1.7.6.
Didn't found license for this one, so it goes by LGPL too.

- libmspack (http://www.cabextract.org.uk/libmspack/)
libmspack is a library which provides compressors and decompressors,
archivers and dearchivers for Microsoft compression formats.

- NSIS (http://nsis.sourceforge.net/)
Code used is for bzip2 support. Unpacker version is specific for NSIS.

- X2 FileDriver (http://x-tools.doubleshadow.wz.cz/)
Used for Egosoft (X2, X3, X3:TC) archives support.

- Homeworld 1 Source Code (http://www.relic.com)
Homeworld 1 container decompression code is ripped from publicly available source code.

- PST File Format SDK (http://pstsdk.codeplex.com)
Cross platform header only C++ library for reading PST files (from MS Outlook).
Distributed under Apache License 2.0.

- mimetic (http://codesink.org/mimetic_mime_library.html)
MIME parsing library. Distributed under MIT license.

- Wise UNpacker by J.Markus/Icebird
Used as a reference, minor portions of code directly ported from Delphi to C++.

3. System requirements.
Minimal FAR versions are 1.75 build 2632 / 2.0 build 1666
Windows Installer 4+ is required for MSI support.
OS: Win2k+ (tested on WinXP and Win2003, but 2k should be suported too).

4. Diclaimer.
This software is provided 'AS IS', without any express or implied warranty.
In no event will the author be held liable for any damages arising from the use or not use of this software.

5. Contacts.
Send bug reports, suggeestions, etc to ariman@inbox.ru
