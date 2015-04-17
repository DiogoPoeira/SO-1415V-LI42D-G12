#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include <stdbool.h>

bool isLeaking(int processID, int percent);

int _tmain(){
	printf("Insira o ID do processo:\n");
	int id,max_size;
	scanf_s("%d",&id);
	printf("Insira o tamanho máximo (em MB):\n");
	scanf_s("%d", &max_size);
	printf("\n");
	isLeaking(id, max_size*1024*1024);
	printf("Terminou a Leitura!\n");
	return 0;
}


bool isLeaking(int processID,int threshold){
	HANDLE process_handler;
	PSAPI_WORKING_SET_INFORMATION wsi, *buffer = 0;
	int buffer_size, iterator, iterations = 30, private_memory = 0, private_vars = 0;

	process_handler = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processID);
	if (!process_handler){
		_tprintf(_T("Falha ao abrir o processo %d (%d)\n"),processID, GetLastError());
		return 1;
	}

	for (; iterations >= 0; --iterations){

		_tprintf(_T("ID do Processo: %d\n"), processID);

		QueryWorkingSet(process_handler, (LPVOID)&wsi, sizeof(wsi));
		buffer_size = sizeof(PSAPI_WORKING_SET_INFORMATION)+sizeof(PSAPI_WORKING_SET_BLOCK)* wsi.NumberOfEntries;

		buffer = (PSAPI_WORKING_SET_INFORMATION*)malloc(buffer_size);

		if (!QueryWorkingSet(process_handler, (LPVOID)buffer, buffer_size)){
			_tprintf(_T("Falha na leitura do workingSet (%d) \n"), GetLastError());
			return false;
		}


		for (iterator = 0; iterator < wsi.NumberOfEntries; ++iterator){

			if (buffer->WorkingSetInfo[iterator].Shared == 0){
				private_vars++;
			}
		}

		private_memory = private_vars*sizeof(PSAPI_WORKING_SET_BLOCK)*1024;
		_tprintf(_T("Tamanho pre-definido: %d KB\n"), threshold/1024);


		if (private_memory > threshold){
			_tprintf(_T("Ultrapassou o limite de pre-definido\n"));
		}

		_tprintf(_T("Numero de variaveis nao partilhadas %d \n"), private_vars);
		_tprintf(_T("Memoria usada :  %d KB \n\n"), private_memory/1024);

		private_vars = 0;
		private_memory = 0;
		free(buffer);
		Sleep(5000);

	}


	CloseHandle(process_handler);

	return true;
}

