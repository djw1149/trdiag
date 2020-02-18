/*
 * $Header: nrdiag.c,v 1.3 87/12/08 17:54:53 djw Locked $
 *
 * file: diag.c
 *
 * synopsis: router diagnosis simple user interface module
 *	     this module uses rd_utils.c, D*.c (only Dil.c now), showdevcnt.c,
 *	     and rcp.c to implement a simple user interface for remote router
 *	     diagnosis.  See the man page for details on usage.
 *
 * modification history:
 *
 * Wed Dec  2 1987 david waitzman (djw) at cmu
 *	create from rdiag.c version 1.7
 *
 * Wed Nov  5 1986 david waitzman (djw) at cmu
 *	fixed printing format problem- now prints card numbers on a nonverbose
 *	diagnosis.
 *
 * Jul-86 Chriss Stephens (chriss) at Carnegie Mellon University modified to
 *	use customized perror (see rcp.c) routine for consistent handling of
 *	error conditions.
 *
 * david waitzman (djw@ampere.ece.cmu.edu) march '86
 *	created.  much help from rcp code by Matt Mathis, Kevin Krimse
 *	and Mike Acceta.
 *
 */

#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include "./h/rtypes.h"
#include <netinet/in.h>

#include "../h/queue.h"
#include "./h/mch.h"		/* define byte order for target machine */
#include "../h/ip.h"
#include "../h/rcp.h"
#include "../h/errorlog.h"
#include "./h/rcpinfo.h"
#include "./h/perror.h"			/* define error constants */
#include "./h/rdiag.h"

extern int errno;
extern int perror();
extern char *strcpy();
extern struct rcp_diagfo *rdiag();
extern char getopt();
extern char *optarg;
extern int optind;

enum diag_oform_en outformat;
char xpswd[20];
char *pswd = xpswd;	/* the password for authorizing diagnoses */

main(argc, argv)
int argc;
char *argv[];
{
    int c, errflag;
    struct sockaddr_in sin;
    int card = -1, diagtype = -1, devtype = -1;
    short needsauth, demands_auth = 0;
    int count = 1, delay = 1, i;
    char rname[100];

    outformat = of_human_terse_ev;	/* Default */

    while ((c = getopt(argc, argv, "r:c:D:d:R:I:ivtmp")) != EOF)
        switch (c) {
	  case 'r':
	    if (set_rcp_sin(optarg, &sin)) {
		errno = NO_SOCKET;
		print_error("rdiag", FATAL);
	    }
	    strcpy(rname, optarg);	/* save the name to label output */
	    break;
	  case 'c':
	    if (!(card = atoi(optarg))) {
		fprintf(stderr, "Bad card <%s>\n", optarg);
		errflag++;
	    }
	    break;
	  case 'D':
	    if (!(devtype = devname_to_devtype(optarg))) {
		fprintf(stderr, "Bad device type <%s>\n", optarg);
		errflag++;
	    }
	    break;
	  case 'd':
	    if (!(diagtype =diagname_to_diagtype(optarg,&devtype,
	    					 &needsauth))) {
		fprintf(stderr, "Bad diagnosis type <%s>\n", optarg);
	        errflag++;
	    }
	    break;
	  case 'i':
	    count = -1;
	    break;
	  case 'R':
	    count = atoi(optarg);
	    if (count < 1) {
		fprintf(stderr, "Bad count <%s>\n", optarg);
		errflag++;
	    }
	    break;
	  case 'I':
	    if (sscanf(optarg, "%d", &delay) != 1) {
		fprintf(stderr, "Bad delay interval <%s>\n", optarg);
		errflag++;
	    }
	    break;
	  case 'v':
            outformat = of_human_verbose_ev;
	    break;
	  case 'm':
            outformat = of_machine_ev;
	    break;
	  case 't':	    
	    outformat = of_human_terse_ev;
	    break;
	  case 'p':
	    demands_auth = 1;
	    break;
	  case '?':
	  default:
	    errflag++;
	    break;
	}
     if (errflag || diagtype == -1 || (devtype == -1 && card == -1)) {
       fprintf(stderr,
"Usage: %s\n\
-r router\n\
[-c card# | -D device_name]\n\
-d diagnosis_type\n\
[-i (infinite loop) | -R repeat_count] [-I delay_interval (in seconds)]\n\
[-v (verbose) | -t (terse) | -m (machine readable)]\n\
[-p (force asking for password)]\n",argv[0]);
       return(0);
    }

    /*
     * check if should get a password, and do so if needed, else make the
     * password null so we don't try to authorize with the null password
     */
    if ((needsauth == AY) || demands_auth) {
	char *temppswd = (char *)getpass("Enter password: ");
	strcpy(pswd, temppswd);
    } else pswd = NULL;

    for (i = 0; (count == -1) || count-- ;) {
	if (devtype == -1)
	    devtype = 0;	/* Convert to what rcp thinks is unset */
        urdiag(&sin, rname, card, diagtype, devtype);
	if ((count != 1) && (count != 0))	/* sleep if repeated diags */
	    sleep(delay);
    }
}

/*
 * process the results from a diagnosis. Now compatible with version 1
 * of software.
 */
