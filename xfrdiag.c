/*
 * $Header: xfrdiag.c,v 1.2 87/11/29 15:28:06 djw Locked $
 *
 *
 * synopsis: router diagnosis simple user interface module
 *	     this module uses rd_utils.c, D*.c (only Dil.c now), showdevcnt.c,
 *	     and rcp.c to implement a simple user interface for remote router
 *	     diagnosis.  See the man page for details on usage.
 *
 * modification history:
 *
 * Sat Nov 28 1987 david waitzman (djw) at cmu
 *	created from rdiag.c
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

#include <X/Xtext.h>
#include <X/Xt/Xtlib.h>
#include "./h/XF.h"

extern int errno;
extern int perror();
extern char *strcpy();
extern struct rcp_diagfo *rdiag();

static struct sockaddr_in sin;

enum diag_oform_en outformat;

/*
 * When all these flags are TRUE, enable doing the actual diagnosis
 */
static int have_router = FALSE,
	   have_card = FALSE,
	   have_diag = FALSE;

int card, diagtype;
int devtype1 = 0, devtype2 = 0;
int needsauth;

#define DEFLABEL(NUM,STR,X,Y) \
	XFfd_label_t fld/**/NUM/**/def = {STR}; \
	XFfldarr_t fld/**/NUM/**/ = { \
	(int *)&fld/**/NUM/**/def, fd_label_ev, X, Y, fdstat_norm_ev, \
	fupstat_null_en, NULL, NULL \
	};

void check_pass();

/*
 * forward declarations of call back routines
 */
void f6cb(), f7cb(), f8cb(), f10cb(), f11cb(), f12cb();

DEFLABEL(1, "Rdiag", 1, 1);
DEFLABEL(2, "Router: ", 5, 45);
DEFLABEL(3, "Card: ", 5, 65);
DEFLABEL(4, "DiagType: ", 5, 85);

/* Not used */ char *fld5k[] = {"tr", "il"};
 char *fld5e[] = {"tr - token ring", "il - interlan"};
 XFfd_keyw_t fld5def = {
     0, 0, 2, fld5k, fld5e
 };

char *fld6k[] = {"1", "2", "3"};
char *fld6e[] = {"card 1", "card 2", "card 3"};
XFfd_keyw_t fld6def = {
    0, 0, 3, fld6k, fld6e
};
XFfldarr_t fld6 = {
    (int *)&fld6def, fd_keyw_ev, 90, 65, fdstat_lock_ev, fupstat_null_en,
    	f6cb,
"Enter the card number to diagnose.  This depends on the router configuration"
};

#define DIAGCNT	5
char *fld7k[DIAGCNT] = {"errs", "stats", "dump", "tdr", "selfdiag"};
/*
 * AN = no authorization needed
 * AY = authorization needed
 */
u_char authorize[DIAGCNT] = {AN, AN, AY, AY, AY};
u_long diagtypes[DIAGCNT] = {
    TRDIAG_ERRS,
    ILDIAG_STATS,
    ILDIAG_DUMP,
    ILDIAG_TDR,
    ILDIAG_DIAG
};
char *fld7e[DIAGCNT] = {
    "tr: error registers",
    "il: error statistics",
    "il: dump",
    "il: time domain reflectometry",
    "il: self diagnosis"
};

XFfd_keyw_t fld7def = {
    0, 0, DIAGCNT, fld7k, fld7e
};
XFfldarr_t fld7 = {
    (int *)&fld7def, fd_keyw_ev, 90, 85, fdstat_lock_ev, fupstat_null_en,
    	f7cb,
"Enter the kind of diagnosis to do.\n\
The diagnosis type depends on the card type"
};


char *fld8k[] = {"verbose", "terse", "machine"};
char *fld8e[] = {"be verbose", "be terse", "be machine readable"};
XFfd_keyw_t fld8def = {
    1, 0, 3, fld8k, fld8e
};
XFfldarr_t fld8 = {
    (int *)&fld8def, fd_keyw_ev, 90, 105, fdstat_lock_ev, fupstat_null_en,
    f8cb, "Enter the kind of output you want"
};

DEFLABEL(9, "Output: ", 5, 105);

XFfd_cmd_t fld10def = {
    "Diagnose!"
};

XFfldarr_t fld10 = {
    (int *)&fld10def, fd_cmd_ev, 40, 140, fdstat_lock_ev, fupstat_null_en,
    	f10cb, "Press this to do a single diagnosis"	
};

char fld11val[105];
XFfd_str_t fld11def = {
    "", fld11val, 104, 20
};

XFfldarr_t fld11 = {
    (int *)&fld11def, fd_str_ev, 90, 45, fdstat_norm_ev, fupstat_null_en,
    	f11cb, "Enter the name or ipaddress of the router to diagnose"
};


XFfd_cmd_t fld12def = {
    "Exit"
};

