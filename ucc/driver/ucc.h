#ifndef __UCC_H_
#define __UCC_H_


enum { C_FILE, PP_FILE, ASM_FILE, OBJ_FILE, LIB_FILE, EXE_FILE };

typedef struct list
{
	char *str;
	struct list *next;
} *List;

struct option
{
	List pflags;	// flags for preprocessor
	List cflags;	// flags for compiler
	List aflags;	// flags for assembler
	List lflags;	// flags for linker
	List cfiles;	//.c
	List pfiles;	//.i
	List afiles;	//.s
	List ofiles;	//.obj
	List lfiles;	//.a	.lib
	List linput;	//input for linker
	int verbose;
	int oftype;	//output file type
	char *out;
};

void* Alloc(int len);
char* FileName(char *name, char *ext);
List  ListAppend(List list, char *str);
List  ListCombine(List to, List from);
List  ParseOption(char *opt);
char** BuildCommand(char *cmd[], List flags, List infiles, List outfiles);

void SetupToolChain(void);
int  InvokeProgram(int oftype);

extern char* ExtNames[];
extern List PPFiles;	//*.i
extern List ASMFiles;	//*.s
extern List OBJFiles;	//*.obj
extern struct option Option;


extern char *CPPProg[];
extern char *CCProg[];
extern char *ASProg[];
extern char *LDProg[];
#define	EMPTY_EXT		0
#ifdef	_WIN32
#define	DEF_PP_EXT		".i"
#define	DEF_ASM_EXT	".asm"
#define	DEF_OBJ_EXT	".obj"
#define	DEF_EXE_EXT	".exe"
#define	DEF_OPTION_OUT		Option.ofiles->str	
#else
#define	DEF_PP_EXT		".i"
#define	DEF_ASM_EXT	".s"
#define	DEF_OBJ_EXT	".o"
#define	DEF_EXE_EXT	EMPTY_EXT
#define	DEF_OPTION_OUT		"a.out"
#endif


int Execute(char **cmd);
#define	PRINT_DEBUG_INFO(LP_args_RP)	do{printf("%s %d:  ", __FILE__,__LINE__);	\
									printf LP_args_RP ;	\
									printf("\n");	\
									}while(0)



#endif
