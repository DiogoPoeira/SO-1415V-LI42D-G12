/////////////////////////////////////////////////////////////////
//
// CCISEL 
// 2007-2011
//
// UThread library:
//   User threads supporting cooperative multithreading.
//
// Authors:
//   Carlos Martins, João Trindade, Duarte Nunes, Jorge Martins
// 

#include <crtdbg.h>
#include <stdio.h>
#include "UThreadInternal.h"

//////////////////////////////////////
//
// UThread internal state variables.
//

//
// The number of existing user threads.
//
static
ULONG NumberOfThreads;

//
// The sentinel of the circular list linking the user threads that are
// currently schedulable. The next thread to run is retrieved from the
// head of this list.
//
static
LIST_ENTRY ReadyQueue;

static
LIST_ENTRY AliveList;

//
// The currently executing thread.
//
#ifndef _WIN64
static
#endif
PUTHREAD RunningThread;

static
ULONG SwitchCount;

static
ULONG SwitchTime;

static
ULONG SwitchStart;

static
ULONG SwitchTimeTotal;

//
// The user thread proxy of the underlying operating system thread. This
// thread is switched back in when there are no more runnable user threads,
// causing the scheduler to exit.
//
static
PUTHREAD MainThread;

////////////////////////////////////////////////
//
// Forward declaration of internal operations.
//

//
// The trampoline function that a user thread begins by executing, through
// which the associated function is called.
//
static
VOID InternalStart(UT_FUNCTION Function, UT_ARGUMENT Argument);


#ifdef _WIN64
//
// Performs a context switch from CurrentThread to NextThread.
// In x64 calling convention CurrentThread is in RCX and NextThread in RDX.
//
VOID __fastcall  ContextSwitch64 (PUTHREAD CurrentThread, PUTHREAD NextThread);

//
// Frees the resources associated with CurrentThread and switches to NextThread.
// In x64 calling convention  CurrentThread is in RCX and NextThread in RDX.
//
VOID __fastcall InternalExit64 (PUTHREAD Thread, PUTHREAD NextThread);

#define ContextSwitch ContextSwitch64
#define InternalExit InternalExit64

#else

static
VOID __fastcall ContextSwitch32 (PUTHREAD CurrentThread, PUTHREAD NextThread);

//
// Frees the resources associated with CurrentThread and switches to NextThread.
// __fastcall sets the calling convention such that CurrentThread is in ECX
// and NextThread in EDX.
//
static
VOID __fastcall InternalExit32 (PUTHREAD Thread, PUTHREAD NextThread);

#define ContextSwitch ContextSwitch32
#define InternalExit InternalExit32
#endif

////////////////////////////////////////
//
// UThread inline internal operations.
//

//
// Returns and removes the first user thread in the ready queue. If the ready
// queue is empty, the main thread is returned.
//
static
FORCEINLINE
PUTHREAD ExtractNextReadyThread () {
	if (IsListEmpty(&ReadyQueue))
		return MainThread;
	return CONTAINING_RECORD(RemoveHeadList(&ReadyQueue), UTHREAD, Link);
}

//
// Schedule a new thread to run
//
static
FORCEINLINE
VOID Schedule () {
	PUTHREAD NextThread;
    NextThread = ExtractNextReadyThread();
	NextThread->State = RUNNING;
	SwitchStart = GetTickCount();
	ContextSwitch(RunningThread, NextThread);
}

///////////////////////////////
//
// UThread public operations.
//

//
// Initialize the scheduler.
// This function must be the first to be called. 
//
VOID UtInit() {
	InitializeListHead(&ReadyQueue);
	InitializeListHead(&AliveList);
	SwitchCount = 0;
	SwitchTime = 0;
	SwitchTimeTotal = 0;
}

//
// Cleanup all UThread internal resources.
//
VOID UtEnd() {
	/* (this function body was intentionally left empty) */
}

//
// Run the user threads. The operating system thread that calls this function
// performs a context switch to a user thread and resumes execution only when
// all user threads have exited.
//
VOID UtRun () {
	UTHREAD Thread; // Represents the underlying operating system thread.

	//
	// There can be only one scheduler instance running.
	//
	_ASSERTE(RunningThread == NULL);

	//
	// At least one user thread must have been created before calling run.
	//
	if (IsListEmpty(&ReadyQueue)) {
		return;
	}

	//
	// Switch to a user thread.
	//
	MainThread = &Thread;
	RunningThread = MainThread;
	Schedule();
 
	//
	// When we get here, there are no more runnable user threads.
	//
	_ASSERTE(IsListEmpty(&ReadyQueue));
	_ASSERTE(NumberOfThreads == 0);

	//
	// Allow another call to UtRun().
	//
	RunningThread = NULL;
	MainThread = NULL;
}

