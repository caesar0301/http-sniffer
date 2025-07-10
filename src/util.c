/* util.c */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <arpa/inet.h>

#include "util.h"

/* Simple wrapper around the malloc() function */
void *check_malloc(unsigned long size)
{

	void *ptr = NULL;
	if ((ptr = malloc(size)) == NULL) {
		printf("Out of memory!\n");
		exit(1);
	}
	return ptr;
}

char *ip_ntos(u_int32_t n){
	char *buf = malloc(sizeof("aaa.bbb.ccc.ddd"));
	if (buf == NULL) {
		printf("Out of memory!\n");
		exit(1);
	}
	memset(buf, '\0', 16);

	// Convert from network byte order to host byte order for processing
	u_int32_t host_n = ntohl(n);
	sprintf(buf, "%d.%d.%d.%d",
			(int)((host_n >> 24) & 0xff),
			(int)((host_n >> 16) & 0xff),
			(int)((host_n >> 8) & 0xff),
			(int)(host_n & 0xff));

	return buf;
}

/* stolen from inet_aton.c BSD */
u_int32_t ip_ston(char *cp){
	u_int32_t val;
	int base, n;
	char c;
	u_int parts[4];
	u_int *pp = parts;

	c = *cp;
	for (;;) {
			/*
			 * Collect number up to ``.''.
			 * Values are specified as for C:
			 * 0x=hex, 0=octal, isdigit=decimal.
			 */
			if (!isdigit(c))
					return (0);
			val = 0; base = 10;
			if (c == '0') {
					c = *++cp;
					if (c == 'x' || c == 'X')
							base = 16, c = *++cp;
					else
							base = 8;
			}
			for (;;) {
					if (isascii(c) && isdigit(c)) {
							val = (val * base) + (c - '0');
							c = *++cp;
					} else if (base == 16 && isascii(c) && isxdigit(c)) {
							val = (val << 4) |
									(c + 10 - (islower(c) ? 'a' : 'A'));
							c = *++cp;
					} else
							break;
			}
			if (c == '.') {
					/*
					 * Internet format:
					 *      a.b.c.d
					 *      a.b.c   (with c treated as 16 bits)
					 *      a.b     (with b treated as 24 bits)
					 */
					if (pp >= parts + 3)
							return (0);
					*pp++ = val;
					c = *++cp;
			} else
					break;
	}
	/*
	 * Check for trailing characters.
	 */
	if (c != '\0' && (!isascii(c) || !isspace(c)))
			return (0);
	/*
	 * Concoct the address according to
	 * the number of parts specified.
	 */
	n = pp - parts + 1;
	switch (n) {

	case 0:
			return (0);             /* initial nondigit */

	case 1:                         /* a -- 32 bits */
			break;

	case 2:                         /* a.b -- 8.24 bits */
			if ((val > 0xffffff) || (parts[0] > 0xff))
					return (0);
			val |= parts[0] << 24;
			break;

	case 3:                         /* a.b.c -- 8.8.16 bits */
			if ((val > 0xffff) || (parts[0] > 0xff) || (parts[1] > 0xff))
					return (0);
			val |= (parts[0] << 24) | (parts[1] << 16);
			break;

	case 4:                         /* a.b.c.d -- 8.8.8.8 bits */
			if ((val > 0xff) || (parts[0] > 0xff) || (parts[1] > 0xff) || (parts[2] > 0xff))
					return (0);
			val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
			break;
	}
	// Convert to network byte order before returning
	return htonl(val);

}
