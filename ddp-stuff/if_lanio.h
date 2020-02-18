/*
 * 5799-CGZ (C) COPYRIGHT IBM CORPORATION 1986
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */
/* $Header: if_lanio.h,v 1.1 86/05/20 11:11:37 donna Exp $ */
/* $Source: /usr/sys/DONNA/if_lanio.h,v $ */

#if !defined(lint) && !defined(LOCORE)  && defined(RCS_HDRS)
static char *rcsidif_lanio = "$Header: if_lanio.h,v 1.1 86/05/20 11:11:37 donna Exp $";
#endif

/*      Adapter Storage for Freeze-Dump Data				*/
#define  LAN_FREEZE_DUMP 4096	/* Size of freeze-dump data in bytes	*/
#define  LAN_FREEZE_CHUNK 2048	/* Size of read window when frozen	*/
#define  LAN_FREEZE_INCR	0xff00	/* Increment addr during freeze	*/
#define  LAN_ADAP_FREEZE_PULSES	2	/* Number pulses to freeze	*/
#define  LAN_ADAP_FREEZE_DELAY	120	/* Delay between pulses to detect*/
#define  LAN_ADAP_MIN_RESET	4	/* Delay between back-to-back dio*/
#define  LAN_FREEZE_SIG_u12	0x80	/* ucode level 12 compatibility	*/
struct lan_dump {
	int lan_len;		/* buffer length	*/
	unsigned short lan_dump_data[LAN_FREEZE_DUMP/2];
	};

/* lan freeze-dump controls */
#define	SIOCFLANDUMP	_IOW(i, 128, struct ifreq)	/* freeze lan */
#define	SIOCSLANDUMP	_IOW(i, 127, struct ifreq)	/* set lan dump */
#define	SIOCGLANDUMP	_IOW(i,126, struct ifreq)	/* get lan dump */

/*DDP - Begin */
/* additional lan controls */
#define	SIOCSIFOPENOPT	_IOW(i,125, struct ifreq)	/* set open options */
#define	SIOCGIFOPENOPT	_IOWR(i,124, struct ifreq)	/* get open options */
#define	SIOCSIFFADDR	_IOW(i,123, struct ifreq)	/* set functional address */
#define	SIOCGIFFADDR	_IOWR(i,122, struct ifreq)	/* get functional address */
#define	SIOCSIFGADDR	_IOW(i,121, struct ifreq)	/* set group address */
#define	SIOCGIFGADDR	_IOWR(i,120, struct ifreq)	/* get group address */
#define	SIOCRDERRORLOG	_IOWR(i,119, struct ifreq)	/* read adapter error log */
#define	SIOCRDADAPBUFF	_IOWR(i,118, struct ifreq)	/* read adapter buffer */
