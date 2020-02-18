/*****************************************************************
 *
 * $Header: iplog.c,v 1.5 86/10/10 09:45:52 chriss Exp $
 *
 * file : iplog.c
 *
 * synopsis: transcribe all IP traffic from a router
 *             *** Caution this	uses a lot of resources ***
 *
 * modification history:
 *
 *  Oct-86 modified by Chriss Stephens (chriss) at Carnegie Mellon - moved
 *     printf indicating number of packets dropped from getelog routine
 *     to here
 *
 * Feb 1986 by Matt Mathis at CMU
 *	
 */

#include <stdio.h>
#include <sys/types.h>
#include "./h/rtypes.h"
#include <netinet/in.h>
#include <signal.h>

#include "../h/queue.h"
#include "./h/mch.h"		/* define byte order for target machine */
#include "../h/ip.h"
#include "../h/rcp.h"
#include "../h/errorlog.h"
#include "../h/globalsw.h"
#include "./h/perror.h"		/* define error constants */

#define DELAY   1

extern void showerror ();
extern int errno;

intr();

struct glsw_tran tmpsw;
struct sockaddr_in sin;		/* socket open to router under scrutiny */
char *router;
char password[100];


main(carg,varg)
int carg;
char *varg[];
{
    int delay=DELAY;
    int maxseq=0;
    int res;

    if (carg < 2 || carg > 3) {
       printf("usage %s ?.?.?.? [delay]\n",varg[0]);
       exit(1);
    }
    if (set_rcp_sin (varg[1], &sin))
    {
	errno = NO_SOCKET;
	print_error("iplog",FATAL);
    }
    router=varg[1];
    if (carg == 3)
       if ((delay = atoi(varg[2])) <= 0)
          delay = DELAY;
    {
	char *temppswd = (char *) getpass("Enter password: ");
	strcpy(password, temppswd);
    }
    signal(SIGINT, intr);
    if (rcp_get_glsw(&sin,&tmpsw) == ERROR_EXIT)
      print_error("iplog",FATAL);
/*  printf(" %4x %4x\n",tmpsw.fcnflags,tmpsw.dbgflags); */
    tmpsw.fcnflags = FCNF_IPCPY;
    if (rcp_put_glsw(&sin,&tmpsw,password) == ERROR_EXIT)
      print_error("iplog",FATAL);

    while(1)
    {
	/* res+1==discard my own */
	if ((res=getelog(&sin,maxseq,showerror)) == ERROR_EXIT)
	  print_error("iplog",RECOVER);
	else
        {
          if ((res-ELISTLEN) > maxseq)
            printf("Lost %d entries\n",res-ELISTLEN-maxseq);
	  maxseq=res+1;
	}
        if (delay) sleep(delay);
    }
}

intr()
{
	printf("\nWait for switchs to reset:\n");
	rcp_get_glsw(&sin,&tmpsw);
/*	printf(" %4x %4x\n",tmpsw.fcnflags,tmpsw.dbgflags); */
	tmpsw.fcnflags = 0x0;			/* reset switches */
	rcp_put_glsw(&sin,&tmpsw,password);
	printf("Done\n");
	exit(0);
}
