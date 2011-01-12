m4_include(`version.m4')m4_dnl
.Language=Russian,Russian (Русский)
.PluginContents=Observer

@Contents
`$^#Observer' OBSERVER_VERSION`#'

 This plugins allows to browse several types of file containers.
 All formats support files listing and extraction.

 Supported file formats:
 - #MSI# installation packages
 - #NSIS# installation packages
 - #UDF# (ISO 13346) images
 - CD disk images : #ISO-9660# (incl. Joliet), #CUE/BIN#, #MDF/MDS#
 - Volition Pack V2 files (from Freespace 1/2/Open)
 - Egosoft packages (for X-series games)
 - MIME containers
 - MS Outlook databases
 - #WISE# Installer packages

 (c) 2009-2010, Ariman
 -----------------------------------------
 Web-site: ~http://ariman.creiac.com~@http://ariman.creiac.com@
 Contact E-mail: ~ariman@@inbox.ru~@mailto:ariman@@inbox.ru@

@ObserverConfig
$ #Observer configuration#
    In this dialog you may change following parameters:

 #Enable plugin#
 Enables / Disables plugin's reaction to Enter and Ctrl-PgDn.
 This option does not affect plugins menu (F11).

 #Use prefix#
 Enables / Disables reaction on prefix.
 
 #Prefix value#
 Text that will be used as prefix (32 chars max).
 
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