VOID UtDump(){
	UThreadDump((HANDLE)RunningThread);
	PLIST_ENTRY curr = ReadyQueue.Flink;
	while (curr != &ReadyQueue)
	{
		UThreadDump(CONTAINING_RECORD(&(*curr), UTHREAD, Link));
		curr = curr->Flink;
	}
}

VOID UThreadDump(HANDLE threadHandle){
	PUTHREAD thread = (PUTHREAD)threadHandle;
	ULONG stack_addr = thread->Stack, esp = thread->ThreadContext;
	DOUBLE stack_percentage = ((DOUBLE)(thread->Stack_Size - (esp - stack_addr)) * 100) / (DOUBLE)thread->Stack_Size;
	printf("\n :: THREAD DUMP :: \n Handle : %p \n Name : %s \n State : %s \n Stack Ocupation (percentage) : %f%% \n",
		threadHandle,
		thread->Name,
		thread->State,
		stack_percentage);
	PLIST_ENTRY curr = thread->Joining_threads.Flink;
	while (curr != &thread->Joining_threads)
	{
		UThreadDump(CONTAINING_RECORD(&(*curr),UTHREAD,Link));
		curr = curr->Flink;
	}
}


//
// Terminates the execution of the currently running thread. All associated
// resources are released after the context switch to the next ready thread.
//
VOID UtExit () {
	NumberOfThreads -= 1;	
	PLIST_ENTRY curr = RunningThread->Joining_threads.Flink,aux;
	while (curr != &RunningThread->Joining_threads)
	{
		aux = curr;
		curr = curr->Flink;
		((PUTHREAD)aux)->State=READY;
		InsertTailList(&ReadyQueue, aux);
	}
	curr = AliveList.Flink;
	while (curr != &AliveList)
	{
		if (curr == &RunningThread->Alive_Link){
			curr->Blink->Flink = curr->Flink;
			curr->Flink->Blink = curr->Blink;
			break;
		}
		curr = curr->Flink;
	}
	SwitchStart = GetTickCount();
	InternalExit(RunningThread, ExtractNextReadyThread());
	_ASSERTE(!"Supposed to be here!");
}

//
// Relinquishes the processor to the first user thread in the ready queue.
// If there are no ready threads, the function returns immediately.
//
VOID UtYield () {
	if (!IsListEmpty(&ReadyQueue)) {
		InsertTailList(&ReadyQueue, &RunningThread->Link);
		RunningThread->State = READY;
		Schedule();
	}
}

//
// Returns a HANDLE to the executing user thread.
//
HANDLE UtSelf () {
	return (HANDLE)RunningThread;
}




//
// Halts the execution of the current user thread.
//
VOID UtDeactivate() {
	Schedule();
}


//
// Places the specified user thread at the end of the ready queue, where it
// becomes eligible to run.
//
VOID UtActivate (HANDLE ThreadHandle) {
	InsertTailList(&ReadyQueue, &((PUTHREAD)ThreadHandle)->Link);
}

BOOL UtJoin(HANDLE thread){
	PUTHREAD puthread = (PUTHREAD)thread;
	if (puthread == RunningThread || !UtAlive(thread)) return FALSE;
	RunningThread->State = BLOCKED;
	InsertTailList(&puthread->Joining_threads, &RunningThread->Link);
	UtDeactivate();
	return TRUE;
}

BOOL UtAlive(HANDLE thread){
	PLIST_ENTRY curr = AliveList.Flink;
	PUTHREAD pThread = (PUTHREAD)thread;
	while (curr != &AliveList)
	{
		if (curr == &pThread->Alive_Link)return TRUE;
		curr = curr->Flink;
	}
	return FALSE;
}

///////////////////////////////////////
//
// Definition of internal operations.
//

//
// The trampoline function that a user thread begins by executing, through
// which the associated function is called.
//
VOID InternalStart (UT_FUNCTION Function , UT_ARGUMENT Argument) {
	SwitchTime = GetTickCount() - SwitchStart;
	SwitchTimeTotal += SwitchTime;
	Function(Argument);
	UtExit(); 
}


//
// Frees the resources associated with Thread..
//
VOID __fastcall CleanupThread (PUTHREAD Thread) {
	free(Thread->Stack);
	free(Thread);
}

ULONG UtGetSwitchCount(){
	return SwitchCount;
}

DWORD UtSwitchTime(){
	return SwitchTime;
}

DWORD UtSwitchTimeTotal(){
	return SwitchTimeTotal;
}

//
// functions with implementation dependent of X86 or x64 platform
//

#ifndef _WIN64

