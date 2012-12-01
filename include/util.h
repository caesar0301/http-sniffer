/* util.h */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <sys/types.h>

/************************Macros*****************************/
#define MALLOC(type, num)  (type *) check_malloc((num) * sizeof(type))

#ifndef BOOL
#define BOOL int
#endif /* BOOL */

#ifndef TRUE
#define TRUE 1
#endif	/* TRUE */

#ifndef FALSE
#define FALSE 0
#endif	/* FALSE */


/**********************Functions*****************************/

void *check_malloc(unsigned long size);
char *ip_ntos(u_int32_t n);
u_int32_t ip_ston(char *s);

#endif	/* __UTIL_H__ */
