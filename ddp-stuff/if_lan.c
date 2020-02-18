/*
 * 5799-CGZ (C) COPYRIGHT IBM CORPORATION 1986
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */
/* $Header: if_lan.c,v 2.10 86/07/08 14:11:58 donna Exp $ */
/* $Source: /usr/sys/DONNA/if_lan.c,v $ */

#ifndef lint
static char *rcsid = "$Header: if_lan.c,v 2.10 86/07/08 14:11:58 donna Exp $";
#endif

/*
 * IBM Token-Ring Local Area Network Adapter Driver
 */

#include "lan.h"
#if NLAN > 0

#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/vmmac.h"
#include "../h/ioctl.h"
#include "../h/errno.h"
#include "../h/un.h"

#include "/usr/src/include/netdb.h"

#include "../net/if.h"
#include "../net/netisr.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/if_ether.h"
#include "../netpup/pup.h"

#include "../machine/io.h"
#include "../machineio/ioccvar.h"
#include "../machineio/dmareg.h"
#include "../machine/debug.h"
#include "../h/kernel.h"

#include "if_lanreg.h"
#include "if_landma.h"
#include "if_lanvar.h"
#include "if_lanio.h"

#ifdef DEBUG
char lan_debug = 0x00;			/* controls printf's for debugging */
#endif DEBUG

int lan_probe(), lan_attach();
int lan_intr(), lan_init(), lan_ioctl(), lan_output(), lan_reset();
unsigned short mior();
struct mbuf *lan_get();

caddr_t lan_std[] = {
	(caddr_t)0xf00001c0, (caddr_t)0xf0000140, 0
};
struct iocc_device *lan_info[NLAN];

struct iocc_driver landriver = {
	lan_probe, 0, lan_attach, 0, lan_std, "lan", lan_info,
	0, 0, lan_intr, LAN_CMDREG
};

int lan_initialize(), lan_ring_watch(), lan_thaw();
struct lan_softc lan_softc[NLAN];
char lan_xid_resp[LAN_L_XID_RESP] = { '\201', '\001', '\002'};

/*
 * lan_probe - adapter does not interrupt in this state
 */
lan_probe(addr)
	register caddr_t addr;
{
	DEBUGF(lan_debug, printf("lan adapter probed\n");
	);
	return (PROBE_NOINT);
}


/*
 * lan_attach - make interface available to network software if
 *		autoconfig determines that interface exists.
 */

lan_attach(iod)
	register struct iocc_device *iod;
{
	register struct lan_softc *lns = &lan_softc[iod->iod_unit];
	register struct ifnet *ifp = &lns->lns_if;
	register struct sockaddr_in *sin;

	ifp->if_unit = iod->iod_unit;
	ifp->if_name = "lan";
	ifp->if_mtu = LAN_MTU;
	sin = (struct sockaddr_in *) & lns->lns_if.if_addr;
	sin->sin_family = AF_INET;
	sin->sin_addr = arpmyaddr((struct arpcom *)0);
	ifp->if_init = lan_init;
	ifp->if_ioctl= lan_ioctl;
	ifp->if_output = lan_output;
	ifp->if_reset = lan_reset;
	if_attach(ifp);
	lns->lns_open_options = LAN_OPEN_OPTIONS;   /* DDP - Start with default */

	DEBUGF(lan_debug, printf("lan%d attached\n", iod->iod_unit);
	);
}


/*
 * lan_reset - reset interface
 */

lan_reset(unit)
	register unsigned int unit;
{
	register struct iocc_device *iod;

	if (unit < NLAN && (iod = lan_info[unit]) != 0 && iod->iod_alive != 0)
		lan_init(unit);
	DEBUGF(lan_debug, printf("lan%d reset\n", unit);
	);
}


/*
 * lan_init - initialize and open adapter
 */

lan_init(unit)
	register int unit;
{
	register struct lan_softc *lns = &lan_softc[unit];
	struct ifnet *ifp = &lns->lns_if;
	struct sockaddr_in *sin;
	register struct lan_device *addr;

	sin = (struct sockaddr_in *) &ifp->if_addr;
	if (sin->sin_addr.s_addr == 0)
		return;

	DEBUGF(lan_debug, printf("lan%d begin init\n", unit);
	);

	addr = (struct lan_device *)(lan_info[unit]->iod_addr);

	if((lns->lns_if.if_flags & IFF_RUNNING) == 0) {
		/* read node address pointer once */
		lns->lns_adapter &= LAN_ADAP_BIA_READ;
		if (lan_initialize(unit) == 0) {
			lns->lns_if.if_flags |= IFF_RUNNING;
			if (!(lns->lns_adapter & LAN_ADAP_BIA_READ)) {
				lns->lns_ctl->lan_bia.adap_addr = LAN_ADDRESSES;
				lns->lns_ctl->lan_bia.count = LAN_L_ADDR;
				lns->lns_ctl->lan_bia.flag |= LAN_ADDR_PENDING;
				lan_exec(LAN_RDADAPTR, &lns->lns_ctl->lan_bia, addr, unit);
			}
			DEBUGF(lan_debug, printf("lan%d: end init\n", unit);
			);

		} else {
			lns->lns_adapter |= LAN_ADAP_BROKEN;
			lns->lns_if.if_flags &= ~(IFF_RUNNING | IFF_UP);
		}
	}
	if(lns->lns_if.if_flags & IFF_RUNNING) {
		if((lns->lns_adapter & LAN_ADAP_OPEN) == 0) {
			lan_open(unit, addr);  /* Open adapter */
		}
		if_rtinit(&lns->lns_if, RTF_UP);
		arpattach(&lns->lns_ac);
	}
}


 /* end lan_init */

/*
 * lan_start - start output to the adapter
 */

lan_start(lns, addr, xbuf, unit)
	register struct lan_softc *lns;
	register struct lan_device *addr;
	register int xbuf;
	register int unit;
{

	lns->lns_oactive = 1;
	lns->lns_xbuf = xbuf;
	DEBUGF(lan_debug,
		printf("lan%d start xmit buf %x length = %x\n",
		unit, xbuf, lns->xp[xbuf]->frame_size);
	);
	/* execute transmit command */
	lan_exec(LAN_TRANSMIT, lns->xp[xbuf], addr, unit);
}


/*
 * lan_ssb_clear - clear adapter -> system interrupt
 */

lan_ssb_clear(addr)
	register struct lan_device *addr;
{
	miow(&(addr->lan_cmdstat), LAN_SSBCLEAR);
}


/*
 * lan_recv - get mbufs and initialize receive lists for reception
 *		into mbuf data area
 */

lan_recv(unit, addr)
	register int unit;
	register struct lan_device *addr;
{
	register struct lan_softc *lns = &lan_softc[unit];
	register struct lan_recv_ctl *rcv;
	struct mbuf *m, *p;
	register int tcw;
	int i;
	/*
	 * use an mbuf to hold
	 * receive lists and headers
	 */
	if(lns->lns_recv == 0) {
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == 0)
			return(ENOBUFS);
		lns->lns_recv = mtod(m, struct lan_recv_ctl *);
	}
	rcv = lns->lns_recv;
	tcw = lan_dma_setup(rcv, TCW_RESERVE, lns->lan_dma_chan);
	for (i = 0; i < 2; i++) {
		lns->rp[i] = &rcv->lan_rlist[i];
		lns->rh[i] = &rcv->list_hdr[i];
		/* set up lan header areas */
		lns->rp[i]->d_parm[0].d_cnt =
			(((short)(sizeof(struct list_hdr))) | LAN_CHAIN);
		lns->rp[i]->d_parm[0].d_haddr = DMA_HI_ADDR(tcw);
		lns->rp[i]->d_parm[0].d_laddr = DMA_LO_ADDR(lns->rh[i], tcw);
		lns->rp[i]->cstat = LAN_R_CSTAT_REQ;
		lns->rp[i]->xlp_h =
			DMA_HI_ADDR(tcw);
	}
	lns->rp[0]->xlp_l =
		DMA_LO_ADDR(lns->rp[1], tcw);
	lns->rp[1]->xlp_l = 
		DMA_LO_ADDR(lns->rp[0], tcw);
	/*
	 * use large mbufs for
	 * receive buffers
	 */
	for (i = 0; i < 2; i++) {
		if(lns->lns_rdata[i] == 0) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0)
				return(ENOBUFS);
			MCLGET(p, 1);
			if (p == 0)
				return(ENOBUFS);
			else
				m->m_off = (int)p - (int)m;
			lns->lns_rbufp[i] = m;
			lns->lns_rdata[i] = p;
		}
		tcw = lan_dma_setup(lns->lns_rdata[i],TCW_RESERVE, lns->lan_dma_chan);
		lns->rp[i]->d_parm[1].d_cnt = LAN_MTU & (~LAN_CHAIN);
		lns->rp[i]->d_parm[1].d_haddr = DMA_HI_ADDR(tcw);
		lns->rp[i]->d_parm[1].d_laddr = DMA_LO_ADDR(lns->lns_rdata[i], tcw);
	}
	return(0);
}


