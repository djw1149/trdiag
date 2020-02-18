/*
 * $Header: rtypes.h,v 1.2 86/09/06 12:38:53 chriss Exp $
 *
 * file : rtypes.h
 *
 * synopsis : data type declarations for router tools ONLY.
 *
 * modification history :
 *
 *    Jul-86  Chriss Stephens (chriss) at Carnegie Mellon University modified
 *	to be used with tools only. Each tools program must now also include
 *	the 4.2 sys/types.h file.
 *
 * 10-Oct-85  Matt Mathis (mathis) at CMU
 * 	Added caddr_t etc from sys/types.h so this can be used with the 4.2
 *	library in place of sys/types.h
 *
 * 12-Oct-84  Mike Accetta (mja) at Carnegie-Mellon University
 *	Moved p_alarm declaration from time.h [V2.0(402)].
 *
 * 12-Oct-84  Mike Accetta (mja) at Carnegie-Mellon University
 *	Started history.
 *
 **********************************************************************
 */

typedef	long *			p_long;
typedef	int *			p_int;
typedef	short *			p_short;
typedef	char *			p_char;

typedef	unsigned int *		pu_int;
typedef	unsigned short *	pu_short;
typedef	unsigned char *		pu_char;

typedef	int 		      (*pf_int)();

#define	SWAB(const)	 (((const)<<8)|(((const)>>8)&0377))

typedef struct device *p_device;
typedef struct packet *p_packet;
typedef struct alarm  *p_alarm;

union longval
{
    long lv_long;
    struct
    {
	short lvs_hiword;
	short lvs_loword;
    }    lv_s;
};
#define	lv_hiword	lv_s.lvs_hiword
#define	lv_loword	lv_s.lvs_loword

#define	TRUE	1
#define	FALSE	0

#define MAXHWADDRL 6
