// filesys.h 
//	Data structures to represent the Nachos file system.
//
//	A file system is a set of files stored on disk, organized
//	into directories.  Operations on the file system have to
//	do with "naming" -- creating, opening, and deleting files,
//	given a textual file name.  Operations on an individual
//	"open" file (read, write, close) are to be found in the OpenFile
//	class (openfile.h).
//
//	We define two separate implementations of the file system. 
//	The "STUB" version just re-defines the Nachos file system 
//	operations as operations on the native UNIX file system on the machine
//	running the Nachos simulation.
//
//	The other version is a "real" file system, built on top of 
//	a disk simulator.  The disk is simulated using the native UNIX 
//	file system (in a file named "DISK"). 
//
//	In the "real" implementation, there are two key data structures used 
//	in the file system.  There is a single "root" directory, listing
//	all of the files in the file system; unlike UNIX, the baseline
//	system does not provide a hierarchical directory structure.  
//	In addition, there is a bitmap for allocating
//	disk sectors.  Both the root directory and the bitmap are themselves
//	stored as files in the Nachos file system -- this causes an interesting
//	bootstrap problem when the simulated disk is initialized. 
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef FS_H
#define FS_H

#include "copyright.h"
#include "sysdep.h"
#include "openfile.h"
#include "debug.h" 		//just for test!!!
/*======================*/
#include <string.h>
/*======================*/
#ifdef FILESYS_STUB 		// Temporarily implement file system calls as 
				// calls to UNIX, until the real file system
				// implementation is available
typedef int OpenFileId;

class FileSystem {
  public:
    FileSystem() { 
	    for (int i = 0; i < 20; i++) fileDescriptorTable[i] = NULL; 
    }

    bool Create(char *name) {
        int fileDescriptor = OpenForWrite(name);

        if (fileDescriptor == -1) return FALSE;
        Close(fileDescriptor); 
        return TRUE; 
    }

/*=====================================================================*/

    // This function needs to be modified to handle file open errors
    OpenFile* Open(char *name) {
	    int fileDescriptor = OpenForReadWrite(name, FALSE);

	    if (fileDescriptor == -1) return NULL;
	    return new OpenFile(fileDescriptor, name);
    }

    OpenFileId HandleOpen(char* filename) {
        OpenFile* openfile = this->Open(filename);

        if (openfile == NULL) {
            DEBUG(dbgSys, "No such file");
            return -1;
        }

        for (int i = 0; i < 20; i++) {
            if (fileDescriptorTable[i] != NULL && strcmp(fileDescriptorTable[i]->name,filename) == 0) {
                delete openfile;
                DEBUG(dbgSys, "duplicated opened file" << filename);
                return -1;
            }
        }

        for (int i = 0; i < 20; i++) {
            if (fileDescriptorTable[i] == NULL) {
                fileDescriptorTable[i] = openfile;
                DEBUG(dbgSys, "open file success at " << i);
                return i;
            }
        }

        delete openfile;
        DEBUG(dbgSys, "fileDescriptorTable is full");
        return -1;
    }
 
    int WriteFile(char *buffer, int size, OpenFileId id){
        int entry = id;
        if (fileDescriptorTable[entry] == NULL)
            return -1; // the file doesn't exist in fileDescriptorTable
        
        return fileDescriptorTable[entry]->Write(buffer, size);
    }

    int ReadFile(char *buffer, int size, OpenFileId id){
        int entry = id;
        if (fileDescriptorTable[entry] == NULL)
            return -1; // the file doesn't exist in fileDescriptorTable
        
        return fileDescriptorTable[entry]->Read(buffer, size);
    }

    int CloseFile(OpenFileId id){
        int entry = id;
        if (fileDescriptorTable[entry] == NULL)
            return 0; // the file doesn't exist in fileDescriptorTable
        
        delete fileDescriptorTable[entry];
        fileDescriptorTable[entry] = NULL;
        return 1;
    }

/*=====================================================================*/

    bool Remove(char *name) { return Unlink(name) == 0; }

    OpenFile *fileDescriptorTable[20];
};

#else // FILESYS
class FileSystem {
  public:
    FileSystem(bool format);		// Initialize the file system.
					// Must be called *after* "synchDisk" 
					// has been initialized.
    					// If "format", there is nothing on
					// the disk, so initialize the directory
    					// and the bitmap of free blocks.

    bool Create(char *name, int initialSize);  	
					// Create a file (UNIX creat)

    OpenFile* Open(char *name); 	// Open a file (UNIX open)

    bool Remove(char *name);  		// Delete a file (UNIX unlink)

    void List();			// List all the files in the file system

    void Print();			// List all the files and their contents

  private:
   OpenFile* freeMapFile;		// Bit map of free disk blocks,
					// represented as a file
   OpenFile* directoryFile;		// "Root" directory -- list of 
					// file names, represented as a file
};

#endif // FILESYS

#endif // FS_H
