/*
 * $Header: dispmem.c,v 1.2 86/07/24 12:38:42 chriss Exp $
 *
 * file : dispmem.c
 *
 */

dispmem(buff,offset,length)
char *buff;
int offset,length;
{
    int cnt,temp;
    for (cnt = 0;cnt<length;cnt ++) {
        if (((cnt) % 8) == 0 )
	  printf("  ");
        if (((cnt) % 16) == 0)
           printf("\noffset [%4x] : ",cnt + offset);
        temp = (unsigned char) buff[cnt];
        printf("%2x ",temp);
    }
    printf("\n");
}

