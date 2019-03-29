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

	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	int coreCount = sysinfo.dwNumberOfProcessors;

	ThreadQueue threadQueue;
	// threadQueue.init(coreCount-1, 100);
	threadQueue.init(coreCount-1, 100);

	AppMemory appMemory = {};

	bool firstFrame = true;
	bool secondFrame = false;
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





// #define arrayCount(array) (sizeof(array) / sizeof((array)[0]))

// #define USE_AFFINITY false

// #include <windows.h>
// #include <cstdio>

// struct Timer {
// 	double frequency;
// 	LARGE_INTEGER timeStamp;
// 	double dt;

// 	void init() {
// 		LARGE_INTEGER frequency;
// 		QueryPerformanceFrequency(&frequency);
// 		this->frequency = (double)frequency.QuadPart;
// 	}

// 	void start() { QueryPerformanceCounter(&timeStamp); }

// 	double stop() {
// 		LARGE_INTEGER newTimeStamp;
// 		QueryPerformanceCounter(&newTimeStamp);

// 		dt = newTimeStamp.QuadPart - timeStamp.QuadPart;
// 		dt /= frequency;

// 		return dt;
// 	}
// };

// struct ThreadSettings {
// 	bool doJob;
// 	bool active;
// 	HANDLE semaphore;
// 	bool close;

// 	void* data;
// 	void (*function)(void* data);
// };

// DWORD WINAPI threadProcess(LPVOID data) {
// 	ThreadSettings* settings = (ThreadSettings*)data;
// 	while(true) {
// 		WaitForSingleObjectEx(settings->semaphore, INFINITE, FALSE);
// 		settings->function(settings->data);
// 		settings->active = false;

// 		// if(settings->doJob) {
// 		// 	settings->doJob = false;
// 		// 	// WaitForSingleObjectEx(settings->semaphore, INFINITE, FALSE);
// 		// 	settings->function(settings->data);
// 		// 	settings->active = false;
// 		// }
// 		if(settings->close) {
// 			break;
// 		}
// 	}

// 	return 0;
// }

// struct ThreadData {
// 	Timer timer;
// 	int count;
// 	float temp;
// };

// void threadFunc(void* data) {
// 	ThreadData* d = (ThreadData*)data;
// 	d->timer.start();

// 	d->temp = 0;
// 	int count = 1000000000 / d->count;
// 	for(int i = 0 ; i < count; i++) {
// 		d->temp += 123 * 456; // Do some work.
// 	}

// 	d->timer.stop();
// };

// int main(int argc, char** argv) {
// 	if(USE_AFFINITY) {
// 		__int64 threadMask = SetThreadAffinityMask(GetCurrentThread(), 1);
// 	}

// 	ThreadSettings settings[8] = {};

// 	int i = 0;
// 	for(int i = 0; i < 8; i++) {
// 		ThreadSettings* it = settings + i;
// 		it->active = false;
// 		it->semaphore = CreateSemaphoreA(0, 0, 1, 0);
// 		it->doJob = false;

// 		HANDLE thread = CreateThread(0, 0, threadProcess, it, 0, 0);
// 		if(USE_AFFINITY) {
// 			SetThreadAffinityMask(thread, 2 << i);
// 		}

// 		CloseHandle(thread);
// 	}

// 	Timer timer;
// 	timer.init();


	
// 	for(int i = 0; i < 8; i++) {
// 		int threadCount = i+1;

// 		timer.start();
// 		ThreadData threadData[8] = {};
// 		for(int i = 0; i < threadCount; i++) {
// 			Timer tim = timer;
// 			threadData[i].timer = tim;
// 			threadData[i].count = threadCount;
// 		}

// 		for(int i = 0; i < threadCount; i++) {
// 			if(i < threadCount-1) {
// 				settings[i].active = true;
// 				settings[i].function = threadFunc;
// 				settings[i].data = threadData + i;
// 				ReleaseSemaphore(settings[i].semaphore, 1, 0);
// 				// settings[i].doJob = true;
// 			}
// 			else {
// 				threadFunc(threadData + i);
// 				settings[i].doJob = false;
// 				settings[i].active = false;
// 			}
// 		}