//
// Creates a user thread to run the specified function. The thread is placed
// at the end of the ready queue.
//
HANDLE UtCreate32 (UT_FUNCTION Function, UT_ARGUMENT Argument,ULONG Stack_Size, PCHAR Name) {
	PUTHREAD Thread;
	
	//
	// Dynamically allocate an instance of UTHREAD and the associated stack.
	//
	Thread = (PUTHREAD) malloc(sizeof (UTHREAD));
	Thread->Stack = (PUCHAR)malloc(Stack_Size);
	_ASSERTE(Thread != NULL && Thread->Stack != NULL);
	InitializeListHead(&Thread->Joining_threads);
	Thread->Name = Name;
	//
	// Zero the stack for emotional confort.
	//
	memset(Thread->Stack, 0, Stack_Size);
	Thread->Stack_Size = Stack_Size;
	//
	// Map an UTHREAD_CONTEXT instance on the thread's stack.
	// We'll use it to save the initial context of the thread.
	//
	// +------------+
	// | 0x00000000 |    <- Highest word of a thread's stack space
	// +============+       (needs to be set to 0 for Visual Studio to
	// |  RetAddr   | \     correctly present a thread's call stack).
	// +------------+  |
	// |    EBP     |  |
	// +------------+  |
	// |    EBX     |   >   Thread->ThreadContext mapped on the stack.
	// +------------+  |
	// |    ESI     |  |
	// +------------+  |
	// |    EDI     | /  <- The stack pointer will be set to this address
	// +============+       at the next context switch to this thread.
	// |            | \
	// +------------+  |
	// |     :      |  |
	//       :          >   Remaining stack space.
	// |     :      |  |
	// +------------+  |
	// |            | /  <- Lowest word of a thread's stack space
	// +------------+       (Thread->Stack always points to this location).
	//

	Thread->ThreadContext = (PUTHREAD_CONTEXT) (Thread->Stack +
		Stack_Size -sizeof(UT_FUNCTION) - sizeof(UT_ARGUMENT) - sizeof (ULONG)-sizeof (UTHREAD_CONTEXT));

	//
	// Set the thread's initial context by initializing the values of EDI,
	// EBX, ESI and EBP (must be zero for Visual Studio to correctly present
	// a thread's call stack) and by hooking the return address.
	// 
	// Upon the first context switch to this thread, after popping the dummy
	// values of the "saved" registers, a ret instruction will place the
	// address of InternalStart on EIP.
	//
	Thread->ThreadContext->EDI = 0x33333333;
	Thread->ThreadContext->EBX = 0x11111111;
	Thread->ThreadContext->ESI = 0x22222222;
	Thread->ThreadContext->EBP = 0x00000000;									  
	Thread->ThreadContext->RetAddr = InternalStart;
	//
	// Memorize Function and Argument for use in InternalStart.
	//
	memcpy((void *)(Thread->Stack + Stack_Size - sizeof(UT_ARGUMENT)), &Argument, sizeof(UT_ARGUMENT));
	memcpy((void *)(Thread->Stack + Stack_Size - sizeof(UT_FUNCTION)-sizeof(UT_ARGUMENT)), &Function, sizeof(UT_FUNCTION));


	//
	// Ready the thread.
	//
	NumberOfThreads += 1;
	InsertTailList(&AliveList, &Thread->Alive_Link);
	UtActivate((HANDLE)Thread);

	Thread->State = READY;
	
	return (HANDLE)Thread;
}

//
// Performs a context switch from CurrentThread to NextThread.
// __fastcall sets the calling convention such that CurrentThread is in ECX and NextThread in EDX.
// __declspec(naked) directs the compiler to omit any prologue or epilogue.
//
__declspec(naked) 
VOID __fastcall ContextSwitch32 (PUTHREAD CurrentThread, PUTHREAD NextThread) {
	SwitchCount++;
	__asm {
		// Switch out the running CurrentThread, saving the execution context on the thread's own stack.   
		// The return address is atop the stack, having been placed there by the call to this function.
		//
		push	ebp
		push	ebx
		push	esi
		push	edi
		//
		// Save ESP in CurrentThread->ThreadContext.
		//
		mov		dword ptr [ecx].ThreadContext, esp
		//
		// Set NextThread as the running thread.
		//
		mov     RunningThread, edx
		//
		// Load NextThread's context, starting by switching to its stack, where the registers are saved.
		//
		mov		esp, dword ptr [edx].ThreadContext

		pop		edi
		pop		esi
		pop		ebx
		pop		ebp
		//
		// Jump to the return address saved on NextThread's stack when the function was called.
		//
		ret
	}

}



