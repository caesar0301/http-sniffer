/* util.c */

#include <stdio.h>
#include <string.h>

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
	static char buf[sizeof("aaa.bbb.ccc.ddd")];
	memset(buf, '\0', 15);

	sprintf(buf, "%d.%d.%d.%d",
			(n & 0xff000000) >> 24,
			(n & 0x00ff0000) >> 16,
			(n & 0x0000ff00) >> 8,
			(n & 0x000000ff) >> 0);

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
	return val;

}
