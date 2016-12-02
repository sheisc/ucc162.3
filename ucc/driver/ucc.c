#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ucc.h"

static char **Command;
struct option Option;
List PPFiles;
List ASMFiles;
List OBJFiles;

void* Alloc(int size)
{
	void *p = malloc(size);
	if (p == NULL)
	{
		fprintf(stderr, "memory exhausted\n");		
		exit(-1);
	}
	return p;
}
/***************************************
Replace the postfix of @name with @ext.
For example:
	@name	abc.c
	@ext	.obj
	@return	abc.obj
****************************************/
char* FileName(char *name, char *ext)
{
	char *str;
	int len;
	int i;
	
	if(ext == EMPTY_EXT){
		return name;
	}
	i = len = strlen(name);
	while (i > 0 && name[i - 1] != '.'){
		i--;
	}
	if(i > 0){
		len = i - 1;
	}
	str = Alloc(len + strlen(ext)+1);
	strncpy(str,name,len);
	str[len] = '\0';
	strcat(str,ext);	
	return str;		

}
/**********************************************
Append a struct list object containing @str at the 
end of @list.
***********************************************/
List ListAppend(List list, char *str)
{
	List *tail = &list;
	List p = list;

	while (p != NULL)
	{
		tail = &p->next;
		p = p->next;
	}
	*tail = p = Alloc(sizeof(*p));
	p->str = str;
	p->next = NULL;

	return list;
}
/***************************************************
	Append List @from at the end of @to.
****************************************************/
List ListCombine(List to, List from)
{
	List *tail = &to;
	List p = to;

	while (p != NULL)
	{
		tail = &p->next;
		p = p->next;
	}
	*tail = from;

	return to;
}
/*****************************************************
-Wa,options  Pass comma seperated 'options' to the assembler
-Wl,options  Pass comma seperated 'options' to the linker
******************************************************/
List ParseOption(char *opt)
{
	char *s, *t;
	char *str;
	List p = NULL;

	s = opt;
	t = strchr(s, ',');
	while (t != NULL)
	{
		str = Alloc(t - s + 1);
		strncpy(str, s, t - s);
		str[t-s] = 0;
		p = ListAppend(p, str);
		s = t + 1;
		t = strchr(s, ',');
	}

	return ListAppend(p, s);
}
/****************************************
for "ucc -v",	verbose
*****************************************/
static void PrintCommand(void)
{
	int i;

	if (Command[0] != NULL)
	{
		printf("%s", Command[0]);
		for (i = 1; Command[i] != NULL; ++i)
		{
			printf(" %s", Command[i]);
		}
		fprintf(stdout, "\n");
	}
}
/****************************************************************
	@cmd
		see	CPPProg/CCProg/ASProg/LDProg in linux.c
	@flags	
		$1	in CPPProg/....	will be replaced with @flags
	@infiles
		$2	in CPPProg/...		will be replacedd with @infiles
	@outfiles
		$3	in CPPProg/...		will be replaced with @outfiles
****************************************************************/
char** BuildCommand(char *cmd[], List flags, List infiles, List outfiles)
{
	int i, j;
	List lists[3];
	List p;
	//	$1
	lists[0] = flags;
	//	$2
	lists[1] = infiles;
	//	$3
	lists[2] = outfiles;
	for (i = j = 0; cmd[i] != NULL; i++)
	{
		char *s = strchr(cmd[i], '$');
		if (s && isdigit(s[1]))
		{
			int d = s[1] - '0';

			if (p = lists[d - 1])
			{
				/***************************************************
					see  "-Fo$3" in ASProg[] in Win32.c 
				****************************************************/
				Command[j] = Alloc(strlen(cmd[i]) + strlen(p->str) - 1);
				strncpy(Command[j], cmd[i], s - cmd[i]);
				Command[j][s - cmd[i]] = '\0';
				strcat(Command[j], p->str);
				strcat(Command[j++], s + 2);
				p = p->next;
				while (p != NULL)
				{
					Command[j++] = p->str;
					p = p->next;
				}
			}
		}
		else
		{
			Command[j++] = cmd[i];
		}
	}
	Command[j] = NULL;
	if (Option.verbose)
	{
		PrintCommand();
	}
	return Command;
}
/***********************************************
	abc.c		---> return	C_FILE
	abc.i			---> return 	PP_FILE
	abc.s		---> return	ASM_FILE
	abc.o		---> return	OBJ_FILE
************************************************/
static int GetFileType(char *filename)
{
	int i, j;
	int len;

	len = strlen(filename);
	for (i = C_FILE; i <= OBJ_FILE; ++i)
	{
		char *s = ExtNames[i], *t;

		t = strchr(s, ';');
		while (t != NULL)
		{
			j = t - s;
			if (strncmp(&filename[len -j], s, j) == 0)
				return i;
			s = t + 1;
			t = strchr(s, ';');
		}
		j = strlen(s);
		if (strncmp(&filename[len - j], s, j) == 0)
			return i;
	}

	return -1;
}