//
// Frees the resources associated with CurrentThread and switches to NextThread.
// __fastcall sets the calling convention such that CurrentThread is in ECX and NextThread in EDX.
// __declspec(naked) directs the compiler to omit any prologue or epilogue.
//
__declspec(naked)
VOID __fastcall InternalExit32 (PUTHREAD CurrentThread, PUTHREAD NextThread) {
	SwitchCount++;
	__asm {

		//
		// Set NextThread as the running thread.
		//
		mov     RunningThread, edx
		
		//
		// Load NextThread's stack pointer before calling CleanupThread(): making the call while
		// using CurrentThread's stack would mean using the same memory being freed -- the stack.
		//
		mov		esp, dword ptr [edx].ThreadContext

		call    CleanupThread

		//
		// Finish switching in NextThread.
		//
		pop		edi
		pop		esi
		pop		ebx
		pop		ebp
		ret
	}
}

#else

//
// Creates a user thread to run the specified function. The thread is placed
// at the end of the ready queue.
//
HANDLE UtCreate64 (UT_FUNCTION Function, UT_ARGUMENT Argument,ULONG Stack_Size, PCHAR Name) {
	PUTHREAD Thread;
	
	//
	// Dynamically allocate an instance of UTHREAD and the associated stack.
	//
	Thread = (PUTHREAD) malloc(sizeof (UTHREAD));
	Thread->Stack = (PUCHAR) malloc(Stack_Size);
	_ASSERTE(Thread != NULL && Thread->Stack != NULL);
	InitializeListHead(&Thread->Joining_threads);
	Thread->Name = Name;
	//
	// Zero the stack for emotional confort.
	//
	memset(Thread->Stack, 0, Stack_Size);

	//
	// Memorize Function and Argument for use in InternalStart.
	//
	Thread->Stack_Size = Stack_Size;
	//
	// Map an UTHREAD_CONTEXT instance on the thread's stack.
	// We'll use it to save the initial context of the thread.
	//
	// +------------+  <- Highest word of a thread's stack space
	// | 0x00000000 |    (needs to be set to 0 for Visual Studio to
	// +------------+      correctly present a thread's call stack).   
	// | 0x00000000 |  \
	// +------------+   |
	// | 0x00000000 |   | <-- Shadow Area for Internal Start 
	// +------------+   |
	// | 0x00000000 |   |
	// +------------+   |
	// | 0x00000000 |  /
	// +============+       
	// |  RetAddr   | \    
	// +------------+  |
	// |    RBP     |  |
	// +------------+  |
	// |    RBX     |   >   Thread->ThreadContext mapped on the stack.
	// +------------+  |
	// |    RDI     |  |
	// +------------+  |
	// |    RSI     |  |
	// +------------+  |
	// |    R12     |  |
	// +------------+  |
	// |    R13     |  |
	// +------------+  |
	// |    R14     |  |
	// +------------+  |
	// |    R15     | /  <- The stack pointer will be set to this address
	// +============+       at the next context switch to this thread.
	// |            | \
	// +------------+  |
	// |     :      |  |
	//       :          >   Remaining stack space.
	// |     :      |  |
	// +------------+  |
	// |            | /  <- Lowest word of a thread's stack space
	// +------------+       (Thread->Stack always points to this location).
	//

	Thread->ThreadContext = (PUTHREAD_CONTEXT) (Thread->Stack +
		Stack_Size -sizeof (UTHREAD_CONTEXT)-sizeof(ULONGLONG)*5);

	//
	// Set the thread's initial context by initializing the values of 
	// registers that must be saved by the called (R15,R14,R13,R12, RSI, RDI, RBCX, RBP)
	
	// 
	// Upon the first context switch to this thread, after popping the dummy
	// values of the "saved" registers, a ret instruction will place the
	// address of InternalStart on EIP.
	//
	Thread->ThreadContext->R15 = 0x77777777;
	Thread->ThreadContext->R14 = 0x66666666;
	Thread->ThreadContext->R13 = 0x55555555;
	Thread->ThreadContext->R12 = 0x44444444;	
	Thread->ThreadContext->RSI = 0x33333333;
	Thread->ThreadContext->RDI = 0x11111111;
	Thread->ThreadContext->RBX = 0x22222222;
	Thread->ThreadContext->RBP = 0x11111111;		
	Thread->ThreadContext->RetAddr = InternalStart;


	memcpy((Thread->ThreadContext + sizeof(UTHREAD_CONTEXT)), &Function, sizeof(UT_FUNCTION));
	memcpy((Thread->ThreadContext + sizeof(UTHREAD_CONTEXT)+sizeof(UT_FUNCTION)), &Argument, sizeof(UT_ARGUMENT));

	//
	// Ready the thread.
	//
	NumberOfThreads += 1;
	InsertTailList(&AliveList, &Thread->Alive_Link);
	UtActivate((HANDLE)Thread);

	Thread->State = READY;
	
	return (HANDLE)Thread;
}




#endif