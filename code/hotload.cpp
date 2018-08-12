#pragma once

#define LWSTDAPI_(type) EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
LWSTDAPI_(BOOL) PathFileExistsA(__in LPCSTR pszPath);
#define PathFileExists PathFileExistsA

struct HotloadDll {
    char * libFilePath; 
    char * libTempFilePath; 
    char * lockFilePath;

    HMODULE dll;
    FILETIME lastLibWriteTime;
};

inline FILETIME getLastWriteTime(char *filename) {
    FILETIME lastWriteTime = {};
    
    WIN32_FIND_DATA data = {};
    if(GetFileAttributesEx(filename, GetFileExInfoStandard, &data)) {
        lastWriteTime = data.ftLastWriteTime;
    }

    return lastWriteTime;
}

void loadDll(HotloadDll* hotloadDll) {
	CopyFile(hotloadDll->libFilePath, hotloadDll->libTempFilePath, FALSE);
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
    	(PathFileExists(hotloadDll->lockFilePath) == FALSE)) {
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