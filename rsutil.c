/*****************************************************************
 *
 * $Header: rsutil.c,v 1.2 86/07/24 12:40:43 chriss Exp $
 *
 * file: rsutil.c
 *
 * Router status utilities
 *
 * HISTORY
 *
 *    Jun-86 Modified by Chriss Stephens (chriss) at Carnegie Mellon
 *	University. Modified to use version 1 of router software, and
 *      now understands new diagfo packet format. Getdevicestats error
 *	exits with an SNATCH_ERROR indicating that last card has been
 *	found.
 *
 *
 * 10/7/85  started by Matt Mathis
 *	Several cosmetic, organizational changes and linted
 *
 * Written July 1985 Kevin Kirmse
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

#include "../h/proto.h"
#include "../h/rcp.h"		/* BUG move to rcp.c */
#include "./h/rcpinfo.h"
#include "../h/devstats.h"
#include "./h/perror.h"		/* define error constants */

#define RETRIES 10
#define dup_struct(f,s,t,l) {(s)=(t *)calloc(1,l);bcopy((f),(s),(l));}

#define MBUFSIZ 1500
extern char *rcpsplit();
extern int errno;
char *timeup();

/*****************************************************************
 *	Router status
 *		get some generic health indicators:
 *			uptime, through packets etc
 */

struct rcp_rstat * rstat(sin,pflag,rname)

  struct socketaddr_in  *sin;		/* socket opened to the router */
         int             pflag;		/* Print flag */
	 char	        *rname;		/* name of router */

{
    static               char  data[100];
    static struct   rcp_rstat *rs;


    if (V1_rcpsnatch (sin, RINFO_UPTM, data, 0) == ERROR_EXIT)
	return((struct rcp_rstat *)ERROR_EXIT);
    rs = (struct rcp_rstat *) data;
    rs -> stat_timeup = ntohl (rs -> stat_timeup);
    rs -> stat_vrecv = ntohl (rs -> stat_vrecv);
    rs -> stat_vtran = ntohl (rs -> stat_vtran);
    rs -> stat_frpac = ntohs (rs -> stat_frpac);
    rs -> stat_frmem = ntohl (rs -> stat_frmem);
    rs -> stat_mxmem = ntohl (rs -> stat_mxmem);
    rs -> versmajor = ntohs (rs -> versmajor);
    rs -> versminor = ntohs (rs -> versminor);
    rs -> versedit = ntohs (rs -> versedit);
    if (pflag) {
	printf ("[%s]", rname);
	printf ("\t[");
	printf (" %s ", timeup (rs -> stat_timeup));
	printf ("Free Packets=%d, ", rs -> stat_frpac);
	printf ("%d/%d bytes free=", rs -> stat_frmem, rs -> stat_mxmem);
	printf ("%d%% ]\n", rs -> stat_frmem * 100 / rs -> stat_mxmem);
	printf ("\t[ ");
	printf ("Valid IP recv/trans %d/%d, ", rs->stat_vrecv, rs->stat_vtran);
	printf ("V%d.%d(%d) ", rs -> versmajor, rs->versminor, rs->versedit);
	printf ("%s ]\n", rs -> versdate);
    }
    return(rs);
}

/*****************************************************************
 *	Format and print uptime
 *
 */
char *timeup(now)
long now;
{
    int hours, mins, secs, days;
    static char str[60];

    hours = now/3600;
    mins = (now/60)%60;
    secs = (now%60);
    days = hours/24;
    hours = hours%24;
    (void) (char *)sprintf(str, "%2d+%02d:%02d:%02d", days, hours, mins, secs);
    return(str);
}

/*****************************************************************
 *	Get device statistics
 * 		Get all of the statistics (mostly event counters) associated
 *		with the selected router and interface.
 */

getdevicestats(sin,card,rdev)

   struct sockaddr_in  *sin;		/* socket to the router */
          int		card;		/* the card */
   struct devinfo      *rdev;		/* the resultant statistics */

{
             char       data[MBUFSIZ];
    struct   devinfo   *lrdev;
    register int        c;

    if (V1_rcpsnatch (sin, RINFO_DEVH, data, card) == ERROR_EXIT)
	return(ERROR_EXIT);

    lrdev = (struct devinfo *) data;
    
    rdev -> dvc_br = ntohl (lrdev -> dvc_br);
    rdev -> dvc_nunit = ntohl (lrdev -> dvc_nunit);
    rdev -> dvc_istatus = ntohl (lrdev -> dvc_istatus);
    rdev -> dvc_dstate = ntohl (lrdev -> dvc_dstate);
    rdev -> dvc_cable = ntohl (lrdev -> dvc_cable);
    rdev -> dvc_devtype = ntohl (lrdev -> dvc_devtype);
    rdev -> dvc_spare = ntohl (lrdev -> dvc_spare);