process_diag(diag, rname, diagtype)
struct rcp_diagfo *diag;	/* the returned diagnosis packet */
char *rname;			/* router name */
int  diagtype;			/* diagnosis type */
{
    u_short devtype = *(u_short *)diag->data;

    switch (outformat) {
      case of_human_verbose_ev:
        printf("%s:: Card: %d, Diagnosis: %s, Device type: ",
		rname, ntohs(diag->u_di.card),
		diagnum_to_name(diagtype, ntohs(devtype)));
        devswt(ntohs(devtype));	/* show the device type */
        putchar('\n');
	break;
      case of_human_terse_ev:
        printf("%d: ", ntohs(diag->u_di.card));	/* djw: Wed Nov  5 1986 */
	break;
      case of_machine_ev:
	if (ntohs(diag->status) == RCED_NOERR)
	    printf("%d ", ntohs(diag->u_di.card));
	    /*
	     * does not print a card label on errors!
	     */
	break;
    }
    if (ntohs(diag->status) != RCED_NOERR) {
	if (outformat == of_machine_ev) printf("? ");	/* signals error */
	printf("rdiag: diagnosis error- ");
	put_diag_status(diag->status);
	putchar('\n');
	return(0);
    }

    /* add cases here to diagnose more device types */
    switch (ntohs(devtype)) {
      case DVT_IL10:
        ildiag_main(diag);
	break;
      case DVT_TR:
        trdiag_main(diag);
	break;
      default:
	generic_diag_output(diag);
	break;
    }
    return(1);
}

/*
 * The main diagnosis control code.  loops for multiple device diagnosis.
 * calls process_diag to show results of diagnosis.
 */
int urdiag(sin, rname, card, diagtype, devtype)
struct sockaddr_in *sin;		/* Socket to the router */
char *rname;				/* router name */
int  card;				/* which router interface */
int  diagtype;				/* diagnosis type */
int  devtype;				/* device type */
{
    struct rcp_diagfo *diag;
	   u_short    *rc_devtype = (u_short *)diag->data;
    int processed = 0;	/* Count of cards processed */

    /* card is positive if not a multiple diagnosis */
    if (card != -1) {
      if ((diag = rdiag(sin, card, diagtype, devtype, pswd)) == NULL) {
        if (outformat == of_human_verbose_ev)
	  printf("%s:: Card: %d, Diagnosis: %s\n", rname, card,
		  diagnum_to_name(diagtype, ntohs(*rc_devtype)));
        print_error ("rdiag",FATAL);
      }
      return (process_diag(diag, rname, diagtype));
    }

    /* a multiple diagnosis: */
    for (card = 1; card < MAXPOSSIBLECARD; card++) {
        if ((diag = rdiag(sin, card, diagtype, devtype, pswd)) == NULL) {
          if (outformat == of_human_verbose_ev)
	    printf("%s:: Card: %d, Diagnosis: %s\n", rname, card,
		    diagnum_to_name(diagtype, ntohs(*rc_devtype)));
	  print_error("rdiag",FATAL);
	}
        /* check if done with the multiple diagnosis, must do this before
	   process_diag or the user is bothered with an error message */
        if (ntohs(diag->status) == RCED_NOCARD)
	    continue;

        if (ntohs(diag->status) == RCED_BADTYPE)
	    continue;

        processed++;
        process_diag(diag, rname, diagtype);

	/* determine next state */
	switch(ntohs(diag->status)) {
	    case RCED_NOERR:
	    /*
	     * Have cases for all types of errors which only affect one card
	     * in a router- i.e. those errors for which to diagnosis the other
	     * cards
	     */
	    case RCED_BADTYPE:
	    case RCED_DFAIL:
	    case RCED_NOTREADY:
	      break;
	    default:
	      switch (outformat) {
		case of_human_verbose_ev:
		  fprintf(stderr, "rdiag: can't continue multiple diagnosis\n");
		break;
		case of_human_terse_ev:
		  fprintf(stderr, "can't continue\n");
		break;
		case of_machine_ev:
		  printf("? can't continue multiple diagnosis\n");
		break;
	      }
	      return(0);
	}
    }
    if (!processed) {
       if (outformat == of_machine_ev) printf("? "); /* signals error */
       printf("rdiag: diagnosis error- No card of that device type exists\n");
       exit(-1);
    }    
}


/*
 * prints the decoded diagnosis status field.  status must be in net format
 */
put_diag_status(status)
short status;
{
    switch (ntohs(status)) {
	case RCED_NOERR: printf("no error");	/* this case not reached */
			 break;
	case RCED_UNSUP: printf("unsupported diagnosis for given device");
			 break;
	case RCED_DFAIL: printf("diagnosis failed");
			 break;
	case RCED_BADCARD: printf("that card doesn't have any diagnostics");
			 break;
	case RCED_NOCARD: printf("no card of that number exists");
			 break;
	case RCED_BADTYPE: printf("given device type != actual device type");
			 break;
	case RCED_NOAUTH: printf("no authorization given for diagnosis");
			 break;
	case RCED_NOTREADY:
		     printf("device is not ready for diagnosis- retry later");
			 break;
	default:	 printf("unknown status:%d", ntohs(status));
    }
}
