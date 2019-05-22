// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

//--------------------------------MP3-----------------------------//
int L1comp(Thread *newThread, Thread *oldThread)
{
    //		returns -1 if x < y
    //		returns 0 if x == y
    //		returns 1 if x > y
    int newPriority = newThread->getPriority();
    int oldPriority = oldThread->getPriority();
    int newTime = newThread->getPredictBurstTime();
    int oldTime = newThread->getPredictBurstTime();

    if(newPriority < oldPriority) // new thread high priority
        return 1;
    else if(newPriority > oldPriority) // new thread low priority
        return -1;
    else if(newTime < oldTime) // new thread low time
        return -1;
    else if(newTime > oldTime) // new thread high time
        return 1;
    else
        return 0;
}

int L2comp(Thread *newThread, Thread *oldThread)
{
    //		returns -1 if x < y
    //		returns 0 if x == y
    //		returns 1 if x > y
    int newPriority = newThread->getPriority();
    int oldPriority = oldThread->getPriority();

    if(newPriority < oldPriority) // new thread high priority
        return 1;
    else if(newPriority > oldPriority) // new thread low priority
        return -1;
    else
        return 0;
}
//--------------------------------MP3-----------------------------//

Scheduler::Scheduler()
{ 
    // readyList = new List<Thread *>;
    //--------------------------------MP3-----------------------------//
    L1 = new SortedList<Thread *>(L1comp);
    L2 = new SortedList<Thread *>(L2comp);
    L3 = new List<Thread *>; 
    //--------------------------------MP3-----------------------------//
    toBeDestroyed = NULL;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    // delete readyList; 
    //--------------------------------MP3-----------------------------//
    delete L1;
    delete L2;
    delete L3;
    //--------------------------------MP3-----------------------------//
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    // DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    // DEBUG(dbgOS, "in function scheduler::readytorun");
    if (thread->getStatus() == BLOCKED) {
        // DEBUG(dbgOS, "return from IO so set burst time zero");
        // thread->setBurstTime(0);
    }
    thread->setStatus(READY);
    //--------------------------------MP3-----------------------------//
    int priority = thread->getPriority();
    int queueLevel = 0; 
    if(priority >= 100 && priority <= 149){
        L1->Insert(thread);
        queueLevel = 1;
        DEBUG(dbgOS, "New L1 Thread PredicTime: " << thread->getPredictBurstTime() << " Curr Thread " << kernel->currentThread->getName() << " RemainTime: " << kernel->currentThread->getPredictBurstTime()-kernel->currentThread->getBurstTime());
    }
    else if(priority >= 50 && priority <= 99){
        L2->Insert(thread);
        queueLevel = 2;
    }
    else if(priority >= 0 && priority <= 49){
        L3->Append(thread);
        queueLevel = 3;
    }
    else {
        ASSERTNOTREACHED();
    }
    DEBUG(dbgOS, "Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getName() << "] is inserted into queue L[" << queueLevel << "]");
    //--------------------------------MP3-----------------------------//
    // readyList->Append(thread);
    // printf("Tick [%d]: Thread [%s] is inserted into readyList\n", kernel->stats->totalTicks, thread->getName());
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    // DEBUG(dbgOS, "in function scheduler::findnexttorun");
    // if (readyList->IsEmpty()) {
	// 	return NULL;
    // } else {
    // 	return readyList->RemoveFront();
    // }
    kernel->scheduler->Aging();
    //--------------------------------MP3-----------------------------//
    Thread *RemoveThread;
    if (!L1->IsEmpty()) {
        RemoveThread = L1->RemoveFront();
        DEBUG(dbgOS, "Tick [" << kernel->stats->totalTicks << "]: Thread [" << RemoveThread->getName() << "] is removed from L[1]");
    } else if (!L2->IsEmpty()) {
        RemoveThread = L2->RemoveFront();
        DEBUG(dbgOS, "Tick [" << kernel->stats->totalTicks << "]: Thread [" << RemoveThread->getName() << "] is removed from L[2]");
    } else if (!L3->IsEmpty()) {
        RemoveThread = L3->RemoveFront();
        DEBUG(dbgOS, "Tick [" << kernel->stats->totalTicks << "]: Thread [" << RemoveThread->getName() << "] is removed from L[3]");
    } else {
        RemoveThread = NULL;
        //printf("Tick [%d]: All Queue Empty\n",  kernel->stats->totalTicks);
    }
    //--------------------------------MP3-----------------------------//
    return RemoveThread;
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    // DEBUG(dbgOS, "in function scheduler::run");
    DEBUG(dbgTraCode, "old Thread: " << oldThread->getName() << ", next Thread: " << nextThread->getName());
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
        ASSERT(toBeDestroyed == NULL);
	    toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	    oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow


    kernel->currentThread = nextThread;  // switch to the next thread
    
    //--------------------------------MP3-----------------------------//
    DEBUG(dbgOS, "Tick [" << kernel->stats->totalTicks << "]: Thread [" << nextThread->getName() << "] is now selected for execution");
    DEBUG(dbgOS, "Tick [" << kernel->stats->totalTicks << "]: Thread [" << oldThread->getName() << "] is replaced, and it has executed [" << oldThread->getBurstTime() << "] ticks");
    if (oldThread->getStatus() == BLOCKED || oldThread == nextThread) {
        // DEBUG(dbgOS, "go to IO so set burst time zero");
        oldThread->setBurstTime(0);
    }
    int nextPriority = nextThread->getPriority();
    if (nextPriority >= 0 && nextPriority < 50) {
        kernel->alarm->EnableTimer();
    } else {
        kernel->alarm->DisableTimer();
    }
    //--------------------------------MP3-----------------------------//

    nextThread->setStatus(RUNNING);      // nextThread is now running

    // DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".
    if (nextThread != oldThread) {
        SWITCH(oldThread, nextThread);
    }
    
    // we're back, running oldThread
    
    int oldPriority = oldThread->getPriority();
    if (oldPriority >= 0 && oldPriority < 50) {
        kernel->alarm->EnableTimer();
    } else {
        kernel->alarm->DisableTimer();
    }

    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	    oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    //readyList->Apply(ThreadPrint);
}