    bcopy ((char *)lrdev -> dvc_phys,(char *) rdev -> dvc_phys, ACMAXHLN);
    bcopy ((char *)lrdev -> dvc_praddr,(char *) rdev -> dvc_praddr, PRL_ALL);
    for (c=0;c<((sizeof (struct devstats))/(sizeof (long)));c++) {
	((long *) &rdev->ds)[c] = ntohl( ((long *) &lrdev->ds)[c] );

    }
}

/*****************************************************************
 * Build a array of all statistics for all devices
 *	Entries 1,2,3..	are the statistics for each card
 *	Entry 0 the sumation of the cards
 *
 * Returns the number of interfaces
 */

rcpdevlist(sin,rdev)

   struct sockaddr_in *sin;		/* socket to the router */
   struct devinfo      rdev[];		/* resulting array */

{

   int card;
   int c;

   card = 2;

   if (getdevicestats(sin,1,&rdev[1]) == ERROR_EXIT)
     return(ERROR_EXIT);   /* Card 1 */

   do if (getdevicestats(sin,card,&rdev[card]) == ERROR_EXIT)
	 return(ERROR_EXIT);
     while (rdev[1].dvc_cable != rdev[card++].dvc_cable);

   card-=2;
   bzero((char *)rdev, sizeof (struct devinfo));

   for (c=1; c<=card; c++)	devsum(&rdev[0],&rdev[c],&rdev[0]);

   exceptnonctrs(&rdev[1],&rdev[0]);	/* Use card 1 for non counter stats */
   return(card);
}

/*****************************************************************
 * devdelta: computes delta=new-old, and moves new to old
 * 			for numdev cards
 */
devdelta(rdev,old_rdev,delta_rdev,numdev)
struct devinfo *rdev,*old_rdev,*delta_rdev;
int numdev;
{
     int cnt;
     for(cnt = 0; cnt <= numdev; cnt ++) {
         devdiff (&rdev[cnt],&old_rdev[cnt],&delta_rdev[cnt]);
	 exceptnonctrs(&rdev[cnt],&delta_rdev[cnt]);
     }
     bcopy((char *) rdev,(char *) old_rdev,
		sizeof(struct devinfo) * (numdev + 1));
}

/*****************************************************************
 * devsum: rdev3=rdev1+rdev2
 * 		Caution: non counter fields are not treated
 *		consistantly: Some are summed, others are neglected
 */
devsum(rdev1,rdev2,rdev3)
struct devinfo *rdev1,*rdev2,*rdev3;
{
    register c;
    for (c=0;c<((sizeof (struct devstats))/(sizeof (long)));c++) {
	((long *) &rdev3->ds)[c] = 
		((long *) &rdev1->ds)[c] + ((long *) &rdev2->ds)[c];
    }
}

/*****************************************************************
 * devdiff:  rdev3 = rdev1 - rdev2
 * 		Caution: non counter fields are not treated
 *		consistantly: Some are summed, others are neglected
 *
 *		dr_rq,dr_rqmax,dr_xq,dr_xqmax are copied from rdev1
 */ 
devdiff(rdev1,rdev2,rdev3)
struct devinfo *rdev1,*rdev2,*rdev3;
{
     register c;
     int rq = rdev1->ds.dr_rq;
     int rqmax = rdev1->ds.dr_rqmax;
     int xq = rdev1->ds.dr_xq;
     int xqmax = rdev1->ds.dr_xqmax;
    for (c=0;c<((sizeof (struct devstats))/(sizeof (long)));c++) {
	((long *) &rdev3->ds)[c] = 
		((long *) &rdev1->ds)[c] - ((long *) &rdev2->ds)[c];
    }
    rdev3->ds.dr_rq = rq;
    rdev3->ds.dr_rqmax = rqmax;
    rdev3->ds.dr_xq = xq;
    rdev3->ds.dr_xqmax = xqmax;
}

/*****************************************************************
 *	except non counters
 * 		copies non-counter fields from rdev1 to rdev2
 * 
 */
exceptnonctrs(rdev1,rdev2)
struct devinfo *rdev1, *rdev2;
{
    int n;

    rdev2->dvc_br = rdev1 -> dvc_br;
    rdev2->dvc_nunit = rdev1 -> dvc_nunit;
    rdev2->dvc_istatus = rdev1 -> dvc_istatus;
    rdev2->dvc_dstate = rdev1 -> dvc_dstate;
    rdev2->dvc_cable = rdev1 -> dvc_cable;
    rdev2->dvc_devtype = rdev1 -> dvc_devtype;
    rdev2->dvc_spare = rdev1 -> dvc_spare;
    for (n=0;n<=ACMAXHLN;n++)
	rdev2->dvc_phys[n] = rdev1 -> dvc_phys[n];
    for (n=0;n<=PRL_ALL;n++)
	rdev2->dvc_praddr[n] = rdev1 -> dvc_praddr[n];
    /* dv_rq and dv_xq are not counters, but since summing makes sense they
	are handeled elsewhere */
}

