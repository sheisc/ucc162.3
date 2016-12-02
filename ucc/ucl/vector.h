#ifndef __VECTOR_H_
#define __VECTOR_H_
/****************************************************
vector:
	data[0] --->  item data
	data[1] --->  item data
	......
	data[size-1] ---> item data

	size:		capacity of the vector
	len:			the number of items in vector	[0, size-1]
	
 ****************************************************/
typedef struct vector
{
	void **data;
	int len;
	int size;
} *Vector;
//  the length of the vector
#define LEN(v)   (v->len)

#define INSERT_ITEM(v, item)   \
do {                           \
    if (v->len >= v->size)     \
        ExpandVector(v);       \
    v->data[v->len] = item;    \
    v->len++;                  \
} while (0)

#define GET_ITEM(v, i) (v->data[i])

#define TOP_ITEM(v)    (v->len == 0 ? NULL : v->data[v->len - 1])
/***************************************
	Macros FOR_EACH_ITEM and ENDFOR are 
	used for iterating vectors
 ***************************************/
#define FOR_EACH_ITEM(ty, item, v)   \
{                                    \
    int len = v->len;                \
    int i = 0;                       \
    for (i = 0; i < len; i++)        \
    {                                \
	    item = (ty)v->data[i];

#define ENDFOR \
    }          \
}

Vector CreateVector(int size);
void ExpandVector(Vector v);

#endif

