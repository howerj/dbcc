#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include "mpc.h"

typedef enum {
	endianess_motorola_e = 0,
	endianess_intel_e = 1,
} endianess_e;

typedef struct {
	unsigned dlc;  /**< length of CAN message 0-8 bytes */
	unsigned id;   /**< identifier, 11 or 29 bit */
	uint64_t data; /**< data, up to eight bytes */
} can_msg_t;

typedef struct {
	unsigned length;    /**< bit length */
	unsigned position;  /**< starting bit position in message */
	endianess_e endianess; /**< endianess of message */
	double scaling; /**< scaling */
	double offset;  /**< offset */
	union {
		int64_t integer;
		uint64_t uinteger;
		double floating;
	} data;
} signal_t;

static const char *emsg(void)
{
	return errno ? strerror(errno) : "unknown reason";
}

static void fatal(const char *fmt, ...)
{
	va_list args;
	assert(fmt);
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fputc('\n', stderr);
	exit(EXIT_FAILURE);
}

static FILE *fopen_or_die(const char *name, const char *mode)
{
	assert(name && mode);
	errno = 0;
	FILE *r = fopen(name, mode);
	if(!r) {
		fatal("open '%s' (mode '%s'): %s", name, mode, emsg());
	}
	return r;
}

static void *allocate(size_t sz)
{
	errno = 0;
	void *r = calloc(sz, 1);
	if(!r)
		fatal("allocated failed: %s", emsg());
	return r;
}

/**@warning does not work for large file >4GB */
static char *slurp(FILE *f)
{ 
	long length = 0, r = 0;
	char *b = NULL;
	errno = 0;
	if((r = fseek(f, 0, SEEK_END)) < 0)
		goto fail;
	errno = 0;
	if((r = length = ftell(f)) < 0)
		goto fail;
	errno = 0;
	if((r = fseek(f, 0, SEEK_SET)) < 0)
		goto fail;
	b = allocate(length + 1);
	errno = 0;
	if((unsigned long)length != fread(b, 1, length, f))
		goto fail;
	return b;
fail:
	free(b);
	fprintf(stderr, "slurp failed: %s", emsg());
	return NULL;
}

static void usage(const char *arg0)
{
	fprintf(stderr, "%s: [-] [-h] [-v] file*\n", arg0);
}

static void help(void)
{
	static const char *msg = "\
dbcc - compile CAN DBC files to C code\n\
\n\
Options:\n\
\t-     stop processing command line arguments\n\
\t-h    print out a help message and exit\n\
\t-v    make the program more verbose\n\
\tfile  process a DBC file\n\
\n\
Files must come after the arguments have been processed.\n\
\n\
The parser combinator library (mpc) used in this program is licensed from\n\
Daniel Holden, Copyright (c) 2013, under the BSD3 license\n\
(see https://github.com/orangeduck/mpc/).\n\
\n\
dbcc itself is licensed under the MIT license, Copyright (c) 2016, Richard\n\
Howe. (see https://github.com/howerj/dbcc for the full program source).\n";
	fputs(msg, stderr);
}

int main(int argc, char **argv)
{
	int log_level;
	/**@note see <https://github.com/orangeduck/mpc> for details on the parser*/
	mpc_parser_t 
	*sign      =  mpc_new("sign"),
	*flt       =  mpc_new("float"),
	*ident     =  mpc_new("ident"),
	*integer   =  mpc_new("integer"),
	*factor    =  mpc_new("factor"),
	*offset    =  mpc_new("offset"),
	*range     =  mpc_new("range"),
	*length    =  mpc_new("length"),
	*node      =  mpc_new("node"),
	*nodes     =  mpc_new("nodes"),
	*unit      =  mpc_new("unit"),
	*startbit  =  mpc_new("startbit"),
	*endianess =  mpc_new("endianess"),
	*y_mx_c    =  mpc_new("y_mx_c"), /* scalings; y = m*x + c */
	*name      =  mpc_new("name"),
	*dlc       =  mpc_new("dlc"),
	*signal    =  mpc_new("signal"),
	*message   =  mpc_new("message");

	mpca_lang(MPCA_LANG_DEFAULT,
			" sign      : '+' | '-' ; "
			" float     :  <sign>? /[0-9]+\\.[0-9]*[fF]?/ ; " /* not quite right */
			" ident     : /[a-zA-Z_][a-zA-Z0-9_]*/ ;"
			" integer   : <sign>? /[0-9]+/ ; "
			" factor    : <float> | <integer> ; "
			" offset    : <float> | <integer> ; "
			" length    : /[0-9]+/ ; "
			" range     : '[' ( <float> | <integer> ) '|' ( <float> | <integer> ) ']' ;"
			" node      : <ident> ; "
			" nodes     : <node> ( ',' <node>)* ; "
			" unit      : '\"' <ident>? '\"' ; " /**@bug non-ASCII chars are allowed here */
			" startbit  : <integer> ; "
			" endianess : '0' | '1' ; " /* for the endianess; 0 = Motorola, 1 = Intel */
			" y_mx_c    : '(' <factor> ',' <offset> ')' ; "
			" name      : <ident> ; "
			" dlc       : <integer> ; "
			" signal    : \"SG_\" <name> ':' <startbit> '|' <length> '@' <endianess> <sign> <y_mx_c> "
			"             <range> <unit> <nodes> ; "
			" message   : \"BO_\" <integer> <name> ':' <dlc> <name> <signal>* ; "
			, 
			sign, flt, ident, integer, factor, offset, range, length, node, 
			nodes, unit, startbit, endianess, y_mx_c, name, dlc, signal, message, NULL);

	int i;
	for(i = 1; i < argc && argv[i][0] == '-'; i++)
		switch(argv[i][1]) {
		case '\0': /* stop argument processing */
			goto done; 
		case 'v':
			log_level++;
			break;
		case 'h':
			usage(argv[0]);
			help();
			break;
		default:
			usage(argv[0]);
			fatal("unknown command line option '%c'", argv[i]);
		}
done:
	for(; i < argc; i++) {
		const char *file_name = argv[i];
		FILE *file  = fopen_or_die(file_name, "rb");
		char *input = slurp(file);
		if(!input)
			fatal("could not slurp file '%s'", file_name);
		mpc_result_t r;
		if (mpc_parse(file_name, input, message, &r)) {
			mpc_ast_print(r.output);
			mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		free(input);
	}

	return 0;
}

