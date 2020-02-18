/*  file: perror.h
 *
 *  sysnopsis:	
 *      These codes are intended to be returned by the rcp client
 *	side primitives by stuffing them into the global variable
 *	errno. They are compatible with the customized version of
 *	perror only.
 *
 *  modification history:
 *
 *	Jul-86 Chriss Stephens (chriss) at Carnegie Mellon Universy created
 */

#define USER_ERROR	500
#define FATAL		1
#define RECOVER		0

/* 
 *  generic error codes - used by client software, error codes above
 *			  5 are reserved for RCP errors.
 */

#define BAD_DIAG	USER_ERROR+6	/* unknown diag requested|returned */
#define DEAD_ROUTER     USER_ERROR+7	/* router not responding           */
#define NO_SOCKET	USER_ERROR+8	/* unable to open socket to router */
#define BAD_IPADDR	USER_ERROR+9
#define BOOT_FAIL	USER_ERROR+10
#define PGLSW_FAIL	USER_ERROR+11
#define UNSUPPORTED	USER_ERROR+12
#define TIMEOUT		USER_ERROR+13
#define BUCKET_FULL	USER_ERROR+14

#ifdef RCPCOM		/* are we compiling rcp.c, client side ? */

char *usr_errlist[] =
{
 "RCP ERROR: generic error",
 "RCP ERROR: length",
 "RCP ERROR: invalid number of arguments",
 "RCP ERROR: authorization failure",
 "RCP ERROR: packet request type",
 "RCP ERROR: unsupported request",
 "unknown diagnosis requested or returned",
 "socket open to router, but router not responding",
 "can't open socket to router",
 "set_rcp_in: illegal ip address requested",
 "reboot failed",
 "put global swith failure",
 "unsupported RCP request",
 "rcptrans: exceeded maximum number of retries (timeout)",
 "a bucket of the arpcache hash table overflowed"
 /* include additional error strings here as elements
    of comma seperated list */
};

#endif RCPCOM
