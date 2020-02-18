/*****************************************************************
 *
 * $Header: ralarm.c,v 1.3 86/10/10 09:46:06 chriss Exp $
 *
 * file: ralarm.c
 *
 * synopsis: router failure alarm
 *
 * 	Does router stat's every CTIME (10 minutes) on a list of routers
 *	and complains if 1) the router does not respond (might be down)
 *			 2) Uptime decreases (it was rebooted)
 *			 3) The number of free packets falls by 2
 *			    (A device may be wedged or overloaded)
 *
 * modification history:
 *
 *  Jul-86 Chriss Stephens (chriss) at Carnegie Mellon University modified
 *	to be compatible with new perror, and to grab socket at this level
 *	rather than at rstat level.
 *
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
#include "./h/perror.h"		/* define error constants */


#define MAXRTE 20	/* maximum number of routers */
#define CTIME 600	/* number of seconds for each testing cycle */
#define MAXLOSS 2	/* change in freelist size threshold */

extern struct rcp_rstat *rstat();
extern int errno;

main(argc,argv)
int argc;
char *argv[];
{
  struct sockaddr_in  sin;
  struct rcp_rstat   *st;
         int          rte,
		      uptimes[MAXRTE],
		      fpkts[MAXRTE],
		      verbose = 0;
         unsigned     delay;

    if (argc >1 && argv[1][0] == '-' && argv[1][1] == 'v')
    {
	verbose = 1;
	argv++;
	argc--;
    }
    if (argc < 2) 
    {
	printf ("Suggested usage: ralarm [-v] <router> <router2> .. &\n");
	exit (-1);
    }
    for (rte = 1; rte < argc; rte++)
    {
	if (set_rcp_sin(argv[rte], &sin) == ERROR_EXIT)
	{
	   errno = NO_SOCKET;
	   print_error("ralarm",RECOVER);
	}
	if ((st = rstat(&sin,verbose,argv[rte]))
	     == (struct rcp_rstat *)ERROR_EXIT)
        {
	    if (verbose) print_error("ralarm",RECOVER);
	    printf ("ralarm: router %s is not responding\n",argv[rte]);
	    uptimes[rte]=0;
	    fpkts[rte]=0;
	}
	else
	{
	  uptimes[rte]=st->stat_timeup;
	  fpkts[rte]=st->stat_frpac;
	}
    }

    delay = CTIME / (argc-1);

  while (1) 
  {
    for (rte = 1; rte < argc; rte++) 
    { 
	sleep (delay);
	if (set_rcp_sin(argv[rte], &sin) == ERROR_EXIT)
	{
	   errno = NO_SOCKET;
	   print_error("ralarm",RECOVER);
	}
	if ((st = rstat(&sin,verbose,argv[rte]))
	     == (struct rcp_rstat *)ERROR_EXIT)
	{
	  if (verbose) print_error("ralarm",RECOVER);
	  if (printf ("ralarm: router %s not responding\n", argv[rte])) 
	    exit (1);
	}
	else 
	{
	  if (uptimes[rte] > st->stat_timeup)
	  {
	    if (printf ("Router %s was rebooted\n", argv[rte]))
	       exit (1);
	    fpkts[rte]= st->stat_frpac;
	  }
	  else 
	  {
            if ((fpkts[rte] - st->stat_frpac) > MAXLOSS)
	    {
	      if (printf ("Router %s:hung interface\n", argv[rte]))
		exit (1);
	    }
	  }
	    if (fpkts[rte] < st->stat_frpac) fpkts[rte] = st->stat_frpac;
	    uptimes[rte] =  st->stat_timeup;
	} 
    }   
}
}
