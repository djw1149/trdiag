/*****************************************************************
 *
 * $Header: elutil.c,v 1.3 86/10/10 09:12:35 chriss Exp $
 *
 * file : elutil.c
 *
 * synopsis : utilities to manipluate the errorlog from a router
 *
 * modification history:
 *
 *  Oct-86 modified by Chriss Stephens (chriss) at Carnegie Mellon so that
 *	getelog no longer prints messages to stdout.
 *
 *  Jun-86 modified by Chriss Stephens (chriss) at Carnegie Mellon University
 *	to use version 1 of router server software and support new diagfo
 *	rcp packet header.
 *
 * Dec 1985 by Matt Mathis at CMU
 *	Rewritten
 * Summer 1985 by Kevin Kirmse
 *	Created
 */

#include <stdio.h>
#include <sys/types.h>
#include "./h/rtypes.h"
#include <netinet/in.h>

#include "../h/queue.h"
#include "../h/arp.h"
#include "./h/mch.h"		/* define byte order for target machine */
#include "../h/ip.h"
#define IPP_TCP 6		/* should be in ip.h */
#include "../h/rcp.h"
#include "../h/errorlog.h"
#include "./h/perror.h"	/* error constant definition */

#define MBUFSIZ 1500

#define EN_SIZE 14		/* The size of a generic ethernet header */
#define EN_TYPE 12		/* The offset to its protocol type */
#define EN_ARP 0x0806		/* ethernet protocol types for arp */
#define EN_IP  0x0800		/* and IP */

#define VV_SIZE 6		/* The size of a generic pronet header */
#define VV_TYPE 3		/* The offset to its protocol type */
#define VV_ARP 0x3		/* pronet protocol types for arp */
#define VV_IP  0x1		/* and IP */

extern int errno;		/* for error exits */

/*****************************************************************
 *	Apply a function to every entry in the errorlog
 *
 *	Return the new max sequence (or -1 if there was an error)
 */

getelog(sin,oldmaxseq,fun)

  struct sockaddr_in *sin;		/* socket to the router */
         int          oldmaxseq;	/* how far back we want to go */
         void         (*fun)();		/* function applied to each entry */

{
    struct  errorlist *errorlist;
    struct  erroritem *erroritem;
            char       data[MBUFSIZ];
            int        offset = 0;
   register int        seq,index;
            int        sindex;

    if (V1_rcpsnatch(sin,RINFO_ELOG,data,0)== ERROR_EXIT)
       return(ERROR_EXIT);

    errorlist = (struct errorlist *) data;

    /* get the next free slot which also happens to be the oldest */
    sindex = errorlist->el_current = ntohl(errorlist->el_current);
    seq = errorlist->el_seq = ntohl(errorlist->el_seq);
    index = sindex;

do {
    erroritem = &(errorlist -> errors[index]);

    erroritem -> e_seq = ntohl (erroritem -> e_seq);
    erroritem -> e_time = ntohl (erroritem -> e_time);

    if (erroritem -> e_seq >= oldmaxseq && erroritem -> e_time != 0) {
	erroritem -> e_type = ntohs (erroritem -> e_type);
	erroritem -> p_len = ntohs (erroritem -> p_len);
	erroritem -> p_off = ntohs (erroritem -> p_off);
	erroritem -> p_maxlen = ntohs (erroritem -> p_maxlen);
	erroritem -> p_flag = ntohs (erroritem -> p_flag);
	fun (erroritem);
    }

    index = (index + 1) % ELISTLEN;
} while (index != sindex);
    return (seq);
}

/*****************************************************************
 *   Display everything known about an error
 *	Hack:  The originating interface is identified by p_off
 *		This is subject to breakage
 */
 void showerror(ei)
struct erroritem *ei;			/* The error under examination */
{
    u_short type;
    plogheader ( ei);
    if (ei->p_off == EN_SIZE) {
	fprtpkt("Enet    :    DA=# # # # # #    SA=# # # # # #    TY=# #\n",
		ei->e_header);
	type = ntohs(*( (u_short *) &(ei->e_header[EN_TYPE])));
    }
    else if (ei->p_off == VV_SIZE) {
	fprtpkt("Pro net :    DA=#    SA=#   VER=#    TY=#   RES=# #\n",
		ei->e_header);
	type = (u_short) ei->e_header[VV_TYPE];
    }
    else if (ei->p_off > ERRORCOPY) {
	fprtdata("????    :",ei->e_header,ERRORCOPY);
	return;
    }
    else {
	fprtdata("Network :",ei->e_header,ei->p_off);
	type = -1;
    }
    switch (type){
	case VV_ARP:
	case EN_ARP:
		fprtarp(ei,&ei->e_header[ei->p_off]);
		return;
	case VV_IP:
	case EN_IP:
		fprtip(ei,&ei->e_header[ei->p_off]);
		return;
	default:fprtdata("Data    :",&ei->e_header[ei->p_off],
			ERRORCOPY-ei->p_off);
    }
}