static void AddFile(char *filename)
{
	int type = GetFileType(filename);
	//	4	R_OK	,2	W_OK	,1	X_OK
	if (access(filename, 4) != 0)
	{
		fprintf(stderr, "No such file: %s\n", filename);
		return;
	}

	if (type == -1)
	{
		fprintf(stderr, "unrecognized file: %s\n", filename);
		return;
	}

	switch (type)
	{
	case C_FILE:
		Option.cfiles = ListAppend(Option.cfiles, filename);
		break;

	case PP_FILE:
		Option.pfiles = ListAppend(Option.pfiles, filename);
		break;

	case ASM_FILE:
		Option.afiles = ListAppend(Option.afiles, filename);
		break;

	case OBJ_FILE:
		Option.ofiles = ListAppend(Option.ofiles, filename);
		break;

	case LIB_FILE:
		Option.lfiles = ListAppend(Option.lfiles, filename);
		break;
	}
	/*************************************************
	(1)
		type == -1  has been treated before.
	(2)
		for  abc.c	/abc.i/abc.s, we add corresponding abc.obj
		into Option.linput
	**************************************************/
	if (type < OBJ_FILE)
	{
		char *ext = ExtNames[OBJ_FILE];
		char *s = strchr(ext, ';');

		if (s != NULL)
		{
			char buf[512];
			strncpy(buf, ext, s - ext);
			buf[s - ext] = '\0';
			ext = buf;
		}
		filename = FileName(filename, ext);
	}
	Option.linput = ListAppend(Option.linput, filename);
}
/*******************************************************
	remove temporary files:
	(1)	remove *.i 
	(2)	remove *.s
	(3)	remove *.o
********************************************************/
static void RemoveFiles(void)
{
	List p;

	if (Option.oftype > PP_FILE)
	{
		for (p = PPFiles; p != NULL; p = p->next)
		{
			remove(p->str);
		}
	}

	if (Option.oftype > ASM_FILE)
	{
		for (p = ASMFiles; p != NULL; p = p->next)
		{
			remove(p->str);
		}
	}

	if (Option.oftype > OBJ_FILE)
	{
		for (p = OBJFiles; p != NULL; p = p->next)
		{
			remove(p->str);
		}
	}
}

static void ShowHelp(void)
{
	char *msg[] = 
	{
		"Usage: ucc [options] (file | -xxx) ...\n",
		"Options:\n",
		"  --dump-ast   Dump syntax tree which is put into a file named xxx.ast\n",
		"  --dump-IR    Dump intermediate code which is put into a file named xxx.uil\n",
		"  -E           Preprocess only\n",
		"  -S           Compile only\n",
		"  -c           Compile and assemble only\n",
		"  -o file      Place the output into 'file'\n",
		"  -Idir        Add dir to the include file search directories\n",
		"  -Dname=def   Define preprocess macro 'name',its value is 'def'\n",
		"  -Uname       Undefine the preprocess macro 'name'\n",
		"  -h           Show this help information\n",
		"  -v           Verbose mode, show the commands invoked\n",
		"  -Wa,options  Pass comma seperated 'options' to the assembler\n"
		"  -Wl,options  Pass comma seperated 'options' to the linker\n",
		"All other options after first file will be passed to the linker\n",
		0
	};
	int i = 0;

	while (msg[i])
	{
		printf("%s",msg[i]);
		i++;
	}		
}

