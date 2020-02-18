/*
 * $Header: Dtr.c,v 1.2 87/11/29 22:50:22 djw Locked $
 *
 * file : Dtr.c
 *
 * synopsis:
 *	 router token ring (TMS380) diagnosis module
 *
 * modification history:
 *
 *   Wed Nov 11 1987 david waitzman (djw) at cmu
 *	created from Dil.c
 *
 *   Jul-86 Chriss Stephens (chriss) at Carnegie Mellon University modified
 *	to use new router organization (i.e. modified includes). Modified
 *	to return error code ERROR_EXIT and set perror variable in the
 *	event that diagnosis type in returned packet is not defined.
 *
 * david waitzman (djw@ampere.ece.cmu.edu) march '86
 *	created
 *
 *
 */

#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include "./h/rtypes.h"
#include <netinet/in.h>
#include "../h/rcp.h"
#include "./h/rcpinfo.h"
#include "./h/perror.h"			/* define error constants */
#include "./h/rdiag.h"

extern enum diag_oform_en outformat; 	/* from rdiag.c */
extern int errno;

/*
 * execute the appropriate diagnosis routine
 */
trdiag_main(diag)
struct rcp_diagfo *diag;
{
    switch (ntohs(diag->subtype)) {
      case TRDIAG_ERRS:
	trdiag_errs((u_long *)diag->data);
	break;
      default:
	errno = BAD_DIAG;
	return(ERROR_EXIT);
    }
}


/*
 * Array of the names of the various token ring error counting registers
 * This is array is in the same order & spacing as the array that
 * the TMS chips write out.
 */
#define RESERVED NULL	/* shows not defined in chip set documentation */

char *trregnames[14] = {
    "line",			/* 0 */
    RESERVED,			/* 1 */
    "burst",			/* 2 */
    "ari/fci",			/* 3 */
    RESERVED,			/* 4 */
    RESERVED,			/* 5 */
    "lost frame",		/* 6 */
    "receive congestion",	/* 7 */
    "frame copied",		/* 8 */
    RESERVED,			/* 9 */
    "token error",		/* 10 */
    RESERVED,			/* 11 */
    "dma bus",			/* 12 */
    "dma parity"		/* 13 */
};

/*
 * Dump out the error log data
 */
trdiag_errs(data)
u_long *data;
{
    int i, lc;
    char buf[200];

    /*
     * The following line is needed because of a BUG in leaving the device
     * type (a short == 0x3B == 0113 == 75 == DVT_TR) in front of the data.
     * This is due to unknown causes
     */
    data = (u_long *)((u_char *)data + 2);

    switch (outformat) {
      case of_human_verbose_ev:
	for (i = 0; i < 14; i++)
	    if (trregnames[i]) {
		printf("%25s errors: %6d, pegs: %6d\n",
			trregnames[i], ntohl(data[i]), ntohl(data[i+14]));
	    }
	break;
      case of_human_terse_ev:
      case of_machine_ev:
	for (i = 0; i < 28; i++)
	    printf("%d ", ntohl(data[i]));
	putchar('\n');
	break;
    }
}
