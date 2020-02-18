/*****************************************************************
 *
 * $Header: arplog.c,v 1.5 86/10/10 09:44:54 chriss Exp $
 *
 * file : arplog.c
 *
 * synopsis: transcribe all ARP traffic from a router
 *            *** Caution this uses a lot of resources ***
 *
 * modification history:
 *
 *  Oct-86 modified by Chriss Stephens (chriss) at Carnegie Mellon - moved
 *	printf indicating number of packets dropped from getelog to here.
 *
 * Feb 1986 by Matt Mathis at CMU
 *	
 */
#include <stdio.h>
#include <sys/types.h>
#include "./h/rtypes.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>

#include "../h/queue.h"
#include "../h/arp.h"
#include "./h/mch.h"		/* defines byte order for target machine */
#include "../h/ip.h"
#include "../h/rcp.h"
#include "../h/errorlog.h"
#include "../h/globalsw.h"
#include "./h/perror.h"			/* define error constants */

#define DELAY   1
#define NSIZE 14		/* The size of a generic network header */
#define NTYPE 12		/* The offset to its protocol type */
#define BCAST 1			/* broadcast flag */
#define PR_ARP 0x0806
#define PR_IP  0x0800

extern void showarp ();
extern int  errno;
intr();

struct glsw_tran logon={FCNF_ARPCPY,0};
struct glsw_tran logoff={0,0};
struct glsw_tran tmpsw;
struct sockaddr_in sin;		/* socket open to router under scrutiny */
char *router;
char password[100];
int verbose;


main(carg,varg)
int carg;
char *varg[];
{
    int delay=DELAY;
    int maxseq=0;
    int res;

    if (carg >1 && varg[1][0] == '-' && varg[1][1] == 'v'){
	verbose = 1;
	varg++;
	carg--;
    }

    if (carg < 2 || carg > 3) {
       printf("usage: arplog [-v] ?.?.?.? [delay]\n");
       exit(1);
    }
    if (set_rcp_sin (varg[1], &sin))
    {
	errno = NO_SOCKET;
	print_error("arplog",FATAL);
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
      print_error("arplog: rcp_get_glsw",FATAL);
/*  printf(" %4x %4x\n",tmpsw.fcnflags,tmpsw.dbgflags); */
    tmpsw.fcnflags = FCNF_ARPCPY;
    if (rcp_put_glsw(&sin,&tmpsw,password) == ERROR_EXIT)
      print_error("arplog: rcp_put_glsw",FATAL);

    while(1) {
	if ((res=getelog(&sin,maxseq,showarp)) == ERROR_EXIT)
          print_error("arplop",RECOVER);
        else
        {
          if ((res-ELISTLEN) > maxseq)
       	    printf("Lost %d entries\n",res-ELISTLEN-maxseq);
          maxseq = res;
	}
	if (delay) sleep(delay);
    }
}

/*****************************************************************
 *   Display some arp - discard all others
 */
void showarp(ei)
struct erroritem *ei;			/* The error under examination */
{
    u_short type;
    register struct addres *ar;
    struct hostent *hent;
    unsigned int a,b,c,d;	/* The bytes of the IP addr */

/* plogheader (ei); */
    if (ei -> p_off != NSIZE) {		/* only works for Enet type headers*/
	return;
    }
/* Discard all non-broadcasts */
    if (!(ei->p_flag & BCAST)) return;

    type = ntohs (*((u_short *) & (ei -> e_header[NTYPE])));
    if (type != PR_ARP) {
	return;
    }
    ar = (struct addres *) & (ei -> e_header[NSIZE]);

    printf( "Tick:%02d ",ei -> e_time%60);
    if (ei->p_flag & BCAST) {
	printf("Bcst ");
    }
    else {
	printf("PtoP ");
    }
    switch(ntohs(ar->ar_op)) {
	case AR_REQUEST:
	    printf("Req ");
	    break;
	case AR_REPLY:
	    printf("Rep ");
	    break;
	default:
	    printf(" ?????  ");
    }

    if (verbose && (hent = gethostbyaddr (ar_spa (ar), 4, AF_INET))) {
	c=((u_char *) ar_spa(ar))[2];
	d=((u_char *) ar_spa(ar))[3];
	printf ("%20s(%03u.%03u)", hent->h_name,c,d);
    }
    else {
	fprtpkt ("             $.$.$.$ ",ar_spa(ar));
    }
    if (verbose && (hent = gethostbyaddr (ar_tpa (ar), 4, AF_INET))) {
	c=((u_char *) ar_tpa(ar))[2];
	d=((u_char *) ar_tpa(ar))[3];
	printf ("%20s(%03u.%03u)\n", hent->h_name,c,d);
    }
    else {
	fprtpkt ("             $.$.$.$\n",ar_tpa(ar));
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
