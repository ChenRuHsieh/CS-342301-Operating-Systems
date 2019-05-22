/***************************1.userprog/exception.cc***************************/
ExceptionHandler(ExceptionType which)
{
    int type = kernel->machine->ReadRegister(2);
    switch (which) {
        case SyscallException:
        switch(type) {
            case SC_PrintInt:
			DEBUG(dbgSys, "Print Int\n");
			val=kernel->machine->ReadRegister(4);
			DEBUG(dbgTraCode, "In ExceptionHandler(), into SysPrintInt, " << kernel->stats->totalTicks);    
			SysPrintInt(val); /*******************************/	
			DEBUG(dbgTraCode, "In ExceptionHandler(), return from SysPrintInt, " << kernel->stats->totalTicks);
			// Set Program Counter
			kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));
			kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);
			kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
			return;
			ASSERTNOTREACHED();
			break;
        }
    }
}
/***************************2.userprog/ksyscall.h***************************/
void SysPrintInt(int val)
{ 
  DEBUG(dbgTraCode, "In ksyscall.h:SysPrintInt, into synchConsoleOut->PutInt, " << kernel->stats->totalTicks);
  kernel->synchConsoleOut->PutInt(val);/*******************************/	
  DEBUG(dbgTraCode, "In ksyscall.h:SysPrintInt, return from synchConsoleOut->PutInt, " << kernel->stats->totalTicks);
}
/***************************************************************************/
/***************************3.userprog/synchconsole.cc ***************************/
//----------------------------------------------------------------------
// SynchConsoleOutput::SynchConsoleOutput
//      Initialize synchronized access to the console display
//
//      "outputFile" -- if NULL, use stdout as console device
//              otherwise, read from this file
//----------------------------------------------------------------------

SynchConsoleOutput::SynchConsoleOutput(char *outputFile)
{
    consoleOutput = new ConsoleOutput(outputFile, this);
    lock = new Lock("console out");
    waitFor = new Semaphore("console out", 0);
}

void SynchConsoleOutput::PutChar(char ch)
{
    lock->Acquire();
    consoleOutput->PutChar(ch);
    waitFor->P();
    lock->Release();
}
void SynchConsoleOutput::PutInt(int value)
{
    char str[15];
    int idx=0;
    //sprintf(str, "%d\n\0", value);  the true one
    sprintf(str, "%d\n\0", value); //simply for trace code
    lock->Acquire();
    do{
	DEBUG(dbgTraCode, "In SynchConsoleOutput::PutChar, into consoleOutput->PutChar, " << kernel->stats->totalTicks);
        consoleOutput->PutChar(str[idx]);
	DEBUG(dbgTraCode, "In SynchConsoleOutput::PutChar, return from consoleOutput->PutChar, " << kernel->stats->totalTicks);
	idx++;
		
	DEBUG(dbgTraCode, "In SynchConsoleOutput::PutChar, into waitFor->P(), " << kernel->stats->totalTicks);
        waitFor->P();
	DEBUG(dbgTraCode, "In SynchConsoleOutput::PutChar, return form  waitFor->P(), " << kernel->stats->totalTicks);
    } while (str[idx] != '\0');
    lock->Release();
}
// The following class defines a "lock".  A lock can be BUSY or FREE.
// There are only two operations allowed on a lock: 
//
//	Acquire -- wait until the lock is FREE, then set it to BUSY
//
//	Release -- set lock to be FREE, waking up a thread waiting
//		in Acquire if necessary
//
// In addition, by convention, only the thread that acquired the lock
// may release it.  As with semaphores, you can't read the lock value
// (because the value might change immediately after you read it).  

class Lock {
  public:
    //----------------------------------------------------------------------
    // Lock::Lock
    // 	Initialize a lock, so that it can be used for synchronization.
    //	Initially, unlocked.
    //
    //	"debugName" is an arbitrary name, useful for debugging.
    //----------------------------------------------------------------------
    Lock::Lock(char* debugName)
    {
        name = debugName;
        semaphore = new Semaphore("lock", 1);  // initially, unlocked
        lockHolder = NULL;
    }  	// initialize lock to be FREE
    ~Lock();			// deallocate lock
    char* getName() { return name; }	// debugging assist

