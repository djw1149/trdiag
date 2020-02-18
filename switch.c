/*
 * $Header: switch.c,v 1.3 86/07/25 16:17:01 chriss Exp $
 *
 * file : switch.c
 *
 * history:
 * 3/31/86: david waitzman (djw) at carnegie mellon
 *	changed password readin code to use getpass, so the password won't
 *	be echoed on the terminal.
 */
 
#include <stdio.h>

getritype(argc,argv)
int argc;
char *argv[];
{
   int cnt;
   int infotype = 's';
   for(cnt = 1; cnt < argc; cnt ++) {
      if (argv[cnt][0] == '-')
         if (argv[cnt][1] == 'T')
            infotype = argv[cnt][2];
   }
   return(infotype);
}
         

initswitch(argc,argv,router,
	password,bootstr,bstrflag,	/* boot options */
	card,detail,			/* card control options */
	total,once,getstat,putstat,	/* time control options */
	verbose)			/* format */
int argc;
char *argv[];
char *router,*password,*bootstr,*getstat,*putstat;
int *bstrflag,*card,*detail,*total,*once,*verbose;
{
    int     cnt;
    int     routerflag = 0;
    char   *targ;

    if (bstrflag)
	*bstrflag = 0;
    if (card)
	*card = 0;
    if (detail)
	*detail = 0;
    if (total)
	*total = 0;
    if (once)
	*once = 0;
    if (verbose)
	*verbose = 0;

    for (cnt = 1; cnt < argc; cnt++) {
	targ = argv[cnt];
	if (*targ++ == '-')
	    switch (*targ++) {
		case 'T': 
		    break;
		case 'r': 
		    if (router)
			strncpy (router, targ, 20);
		    routerflag = 1;
		    break;
		case 'f': 
		    if (bootstr)
			strncpy (bootstr, targ, 100);
		    if (bstrflag)
			*bstrflag = 1;
		    break;
		case 'm': 
		    if (bstrflag)
			*bstrflag = -1;
		    break;
		case 'a': 
		    if (bstrflag)
			*bstrflag = 0;
		    break;
		case 'c': 
		    if (card)
			*card = atoi (targ);
		    break;
		case 'd': 
		    if (detail)
			*detail = 1;
		    break;
		case 'o': 
		    if (once)
			*once = 1;
		    break;
		case 't': 
		    if (total)
			*total = 1;
		    break;
		case 'g': 
		    if (getstat)
			strncpy (getstat, targ, 100);
		    break;
		case 'p': 
		    if (putstat)
			strncpy (putstat, targ, 100);
		    break;
		case 'v': 
		    if (verbose) {
			do {
			    (*verbose)++;
			}
			while ((*targ++) == 'v');
		    }
		    break;
		default: 
		    printf ("Usage: rinfo -r<router> [ -c<card> -d -o -t -v[v] ] -T<format> ...\n");
		    exit(-1);
	    }
    }
    if (router && !routerflag) {
	printf ("Enter router : ");
	scanf ("%s", router);
    }
    if (password) {
	char *temppswd = (char *)getpass("Enter password: ");
	strcpy(password, temppswd);
    }
}
