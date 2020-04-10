# Observer #

Compound plugin for FAR File Manager that allows users to browse various file containers and extract content from them.

This is an official repository for the project.

## How to install ##

Download prebuilt binaries and place them in Plugins directory of your FAR Manager installation.

## System requirements ##

* MS Visual C++ 2017 Redistributable Package
* MS Windows Installer 4 or higher (for .msi files support)
* MS .NET Framework 4 (for virtual disks support)

## How to build from source ##

Project is developed under MS Visual Studio 2017.

Observer modules depend on several libraries that are not supplied with sources.
The easiest way to use these libraries is with [vcpkg](https://github.com/Microsoft/vcpkg) tool.

For x86 version run:
* vcpkg install zlib bzip2 glib gmime libmspack --triplet x86-windows

For x64 version run:
* vcpkg install zlib bzip2 glib gmime libmspack --triplet x64-windows

Additional requirements:

* Boost C++ Libraries.
* M4 Macro Processor (must be in %PATH%).

## Links ##

[Support Forum (in Russian)](https://forum.farmanager.com/viewtopic.php?f=5&t=4644)

## License ##

Observer is [free](http://www.gnu.org/philosophy/free-sw.html) software: you can use it, redistribute it and/or modify it under the terms of the [GNU Lesser General Public License](http://www.gnu.org/licenses/lgpl.html) as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
