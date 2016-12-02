#ifndef __ASSERT_H_
#define __ASSERT_H_

void assert(int);

#endif

#undef assert

#ifdef NDEBUG

#define assert(ignore) ((void)0)

#else
/***********************************************************
	assert(sym->kind == SK_TypedefName);

	(void)((e)||_assert(#e, __FILE__, __LINE__))

	(void)((sym->kind == SK_TypedefName)||_assert("sym->kind == SK_TypedefName", __FILE__, __LINE__))
 ***********************************************************/
extern int _assert(char *, char *, unsigned);
#define assert(e) ((void)((e)||_assert(#e, __FILE__, __LINE__)))

#endif
