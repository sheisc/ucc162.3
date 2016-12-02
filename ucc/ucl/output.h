#ifndef __OUTPUT_H_
#define __OUTPUT_H_

void LeftAlign(FILE *file, int pos);
FILE* CreateOutput(char *filename, char *ext);
char* FormatName(const char *fmt, ...);
void Print(const char *fmt, ...);
void PutString(char *str);
void PutChar(int ch);
void Flush(void);

#endif

