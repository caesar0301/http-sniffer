/* util.h */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <sys/types.h>

#define MALLOC(type, num)  (type *) check_malloc((num) * sizeof(type))

void *check_malloc(unsigned long size);
char *ip_ntos(u_int32_t n);
u_int32_t ip_ston(char *s);

#endif	/* __UTIL_H__ */
