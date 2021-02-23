/* From: * <https://stackoverflow.com/questions/10404448/getopt-h-compiling-linux-c-code-in-windows> */
#include <string.h>
#include <stdio.h>
#include "options.h"

int dbcc_opterr = 1, /**< if error message should be printed */
    dbcc_optind = 1, /**< index into parent argv vector */
    dbcc_optopt,     /**< character checked for validity */
    dbcc_optreset;   /**< reset getopt */
char *dbcc_optarg;   /**< argument associated with option */

#define BADCH   ((int)'?')
#define BADARG  ((int)':')
#define EMSG    ("")

/* getopt -- Parse argc/argv argument vector. */
int dbcc_getopt(int nargc, char *const nargv[], const char *ostr)
{
	static char *place = EMSG; /* option letter processing */
	const char *oli;           /* option letter list index */

	if (dbcc_optreset || !*place) { /* update scanning pointer */
		dbcc_optreset = 0;
		if (dbcc_optind >= nargc || *(place = nargv[dbcc_optind]) != '-') {
			place = EMSG;
			return -1;
		}
		if (place[1] && *++place == '-') { /* found "--" */
			++dbcc_optind;
			place = EMSG;
			return -1;
		}
	}
	if ((dbcc_optopt = (int)*place++) == (int)':' || !(oli = strchr(ostr, dbcc_optopt))) { /* option letter okay? */
		/* if the user didn't specify '-' as an option, assume it means -1. */
		if (dbcc_optopt == (int)'-')
			return (-1);
		if (!*place)
			++dbcc_optind;
		if (dbcc_opterr && *ostr != ':')
			(void)printf("illegal option -- %c\n", dbcc_optopt);
		return BADCH;
	}
	if (*++oli != ':') { /* don't need argument */
		dbcc_optarg = NULL;
		if (!*place)
			++dbcc_optind;
	} else {                /* need an argument */
		if (*place)     /* no white space */
			dbcc_optarg = place;
		else if (nargc <= ++dbcc_optind) { /* no arg */
			place = EMSG;
			if (*ostr == ':')
				return (BADARG);
			if (dbcc_opterr)
				(void) printf ("option requires an argument -- %c\n", dbcc_optopt);
			return BADCH;
		} else { /* white space */
			dbcc_optarg = nargv[dbcc_optind];
		}
		place = EMSG;
		++dbcc_optind;
	}
	return dbcc_optopt; /* dump back option letter */
}

