#ifndef _2BSM_H
#define _2BSM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "can.h"

int dbc2bsm(dbc_t *dbc, FILE *output, bool use_time_stamps);

#ifdef __cplusplus
}
#endif

#endif
