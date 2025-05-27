# dbcc
## DBC converter/compiler 

This program turns a [DBC][] file into a number of different formats.

For the professional version of DBCC visit:

<https://subleq.co.uk/>

For pricing and contact information. The professional version includes support
for CAN-FD, as well as other languages, and better MISRA-C support.

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

To build, you only need a C (C99) compiler and Make (probably GNU make, I make no
effort to support other Make implementations). The dbcc program itself it
written in what should be portable C with the only external dependency being
your platforms C library.

You should be able to type:

	make

To build, an executable called 'dbcc' is produced. To test run the tests, 
[xmllint][] is required. 

## C Coding Standards

* When in doubt, format with [indent][] with the "-linux" option. 
* Use tabs, not spaces for formatting
* Use assertions where possible (not for error checking, for checking pre/post
conditions and invariants).
* The tool should run on Windows and Linux with no modification. The project is
written in pure [C][].
* No external dependencies should brought into the project.

## Notes on generated code

* If you want a specific format in the generated code, integrate [indent][]
into you toolchain instead of trying to change the code generator.
* The output of the generated code is generally not stable, it may change from commit to
commit, download and maintain a specific version if you want stability.
* That said, the `-n` option can be used to specify the output version which might give you access
  to previous behaviour if backwards compatibility has been implemented.

## Generated output versions

You can specify the version to use on a command line with the `-n` option.
The latest version will be used by default.

**Version 1:**

Legacy/original behaviour. Note that this still won't provide a stable output, but will have
a better chance of not having breaking changes.

**Version 2:**

- Latest version

- Enum names are qualified with the CAN message name
- encode/decode function names are also qualified with the message name

## How to use the generated code

The code generator can make code to unpack a message (turn some bytes into a
data structure), decode a message (apply a scaling/offset minimum and maximum
values to the values in a data structure), and the inverse can be done
(pack/encode).

You can look at the code generated from the DBC files within the project to get
an understanding of how it should work. 

If you want to process a CAN message that you have received you will need to
call the 'unpack\_message'. The code generate is agnostic to the CPUs byte
order, it takes a 'uint64\_t' value containing a single CAN packet (along with
the CAN ID and the DLC for the that packet) and unpacks that into a structure
it generates. The first byte of the CAN packet should be put in the least
significant byte of the 'uint64\_t'.

You can use the following functions to convert to/from a CAN message:

	static uint64_t u64_from_can_msg(const uint8_t m[8]) {
		return ((uint64_t)m[7] << 56) | ((uint64_t)m[6] << 48) | ((uint64_t)m[5] << 40) | ((uint64_t)m[4] << 32) 
			| ((uint64_t)m[3] << 24) | ((uint64_t)m[2] << 16) | ((uint64_t)m[1] << 8) | ((uint64_t)m[0] << 0);
	}

	static void u64_to_can_msg(const uint64_t u, uint8_t m[8]) {
		m[7] = u >> 56;
		m[6] = u >> 48;
		m[5] = u >> 40;
		m[4] = u >> 32;
		m[3] = u >> 24;
		m[2] = u >> 16;
		m[1] = u >>  8;
		m[0] = u >>  0;
	}

The code generator will make a structure based on the file name of the DBC
file, so for the example DBC file 'ex1.dbc' a data structure called
'can\_obj\_ex1\_h\_t' is made. This structure contains all of the CAN message
structures, which in turn contain all of the signals. Having all of the
messages/signals in one structure has advantages and disadvantages, one of the
things it makes easier is defining the data structures needed.

	/* reminder of the 'unpack_message' prototype */
	int unpack_message(can_obj_ex1_h_t *o, const unsigned long id, uint64_t data, uint8_t dlc);

	static can_obj_ex1_h_t ex1;

	uint8_t can_message_raw[8];
	unsigned long id = 0;
	uint8_t dlc = 0;
	your_function_to_receive_a_can_message(can_message_raw, &id, &dlc);
	if (unpack_message(&ex1, id, can_message_u64, dlc) < 0) {
		// Error Condition; something went wrong
		return -1;
	}

'unpack\_message' calls the correct unpack function for that ID, as an example
for ID '0x020':

	case 0x020: return unpack_can_0x020_MagicCanNode1RBootloaderAddress(&o->can_0x020_MagicCanNode1RBootloaderAddress, data, dlc);

The unpack function populates the message object in the 'can\_obj\_ex1\_h\_t'
structure for that ID. The individual signals can then be decoded with the
appropriate functions for that signal. For example:

	uint16_t b = 0;
	if (decode_can_0x020_MagicNode1R_BLAddy(o, &b)) {
		/* error */
	}

