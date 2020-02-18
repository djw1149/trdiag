/*
 * 5799-CGZ (C) COPYRIGHT IBM CORPORATION 1986
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */
/* $Header: if_lanreg.h,v 2.7 86/07/08 14:12:51 donna Exp $ */
/* $Source: /usr/sys/DONNA/if_lanreg.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidif_lanreg = "$Header: if_lanreg.h,v 2.7 86/07/08 14:12:51 donna Exp $";
#endif

/* Adapter register addresses */
struct lan_device {
	unsigned short lan_data;	/* system interface data reg */
	unsigned short lan_datai;	/* system interface data incr */
	unsigned short lan_address;	/* system interface address */
	unsigned short lan_cmdstat;	/* command/status register */
	unsigned short lan_enable;	/* enable adapter interrupts */
	unsigned short lan_undef;	/* undefined adapter address */
	unsigned short lan_disable;	/* disable adapter interrupts */
	unsigned short lan_hreset;	/* hard reset adapter */
};

/* IRQ12 interrupt reset (shared level interrupt) */
#define LAN_IRQ12	((char *)0xF00006F4)

/* Adapter command/status register offset for probe */
#define LAN_CMDREG	0x6

/* Adapter locations for initialization and adapter check */
/* These values are written to the lan_address register */
#define LAN_INIT_DATAA	0x0200
#define LAN_ACHECK_DATA 0x05E0

/* System to Adapter Interrupts */
/* These values are written to the lan_cmdstat register */
#define LAN_RESET	0xFF80	/* reset adapter */
#define LAN_SSBCLEAR	0xA000	/* notify that status block available */
#define LAN_EXECUTE	0x9080	/* initiate command in command block */
#define LAN_RECVCONT	0x8480	/* request recv operation to continue */
#define LAN_RECVALID	0x8280	/* signal recv list suspension cleared */
#define LAN_XMTVALID	0x8180	/* signal xmit list suspension cleared */

/* Adapter to System Response */
/* These values are read from the lan_cmdstat register */
#define LAN_INT	0x0080	/* valid interrupt */
#define LAN_ADAP_INT	0x000E	/* adapter -> system interrupt code */
#define LAN_ACHECK	0x0000	/* unrecoverable adapter error */
#define LAN_IMPLFRC	0x0002	/* IMPL force mac frame received */
#define LAN_RINGSTAT	0x0004	/* ring status update */
#define LAN_SCBCLEAR	0x0006	/* system command block clear */
#define LAN_CMDSTAT	0x0008	/* command status update */
#define LAN_RECVSTAT	0x000A	/* receive status update */
#define LAN_XMITSTAT	0x000C	/* transmit status update */
#define LAN_STAT_BITS	\
"\20\20INTADAP\17RESET\16SSBCLR\15EXECUTE\14SCBREQ\13RCVCONT\12RCVLD\11XMTVLD\
\10INTSYS\7INIT\6TEST\5ERR"

/* System Command Block */
struct lan_scb {
	unsigned short command;
	unsigned short h_addr;
	unsigned short l_addr;
	};

/* Adapter Commands */
/* These values are written to the lan_scb control block */
#define LAN_OPEN	0x0003	/* open adapter */
#define LAN_TRANSMIT	0x0004	/* transmit frame */
#define LAN_TRANSHLT	0x0005	/* interrupt transmit list chain */
#define LAN_RECEIVE	0x0006	/* receive frames */
#define LAN_CLOSE	0x0007	/* close adapter */
#define LAN_SETGADDR	0x0008	/* set group address */
#define LAN_SETFADDR	0x0009	/* set functional address */
#define LAN_RDERRORLOG	0x000A	/* read error log */
#define LAN_RDADAPTR	0x000B	/* read adapter storage */

/* System Status Block */
struct lan_ssb {
	unsigned short command;
	unsigned short status0;
	unsigned short status1;
	unsigned short status2;
	};

/* Adapter Status */
/*	These values are read from the lan_ssb control block */
#define LAN_SSB_RING	0x0001	/* ring status update */
#define LAN_SSB_REJECT	0x0002	/* command reject */
#define LAN_CMD_REJ_BITS	\
"\20\20ILLCMD\17ADDRERR\16ADAPOPN\15ADAPCLS\14SAMCMD"

/* Adapter Check Status */
/* These values are read from the adapter check field */

#define ADAP_CHK_SIZE	4
#define RECV_PARITY	0x80
#define XMIT_PARITY	0x40
#define XMIT_UNDERRUN	0x20
#define RECV_OVERRUN	0x10
#define LAN_ACHECK_BITS	\
"\20\20DIOPAR\17DMARD\16DMAWT\15ILLOP\14LBPAR\13EMPAR\12SIFPAR\11PHPAR\
\10RCVPAR\7WTPAR\6UNDRN\5OVRUN\4INVINT\3INVERR\2INVXOP\1PGMCHK"

