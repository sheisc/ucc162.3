#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int ErrorCount;

void Error(const char * format,...){
	va_list ap;

	ErrorCount++;
	fprintf(stderr, "error:");
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(-1);
}
