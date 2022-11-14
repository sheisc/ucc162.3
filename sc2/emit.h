#ifndef		EMIT_H
#define		EMIT_H
//.fmtstr:	.string	"%d"
#define		INPUT_FORMAT_STR_NAME		".input_fmtstr"
#define		OUTPUT_FORMAT_STR_NAME		".output_fmtstr"
void EmitAssembly(const char * fmt, ...);
void EmitLabel(const char * fmt, ...);

void EmitPrologue(int frameSize);
void EmitEpilogue(void);

#endif























