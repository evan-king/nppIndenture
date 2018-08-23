# nppIndenture

[![version][version-img]][version-url]
[![mit license][license-img]][license-url]
[![build status][appveyor-img]][appveyor-url]

nppIndenture is a [Notepad++](https://notepad-plus-plus.org/) plugin that detects
indentation used for each file opened and automatically configures Notepad++ to
match.  Supported options are tabs and 2 to 8 spaces.

## Download

[Latest version](https://github.com/evan-king/nppIndenture/releases/download/1.0/nppIndenture-1.0.zip) (x86 & x64 included)

## Installation

Extract the appropriate version of `nppIndenture.dll` (x86 or x64) into the appropriate
`plugin` folder:
 - For 32bit: extract from `x86` into `%ProgramFiles(x86)%\Notepad++\plugins`.
 - For 64bit: extract from `x64` into `%ProgramFiles%\Notepad++\plugins`.
 - If user plugins enabled and installing per-user, change the destination to `%APPDATA%\Notepad++\plugins`.

## See Also

I recommend [npp_tabs](http://www.virtualroadside.com/software/#npp_tabs) as an
ideal companion plugin.  It will remove one level of indentation on backspace as
per the file-specific configuration.

## Background

This plugin is forked from [nppAutoDetectIndent](https://github.com/Chocobo1/nppAutoDetectIndent)
by Chocobo1.  The original plugin struggles (more) with space-indented files containing
few lines only indented once, block comments that cascade lines one space further,
and other content errors or inconsistencies.

When the original author would not readily recognize the problem, it became more
expedient to maintain an independent fork, which is now renamed to prevent future
confusion between the two.

[version-url]: https://github.com/evan-king/nppIndenture/releases
[version-img]: https://img.shields.io/github/release/evan-king/nppIndenture.svg?style=flat

[appveyor-url]: https://ci.appveyor.com/project/evan-king/nppIndenture
[appveyor-img]: https://ci.appveyor.com/api/projects/status/github/evan-king/nppIndenture?branch=master&svg=true

[license-url]: https://github.com/evan-king/nppIndenture/blob/master/LICENSE
[license-img]: https://img.shields.io/aur/license/yaourt.svg?style=flat