/*
 * lan_exec - serialize command requests to the adapter;
 *		do one at a time for now except for
 *		receive which needs to be outstanding.
 */
lan_exec(cmd, plist, addr, unit)
	register short cmd;
	register short *plist;
	register struct lan_device *addr;
	register int unit;
{
	struct lan_softc *lns = &lan_softc[unit];
	register struct lan_ctl *ctl = lns->lns_ctl;
	register int i;
	int tcw;
	int s = splimp();

	tcw = lan_dma_setup(plist, TCW_RESERVE, lns->lan_dma_chan);
	for (i = 5*ONESEC; i>0; i--) {
		DELAY(1);
		if (ctl->lan_scb.command == 0)  /* wait until scb cleared */
			break;
	}
	if(i>0) {
		ctl->lan_scb.command = cmd;
					/* DDP - Begin */
		if(cmd == LAN_SETFADDR || cmd == LAN_SETGADDR) {
		    ctl->lan_scb.h_addr = *plist++;
		    ctl->lan_scb.l_addr = *plist;
		} else {
					/* DDP - End */
		    ctl->lan_scb.h_addr = DMA_HI_ADDR(tcw);
		    ctl->lan_scb.l_addr = DMA_LO_ADDR(plist, tcw);
		}			/* DDP */
		DEBUGF(lan_debug,
			printf("lan%d exec cmd %x, addr %x %x\n",
				unit, ctl->lan_scb.command,
				ctl->lan_scb.h_addr, ctl->lan_scb.l_addr);
		);
		miow(&(addr->lan_cmdstat), LAN_EXECUTE);
	} else printf("lan%d: adapter jammed\n",unit);
	splx(s);
}


/*
 * lan_open - open adapter
 */

lan_open(unit, addr)
	register int unit;
	register struct lan_device *addr;
{
	register struct lan_softc *lns = &lan_softc[unit];
	register struct lan_ctl *ctl = lns->lns_ctl;

	if((lns->lns_adapter & LAN_OPEN_IN_PROGRESS) == 0) {
		lns->lns_adapter |= LAN_OPEN_IN_PROGRESS;
/* DDP - Begin */
		ctl->open_parm[0] = lns->lns_open_options;
		ctl->open_parm[1] = ctl->open_parm[2] = ctl->open_parm[3] = 0;
		ctl->open_parm[4] = ctl->lan_gaddr[0];
		ctl->open_parm[5] = ctl->lan_gaddr[1];
		ctl->open_parm[6] = ctl->lan_faddr[1];
		ctl->open_parm[7] = ctl->lan_faddr[1];
/* DDP - End */
		ctl->open_parm[8] = LAN_OPEN_RLIST;
		ctl->open_parm[9] = LAN_OPEN_XLIST;
		ctl->open_parm[10] = LAN_OPEN_BUFSIZE;
		ctl->open_parm[11] = LAN_OPEN_RAMSTART;
		ctl->open_parm[12] = LAN_OPEN_RAMEND;
		ctl->open_parm[13] = LAN_OPEN_XMINMAX;

		lan_exec(LAN_OPEN, ctl->open_parm, addr, unit);
		DEBUGF(lan_debug,
			printf("lan%d attempting to open addr %x %x %x\n",
				unit, ctl->open_parm[1],
				ctl->open_parm[2], ctl->open_parm[3]);
		);
	}
}


/*
 * lan_intr - adapter interrupt handler
 */

