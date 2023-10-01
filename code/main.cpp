#ifndef SHIPPING_MODE

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
// #include <windows.h>
#include "external\win32\atomic.h"
#include "external\win32\dbghelp.h"
#include "external\win32\file.h"
#include "external\win32\io.h"
#include "external\win32\misc.h"
#include "external\win32\sysinfo.h"
#include "external\win32\threads.h"

#include "misc.cpp"
#include "memory.h"
#include "hotload.cpp"
#include "threadQueue.cpp"

#else 
#include "app.cpp"

#endif

#include "external\physical_processors.cpp"

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode) {
	#ifndef SHIPPING_MODE

	AllocConsole();
	freopen("conin$","r",stdin);
	freopen("conout$","w",stdout);
	freopen("conout$","w",stderr);

	HotloadDll hotloadDll;
	initDll(&hotloadDll, "app.dll", "appTemp.dll", "lock.tmp");

	#endif

	WindowsData wData = windowsData(instance, prevInstance, commandLine, showCode);

	// SYSTEM_INFO sysinfo;
	// GetSystemInfo(&sysinfo);
	// int coreCount = sysinfo.dwNumberOfProcessors;

	int logicalProc = 0;
	int physicalProc = physicalProcessors(&logicalProc); 

	ThreadQueue threadQueue;
	threadQueue.init(physicalProc-1, 100);

	AppMemory appMemory = {};

	bool firstFrame = true;
	bool isRunning = true;
	while(isRunning) {
		bool reload = false;

		#ifndef SHIPPING_MODE

		if(threadQueue.finished()) reload = updateDll(&hotloadDll);
		platform_appMain = (appMainType*)getDllFunction(&hotloadDll, "appMain");
		platform_appMain(firstFrame, reload, &isRunning, wData, &threadQueue, &appMemory);

		#else 

		appMain(firstFrame, reload, &isRunning, wData, &threadQueue, &appMemory);

		#endif

		if(firstFrame) firstFrame = false;
	}

	return 0;
}


