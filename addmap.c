/*
 * $Header: addmap.c,v 1.5 86/10/10 09:44:34 chriss Exp $
 *
 * file: addmap.c
 *
 * Router ARP cache extraction and manipulation tools
 *
 * Modification History
 *
 *  Jun-86 modified by Chriss Stephens (chriss) at Carnegie Mellon University.
 *	Added support for version 1 server. In particular, compatible with the
 *	new diagfo header, and generates version 1 rcp requests.
 *	Added consistent error handling and treatment of sockets. The
 *	calling routine must pass in the socket to the router.
 *
 *  10/8/85 revised by Matt Mathis
 * 
 *     8/85 created by Kevin Kirmse
 */

#include <sys/types.h>
#include "./h/rtypes.h"
#include <sys/socket.h>
#include <sys/file.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>

#include "../h/queue.h"
#include "../h/rcp.h"
#include "./h/hash.h"
#include "../h/arp.h"
#include "../h/arpquery.h"
#include "./h/perror.h"		/* error constant definitions */

#include "./debug/addmap.h"


#define RETRIES 10
#define MBUFSIZ 1500
static char main_xbuff[MBUFSIZ];
static char main_rbuff[MBUFSIZ];
extern int errno;

/*****************************************************************
 *	Display a routers ARP cache (address map)
 *
 */

rcp_addmap(flag,rsocket)

         int            flag;
  struct socketaddr_in *rsocket;   /* socket open to router under scrutiny */

{
    BUCK_TYPE *addmap[NADDMAP];

    if (getarpcache(addmap,rsocket) == ERROR_EXIT) 
    {
      errno = DEAD_ROUTER;
      return(ERROR_EXIT);
    }   
    sethostent(1);
    disparpq(stdout,addmap,flag);
    return(SUCESSFULL);
}

/*****************************************************************
 *	Get the ARP cache
 *
 */

getarpcache(head,rsocket)

  BUCK_TYPE           **head;	   /* Hash table to put results */
  struct socketaddr_in *rsocket;   /* socket open to router under scrutiny */

{
    int     n,
            mempos,
            numres,
            num;
    struct rcp *rcp_out;
    struct rcp *rcp_in;
    struct arpinfo *artemp,
                   *ar;

    rcp_out = (struct rcp *) & (main_xbuff[0]);
    rcp_in  = (struct rcp *) &(main_rbuff[0]);
    for (num = 0; num < NADDMAP; num++) {

#ifdef ADDMDEBUG
       printf("\nGetting bucket %d\n",num);
#endif

	head[num] = NULL;
	rcp_out -> rc_type = htons (V1_RCT_INFO);
	rcp_out -> rc_diagfo.u_di.bucket = htons (num);
	rcp_out -> rc_diagfo.subtype = htons (RINFO_DGET);
	if ((n = rcptrans (rsocket, rcp_out, rcp_in, RETRIES,
			   sizeof (struct rcp), MBUFSIZ)) < 0)
          return (ERROR_EXIT);
        if (ntohs(rcp_in->rc_diagfo.u_di.bucket) == ERROR_EXIT)	
	{
	  errno = BUCKET_FULL;
	  print_error("addmap",RECOVER);
	}
    /* dispmem(rcp_in,rcp_in,n); */
	mempos = 0;
	numres = ntohs ((u_short) rcp_in -> rc_diagfo.length);
	numres = (numres / sizeof(struct arpinfo));
	
#ifdef ADDMDEBUG
	printf("address resolutions : %d \n",numres);
#endif

	while ((mempos + sizeof(struct rcp) < n) && numres--)
        {
	    artemp = (struct arpinfo *) &(rcp_in->rc_diagfo.data[mempos]);
	    ar=(struct arpinfo *)(char *)malloc(sizeof(struct arpinfo));

	    ar -> ari_hrd = ntohs (artemp -> ari_hrd);
	    ar -> ari_pro = ntohs (artemp -> ari_pro);
	    ar -> ari_hln = artemp -> ari_hln;
	    ar -> ari_pln = artemp -> ari_pln;
	    ar -> ari_flag = ntohs(artemp -> ari_flag);
	    ar -> ari_cable = ntohl(artemp -> ari_cable);
	    bcopy(artemp->ari_harda, ar->ari_harda, AMPHYSIZE+AMPROSIZE);
	    (void)(char *) addnode (head, ar->ari_prota,
			            ar->ari_pln, (char *)ar);
	    mempos += sizeof (struct arpinfo);
	}
    }
    return (SUCESSFULL);
}

/*****************************************************************
 * Display the entire arp cache
 *
 */
static disparpq(fp,head,flag)

   FILE      *fp;		/* file for output */
   BUCK_TYPE *head[];		/* the cache to be shown */
   int        flag;		/* look up host names for IP addrs? */

{
    int        cnt,numres = 0;
    BUCK_TYPE *temp;

    for(cnt=0;cnt < NADDMAP;cnt++)
    {
       temp = head[cnt];
       while(temp)
       {
            numres++;
            dispaddres(fp,(struct arpinfo *)temp->data,flag);
            temp = temp->next;
       }
    }
    fprintf(fp,"\n(numres = %d)\n",numres);
}

/*****************************************************************
 * Display one ARP entry
 *
 */

static dispaddres(fp,ar,flag)

   FILE           *fp;		/* file for output */
   struct arpinfo *ar;		/* the cache entry */
   int             flag;	/* show names ? */

{
             int      cnt;
    unsigned int      temp;
    struct   hostent *hent;

#ifdef ADDMDEBUG
    fprintf(fp,"[ ar_hrd = %x ",(unsigned ) ar->ari_hrd); 
    fprintf(fp,"/ ar_pro = %x ",(unsigned ) ar->ari_pro); 
    fprintf(fp,"/ ar_pln = %x ",(unsigned ) ar->ari_pln); 
    fprintf(fp,"/ ar_hln = %x ",(unsigned ) ar->ari_hln); 
    fprintf(fp,"]\n");
#endif

    fprintf(fp,"[%2x] ",ar->ari_cable);		/* card number*/
    fprintf(fp," [");				/* Flags */
    if (ar->ari_flag&AM_PERM)
       printf("P");
    else 
       printf(" ");
    if (ar->ari_flag&AM_REQBCAST)
       printf("B");
    else 
       printf(" ");
    if (ar->ari_flag&AM_INLRUQ)
       printf("L");
    else 
       printf(" ");
    if (ar->ari_flag&AM_XMITPP)
       printf("X");
    else 
       printf(" ");
    fprintf(fp,"]");
    fprintf(fp," [");				/* protocol address */
    for(cnt = 0;cnt < ar->ari_pln;cnt ++) {
       temp = (unsigned char) ar->ari_prota[cnt];
       fprintf(fp,"%d",temp);
       if (cnt != (ar->ari_pln-1))
           fprintf(fp,".");
    }
    fprintf(fp,"]  \t=> [");			/* physical address */
     for(cnt = 0;cnt < ar->ari_hln;cnt ++) {
       temp = (unsigned char) ar->ari_harda[cnt];
       fprintf(fp,"%x",temp);
       if (cnt != (ar->ari_hln-1))
           fprintf(fp,".");
    }
    fprintf(fp,"] ");				/* Host name */
    if (flag) {	
       if (hent = gethostbyaddr(ar->ari_prota,4,AF_INET)) {
          fprintf(fp,"\t%s\n",hent->h_name);
       }
       else {
          fprintf(fp,"\t***Unknown***\n");
       }
    }
    else
       fprintf(fp,"\n");
}