lan_intr(unit)
	register int unit;
{
	register struct lan_softc *lns = &lan_softc[unit];
	register struct lan_device *addr = (struct lan_device *)lan_info[unit]->iod_addr;
	struct lan_ctl *ctl = lns->lns_ctl;
	register struct mbuf *m;
	register short sifrbuf, sifwbuf;
	struct ifqueue *inq;
	struct mbuf *p;
	char type, *c;
	unsigned short len;
	register int next_buf;
	int k, i, tcw;
	short retparm[4];
	short *retptr;
	unsigned short *initptr, *shortptr;
	unsigned short open_phase, open_status;
	struct ifnet *ifp = &lns->lns_if;
	struct sockaddr_in *sin;
	int alt;

	sifrbuf = (mior(&(addr->lan_cmdstat)));
	if ((sifrbuf & LAN_INT) == 0) {
		return (1);
	}
	sifrbuf &= LAN_ADAP_INT;	/* interrupt type */
	DEBUGF(lan_debug,
		printf("lan%d int: status = 0x%b, code = 0x%x, ssb = %x,%x,%x,%x\n",
			unit, sifrbuf & 0xfff0, LAN_STAT_BITS, sifrbuf & 0x000f,
			ctl->lan_ssb.command, ctl->lan_ssb.status0,
			ctl->lan_ssb.status1, ctl->lan_ssb.status2);
	);

	switch (sifrbuf) {
	case LAN_RECVSTAT:
		/*
		 * Receive notify
		 *
		 * When frame complete, copy data to mbuf chain.
		 * Signal adapter to continue.  Then either enqueue IP 
		 * packets or process arp packets.
		 */

		/* determine buffer(s) from receive list address */
		if ((ctl->lan_ssb.status1 == lns->rp[1]->xlp_h) &&
			(ctl->lan_ssb.status2 == lns->rp[1]->xlp_l))
			i = 0;
			else i = 1;
		if (ctl->lan_ssb.status0 == LAN_FRAME_COMPLETE) {
			DEBUGF(lan_debug,
				printf("lan%d recv: status=0x%b, cstat=0x%b, length=%x\n",
					unit, ctl->lan_ssb.status0, LAN_RECV_BITS,
					lns->rp[i]->cstat, LAN_RCSTAT_BITS,
					lns->rp[i]->frame_size);
			);
		lan_ssb_clear(addr);
		alt = 0;
		/* if alternate buffer full, get that one first */
		if (lns->rp[1-i]->cstat & LAN_RCSTAT_COMPLETE) {
			alt=LAN_RECV_ALT;
			i=1-i;
			}
		check_alt:
			lns->lns_if.if_ipackets++;
			len = lns->rp[i]->frame_size - sizeof(struct list_hdr);
			if (len == 0)
				goto chk_xid;
			p = lns->lns_rdata[i];
			m = lan_get(p, len);
			type = lns->rh[i]->dsap;
			if (m == 0)
				goto clear;
		chk_xid:
#ifdef LANMAC							/* DDP - Begin */
			if(!(lns->rh[i]->pcf1 & LAN_PCF1)) {	/* Check for MAC frame */
			    struct mbuf *e;
			    extern struct sockaddr lanmac_src;
			    extern struct sockaddr lanmac_dst;
			    extern struct sockproto lanmac_proto;

/* Here m points to a chain of mbufs containing only the information field
 *  from the received frame.
 *  Now copy the header part of the frame.
 */
			    DEBUGF(lan_debug,
				printf("lan%d got MAC frame.\n", unit);
			    );
			    e = m_get(M_DONTWAIT, MT_HEADER);	/* get mbuf for header */
			    if (e == 0) {			/* free m if none available */
				m_free(m);
				goto clear;
			    }
			    e->m_next = m;
			    e->m_len = sizeof(struct list_hdr); 
			    *mtod(e, struct list_hdr *) = *(lns->rh[i]); /* make copy */

			    /* the frame goes to the raw socket used by the net manager */
			    raw_input(e, &lanmac_proto, &lanmac_src, &lanmac_dst);
			    goto clear;
			}
#endif LANMAC							/* DDP - End */
			if((lan_xid_test(m, i, unit)) || (len == 0))
				goto clear;
			switch (type) {
#ifdef INET
			case LAN_IPTYPE:
				schednetisr(NETISR_IP);
				inq = &ipintrq;
				break;
			case LAN_ARPTYPE:
				arpinput(&lns->lns_ac, m);
				goto clear;
#endif
			default:
				DEBUGF(lan_debug,
					printf("lan%d packet not recognized, freeing mbuf\n",
						unit);
				);
				m_freem(m);
				goto clear;
			}
			if (IF_QFULL(inq)) {
				DEBUGF(lan_debug, printf("lan%d: ip qfull\n", unit);
				);
				IF_DROP(inq);
				m_freem(m);
				goto clear;
			}
			IF_ENQUEUE(inq, m);

		}
		else lan_ssb_clear(addr);
		clear:
		lns->rp[i]->cstat = LAN_R_CSTAT_REQ;
		miow(&(addr->lan_cmdstat), LAN_RECVALID);
		if (alt) {
			alt = 0;
			i=1-i;
			goto check_alt;
			}
		break;

	case LAN_XMITSTAT:
		/*
		 * Transmit status
		 *
		 * If next buffer full, start output.
		 * Dequeue next IP buffer.
		 */

		lan_ssb_clear(addr);
		lns->lns_if.if_opackets++;

		if ((ctl->lan_ssb.status1 == lns->xp[1]->xlp_h) &&
			(ctl->lan_ssb.status2 == (lns->xp[1]->xlp_l & ~LAN_ODD_PTR)))
			next_buf = 1;
			else next_buf = 0;
		if (lns->xp[next_buf]->cstat & LAN_XMIT_VALID) {
			lan_start(lns, addr, next_buf, unit);
			if (lns->lns_if.if_snd.ifq_head) {
				struct mbuf *m;
				IF_DEQUEUE(&lns->lns_if.if_snd, m);
				if (lan_put(lns, m, 1 - next_buf, unit) != 0)
					m_freem(m);
			}
		} else
			lns->lns_oactive = 0;
		break;

	case LAN_ACHECK:
		/*
		 * Adapter check - Retry adapter initialization
		 */
		lns->lns_adapter |= LAN_ADAP_BROKEN;
		lns->lns_if.if_flags &= ~(IFF_RUNNING | IFF_UP);
		sifwbuf = LAN_ACHECK_DATA;
		shortptr = &(addr->lan_address);
		initptr = &(addr->lan_data);
		for (k = ADAP_CHK_SIZE, retptr = ((short *)retparm); k > 0; k--, retptr++) {
			miow(shortptr, sifwbuf);
			*retptr = mior(initptr);
			sifwbuf = sifwbuf + 2;
		}
		printf("lan%d: unrecoverable token ring adapter failure, ", unit);
		printf("adapter check field = 0x%b, parm0=0x%x, parm1=0x%x, parm2=0x%x\n",
			retparm[0], LAN_ACHECK_BITS, retparm[1], retparm[2], retparm[3]);
		if (lns->lns_freezer != 0) {
			lan_freeze(unit); /* freeze the adapter */
			wakeup(lns);
			timeout(lan_thaw, (caddr_t)unit, 180 * hz);
		} else {
			lan_ssb_clear(addr);
			timeout(lan_reset, (caddr_t)unit, 1 * hz);
		}
		if (retparm[0] & (XMIT_PARITY || XMIT_UNDERRUN)) {
			lns->lns_if.if_oerrors++;
		}
		if (retparm[0] & (RECV_PARITY || RECV_OVERRUN)) {
			lns->lns_if.if_ierrors++;
		}
		break;

	case LAN_RINGSTAT:
		/*
		 * Ring status 
		 *
		 * Processing depends on ring status.  For
		 * ring error set timer to check again in 60
		 * seconds to allow for possible ring
		 * recovery.  Certain statuses update adapter
		 * state.  Others informational only.
		 */

		DEBUGF(lan_debug,
			printf("lan%d ring status = %b\n",
				unit, ctl->lan_ssb.status0, LAN_RING_BITS);
		);
		if (ctl->lan_ssb.status0 & LAN_AUTOER1) {
			lns->lns_adapter |= LAN_ADAP_AUTOER1;
			lns->lns_if.if_flags &= ~IFF_UP;
			lan_ssb_clear(addr);
			lan_open(unit, addr);
			break;
		} else if (ctl->lan_ssb.status0 & LAN_REMOVE_RECV) {
			lns->lns_adapter &= ~LAN_ADAP_OPEN;
			lns->lns_adapter |= LAN_ADAP_DOWN;
			lns->lns_if.if_flags &= ~IFF_UP;
			printf("lan%d: removed from network by network manager.\n", unit);
		} else if (ctl->lan_ssb.status0 & LAN_WIRE_FAULT) {
			printf("lan%d: cable failure\n", unit);
			lan_ssb_clear(addr);
			lns->lns_adapter &= ~LAN_ADAP_OPEN;
			lns->lns_if.if_flags &= ~IFF_UP;
			lns->lns_ring |= LAN_CABLE_FAIL;
			if(!lns->lns_ring_watch) {
				lns->lns_ring_watch++;
				timeout(lan_ring_watch, (caddr_t)unit, SIXTY * hz);
			}
			break;
		} else if ( ctl->lan_ssb.status0 &
			(LAN_SIGNAL_LOSS | LAN_HARD_ERROR | LAN_XMIT_BEACON)
			) {
			if (lns->lns_ring & LAN_RECOVERY)
				lns->lns_ring = ctl->lan_ssb.status0 | LAN_RECOVERY;
			else {
				lns->lns_ring |= LAN_RECOVERY;
				if (ctl->lan_ssb.status0 & LAN_XMIT_BEACON)
					printf("lan%d: beaconing\n", unit);
				if(!lns->lns_ring_watch) {
					lns->lns_ring_watch++;
					timeout(lan_ring_watch, (caddr_t)unit, SIXTY * hz);
				}
			}
		}
		lns->lns_ring &= ~LAN_RECOVERY;
		if (ctl->lan_ssb.status0 & LAN_SINGLE) {
			printf("lan%d: single station on network\n", unit);
		}
		lan_ssb_clear(addr);
		break;


	case LAN_CMDSTAT:
		/*
		 * Command status 
		 *
		 * On open, examine completion status.  Set
		 * adapter state and, if failure, print
		 * informational message and retry open if error
		 * not permanent.  If success, set up transmit
		 * lists and begin output.
		 */

		DEBUGF(lan_debug, printf("lan%d cmd stat: %x %x %x %x\n",
			unit, ctl->lan_ssb.command, ctl->lan_ssb.status0,
			ctl->lan_ssb.status1, ctl->lan_ssb.status2);
		);
		if (ctl->lan_ssb.command == LAN_OPEN) {
			if (ctl->lan_ssb.status0 == LAN_OPEN_COMPLETE) {
				lan_ssb_clear(addr);
				DEBUGF(lan_debug, printf("lan%d open complete\n", unit);
				);
				lns->lns_adapter &= LAN_ADAP_BIA_READ;
				lns->lns_adapter |= LAN_ADAP_OPEN;
				lns->lns_ring &= ~(
					LAN_RECOVERY |
					LAN_XMIT_BEACON |
					LAN_CABLE_FAIL);
				ctl->lan_bia.adap_addr = lns->lns_node_addr;
				ctl->lan_bia.count = LAN_L_ADDR;
				lan_exec(LAN_RDADAPTR, &ctl->lan_bia, addr, unit);
			} else {
				lns->lns_adapter &=
					~(LAN_ADAP_OPEN | LAN_OPEN_IN_PROGRESS);
				/* adapter open error */
				lns->lns_if.if_flags &= ~IFF_UP;
				DEBUGF(lan_debug,
					printf("lan%d open failure, status = 0x%b, error = 0x%x\n",
						unit, ctl->lan_ssb.status0 & 0xff00,
						LAN_OPEN_STAT_BITS,
						ctl->lan_ssb.status0 & 0x00ff);
				);
				open_phase =
					ctl->lan_ssb.status0 & ~LAN_OPEN_PHASE_MASK;
				open_status =
					ctl->lan_ssb.status0 & LAN_OPEN_PHASE_MASK;
				lan_ssb_clear(addr);
				if (open_status & LAN_OPEN_NODE_ERROR) {
					printf("lan%d: node address error\n", unit);
					lns->lns_adapter |= LAN_ADAP_DOWN;
					break;
				} else if (lns->lns_adapter & LAN_ADAP_AUTOER1) {
					lns->lns_adapter &= ~LAN_ADAP_AUTOER1;
					printf("lan%d: hardware error, ", unit);
					printf("adapter removed from ring\n");
					break;
				}
					printf("lan%d: open err=0x%x",unit, open_status);
					printf(" %s\n", open_errmsg[open_status & 0x0f]);
					switch (open_status) {
					case LAN_OPEN_TIMEOUT:
					case LAN_OPEN_OREQ_PARM:
					case LAN_OPEN_OIMPL:
						if(lns->lns_open_retries < LAN_MAX_OPEN_RETRY)
						{
							lns->lns_open_retries++;
							lan_open(unit,addr);
						}
						else lns->lns_open_retries = 0;
						break;
					case LAN_OPEN_FUNC_FAILURE:
						if ((open_phase == LAN_OPEN_LOBE_TEST) || (open_phase == LAN_OPEN_INSERTION))
						{
							lns->lns_ring |= LAN_CABLE_FAIL;
							if(!lns->lns_ring_watch) {
								lns->lns_ring_watch++;
								timeout(lan_ring_watch, (caddr_t)unit, SIXTY * hz);
							}
						}
						else
						if(lns->lns_adapter & LAN_ADAP_FCTNFAIL == 0)
						{
							lns->lns_adapter |= LAN_ADAP_FCTNFAIL;
							lan_open(unit,addr);
						}
						break;
					case LAN_OPEN_OSIGNAL_LOSS:
					case LAN_OPEN_RING_FAILURE:
					case LAN_OPEN_RING_BEACON:
						if(!lns->lns_ring_watch) {
							lns->lns_ring_watch++;
							timeout(lan_ring_watch, (caddr_t)unit, SIXTY * hz);
						}
					}
			}
		} else if (ctl->lan_ssb.command == LAN_RDADAPTR) {
/*			if (lns->lns_reader != 0) {
			    wakeup(lns);
			}
*/
			lan_ssb_clear(addr);
			if (ctl->lan_bia.flag & LAN_ADDR_PENDING) {
				lns->lns_node_addr = ctl->lan_bia.count;
				ctl->lan_bia.flag &= ~LAN_ADDR_PENDING;
			} else {
				bcopy(&ctl->lan_bia.count, lns->lns_addr, LAN_L_ADDR);
				printf ("lan%d: address ",unit);
				c = (char *)lns->lns_addr;
				for (i = 0; i < LAN_L_ADDR; i++) {
					if (i != 0) printf (":");
					printf ("%x", *c++);
				}
				printf("\n");
				/* allocate transmit list buffers very first time */
				if ((lns->lns_adapter & LAN_ADAP_BIA_READ) == 0) {
					/* allocate xmit lists */
					for (i = 0; i < LAN_XMITLIST_CT; i++) {
						MGET(m, M_DONTWAIT, MT_DATA);
						if (m == 0)
							goto lan_mbuf_fail;
						MCLGET(p, 1);
						if (p == 0)
							goto lan_mbuf_fail;
						else
							m->m_off = (int)p - (int)m;
						lns->xp[i] = (struct lan_list *)p;
						lns->lns_xbufp[i] = (char *) ((int)lns->xp[i] + sizeof(struct lan_list));
						tcw = lan_dma_setup(lns->lns_xbufp[i],TCW_RESERVE, lns->lan_dma_chan);
						lns->xp[i]->d_parm[0].d_haddr = DMA_HI_ADDR(tcw);
						lns->xp[i]->d_parm[0].d_laddr = DMA_LO_ADDR(lns->lns_xbufp[i], tcw);
					}
					lns->lns_oactive = 0;
					lns->lns_adapter |= LAN_ADAP_BIA_READ;
				} 
				for (i = 0; i < LAN_XMITLIST_CT; i++) {
					tcw = lan_dma_setup(lns->xp[i],TCW_RESERVE,lns->lan_dma_chan);
					lns->xp[1-i]->xlp_h =
						DMA_HI_ADDR(tcw);
					lns->xp[1-i]->xlp_l =
						(DMA_LO_ADDR(lns->xp[i], tcw)) | LAN_ODD_PTR;
					lns->xp[i]->cstat = LAN_XCSTAT_COMPLETE;
				}
				/* reset receive lists */
				if (lan_recv(unit, addr) != 0) goto lan_mbuf_fail;
				lan_exec(LAN_RECEIVE, lns->rp[0], addr, unit);
				lns->lns_if.if_flags |= IFF_UP;
				sin = (struct sockaddr_in *) & ifp->if_addr;
				arpwhohas(&lns->lns_ac, &sin->sin_addr);
				if (lns->lns_if.if_snd.ifq_head) {
					struct mbuf *m;

					IF_DEQUEUE(&lns->lns_if.if_snd, m);
					if (lan_put(lns, m, 0, unit) == 0) {
						lan_start(lns, addr, 0, unit);
						if (lns->lns_if.if_snd.ifq_head) {
							IF_DEQUEUE(&lns->lns_if.if_snd,
								m);
							if (lan_put(lns, m, 1, unit) != 0)
								m_freem(m);
						}
					} else
						m_freem(m);
				}
			}
		} else if (ctl->lan_ssb.command == LAN_CLOSE) {
			if (ctl->lan_ssb.status0 == LAN_CLOSE_COMPLETE) {
				lns->lns_adapter |= LAN_ADAP_DOWN;
				DEBUGF(lan_debug, printf("lan%d closed\n", unit);
				);
			}
			lan_ssb_clear(addr);
					/* DDP - Begin */
		} else if (ctl->lan_ssb.command == LAN_SETFADDR) {
			if (ctl->lan_ssb.status0 == LAN_SETFADDR_COMPLETE) {
				DEBUGF(lan_debug, printf("lan%d functional address set.\n", unit);
				);
			}
			lan_ssb_clear(addr);
		} else if (ctl->lan_ssb.command == LAN_SETGADDR) {
			if (ctl->lan_ssb.status0 == LAN_SETGADDR_COMPLETE) {
				DEBUGF(lan_debug, printf("lan%d group address set.\n", unit);
				);
			}
			lan_ssb_clear(addr);
		} else if (ctl->lan_ssb.command == LAN_RDERRORLOG) {
			if (ctl->lan_ssb.status0 == LAN_RDERRORLOG_COMPLETE) {
				DEBUGF(lan_debug, printf("lan%d done reading error log.\n", unit);
				);
				if (lns->lns_reader != 0) {
				    wakeup(lns);
				}
			}
			lan_ssb_clear(addr);
					/* DDP - End */
		} else {
			if (ctl->lan_ssb.command == LAN_SSB_REJECT) { /* DDP Add Missing { */
				printf("lan%d: command reject, internal software error, ",
					unit);
				printf("reject reason = 0x%b, command = 0x%x.\n",
					ctl->lan_ssb.status0, LAN_CMD_REJ_BITS,
					ctl->lan_ssb.status1);
			}			/* DDP */
			lan_ssb_clear(addr);
		}
		break;
lan_mbuf_fail:
		printf("lan%d: mbufs.\n", unit);
		break;


	default:
		DEBUGF(lan_debug,
			printf("lan%d unexpected interrupt: status = 0x%b, code = 0x%x\n",
				unit, sifrbuf & 0xfff0, LAN_STAT_BITS, sifrbuf & 0x000f);
		);
		lan_ssb_clear(addr);
		break;
	}
	/* end switch */

	*LAN_IRQ12 = 0;			/* enable interrupt level */
	return (0);
}


