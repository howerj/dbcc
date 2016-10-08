/**@file main.c
 * @brief dbcc - produce serialization and deserialization code for CAN DBC files*/
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
	fprintf(stderr, "%s: [-] [-h] [-v] [-g] [-x] [-o dir] file*\n", arg0);
}

static void help(void)
{
	static const char *msg = "\
dbcc - compile CAN DBC files to C code\n\
\n\
Options:\n\
\t-      stop processing command line arguments\n\
\t-h     print out a help message and exit\n\
\t-v     make the program more verbose\n\
\t-g     print out the grammar used to parse the DBC files\n\
\t-x     convert output to XML instead of the default C code\n\
\t-o dir set the output directory\n\
\tfile   process a DBC file\n\
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

char *replace_file_type(const char *file, const char *suffix)
{
	char *name = duplicate(file);
	char *dot = strrchr(name, '.');
	if(*dot)
		*dot = '\0';
	name = reallocator(name, strlen(name) + strlen(suffix) + 2); /* + 1 for '.', + 1 for '\0' */
	strcat(name, ".");
	strcat(name, suffix);
	return name;
}

int dbc2cWrapper(dbc_t *dbc, const char *dbc_file, const char *file_only)
{
	char *cname = replace_file_type(dbc_file,  "c");
	char *hname = replace_file_type(dbc_file,  "h");
	char *fname = replace_file_type(file_only, "h");
	FILE *c = fopen_or_die(cname, "wb");
	FILE *h = fopen_or_die(hname, "wb");
	int r = dbc2c(dbc, c, h, fname);
	fclose(c);
	fclose(h);
	free(cname);
	free(hname);
	free(fname);
	return r;
}

int dbc2xmlWrapper(dbc_t *dbc, const char *dbc_file)
{
	char *name = replace_file_type(dbc_file, "xml");
	FILE *o = fopen_or_die(name, "wb");
	int r = dbc2xml(dbc, o);
	fclose(o);
	free(name);
	return r;
}

int main(int argc, char **argv)
{
	log_level_e log_level = get_log_level();
	conversion_type_e convert = convert_to_c; 
	const char *outdir = NULL;

	int i;
	for(i = 1; i < argc && argv[i][0] == '-'; i++)
		switch(argv[i][1]) {
		case '\0': /* stop argument processing */
			goto done; 
		case 'h':
			usage(argv[0]);
			help();
			break;
		case 'v':
			set_log_level(++log_level);
			debug("log level: %u", log_level);
			break;
		case 'g':
			return printf("DBCC Grammar =>\n%s\n", parse_get_grammar()) < 0;
		case 'x':
			convert = convert_to_xml;
			break;
		case 'o':
			if(i >= argc - 1)
				goto fail;
			outdir = argv[++i];
			debug("output directory: %s", outdir);
			break;
		default:
		fail:
			usage(argv[0]);
			error("unknown/invalid command line option '%c'", argv[i][1]);
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

		char *outpath = argv[i];
		if(outdir) {
			outpath = allocate(strlen(outpath) + strlen(outdir) + 2 /* '/' + '\0'*/);
			strcat(outpath, outdir);
			strcat(outpath, "/");
			strcat(outpath, argv[i]);
		}

		int r = 0;
		switch(convert) {
		case convert_to_c:
			r = dbc2cWrapper(dbc, outpath, argv[i]);
			break;
		case convert_to_xml:
			r = dbc2xmlWrapper(dbc, outpath);
			break;
		default:
			error("invalid conversion type: %d", convert);
		}
		if(r < 0)
			warning("conversion process failed: %u/%u", r, convert);

		if(outdir)
			free(outpath);
		dbc_delete(dbc);
		mpc_ast_delete(ast);
	}

	return 0;
}

