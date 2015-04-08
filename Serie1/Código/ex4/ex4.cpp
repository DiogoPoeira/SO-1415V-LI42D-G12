#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include <stdbool.h>

bool isLeaking(int processID, int percent);

int _tmain(){
	printf("Insert process ID:\n");
	int id;
	scanf_s("%d",&id);
	isLeaking(id, 0);
	return 0;
}


bool isLeaking(int processID,int percent){
	HANDLE hProcess;
	PSAPI_WORKING_SET_INFORMATION pv, *buffer = 0;
	int countPrivate = 0, threshold = -1;
	int pv_size, block_size, itBlock, nTimes = 30;

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processID);
	if (!hProcess){
		_tprintf(_T("Falha ao abrir o processo (%d)\n"), GetLastError());
		return 1;
	}

	for (; nTimes >= 0; --nTimes){

		_tprintf(_T("ID Processo: %d\n"), processID);

		//verificar a quantidade de entradas nas pageframes, para alocar o tamanho correcto
		QueryWorkingSet(hProcess, (LPVOID)&pv, sizeof(pv));

		pv_size = sizeof(PSAPI_WORKING_SET_INFORMATION)+sizeof(PSAPI_WORKING_SET_BLOCK)* pv.NumberOfEntries;
		block_size = sizeof(PSAPI_WORKING_SET_BLOCK)* pv.NumberOfEntries;

		//alocar memoria dinamicamente para o alojamento da informacao das pageframes
		buffer = (PSAPI_WORKING_SET_INFORMATION*)malloc(pv_size);

		if (!QueryWorkingSet(hProcess, (LPVOID)buffer, pv_size)){
			_tprintf(_T("Falha na leitura do workingSet (%d) \n"), GetLastError());
			return 1;
		}

		for (itBlock = 0; itBlock < block_size / sizeof(PSAPI_WORKING_SET_BLOCK); ++itBlock){

			if ((buffer)->WorkingSetInfo[itBlock].Shared == 0)
				++countPrivate;

		}

		if (threshold == -1){
			threshold = ((countPrivate * percent) / 100) + countPrivate;
		}

		_tprintf(_T("threshold = %d\n"), threshold);

		if (countPrivate > threshold){
			_tprintf(_T("Ultrapassou o limite de pre-definido\n"));
		}

		_tprintf(_T("Numero de variaveis nao partilhadas %d \n"), countPrivate);

		countPrivate = 0;
		Sleep(5000);

	}

	free(buffer);
	CloseHandle(hProcess);

	return true;
}