/*
 * lan_ioctl - adapter ioctl
 */
lan_ioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	register int cmd;
	register caddr_t data;
{
	register struct ifreq *ifr = (struct ifreq *)data;
	register int s = splimp();
	register int error = 0;
	register struct lan_softc *lns = &lan_softc[ifp->if_unit];
	struct lan_device *addr = (struct lan_device *)
	lan_info[ifp->if_unit]->iod_addr;
	unsigned short *dump, dump_data;
	int i, j;

	switch (cmd) {
	case SIOCSIFADDR:
		if (ifp->if_flags & IFF_RUNNING)
			if_rtinit(ifp, -1); /* delete previous route */
		lan_setaddr(ifp, (struct sockaddr_in *) & ifr->ifr_addr);
		lan_init(ifp->if_unit);
		break;
	case SIOCSIFFLAGS:
		if((ifr->ifr_flags & IFF_UP)
			&& ((lns->lns_adapter & LAN_ADAP_OPEN) == 0)) {
			lan_init(ifp->if_unit);
		} else if (((ifr->ifr_flags & IFF_UP) == 0)
			&& (lns->lns_adapter & LAN_ADAP_OPEN)) {
			lan_close(ifp->if_unit);
			}
		break;
	case SIOCSIFFADDR:	/* DDP - Begin */
		DEBUGF(lan_debug,
		    printf("lan%d got SIOCSIFFADDR ioctl.\n", ifp->if_unit);
		);
		if(!suser())
		    return(u.u_error);
		lan_set_faddr(ifp->if_unit, (struct sockaddr *) & ifr->ifr_addr);
		break;
	case SIOCGIFFADDR:
		DEBUGF(lan_debug,
		    printf("lan%d got SIOCGIFFADDR ioctl.\n", ifp->if_unit);
		);
		lan_get_faddr(ifp->if_unit, (struct sockaddr *) & ifr->ifr_addr);
		break;
	case SIOCSIFGADDR:
		DEBUGF(lan_debugg,
		    printf("lan%d got SIOCSIFGADDR ioctl.\n", ifp->if_unit);
		);
		if(!suser())
		    return(u.u_error);
		lan_set_gaddr(ifp->if_unit, (struct sockaddr *) & ifr->ifr_addr);
		break;
	case SIOCGIFGADDR:
		DEBUGF(lan_debugg,
		    printf("lan%d got SIOCGIFGADDR ioctl.\n", ifp->if_unit);
		);
		lan_get_gaddr(ifp->if_unit, (struct sockaddr *) & ifr->ifr_addr);
		break;
	case SIOCSIFOPENOPT:
		DEBUGF(lan_debugg,
		    printf("lan%d got SIOCSIFOPENOPT ioctl.\n",	ifp->if_unit);
		);
		if(!suser())
		    return(u.u_error);
		lan_set_open_opt(ifp->if_unit, ifr->ifr_flags);
		break;
	case SIOCGIFOPENOPT:
		DEBUGF(lan_debugg,
		    printf("lan%d got SIOCGIFOPENOPT ioctl.\n",
			ifp->if_unit);
		);
		lan_get_open_opt(ifp->if_unit, &ifr->ifr_flags);
		break;
	case SIOCRDERRORLOG:
		DEBUGF(lan_debugg,
		    printf("lan%d got SIOCRDERRORLOG ioctl.\n",
			ifp->if_unit);
		);
		lan_read_errlog(ifp->if_unit, ifr->ifr_data);
		break;
	case SIOCRDADAPBUFF:
		DEBUGF(lan_debugg,
		    printf("lan%d got SIOCRDADAPBUFF ioctl.\n",
			ifp->if_unit);
		);
		if(!suser())	    /* Bad pointers can crash the card */
		    return(u.u_error);
		lan_read_adapter(ifp->if_unit, ifr->ifr_data);
		break;		/* DDP - End */
	case SIOCSLANDUMP:
		if(!suser())
		    return(u.u_error);
		lns->lns_freezer = u.u_procp;
		sleep(lns,PZERO+1);
		break;
	case SIOCFLANDUMP:
		if(!suser())
		    return(u.u_error);
		lan_freeze(ifp->if_unit); /* freeze the adapter */
		timeout(lan_thaw, (caddr_t)ifp->if_unit, 180 * hz);
		break;
	case SIOCGLANDUMP:
		if(!suser())
		    return(u.u_error);
		if (lns->lns_adapter & LAN_ADAP_FROZEN) {
			dump = ((struct lan_dump *)ifr->ifr_data)->lan_dump_data;
			for (j = 0; j < LAN_FREEZE_DUMP / LAN_FREEZE_CHUNK; j++) {
				if (j) {
					miow(&(addr->lan_cmdstat), LAN_FREEZE_INCR);
					DELAY(LAN_ADAP_MIN_RESET);
				}
				miow(&(addr->lan_address), 0);
				for (i = 0; i < LAN_FREEZE_CHUNK / 2; i++, dump++) {
					dump_data = mior(&(addr->lan_datai));
					if(copyout( (caddr_t)&dump_data,
						 (caddr_t)dump,sizeof(short)) !=0) {
						goto dump_done;
					}
				}
			}
		dump_done:
			lan_unfreeze(ifp->if_unit);
			timeout(lan_reset, (caddr_t)(ifp->if_unit), 1 * hz);
		}
		break;
	default:
		error = EINVAL;
	}
	splx(s);
	return (error);
}


