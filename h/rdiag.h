#define MAXPOSSIBLECARD 10	/* something reasonable */

#define MAXDEVTYPES	15
#define MAXDIAGTYPES	10
#define DEVNAMELEN	4
#define DIAGNAMELEN	20
#define AY 1	/* Authorize request: Yes */
#define AN 0	/* Authorize request: No */

struct rd_diagtypesT {
    char diagname[DIAGNAMELEN];
    int  diagnum;
    short needsauth;	/* AY = need authorization, AN = don't */
};

struct rd_devtypesT {
    char devname[DEVNAMELEN];	 /* short abbrev from autoconf struct */
    int  devtype;		 /* from h/rcpinfo.h */
    struct rd_diagtypesT diagtypes[MAXDIAGTYPES];
};

/*
 * Types of diagnosis output
 */
enum diag_oform_en {
    of_human_verbose_ev,
    of_human_terse_ev,
    of_machine_ev
};

