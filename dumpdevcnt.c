/*****************************************************************
 *
 * $Header: dumpdevcnt.c,v 1.4 87/07/22 16:52:41 router Locked $
 *
 * file : dumpdevcnt.c
 *
 *	Show everything in a (sort of) readable format
 *		shstat does all of the interesting work
 *
 *	This should be directly built from devstats.h with a sed script
 *		(At make time)
 */
#include "./h/rcpinfo.h"
#include "./h/rtypes.h"
#include "../h/proto.h"
#include "../h/devstats.h"

/*****************************************************************
 * shstat - Macro to print stuff out
 *		BUG -very wasteful since fullname always expands to
 *		a string constant
 */
#define shstat(stat,fullname) 					\
printf("%-20s:","stat");					\
for(cnt=0; cnt <= numdev; cnt++)				\
printf("%12d |",rdev[cnt].stat);				\
printf("\n");							\
if(*fullname && verbose) printf("	%s\n",fullname);


rcp_devdump(router, card, rdev, verbose, first)
char *router;
int card, verbose, first;
struct devinfo *rdev;
{
    int     cnt,
            cnt2;

    printf ("router stats  :");
    printf ("       Total |");
    for (cnt = 1; cnt <= numdev; cnt++)
	printf ("     card %2d |", cnt);
    printf ("\n");

 shstat(dvc_br,"")
 shstat(dvc_nunit,"")
 shstat(dvc_istatus,"")
 shstat(dvc_dstate,"")
 shstat(dvc_cable,"")
 shstat(dvc_devtype,"")
 shstat(dvc_spare,"")

/* char dvc_phys[ACMAXHLN] */
/* char dvc_praddr[PRL_ALL] */

 shstat(ds.dr_unsol,"unsolicited interupts")
 shstat(ds.dr_rnerr,"network receive error (checksum etc)")
 shstat(ds.dr_rierr,"internal driver problem (dma overun etc)")
 shstat(ds.dr_rcnt,"sucessfully received packets")
 shstat(ds.dr_rdrop,"discarded: no queue space **RESOURCE**")
 shstat(ds.dr_rbcast,"received broadcasts")
 shstat(ds.dr_rlen,"discarded: bad length")
 shstat(ds.dr_rproto,"discarded: unknown protocol type")
 shstat(ds.dr_xcnt,"queued transmitted packets")
 shstat(ds.dr_xnerr,"transmit errors attributed to the net")
 shstat(ds.dr_xierr,"transmit errors attr. to the router (us)")
 shstat(ds.dr_jcnt,"retrys due to jams")
 shstat(ds.dr_rq,"receive queue")
 shstat(ds.dr_rqmax,"receive queue max")
 shstat(ds.dr_xq,"transmit queue")
 shstat(ds.dr_xqmax,"transmit queue max")

 shstat(ds.dr_spare1,"")
 shstat(ds.dr_spare2,"")


 shstat(ds.ct_rcnt,"currently discarded")
 shstat(ds.ct_xcnt,"future")
 shstat(ds.ct_spare1,"")
 shstat(ds.ct_spare2,"")


 shstat(ds.ar_rmin,"illegal length")
 shstat(ds.ar_rhrd,"illegal hardware space")
 shstat(ds.ar_rpro,"illegal (usupported) target protocol")
 shstat(ds.ar_rhln,"illegal hardware address length")
 shstat(ds.ar_rfraud,"illegal source HW address (!= actual)")
 shstat(ds.ar_rmine,"discard my own broadcasts")
 shstat(ds.ar_rcnt,"processed ARP")
 shstat(ds.ar_runchanged,"source is unchanged")
 shstat(ds.ar_runknown,"source is new")
 shstat(ds.ar_rchanged,"source moved")
 shstat(ds.ar_rlockerr,"source moved too quickly (a loop ?)")
 shstat(ds.ar_rng_xpp,"pp request sent from ring")
 shstat(ds.ar_rng_unm,"entry unmapped because of pp failure")
 shstat(ds.ar_rng_alru,"entry added to lru queue from ring")
 shstat(ds.ar_lru_used,"addmap entry in lru queue used")
 shstat(ds.ar_xcnt,"transmitted ARP packets")
 shstat(ds.ar_xcast,"transmitted ARP broadcast packets")
 shstat(ds.ar_xdrop,"transmitted ARP packets dropped")
 shstat(ds.ar_xppreq,"transmitted point to point requests")
 shstat(ds.ar_req_me,"recieved ARP request destination me")
 shstat(ds.ar_req_bc_dnc,"recieved bcast req. dest addr not known")
 shstat(ds.ar_req_bc_smv,"recieved bcast req. source moved")
 shstat(ds.ar_req_bc_rbc,"recieved bcast req. dest REQBCAST")
 shstat(ds.ar_req_bc_sndn,"recieved bcast req. src net != dest net")
 shstat(ds.ar_req_bc_rem,"recieved bcast req. misc.")
 shstat(ds.ar_req_pp_dnc,"recieved pp req. dest not known")
 shstat(ds.ar_req_pp_sndn,"recieved pp req. src net != dest net")
 shstat(ds.ar_req_pp_rem,"recieved pp req. misc.")
 shstat(ds.ar_rep_me,"recieved ARP request destination me")
 shstat(ds.ar_rep_bc,"recieved bcast reply")
 shstat(ds.ar_rep_pp_dnc,"recieved pp reply dest addr not known")
 shstat(ds.ar_rep_pp_for,"recieved pp reply forwarded")
 shstat(ds.ar_spare1,"")
 shstat(ds.ar_spare2,"")


