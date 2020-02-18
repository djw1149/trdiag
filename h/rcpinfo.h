/*
 * $Revision:$
 * History:
 ***
 * Wed Nov 11 1987 david waitzman (djw) at cmu
 *	Start history - added new devices types (DVT_*) from m68k/mch/device.h
 ***
 */
 
#define ACMAXHLN  6
#define IC_MAX    17
#define MAXPORT   4

typedef struct rcpdevice RDEVLIST[MAXPORT + 1];

/*
 *  Device type definitions.
 */
				/* support devices: */
#define	DVT_KW11	01	/* KW-11L line time clock */
#define	DVT_TTY		02	/* console terminal */

				/* network devices: */
#define	DVT_3ETHER	0100	/* 3MB ethernet */
#define	DVT_IL10	0101	/* 10MB (Interlan) ethernet */
#define	DVT_DZ11	0102	/* DZ-11 serial line(s) */
#define	DVT_LHDH11	0103	/* ACC LH/DH-11 IMP interface */
#define	DVT_DTE		0104	/* DTE-20 interface */
#define	DVT_DA28	0105	/* DA28-F interface */
#define	DVT_PRONET	0106	/* proNET 10MB ring interface */
#define	DVT_DUP11	0107	/* DUP11 synchronous line interface */
#define	DVT_3COM10	0110	/* 10MB (3Com) ethernet */
#define	DVT_DEUNA	0111	/* 10MB (DEUNA) ethernet */
#define DVT_APBUS	0112	/* Bob's AppleBus net */
#define DVT_TR		0113	/* 4MB (IBM) 802.5 token-ring */
#define DVT_CH		0114	/* 10MB (UB) ethernet */
#define DVT_SCC		0115	/* SCC serial line interface */
#define	DVT_DMR11	0116	/* DMR11 interface */


/* char router[20];	*/	/* The router with whom we are conversing */
int numdev;			/* number of interfaces in this router */

