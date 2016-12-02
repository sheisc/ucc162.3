#include "ucl.h"
#include "ast.h"
#include "target.h"

// flag to control if dump abstract syntax tree
static int DumpAST;
// flag to control if dump intermediate code
static int DumpIR;
// file to hold abstract synatx tree
FILE *ASTFile;
// file to hold intermediate code
FILE *IRFile;
// file to hold assembly code
FILE *ASMFile;
// assembly file's extension name
char *ExtName = ".s";

char * ASMFileName = NULL;

// please see alloc.h, defaultly, ALLOC() allocates memory from CurrentHeap
/*******************************************
	CurrentHeap switch between ProgramHeap / FileHeap.
 *******************************************/
Heap CurrentHeap;
// hold memory whose lifetime is the whole program
HEAP(ProgramHeap);
// hold memory whose lifetime is the whole file
HEAP(FileHeap);
// all the strings and identifiers are hold in StringHeap, the lifetime is the whole program 
HEAP(StringHeap);
// number of warnings in a file
int WarningCount;
// number of errors in a file 
int ErrorCount;
Vector ExtraWhiteSpace;
Vector ExtraKeywords;

static void Initialize(void)
{
	CurrentHeap = &FileHeap;
	ErrorCount = WarningCount = 0;
	InitSymbolTable();
	ASTFile = IRFile = ASMFile = NULL;
}

static void Finalize(void)
{
	FreeHeap(&FileHeap);
}

static void Compile(char *file)
{
	AstTranslationUnit transUnit;

	Initialize();

	// parse preprocessed C file, generate an abstract syntax tree
	transUnit = ParseTranslationUnit(file);

	if (ErrorCount != 0)
		goto exit;	

	// perform semantic check on abstract synatx tree
	CheckTranslationUnit(transUnit);

	if (ErrorCount != 0)
		goto exit;

	if (DumpAST)
	{
		// Dump syntax tree which is put into a file named xxx.ast
		DumpTranslationUnit(transUnit);
	}

	// translate the abstract synatx tree into intermediate code
	Translate(transUnit);

	if (DumpIR)
	{
		// Dump intermediate code which is put into a file named xxx.uil
		DAssemTranslationUnit(transUnit);
	}

	// emit assembly code from intermediate code.
	// The kernel function is EmitIRInst(inst).
	// for example, see function EmitAssign(IRInst inst) 
	EmitTranslationUnit(transUnit);

exit:
	Finalize();
}
// consider some strings as white space:
// CCProg[1] = "-ignore __declspec(deprecated),__declspec(noreturn),__inline,__fastcall";
static void AddWhiteSpace(char *str)
{
	char *p, *q;


	if(!ExtraWhiteSpace){
		ExtraWhiteSpace = CreateVector(1);
	}

	p = str;
	while ((q = strchr(p, ',')) != NULL)
	{
		*q = 0;
		INSERT_ITEM(ExtraWhiteSpace, p);
		p = q + 1;
	}
	INSERT_ITEM(ExtraWhiteSpace, p);	
}
// see win32.c in UCC project
// 	CCProg[2] = "-keyword __int64";
static void AddKeyword(char *str)
{
	char *p, *q;

	if(!ExtraKeywords){
		ExtraKeywords = CreateVector(1);
	}

	p = str;
	while ((q = strchr(p, ',')) != NULL)
	{
		*q = 0;
		INSERT_ITEM(ExtraKeywords, p);		
		p = q + 1;
	}
	INSERT_ITEM(ExtraKeywords, p);	
}
//Most of the cmd line parameters have been parsed by compiler driver UCC already.
// only the following options are parsed by compiler ucl.
static int ParseCommandLine(int argc, char *argv[])
{
	int i;

	for (i = 0; i < argc; ++i)
	{
		//PRINT_DEBUG_INFO(("%s",argv[i]));
		// see linux.c in UCC project,  "ucl", "-ext:.s", "$1", "$2", 0 
		if (strncmp(argv[i], "-ext:", 5) == 0)
		{
			ExtName = argv[i] + 5;
		}
		else if (strcmp(argv[i], "-o") == 0){
			i++;
			ASMFileName = argv[i];
		}

		// see win32.c in UCC project
		// CCProg[1] = "-ignore __declspec(deprecated),__declspec(noreturn),__inline,__fastcall";
		else if (strcmp(argv[i], "-ignore") == 0)
		{
			i++;			
			AddWhiteSpace(argv[i]);
		}
		// see win32.c in UCC project
		// 	CCProg[2] = "-keyword __int64";
		else if (strcmp(argv[i], "-keyword") == 0)
		{
			i++;
			AddKeyword(argv[i]);
		}
		// see ucc.c in UCC project
		// "  --dump-ast   Dump syntax tree which is put into a file named xxx.ast\n",
		else if (strcmp(argv[i], "--dump-ast") == 0)
		{
			DumpAST = 1;
		}
		// see ucc.c in UCC project
		// "  --dump-IR    Dump intermediate code which is put into a file named xxx.uil\n",
		else if (strcmp(argv[i], "--dump-IR") == 0)
		{
			DumpIR = 1;
		} 
		else
			return i;
	}
	return i;
}


/**
 * The compiler's main entry point. 
 * The compiler handles C files one by one.
 */
int main(int argc, char *argv[])
{
	int i;

	CurrentHeap = &ProgramHeap;
	argc--; argv++;
	i = ParseCommandLine(argc, argv);

	SetupRegisters();
	SetupLexer();
	SetupTypeSystem();
	/*****************************************************
	(1)	All the heap space are allocated from ProgramHeap before
		the following for-statement.
		command-line info	/ basic type info
	(2)
		When we call compile(), we first call Initialize() to set 
		CurrentHeap to FileHeap.
		At the end of compile(), finalize() is called to clear the FileHeap.
		That is ,we allocate heap memory from FileHeap when we 
		are compiling a C file; Clear it after the compilation is finished.
		And then , compile the next C file.
	 *****************************************************/
	for (; i < argc; ++i)
	{
		Compile(argv[i]);
	}

	return (ErrorCount != 0);
}


