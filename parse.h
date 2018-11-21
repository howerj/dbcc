#ifndef PARSE_H
#define PARSE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mpc.h"
#include <stdio.h>

mpc_ast_t *parse_dbc_file_by_name(const char *name);
mpc_ast_t *parse_dbc_file_by_handle(FILE *handle);
mpc_ast_t *parse_dbc_string(const char *string);
const char *parse_get_grammar(void);

#ifdef __cplusplus
}
#endif

#endif
