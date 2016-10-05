#ifndef PARSE_H
#define PARSE_H

#include "mpc.h"
#include <stdio.h>

mpc_ast_t *parse_dbc_file_by_name(const char *name);
mpc_ast_t *parse_dbc_file_by_handle(FILE *handle);
mpc_ast_t *parse_dbc_string(const char *string);

#endif
