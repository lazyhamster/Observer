# Observer #

Compound plugin for FAR File Manager that allows users to browse various file containers and extract content from them.

This is an official repository for the project.

## How to install ##

Download prebuilt binaries and place them in Plugins directory of your FAR Manager installation.

## System requirements ##

* Latest MS Visual C++ v14 Redistributable
* MS Windows Installer 4 or higher (for .msi files support)
* MS .NET Framework 4.8 (for virtual disks support)

## How to build from source ##

Project is developed under MS Visual Studio 2026. Platform toolset is defined in `src\Directory.Build.props`.

Required Visual Studio components:

* Desktop development with C++ workload
* C++/CLI support
* .NET Framework 4.8 targeting pack

Observer modules depend on several libraries that are not supplied with sources.
The easiest way to use these libraries is with [vcpkg](https://github.com/Microsoft/vcpkg) tool.

Use a standalone copy of vcpkg and run `vcpkg integrate install` once, the copy bundled with Visual Studio does not support commands below.

```cmd
git clone https://github.com/microsoft/vcpkg C:/vcpkg
C:/msys64/usr/bin/pacman.exe -S --noconfirm m4
C:/vcpkg/vcpkg.exe integrate install
```

* For x86 version run:
```cmd
C:/vcpkg/vcpkg.exe install zlib bzip2 glib gmime libmspack boost-iostreams --triplet x86-windows
```

* For x64 version run:
```cmd
C:/vcpkg/vcpkg.exe install zlib bzip2 glib gmime libmspack boost-iostreams --triplet x64-windows
```

Additional requirements:
* M4 Macro Processor (must be in %PATH%, or installed with MSYS2 to the default C:\msys64 folder).

```cmd
winget install --id MSYS2.MSYS2 -e
C:/msys64/usr/bin/pacman.exe -S --noconfirm m4
```


## Links ##

[Support Forum (in Russian)](https://forum.farmanager.com/viewtopic.php?f=5&t=4644)

## License ##

Observer is [free](http://www.gnu.org/philosophy/free-sw.html) software: you can use it, redistribute it and/or modify it under the terms of the [GNU Lesser General Public License](http://www.gnu.org/licenses/lgpl.html) as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
