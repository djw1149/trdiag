/*
 * XForm public header file
 *
 * $Revision: 1.15 $
 ****
 * HISTORY
 ****
 *
 * Fri Nov 20 1987 david waitzman (djw) at cmu
 *	start history
 */

/*
 * field types- the field data types supported by XForm
 */
enum XFftype_en {
    fd_bool_ev	= 'B',	/* boolean */
    fd_float_ev	= 'F',	/* floating */
    fd_int_ev	= 'I',	/* integer */
    fd_keyw_ev	= 'K',	/* key word */
    fd_label_ev	= 'L',	/* read only, not selectable */
    fd_str_ev	= 'S',	/* scrolling string */
    fd_cmd_ev	= 'C'	/* command button */
};

/*
 * field update statuses- set for each field after editing the field
 * to communicate to the application what has happened to that field.
 * the fupstate_err_en is possible if a user miss enters something like a
 * number and forces the form editing to exit (this may not be possible to
 * allow).  
 *
 * Not all field types can get all the possible values.
 * These are never set for a label nor for a command field.
 */
enum XFfupstat_en {
    fupstat_null_en = 1,	/* field is null */
    fupstat_mod_en,		/* field has been modified */
    fupstat_same_en,		/* field has been modified to same value */
    fupstat_err_en		/* field is in error (no value) */
};

typedef int point_t;		/* an X windows coordinate point */

/*
 * field display statuses- controls how the field appears on the 
 * screen.  Overlapping fields are allowed if only one field is
 * visible at a time, though this is not explictly check for- things would
 * just break. Through use of the call back routine (cbr), the
 * fdstat may change during the entry of a form.  The field will
 * then be displayed the appropriate way (perhaps turned
 * invisible or unlocked).
 */
enum XFfdstat_en {
    fdstat_inv_ev,	/* field is invisible (like unmapped) */
    fdstat_norm_ev,	/* field is enterable and displayed */
    fdstat_lock_ev	/* field is locked and displayed */
};

/*
 * this specifies the values that a callback routine may return
 * to Xform.  
 */
typedef struct XFcallback_ret_st {
unsigned int
    invalid	:1,	/* this field is invalid and must be reentered */
    refresh	:1,	/* the form display is dirty, refresh it */
    reevalfrm	:1,	/* the form has been changed, update it implies refresh = TRUE */
    reevalfld	:1,	/* just this field has been changed, update it implies refresh = TRUE */
    exit	:1,	/* exit the form now (with value exitcode)*/
    display_msg :1;	/* display the msg_txt in the message window */
    char*	msg_txt;
    int		exitcode;	/* value to exit the form with */
} XFcallback_ret_t;

/*
 * pointer to a function returning void- used for callback routines
 * these routines are passed a pointer to an XFcallback_ret_t
 * An callback routine is not associated with a label field.
 */
typedef void (*PUF)();

/*
 * An array of these define all of the form''s fields.  A structure element
 * with 'OPT' in the comment are optional- use a NULL for the field to lose
 * that feature.
 * A label field does not use the cbr, and help structure elements.
 * A command field calls the callback routine on a button press, not
 * on leaving the field.  All other fields call the callback routine when
 * the mouse leaves the field
 */
typedef struct XFfldarr_st {
    int              *fdp;	/* field definition structure */
    enum XFftype_en   ftype;	/* type of this field */
    point_t	      x,y;	/* position of this field relative to top left */
    enum XFfdstat_en  fdstat;	/* display status of this field */
    enum XFfupstat_en upstat;	/* update status of this field */
    PUF               cbr;	/* pointer to callback routine OPT */
    char	     *help;	/* help string OPT */
} XFfldarr_t;

/*
 * the main form definition structure- this structure defines one 
 * field in a form.
 */
typedef struct XFform_st {
    point_t	  x, y,		/* x & y location of top left corner */
                  x_siz, y_siz;	/* x & y sizes */
    int		  fld_cnt;	/* number of fields */
    XFfldarr_t	**flds;		/* array of fields */
} XFform_t;
/*
 * String field
 * a string field may be scrolled- maxlen characters are
 * displayed fwidth characters at a time.  
 */
