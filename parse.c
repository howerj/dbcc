#include "parse.h"
#include "util.h"

static mpc_ast_t *_parse_dbc_string(const char *file_name, const char *string);
static mpc_ast_t *_parse_dbc_file_by_handle(const char *name, FILE *handle);

#define X_MACRO_PARSE_VARS\
	X(spaces,     "s")\
	X(newline,    "n")\
	X(sign,       "sign")\
	X(flt,        "float")\
	X(ident,      "ident")\
	X(integer,    "integer")\
	X(factor,     "factor")\
	X(offset,     "offset")\
	X(range,      "range")\
	X(length,     "length")\
	X(node,       "node")\
	X(nodes,      "nodes")\
	X(stringp,    "string")\
	X(unit,       "unit")\
	X(startbit,   "startbit")\
	X(endianess,  "endianess")\
	X(y_mx_c,     "y_mx_c")\
	X(name,       "name")\
	X(ecu,        "ecu")\
	X(dlc,        "dlc")\
	X(id,         "id")\
	X(multiplexor, "multiplexor")\
	X(signal,     "signal")\
	X(message,    "message")\
	X(messages,   "messages")\
	X(types,      "types")\
	X(etcetera,   "etcetera")\
	X(version,    "version")\
	X(ecus,       "ecus")\
	X(symbols,    "symbols")\
	X(bs,         "bs")\
	X(whatever,   "whatever")\
	X(values,     "values")\
	X(dbc,        "dbc")

/**@todo This grammar needs expanding and fixing, one addition would be comment lines,
 * which certain tools allow. This consists of a '//' with the rest of the line ignored.
 * Ideally the grammar would be rewritten, to follow Vectors documentation on it. */

static const char *dbc_grammar = 
" s         : /[ \\t]/ ; \n"
" n         : /\\r?\\n/ ; \n"
" sign      : '+' | '-' ; \n"
" float     : /[-+]?[0-9]+(\\.[0-9]+)?([eE][-+]?[0-9]+)?/ ; \n"
" ident     : /[a-zA-Z_][a-zA-Z0-9_]*/ ;\n"
" integer   : <sign>? /[0-9]+/ ; \n"
" factor    : <float> | <integer> ; \n"
" offset    : <float> | <integer> ; \n"
" length    : /[0-9]+/ ; \n"
" range     : '[' ( <float> | <integer> ) '|' ( <float> | <integer> ) ']' ;\n"
" node      : <ident> ; \n"
" nodes     : <node> <s>* ( ',' <s>* <node>)* ; \n"
" string    : '\"' /[^\"]*/ '\"' \n; " 
" unit      : <string> ; \n" 
" startbit  : <integer> ; \n"
" endianess : '0' | '1' ; \n" /* for the endianess; 0 = Motorola, 1 = Intel */
" y_mx_c    : '(' <factor> ',' <offset> ')' ; \n"
" name      : <ident> ; \n"
" ecu       : <ident> ; \n"
" dlc       : <integer> ; \n"
" id        : <integer> ; \n"
" multiplexor : 'M' | 'm' <s>* <integer> ; \n" 
" signal    : <s>* \"SG_\" <s>+ <name> <s>* <multiplexor>? <s>* ':' <s>* <startbit> <s>* '|' <s>* \n"
"             <length> <s>* '@' <s>* <endianess> <s>* <sign> <s>* <y_mx_c> <s>* \n"
"             <range> <s>* <unit> <s>* <nodes> <s>* <n> ; \n"
" message   : \"BO_\" <s>+ <id> <s>+ <name>  <s>* ':' <s>* <dlc> <s>+ <ecu> <s>* <n> <signal>* ; \n"
" messages  : (<message> <n>+)* ; \n"
" version   : \"VERSION\" <s> <string> <n>+ ; \n"
" ecus      : \"BU_\" <s>* ':' (<ident>|<s>)* <n> ; \n"
" symbols   : \"NS_\" <s>* ':' <s>* <n> ('\t' <ident> <n>)* <n> ; " 
" whatever  : (<ident>|<string>|<integer>|<float>) ; \n"
" bs        : \"BS_\" <s>* ':' <n>+ ; " 
" types     : <s>* <ident> (<whatever>|<s>)+ ';' (<n>*|/$/) ; \n"
" etcetera  : <types>+ ; \n" /**@note don't care about the rest of the file, for now*/
" values    : \"VAL_TABLE_\" (<whatever>|<s>)* ';' <n> ; \n" /**@note don't care about this, for now*/
" dbc       : <version> <symbols> <bs> <ecus> <values>* <n>* <messages> <etcetera>  ; \n" ;

const char *parse_get_grammar(void)
{
	return dbc_grammar;
}

mpc_ast_t *parse_dbc_file_by_name(const char *name)
{
	mpc_ast_t *ast = NULL;
	FILE *input = NULL; 
	
	if(!(input = fopen(name, "rb")))
		goto end;
	ast = _parse_dbc_file_by_handle(name, input);
end:
	if(input)
		fclose(input);
	return ast;
}

mpc_ast_t *parse_dbc_file_by_handle(FILE *handle)
{
	return _parse_dbc_file_by_handle("<FILE*>", handle);
}

static mpc_ast_t *_parse_dbc_file_by_handle(const char *name, FILE *handle)
{
	mpc_ast_t *ast = NULL;
	char *istring = NULL;
	if(!(istring = slurp(handle)))
		goto end;
	ast = _parse_dbc_string(name, istring);
end:
	free(istring);
	return ast;
}

mpc_ast_t *parse_dbc_string(const char *string)
{
	return _parse_dbc_string("<string>", string);
}

enum cleanup_length_e
{
#define X(CVAR, NAME) _ignore_me_ ## CVAR,
	X_MACRO_PARSE_VARS
	CLEANUP_LENGTH
#undef X
};

static mpc_ast_t *_parse_dbc_string(const char *file_name, const char *string)
{
	#define X(CVAR, NAME) mpc_parser_t *CVAR = mpc_new((NAME));
	X_MACRO_PARSE_VARS
	#undef X

	/**@todo process more of the DBC format */
	#define X(CVAR, NAME) CVAR,
	mpc_err_t *language_error = mpca_lang(MPCA_LANG_WHITESPACE_SENSITIVE, dbc_grammar, X_MACRO_PARSE_VARS NULL);
	#undef X

	if (language_error != NULL) {
		mpc_err_print(language_error);
		mpc_err_delete(language_error);
		exit(EXIT_FAILURE);
	}

	mpc_result_t r;
	mpc_ast_t *ast = NULL;
	if (mpc_parse(file_name, string, dbc, &r)) {
		ast = r.output;
	} else {
		mpc_err_print(r.error);
		mpc_err_delete(r.error);
	}

#define X(CVAR, NAME) CVAR,
	mpc_cleanup(CLEANUP_LENGTH,
		X_MACRO_PARSE_VARS NULL
		);
#undef X
	return ast;
}