/*
 * lan_output - token ring output routine
 */
lan_output(ifp, m0, dst)
	register struct ifnet *ifp;
	register struct mbuf *m0;
	struct sockaddr *dst;
{
	int error;
	char type;
	u_char edst[LAN_L_ADDR];
	struct in_addr idst;
	register struct lan_softc *lns = &lan_softc[ifp->if_unit];
	register struct mbuf *m = m0;
	register struct ether_header *un;
	extern struct ifnet loif;

	DEBUGF(lan_debug, printf("lan%d output request\n", ifp->if_unit);
	);

	switch (dst->sa_family) {
#ifdef INET
	case AF_INET:
		idst = ((struct sockaddr_in *)dst)->sin_addr;
		if (!arpresolve(&lns->lns_ac, m, &idst, edst))
			return (0);
		type = LAN_IPTYPE;
		break;
#endif
	case AF_UNSPEC:
		un = (struct ether_header *)dst->sa_data;
		bcopy((caddr_t)un->ether_dhost, (caddr_t)edst, sizeof(edst));
		type = LAN_ARPTYPE;
		break;
	default:
		printf("lan%d: can't handle af%d.\n", ifp->if_unit, dst->sa_family);
		error = EAFNOSUPPORT;
		goto bad;
	}
	return (lan_output_llc(ifp, m0, edst, LAN_UI_CMD, type));
bad:
	m_freem(m0);
	return (error);
}


lan_output_llc(ifp, m0, dst, llc_ctl, type)
	register struct ifnet *ifp;
	register struct mbuf *m0;
	u_char * dst;
	char llc_ctl;
	char type;
{
	int s, error;
	u_char edst[LAN_L_ADDR];
	struct lan_device *addr = (struct lan_device *)
	lan_info[ifp->if_unit]->iod_addr;
	register struct lan_softc *lns = &lan_softc[ifp->if_unit];
	register struct mbuf *m = m0;
	register struct list_hdr *lan;
	extern struct ifnet loif;
	/* Add token-ring header */

	m = m_get(M_DONTWAIT, MT_HEADER);
	if (m == 0) {
		error = ENOBUFS;
		goto bad;
	}
	m->m_next = m0;
	m->m_off = MMINOFF;
	m->m_len = sizeof(struct list_hdr);
	lan = mtod(m, struct list_hdr *);
	bcopy((caddr_t)dst, (caddr_t)lan->to_addr, sizeof(edst));
	bcopy((caddr_t)lns->lns_addr, (caddr_t)lan->from_addr, LAN_L_ADDR);
	lan->dsap = type;
	lan->ssap = type;
	if(llc_ctl != LAN_UI_CMD)
		lan->ssap |= 0x01;
	lan->llc_ctl = llc_ctl;
	lan->pcf0 = LAN_PCF0;
	lan->pcf1 = LAN_PCF1;
	s = splimp();
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		error = ENOBUFS;
		goto qfull;
	}
	if (!(lns->lns_adapter & (LAN_ADAP_BROKEN | LAN_ADAP_DOWN))) {
		if (lns->lns_adapter & LAN_ADAP_OPEN) {
			int next_buf=lns->lns_nextbuf;
			if(lns->xp[next_buf]->cstat & LAN_XCSTAT_COMPLETE) {
				if (lan_put(lns, m, next_buf, ifp->if_unit) == 0) {
					DEBUGF(lan_debug,
						printf("lan%d xmit buf %x filled\n",
							ifp->if_unit, next_buf);
					);
					if (lns->lns_oactive == 0)
						lan_start(lns, addr, next_buf, ifp->if_unit);
				} else
					m_freem(m);
			} else
				IF_ENQUEUE(&ifp->if_snd, m);
		} else {
			if ((lns->lns_adapter & LAN_OPEN_IN_PROGRESS) == 0) {
				lan_open(ifp->if_unit, addr);
			}
			IF_ENQUEUE(&ifp->if_snd, m);
		}
	} else
		m_freem(m);

	splx(s);
	return (0);

qfull:
	m0 = m;
	splx(s);
	DEBUGF(lan_debug, printf("lan%d IP output queue full\n", ifp->if_unit);
	);
bad:
	m_freem(m0);
	return (error);
}


/*
 * lan_ring_watch
 *
 * This routine is entered upon expiration of the 60-second interval
 * timer set to examine ring state and print status message.
 * If necessary, attempt to reopen the adapter.
 */

lan_ring_watch(unit)
	register int unit;
{
	register struct lan_device *addr = (struct lan_device *)lan_info[unit]->iod_addr;
	register struct lan_softc *lns = &lan_softc[unit];
	int s;

	if (lns->lns_ring & LAN_RECOVERY) {
		if (lns->lns_ring & LAN_XMIT_BEACON)
			printf("lan%d: beaconing\n", unit);
	}
	s = splimp();
	if ((lns->lns_adapter & (
		LAN_OPEN_IN_PROGRESS |
		LAN_ADAP_OPEN |
		LAN_ADAP_BROKEN |
		LAN_ADAP_DOWN
	)) == 0) {
		lan_open(unit, addr);
		timeout(lan_ring_watch, (caddr_t)unit, SIXTY * hz);
	}
	else lns->lns_ring_watch = 0;
	splx(s);
}

/*
 * lan_put - copy mbufs into large xmit mbuf already mapped for dma operation
 */