/* Ring Status */
/* These values are read from the lan_ssb on ring status update */
#define LAN_SIGNAL_LOSS	0x8000	/* signal loss */
#define LAN_HARD_ERROR	0x4000	/* xmit/recv beacon frames */
#define LAN_SOFT_ERROR	0x2000	/* xmit report error mac frame */
#define LAN_XMIT_BEACON	0x1000	/* xmit beacon frames */
#define LAN_WIRE_FAULT	0x0800	/* short circuit in data path */
#define LAN_AUTOER1	0x0400	/* auto-removal process */
#define LAN_AUTOER2	0x0200	/* reserved */
#define LAN_REMOVE_RECV	0x0100	/* remove received */
#define LAN_CTR_OVER	0x0080	/* counter overflow */
#define LAN_SINGLE	0x0040	/* single station */
#define LAN_RING_BITS	\
"\20\20SIGLOSS\17HRDERR\16SFTERR\15BEACON\14WRFLT\13AERR\11RMV\8CTROVFL\7SNG"

/* Adapter Initialization Parameters */
#define LAN_INIT_OPTIONS	0x8000	/* resv bit on, default options */
#define LAN_INIT_CMDSTAT	0x0C0C	/* int vector cmd, xmit stat */
#define LAN_INIT_RECVRING	0x0C0C	/* int vector recv,ring stat */
#define LAN_INIT_SCBCHK	0x0C0C	/* int vector scb,adapter check */
#define LAN_INIT_RBURST	0x0000	/* dma burst size for recv data */
#define LAN_INIT_XBURST	0x0000	/* dma burst size for xmit data */
#define LAN_INIT_ABORT	0x0101	/* no. of dma attempts if error */

/* Adapter Initialization Status */
#define LAN_INITIALIZE	0x0040	/* bring-up diagnostics complete */
#define LAN_TEST	0x0020	/* initialization test */
#define LAN_ERROR	0x0010	/* initialization error */

/* Adapter Open Parameters */
#define LAN_OPEN_OPTIONS	0x0000	/* default open options */
					/* DDP - Begin */
#define LAN_OPEN_WRAP_INTERFACE	0x8000	/* wrap interface */
#define LAN_OPEN_PASS_ADAP_MAC	0x1000	/* pass adapter mac frames */
#define LAN_OPEN_PASS_ATTN_MAC	0x0800	/* pass attention mac frames */
#define LAN_OPEN_PASS_BEACON	0x0080	/* pass beacon mac frames */
#define LAN_OPEN_RCV_ALL_MAC   	0x1880	/* pass all mac frames */
					/* DDP - End */
#define LAN_OPEN_RLIST		0x0000	/* receive list size */
#define LAN_OPEN_XLIST		0x0000	/* transmit list size */
#define LAN_OPEN_BUFSIZE	0x00e8	/* buffer size = 224 bytes */
#define LAN_OPEN_RAMSTART	0x4006	/* RAM start address */
#define LAN_OPEN_RAMEND		0x7FFE	/* RAM end address */
#define LAN_OPEN_XMINMAX	0x040e	/* xmit buffer min/max counts */

/* Open Status */
#define LAN_OPEN_COMPLETE	0x8000	/* open complete */
#define LAN_OPEN_NODE_ERROR	0x4000	/* node address error */
#define LAN_OPEN_LIST_ERROR	0x2000	/* recv/xmit list size error */
#define LAN_OPEN_BUF_ERROR	0x1000	/* buffer size error */
#define LAN_OPEN_RAM_ERROR	0x0800	/* RAM address error */
#define LAN_OPEN_XMIT_ERROR	0x0400	/* xmit buffer count error */
#define LAN_OPEN_ERROR		0x0200	/* error detected during open */
#define LAN_OPEN_STAT_BITS	\
"\20\20OPENOK\17ADDRERR\16LSTSZ\15BUFSZ\14RAMERR\13XMTBFCT\12OPENERR"
#ifdef KERNEL
char *open_errmsg[16] = {
	"undefined",
	"function failure",
	"receiver exception",
	"undefined",
	"undefined",
	"timeout",
	"ring failure",
	"ring beaconing",
	"duplicate node address",
	"request parameters",
	"remove received",
	"IMPL force received" };
#endif

/* Open Command Phases */
#define LAN_OPEN_LOBE_TEST	0x0010	/* lobe media test */
#define LAN_OPEN_INSERTION	0x0020	/* physical insertion */
#define LAN_OPEN_ADDR_VER	0x0030	/* address verification */
#define LAN_OPEN_ROLL_CALL	0x0040	/* roll call poll */
#define LAN_OPEN_REQ_PARM	0x0050	/* request parameters */

/* Open Error Codes */
#define LAN_OPEN_FUNC_FAILURE	0x0201	/* function failure */
#define LAN_OPEN_OSIGNAL_LOSS	0x0202	/* signal loss */
#define LAN_OPEN_OWIRE_FAULT	0x0203	/* wire fault */
#define LAN_OPEN_FREQ_ERROR	0x0204	/* unused */
#define LAN_OPEN_TIMEOUT	0x0205	/* timeout */
#define LAN_OPEN_RING_FAILURE	0x0206	/* ring failure */
#define LAN_OPEN_RING_BEACON	0x0207	/* ring beaconing */
#define LAN_OPEN_DUP_NODE	0x0208	/* duplicate node address */
#define LAN_OPEN_OREQ_PARM	0x0209	/* request parameters */
#define LAN_OPEN_OREM_RECV	0x020A	/* remove received */
#define LAN_OPEN_OIMPL		0x020B	/* IMPL force received */

