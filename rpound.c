/*****************************************************************
 *
 * $Header: rpound.c,v 1.3 86/10/10 09:47:23 chriss Exp $
 *
 * file : rpound.c
 *
 * Pound on a Router
 * 	Does router stat's as fast as possible
 *	and complains if 1) the router does not respond (might be down)
 *			 2) Uptime decreases (it was rebooted)
 *
 * modification history:
 *
 *   Jun-86 Modified by Chriss Stephens (chriss) at Carnegie Mellon University
 *	to use version 1 of router software. Also uses new standardized error
 *	handling through customized perror.
 */

#include <stdio.h>
#include <sys/types.h>
#include "./h/rtypes.h"
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>

#include "../h/rcp.h"
#include "./h/perror.h"			/* define error constants */

extern int errno;

main(argc,argv)
int argc;
char *argv[];
{
    char    data[100];
    struct rcp_rstat   *rstat;
    struct sockaddr_in  sin;
    int uptimenow,uptimethen;
    int verbose;

    if (argc >1 && argv[1][0] == '-' && argv[1][1] == 'v'){
	verbose = 1;
	argv++;
	argc--;
    }
    if (argc < 2) {
	printf ("Usage: rpound [-v] <router>\n");
	exit (-1);
    }

    if (set_rcp_sin (argv[1], &sin) == ERROR_EXIT)
    {
	errno = NO_SOCKET;
	print_error ("rpound",FATAL);
    }
    while (1) {					/* do almost forever */
	if (V1_rcpsnatch (&sin, RINFO_XPING, data, 0) == ERROR_EXIT)
	{
	    print_error ("rpound",RECOVER);
	    fprintf (stderr,"router died\n");
	    exit(1);
/*	} else {
	    rstat = (struct rcp_rstat  *) data;
	    uptimenow = ntohl (rstat -> stat_timeup);
	    if (uptimenow < uptimethen) {
		printf("Router was rebooted\n");
	    }
	    uptimethen = uptimenow; */
	}
    }
}