/*****************************************************************
 * Display the error header information
 *
 */
 plogheader(el)
struct erroritem *el;
{
   printf("	Time=%d ",(el->e_time));
   printf("sequ=%d ",(el->e_seq));
   printf("code=%03o ",(el->e_type));
/* fpcode(el->e_type);   BUG future feature */
   printf("len=%d ",(el->p_len));
/* printf("maxlen=%d ",(el->p_maxlen)); */
   printf("offset=%d " ,(el->p_off));
   printf("flags=%03x\n",(el->p_flag));
}

/*****************************************************************
 * Display an arp packet
 *
 */
 fprtarp(ei,ar)
struct erroritem *ei;			/* The error under examination */
register struct addres *ar;
{
    fprtpkt ( "ARP     :   HRD=# #   PRO=# #   HLN=#   PLN=#    OP=# #\n",
	    (char *) ar);
    if (ei->p_off + 8 + (2 * (ar -> ar_hln + ar -> ar_pln)) <= ERRORCOPY) {

	fprtdata ( "sha     :", ar_sha (ar), ar -> ar_hln);
	fprtdata ( "spa     :", ar_spa (ar), ar -> ar_pln);
	fprtdata ( "tha     :", ar_tha (ar), ar -> ar_hln);
	fprtdata ( "tpa     :", ar_tpa (ar), ar -> ar_pln);
    }
    else {
	fprtdata("ARP data:",ar->ar_data, ERRORCOPY - 8 - ei->p_off);
    }
}

/*****************************************************************
 *	Display an IP packet
 *
 */
 fprtip(ei,ip)
struct erroritem *ei;			/* The error under examination */
struct ip *ip ;			/* The offensive IP header */
{
    int     optlen;
    int     doff,dlen;		/* IP data length and offset */
    int	    proto;

    fprtpkt ( "IP      : V/ILH=#   TOS=#   LEN=# #:   ID=# #  FRAG=# #\n",
	    (char *) ip);
    fprtpkt ( "IP      :   TTL=# PROTO=#  HCRC=# #:\n",
	    (char *) & ip -> ip_ttl);
    fprtpkt ( "IP      :    SA=# # # #            :DA=# # # #\n",
	    (char *) & ip -> ip_src);
    optlen = ip->ip_hl * 4 - sizeof (struct ip);
    proto = (int) ip->ip_pr;			/* get the protocol */
    if (optlen > 0) {
	if (optlen > ERRORCOPY - sizeof (struct ip) - ei->p_off)
	    optlen = ERRORCOPY - sizeof (struct ip) - ei->p_off;
	fprtdata ( "IP opt  :", &((char *) ip)[sizeof(struct ip)], optlen);
    }
    doff=sizeof(struct ip)+optlen;
    dlen=ERRORCOPY - ei->p_off - doff;
    switch (proto){
	case IPP_ICMP:
	    fprtdata("ICMP    :",&((char *) ip)[doff],dlen);
	    break;
	case IPP_EGP:
	    fprtdata("EGP     :",&((char *) ip)[doff],dlen);
	    break;
	case IPP_UDP:
	    fprtdata("UDP     :",&((char *) ip)[doff],dlen);
	    break;
	case IPP_TCP:
	    fprtpkt("TCP     : SP=# #    DP=# #   SEQ=# # # #   ACK=# # # #\n"
		,&((char *) ip)[doff]);
	    fprtpkt("TCP     : DO=#    CB=#      WIN=# #   CKS=# #    UR=# #\n"
		,&((char *) ip)[doff+12]);
	    fprtdata("TCP rest:",&((char *) ip)[doff+20],dlen-20);
	    break;
	default:
	    fprtdata("IP data?:",&((char *) ip)[doff],dlen);
    }
}

/*****************************************************************
 *	printf a portion of a packet, in network order
 *		'#' in the format string become '%2x'
 *		'$' in the format string become '%03d'
 */
 fprtpkt(fmt,data)
char *fmt;			/* format string */
u_char *data;			/* buffer to be printed */
{
    register char   c;

    while (c = *fmt++) {
	if (c == '#') {
	    printf ( "%02x", *data++);
	}
	else if (c == '$'){
	    printf ( "%03d", *data++);
	}
	else {
	    printf("%c",c);
	}
    }
}

/*****************************************************************
 *	Display undeciphered bytes in a reasonable format
 *
 */
fprtdata(comment,data,len)
char *comment;				/* A name for what we are showing */
u_char *data;				/* The bytes */
int len;				/* how many */
{
    int     cnt;
    for (cnt = 0; cnt < len; cnt++) {
	if (((cnt) % 16) == 0) {
	    if (cnt != 0)
		printf ( "\n");
	    printf ( "%s ", comment);
	}
	else if (((cnt) % 8) == 0)
	    printf ( "   ");
	printf ("%02x ", *data++);
    }
    printf ("\n");
}
/* ruler        "comment : dd dd dd dd dd dd dd dd dd dd dd dd dd dd dd dd dd dd dd dd dd dd dd dd dd dd dd */
