iso plugin for Total Commander (read CD-ROM images) version 1.7.6

Official Polish ISO.WCX plugin website, with tutorials and FAQ
can be found at http://cdrlab.pl

installation:

1. unzip the iso.wcx to your Total Commander installation directory
2. choose the menu configuration - options
3. choose the packer tab
4. click the configure packer extension dlls button
5. type ISO as new extension
6. click new type, and select iso.wcx
7. click ok
skip step 8 if you have TC version over 5.5
8. repeat steps 2-7 for BIN, IMG and NRG extensions

Author: Sergey Oblomov <hoopoepg@gmail.com>
or secondary e-mail hoopoepg@mail.ru
ICQ 12411939

===================================================================

Changes:
2002.03.20  Support for WinOnCD c2d image format. Author: Oliver von Bueren
2003.04.06  Added support for:
            - BIN/CUE images
            - IMG (CloneCD) images
            - NRG (Nero) images
            - Large (>4GB) images (that is, DVD images with ISO Bridge format)
            - Open by Content (Ctrl+PgDn) enabled
            - File Search enabled
            - Mode2/Form2 (S/VCD & XCD) extraction (emulates Windows behaviour)
            Author: DeXT
2003.07.23  version 1.5
            - redesign of code
            - now plugin works MUCH faster
2003.08.19  version 1.5.2 (Oliver von Bueren)
            - changed detection function for VolumeDescriptor, tested with
              c2d (WinOnCD 3.8) and nrg (Nero 6.x) files, should work with
              most other formats without changes.
2003.09.17  version 1.5.2
            - fixed bug with reading last file or folder in directory (it was nor read
              if length of name was 1 symbol)
2003.10.09  version 1.5.4
            - fixed bug with reading .iso images without Jouliet extention:
              in some cases files were named like "filename.ext;1" instead
              of "filename.ext"
2003.29.10  version 1.6
            - added supporting boot images: now boot images may be extracted from
              iso image as regular file
            - updated algorithm for scanning signature of CD images
            - unfortunately i lost most of test images, so, if you found some
              degradation of plugin - let me know (if possible, with link to file)
2003.31.10  version 1.6.1
            - fixed potential bug in detecting size of sector
            - fixed critical bug with reading large images
            - special thanks vmb2003 and SoBal(sobal@cdrinfo.pl)
2003.03.11  version 1.6.2
            - fixed minor bug: on some images boot image was not detected
2003.16.12  version 1.6.3
            - added support of some strange nero format :)
2004.02.04  version 1.6.4
            - fixed minor bug: some non-ISO images were detected as ISO images with
              empty content
2004.02.09  version 1.6.5
            - fixed bug: some ISO-like images (,nrg...) couldn't be read by plugin,
              there was some regression from version 1.6.2
            - added official Polish website for plugin :)
            - fixed extraction bug: file date/time was set incorrectly
2004.02.10  version 1.6.6
            - fixed bug: files 0-size could not be extracted
2004.02.24  version 1.6.7
            - fixed bug: some non comopressed rar images with included .iso images
              were detected as native .iso images
2004.02.27  version 1.6.8
            - feature request: check for canceling operation (user pressed "Cancel"
              button) every few kbytes of extraction
2004.04.08  version 1.6.9
            - changed compilation flags: removed flag -Gs (to prevent fails on some
              systems)
2004.04.23  version 1.6.10
            - fixed serious bug: on some images plugin may enter to infinite loop - 
              TC may halt. Highly recommend to upgrade from older versions
2004.05.21  version 1.6.11
            - try to use user's system locale to convert filenames from unicode to
              ACP codepage... no chance to test - but it should work :)...
              if doesn't work - please, let me know. thanks
2004.06.28  version 1.7b
            - try to implement processing of multisession images... actually processed
              .iso images only (i have no idea about .nrg & other .iso clones files)
2004.09.24  version 1.7.b2
            - fixed bug in detection of Jouliet extension (some images with this
              extension was detected as single ISO9669 images, without extension)
2004.10.05  version 1.7.b3
            - fixed critical bug in file extraction: files with large size in native
              iso images (ISO9660 or Jouliet ext.) was extraxted incorrectly. Highly
              recommended upgrade from previous betas
2004.11.01  version 1.7
            - no changes since 1.7b3 - just change status to Release
            - changed my primary e-mail to hoopoepg@gmail.com
2004.11.04  version 1.7.2
            - changed extension for floppy boot images (now it will have extension .ima)
2005.03.14  version 1.7.3 beta
            - added support of XBOX iso images. Special thanks to Beketata for
              information about this image format and help in implementation
            - fixed bug: large files (>4Gb) may be read with errors
2006.01.12  - fixed bug: buffer overflow in reading of ISO headers
2007.01.23  version 1.7.4 beta 1
            - fixed bug: crash on images with long names. special thanks to
              Tan Chew Keong for bug report (http://vuln.sg/)
2009.03.10  version 1.7.6
            - fixed minor bug: on large (DVD) images opening of image may take
              significant time (up to 20 minutes)
2010.11.09  version 1.7.7 beta 1
            - added unicode API support for TotalCommander (used in Joliet ISO extension)
2010.11.10  version 1.7.7 beta 2
            - fixed issue in detection utf-8 locale of file name in Joliet extension
2010.11.18  version 1.7.7 beta 3
            - disabled by default detection of utf-8 codepage in Unicode filenames
              (because this is not standard and some UCS2 Japan names were detected as
               utf-8 names :)). to enable this heuristic define environment variable
               set ISO_WCX_ENABLE_UTF8 = 1
            - fixed critical issue: plugin may crash on some strange images where
              set of volume descriptors is not complete
            - fixed major issue: some files can't be extracted from large images (~2Gb)
2011.07.25 version 1.7.7 beta 4
            - fixed major issue: on some multi-session images plugin may crush



