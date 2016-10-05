/**@file main.c
 * @brief dbcc - produce serialization and deserialization code for CAN DBC files
 * @todo hunt down the causes of the memory leaks */
#include <assert.h>
#include <stdint.h>
#include "mpc.h"
#include "util.h"
#include "can.h"
#include "parse.h"

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
	log_level_e log_level = get_log_level();
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
			set_log_level(++log_level);
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
		debug("reading => %s", argv[i]);
		mpc_ast_t *ast = parse_dbc_file_by_name(argv[i]);
		if(!ast) {
			warning("could not parse file '%s'", argv[i]);
			continue;
		}
		if(verbose(LOG_DEBUG))
			mpc_ast_print(ast);

		dbc_t *dbc = ast2dbc(ast);
		/*dbc_delete(dbc);*/
		mpc_ast_delete(ast);
	}

	return 0;
}

