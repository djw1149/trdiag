/* 
 * $Header: hash.c,v 1.2 86/07/24 12:39:33 chriss Exp $
 *
 * file : hash.c
 *
 * HISTORY
 * 14-Mar-86  breen at Carnegie-Mellon University
 *	Fixed fatal bug in addnode. 
 *
 *      ARP Hash table manipulation primitives
 */

#include <sys/types.h>

#include "./h/rtypes.h"
#include "../h/queue.h"
#include "../h/rcp.h"
#include "./h/hash.h"
#include "../h/arp.h"
#include "../h/arpquery.h"

#define HASHSIZE NADDMAP	/* Hash size is the size of the ARP cache */

extern char * malloc();

/*****************************************************************
 * Hash an entry into the proper bucket
 * 
 *	returns the offset into the bucket array
 */	

hash(pa, pl)
register char *pa;		/* The key to be hashed */
register int pl;		/* Its lenght */
{
register unsigned int   hash = 0;

while (pl--) {
	hash <<= 1;
	hash += (u_char) * pa++;
	}

return (hash % HASHSIZE);
}

/*****************************************************************
 * look up an entry in the hash table
 *
 *	returns a pointer to the saved data, or Null if not found
 */

char *getnode(head,key,keylen)
BUCK_TYPE *head[];		/* The hash table */
register char *key;		/* The key to look up */
register int keylen;		/* Its length */
{
register BUCK_TYPE *temp = head[hash(key,keylen)];
while(temp) {
/* 	if (keylen == temp->keylen) */

       	if (!bcmp(key,temp->key,keylen)) return(temp->data);
        temp = temp->next;
	}
return(0);
}

/*****************************************************************
 *	Remove an entry from the hash table
 *
 */

clipnode(head,key,keylen)
BUCK_TYPE *head[];		/* The hash table */
register char *key;		/* the key for the offending entry */
register int keylen;		/* its length */
{
register BUCK_TYPE *pretemp,*temp;
pretemp = (temp = head[hash(key,keylen)]);

while(temp) {
	if (keylen == temp->keylen)
        	if (!bcmp(key,temp->key,keylen)) {
                	if (temp != pretemp) pretemp->next = temp->next;
 			free((char *)temp);
			}
	pretemp = temp;
	temp = temp->next;
	}
}

/*****************************************************************
 * Add an entry to the hash table
 *
 */

addnode(head,key,keylen,data)
BUCK_TYPE *head[];
char *key;
int keylen;
char *data;
{
BUCK_TYPE *temp;
int hashkey = hash(key,keylen);

temp = (BUCK_TYPE *) malloc(sizeof(BUCK_TYPE));
temp->key = malloc(keylen);
bcopy(key,temp->key,keylen);

temp->data = data;
temp->next = head[hashkey];
head[hashkey] = temp;
}

