/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls 
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__ 
#define __USERPROG_KSYSCALL_H__ 

#include "kernel.h"

#include "synchconsole.h"


void SysHalt()
{
  kernel->interrupt->Halt();
}

int SysAdd(int op1, int op2)
{
  return op1 + op2;
}

/*===================MP4=======================*/
int SysCreate(char *name, int size)
{

	return kernel->fileSystem->Create(name, size);
}

OpenFileId SysOpen(char *filename)
{
  // return fileDescriptorTable entry (1~20)
  // -1 : failed
	OpenFile* file_pointer = kernel->fileSystem->Open(filename);
	if (file_pointer == NULL) return -1;
	else return 1;
}

int SysClose(int id)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->fileSystem->CloseFile(id);
}

int SysWrite(char *buffer, int size, int id)
{
	// return value
	// -1: failed
  // else: numBytes actually written
	return kernel->fileSystem->WriteFile(buffer, size, id);
}

int SysRead(char *buffer, int size, int id)
{
	// return value
	// -1: failed
  // else: numBytes actually read
	return kernel->fileSystem->ReadFile(buffer, size, id);
}

/*===================MP4=======================*/
#ifdef FILESYS_STUB
int SysCreate(char *filename)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->interrupt->CreateFile(filename);
}
#endif


#endif /* ! __USERPROG_KSYSCALL_H__ */