    void Acquire(); 		// these are the only operations on a lock
    void Release(); 		// they are both *atomic*

    bool IsHeldByCurrentThread() { 
    		return lockHolder == kernel->currentThread; }
    				// return true if the current thread 
				// holds this lock.
    
    // Note: SelfTest routine provided by SynchList
    
  private:
    char *name;			// debugging assist
    Thread *lockHolder;		// thread currently holding lock
    Semaphore *semaphore;	// we use a semaphore to implement lock
};

// The following class defines a "semaphore" whose value is a non-negative
// integer.  The semaphore has only two operations P() and V():
//
//	P() -- waits until value > 0, then decrement
//
//	V() -- increment, waking up a thread waiting in P() if necessary
// 
// Note that the interface does *not* allow a thread to read the value of 
// the semaphore directly -- even if you did read the value, the
// only thing you would know is what the value used to be.  You don't
// know what the value is now, because by the time you get the value
// into a register, a context switch might have occurred,
// and some other thread might have called P or V, so the true value might
// now be different.

class Semaphore {
  public:
    //----------------------------------------------------------------------
    // Semaphore::Semaphore
    // 	De-allocate semaphore, when no longer needed.  Assume no one
    //	is still waiting on the semaphore!
    //----------------------------------------------------------------------
    Semaphore::Semaphore(char* debugName, int initialValue)
    {
        name = debugName;
        value = initialValue;
        queue = new List<Thread *>;
    }
	// set initial value
    ~Semaphore();   					// de-allocate semaphore
    char* getName() { return name;}			// debugging assist
    
    void P();	 	// these are the only operations on a semaphore
    void V();	 	// they are both *atomic*
    void SelfTest();	// test routine for semaphore implementation
    
  private:
    char* name;        // useful for debugging
    int value;         // semaphore value, always >= 0
    List<Thread *> *queue;     
		  	// threads waiting in P() for the value to be > 0
   };
void
Semaphore::P()
{
	DEBUG(dbgTraCode, "In Semaphore::P(), " << kernel->stats->totalTicks);
    Interrupt *interrupt = kernel->interrupt;
    Thread *currentThread = kernel->currentThread;
    
    // disable interrupts
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	
    
    while (value == 0) { 		// semaphore not available
        queue->Append(currentThread);	// so go to sleep
        currentThread->Sleep(FALSE); // if true, this Thread will be deleted 
    } 
    value--; 			// semaphore available, consume its value
   
    // re-enable interrupts
    (void) interrupt->SetLevel(oldLevel);	
}
void
Thread::Sleep (bool finishing)
{
    Thread *nextThread;
    
    ASSERT(this == kernel->currentThread);
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    
    DEBUG(dbgThread, "Sleeping thread: " << name);
    DEBUG(dbgTraCode, "In Thread::Sleep, Sleeping thread: " << name << ", " << kernel->stats->totalTicks);

    status = BLOCKED;
	//cout << "debug Thread::Sleep " << name << "wait for Idle\n";
    while ((nextThread = kernel->scheduler->FindNextToRun()) == NULL) {
		kernel->interrupt->Idle();	// no one to run, wait for an interrupt
	}    
    // returns when it's time for us to run
    kernel->scheduler->Run(nextThread, finishing); 
}
/***************************4.machine/console.cc***************************/
//----------------------------------------------------------------------
// ConsoleOutput::ConsoleOutput
// 	Initialize the simulation of the output for a hardware console device.
//
//	"writeFile" -- UNIX file simulating the display (NULL -> use stdout)
// 	"toCall" is the interrupt handler to call when a write to 
//	the display completes.
//----------------------------------------------------------------------

ConsoleOutput::ConsoleOutput(char *writeFile, CallBackObj *toCall)
{
    if (writeFile == NULL)
	    writeFileNo = 1;				// display = stdout
    else
    	writeFileNo = OpenForWrite(writeFile);

    callWhenDone = toCall;
    putBusy = FALSE;
}

