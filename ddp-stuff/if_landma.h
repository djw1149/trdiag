/*	Settings to en/disable the dma channels used by the adapter	*/
#define  CH5_PAGE	0x04
#define  CH6_PAGE	0x02
#define  CH_ENABLE	0x00
#define  CH_DISABLE	0x04
#define  CH5  		0x01
#define  CH6		0x02

/*	TCW Variables							*/
#define  NUMTCW	64
#define  TCW_FREE	0xffffffff	/* in raddr means tcw slot free */
#define  TCW_SINGLE_USE	1		/* tcw freed after single dma op*/
#define  TCW_RESERVE		2	/* tcw will remain in use*/	
#define  TCW_ERROR		-1	/* error in tcw allocation*/
#define  ALTMAST_PAGEMODE_HIBITS	0xfe
#define  ALTMAST_PAGEMODE_DISP		0x7ff
#define  ALTMAST_PAGEMODE_TCW_LOBITS	11
#define  ALTMAST_PAGEMODE_TCW_HIBIT	5

/*	Address conversion macros					*/
#define  PAGESIZE	2048
#define  LOG2PAGESIZE	11
#define  DMA_HI_ADDR(x) (ALTMAST_PAGEMODE_HIBITS | (x >> ((sizeof(u_short) << 3)-LOG2PAGESIZE)))
#define  DMA_LO_ADDR(y,x) (((int)y & (PAGESIZE - 1)) | (x << LOG2PAGESIZE))
