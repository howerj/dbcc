#ifndef OPTIONS_H
#define OPTIONS_H

extern int dbcc_opterr,   /**< if error message should be printed */
    dbcc_optind,          /**< index into parent argv vector */
    dbcc_optopt,          /**< character checked for validity */
    dbcc_optreset;        /**< reset getopt */
extern char *dbcc_optarg; /**< argument associated with option */

int dbcc_getopt(int nargc, char *const nargv[], const char *ostr);

#endif