void ConsoleOutput::PutChar(char ch)
{
    ASSERT(putBusy == FALSE);// 如果不符合條件，就Halt
    WriteFile(writeFileNo, &ch, sizeof(char));
    putBusy = TRUE;
    kernel->interrupt->Schedule(this, ConsoleTime, ConsoleWriteInt);
}
/***************************5.machine/interrupt.cc***************************/
//----------------------------------------------------------------------
// PendingInterrupt::PendingInterrupt
// 	Initialize a hardware device interrupt that is to be scheduled 
//	to occur in the near future.
//
//	"callOnInt" is the object to call when the interrupt occurs
//	"time" is when (in simulated time) the interrupt is to occur
//	"kind" is the hardware device that generated the interrupt
//----------------------------------------------------------------------
PendingInterrupt::PendingInterrupt(CallBackObj *callOnInt, int time, IntType kind)
{
    callOnInterrupt = callOnInt;
    when = time;
    type = kind;
}
//----------------------------------------------------------------------
// Interrupt::Schedule
// 	Arrange for the CPU to be interrupted when simulated time
//	reaches "now + when".
//
//	Implementation: just put it on a sorted list.
//
//	NOTE: the Nachos kernel should not call this routine directly.
//	Instead, it is only called by the hardware device simulators.
//
//	"toCall" is the object to call when the interrupt occurs
//	"fromNow" is how far in the future (in simulated time) the 
//		 interrupt is to occur
//	"type" is the hardware device that generated the interrupt
//----------------------------------------------------------------------
void
Interrupt::Schedule(CallBackObj *toCall, int fromNow, IntType type)
{
    int when = kernel->stats->totalTicks + fromNow;
    PendingInterrupt *toOccur = new PendingInterrupt(toCall, when, type);

    DEBUG(dbgInt, "Scheduling interrupt handler the " << intTypeNames[type] << " at time = " << when);
    ASSERT(fromNow > 0);

    pending->Insert(toOccur);
}
/***************************6.machine/mipssim.cc***************************/
//----------------------------------------------------------------------
// Machine::Run
// 	Simulate the execution of a user-level program on Nachos.
//	Called by the kernel when the program starts up; never returns.
//
//	This routine is re-entrant, in that it can be called multiple
//	times concurrently -- one for each thread executing user code.
//----------------------------------------------------------------------
void Machine::Run()
{
    Instruction *instr = new Instruction;  // storage for decoded instruction

    if (debug->IsEnabled('m')) {
        cout << "Starting program in thread: " << kernel->currentThread->getName();
	    cout << ", at time: " << kernel->stats->totalTicks << "\n";
    }
    kernel->interrupt->setStatus(UserMode);
    for (;;) {
	DEBUG(dbgTraCode, "In Machine::Run(), into OneInstruction " << "== Tick " << kernel->stats->totalTicks << " ==");
        OneInstruction(instr);
	DEBUG(dbgTraCode, "In Machine::Run(), return from OneInstruction  " << "== Tick " << kernel->stats->totalTicks << " ==");
		
	DEBUG(dbgTraCode, "In Machine::Run(), into OneTick " << "== Tick " << kernel->stats->totalTicks << " ==");
	kernel->interrupt->OneTick();
	DEBUG(dbgTraCode, "In Machine::Run(), return from OneTick " << "== Tick " << kernel->stats->totalTicks << " ==");
	if (singleStep && (runUntilTime <= kernel->stats->totalTicks))
		Debugger();
    }
}
/***************************7.machine/interrupt.cc***************************/
//----------------------------------------------------------------------
// Interrupt::OneTick
// 	Advance simulated time and check if there are any pending 
//	interrupts to be called. 
//
//	Two things can cause OneTick to be called:
//		interrupts are re-enabled
//		a user instruction is executed
//----------------------------------------------------------------------
void Interrupt::OneTick()
{
    MachineStatus oldStatus = status;
    Statistics *stats = kernel->stats;

// advance simulated time
    if (status == SystemMode) {
        stats->totalTicks += SystemTick;
	stats->systemTicks += SystemTick;
    } else {
	stats->totalTicks += UserTick;
	stats->userTicks += UserTick;
    }
    DEBUG(dbgInt, "== Tick " << stats->totalTicks << " ==");

// check any pending interrupts are now ready to fire
    ChangeLevel(IntOn, IntOff);	// first, turn off interrupts
				                // (interrupt handlers run with interrupts disabled)
    CheckIfDue(FALSE);		// check for pending interrupts
    ChangeLevel(IntOff, IntOn);	// re-enable interrupts
    if (yieldOnReturn) {	// if the timer device handler asked 
    				        // for a context switch, ok to do it now
        yieldOnReturn = FALSE;
        status = SystemMode;		// yield is a kernel routine
        kernel->currentThread->Yield(); // this thread go to ready queue
        status = oldStatus;
    }
}
/***************************8.machine/interrupt.cc***************************/
//----------------------------------------------------------------------
// Interrupt::CheckIfDue
// 	Check if any interrupts are scheduled to occur, and if so, 
//	fire them off.
//
// Returns:
//	TRUE, if we fired off any interrupt handlers
// Params:
//	"advanceClock" -- if TRUE, there is nothing in the ready queue,
//		so we should simply advance the clock to when the next 
//		pending interrupt would occur (if any).
//----------------------------------------------------------------------
bool Interrupt::CheckIfDue(bool advanceClock)
{
    PendingInterrupt *next; 
    Statistics *stats = kernel->stats;

    ASSERT(level == IntOff);		// interrupts need to be disabled,
					// to invoke an interrupt handler
    if (debug->IsEnabled(dbgInt)) {
	DumpState();
    }
    if (pending->IsEmpty()) {   	// no pending interrupts
	return FALSE;	
    }		
    next = pending->Front(); // = toOccur

    if (next->when > stats->totalTicks) {
        if (!advanceClock) {		// not time yet
            return FALSE;
        }
        else {      		// advance the clock to next interrupt
	    stats->idleTicks += (next->when - stats->totalTicks);
	    stats->totalTicks = next->when;
	    // UDelay(1000L); // rcgood - to stop nachos from spinning.
	}
    }

    DEBUG(dbgInt, "Invoking interrupt handler for the ");
    DEBUG(dbgInt, intTypeNames[next->type] << " at time " << next->when);

    if (kernel->machine != NULL) {
    	kernel->machine->DelayedLoad(0, 0);
    }

    inHandler = TRUE;
    do {
        next = pending->RemoveFront();    // pull interrupt off list = toOccur
		DEBUG(dbgTraCode, "In Interrupt::CheckIfDue, into callOnInterrupt->CallBack, " << stats->totalTicks);
        next->callOnInterrupt->CallBack();// call the interrupt handler
		DEBUG(dbgTraCode, "In Interrupt::CheckIfDue, return from callOnInterrupt->CallBack, " << stats->totalTicks);
	delete next;
    } while (!pending->IsEmpty() && (pending->Front()->when <= stats->totalTicks));
    inHandler = FALSE;
    return TRUE;
}
/***************************9.machine/interrupt.cc***************************/
//----------------------------------------------------------------------
// ConsoleOutput::CallBack()
// 	Simulator calls this when the next character can be output to the
//	display.
//----------------------------------------------------------------------

void ConsoleOutput::CallBack()
{
	DEBUG(dbgTraCode, "In ConsoleOutput::CallBack(), " << kernel->stats->totalTicks);
    putBusy = FALSE;
    kernel->stats->numConsoleCharsWritten++;
    callWhenDone->CallBack(); // = SynchConsoleOutput
}

/***************************10.userprog/synchconsole.cc***************************/
//----------------------------------------------------------------------
// SynchConsoleOutput::CallBack
//      Interrupt handler called when it's safe to send the next 
//	character can be sent to the display.
//----------------------------------------------------------------------

void SynchConsoleOutput::CallBack()
{
    DEBUG(dbgTraCode, "In SynchConsoleOutput::CallBack(), " << kernel->stats->totalTicks);
    waitFor->V();
}
/***************************End***************************/