// 		while(true) {
// 			bool done = true;
// 			for(int i = 0; i < threadCount-1; i++) {
// 				ThreadSettings* s = settings + i;
// 				if(s->active) {
// 					done = false;
// 					break;
// 				}
// 			}
// 			if(done) break;
// 		}

// 		timer.stop();

// 		printf("\n");
// 		for(int i = 0; i < threadCount; i++) {
// 			ThreadData* it = threadData + i;
// 			float startTime = (it->timer.timeStamp.QuadPart - timer.timeStamp.QuadPart) / timer.frequency;
// 			printf("Time : %f, Start time: %f\n", it->timer.dt, startTime);
// 		}
// 		printf("Total: %f, \n", timer.dt);
// 	}

// 	// Sleep(2000);

// 	for(auto& it : settings) {
// 		it.close = true;
// 	}

// 	Sleep(1);


// 	return 0;
// }



#if 0

#define USE_AFFINITY true

#include <windows.h>
#include <cstdio>

struct Timer {
	double frequency;
	LARGE_INTEGER timeStamp;
	double dt;

	void init() {
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		this->frequency = (double)frequency.QuadPart;
	}

	void start() { QueryPerformanceCounter(&timeStamp); }

	double stop() {
		LARGE_INTEGER newTimeStamp;
		QueryPerformanceCounter(&newTimeStamp);

		dt = newTimeStamp.QuadPart - timeStamp.QuadPart;
		dt /= frequency;

		return dt;
	}
};

struct ThreadSettings {
	bool doJob;
	bool active;
	HANDLE semaphore;

	void* data;
	void (*function)(void* data);
};

DWORD WINAPI threadProcess(LPVOID data) {
	ThreadSettings* settings = (ThreadSettings*)data;
	while(true) {
		WaitForSingleObjectEx(settings->semaphore, INFINITE, FALSE);
		settings->function(settings->data);
		settings->active = false;

		// if(settings->doJob) {
		// 	settings->doJob = false;
		// 	settings->function(settings->data);
		// 	settings->active = false;
		// }
	}

	return 0;
}

int main(int argc, char** argv) {

	// BOOL GetProcessAffinityMask(
	//   HANDLE     hProcess,
	//   PDWORD_PTR lpProcessAffinityMask,
	//   PDWORD_PTR lpSystemAffinityMask
	// );

	// DWORD_PTR mask;
	// DWORD_PTR sMask;
	// GetProcessAffinityMask(GetCurrentProcess(), &mask, &sMask);

	// ABOVE_NORMAL_PRIORITY_CLASS
	// SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	if(USE_AFFINITY) {
		__int64 threadMask = SetThreadAffinityMask(GetCurrentThread(), 1);
	}

	static ThreadSettings settings[8] = {};
	int i = 0;
	HANDLE handles[8];
	for(auto& it : settings) {
		it.active = false;
		it.semaphore = CreateSemaphoreA(0, 0, 1, 0);
		it.doJob = false;

		HANDLE thread = CreateThread(0, 0, threadProcess, &it, 0, 0);
		if(USE_AFFINITY) {
			SetThreadAffinityMask(thread, 2 << i);
		}
		handles[i] = thread;

		CloseHandle(thread);
		i++;
	}

	Timer timer;
	timer.init();

	struct ThreadData {
		Timer timer;
		int count;
		float temp;
	};

	auto threadFunc = [](void* data) {
		ThreadData* d = (ThreadData*)data;
		d->timer.start();

		d->temp = 0;
		int count = 5000000 / d->count;
		for(int i = 0 ; i < count; i++) {
			d->temp += 123 * 456; // Do some work.
		}

		d->timer.stop();
	};
	
	for(int i = 0; i < 8; i++) {
		int threadCount = i+1;

		timer.start();
		ThreadData threadData[8] = {};
		for(int i = 0; i < threadCount; i++) {
			Timer tim = timer;
			threadData[i] = {tim, threadCount};
		}

		for(int i = 0; i < threadCount; i++) {
			if(i < threadCount-1) {
				settings[i].active = true;
				settings[i].function = threadFunc;
				settings[i].data = threadData + i;
				ReleaseSemaphore(settings[i].semaphore, 1, 0);
				// settings[i].doJob = true;
			}
			else {
				threadFunc(threadData + i);
				settings[i].doJob = false;
				settings[i].active = false;
			}
		}

		// Sleep(10);

		while(true) {
			bool done = true;
			for(int i = 0; i < threadCount-1; i++) {
				ThreadSettings* s = settings + i;
				if(s->active) {
					done = false;
					break;
				}
			}
			if(done) break;
		}

		timer.stop();

		printf("\n");
		for(int i = 0; i < threadCount; i++) {
			ThreadData* it = threadData + i;
			float startTime = (it->timer.timeStamp.QuadPart - timer.timeStamp.QuadPart) / timer.frequency;
			printf("Time : %f, Start time: %f\n", it->timer.dt, startTime);
		}
		printf("Total: %f, \n", timer.dt);
	}

	return 0;
}

