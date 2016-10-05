/**@file main.c
 * @brief dbcc - produce serialization and deserialization code for CAN DBC files
 * @todo hunt down the causes of the memory leaks */
#include <assert.h>
#include <stdint.h>
#include "mpc.h"
#include "util.h"
#include "can.h"
#include "parse.h"
#include "2xml.h"
#include "2c.h"

typedef enum {
	convert_to_c,
	convert_to_xml
} conversion_type_e;

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
\t-x    convert output to XML instead of the default C code\n\
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
	conversion_type_e convert = convert_to_c; 

	int i;
	for(i = 1; i < argc && argv[i][0] == '-'; i++)
		switch(argv[i][1]) {
		case '\0': /* stop argument processing */
			goto done; 
		case 'x':
			convert = convert_to_xml;
			break;
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
			error("unknown command line option '%c'", argv[i]);
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

		int r = 0;
		switch(convert) {
		case convert_to_c:
			r = dbc2c(dbc, stdout);
			break;
		case convert_to_xml:
			r = dbc2xml(dbc, stdout);
			break;
		default:
			error("invalid conversion type: %d", convert);
		}
		if(r < 0)
			warning("conversion process failed: %u/%u", r, convert);

		dbc_delete(dbc);
		mpc_ast_delete(ast);
	}

	return 0;
}