int InvokeProgram(int oftype)
{
	List p, il, ol;
	char *ofname;
	char **cmd;
	int status = 0;

	switch (oftype)
	{
	case PP_FILE:
		for (p = Option.cfiles; p != NULL; p = p->next)
		{
			if(Option.out && Option.oftype == PP_FILE){
				ofname = Option.out;
			}else{
				ofname = FileName(p->str, DEF_PP_EXT);
			}
			PPFiles = ListAppend(PPFiles, ofname);
			il = ListAppend(NULL, p->str);
			ol = ListAppend(NULL, ofname);
			cmd = BuildCommand(CPPProg, Option.pflags, il, ol);
			status = Execute(cmd);
		}
		Option.pfiles = ListCombine(Option.pfiles, PPFiles);
		break;

	case ASM_FILE:
		if (Option.pfiles == NULL)
			return 0;

		for (p = Option.aflags, Option.aflags = NULL; p != NULL; p = p->next)
		{
			// see "-Wa,"	strlen("-Wa,") is 4	,ParseCmdLine()
			Option.aflags = ListCombine(Option.aflags, ParseOption(p->str + 4));
		}
		for (p = Option.pfiles; p != NULL; p = p->next)
		{
			if(Option.out && Option.oftype == ASM_FILE){
				//PRINT_DEBUG_INFO(("Option.out = %s\n",Option.out));
				ofname = Option.out;				
			}else{
				ofname = FileName(p->str, DEF_ASM_EXT);				
			}
			ASMFiles = ListAppend(ASMFiles, ofname);
			il = ListAppend(NULL, p->str);
			ol = ListAppend(NULL, ofname);
			cmd = BuildCommand(CCProg, Option.cflags, il, ol);			
			status = Execute(cmd);
		}
		Option.afiles = ListCombine(Option.afiles, ASMFiles);

		break;

	case OBJ_FILE:
		for (p = Option.afiles; p != NULL; p = p->next)
		{
			if(Option.out && Option.oftype == OBJ_FILE){
				ofname = Option.out;
			}else{
				ofname = FileName(p->str, DEF_OBJ_EXT);
			}
			OBJFiles = ListAppend(OBJFiles, ofname);
			il = ListAppend(NULL, p->str);
			ol = ListAppend(NULL, ofname);
			cmd = BuildCommand(ASProg, Option.aflags, il, ol);
			status = Execute(cmd);
		}
		Option.ofiles = ListCombine(Option.ofiles, OBJFiles);
		break;

	case LIB_FILE:
		return 0;

	case EXE_FILE:
		if (Option.ofiles == NULL)
			return 0;
	
		if (Option.out == NULL){			
			Option.out = FileName(DEF_OPTION_OUT, DEF_EXE_EXT);
		}		
		for (p = Option.lflags, Option.lflags = NULL; p != NULL; p = p->next)
		{
			// see "-Wl,"	strlen("-Wl,") is 4	,ParseCmdLine()
			Option.lflags = ListCombine(Option.lflags, ParseOption(p->str + 4));
		}

		cmd = BuildCommand(LDProg, Option.lflags, Option.linput, ListAppend(NULL, Option.out));
		status = Execute(cmd);
		break;
	}

	return status;
}


/*********************************************************
This function only parses options which comes from command line for "ucc",
not the options fabricated in Win32.c like "CPPProg[]"
**********************************************************/
static int ParseCmdLine(int argc, char *argv[])
{
	int i;

	for (i = 0; i < argc; ++i)
	{
		//PRINT_DEBUG_INFO(("%s",argv[i]));
		if (strcmp(argv[i], "--dump-ast") == 0 ||
			strcmp(argv[i], "--dump-IR") == 0)
		{
			Option.cflags = ListAppend(Option.cflags, argv[i]);
		}
		else if (strcmp(argv[i], "-E") == 0)
		{
			Option.oftype = PP_FILE;
		}
		else if (strcmp(argv[i], "-S") == 0)
		{
			Option.oftype = ASM_FILE;
		}
		else if (strcmp(argv[i], "-c") == 0)
		{
			Option.oftype = OBJ_FILE;
		}
		else if (strcmp(argv[i], "-o") == 0)
		{
			Option.out = argv[++i];
		}
		else if (strncmp(argv[i], "-D", 2) == 0 ||
			strncmp(argv[i], "-U", 2) == 0 ||
			strncmp(argv[i], "-I", 2) == 0)
		{
			Option.pflags = ListAppend(Option.pflags, argv[i]);
		}
		else if (strcmp(argv[i], "-h") == 0)
		{
			ShowHelp();
		}
		else if (strcmp(argv[i], "-v") == 0)
		{
			Option.verbose = 1;
		}
		else if (strncmp(argv[i], "-Wl,", 4) == 0)
		{
			Option.lflags = ListAppend(Option.lflags, argv[i]);
		}
		else if (strncmp(argv[i], "-Wa,", 4) == 0)
		{
			Option.aflags = ListAppend(Option.aflags, argv[i]);
		}
		else if (argv[i][0] == '-')
		{			
			Option.linput = ListAppend(Option.linput, argv[i]);
		}
		else{
			AddFile(argv[i]);	
		}
	}

	return i;
}

static int GetListLen(List list){
	int count = 0;
	while(list){
		count++;
		list = list->next;
	}
	return count;
}

int main(int argc, char *argv[])
{
	int i;

	if (argc <= 1)
	{
		ShowHelp();
		exit(0);
	}

	Option.oftype = EXE_FILE;
	SetupToolChain();
	Command = Alloc((argc + 60) * sizeof(char *));
	Command[0] = NULL;

	ParseCmdLine(--argc, ++argv);
	if(Option.out && Option.oftype != EXE_FILE &&
		(GetListLen(Option.cfiles) > 1 || GetListLen(Option.pfiles) > 1 || GetListLen(Option.afiles) > 1)){
		fprintf(stderr, "cannot specify -o with -c, -S or -E with multiple files\n");
		return -1;
	}

	/******************************************************
	(1)	.c	 ----> 	.i
	(2)	.i	---->	.asm
	(3)	.asm---->	.obj
	(4)	.obj	---->	.exe
		.obj	---->	.lib
	*******************************************************/
	for (i = PP_FILE; i <= Option.oftype; ++i)
	{
		if (InvokeProgram(i) != 0)
		{
			RemoveFiles();
			fprintf(stderr, "ucc invoke command error:");
			PrintCommand();
			return -1;
		}
	}

	RemoveFiles();
	return 0;
}
