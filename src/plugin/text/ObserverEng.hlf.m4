m4_include(`version.m4i')m4_dnl
.Language=English,English
.PluginContents=Observer

@Contents
`$^#Observer' OBSERVER_VERSION`#'

 This plugins allows to browse several types of file containers.
 All formats support files listing and extraction.

 Supported file formats:
 - Installation packages (#MSI#, #NSIS#, #WISE#, #Install Shield#, #Setup Factory#, #CreateInstall#)
 - Optical disk images (#ISO-9660# (incl. Joliet), #CUE/BIN#, #MDF/MDS#, #UDF#)
 - #MIME# containers
 - Mail containers (#MBox#, The Bat!, #MS Outlook#)
 - Virtual disk images (VMDK, VDI, VHD, XVA)
 - #Steam# containers (GCF, WAD, XZP, PAK, BSP, VBSP)
 - #MoPaQ# containers
 - #PDF# embedded files
 - Containers from various games
 
 Also plugin supports Total Commander's #WCX# modules.

 For additional information look in supplied readme.txt

 (c) 2009-2020, Ariman
 -----------------------------------------
 Plugin's Home Page: ~https://github.com/lazyhamster/Observer~@https://github.com/lazyhamster/Observer@
 Contact E-mail: ~ariman@@inbox.ru~@mailto:ariman@@inbox.ru@

@ObserverConfig
$ #Observer configuration#
    In this dialog you may change following parameters:

 #Open files on Enter#
 Enables / Disables plugin's reaction to #Enter#.
 This option does not affect plugins menu (F11).
 
 #Open files on Ctrl-PgDn#
 Enables / Disables plugin's reaction to #Ctrl-PgDn#.
 This option does not affect plugins menu (F11).
 
 #Use prefix#
 Enables / Disables reaction on prefix.
 
 #Prefix value#
 Text that will be used as prefix (32 chars max).

 #Use extension filters#
 Use extension filters when opening files with Enter/Ctrl-PgDn.
 Filters can be set in configuration file.
 
@ObserverExtract
$ #Extracting files#
    In this dialog you may enter destination path for extraction and change several parameters

 #Overwrite existing files#
 Controls files overwriting (3-state).
 [ ] - skip extraction if file already exists,
 [x] - overwrite existing files,
 [?] - ask. 

 #Keep paths#
 Controls the way plugin processes file paths stored inside container (3-state).
 [ ] - all files will be extracted to same directory,
 [x] - keep full path from container root,
 [?] - keep path relative to current directory inside container.