lan_put(lns, m, xbuf, unit)
	struct lan_softc *lns;
	struct mbuf *m;
	int xbuf;
	int unit;
{
	struct lan_device *addr = (struct lan_device *)lan_info[unit]->iod_addr;
	struct mbuf *mp;
	register struct lan_list *xcp;
	register int total_length = 0;
	int s;
	register char *bp;

	xcp = lns->xp[xbuf];
	xcp->xlp_l |= LAN_ODD_PTR;

	bp = ((char *) lns->lns_xbufp[xbuf]);
	for (mp = m; mp; mp = mp->m_next) {
		register int len = mp->m_len;

		bcopy(mtod(mp, char *), bp, len);
		bp += len;
		total_length += len;
	}
	xcp->frame_size = total_length;
	xcp->d_parm[0].d_cnt = total_length & (~LAN_CHAIN);
	lns->lns_nextbuf = 1-xbuf;
	s = splimp();
	xcp->cstat = LAN_X_CSTAT_REQ;
	if(lns->lns_oactive) {
		lns->xp[1-xbuf]->xlp_l &= ~LAN_ODD_PTR;
		miow(&(addr->lan_cmdstat), LAN_XMTVALID);
	}
	splx(s);
	m_freem(m);
	return (0);
}

/*
 * lan_get - copy from driver receive buffers into mbuf's
 */
struct mbuf *lan_get(faddr, totlen)
	u_char * faddr;
	register unsigned short totlen;
{
	register struct mbuf *m;
	struct mbuf *top = 0;
	register struct mbuf **mp = &top;
	register unsigned short len;
	register u_char * cp;

	cp = faddr;
	while (totlen > 0) {
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == 0)
			goto bad;
		len = totlen;
		if (len >= CLBYTES) {
			register struct mbuf *p;

			MCLGET(p, 1);
			if (p != 0) {
				m->m_len = len = CLBYTES;
				m->m_off = (int)p - (int)m;
			} else {
				m->m_len = len = MIN(MLEN, len);
				m->m_off = MMINOFF;
			}
		} else {
			m->m_len = len = MIN(MLEN, len);
			m->m_off = MMINOFF;
		}
		bcopy(cp, mtod(m, char *), len);
		cp += len;
		*mp = m;
		mp = &m->m_next;
		totlen -= len;
	}
	return (top);

bad:

	DEBUGF(lan_debug, printf("lan mbuf request failed\n");
	);
	if (top != 0)
		m_freem(top);
	return (0);
}


/*
 * lan_initialize - initialize adapter;
 *
 * Initialization consists of 3 phases:
 *	1) check of bring-up diagnostics
 *	2) transfer of initialization parameters
 *	3) dma interface check.
 */

lan_initialize(unit)
	register int unit;
{
	register struct lan_softc *lns = &lan_softc[unit];
	register struct lan_device *addr;
	register struct lan_ctl *ctl;
	int retry, success, failure, j, k; /* loop controls */
	unsigned short initparm[LAN_NUM_IPARMS];
	register unsigned short *parmptr, *initptr;
	unsigned short sifrbuf, sifwbuf;
	register unsigned short *shortptr;
	int tcw = 0;
	struct mbuf *m;
	char c;

	/*
	 * assume initial attempt plus
	 * 3 more retries of full procedure
	 */

	DEBUGF(lan_debug, printf("lan%d initialization\n", unit);
	);
	addr = (struct lan_device *)(lan_info[unit]->iod_addr);

	/* disable dma channel */
	if ((caddr_t)addr == lan_std[0]) {
		lns->lan_dma_chan = DM_CHAN5;
		*((char *)CTL2_SMASK) = (CH_DISABLE | CH5);
	} else {
		lns->lan_dma_chan = DM_CHAN6;
		*((char *)CTL2_SMASK) = (CH_DISABLE | CH6);
	}
	lan_dma_init(lns->lan_dma_chan);

	/* begin adapter initialization */
	for (retry = LAN_RETRY; retry > 0; retry--) {
		shortptr = &(addr->lan_cmdstat);
		miow(shortptr, LAN_RESET);
		/* check bring-up diagnostics results */
		DEBUGF(lan_debug,
			printf("lan%d init retry %d\n", unit, LAN_RETRY - retry);
		);
		for (j = LAN_RESET_WAIT, success = failure = 0; j > 0; j--) {
			DELAY(TEN_MS);
			sifrbuf = mior(shortptr);
			DEBUGF(lan_debug,
				printf("lan%d status = 0x%b, int/err code = 0x%x\n",
					unit, sifrbuf & 0xfff0,
					LAN_STAT_BITS, sifrbuf & 0x000f);
			);
			if (sifrbuf & LAN_INITIALIZE) {
				if ((sifrbuf &
					(LAN_TEST |
					LAN_ERROR |
					LAN_ADAP_INT)) == 0) {
					/* diagnostics successful */
					success++;
					break;
			} else if ((sifrbuf & LAN_TEST) &&
					(sifrbuf & LAN_ERROR)) {
					/* unrecoverable error */
					failure++;
					break;
				}
			}
		}
		DEBUGF(lan_debug, printf("lan%d pods complete\n", unit);
		);			/* end bring-up diagnostics */

		if (success) {		/* diagnostics ok,now handshake */
			/* transfer initialization parms */
			*LAN_IRQ12 = 0;	/* enable interrupt level */
			success = 0;
			sifrbuf = mior(&(addr->lan_enable));
			/*
			 * use an mbuf to store scb, ssb
			 * to guarantee alignment
			 */
			if(lns->lns_ctl == 0) {
				MGET(m, M_DONTWAIT, MT_DATA);
				if (m == 0) {
					failure++;
					break;
				}
				ctl = lns->lns_ctl = mtod(m, struct lan_ctl *);
				ctl->lan_gaddr[0] = ctl->lan_gaddr[1] = 0; /* DDP */
				ctl->lan_faddr[0] = ctl->lan_faddr[1] = 0; /* DDP */
			}
			tcw = lan_dma_setup(lns->lns_ctl, TCW_RESERVE, lns->lan_dma_chan);
			initparm[0] = LAN_INIT_OPTIONS;
			initparm[1] = initparm[2] = initparm[3] = 0;
			initparm[4] = LAN_INIT_RBURST;
			initparm[5] = LAN_INIT_XBURST;
			initparm[6] = LAN_INIT_ABORT;
			initparm[7] = DMA_HI_ADDR(tcw);
			initparm[8] = DMA_LO_ADDR(lns->lns_ctl, tcw);
			initparm[9] = initparm[7];
			initparm[10] = DMA_LO_ADDR(&lns->lns_ctl->lan_ssb, tcw);
			/* write adapter init parameters */
			miow(&(addr->lan_address), LAN_INIT_DATAA);
			initptr = &(addr->lan_datai);
			for (k = LAN_NUM_IPARMS, parmptr = initparm;
				k > 0;
				k--, parmptr++) {
				miow(initptr, *parmptr);
			}
			/* read status to clear */
			c = *((char *)CTL2_CMD);
			if (lns->lan_dma_chan == DM_CHAN5) {
				/* set page mode for dma */
				*((char *)DMRA) &= ~CH5_PAGE;
				*((char *)CTL2_SMASK) = (CH_ENABLE | CH5);
				*((char *)CTL2_MODE) = (DM_CASCADE | CH5);
			} else {
				/* set page mode for dma */
				*((char *)DMRA) &= ~CH6_PAGE;
				*((char *)CTL2_SMASK) = (CH_ENABLE | CH6);
				*((char *)CTL2_MODE) = (DM_CASCADE | CH6);
			}
			miow(&(addr->lan_cmdstat), LAN_EXECUTE);

			/*
			 * wait at least 10 seconds before detecting
			 * initialization error to allow for dma timeout
			 */

			for (k = LAN_DMA_TIMEOUT + 1; k > 0; k--) {
				DELAY(TEN_MS);
				sifrbuf = mior(&(addr->lan_cmdstat));
				if ((sifrbuf &
					(LAN_INITIALIZE |
					LAN_TEST |
					LAN_ERROR)) == 0) {
					DEBUGF(lan_debug,
						printf("lan%d dma xface test ok\n",
							unit);
					);
					success++;
					break;
				} else if (sifrbuf & LAN_ERROR) {
					DEBUGF(lan_debug,
						printf("lan%d dma xface error, ",
							unit);
						printf("status = 0x%b, code = 0x%x\n",
							sifrbuf & 0xfff0,
							LAN_STAT_BITS,
							sifrbuf & 0x000f);
					);
					break;
				}
			}
		}
		/* end handshake */
		if (success)
			return (0);
	}
	/* retry init procedure */
	printf("lan%d: token ring adapter initialization failure, status = 0x%b, error code = 0x%x.\n",
		unit, sifrbuf & 0xfff0, LAN_STAT_BITS, sifrbuf & 0x000f);
	return (LAN_INIT_ERROR);
}


 /* end lan_initialize */


/*
 * lan_setaddr - set adapter's internet address
 */
lan_setaddr(ifp, sin)
	register struct ifnet *ifp;
	register struct sockaddr_in *sin;
{
	ifp->if_addr = *(struct sockaddr *)sin;
	ifp->if_net = in_netof(sin->sin_addr);
	ifp->if_host[0] = in_lnaof(sin->sin_addr);
	sin = (struct sockaddr_in *) & ifp->if_broadaddr;
	sin->sin_family = AF_INET;
	sin->sin_addr = if_makeaddr(ifp->if_net, INADDR_ANY);
	ifp->if_flags |= IFF_BROADCAST;
}