XFfldarr_t fld12 = {
    (int *)&fld12def, fd_cmd_ev, 120, 140, fdstat_norm_ev, fupstat_null_en,
    	f12cb, "Press this to exit"
};

/* array of fields */
XFfldarr_t *fldsx[] =  {&fld1, &fld2, &fld3, &fld4, /* &fld5,*/ &fld6, &fld7,
			&fld8, &fld9, &fld10, &fld11, &fld12};

XFform_t form = {
    10, 10, 300, 200, sizeof(fldsx)/sizeof(XFfldarr_t*), fldsx
};

void f11cb(cbret)
XFcallback_ret_t *cbret;
{
    cbret->display_msg = TRUE;

    if ((strlen(fld11val) == 0) || set_rcp_sin(fld11val, &sin)) {
	cbret->msg_txt = "Bad router name";
	cbret->invalid = TRUE;
    } else {
	cbret->msg_txt = "Ok router name";
        cbret->reevalfrm = TRUE;
        have_router = TRUE;
        fld11.fdstat = fdstat_lock_ev;
	fld7.fdstat = fdstat_norm_ev;
	fld8.fdstat = fdstat_norm_ev;
	fld6.fdstat = fdstat_norm_ev;
    }
}

void f12cb(cbret)
XFcallback_ret_t *cbret;
{
    cbret->exit = TRUE;
    cbret->exitcode = 1;
}

void f6cb(cbret)
XFcallback_ret_t *cbret;
{
    have_card = TRUE;
    card = fld6def.result + 1;	/* adjust for zero index base in XF */

    if (have_router & have_diag) {
	if (fld10.fdstat != fdstat_norm_ev) {
	    fld10.fdstat = fdstat_norm_ev;
	    cbret->reevalfrm = TRUE;
	}
    }
}

void f7cb(cbret)
XFcallback_ret_t *cbret;
{
    have_diag = TRUE;
    diagtype = diagtypes[fld7def.result];
    needsauth = authorize[fld7def.result];

    if (have_router & have_card) {
	if (fld10.fdstat != fdstat_norm_ev) {
	    fld10.fdstat = fdstat_norm_ev;
	    cbret->reevalfrm = TRUE;
	}
    }
}

/*
 * Diagnose! button call back routine:
 */
void f10cb(cbret)
XFcallback_ret_t *cbret;
{
    check_pass();
    urdiag(&sin, fld11val, card, diagtype, 0);
}

void f8cb(cbret)
XFcallback_ret_t *cbret;
{
    switch (fld8def.result) {
      case 0:
        outformat = of_human_verbose_ev;
	break;
      case 1:
        outformat = of_human_terse_ev;
	break;
      case 2:
        outformat = of_machine_ev;
	break;
    }
}

static char xpswd[25] = NULL;
static char *pswd = NULL;

void check_pass()
{
    static have_pswd = FALSE;
    /*
     * check if should get a password, and do so if needed, else make the
     * password null so we don't try to authorize with the null password
     */
    if (!have_pswd && (needsauth == AY)) {
	char *temppswd = (char *)getpass("Enter password: ");
	pswd = xpswd;
	strcpy(pswd, temppswd);
	have_pswd++;
    }
}


main(argc, argv)
int argc;
char *argv[];
{
    XFfhandle_t fh;
    int edres;

    XFinit_all("");
    if (!(fh = XFdefine_form(&form))) {
        fprintf(stderr, "define error!\n");
	exit(-1);
    }

    XFedit_form(fh, TRUE);

    XFdelete_form(fh);

    XFexit_all("");
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
	    printf("%d", ntohs(diag->u_di.card));
	    /*
	     * does not print a line for errors! (currently)
	     */
	break;
    }

    if (ntohs(diag->status) != RCED_NOERR) {
	fprintf(stderr,"rdiag: diagnosis error ");
	put_diag_status(diag->status);
	putchar('\n');
	return(0);
    }

    /* add cases here to diagnose more device types */
    switch (ntohs(devtype)) {
	case DVT_IL10: ildiag_main(diag);
		       break;
	case DVT_TR:   trdiag_main(diag);
		       break;
	default:       fprintf(stderr,
			    "rdiag: panic! unhandled returned device type\n");
		 exit(0);
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
	   
    if ((diag = rdiag(sin, card, diagtype, devtype, pswd)) == NULL) {
        if (outformat == of_human_verbose_ev)
	  printf("%s:: Card: %d, Diagnosis: %s\n", rname,
     	          card,
		  diagnum_to_name(diagtype, ntohs(*rc_devtype)));
        print_error ("rdiag",FATAL);
    }
    return (process_diag(diag, rname, diagtype));
}

/*
 * prints the decoded diagnosis status field.  status must be in net format
 */
put_diag_status(status)
short status;
{
    switch(ntohs(status)) {
	case RCED_NOERR: printf("no error");
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
