/**@file main.c
 * @brief dbcc - produce serialization and deserialization code for CAN DBC files
 * @todo hunt down the causes of the memory leaks */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdbool.h>
#include "mpc.h"

typedef enum {
	LOG_NO_MESSAGES,
	LOG_ERRORS,
	LOG_WARNINGS,
	LOG_NOTES,
	LOG_DEBUG,
	LOG_ALL_MESSAGES,
} log_level_e;

typedef enum {
	endianess_motorola_e = 0,
	endianess_intel_e = 1,
} endianess_e;

typedef struct {
	char *name;          /**< name of the signal */
	unsigned bit_length; /**< bit length in message buffer */
	unsigned start_bit;  /**< starting bit position in message */
	endianess_e endianess; /**< endianess of message */
	bool is_signed;      /**< if true, value is signed */
	double scaling;      /**< scaling */
	double offset;       /**< offset */
	size_t ecu_count;    /**< ECU count */
	char **ecus;         /**< ECUs sending/receiving */
	union { /**< not used, but would contain the data */
		int64_t integer;
		uint64_t uinteger;
		double floating;
	} data;
} signal_t;

typedef struct {
	char *name;    /**< can message name */
	char *ecu;     /**< name of ECU @todo check this makes sense */
	unsigned dlc;  /**< length of CAN message 0-8 bytes */
	unsigned id;   /**< identifier, 11 or 29 bit */
	uint64_t data; /**< data, up to eight bytes, not used for generation */
	size_t signal_count; /**< number of signals */
	signal_t **signals; /**< signals that can decode/encode this message*/
} can_msg_t;

static log_level_e log_level = LOG_NOTES;

bool verbose(log_level_e ll)
{
	return ll <= log_level && log_level != LOG_NO_MESSAGES;
}

static const char *emsg(void)
{
	return errno ? strerror(errno) : "unknown reason";
}

static void logmsg(log_level_e ll, const char *prefix, const char *fmt, va_list ap)
{
	assert(prefix && fmt && ll < LOG_ALL_MESSAGES && ll >= LOG_NO_MESSAGES);
	if(!verbose(ll))
		return;
	fputs(prefix , stderr);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
}

#define LOG_INTERAL(LEVEL, PREFIX, FMT)\
	do {\
		va_list args;\
		assert(fmt);\
		va_start(args, fmt);\
		logmsg((LEVEL), (PREFIX), (FMT), args);\
		va_end(args);\
	} while(0);


void fatal(const char *fmt, ...)
{
	LOG_INTERAL(LOG_ERRORS, "error: ", fmt);
	exit(EXIT_FAILURE);
}

void warning(const char *fmt, ...)
{
	LOG_INTERAL(LOG_WARNINGS, "warning: ", fmt);
}

void note(const char *fmt, ...)
{
	LOG_INTERAL(LOG_NOTES, "note: ", fmt);
}

void debug(const char *fmt, ...)
{
	LOG_INTERAL(LOG_DEBUG, "error: ", fmt);
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

char *duplicate(const char *s)
{
	assert(s);
	size_t length = strlen(s);
	char *r = allocate(length);
	memcpy(r, s, length);
	return r;
}

signal_t *signal_new(void)
{
	return allocate(sizeof(signal_t));
}

void signal_delete(signal_t *signal)
{
	if(signal)
		free(signal->name);
	for(size_t i = 0; i < signal->ecu_count; i++)
		free(signal->ecus[i]);
	free(signal->ecus);
	free(signal);
}

can_msg_t *can_msg_new(void)
{
	return allocate(sizeof(can_msg_t));
}

void can_msg_delete(can_msg_t *msg)
{
	if(!msg)
		return;
	for(size_t i = 0; i < msg->signal_count; i++)
		signal_delete(msg->signals[i]);
	free(msg->signals);
	free(msg->name);
	free(msg->ecu);
	free(msg);
}

signal_t *ast2signal(mpc_ast_t *ast)
{
	/**@todo clean this up so magic numbers are not used */
	signal_t *sig = signal_new();
	sig->name = duplicate(ast->children[1]->contents);
	sscanf(ast->children[3]->contents, "%u", &sig->start_bit);
	sscanf(ast->children[5]->contents, "%u", &sig->bit_length);
	if(!strcmp("1", ast->children[6]->contents))
		sig->endianess = endianess_intel_e;
	else
		sig->endianess = endianess_motorola_e;
	if(!strcmp("-", ast->children[7]->contents))
		sig->is_signed = true;
	else
		sig->is_signed = false;

	note("signal %s => start %u, length %u, endian: %s, signed: %s", 
		sig->name, sig->start_bit, sig->bit_length,
		sig->endianess == endianess_intel_e ? "intel" : "motorola",
		sig->is_signed ? "true" : "false");
	/**@todo process units, scalar, offset, range and ECUs*/
	return sig;
}

can_msg_t *ast2msg(mpc_ast_t *ast)
{
	/**@todo clean this up so magic numbers are not used */
	can_msg_t *c = can_msg_new();
	sscanf(ast->children[1]->contents, "%u", &c->id);
	c->name = duplicate(ast->children[2]->contents);
	sscanf(ast->children[4]->contents, "%u", &c->dlc);
	c->ecu  = duplicate(ast->children[5]->contents);
	c->signal_count = ast->children_num - 6;
	c->signals = allocate(sizeof(c->signals[0]) * c->signal_count);
	for(size_t i = 0; i < c->signal_count; i++)
		c->signals[i] = ast2signal(ast->children[i+6]);
	note("name %s, ecu %s, id %u, dlc %u", c->name, c->ecu, c->id, c->dlc);
	return c;
}

can_msg_t **ast2msgs(mpc_ast_t *ast)
{
	int n = ast->children_num;
	if(n <= 0)
		return NULL;
	can_msg_t **r = allocate(sizeof(*r) * (n+1));
	for(int i = 0; i < n; i++)
		r[i] = ast2msg(ast->children[i]);
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
	*message   =  mpc_new("message"),
	*messages  =  mpc_new("messages");

	/**@todo process more of the DBC format */
	mpc_err_t *language_error = mpca_lang(MPCA_LANG_DEFAULT,
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
			" messages  : <message>* ; "
			, 
			sign, flt, ident, integer, factor, offset, range, length, node, 
			nodes, unit, startbit, endianess, y_mx_c, name, dlc, signal, message, messages, NULL);

	if (language_error != NULL) {
		mpc_err_print(language_error);
		mpc_err_delete(language_error);
		exit(EXIT_FAILURE);
	}

	int i;
	for(i = 1; i < argc && argv[i][0] == '-'; i++)
		switch(argv[i][1]) {
		case '\0': /* stop argument processing */
			goto done; 
		case 'v':
			log_level++;
			debug("log level: %u", log_level);
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
		debug("reading => %s", argv[i]);
		FILE *file  = fopen_or_die(file_name, "rb");
		char *input = slurp(file);
		mpc_ast_t *ast;
		if(!input)
			fatal("could not slurp file '%s'", file_name);
		mpc_result_t r;
		if (mpc_parse(file_name, input, messages, &r)) {
			ast = r.output;
			ast2msgs(ast);
			if(verbose(LOG_DEBUG))
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

