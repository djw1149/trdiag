/*****************************************************************
 *
 * $Header: rinfo.c,v 1.3 86/10/10 09:47:04 chriss Exp $
 *
 * file: rinfo.c
 *
 * Router information
 *	A general user interface to router information tools
 *	including device statistics, ARP cache and reboot commands
 *	See the man page for details
 *
 * HISTORY
 *
 *	  Jun-86 Modified by Chriss Stephens (chriss) at Carnegie Mellon 
 *	    University to use version 1 of client software and integrated
 *	    subheaded, diagfo, for both information and diagnositc requests.
 *
 * 	10/14/85 Created by Matt Mathis
 */


#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "./h/rtypes.h"
#include "../h/rcp.h"
#include "./h/rcpinfo.h"
#include "./h/perror.h"			/* define error constants */
#include "../h/proto.h"
#include "../h/devstats.h"

#define DELAY 10

extern int errno;

/*
 *
 * Display device statistics in the selected format
 *
 */

dispinfo(router,rdev,informat,card,detail,verbose,first)
    char *router;		/* The router's name */
    struct rcpdevice *rdev;	/* The statistics to be shown */
    char informat;		/* display format switch */
    int detail;			/* give per interface breakdowns */
    int verbose;		/* give long form  */
    int first;			/* display title banner */
{
    int     c;
    static int  lines;

/* The first group of information services are 'global' and do not use card or
	detail switches */
    switch (informat) {
	case 'D': 
	    return (rcp_devdump (router, c, rdev, verbose, first));
    }
/* The remaining commands display the statisitcs per requested card */
    if (first)
	lines = 25;
    if (first = (lines > 20))
	lines = 0;
    c = 0;
    do {
	if (++c > numdev)	/* sequence is 1,2,3..numdev,0 */
	    c = 0;

	if (c == card || detail) {
	    switch (informat) {
		case 'a': 
		    rcp_arcnt (router, c, rdev, verbose, first);
		    break;
		case 'd': 
		    rcp_drinfo (router, c, rdev, verbose, first);
		    break;
		case 'i': 
		    rcp_ipcnt (router, c, rdev, verbose, first);
		    break;
		case 't': 
		    rcp_trcnt (router, c, rdev, verbose, first);
		    break;
		default: 
		    printf ("Unknown format, use:  -T{abdDimstu}\n");
		    exit (-1);
	    }
	    first = 0;
	    lines++;
	}
    } while (c != 0);
    if (detail) {
	printf ("\n");
	lines++;
    }
}

readstat() {};
writestat() {};


main(argc,argv)
  
  int   argc;
  char *argv[];

{
    char    pswd[20],
            bstr[200],
            getstat[100],
            putstat[100],
	    router[30];
    int     bstrflag = 0;
    int     card = 0;
    int     detail = 0;
    int     total = 0;
    int     once = 0;
    int     verbose = 0;
    int     itype;
    int     first;
    struct devinfo      past[MAXPORT + 1],
                        curr[MAXPORT + 1],
                        delta[MAXPORT + 1];
    struct sockaddr_in sin;

    itype = getritype (argc, argv);
    initswitch (argc, argv, router,
	        0, bstr, &bstrflag,	/* boot */
	        &card, &detail,	/* cards */
	        &total, &once, getstat, putstat,/* time */
	        &verbose);		/* format */

    if (set_rcp_sin (router, &sin)<0)
    {
	errno = NO_SOCKET;
	print_error("rinfo",FATAL);
    }
 
/* process non-device information commands here */
    switch (itype) {
	case 'b': 		/* reboot */
	    switch (bstrflag) {
		case 0: 
		   printf("REBOOT FROM DEFAULT\n");
		   initswitch (argc, argv, 0, pswd, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		   if (rcpboot (&sin, "", pswd))
		     print_error("rinfo",FATAL);
		   break;
		case -1: 
		   printf("HALT and exit to the monitor\n");
		   printf("*** STOP! **** This requires manual intervention to reset\n");
		   pswd[0]=0;
		   initswitch (argc, argv, 0, pswd, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		   if (rcpboot (&sin, 0, pswd))
		     print_error("rinfo",FATAL);
		   break;
		case 1: 
	           printf("REBOOT FROM \"%s\"\n",bstr);
	           initswitch (argc, argv, 0, pswd, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		   if (rcpboot (&sin, bstr, pswd))
		     print_error("rinfo",FATAL);
		   break;
	    }
	    sleep (2);
	    if (rstat (&sin,1,router) == ERROR_EXIT)
	      print_error("rinfo",FATAL);
	    return(SUCESSFULL);
	case 'm': 		/* show arp table */
	    return (rcp_addmap (verbose,&sin));
	case 's': 		/* show uptime and version */
	    if (rstat (&sin,1,router) == ERROR_EXIT)
	      print_error("rinfo",FATAL);
	    return(SUCESSFULL);
	case 'D': 		/* dump all counts */
	case 'd': 		/* network driver counts */
	case 'a': 		/* ARP counts */
	case 'i': 		/* IP counts */
	case 't': 		/* traffic summary counts */
	    break;
	default: 
	    printf ("Unknown command, use: -T{abdDimstu}\n");
	    exit (-1);
    }

    if ((numdev = rcpdevlist (&sin, past)) == ERROR_EXIT)
      print_error("rinfo",FATAL);
    if (getstat[0]) {
	readstat (getstat, past);
    }
    else {
	if (!once && !total) {
	    first = 1;
	    printf ("Events per 10 seconds:\n");
	    while (1) {
		sleep (DELAY);
	        if (rcpdevlist (&sin, curr) == ERROR_EXIT)
		  print_error("rinfo",FATAL);
		devdelta (curr, past, delta, numdev);
		dispinfo (router,delta, itype, card, detail, verbose, first);
		first = 0;
	    }
	}
    }

    if (total) {
	dispinfo (router,past, itype, card, detail, verbose, 1);
    }
    else {			/* assume once */
	if (!getstat[0]) {
	    printf ("Events per 10 seconds:\n");
	    sleep (DELAY);
	}
	if (rcpdevlist (&sin, curr) == ERROR_EXIT)
	  print_error("rinfo",FATAL);
	devdelta (curr, past, delta, numdev);
	dispinfo (router,delta, itype, card, detail, verbose, 1);
    }
    if (putstat[0])
	writestat (putstat, past);

}
