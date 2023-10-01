#pragma once

#ifndef DECLSPEC_IMPORT
#if (defined(_M_IX86) || defined(_M_IA64) || defined(_M_AMD64) || defined(_M_ARM) || defined(_M_ARM64)) && !defined(MIDL_PASS)
#define DECLSPEC_IMPORT __declspec(dllimport)
#else
#define DECLSPEC_IMPORT
#endif
#endif

#define EXTERN_C extern "C"
#define LWSTDAPI_(type) EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE

LWSTDAPI_(BOOL) PathFileExistsA(__in LPCSTR pszPath);

struct HotloadDll {
    char * libFilePath; 
    char * libTempFilePath; 
    char * lockFilePath;

    HMODULE dll;
    FILETIME lastLibWriteTime;
};

inline FILETIME getLastWriteTime(char *filename) {
    FILETIME lastWriteTime = {};
    
    WIN32_FIND_DATAA data = {};
    if(GetFileAttributesExA(filename, GetFileExInfoStandard, &data)) {
        lastWriteTime = data.ftLastWriteTime;
    }

    return lastWriteTime;
}

void loadDll(HotloadDll* hotloadDll) {
	CopyFileA(hotloadDll->libFilePath, hotloadDll->libTempFilePath, FALSE);
	hotloadDll->dll = LoadLibraryA(hotloadDll->libTempFilePath);
	hotloadDll->lastLibWriteTime = getLastWriteTime(hotloadDll->libFilePath);	
}

void initDll(HotloadDll* hotloadDll, char* functionName, char* functionTemp, char* lock) {
	hotloadDll->libFilePath = functionName; 
	hotloadDll->libTempFilePath = functionTemp; 
	hotloadDll->lockFilePath = lock; 

	loadDll(hotloadDll);
}

bool updateDll(HotloadDll* hotloadDll) {
	bool reload = false;
    FILETIME newLibWriteTime = getLastWriteTime(hotloadDll->libFilePath);
    if(CompareFileTime(&newLibWriteTime, &hotloadDll->lastLibWriteTime) != 0 && 
    	(PathFileExistsA(hotloadDll->lockFilePath) == FALSE)) {
        FreeLibrary(hotloadDll->dll);

        loadDll(hotloadDll);
        reload = true;
    }

    return reload;
}

void* getDllFunction(HotloadDll* hotloadDll, char* functionName) {
	void* result = GetProcAddress(hotloadDll->dll, functionName);
	return result; 
}

//

struct WindowsData {
	HINSTANCE instance;
	HINSTANCE prevInstance;
	LPSTR commandLine;
	int showCode;
};

WindowsData windowsData(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode) {
	WindowsData wData;
	wData.instance = instance;
	wData.prevInstance = prevInstance;
	wData.commandLine = commandLine;
	wData.showCode = showCode;

	return wData;
}

//

struct ThreadQueue;
#define APPMAINFUNCTION(name) void name(bool init, bool reload, bool* isRunning, WindowsData windowsData, ThreadQueue* threadQueue, AppMemory* appMemory)
typedef APPMAINFUNCTION(appMainType);
appMainType* platform_appMain;