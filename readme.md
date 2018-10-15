# dbcc
## DBC converter/compiler 

This program turns a [DBC][] file into a number of different formats.

## Introduction

**dbcc** is a program for converting a [DBC][] file primarily into into [C][]
code that can serialize and deserialize [CAN][] messages into structures that
represent those messages and signals. It is also possible to print out the
information contained in a structure.

## Building, Licenses and Dependencies 

See the [license][] file for details of the license for *this* program, it is
released under the [MIT][] license. Dependencies, if linked against, may have
their own license and their own set of restrictions if built against.

The sources file [mpc.c][] and [mpc.h][] originate from a parser combinator
written in [C][] called [MPC][] and are licensed under the [3 Clause BSD][] 
license.

## DBC file specification

For a specification, as I understand it, of the DBC file format, see [dbc.md][]. 
This is a work in progress.

## DBC VIM syntax file

There is a [Vim][] syntax file for DBC files in the project, called [dbc.vim][]

## XML Generation

As well as [C][], [XML][] can be generated, the project contains an [XSD][] and
[XSLT][] file for the generated XML.

## CSV Generation

A flat CSV file can be generated, which is easier to import into Excel.

## Operation

Consult the [manual page][] for more information about the precise operation of the
program.

## Bugs / To Do

* A lot of the DBC file format is not dealt with:
  - Special values
  - Timeouts 
  - Error frames
  - ...
* The generated C code has not been tested that much (it is probably incorrect). The
generated code is also not [MISRA C][] compliant, but it would not take too much to
make it so.
* Integers that cannot be represented in a double width floating point number
should be packed/unpacked correctly, however the encode/decode and printing
functions will not as they use doubles for calculations (pack/unpack do not).
This affects numbers larger than 2^53. 
* Allow the merging of multiple DBC files
* Write unit tests to cover the converter and the generated code.
* Basic sanity checking of the DBC files could be built in.
  - The easiest way to check this is by generating an XML file and verifying it
  with an XSD file
* Find/make more CAN database examples

[DBC]: http://vector.com/vi_candb_en.html
[C]: https://en.wikipedia.org/wiki/C_%28programming_language%29
[CAN]: https://en.wikipedia.org/wiki/CAN_bus
[license]: LICENSE
[manual page]: dbcc.1
[MIT]: https://en.wikipedia.org/wiki/MIT_License
[3 Clause BSD]: https://en.wikipedia.org/wiki/BSD_licenses
[MPC]: https://github.com/orangeduck/mpc
[mpc.c]: mpc.c
[mpc.h]: mpc.h
[dbc.md]: dbc.md
[dbc.vim]: dbc.vim
[Vim]: http://www.vim.org/download.php
[XML]: https://en.wikipedia.org/wiki/XML
[XSD]: dbcc.xsd
[XSLT]: dbcc.xslt
[MISRA C]: https://misra.org.uk/

<style type="text/css">
	body {
		margin:40px auto;max-width:850px;line-height:1.6;font-size:16px;color:#444;padding:0 10px
	}
	h1,h2,h3 {
		line-height:1.2
	}
</style>
