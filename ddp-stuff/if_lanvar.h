/*
 * 5799-CGZ (C) COPYRIGHT IBM CORPORATION 1986
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */
/* $Header: if_lanvar.h,v 2.9 86/07/08 14:12:58 donna Exp $ */
/* $Source: /usr/sys/DONNA/if_lanvar.h,v $ */

#if !defined(lint) && !defined(LOCORE)	&& defined(RCS_HDRS)
static char *rcsidif_lanvar = "$Header: if_lanvar.h,v 2.9 86/07/08 14:12:58 donna Exp $";
#endif

struct lan_ctl {
	struct lan_scb lan_scb;
	struct lan_ssb lan_ssb;
	struct lan_bia lan_bia;
	unsigned short open_parm[16];
	unsigned short close_parm[2];
	unsigned short lan_faddr[2];	/* DDP Arguments for SET FUNCTIONAL ADDRESS */
	unsigned short lan_gaddr[2];	/* DDP Arguments for SET GROUP ADDRESS */
	struct lan_errlog lan_errlog;	/* DDP Error log for READ ERROR LOG */
};

#define LAN_XMITLIST_CT	2	/* Number of transmit lists */
#define LAN_RECVLIST_CT	2	/* Number of receive lists */
#define LAN_MAX_LISTS_PER_PAGE	75	/* Number of lists per big mbuf */

struct lan_recv_ctl{
	struct lan_list lan_rlist[LAN_RECVLIST_CT];
	struct list_hdr list_hdr[LAN_RECVLIST_CT];
};

/* Token-Ring software status per adapter */
struct lan_softc {
	struct arpcom lns_ac;		/* like ethernet structs */
#define lns_if lns_ac.ac_if		/* network-visible interface */
#define lns_addr lns_ac.ac_enaddr	/* hardware lan address */
	short lns_oactive;		/* is output active */
	short lns_xbuf;			/* in-use xmt buf */
	short lns_nextbuf;		/* next buf to fill */
	short lns_xstart[LAN_XMITLIST_CT];
	short lns_ring;			/* ring state */
	short lns_adapter;		/* adapter state */
	short lns_open_retries;		/* open retry count */
	short lns_beacon;		/* open retries while beaconing */
	short lns_ring_watch;		/* control timeout count */
	unsigned short lns_node_addr;	/* node address in adapter storage */
	unsigned short lns_open_options;/* DDP current open options */
	struct proc *lns_freezer;
	struct proc *lns_reader;	/* DDP errlog or adapter buffer reader */
	struct lan_ctl *lns_ctl;	/* control structure pointer */
	struct lan_list *xp[LAN_XMITLIST_CT];/* xmit list ptrs */
	struct lan_list *rp[LAN_RECVLIST_CT];/* recv list ptrs */
	struct list_hdr *rh[LAN_RECVLIST_CT];/* recv hdr ptrs */
	struct lan_recv_ctl *lns_recv;	/* recv struct pointer */
	char *lns_xbufp[LAN_XMITLIST_CT];
	struct mbuf *lns_rbufp[LAN_RECVLIST_CT];	/* receive buffer pointers */
	struct mbuf *lns_rdata[LAN_RECVLIST_CT];	/* receive data area pointers */
	struct lan_tcw_list {		/* remember tcw's in use */
		int num_entries;
		int tcw_slot[NUMTCW];
		} lan_tcw_list[LAN_XMITLIST_CT];
	int lan_dma_chan;		/* dma channel used by this addr*/
};

/* Adapter/Ring Status */
#define LAN_ADAP_OPEN		0x8000	/* Adapter open */
#define LAN_OPEN_IN_PROGRESS	0x4000	/* Adapter open in progress */
#define LAN_RETRY_IN_PROGRESS	0x2000	/* Adapter open being retried */
#define LAN_ADAP_BROKEN		0x1000	/* Adapter failure */
#define LAN_ADAP_AUTOER1	0x0800	/* Adapter internal error */
#define LAN_ADAP_FCTNFAIL	0x0400	/* Adapter function failure */
#define LAN_ADAP_BIA_READ	0x0200	/* Adapter node address read */
#define LAN_ADAP_DOWN		0x0100	/* Adapter closed */
#define LAN_ADAP_FROZEN		0x0008	/* Adapter frozen for dump */
#define LAN_BEACONING		0x0001	/* Ring beaconing */
#define LAN_RECOVERY		0x0002	/* Ring in recovery */
#define LAN_CABLE_FAIL		0x0004	/* Cable failure */

/* Logical Link Control Class 1 Definitions */
#define LAN_IPTYPE	0x06	/* IP packet type */
/* TEMP:  FOLLOWING FAKE VALUE IS PENDING ARP NUMBER ASSIGNMENT */
#define LAN_ARPTYPE	0x99	
/* NB: The bits in the following Unnumbered Information Command are in the 
 *     wrong order.  However, this sequence is being temporarily maintained
 *     for compatibility;  it should be corrected when all workstations are
 *     able to receive frames correctly when the UI command is sent correctly. */
#define LAN_UI_CMD	0xc0
#define LAN_LLC_XID_CMD0	0xaf
#define LAN_LLC_XID_CMD1	0xbf
#define LAN_LLC_TEST_CMD0	0xe3
#define LAN_LLC_TEST_CMD1	0xf3
#define LAN_L_XID_RESP		3


/* Maximum transmission unit */
#define LAN_MTU	ETHERMTU	/* same as ethernet */

/* Miscellaneous */
#define  LAN_MAX_OPEN_RETRY	3	/* Max number of open retries	*/
#define  LAN_INIT_ERROR	-1
#define ONESEC		240000	/* argument to DELAY 1 second */
#define TEN_MS		2400	/* argument to DELAY 10 ms */
#define SIXTY		60	/* ring recovery time in seconds */
#define LAN_ADDR_PENDING	0x01	/* Waiting to update lan addr */