//---------------------------------MP3-------------------------------------//
void
Scheduler::Aging()
{
    ListIterator<Thread *> *L2_iter = new ListIterator<Thread *>((List<Thread *> *)L2);
    ListIterator<Thread *> *L3_iter = new ListIterator<Thread *>(L3);

    Thread* tmp;

    int start, old_piority, new_piority, agecount, currenttick;

    // DEBUG(dbgOS, "in function scheduler::aging");

    for (; !L2_iter->IsDone(); L2_iter->Next()) {
        tmp = L2_iter->Item();

        start = tmp->getStartTime();
        old_piority = tmp->getPriority();
        currenttick = kernel->stats->totalTicks;
        agecount = tmp->getAgeCount();

        new_piority = old_piority + ((currenttick - start) / 1500 - agecount) * 10;
        if (new_piority >= 149){
            new_piority = 149;
        }

        if (new_piority != old_piority){
            DEBUG(dbgOS, "Tick [" << kernel->stats->totalTicks << "]: Thread [" << tmp->getName() << "] changes its priority from [" << old_piority << "] to [" << new_piority << "]");
        }    
        tmp->setPriority(new_piority);
        tmp->setAgeCount((currenttick - start) / 1500);

        if (new_piority >= 100 && new_piority < 150) {
            L1->Insert(tmp);
            L2->Remove(tmp);
            setPreemptive(TRUE);
        } else if (new_piority >= 50 && new_piority < 100) {
            //Do nothing
        } else {
            ASSERTNOTREACHED();
        }
        if (L2->IsEmpty()) break;
    }

    for (; !L3_iter->IsDone(); L3_iter->Next()) {
        tmp = L3_iter->Item();
        
        start = tmp->getStartTime();
        old_piority = tmp->getPriority();
        currenttick = kernel->stats->totalTicks;
        agecount = tmp->getAgeCount();

        new_piority = old_piority + ((currenttick - start) / 1500 - agecount) * 10;
        if (new_piority >= 149){
            new_piority = 149;
        }

        if (new_piority != old_piority){
            DEBUG(dbgOS, "Tick [" << kernel->stats->totalTicks << "]: Thread [" << tmp->getName() << "] changes its priority from [" << old_piority << "] to [" << new_piority << "]");
        }
        tmp->setPriority(new_piority);
        tmp->setAgeCount((currenttick - start) / 1500);

        if (new_piority >= 100 && new_piority < 150) {
            L1->Insert(tmp);
            L3->Remove(tmp);
            setPreemptive(TRUE);
        }else if (new_piority >= 50 && new_piority < 100) {
            L2->Insert(tmp);
            L3->Remove(tmp);
        } else if (new_piority >=0 && new_piority < 50) {
            //Do nothing
        } else {
            ASSERTNOTREACHED();
        }
        if (L3->IsEmpty()) break;
    }
}
//---------------------------------MP3-------------------------------------//