Observer
Plug-in for FAR Manager 1.75+ / 2.0+ / 3.0+
Copyright: 2009-2014, Ariman

-------------------------------------------------------------------

1. General information.

This plug-in assists in listing and extracting content from several types of containers.
Add/Delete/Change operations are not planned.

1.1. Supported formats:

- Installation packages
  - Installation packages made with NSIS
  - MSI packages for Windows Installer
  - Installation packages made with Wise Installer
  - Install Shield packages.
  - Setup Factory packages.

- Образы оптических дисков (CD/DVD/Blu-ray)
  - ISO-images. Following formats are supported:
    - ISO-9660 (incl. Joliet, RockRidge)
    - UDF (ISO 13346) up to revision 2.60
  - NRG-images Nero Burning ROM
  - BIN-images CDRWIN (CUE/BIN)
  - MDF-images Alcohol 120% (MDF/MDS)
  - ISZ-images UltraISO

- Virtual disk images
  - VMDK (VMware)
  - VDI  (Virtual Box)
  - VHD  (Microsoft Virtual PC)
  - XVA  (Xen Virtual Appliance)

- MIME
  - MIME containers (.eml, .mht, etc.)
  - MS Outlook databases (*.pst)
  - MBox containers
  - The Bat! databases (*.tbb)

- Containers used in various games
  - CAT, PCK, PBD, PBB - used by Egosoft for X-series games
  - VP - Volition Pack V2 (from FreeSpace 1/2/Open)
  - BIG, SGA - containers from games made by Relic (Homeworld 1/2, CoH, WH40k DoW 1/2)
  - GCF, WAD, XZP, PAK, BSP, VBSP - used inside Steam
  - MoPaQ packages (used by Blizzard)

- Other
  - Embedded files from PDF.

- Supports usage of WCX modules from Total Commander (read-only)

1.2. Settings.

Some plugin options are not available in configuration dialog.
They can be configured in file observer.ini which resides in plugin directory.

Also plugin supports user-specific configuration file observer_user.ini.
This file may be useful in cases where editing of main configuration file, which is present
in installation package, is unwanted for some reason.
Structure of the observer_user.ini is the same as for observer.ini. Values from user file
take precedence over main configutration file.

Mail plugin options are in [General] section.
Settings for each individual module will reside with section with module name.

Note: For per-user plugin settings you can manualy create value in registry
branch with plugin settings. It must have same name as in .ini file.
Value in registry have priority over same value in file.
All of this is actual for [General] section only.

Currently you can configure following options:

1.2.1 General settings.

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

