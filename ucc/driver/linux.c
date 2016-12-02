#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "ucc.h"

#define _P_WAIT 0
#define UCCDIR "/home/iron/bin/"


/**********************************************************
ucc -E -v hello.c -I../ -DREAL=double -o hello.ii
	For $1/$2/$3	,see BuildCommand() for detail.
	$1		-I../ -DREAL=double,	command-line options
	$2		hello.c,		input file
	$3		hello.ii,		output file
***********************************************************/
char *CPPProg[] = 
{ 
	"/usr/bin/gcc", "-U__GNUC__", "-D_POSIX_SOURCE", "-D__STRICT_ANSI__",
	"-Dunix", "-Di386", "-Dlinux", "-D__unix__", "-D__i386__", "-D__linux__", 
	"-D__signed__=signed", "-D_UCC", "-I" UCCDIR "include", "$1", "-E", "$2", "-o", "$3", 0 
};


/***********************************************************
ucc -S -v hello.c --dump-ast -o hello.asm
	------->
/home/iron/bin/ucl -o hello.asm --dump-ast hello.i
	$1	--dump-ast,	some command-line options
	$2	hello.i,	input file
	$3	hello.asm,	output file
************************************************************/
char *CCProg[] = 
{
	UCCDIR "ucl", "-o", "$3", "$1", "$2", 0 
};

char *ASProg[] = 
{ 
	"/usr/bin/as", "-o", "$3", "$1", "$2", 0 
};

char *LDProg[] = 
{
	"/usr/bin/gcc", "-o", "$3", "$1", "$2", UCCDIR "assert.o", "-lc", "-lm", 0 
};


char *ExtNames[] = { ".c", ".i", ".s", ".o", ".a;.so", 0 };

int Execute(char **cmd)
{
	int pid, n, status;

	pid = fork();
	if (pid == -1)
	{
		fprintf(stderr, "no more processes\n");
		return 100;
	}
	else if (pid == 0)
	{
		execv(cmd[0], cmd);
		perror(cmd[0]);
		fflush(stdout);
		exit(100);
	}
	/**************************************************	
	wait():  on success, returns the process ID of the terminated child; on
	error, -1 is returned.
	***************************************************/
	while ((n = wait(&status)) != pid && n != -1)
		;
	if (n == -1)
		status = -1;
	if (status & 0xff)
	{
		fprintf(stderr, "fatal error in %s\n", cmd[0]);
		status |= 0x100;
	}

	return (status >> 8) & 0xff;
}

void SetupToolChain(void)
{
}


