/***************************1.userprog/exception.cc***************************/
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	is in machine.h.
//----------------------------------------------------------------------
void
ExceptionHandler(ExceptionType which)
{
    char ch;
    int val;
    int type = kernel->machine->ReadRegister(2);
    int status, exit, threadID, programID, fileID, numChar;
    DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");
    DEBUG(dbgTraCode, "In ExceptionHandler(), Received Exception " << which << " type: " << type << ", " << kernel->stats->totalTicks);
    switch (which) {
		case SyscallException:
			switch(type) {
				/*
				.
				.
				*/
				case SC_Create:
					val = kernel->machine->ReadRegister(4);
					{
					char *filename = &(kernel->machine->mainMemory[val]);
					//cout << filename << endl;
					status = SysCreate(filename);
					kernel->machine->WriteRegister(2, (int) status);
					}
					kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));
					kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);
					kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
					return;
					ASSERTNOTREACHED();
					break;

				/*======================================================================================*/
				
				/*
				.
				.
				*/
				default:
					cerr << "Unexpected system call " << type << "\n";
					break;
			}
		break;
		default:
			cerr << "Unexpected user mode exception " << (int)which << "\n";
			break;
    }
    ASSERTNOTREACHED();
}
/***************************2.userprog/ksyscall.h***************************/
int SysCreate(char *filename)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->fileSystem->Create(filename);
}
/***************************3.filesys/filesys.h***************************/
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