[General] -> VerboseModuleLoad
Turns on/off warning message on plugin start if any if the modules failed to load.
Possible values: 1 (show message) or 0 (don't show).

1.2.2 Filters.

[Filters]
This section sets file extention masks for modules to speed up files processing.
Filters are used when entering file by Enter or PgDn keys.
When plugins menu (F11) or prefix is used filters are ignored.
Values are set in following format: ModuleName=.ext1;.ext2;.ext3
Module names are from [Modules] section and case sensitive. Extentions in list are seprated
by semicolon and have dot in front. They are case-insensitive.
If any module don't have filters set or extentions list is empty, then it is considered
that module accepts all files.

1.2.3 Module-specific configuration.

Values marked with * are default values.

[ISO] -> Charset
[VDISK] -> DefaultCodepage
Default code page for non-Unicode file names.
You can set here specific code page (e.g. 866 or 1251),
or you can set special system code page values:
1 - current OEM code page (default value),
0 - current ANSI code page.

[ISO] -> RockRidge
Enables (1)* / disables (0) support for RockRidge extension.

[PST] -> HideEmptyFolder
Enables (1) / disables (0)* hiding of empty folders.

[WCX] -> WcxLocation
Full path to the location of wcx files. Default value is folder named "wcx" in the same folder as module file.
[WCX] -> RecursiveLoad
Enables (1)* / disables (0) recursive scan when searching for wcx.

1.2.4 Module selection shortcuts.

[Shortcuts]
Here you can set shortcut symbol for module that can be used in module selection menu.
Shortcut can be a number of any alphabetical character ('A' - 'Z'). Character case is irrelevant.

1.3 Plugin configuration dialog.

As opposed to setttings, described in 1.2, which applied only on program start,
some parameters can be changed from configuration dialog (F9 -> Options -> Plugins Configuration).
These values are applied right after dialog is closed (unless you hit 'Cancel' or Esc).
Here you can set the following parameters:

- Enable/disable reaction on Ctrl-PgDn (plugin is always available from F11 menu).
- Configure plugin prefix (default value is 'observe').
- Enable/disable using of extesion filters (see 1.2.2 above).

2. License and copyright.

Plug-in includes several third-party code by different authors.
It is distributed under term of GNU LGPL version 3 or any higher
(excluding third-party code that have separate license).

Used third-party code:

- 7-zip (http://www.7-zip.org)
Plug-in uses code for UDF and NSIS support. Distributed under GNU LGPL.

- Total Commander ISO plugin by Sergey Oblomov (http://wincmd.ru/plugring/iso.html)
Modified for wchar_t support in file names.
Didn't found license for this one, so it goes by LGPL too.

- libmspack (http://www.cabextract.org.uk/libmspack/)
libmspack is a library which provides compressors and decompressors,
archivers and dearchivers for Microsoft compression formats.

- X2 FileDriver (http://x-tools.doubleshadow.wz.cz/)
Used for Egosoft (X2, X3, X3:TC) archives support.

- Homeworld 1 Source Code (http://www.relic.com)
Homeworld 1 container decompression code is ripped from publicly available source code.

- PST File Format SDK (http://pstsdk.codeplex.com)
Cross platform header only C++ library for reading PST files (from MS Outlook).
Distributed under Apache License 2.0.

- Wise UNpacker by J.Markus/Icebird
Used as a reference, minor portions of code directly ported from Delphi to C++.

- HLLib (http://nemesis.thewavelength.net/index.php?p=35)
HLLib is a package library for Half-Life that abstracts several package formats and provides a simple interface for all of them.
Distributed under LGPL license.

- DiscUtils (http://discutils.codeplex.com/)
DiscUtils is a .NET library to read and write ISO files and Virtual Machine disk files (VHD, VDI, XVA, VMDK, etc).
Distributed under MIT license.

- StormLib (http://www.zezula.net/en/mpq/stormlib.html)
The StormLib library is a pack of modules, written in C++, which are able to read and also to write files from/to the MPQ archives.

- GMime (http://spruce.sourceforge.net/gmime/)
GMime is a C/C++ library which may be used for the creation and parsing of messages using the Multipurpose Internet Mail Extension (MIME).
Library is distributed under GNU LGPL.

- zlib (http://www.zlib.net)
zlib is lossless data-compression library. Distributed under zlib license.

- libbzip2 (http://bzip.org)
bzip2 is a freely available, patent free (see below), high-quality data compressor.
Distributed under BSD-style license.

- i5comp (ftp://ftp.elf.stuba.sk/pub/pc/pack/i5comp21.rar)
InstallShield 5.x Cabinet Compression & Maintenance Util by fOSSiL.

- i6comp (ftp://ftp.elf.stuba.sk/pub/pc/pack/i6comp02.zip)
InstallShield 6.x Cabinet Util by fOSSiL & Morlac.

- i6compx (http://www.ctpax-x.org/uploads/i6compx.zip)
Patched i6comp for Unicode support.

- unshield (https://github.com/twogood/unshield)
Pocket PC tool from SyncCE project.
Distributed under LGPL license.

- poppler (http://poppler.freedesktop.org/)
PDF rendering library.

3. System requirements.
OS: WinXP or higher.
Minimal FAR versions are 1.75 build 2634 / 2.0 build 1807

Windows Installer 4+
Microsoft Visual C++ 2010 SP1 Redistributable Package

VDISK module requires Microsoft .NET Framework 4.0 to be installed.
If you don't install .NET then this modile will not load.

4. Diclaimer.
This software is provided 'AS IS', without any express or implied warranty.
In no event will the author be held liable for any damages arising from the use or not use of this software.

5. Contacts.
Send bug reports, suggeestions, etc to ariman@inbox.ru