/*
 * close - terminate communication on ring
 */
lan_close(unit)
	register int unit;
{
	register struct lan_softc *lns = &lan_softc[unit];
	register struct lan_device *addr = (struct lan_device *)lan_info[unit]->iod_addr;
	register struct lan_ctl *ctl = lns->lns_ctl;

	lns->lns_if.if_flags &= ~IFF_UP;
	lns->lns_adapter &= ~(LAN_ADAP_OPEN | LAN_OPEN_IN_PROGRESS);
	ctl->close_parm[0] = ctl->close_parm[1] = 0;   /* DDP */
	lan_exec(LAN_CLOSE, ctl->close_parm, addr, unit);
}


/*
 * freeze - freeze adapter to enable internal storage dump
 */
lan_freeze(unit)
	register int unit;
{
	register struct lan_softc *lns = &lan_softc[unit];
	register struct lan_device *addr = (struct lan_device *)lan_info[unit]->iod_addr;
	register int s = splimp();
	register int i;

	lns->lns_adapter |= LAN_ADAP_FROZEN;
	lns->lns_if.if_flags &= ~(IFF_RUNNING | IFF_UP);
	/*
	 * generate pulses on adapter reset line
	 * to freeze; write to sif cmd reg for
	 * microcode level 12 compatibility
	 */
	for (i = 0; i < LAN_ADAP_FREEZE_PULSES; i++) {
		if (i)
			DELAY(LAN_ADAP_FREEZE_DELAY);
		miow(&(addr->lan_hreset), 0);
		DELAY(LAN_ADAP_MIN_RESET);
		miow(&(addr->lan_hreset), 0);
	}
	*(char *)(&(addr->lan_cmdstat)) = LAN_FREEZE_SIG_u12;
	splx(s);
}


/*
 * unfreeze - unfreeze adapter
 */
lan_unfreeze(unit)
	register int unit;
{
	register struct lan_softc *lns = &lan_softc[unit];
	register struct lan_device *addr = (struct lan_device *)lan_info[unit]->iod_addr;
	register int s = splimp();

	/* pulses hardware reset line to unfreeze */
	miow(&(addr->lan_hreset), 0);
	DELAY(LAN_ADAP_MIN_RESET);
	miow(&(addr->lan_hreset), 0);
	lns->lns_adapter &= ~LAN_ADAP_FROZEN;
	splx(s);
}


/*
 * thaw - release adapter frozen state
 */
lan_thaw(unit)
	register int unit;
{
	register struct lan_softc *lns = &lan_softc[unit];

	if (lns->lns_adapter & LAN_ADAP_FROZEN) {
		lan_unfreeze(unit);
		timeout(lan_reset, (caddr_t)unit, 1 * hz);
	}
}


/*
 * lan_read_adapter - transfer adapter storage to system
 */
lan_read_adapter(unit, data)
	register int unit;
	register caddr_t data;
{
#ifdef notdef
	register struct lan_softc *lns = &lan_softc[unit];
	register struct lan_device *addr = (struct lan_device *)lan_info[unit]->iod_addr;
	register struct lan_ctl *ctl = lns->lns_ctl;

	sleep(lns,PZERO+1);
	lan_exec(LAN_RDADAPTR, ctl->lan_faddr, addr, unit);
#endif
}


/*
 * lan_read_errlog - read and reset adapter error log
 */
lan_read_errlog(unit, data, errlog)
	register int unit;
	register caddr_t errlog;
{
	register struct lan_softc *lns = &lan_softc[unit];
	register struct lan_device *addr = (struct lan_device *)lan_info[unit]->iod_addr;
	register struct lan_ctl *ctl = lns->lns_ctl;

#ifdef notdef
	if((lns->lns_adapter & LAN_OPEN_IN_PROGRESS) == 0) {
		lns->lns_adapter |= LAN_OPEN_IN_PROGRESS;
#endif
	lan_exec(LAN_RDERRORLOG, &ctl->lan_errlog, addr, unit);
	DEBUGF(lan_debug,
		printf("lan%d attempting to read adapter errlog.\n", unit);
	);
	lns->lns_reader = u.u_procp;
	sleep(lns,PZERO+1);
	copyout(&ctl->lan_errlog, errlog, sizeof(ctl->lan_errlog));
}


/*
 * lan_set_faddr - reset adapter functional address after open
 */
lan_set_faddr(unit, sin)
	register int unit;
	register struct sockaddr *sin;
{
	register struct lan_softc *lns = &lan_softc[unit];
	register struct lan_device *addr = (struct lan_device *)lan_info[unit]->iod_addr;
	register struct lan_ctl *ctl = lns->lns_ctl;

	ctl->lan_faddr[0] = *(unsigned short *)(&(sin->sa_data[0]));
	ctl->lan_faddr[1] = *(unsigned short *)(&(sin->sa_data[2]));
	lan_exec(LAN_SETFADDR, ctl->lan_faddr, addr, unit);
	DEBUGF(lan_debug,
	    printf("lan%d setting functional address to %x%x\n",
		unit, ctl->lan_faddr[0], ctl->lan_faddr[1]);
	);
}


/*
 * lan_get_faddr - get adapter functional address after lan_set_faddr
 */
lan_get_faddr(unit, sin)
	register int unit;
	register struct sockaddr *sin;
{
	register struct lan_softc *lns = &lan_softc[unit];
	register struct lan_ctl *ctl = lns->lns_ctl;

	*(unsigned short *)(&(sin->sa_data[0])) = ctl->lan_faddr[0];
	*(unsigned short *)(&(sin->sa_data[2])) = ctl->lan_faddr[1];
}


/*
 * lan_set_gaddr - reset adapter group address after open
 */
lan_set_gaddr(unit, sin)
	register int unit;
	register struct sockaddr *sin;
{
	register struct lan_softc *lns = &lan_softc[unit];
	register struct lan_device *addr = (struct lan_device *)lan_info[unit]->iod_addr;
	register struct lan_ctl *ctl = lns->lns_ctl;

	ctl->lan_gaddr[0] = *(unsigned short *)(&(sin->sa_data[0]));
	ctl->lan_gaddr[1] = *(unsigned short *)(&(sin->sa_data[2]));
	lan_exec(LAN_SETGADDR, ctl->lan_gaddr, addr, unit);
	DEBUGF(lan_debug,
	    printf("lan%d setting group address to %x%x\n",
		unit, ctl->lan_gaddr[0], ctl->lan_gaddr[1]);
	);
}


/*
 * lan_get_gaddr - get adapter group address after lan_set_gaddr
 */
lan_get_gaddr(unit, sin)
	register int unit;
	register struct sockaddr *sin;
{
	register struct lan_softc *lns = &lan_softc[unit];
	register struct lan_ctl *ctl = lns->lns_ctl;

	*(unsigned short *)(&(sin->sa_data[0])) = ctl->lan_gaddr[0];
	*(unsigned short *)(&(sin->sa_data[2])) = ctl->lan_gaddr[1];
}

/*
 * lan_set_open_opt - set adapter open options
 *	options take effect on next adpater open.
 */
lan_set_open_opt(unit, flag)
	register int unit;
	register short flag;
{
	register struct lan_softc *lns = &lan_softc[unit];
	register struct lan_ctl *ctl = lns->lns_ctl;
	struct mbuf *m;

	/*
	 * use an mbuf to store scb, ssb
	 * to guarantee alignment
	 */
	if(lns->lns_ctl == 0) {
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == 0) {
			return;
		}
		lns->lns_ctl = mtod(m, struct lan_ctl *);
		ctl->lan_gaddr[0] = ctl->lan_gaddr[1] = 0;
		ctl->lan_faddr[0] = ctl->lan_faddr[1] = 0;
	}
	lns->lns_open_options = flag;
	DEBUGF(lan_debug,
	    printf("lan%d setting open options to %x\n",
		unit, lns->lns_open_options);
	);
}


/*
 * lan_get_open_opt - get adapter open options
 *	options returned are those used on last adpater open,
 *	not necessarily those set by lan_set_open_opt.
 */
lan_get_open_opt(unit, flag)
	register int unit;
	register short *flag;
{
	register struct lan_softc *lns = &lan_softc[unit];
	register struct lan_ctl *ctl = lns->lns_ctl;

	*flag = ctl->open_parm[0];
}


/*
 * lan_dma_setup - for a given virtual address, return a tcw entry
 *		corresponding to the real address mapping
 */
lan_dma_setup(vaddr, type, chan)
	register char *vaddr;
	register int type;
	register int chan;

