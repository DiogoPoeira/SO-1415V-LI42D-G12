// SimpleTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "usynch.h"
#include "List.h"

/////////////////////////////////////////////
//
// CCISEL 
// 2007-2011
//
// UThread    Library First    Test
//
// Jorge Martins, 2011
////////////////////////////////////////////
#define DEBUG

#define MAX_THREADS 10
#define STACK_SIZE (16 * 4096)

#include <crtdbg.h>
#include <stdio.h>
#include "..\Include\Uthread.h"


///////////////////////////////////////////////////////////////
//															 //
// Test 1: N threads, each one printing its number M times	 //
//															 //
///////////////////////////////////////////////////////////////

ULONG Test1_Count, TestUtJoin_Count;


VOID Test1_Thread (UT_ARGUMENT Argument) {
	UCHAR Char;
	ULONG Index;
	Char = (UCHAR) Argument;	

	for (Index = 0; Index < 10000; ++Index) {
	    putchar(Char);
		
		 
	    if ((rand() % 4) == 0) {
		    UtYield();
	    }	 
    }
	++Test1_Count;
}

VOID TestUtJoin_JoiningThread(UT_ARGUMENT Argument) {
	TestUtJoin_Count++;
	HANDLE ThreadHandle = (HANDLE)Argument;
	printf("Thread %d is Starting!\n", TestUtJoin_Count);
	printf("Thread %d is Joining!\n\n", TestUtJoin_Count);
	_ASSERTE(UtJoin(ThreadHandle));
	printf("Thread %d has been notified that the work has been done!\n\n", TestUtJoin_Count);
	TestUtJoin_Count--;
}

VOID TestUtJoin_WorkerThread(UT_ARGUMENT Argument) {
	UtYield();
	TestUtJoin_Count++;
	printf("Thread %d is Starting!\n", TestUtJoin_Count);
	printf("Thread %d is the Worker thread!\n", TestUtJoin_Count);
	printf("Thread %d has finished the work!\n\n", TestUtJoin_Count);
	TestUtJoin_Count--;
}

VOID Test1 ()  {
	ULONG Index;

	
	Test1_Count = 0; 

	printf("\n :: Test 1 - BEGIN :: \n\n");

	for (Index = 0; Index < MAX_THREADS; ++Index) {
		UtCreate(Test1_Thread, (UT_ARGUMENT) ('0' + Index),STACK_SIZE,"");
	}   

	UtRun();

	_ASSERTE(Test1_Count == MAX_THREADS);
	printf("\n\n :: Test 1 - END :: \n");
}

VOID Dumb_TestThread(UT_ARGUMENT Argument){
	UtYield();
}

VOID Test_UtGetContextSwitch(){
	printf("\n\n :: Test UtGetContextSwitch - START :: \n\n");
	UtCreate(Dumb_TestThread, NULL, STACK_SIZE, "");
	UtCreate(Dumb_TestThread, NULL, STACK_SIZE, "");
	UtCreate(Dumb_TestThread, NULL, STACK_SIZE, "");
	UtCreate(Dumb_TestThread, NULL, STACK_SIZE, "");
	UtRun();
	printf("Number of Context Switches : %d\n", UtGetSwitchCount());
	_ASSERTE(9 == UtGetSwitchCount());
	printf("\n\n :: Test UtGetContextSwitch - END :: \n\n");
}

VOID Test_UtJoin()  {
	printf("\n\n :: Test UtJoin - START :: \n\n");
	HANDLE h = UtCreate(TestUtJoin_WorkerThread, NULL, STACK_SIZE, "");
	UtCreate(TestUtJoin_JoiningThread, h, STACK_SIZE, "");
	UtCreate(TestUtJoin_JoiningThread, h, STACK_SIZE, "");
	UtCreate(TestUtJoin_JoiningThread, h, STACK_SIZE, "");
	UtRun();
	printf("\n\n :: Test UtJoin - END :: \n");
}

VOID Dumb_UTDumpTestThread(UT_ARGUMENT Argument){
	UtYield();
	UtDump();
}