typedef struct XFfd_str_st {
    char  *defaultp;		/* pointer to default value */
    char  *resultp;		/* pointer to result */
    int	   maxlen;		/* maximum string length */
    int	   fwidth;		/* field display width */
} XFfd_str_t;

/*
 * Label field
 * A label field is read-only.  the label may not be changed dynamically.
 */
typedef struct XFfd_label_St {
    char  *label;		/* the label */
} XFfd_label_t;

/*
 * Command field
 * A Command field is read-only.  the label may not be changed dynamically.
 */
typedef struct XFfd_cmd_St {
    char  *cmd;			/* label for command button  */
} XFfd_cmd_t;

/*
 * Boolean field
 * has the values TRUE or FALSE (set or unset)
 */
typedef struct XFfd_bool_st {
    char *label;		/* label for boolean button */
    int  defaultval;		/* default value (1 or 0) */
    int  result;		/* result value (1 or 0) */
} XFfd_bool_t;

/*
 * Keyword fields
 * A command button is placed on the form with keywstrs[defaultval] as
 * its label.  When the button is pressed, a menu is displayed with
 * all the explain[] entries.  When a user selects an entry, the matching
 * keywstrs[] entry is displayed in the command button''s label.
 */
typedef struct XFfd_keyw_st {
    int    defaultval;		/* index of default value */
    int    result;		/* index of result */
    int    cnt;			/* number of keywords */
    char **keywstrs;		/* keyword strings (in command button) */
    char **explain;		/* explanatory text (in menu) */
} XFfd_keyw_t;

/*
 * defines the range limits provided for an integer or float field
 */
enum XFrangegiven_en {
    rangegiven_min_ev = 1,
    rangegiven_max_ev,
    rangegiven_minmax_ev
};

/*
 * Integer field (not implemented)
 */
typedef struct XFfd_int_st {
    int	 *defaultp;		/* pointer to default value */
    int  *resultp;		/* pointer to result */
    enum XFrangegiven_en
    	  rangegiven;		/* range restrictions provided */
    int	  min, max;		/* minimum and maximum values */
} XFfd_int_t;

/*
 * Float field (not implemented)
 */
typedef struct XFfd_float_st {
    float  *defaultp;		/* pointer to default value */
    float  *resultp;		/* pointer to result */
    enum XFrangegiven_en
    	    rangegiven;		/* range restrictions provided */
    float   min, max;		/* minimum and maximum values */
} XFfd_float_t;
/*
 * A XFfhandle_t (XForm form handle) is returned by the define_form routine
 * and is passed as a parameter to most routines.
 */
typedef Window XFfhandle_t;

/*
 * initializes the forms package and the Xtoolkit for a given display.
 */
extern int XFinit_all(/* display */);

/*
 * initializes the forms package given an Xtoolkit already up for
 * some display.
 */
extern int XFinit_partial();

/*
 * defines a form.
 * Returns NULL if an error is encountered in the form.
 */
extern XFfhandle_t XFdefine_form(/* XFform_t */);

/*
 * maps the form onto the screen
 */
extern void XFmap_form(/* XFfhandle_t */);

/*
 * unmaps the form onto the screen
 */
extern void XFunmap_form(/* XFfhandle_t */);

/*
 * deletes a form
 */
extern void XFdelete_form(/* XFfhandle */);
	
/*
 * executes a form.  returns an accepted/aborted status
 * the form must be mapped or an error occurs.
 */
extern int XFedit_form(/* XFfhandle */);

/*
 * exits the forms package.
 * requires all forms to be closed.
 */
extern void XFexit_partial();

/*
 * exits the forms package and the X toolkit.
 * requires all forms to be closed.
 */
extern void XFexit_all(/* char *display */);

/*
 * Set the fonts to use, call before calling either XFinit_ routines
 */
extern void XFset_fonts(/* new_f_text, new_f_body */);

#ifndef NULL
#define NULL 0
#endif