{

	register int raddr;
	register unsigned int i;
	register short *tcwp;

	raddr = vtop((int)vaddr & ~(PAGESIZE - 1));

	switch (type) {
	case TCW_RESERVE:
		/*
		 * if tcw is type RESERVE it will not be freed
		 * so try to allocate from end of table down
		 */
		tcwp = (short *)(TCW_BASE + (((chan << 6) + (NUMTCW - 1)) << 1));
		for (i = NUMTCW - 1; i >= 0; i--, tcwp--) {
			if (*tcwp == TCW_FREE) 
				goto alloc_slot;
			if ((((int) *tcwp &
					(PAGESIZE-1)) << 11) == raddr)
				return (i);
		}
		return (TCW_ERROR);
	/*
	 * if tcw is for single use we expect it to be freed
	 * so allocate it from the start of the table up
	 */
	case TCW_SINGLE_USE:
		tcwp = (short *)(TCW_BASE + (chan << 7));
		for (i = 0; i < NUMTCW; i++, tcwp++) {
			if (*tcwp == TCW_FREE)
				goto alloc_slot;
		}
		return (TCW_ERROR);
	default:
		return (TCW_ERROR);
	}
alloc_slot:
	*(short *)(TCW_BASE + (((chan << 6) + i) << 1)) =
		(short)(((int)raddr) >> 11) | RSC_ACC | REAL_ACC;
	return (i);
}

/*
 * lan_dma_init - initialize the tcw table so all entries are free
 */

lan_dma_init(chan)
	register int chan;
{
	register short *tcwp;
	register int i;
	tcwp = (short *)(TCW_BASE + (chan << 7));
	for (i = NUMTCW; i > 0; i--, tcwp++)
		*tcwp = TCW_FREE;
}


/*
 * lan_xid_test - support logical link control type 1 operation
 */

lan_xid_test(m, i, unit)
	register struct mbuf *m;
	register int i;
	register int unit;
{
	register struct lan_softc *lns = &lan_softc[unit];
	register struct ifnet *ifp = &lns->lns_if;
	char *c;
	int j;

	switch (lns->rh[i]->llc_ctl) {
	case LAN_LLC_XID_CMD0:
	case LAN_LLC_XID_CMD1:
		c = mtod(m, char *);
		for (j = 0; j < LAN_L_XID_RESP; j++) {
			*c++ = lan_xid_resp[j];
		}
		m->m_len = LAN_L_XID_RESP;
	case LAN_LLC_TEST_CMD0:
	case LAN_LLC_TEST_CMD1:
		lan_output_llc(ifp, m, lns->rh[i]->from_addr,
			lns->rh[i]->llc_ctl, lns->rh[i]->dsap);
		return (lns->rh[i]->llc_ctl);
		break;
	default:
		return(0);
		break;
	}
}


/*
 * miow - swap bytes before port output
 */

miow(ioport, datawd)
	register unsigned short *ioport;
	register unsigned short datawd;
{
	*ioport = ((datawd >> 8) | (datawd << 8));
}


/*
 * mior - swap bytes after port input
 */
unsigned short
mior(ioport)				/* swap bytes after port input */
	register unsigned short *ioport;
{
	register unsigned short datawd;

	datawd = *ioport;
	return ((unsigned short)(datawd >> 8) | (datawd << 8));
}


#ifdef LANMAC				/* DDP - Begin */
#include "../h/socketvar.h"
#include "../h/domain.h"
#include "../net/raw_cb.h"


int	raw_usrreq(), lanmac_output(), lanmac_usrreq();

struct protosw lanmacsw[] = {
#ifdef PF_MAC
{ SOCK_RAW,	PF_MAC,		0,	PR_ATOMIC|PR_ADDR,
#else
{ SOCK_RAW,	PF_CABLE,	0,	PR_ATOMIC|PR_ADDR,
#endif
  0,		lanmac_output,	0,	0,
  lanmac_usrreq,
  0,		0,		0,	0,
}
};

struct domain lanmacdomain =
#ifdef AF_LANMAC
    { AF_LANMAC,  "mac",  lanmacsw,  &lanmacsw[sizeof(lanmacsw)/sizeof(lanmacsw[0])] };
#else
    { AF_CABLE,  "mac",  lanmacsw,  &lanmacsw[sizeof(lanmacsw)/sizeof(lanmacsw[0])] };
#endif

lanmac_usrreq(so, req, m, nam, rights)
struct socket *so;
int req;
struct mbuf *m, *nam, *rights;
{
    int value;

    DEBUGF(lan_debug,
	printf("lan: got lanmac_usrreq\n");
    );
/*    so->so_state |= SS_PRIV;	/* For debug by !suser() */
    value = raw_usrreq(so, req, m, nam, rights);
    if(value)			/* Error? */
	return(value);
    if (req == PRU_ATTACH) {
/* Make all sockets "greedy" */
	so->so_snd.sb_hiwat = so->so_snd.sb_mbmax;
	so->so_rcv.sb_hiwat = so->so_rcv.sb_mbmax;

/* Make sure it doesn't route */
	((struct rawcb *)(so->so_pcb))->rcb_flags |= RAW_DONTROUTE;
    }
    return(value);
}


lanmac_output(m, so)
struct mbuf *m;
struct socket *so;
{
    register struct sockaddr *dst;
    struct rawcb *rp = sotorawcb(so);
    register struct lan_softc *lns;
    register struct ifnet *ifp;

/*    DEBUGF(lan_debug,*/
	printf("lan: got lanmac_output\n");
/*    );*/
    dst = (struct sockaddr *)&rp->rcb_faddr;
    if(dst->sa_data[0] != 'l' || dst->sa_data[1] != 'a' || dst->sa_data[2] != 'n' ||
       dst->sa_data[3] < '0' || dst->sa_data[3] > '9') {
	printf("lanmac: invalid sockaddr.\n");
	return;
       }

    lns = &lan_softc[dst->sa_data[3] - '0'];
    ifp = &lns->lns_if;

    lan_output_mac(ifp, m);
}


lan_output_mac(ifp, m)
	register struct ifnet *ifp;
	register struct mbuf *m;
{
	int s, error;
	struct lan_device *addr = (struct lan_device *)
	lan_info[ifp->if_unit]->iod_addr;
	register struct lan_softc *lns = &lan_softc[ifp->if_unit];
	register struct list_hdr *lan;

	lan = mtod(m, struct list_hdr *);
	bcopy((caddr_t)lns->lns_addr, (caddr_t)lan->from_addr, LAN_L_ADDR);
	lan->pcf0 = LAN_PCF0;
	lan->pcf1 = LAN_PCF1_MAC;
	s = splimp();
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		error = ENOBUFS;
		goto qfull;
	}
	if (!(lns->lns_adapter & (LAN_ADAP_BROKEN | LAN_ADAP_DOWN))) {
		if (lns->lns_adapter & LAN_ADAP_OPEN) {
			int next_buf=lns->lns_nextbuf;
			if(lns->xp[next_buf]->cstat & LAN_XCSTAT_COMPLETE) {
				if (lan_put(lns, m, next_buf, ifp->if_unit) == 0) {
					DEBUGF(lan_debug,
						printf("lan%d xmit buf %x filled\n",
							ifp->if_unit, next_buf);
					);
					if (lns->lns_oactive == 0)
						lan_start(lns, addr, next_buf, ifp->if_unit);
				} else
					m_freem(m);
			} else
				IF_ENQUEUE(&ifp->if_snd, m);
		} else {
			if ((lns->lns_adapter & LAN_OPEN_IN_PROGRESS) == 0) {
				lan_open(ifp->if_unit, addr);
			}
			IF_ENQUEUE(&ifp->if_snd, m);
		}
	} else
		m_freem(m);

	splx(s);
	return (0);

qfull:
	splx(s);
	DEBUGF(lan_debug, printf("lan%d output queue full\n", ifp->if_unit);
	);
bad:
	m_freem(m);
	return (error);
}


/* Addresses used with lanmac raw sockets */
#ifdef AF_MAC
struct sockaddr lanmac_src = { AF_MAC, "lan0\0\0\0\0\0\0\0\0\0\0" };
struct sockaddr lanmac_dst = { AF_MAC, "lan0\0\0\0\0\0\0\0\0\0\0" };
struct sockproto lanmac_proto = { AF_MAC, PF_MAC };
#else
struct sockaddr lanmac_src = { AF_CABLE, "lan0\0\0\0\0\0\0\0\0\0\0" };
struct sockaddr lanmac_dst = { AF_CABLE, "lan0\0\0\0\0\0\0\0\0\0\0" };
struct sockproto lanmac_proto = { AF_CABLE, PF_CABLE };
#endif AF_MAC

#endif LANMAC				/* DDP - End */

#endif NLAN > 0