VOID Dumb_UTDumpJoinTestThread(UT_ARGUMENT Argument){
	HANDLE ThreadHandle = (HANDLE)Argument;
	_ASSERTE(UtJoin(ThreadHandle));
}

VOID Dumb_UTDumpReadyTestThread(UT_ARGUMENT Argument){ UtYield(); }

VOID Test_UtDump(){
	printf("\n\n :: Test UtDump - START :: \n\n");
	HANDLE h = UtCreate(Dumb_UTDumpTestThread, NULL, STACK_SIZE, "RUNNING THREAD 1");
	UtCreate(Dumb_UTDumpJoinTestThread, h, STACK_SIZE, "JOINING THREAD 1");
	UtCreate(Dumb_UTDumpJoinTestThread, h, STACK_SIZE, "JOINING THREAD 2");
	UtCreate(Dumb_UTDumpJoinTestThread, h, STACK_SIZE, "JOINING THREAD 3");
	UtCreate(Dumb_UTDumpReadyTestThread, NULL, STACK_SIZE, "READY THREAD 1");
	UtCreate(Dumb_UTDumpReadyTestThread, NULL, STACK_SIZE, "READY THREAD 2");
	UtCreate(Dumb_UTDumpReadyTestThread, NULL, STACK_SIZE, "READY THREAD 3");
	UtRun();
	printf("\n\n :: Test UtDump - END :: \n");
}

VOID Test_UtAliveThread(UT_ARGUMENT Argument){
	_ASSERT(UtAlive(Argument));
	printf("\n The Thread is Alive! \n");
}

VOID Test_UtDeadThread(UT_ARGUMENT Argument){
	_ASSERT(!UtAlive(Argument));
	printf("\n The Thread is Dead! \n");
}

VOID Test_UtAlive(){
	printf("\n\n :: Test UtAlive - START :: \n\n");
	HANDLE h1 = UtCreate(Dumb_TestThread, NULL, STACK_SIZE, "");
	HANDLE h2 = UtCreate(Test_UtAliveThread, h1, STACK_SIZE, "");
	UtCreate(Test_UtDeadThread, h2, STACK_SIZE, "");
	UtRun();
	printf("\n\n :: Test UtAlive - END :: \n");
}

VOID Test_CountElapsedThread(UT_ARGUMENT Argument){
	printf("\nContext Switch time : %d\n",UtSwitchTime());
}

VOID Test_CountElapsedThreadTerminator(UT_ARGUMENT Argument){
	UtYield();
	printf("\nContext Switch time : %d\n", UtSwitchTime());
	printf("Context Switch total time : %d\n", UtSwitchTimeTotal());
}

VOID Test_SwitchCount(){
	printf("\n\n :: Test UtSwitchTime - START :: \n\n");
	UtCreate(Test_CountElapsedThreadTerminator, NULL, STACK_SIZE, "READY THREAD 1");
	UtCreate(Test_CountElapsedThread, NULL, STACK_SIZE, "READY THREAD 1");
	UtCreate(Test_CountElapsedThread, NULL, STACK_SIZE, "READY THREAD 1");
	UtCreate(Test_CountElapsedThread, NULL, STACK_SIZE, "READY THREAD 1");
	UtCreate(Test_CountElapsedThread, NULL, STACK_SIZE, "READY THREAD 1");
	UtCreate(Test_CountElapsedThread, NULL, STACK_SIZE, "READY THREAD 1");
	UtRun();
	printf("\n\n :: Test UtSwitchTime - END :: \n\n");
}


int main () {
	printf("\n\n :: Test InternalStart without Function and Arguments on Thread Structure - START :: \n\n");
	UtInit();
	Test_UtGetContextSwitch();
	getchar();
	UtEnd();

	UtInit();
	Test_UtJoin();
	getchar();
	UtEnd();

	UtInit();
	Test_UtDump();
	getchar();
	UtEnd();

	UtInit();
	Test_UtAlive();
	getchar();
	UtEnd();

	UtInit();
	Test_SwitchCount();
	getchar();
	UtEnd();

	UtInit();
	Test1();
	getchar();
	UtEnd();

	printf("\n\n :: Test InternalStart without Function and Arguments on Thread Structure - END :: \n\n");
	return 0;
}


