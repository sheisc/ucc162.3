#ifndef __STDARG_H_
#define __STDARG_H_

#define ALIGN_INT(n)	((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))

#if !defined(_VA_LIST) && !defined(__VA_LIST_DEFINED)

#define _VA_LIST
#define _VA_LIST_DEFINED
typedef char *__va_list;

#endif

typedef __va_list va_list;

#define va_start(list, start) (list = (va_list)&start + ALIGN_INT(start))
#define va_arg(list, t) (*(t *)((list += ALIGN_INT(t)) - ALIGN_INT(t))) 
#define va_end(list) (list = (va_list)0)

typedef void *__gnuc_va_list;

#endif
