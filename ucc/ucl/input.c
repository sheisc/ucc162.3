#if defined(_UCC)

#include "stdio.h"

#elif defined(_WIN32)

#include "windows.h"

#else

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#endif

#include "input.h"
#include "error.h"

#include "stdio.h"
#include "stdlib.h"


unsigned char END_OF_FILE = 255;
struct input Input;

/**
 * Reads the whole preprocessed C source file into memory.
 * When compiling by Windows VC, uses the memory mapping file
 * mechanism in Windows OS; When compiling by ucc, allocates memory.
 * Whatever mechanism used, this function appends an extra byte, 
 * its value is END_OF_FILE. When lexical analyzer reads this byte,
 * the file is taken for finished.
 */
void ReadSourceFile(char *filename)
{
#if defined(_UCC)
	/************************************************************
		Use standard C I/O library to access file.	(*.i files)
	 ************************************************************/
	int len;

	Input.file = fopen(filename, "r");
	if (Input.file == NULL)
	{
		Fatal("Can't open file: %s.", filename);
	}
	/***********************************************************
		The ftell() function obtains the current value  of  the  file  position
       indicator for the stream pointed to by stream.
       	fseek + ftell ---->  file size

	 ***********************************************************/
	fseek(Input.file, 0, SEEK_END);
	Input.size = ftell(Input.file);
	// allocate enough heap memory.
	Input.base = malloc(Input.size + 1);
	if (Input.base == NULL)
	{
		Fatal("The file %s is too big", filename);
		fclose(Input.file);
	}
	// set current position to the beginning of the file.
	fseek(Input.file, 0, SEEK_SET);
	Input.size = fread(Input.base, 1, Input.size, Input.file);
	fclose(Input.file);

#elif defined(_WIN32)
	/************************************************************
		Use Win32 API to access file.	(*.i files)
			FileMapping
	 ************************************************************/

	Input.file = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		                     OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (Input.file == INVALID_HANDLE_VALUE)
	{
		Fatal("Can't open file: %s.", filename);
	}

	Input.size = GetFileSize(Input.file, NULL);
	Input.fileMapping = CreateFileMapping(Input.file, NULL, PAGE_READWRITE, 0, Input.size + 1, NULL);
	if (Input.fileMapping == NULL)
	{
		Fatal("Can't create file mapping: %s.", filename);
	}

	Input.base = (unsigned char *)MapViewOfFile(Input.fileMapping, FILE_MAP_WRITE, 0, 0, 0);
	if (Input.base == NULL)
	{
		Fatal("Can't map file: %s.", filename);
	}

#else
	/************************************************************
		Use Linux System Call to access file.	(*.i files)
			FileMapping
	 ************************************************************/

	struct stat st;
	int fno;

	fno = open(filename, O_RDWR);
	if (fno == -1)
	{
		Fatal("Can't open file %s.\n", filename);
	}
	//fstat() gets file status, returns information about a file.
	if (fstat(fno, &st) == -1)
	{
		Fatal("Can't stat file %s.\n", filename);
	}
	Input.size = st.st_size;
	
	Input.base = mmap(NULL, Input.size + 1, PROT_WRITE, MAP_PRIVATE, fno, 0);
	if (Input.base == MAP_FAILED)
	{
		Fatal("Can't mmap file %s.\n", filename);
	}

	Input.file = (void *)fno;

#endif

	Input.filename = filename;
	// fabricate an EOF
	Input.base[Input.size] = END_OF_FILE;
	Input.cursor = Input.lineHead = Input.base;
	Input.line = 1;

	return;
}

void CloseSourceFile(void)
{
#if defined(_UCC)

	free(Input.base);

#elif defined(_WIN32)

	UnmapViewOfFile(Input.base);
	CloseHandle(Input.fileMapping);
	SetFilePointer(Input.file, Input.size, NULL, FILE_BEGIN);
	SetEndOfFile(Input.file);
	CloseHandle(Input.file);

#else
	close((int)Input.file);
	munmap(Input.base, Input.size + 1);

#endif
}

