
typedef struct buck_stc {
        char *key;
        char keylen;
        char *data;
        struct buck_stc *next;
} BUCK_TYPE;

typedef struct ncon_stc {
        char *pa;
        int  pln;
        char *ha;
        int  hln;
        struct ncon_stc *next;
} NCON_TYPE;

typedef struct host_stc {
        char   isrouter;
        char   isrcp;
        struct ncon_stc *host_ncon;
        struct net_stc *host_net;
	struct buck_stc **host_arp;
} HOST_TYPE;

#define HASHLENGTH 29
#define H_EN_HLEN 29
#define R_EN_HLEN 29
#define H_IP_HLEN 29
#define R_IP_HLEN 29
#define ARP_HLEN 29

typedef struct net_stc  {
        BUCK_TYPE *host_ip_list[H_EN_HLEN],
                  *host_en_list[H_EN_HLEN],
                  *router_en_list[R_EN_HLEN];
        struct ncon_stc *net_host;
} NET_TYPE;

char *getarpq(),*addarpq(), *getnode();
