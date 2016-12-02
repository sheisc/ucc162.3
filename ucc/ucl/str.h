#ifndef __STR_H_
#define __STR_H_

typedef struct nameBucket
{
	char *name;
	int len;
	struct nameBucket *link;
} *NameBucket;

typedef struct string
{
	char *chs;
	int len;
} *String;

#define NAME_HASH_MASK 1023

char* InternName(char *id, int len);
void AppendSTR(String str, char *tmp, int len, int wide);

#endif