// // main.cpp
// // Total: 0.010032, 
// // 0.010030, 0.000002

// // Total: 0.005027, 
// // 0.005025, 0.000001
// // 0.005022, 0.000002

// // Total: 0.003446, 
// // 0.003345, 0.000001
// // 0.003444, 0.000002
// // 0.003336, 0.000003

// // Total: 0.002566, 
// // 0.002514, 0.000001
// // 0.002564, 0.000002
// // 0.002518, 0.000002
// // 0.002503, 0.000003

// // Total: 0.002060, 
// // 0.002003, 0.000001
// // 0.002020, 0.000002
// // 0.002032, 0.000002
// // 0.002028, 0.000003
// // 0.002056, 0.000004

// // Total: 0.003368, 
// // 0.001700, 0.000001
// // 0.001681, 0.000002
// // 0.001690, 0.000002
// // 0.001793, 0.000002
// // 0.001673, 0.001695
// // 0.002000, 0.000006

// // Total: 0.002893, 
// // 0.001430, 0.000001
// // 0.001438, 0.000002
// // 0.001430, 0.000002
// // 0.001441, 0.000002
// // 0.001457, 0.000011
// // 0.001453, 0.001440
// // 0.001430, 0.001433

// // Total: 0.002525, 
// // 0.001251, 0.000001
// // 0.001270, 0.000002
// // 0.001258, 0.000002
// // 0.001252, 0.000003
// // 0.001278, 0.000006
// // 0.001264, 0.001260
// // 0.001251, 0.001253
// // 0.001252, 0.000064


// // main.cpp
// // Total: 0.010076, 
// // 0.010010, 0.000063

// // Total: 0.005037, 
// // 0.005005, 0.000018
// // 0.005004, 0.000032

// // Total: 0.003425, 
// // 0.003335, 0.000003
// // 0.003336, 0.000014
// // 0.003396, 0.000027

// // Total: 0.002575, 
// // 0.002506, 0.000004
// // 0.002505, 0.000005
// // 0.002512, 0.000019
// // 0.002543, 0.000030

// // Total: 0.002066, 
// // 0.002001, 0.000004
// // 0.002001, 0.000005
// // 0.002003, 0.000006
// // 0.002002, 0.000019
// // 0.002031, 0.000034

// // Total: 0.001754, 
// // 0.001670, 0.000004
// // 0.001671, 0.000005
// // 0.001668, 0.000006
// // 0.001670, 0.000007
// // 0.001730, 0.000022
// // 0.001697, 0.000036

// // Total: 0.002867, 
// // 0.001454, 0.000004
// // 0.001439, 0.000004
// // 0.001430, 0.000006
// // 0.001430, 0.000006
// // 0.001430, 0.000008
// // 0.001430, 0.000022
// // 0.001430, 0.001436

// // Total: 0.002516, 
// // 0.001281, 0.000004
// // 0.001258, 0.000005
// // 0.001260, 0.000006
// // 0.001259, 0.000007
// // 0.001253, 0.000009
// // 0.001322, 0.000009
// // 0.001251, 0.001263
// // 0.001250, 0.001264



#endif