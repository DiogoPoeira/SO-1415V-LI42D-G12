#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include <stdbool.h>
#include <Winbase.h>

int  adjust_bmp(LPCTSTR bmp_name, int adjustment);

int _tmain(int argc, _TCHAR* argv[]){
	if (argc < 3)return 1;
	int adjustment = _ttoi(argv[1]);
	if (adjustment<-50 || adjustment>50)return 1;
	_TCHAR *name = argv[2];
	return adjust_bmp(name, adjustment);

}

int  adjust_bmp(LPCTSTR bmp_name, int adjustment){
	BITMAPFILEHEADER *bmp_header;
	BITMAPINFOHEADER *bmp_info;
	DWORD bytesread;
	HANDLE bmp_file = CreateFile(bmp_name, GENERIC_WRITE | GENERIC_READ, 0,	NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL),
		bmp_file_mapping;
	if (bmp_file == NULL){
		_tprintf(_T("Falha ao abrir o ficheiro %s\n"), bmp_name);
		return 1;
	}
	bmp_file_mapping = CreateFileMapping(bmp_file, NULL, PAGE_READWRITE, 0, 0,NULL);
	bmp_header = (BITMAPFILEHEADER*)MapViewOfFile(bmp_file_mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	bmp_info = (BITMAPINFOHEADER*)(bmp_header+1);
	if (!bmp_header){
		_tprintf(_T("Falha ao ler o Header da informacao do ficheiro %s\n"), bmp_name);
		CloseHandle(bmp_file);
		return 1;
	}
	if (bmp_header->bfType != 'MB'){
		_tprintf(_T("Falha! O ficheiro %s nao e do tipo bmp\n"), bmp_name);
		CloseHandle(bmp_file);
		return 1;
	}
	if (bmp_info->biBitCount != 24){
		_tprintf(_T("Falha! O ficheiro %s nao e um bmp a 24bits\n"), bmp_name);
		CloseHandle(bmp_file);
		return 1;
	}
	int size = bmp_header->bfSize - bmp_header->bfOffBits, value,linebytes = bmp_info->biWidth * sizeof(RGBTRIPLE),
		line_length, padding = 0, padding_total = 0;
	BYTE* iterator;
	while ((linebytes + padding) % 4 != 0)padding++;
	line_length = (linebytes + padding);
	for (long i = 0; i < size;i++){
		if (i%line_length == linebytes)
			i += padding;
		iterator = ((BYTE*)(bmp_header)+bmp_header->bfOffBits) + i;
		value = *iterator + adjustment;
		if (value < 0)value = 0;
		if (value > 255)value = 255;
		*iterator = value;
	}
	UnmapViewOfFile(bmp_header);
	CloseHandle(bmp_file);
	return 0;
}