/* Close Status */
#define LAN_CLOSE_COMPLETE	0x8000	/* close complete */

/* Set Functional Address Status */
#define LAN_SETFADDR_COMPLETE	0x8000	/* set functional addresscomplete */

/* Set Group Address Status */
#define LAN_SETGADDR_COMPLETE	0x8000	/* set group addresscomplete */

/* Read Error Log Status */
#define LAN_RDERRORLOG_COMPLETE	0x8000	/* read error log complete */

/* Read Adapter Buffer Status */
#define LAN_RDADAPTR_COMPLETE	0x8000	/* read adapter buffer complete */

/* Adapter error log table */
struct lan_errlog {
    short lan_line_error;
    short lan_reserved1;
    short lan_burst_error;
    short lan_ari_fci;
    short lan_abort_delimeter;
    short lan_reserved2;
    short lan_lost_frames;
    short lan_recv_congestion;
    short lan_frame_copied;
    short lan_reserved3;
    short lan_lost_tokens;
    short lan_reserved4;
    short lan_dma_bus;
    short lan_dma_parity;
};

/* Adapter READ ADAPTER BUFFER Command header */
struct lan_read_adapter_hdr {
    short lan_rdab_data_count;
    short lan_rdab_data_addr;
};

#define LAN_L_ADDR	6	/* length lan address */
#define LAN_N_DATA	3	/* max data fields in recv/xmit list */
/* Receive, Transmit Lists */
struct lan_list {
	unsigned short xlp_h;
	unsigned short xlp_l;
	unsigned short cstat;
	unsigned short frame_size;
	struct d_list {
		unsigned short d_cnt;
		unsigned short d_haddr;
		unsigned short d_laddr;
	} d_parm[LAN_N_DATA];
};

/* Lan header includes control fields, source and destination
 addresses, and llc fields: */
struct list_hdr {
	char pcf0;
	char pcf1;
	char to_addr[LAN_L_ADDR];
	char from_addr[LAN_L_ADDR];
	char dsap;
	char ssap;
	char llc_ctl;
};

/* Receive Status */
#define LAN_FRAME_COMPLETE	0x8000	/* received frame complete */
#define LAN_RECV_SUSPEND	0x4000	/* receive chain ended */
#define LAN_RCSTAT_COMPLETE	0x4000	/* received frame complete */
#define LAN_RECV_BITS		"\20\20RCVCMPL\17RCVSUSP"
#define LAN_RCSTAT_BITS	"\20\17FRMCMPL\16FRMSTRT\15FRMEND"


/* Transmit Status */
#define LAN_XCSTAT_COMPLETE	0x4000	/* transmitted frame complete */

/* Adapter Storage for Burned-In Address */
struct lan_bia {
	unsigned short count;
	unsigned short adap_addr;
	char bia[6];
	char flag;
};

/* Miscellaneous */
#define LAN_RETRY	4	/* Retries during initialization */
#define  LAN_RESET_WAIT	300	/* Wait time (in 10's of ms) for bring-up-diags */
#define  LAN_DMA_TIMEOUT	1000	/* Dma timeout error (10's of ms)	*/
#define  SCB_LEN	6	/* Number bytes in scb			*/
#define  SCB_INIT	0x0000c1e2d48b	/* Scb initialization contents	*/
#define  SSB_LEN	8	/* Number bytes in ssb			*/
#define  SSB_INIT	0xffffd1d7c5d9c3d4	/* Ssb initialization contents */
#define	LAN_ADDRESSES	0x0a04	/* Pointer to adap addresses in adap storage */
#define LAN_OPEN_PHASE_MASK	0xff0f	/* Ignore phase if open error */
#define LAN_NUM_IPARMS	11	/* Number of initialization parameters */
#define LAN_TIMEOUT	10	/* Dma timeout in 10 seconds */
#define LAN_PCF0	0x00	/* Physical Control Field 0 */
#define LAN_PCF1	0x40	/* Physical Control Field 1 (not mac) */
#define LAN_PCF1_MAC	0x00	/* Physical Control Field 1 (mac) */
#define LAN_CHAIN	0x8000	/* chain indicator in recv/xmit list */
#define LAN_ODD_PTR	0x0001	/* end of list indicator */
#define LAN_X_CSTAT_REQ	0xB7FF	/* Xmit cmd/stat on request */
#define LAN_XMIT_VALID		0x8000	/* Xmit list valid indicator */
#define LAN_XMIT_EOF		0x1000	/* Xmit end of frame indicator */
#define LAN_RECV_ALT	1	/* Multiple received frames reported on intr */
#define LAN_R_CSTAT_REQ	0x88FF	/* Recv cmd/stat on request */

