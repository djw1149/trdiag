/*****************************************************************
 *
 * $Header: errorlog.c,v 1.4 86/10/10 09:45:30 chriss Exp $
 *
 * file: errorlog.c
 *
 * sysnopsis: transcribe the errorlog from an rcp controlled router
 *
 * modification history:
 *
 *  Oct-86 modified by Chriss Stephens (chriss) at Carnegie Mellon - moved
 *	printf indicating number of packets lost from getelog routine to here.
 *
 *  Jun-86 modified by Chriss Stephens (chriss) at Carnegie Mellon University
 *	to support new diagfo header and new version of rcpsnatch. This
 *	software makes version 1 rcp requests.
 *
 * Dec 1985 by Matt Mathis at CMU
 *	Rewritten
 * Summer 1985 by Kevin Kirmse
 *	Created
 */

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "./h/rtypes.h"
#include "../h/queue.h"
#include "./h/mch.h"		/* define byte order for target machine */
#include "../h/ip.h"
#include "../h/rcp.h"
#include "../h/errorlog.h"
#include "./h/perror.h"		/* define error constants */

#define DELAY   5

extern void showerror ();
extern int errno;

main(carg,varg)

  int   carg;
  char *varg[];

{
    struct sockaddr_in sin;
           int         delay=DELAY;
           int         maxseq=0;
           int         res;

    if (carg < 2 || carg > 3) {
       printf("usage %s ?.?.?.? [delay]\n",varg[0]);
       exit(1);
    }
    if (set_rcp_sin (varg[1], &sin))
    {
	errno = NO_SOCKET;
	print_error("errorlog",FATAL);
    }
    if (carg == 3)
       if ((delay = atoi(varg[2])) <= 0)
          delay = DELAY;

    while(1) {
      printf("**********\n");
      if ((res=getelog(&sin,maxseq,showerror)) == ERROR_EXIT)
         print_error ("errorlog",RECOVER);
      else
      {
        if (res-ELISTLEN > maxseq)
	  printf("Lost %d entries\n",res-ELISTLEN-maxseq);
        maxseq = res;
      }
      sleep(delay);
    }
}
