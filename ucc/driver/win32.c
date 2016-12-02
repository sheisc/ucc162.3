#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include "ucc.h"


/*******************************************************************************
Platform:	64bit	Windows8.1,	Visual Studio 2013
(1)
	"-D_W64= ",		
		see Line50 in C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\include\vadefs.h 
(2)
	see function SkipWhiteSpace(void) in lex.c in UCL project
	
(3)	Environment Variables
	INCLUDE
	C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\include;
	C:\Program Files (x86)\Windows Kits\8.1\Include\um;
	C:\Program Files (x86)\Windows Kits\8.1\Include\shared
	LIB
	C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\lib;
	C:\Program Files (x86)\Windows Kits\8.1\Lib\winv6.3\um\x86
	PATH
	D:\bin\ucc;
	C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin
*******************************************************************************/
char *CPPProg[] = 
{ 
	"cl", "-nologo", "-u", "-D_WIN32", "-D_M_IX86", "-D_UCC", "-D_W64= ","-D__STDC__",
	"$1", "-P", "$2","-Fi$3", 0 
};

/*************************************************************************
	On windows8/x64 platform,
	We use 
		cl -E -D_CRT_NON_CONFORMING_SWPRINTFS -D_CRT_NONSTDC_NO_WARNINGS
		-D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE
	instead.			
	We use these Macros to suppress warnings as "__declspec(deprecated(This function..."
**************************************************************************/
char *CCProg[] = 
{
	"ucl", /*1*/"", /*2*/"", "-ext:.asm","-o", "$3","$1", "$2", 0 
};
char *ASProg[] = 
{ 
	"ml", "-nologo", "-c", "-Cp", "-coff", "-Fo$3", "$1", "$2", 0 
};
char *LDProg[] = 
{
	"link", "-nologo", "-subsystem:console", "-entry:mainCRTStartup", "$1", /*6*/"-OUT:$3", 
	"$2",   "oldnames.lib", /*8*/"libc.lib", "kernel32.lib", 0
};




char *ExtNames[] = { ".c;.C,", ".i;.I", ".asm;.ASM;.s;.S", ".obj;.OBJ", ".lib", 0 };

void SetupToolChain(void)
{
	char *dir;

	if ((dir = getenv("VS90COMNTOOLS")) != NULL)
	{
		LDProg[8] = "libcmt.lib";
		goto ok;
	}

	if ((dir = getenv("VS80COMNTOOLS")) != NULL)
	{
		LDProg[8] = "libcmt.lib";
		goto ok;
	}

	if ((dir = getenv("VS71COMNTOOLS")) != NULL)
		goto ok;
#if 1	
	LDProg[8] = "libcmt.lib";
#endif
#if 0		// commented
	return;
#endif
ok:	
	CCProg[1] = "-ignore __declspec(deprecated),__declspec(noreturn),__inline,__fastcall,__cdecl,__stdcall";
	CCProg[2] = "-keyword __int64";
}


int Execute(char **cmd){
	return _spawnvp(_P_WAIT, cmd[0], cmd);
}


