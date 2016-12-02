#include "ucl.h"
#include "lex.h"

#include "config.h"

static NameBucket NameBuckets[NAME_HASH_MASK + 1];
/******************************************
	The hash function for calculating hash value.
 ******************************************/
static unsigned int ELFHash(char *str, int len)
{
	unsigned int h = 0;
	unsigned int x = 0;
	int i;

	for (i = 0; i < len; ++i)
	{
		h = (h << 4) + *str++;
		if ((x = h & 0xF0000000) != 0)
		{
			h ^= x >> 24;
			h &= ~x;
		}
	}

	return h;
}

/**
 * For identifiers, ucc maintains a name pool. If two identifiers' name is same,
 * InternName() returns same pointer to a unique copy in the name pool. After this,
 * the comparison of identifier name is very simple, just ==.
 */
char* InternName(char *id, int len)
{
	int i;
	int h;
	NameBucket p;
	// try to find the id in the hash buckets.
	h = ELFHash(id, len) & NAME_HASH_MASK;
	for (p = NameBuckets[h]; p != NULL; p = p->link)
	{
		if (len == p->len && strncmp(id, p->name, len) == 0)
			return p->name;
	}
	// allocate memory for struct nameBucket object
	p = HeapAllocate(&StringHeap, sizeof(*p));
	// allocate memory for string
	p->name = HeapAllocate(&StringHeap, len + 1);
	for (i = 0; i < len; ++i)
	{
		p->name[i] = id[i];
	}
	p->name[len] = 0;
	p->len = len;
	// append the new nameBucket object at the end of the corresponding list in Hash table.
	p->link = NameBuckets[h];
	NameBuckets[h] = p;

	return p->name;
}

/**
 * Different with identifier, ucc doesn't maintain a string pool for string literal. i.e.
 * Even two string literals' character sequence is identical, there are seperate copy for
 * them in memory.
 * 
 * ucc can handle string with arbitrary length unless there is no memory. AppendSTR() 
 * appends the characters or wide characters in tmp to the string str and adds a terminating 
 * '\0' or L'\0'
 */
void AppendSTR(String str, char *tmp, int len, int wide)
{

	int i, size;
	char *p;
	int times = 1;

	// PRINT_DEBUG_INFO(("len = %d",len));
	size = str->len + len + 1;
	// always save wide char as int, no matter	whether sizeof(WCHAR) is 2 or 4
	if (wide){
		times = sizeof(UCC_WC_T);
	}

	p = HeapAllocate(&StringHeap, size * times);
	for (i = 0; i < str->len * times; ++i)
	{
		p[i] = str->chs[i];
	}
	 
	for (i = 0; i < len * times; ++i)
	{
		p[i+str->len * times] = tmp[i];
	}
	str->chs = p;
	str->len = size - 1;
	// str->wide = wide;
	// add 0 at the end of the string
	if (! wide)
	{
		str->chs[size - 1] = 0;
	}
	else
	{
		UCC_WC_T *wcp = (UCC_WC_T *)p + (size - 1);
		*wcp = 0;
	}	

}
