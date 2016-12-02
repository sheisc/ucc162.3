#ifndef __TARGET_H_
#define __TARGET_H_

enum { CODE, DATA };


void PutASMCode(int code, Symbol opds[]);
void SetupRegisters(void);
void BeginProgram(void);
void Segment(int seg);
void Import(Symbol p);
void Export(Symbol p);
void DefineGlobal(Symbol p);
void DefineCommData(Symbol p);
void DefineString(String p, int size);
void DefineFloatConstant(Symbol p);
void DefineAddress(Symbol p);
void DefineLabel(Symbol p);
void DefineValue(Type ty, union value val);
void Space(int size);
void EmitFunction(FunctionSymbol p);
void EndProgram(void);

#endif