 shstat(ds.cn_rcnt,"all discarded")
 shstat(ds.cn_rrut,"")
 shstat(ds.cn_xcnt,"")
 shstat(ds.cn_xdrop,"")
 shstat(ds.cn_tome,"")
 shstat(ds.cn_spare1,"")


 shstat(ds.xp_rcnt,"all discarded")
 shstat(ds.xp_spare1,"")
 shstat(ds.xp_spare2,"")


 shstat(ds.ip_bcast,"received IP broadcast packets")
 shstat(ds.ip_rvers,"received packets with wrong version number")
 shstat(ds.ip_rhovfl,"received packets with an IP header larger than the physical length of the packet")
 shstat(ds.ip_rlen,"received packets with an IP length larger than the physical length of the packet")
 shstat(ds.ip_rcksm,"received packets with an incorrect IP checksum")
 shstat(ds.ip_rttl,"received packets whos time-to-live has expired")
 shstat(ds.ip_rcnt,"received valid IP packets")
 shstat(ds.ip_popt,"packets with options")
 shstat(ds.ip_nopt,"total number of options")
for (cnt2 = 1; cnt2 <= XIPOP_MAX+1; cnt2++){
 shstat(ds.ip_ocnt[cnt2]," parsed option counts (index by option number)")
}
 shstat(ds.ip_oerr,"received packets with bad option lists")
 shstat(ds.ip_osr,"received source routes through us")
 shstat(ds.ip_ofrroute,"full record route options ignored")
 shstat(ds.ip_oftstamp,"full time stamp options ignored")
 shstat(ds.ip_onet,"Added routing entries to other nets")
 shstat(ds.ip_unreach,"unreachable destination")
 shstat(ds.ip_tome,"Addressed to me, may be source routed")
 shstat(ds.ip_rproto,"packets addressed to me of unsupported protocol")
 shstat(ds.ip_togway,"routed to a gateway")
 shstat(ds.ip_xcast,"unknown address converted packet to ar bcast")
 shstat(ds.ip_troll,"traffic that did not pass the troll")
 shstat(ds.ip_redir,"converted to redirects")
 shstat(ds.ip_frag,"messages sucessfully fragmented on output")
 shstat(ds.ip_dfrag,"received messages which wouldve been fragmented if dont fragment had not been specfied")
 shstat(ds.ip_cfrag,"received messages which could not be fragmented due to resource limitations (out of packets)")
 shstat(ds.ip_fdrop,"trailing fragments dropped on output because of problems delivering them to the local network")
 shstat(ds.ip_xcnt,"transmitted valid IP packets")
 shstat(ds.ip_spare1,"")
 shstat(ds.ip_spare2,"")

 shstat(ds.ic_rmin,"")
 shstat(ds.ic_rcksm,"")
for (cnt2 = 1; cnt2 <= XIC_MAX+1; cnt2++){
 shstat(ds.ic_rcnt[XIC_MAX + 1],"")
}
for (cnt2 = 1; cnt2 <= XIC_MAX+1; cnt2++){
 shstat(ds.ic_xcnt[XIC_MAX + 1],"")
}
 shstat(ds.ic_spare,"")

 shstat(ds.ud_rmin,"")
 shstat(ds.ud_rcksm,"")
 shstat(ds.ud_rcnt,"")
 shstat(ds.ud_rproto,"")
 shstat(ds.ud_xcnt,"")
 shstat(ds.ud_spare,"")

 shstat(ds.rc_rmin,"")
 shstat(ds.rc_rcnt,"")
 shstat(ds.rc_rtype,"")
 shstat(ds.rc_xcnt,"")
 shstat(ds.rc_xerr,"")
 shstat(ds.rc_get,"")
 shstat(ds.rc_put,"")
 shstat(ds.rc_auth,"")
 shstat(ds.rc_unauth,"")
 shstat(ds.rc_reboot,"")
 shstat(ds.rc_invalid,"")
 shstat(ds.rc_spare1,"")

 shstat(ds.bp_rmin,"illegal length")
 shstat(ds.bp_rbadop,"illegal opcode")
 shstat(ds.bp_rsecs,"not enough wait")
 shstat(ds.bp_rhops,"too many hops")
 shstat(ds.bp_rloop,"loop detected")
 shstat(ds.bp_reqcnt,"requests received")
 shstat(ds.bp_rgway,"gateway known")
 shstat(ds.bp_rbaddst,"bad ip dest")
 shstat(ds.bp_runkaddr,"unknown dest address")
 shstat(ds.bp_repcnt,"replies received")
 shstat(ds.bp_xpp_rep,"p-p replies sent")
 shstat(ds.bp_xbc_rep,"bcast replies sent")
 shstat(ds.bp_mupdated,"updated bootp entries")
 shstat(ds.bp_mtmo,"timed out bootp entries")
 shstat(ds.bp_mnew,"new bootp entries")
}
