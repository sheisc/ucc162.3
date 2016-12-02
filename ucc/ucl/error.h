#ifndef __ERROR_H_
#define __ERROR_H_


void Do_Error(Coord coord, const char *format, ...);
void Do_Fatal(const char *format, ...);
void Do_Warning(Coord coord, const char *format, ...);

#define	Error	fprintf(stderr, "(%s,%d):", __FILE__, __LINE__) , Do_Error
#define	Fatal	fprintf(stderr, "(%s,%d):", __FILE__, __LINE__) , Do_Fatal
#define	Warning		fprintf(stderr, "(%s,%d):", __FILE__, __LINE__) , Do_Warning



#endif

