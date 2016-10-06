# readme.md
# dbcc
## DBC converter/compiler 

This program turns a [DBC][] file into a number of different formats.

## Introduction

**dbcc** is a program for converting a [DBC][] file primarily into into [C][]
code that can serialize and deserialize [CAN][] messages.

## Building, Licenses and Dependencies 

See the [license][] file for details of the license for *this* program, it is
released under the [MIT][] license. Dependencies, if linked against, may have
their own license and their own set of restrictions if built against.

The sources file [mpc.c][] and [mpc.h][] originate from a parser combinator
written in [C][] called [MPC][] and are licensed under the [3 Clause BSD][] 
license

## Operation

Consult the [manual page][] for more information about the precise operation of the
program.

## Bugs

* It does not work yet (implementation has not been completed)

## Goals

* Turn a DBC file into C code
  (see <https://github.com/orangeduck/mpc>)
* Turn a DBC file into an XML representation of the DBC file (with XSD schema)
  (see <http://www.msweet.org/projects.php?Z3>)
* Offer a template mechanism so arbitrary code can be populated with messages
  and signal lists.
* Allow the merging of multiple DBC files
* Write unit tests to cover the converter and the generated code.
* Basic sanity checking of the DBC files could be built in.

[DBC]: http://vector.com/vi_candb_en.html
[C]: https://en.wikipedia.org/wiki/C_%28programming_language%29
[CAN]: https://en.wikipedia.org/wiki/CAN_bus
[license]: LICENSE
[manual page]: dbcc.1
[MIT]: https://en.wikipedia.org/wiki/MIT_License
[3 Clause BSD]: https://en.wikipedia.org/wiki/BSD_licenses
[MPC]: https://github.com/orangeduck/mpc

<style type="text/css">body{margin:40px auto;max-width:850px;line-height:1.6;font-size:16px;color:#444;padding:0 10px}h1,h2,h3{line-height:1.2}</style>

