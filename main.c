/**@file main.c
 * @brief dbcc - produce serialization and deserialization code for CAN DBC files
 * @copyright Richard James Howe
 * @license MIT */
#include <assert.h>
#include <stdint.h>
#include "mpc.h"
#include "util.h"
#include "can.h"
#include "parse.h"
#include "2c.h"
#include "2xml.h"
#include "2csv.h"
#include "2bsm.h"
#include "2json.h"
#include "options.h"

typedef enum {
	CONVERT_TO_C,
	CONVERT_TO_XML,
	CONVERT_TO_CSV,
	CONVERT_TO_BSM,
	CONVERT_TO_JSON,
} conversion_type_e;

static void usage(const char *arg0)
{
	assert(arg0);
	fprintf(stderr, "%s: [-] [-hvjgtxpkuDC] [-o dir] file*\n", arg0);
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
\t-t     add timestamps to the generated files\n\
\t-x     convert output to XML instead of the default C code\n\
\t-C     convert output to CSV instead of the default C code\n\
\t-b     convert output to BSM (beSTORM) instead of the default C code\n\
\t-j     convert output to JSON instead of the default C code\n\
\t-D     use 'double' for the encode/decode type messages\n\
\t-o dir set the output directory\n\
\t-p     generate only print code\n\
\t-k     generate only pack code\n\
\t-u     generate only unpack code\n\
\t-s     disable assert generation\n\
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

static char *replace_file_type(const char *file, const char *suffix)
{
	assert(file);
	assert(suffix);
	char *name = duplicate(file);
	char *dot = strrchr(name, '.');
	if (*dot)
		*dot = '\0';
	size_t name_size = strlen(name) + strlen(suffix) + 2;
	name = reallocator(name, name_size); /* + 1 for '.', + 1 for '\0' */
	strcat(name, ".");
	strcat(name, suffix);
	return name;
}

static int dbc2cWrapper(dbc_t *dbc, const char *dbc_file, const char *file_only, dbc2c_options_t *copts)
{
	assert(dbc);
	assert(dbc_file);
	assert(file_only);
	char *cname = replace_file_type(dbc_file,  "c");
	char *hname = replace_file_type(dbc_file,  "h");
	char *fname = replace_file_type(file_only, "h");
	FILE *c = fopen_or_die(cname, "wb");
	FILE *h = fopen_or_die(hname, "wb");
	const int r = dbc2c(dbc, c, h, fname, copts);
	fclose(c);
	fclose(h);
	free(cname);
	free(hname);
	free(fname);
	return r;
}

static int dbc2xmlWrapper(dbc_t *dbc, const char *dbc_file, bool use_time_stamps)
{
	assert(dbc);
	assert(dbc_file);
	char *name = replace_file_type(dbc_file, "xml");
	FILE *o = fopen_or_die(name, "wb");
	const int r = dbc2xml(dbc, o, use_time_stamps);
	fclose(o);
	free(name);
	return r;
}

static int dbc2csvWrapper(dbc_t *dbc, const char *dbc_file)
{
	assert(dbc);
	assert(dbc_file);
	char *name = replace_file_type(dbc_file, "csv");
	FILE *o = fopen_or_die(name, "wb");
	const int r = dbc2csv(dbc, o);
	fclose(o);
	free(name);
	return r;
}

static int dbc2bsmWrapper(dbc_t *dbc, const char *dbc_file, bool use_time_stamps)
{
	assert(dbc);
	assert(dbc_file);
	char *name = replace_file_type(dbc_file, "bsm");
	FILE *o = fopen_or_die(name, "wb");
	const int r = dbc2bsm(dbc, o, use_time_stamps);
	fclose(o);
	free(name);
	return r;
}

static int dbc2jsonWrapper(dbc_t *dbc, const char *dbc_file, bool use_time_stamps)
{
	assert(dbc);
	assert(dbc_file);
	char *name = replace_file_type(dbc_file, "json");
	FILE *o = fopen_or_die(name, "wb");
	const int r = dbc2json(dbc, o, use_time_stamps);
	fclose(o);
	free(name);
	return r;
}

int main(int argc, char **argv)
{
	log_level_e log_level = get_log_level();
	conversion_type_e convert = CONVERT_TO_C;
	const char *outdir = NULL;
	dbc2c_options_t copts = {
		.use_id_in_name            =  true,
		.use_time_stamps           =  false,
		.use_doubles_for_encoding  =  false,
		.generate_print            =  false,
		.generate_pack             =  false,
		.generate_unpack           =  false,
		.generate_asserts          =  true,
	};
	int opt = 0;

	while ((opt = dbcc_getopt(argc, argv, "hVvbjgxCNtDpukso:")) != -1) {
		switch (opt) {
		case 'h':
			usage(argv[0]);
			help();
			break;
		case 'V':
			fprintf(stderr, "%s\n", DBCC_VERSION);
			break;
		case 'v':
			set_log_level(++log_level);
			debug("log level: %u", log_level);
			break;
		case 'g':
			return printf("DBCC Grammar =>\n%s\n", parse_get_grammar()) < 0;
		case 'b':
			convert = CONVERT_TO_BSM;
			break;
		case 'j':
			convert = CONVERT_TO_JSON;
			break;
		case 'x':
			convert = CONVERT_TO_XML;
			break;
		case 'C':
			convert = CONVERT_TO_CSV;
			break;
		case 'N':
			copts.use_id_in_name = false;
			break;
		case 't':
			copts.use_time_stamps = true;
			debug("using time stamps");
			break;
		case 'D':
			copts.use_doubles_for_encoding = true;
			debug("using doubles for encoding");
			break;
		case 'p':
			copts.generate_print = true;
			debug("generate code for print");
			break;
		case 'u':
			copts.generate_unpack = true;
			debug("generate code for unpack");
			break;
		case 'k':
			copts.generate_pack = true;
			debug("generate code for pack");
			break;
		case 'o':
			outdir = dbcc_optarg;
			debug("output directory: %s", outdir);
			break;
		case 's':
			copts.generate_asserts = false;
			debug("asserts disabled - apparently you think silent corruption is a good thing", outdir);
			break;
		default:
			fprintf(stderr, "invalid options\n");
			usage(argv[0]);
			help();
			break;
		}
	}

	if (!copts.generate_unpack && !copts.generate_pack && !copts.generate_print) {
		copts.generate_print  = true;
		copts.generate_pack   = true;
		copts.generate_unpack = true;
	}

	for(int i = dbcc_optind; i < argc; i++) {
		debug("reading => %s", argv[i]);
		mpc_ast_t *ast = parse_dbc_file_by_name(argv[i]);
		if (!ast) {
			warning("could not parse file '%s'", argv[i]);
			continue;
		}
		if (verbose(LOG_DEBUG))
			mpc_ast_print(ast);

		dbc_t *dbc = ast2dbc(ast);

		char *outpath = dbcc_basename(argv[i]);
		if (outdir) {
			outpath = allocate(strlen(outpath) + strlen(outdir) + 2 /* '/' + '\0'*/);
			strcat(outpath, outdir);
			strcat(outpath, "/");
			strcat(outpath, dbcc_basename(argv[i]));
		}

		int r = 0;
		switch(convert) {
		case CONVERT_TO_C:
			r = dbc2cWrapper(dbc, outpath, dbcc_basename(argv[i]), &copts);
			break;
		case CONVERT_TO_XML:
			r = dbc2xmlWrapper(dbc, outpath, copts.use_time_stamps);
			break;
		case CONVERT_TO_CSV:
			if (copts.use_time_stamps)
				error("Cannot use time stamps when specifying CSV option");
			r = dbc2csvWrapper(dbc, outpath);
			break;
		case CONVERT_TO_BSM:
			r = dbc2bsmWrapper(dbc, outpath, copts.use_time_stamps);
			break;
		case CONVERT_TO_JSON:
			r = dbc2jsonWrapper(dbc, outpath, copts.use_time_stamps);
			break;
		default:
			error("invalid conversion type: %d", convert);
		}
		if (r < 0)
			warning("conversion process failed: %u/%u", r, convert);

		if (outdir)
			free(outpath);
		dbc_delete(dbc);
		mpc_ast_delete(ast);
	}

	return 0;
}