To transmit a message, each signal has to be encoded, then the pack function
will return a packed message. 

Some other notes:

* Asserts can be disabled with a command line option
* An option to force the encode/decode function to only use the double width
floating point type has been added, so different function types do not have to be
dealt with by the programmer.
* You can remove the message number from the functions and values generated,
which is useful if your message numbers are changing a lot, however the names
for each message and signal must then be unique.

## DBC file specification

For a specification, as I understand it, of the DBC file format, see [dbc.md][]. 
This is a work in progress.

## DBC VIM syntax file

There is a [Vim][] syntax file for DBC files in the project, called [dbc.vim][]

## XML Generation

As well as [C][], [XML][] can be generated, the project contains an [XSD][] and
[XSLT][] file for the generated XML.

## BSM (beSTORM Module) Generation

An XML based file that can be imported into Beyond Security's beSTORM and used to test CAN BUS infrastructure.

* Note: May be replaced with an [XSLT][] in the future, deriving from the
[XML][] output. The BSM output is itself and XML file.

## CSV Generation

A flat CSV file can be generated, which is easier to import into Excel.

## JSON Generation

A JSON file can be generated, which is what all the cool kids use nowadays.

## Operation

Consult the [manual page][] for more information about the precise operation of the
program.

## Bugs / To Do

* Generated manual page from this markdown "readme.md" file.
* For versions going forward, especially versions that break the generated C
code, it might be nice to have an option to generate previous versions of the
code.
* Support CAN-FD (big task).
* Make definitions for message-ids and Data-Length-Codes so the user
does not have to make them as either an enumeration or a define.
* Make the bit-fields more useful
* The floating point conversion routines assume your platform is using
[IEEE-754][] floats. If it does not, then tough.
* A lot of the DBC file format is not dealt with:
  - Special values
  - Timeouts 
  - Error frames
  - ...
* The generated code is not [MISRA C][] compliant.
* Integers that cannot be represented in a double width floating point number
should be packed/unpacked correctly, however the encode/decode and printing
functions will not as they use doubles for calculations (pack/unpack do not).
This affects numbers larger than 2^53. 
* There are two pieces of information that are useful to any CAN stack for
received messages; the time stamp of the received message, and the status
(error CRC/timeout, message okay, or message never set). This information could
be included in the generated C code.
* Enumeration values could be checked for and only correct values should
be decoded, and encoded. There is more stuff that can be done with the
enumeration values, along with command line options to enumeration generation.
* A mechanism for callbacks for custom code for floating point encoding and
decoding, and other callbacks in general, could be added. Packing and unpacking
floats is done in what should be a portable, but not fast, way.
* A mechanism and system for error handling should be added, that is, a simple
communications manager that does the following:
  - Each signal should have three values associated with it; Unknown (the
    signal has never been set), Valid, and Error (for *any* error).
  - If the time out for a message goes out, or a CRC check fails, then
    all of the messages child messages should get invalidated and set the
    error state.
  - If the signal contains an invalid value, it gets set to an Error state.
  - All signal access functions should check the Unknown/Error value, returning
    success only on a Valid signal.
There are a few other details that would need to be sorted out, like how
CRC and time outs could be calculated.
* The code generator makes code for packing/encoding and unpacking/decoding,
this could be done in one step to simplify the code and data structures, it
means decoded/encoded values do not need to recalculated.

It would be possible to generate nice (ASCII ART) images that show how a message is
structured, which helps in understanding the message in question, and is useful
for documentation purposes, for example, something like:


	Message Name: Example-1
	Message ID: 0x10, 16
	DLC: 1 (8-bits)
	+-----+-----.-----.-----.-----+-----.-----+-----+
	|     |                       |           |     |
	|     |                       |           |     |
	+-----+-----.-----.-----.-----+-----.-----+-----+
	   0     1     2     3     4     5     6     7
	Bit     0: Signal-Name-1, 1 bit signal, scalar 1.0, offset 0
	Bits  1-2: Signal-Name-2, 4 bit signal, signed, Motorola, ...
	... etcetera ...

Or something similar. This would be another output module.


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
[IEEE-754]: https://en.wikipedia.org/wiki/IEEE_754
[indent]: https://www.gnu.org/software/indent/
[xmllint]: http://xmlsoft.org/xmllint.html

<style type="text/css">
	body {
		margin:40px auto;max-width:850px;line-height:1.6;font-size:16px;color:#444;padding:0 10px
	}
	h1,h2,h3 {
		line-height:1.2
	}
</style>
