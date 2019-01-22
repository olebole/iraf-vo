/**
 *  VOPARAMS.C -- Interface to manage cmdline options or library parameters.
 *
 *  @file       voParams.c
 *  @author     Mike Fitzpatrick
 *  @date       7/03/12
 *
 *  @brief      Interface to manage cmdline options or library parameters.
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include "voApps.h"
#include "voAppsP.h"


#define	MAXARGS		256
#define	SZ_ARG		128



static  int last_good_index = 0;

/**
 *  VO_PARAMINIT -- Initialize the task parameter vector.
 *
 *  @brief      Initialize the task parameter vector.
 *  @fn         char **vo_paramInit (int argc, char *argv[])
 *
 *  @param  argc        argument count
 *  @param  argv        argument vector
 *  @returns            modified argument vector
 */
char **
vo_paramInit (int argc, char *argv[], char *opts, struct option long_opts[])
{
    static  char *pargv[MAXARGS], arg[SZ_ARG];
    int  i, j, k, len = 0;

    memset (&pargv[0], 0, MAXARGS);
    last_good_index = 0;

    for (i=0; i < argc; i++) {
	/*  Make a local copy of the arg so we can modify it without side
 	 *  effects.
	 */
	memset (arg, 0, SZ_ARG);
	strcpy (arg, argv[i]);
	len = strlen (arg);

	if (arg[0] != '-') {
	    pargv[i] = calloc (1, strlen (arg) + 6);
	    if (strchr (argv[i], (int) '='))
	        sprintf (pargv[i], "--%s", arg);
	    else if (argv[i][len-1] == '+') {
		arg[len-1] = '\0';
	        sprintf (pargv[i], "--%s=1", arg);
	    } else if (argv[i][len-1] == '-') {
		arg[len-1] = '\0';
	        sprintf (pargv[i], "--%s=0", arg);
	    } else
	        sprintf (pargv[i], "%s", arg);

	} else {
	    if (arg[0] == '-' && arg[1] != '-') {
		if (arg[2] == '=') {
		    /*  Argument is of the form '-f=bar' instead of the form
		     *  "--foo=bar" or "--foo bar" or "foo=bar".  We need to 
		     *  rewrite it to be acceptable to getopt_long().
		     */
		    char new[SZ_ARG];

		    memset (new, 0, SZ_ARG);
		    for (j=0; (char *)long_opts[j].name; j++) {
			if ((int) long_opts[j].val == (int) arg[1]) {
			    sprintf (new, "--%s=%s",long_opts[j].name, &arg[3]);
			    memset (arg, 0, SZ_ARG);
			    strcpy (arg, new);
			    len = strlen (arg);
			}
		    }

	        } else if (arg[2] != '=' && strchr (arg, (int)'=')) {
		    fprintf (stderr, "Illegal flag '%s', skipping.\n", arg);
		    continue;

	        } else {
		    /*  Check for a flag of the form "-all", which should
		     *  really be the long-form of "--all".  Rewrite the
		     *  pargv value so the flag isn't interpreted incorrectly
		     *  as "-a", "-l", "-l".
		     */
		    int found = 0;
		    for (k=0; (char *)long_opts[k].name; k++) {
			if (strcmp (long_opts[k].name, &arg[1]) == 0) {
	    		    pargv[i] = calloc (1, strlen (arg) + 6);
	        	    sprintf (pargv[i], "--%s", &arg[1]);
			    found = 1;
			    break;
			}
		    }
		    if (found) continue;
	        }
	    }

	    pargv[i] = calloc (1, strlen (arg) + 1);
	    sprintf (pargv[i], "%s", arg);
	}
    }

    if (PARAM_DBG) {
    for (i=0; i < argc; i++) 
	    fprintf (stderr, "pargv[%d] = '%s'\n", i, pargv[i]);
    }

    return (pargv);
}


/**
 *  VO_PARAMNEXT -- Get the next parameter value.
 *
 *  @brief      Get the next parameter value.
 *  @fn         int vo_paramNext (char *opts, struct option long_opts[],
 *			int argc, char *argv[], char *optval, int *posindex)
 *
 *  @param  opts        option string
 *  @param  long_opts   long options struct
 *  @param  argc        argument count
 *  @param  argv        argument vector
 *  @param  optval      optional parameter argument
 *  @param  posindex    positional parameter index (0-based)
 *  @returns            nothing
 */
int
vo_paramNext (char *opts, struct option long_opts[], int argc, char *argv[],
		char *optval, int *posindex)
{
    int  ch = 0, index;
    static  int pos = 0, apos = 0;


    apos++;
    memset (optval, 0, SZ_FNAME);
#ifdef USE_GETOPT_LONG
    ch = getopt_long (argc, argv, opts, long_opts, &index);
#else
    ch = getopt_long_only (argc, argv, opts, long_opts, &index);
#endif

    if (ch == 63) {
	optind = last_good_index;
	return (-1);
    }

    if (ch >= 0) {
        if (ch > 0 && optarg) {
	    if ((strchr (optarg, (int)'=') != 0) && (optarg[0] != '-') && 
	        (argv[apos][0] == '-' && argv[apos][1] != '-') ) {
		    fprintf (stderr, 
			"Error: invalid argument = '%s' in vot_paramNext()\n",
		        argv[apos]);
		    return (PARG_ERR);
	    } else {
		if (optarg[0] == '-') {
		    // optind--;
		    memset (optval, 0, SZ_FNAME);
		} else
	            strcpy (optval, optarg);
	    }

	} else if (ch == 0) {
	    *posindex = index;
	    if (optarg)
	        strcpy (optval, optarg);
	} else {
	}

    } else {
	if (argv[optind+pos]) {
	    strcpy (optval, argv[optind+pos]);
	    *posindex = pos++;
	    return (-pos);
	} else
	    return (0);
    }
    last_good_index = optind;

    if (PARAM_DBG) {
	fprintf (stderr, 
	    "paramNext: ch=%d (%c) optval='%s' optarg='%s' index=%d\n",
      ch, ch, optval, optarg, index);
    }

    return (ch);
}


/**
 *  VO_PARAMFREE -- Free the allocated parameter vector.
 *
 *  @brief      Free the allocated parameter vector.
 *  @fn         void vo_paramFree (int argc, char *argv[])
 *
 *  @param  argc        argument count
 *  @param  argv        argument vector
 *  @returns            nothing
 */
void 
vo_paramFree (int argc, char *argv[])
{
    register int i;

    for (i=0; i < argc; i++) {
	if (argv[i] && argv[i][0])
	    free ((void *)argv[i]);
    }
}
