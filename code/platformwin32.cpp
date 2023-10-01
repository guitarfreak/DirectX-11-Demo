#pragma once

#include "input.h"

Meta_Parse_Struct(0);
struct MonitorData {
	Rect fullRect;
	Rect workRect;
	HMONITOR handle; // @Ignore
};

Meta_Parse_Struct(0);
struct WindowSettings {
	Vec2i res;
	Vec2i fullRes;
	bool fullscreen;
	uint style;
	WINDOWPLACEMENT g_wpPrev; // @Ignore
	Rect previousWindowRect;

	MonitorData monitors[3];
	int monitorCount;
	Vec2i biggestMonitorSize;
	int refreshRate;

	Vec2i currentRes;
	float aspectRatio;
	float windowScale;

	bool dontUpdateCursor;
	bool customCursor;
	POINT lastMousePosition; // @Ignore

	bool vsync;
	int frameRate;
};

Meta_Parse_Struct(0);
struct SystemData {
	WindowsData windowsData; // @Ignore
	HINSTANCE instance; // @Ignore
	HDC deviceContext; // @Ignore
	HWND windowHandle; // @Ignore

	HANDLE folderHandles[5]; // @Ignore
	int folderHandleCount; // @Ignore

	//

	Input* input; // @Ignore
	void* mainFiber; // @Ignore
	void* messageFiber; // @Ignore

	int coreCount;
	int fontHeight;

	bool maximized;
	bool killedFocus;
	bool setFocus;
	bool windowIsFocused;

	bool vsyncTempTurnOff;
};

//

LRESULT CALLBACK mainWindowCallBack(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	SystemData* sd = (SystemData*)GetWindowLongPtrA(window, GWLP_USERDATA);

	switch(message) {
		case WM_DESTROY: {
			PostMessage(window, message, wParam, lParam);
		} break;

		case WM_CLOSE: {
			PostMessage(window, message, wParam, lParam);
		} break;

		case WM_QUIT: {
			PostMessage(window, message, wParam, lParam);
		} break;

		// #ifdef ENABLE_CUSTOM_WINDOW_FRAME
		// case WM_NCACTIVATE: {
		// 	sd->vsyncTempTurnOff = true;
		// 	SwitchToFiber(sd->mainFiber);
		// } break;
		// #endif

		case WM_SIZE: {
			if(wParam == SIZE_MAXIMIZED) sd->maximized = true;
			else if(wParam == SIZE_RESTORED) sd->maximized = false;

			// sd->vsyncTempTurnOff = true;
			sd->input->resize = true;
		} break;

		// case WM_NCPAINT: {
		//     HDC hdc;
		//     hdc = GetDCEx(window, (HRGN)wParam, DCX_WINDOW|DCX_INTERSECTRGN);
		//     // Paint into this DC
		//     ReleaseDC(window, hdc);

		//     // sd->vsyncTempTurnOff = true;
		//     // SwitchToFiber(sd->mainFiber);

		// 	return 0;
		// } break;

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(window, &ps);
			EndPaint(window, &ps);

			#if USE_FIBERS
			sd->vsyncTempTurnOff = true;
			SwitchToFiber(sd->mainFiber);

			return 0;
			#endif
		} break;

		case WM_SETFOCUS: {
			sd->setFocus = true;
			sd->windowIsFocused = true;

			#if USE_FIBERS
			sd->vsyncTempTurnOff = true;
			SwitchToFiber(sd->mainFiber);
			#endif
		} break;

		case WM_KILLFOCUS: {
			sd->killedFocus = true;
			sd->windowIsFocused = false;

			#if USE_FIBERS
			sd->vsyncTempTurnOff = true;
			SwitchToFiber(sd->mainFiber);
			#endif
		} break;

		#if USE_FIBERS
		case WM_TIMER: {
			sd->vsyncTempTurnOff = true;
			SwitchToFiber(sd->mainFiber);
		} break;
		#endif

		// Make alt+enter not beep....
		case WM_MENUCHAR: {
			if(LOWORD(wParam) & VK_RETURN)
				return MAKELRESULT(0, MNC_CLOSE);
			return DefWindowProc(window, message, wParam, lParam);
		} break;

		default: {
			return DefWindowProc(window, message, wParam, lParam);
		} break;
	}

	return 1;
}

BOOL CALLBACK monitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
void CALLBACK updateInput(SystemData* sd);

void initSystem(SystemData* systemData, WindowSettings* ws, WindowsData wData, Vec2i res, int style, int, int monitor = 0) {
	systemData->windowsData = wData;

	EnumDisplayMonitors(0, 0, monitorEnumProc, ((LPARAM)ws));

	ws->biggestMonitorSize = vec2i(0,0);
	for(int i = 0; i < arrayCount(ws->monitors); i++) {
		Rect r = ws->monitors[i].fullRect;
		ws->biggestMonitorSize.w = max(ws->biggestMonitorSize.w, (int)r.w());
		ws->biggestMonitorSize.h = max(ws->biggestMonitorSize.h, (int)r.h());
	}

	DEVMODE devMode;
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
	ws->refreshRate = devMode.dmDisplayFrequency;


	ws->currentRes = res;
	ws->fullscreen = false;
	ws->aspectRatio = (float)res.w / (float)res.h;

	ws->style = style;

	RECT cr = {0, 0, res.w, res.h};
	AdjustWindowRectEx(&cr, ws->style, 0, 0);

	int ww = cr.right - cr.left;
	int wh = cr.bottom - cr.top;
	int wx, wy;
	{
		MonitorData* md = ws->monitors + monitor;
		wx = md->workRect.cx() - res.w/2;
		wy = md->workRect.cy() - res.h/2;
	}
	ws->res = vec2i(ww, wh);

	WNDCLASS windowClass = {};
	windowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;

	windowClass.lpfnWndProc = mainWindowCallBack;
	windowClass.hInstance = systemData->instance;
	windowClass.lpszClassName = "App";
	// windowClass.hCursor = LoadCursor(0, IDC_ARROW);
	windowClass.hCursor = 0;

	if(!RegisterClass(&windowClass))
		assertLogPrint(false, "System", Log_Error, fString("Unable to register window class. (Error code: %d.)", GetLastError()));

	// systemData->windowClass = windowClass;
	systemData->windowHandle = CreateWindowEx(0, windowClass.lpszClassName, "", ws->style, wx,wy,ww,wh, 0, 0, systemData->instance, 0);

	HWND windowHandle = systemData->windowHandle;
	assertLogPrint(windowHandle, "System", Log_Error, fString("Unable to create window. (Error code: %d.)", GetLastError()));

	SetWindowLongPtr(windowHandle, GWLP_USERDATA, (LONG_PTR)systemData);

	#ifndef HID_USAGE_PAGE_GENERIC
	#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
	#endif
	#ifndef HID_USAGE_GENERIC_MOUSE
	#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
	#endif

	RAWINPUTDEVICE Rid[1];
	Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
	Rid[0].hwndTarget = windowHandle;
	Rid[0].dwFlags = RIDEV_INPUTSINK;
	// Rid[0].dwFlags = 0;
	bool r = RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));
	assert(r);


	#if USE_FIBERS
	systemData->mainFiber = ConvertThreadToFiber(0);
	systemData->messageFiber = CreateFiber(0, (PFIBER_START_ROUTINE)updateInput, systemData);
	#endif

	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	systemData->coreCount = sysinfo.dwNumberOfProcessors;

	// Set icon.
	{
		char* rs = MAKEINTRESOURCE(1);
		HANDLE hbicon = LoadImage(GetModuleHandle(0), rs, IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
		if(hbicon) SendMessage(windowHandle, WM_SETICON, ICON_BIG, (LPARAM)hbicon);

		HANDLE hsicon = LoadImage(GetModuleHandle(0), rs, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
		if(hsicon) SendMessage(windowHandle, WM_SETICON, ICON_SMALL, (LPARAM)hsicon);
	}

	// Set minimal sleep timer resolution.
	{
		TIMECAPS timecaps;
		timeGetDevCaps(&timecaps, sizeof(TIMECAPS));
		int error = timeBeginPeriod(timecaps.wPeriodMin);
		if(error != TIMERR_NOERROR) logPrint("System", Log_Warning, fString("Couldn't set timer sleep resolution."), true);
	}

	SetFocus(windowHandle);
	systemData->windowIsFocused = true;
}

void systemDataInit(SystemData* sd, HINSTANCE instance) {
	sd->instance = instance;
}

// @Window.

BOOL CALLBACK monitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {

	MONITORINFO mi = { sizeof(MONITORINFO) };
	GetMonitorInfo(hMonitor, &mi);

	WindowSettings* ws = (WindowSettings*)(dwData);
	MonitorData* md = ws->monitors + ws->monitorCount;
	md->fullRect = rect(mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom);
	md->workRect = rect(mi.rcWork.left, mi.rcWork.top, mi.rcWork.right, mi.rcWork.bottom);
	md->handle = hMonitor;
	ws->monitorCount++;

	return true;
}

Rect getWindowWindowRect(HWND windowHandle) {
	RECT r;
	GetWindowRect(windowHandle, &r);
	Rect windowRect = rect(r.left, r.bottom, r.right, r.top);

	return windowRect;
}

void getWindowProperties(HWND windowHandle, int* viewWidth, int* viewHeight, int* width, int* height, int* x, int* y) {
	RECT cr;
	GetClientRect(windowHandle, &cr);
	*viewWidth = cr.right - cr.left;
	*viewHeight = cr.bottom - cr.top;

	if(width && height) {
		RECT wr;
		GetWindowRect(windowHandle, &wr);
		*width = wr.right - wr.left;
		*height = wr.bottom - wr.top;
	}

	if(x && y) {
		WINDOWPLACEMENT placement;
		GetWindowPlacement(windowHandle, &placement);
		RECT r;
		r = placement.rcNormalPosition;
		*x = r.left;
		*y = r.top;
	}
}

void setWindowProperties(HWND windowHandle, int width, int height, int x, int y) {
	WINDOWPLACEMENT placement;
	GetWindowPlacement(windowHandle, &placement);
	RECT r = placement.rcNormalPosition;

	if(width != -1) r.right = r.left + width;
	if(height != -1) r.bottom = r.top + height;
	if(x != -1) {
		int width = r.right - r.left;
		r.left = x;
		r.right = x + width;
	}
	if(y != -1) {
		int height = r.bottom - r.top;
		r.top = y;
		r.bottom = y + height;
	}

	placement.rcNormalPosition = r;
	SetWindowPlacement(windowHandle, &placement);
}

enum WindowMode {
	WINDOW_MODE_WINDOWED = 0,
	WINDOW_MODE_FULLBORDERLESS,

	WINDOW_MODE_COUNT,
};

void setWindowStyle(HWND hwnd, DWORD dwStyle) {
	SetWindowLong(hwnd, GWL_STYLE, dwStyle);
}

DWORD getWindowStyle(HWND hwnd) {
	return GetWindowLong(hwnd, GWL_STYLE);
}

void updateResolution(HWND windowHandle, WindowSettings* ws) {
	getWindowProperties(windowHandle, &ws->currentRes.x, &ws->currentRes.y,0,0,0,0);
	ws->aspectRatio = ws->currentRes.w / (float)ws->currentRes.h;

	{
		MONITORINFO monitorInfo;
		monitorInfo.cbSize = sizeof(MONITORINFO);
		bool result = GetMonitorInfo(MonitorFromWindow(windowHandle, MONITOR_DEFAULTTONEAREST), &monitorInfo);
		RECT rWork = monitorInfo.rcMonitor;
		Vec2i monitorRes = vec2i(rWork.right - rWork.left, rWork.bottom - rWork.top);

		ws->windowScale = (float)ws->currentRes.h / monitorRes.h;
	}
}

void setWindowMode(HWND hwnd, WindowSettings* wSettings, int mode) {
	if(mode == WINDOW_MODE_FULLBORDERLESS && !wSettings->fullscreen) {
		wSettings->previousWindowRect = getWindowWindowRect(hwnd);

		wSettings->g_wpPrev = {};

		DWORD dwStyle = getWindowStyle(hwnd);
		if (dwStyle & WS_OVERLAPPEDWINDOW) {
			MONITORINFO mi = { sizeof(mi) };
			if (GetWindowPlacement(hwnd, &wSettings->g_wpPrev) &&
			        GetMonitorInfo(MonitorFromWindow(hwnd,
			                       MONITOR_DEFAULTTOPRIMARY), &mi)) {
				SetWindowLong(hwnd, GWL_STYLE,
				              dwStyle & ~WS_OVERLAPPEDWINDOW);
				setWindowStyle(hwnd, dwStyle & ~WS_OVERLAPPEDWINDOW);

				SetWindowPos(hwnd, HWND_TOP,
				             mi.rcMonitor.left, mi.rcMonitor.top,
				             mi.rcMonitor.right - mi.rcMonitor.left,
				             mi.rcMonitor.bottom - mi.rcMonitor.top,
				             SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
			}
		}

		wSettings->fullscreen = true;

	} else if(mode == WINDOW_MODE_WINDOWED && wSettings->fullscreen) {
		setWindowStyle(hwnd, wSettings->style);
		SetWindowPlacement(hwnd, &wSettings->g_wpPrev);

		wSettings->fullscreen = false;

		InvalidateRect(NULL, NULL, FALSE);
	}
}

bool windowHasFocus(HWND windowHandle) {
	bool result = GetFocus() == windowHandle;
	return result;
}

bool windowSizeChanged(HWND windowHandle, WindowSettings* ws) {
	Vec2i cr;
	getWindowProperties(windowHandle, &cr.x, &cr.y, 0, 0, 0, 0);

	bool result = cr != ws->currentRes;
	return result;
}

bool windowIsMinimized(HWND windowHandle) {
	return IsIconic(windowHandle);
}

bool windowIsMaximized(HWND windowHandle) {
	WINDOWPLACEMENT placement = {sizeof(WINDOWPLACEMENT)};
	GetWindowPlacement(windowHandle, &placement);
	if(placement.showCmd == SW_SHOWMAXIMIZED) return true;
	else return false;
}

void makeWindowTopmost(SystemData* sd) {
	SetWindowPos(sd->windowHandle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void showWindow(HWND windowHandle) {
	ShowWindow(windowHandle, SW_SHOW);
}

Rect getScreenRect(Vec2i res) {
	return rectTLDim(0, 0, res.w, res.h);
}

Rect getScreenRect(WindowSettings* ws) {
	return getScreenRect(ws->currentRes);
}

// @Time.

void sleep(int milliseconds) {
	Sleep(milliseconds);
}

uint getTicks() {
	uint result = GetTickCount();

	return result;
}

__int64 getCycleStamp() {
	return __rdtsc();
}

struct Timer {
	double frequency;
	LARGE_INTEGER timeStamp;

	double dt;

	void init() {
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);

		this->frequency = (double)frequency.QuadPart;
	}

	void start() {
		QueryPerformanceCounter(&timeStamp);
	}

	double stop() {
		LARGE_INTEGER newTimeStamp;
		QueryPerformanceCounter(&newTimeStamp);

		dt = newTimeStamp.QuadPart - timeStamp.QuadPart;

		// In seconds.
		dt /= frequency;

		return dt;
	}

	double update() {
		double time = stop();
		start();

		return time;
	}
};

struct SectionTimer {
	struct TimeLabel {
		double time;
		char* label;
	};

	Timer* timer;
	TimeLabel sections[20];
	int sectionCount;
	double totalTime;

	void start(Timer* timer) {
		this->timer = timer;
		timer->start();
		sectionCount = 0;
		totalTime = 0;
	}
	void stop() { timer->stop(); }

	void add(char* label) {
		double ct = timer->update();
		sections[sectionCount++] = {ct, label};
		totalTime += ct;
	}

	void stopAndPrint() {
		stop();

		int maxTextWidth = 0;
		for(int i = 0; i < sectionCount; i++) {
			maxTextWidth = max(maxTextWidth, strLen(sections[i].label));
		}

		printf("Startup Times:\n");
		for(int i = 0; i < sectionCount; i++) {
			printf("%-*s %fs\n", maxTextWidth, sections[i].label, sections[i].time);
		}
		printf("%-*s %fs\n", maxTextWidth, "Total", totalTime);
	}
};

// @Folder.

void folderExistsCreate(char* path) {
	bool folderExists = PathFileExistsA(path);
	if(!folderExists) {
		CreateDirectory(path, 0);
	}
}

// @Misc.

void shellExecute(char* command) {
	system(command);
}

void shellExecuteNoWindow(char* command) {
	STARTUPINFO si = {};
	PROCESS_INFORMATION pi = {};
	si.cb = sizeof(si);

	if(CreateProcess(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}

// Don't forget to close after opening it.
char* getClipboard() {
	BOOL result = OpenClipboard(0);
	HANDLE clipboardHandle = GetClipboardData(CF_TEXT);
	// char* data = GlobalLock(clipboardHandle);
	char* data = (char*)clipboardHandle;

	return data;
}

void closeClipboard() {
	CloseClipboard();
}

void setClipboard(char* text) {
	int textSize = strLen(text) + 1;
	HANDLE clipHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, textSize);
	char* pointer = (char*)GlobalLock(clipHandle);
	memcpy(pointer, (char*)text, textSize);
	GlobalUnlock(clipHandle);

	OpenClipboard(0);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, clipHandle);
	CloseClipboard();
}

void* mallocWithBaseAddress(void* baseAddress, int sizeInBytes) {
	void* mem = VirtualAlloc(baseAddress, (size_t)sizeInBytes, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	DWORD error = GetLastError();

	return mem;
}

inline void atomicAdd(volatile unsigned int* n) { InterlockedIncrement(n); }
inline void atomicSub(volatile unsigned int* n) { InterlockedDecrement(n); }

void swapBuffers(SystemData* systemData) {
	SwapBuffers(systemData->deviceContext);
}

int getSystemFontHeight(HWND windowHandle) {
	HDC dc = GetDC(windowHandle);

	TEXTMETRIC textMetric;
	GetTextMetrics(dc, &textMetric);

	return textMetric.tmHeight;
}

void updateFontSizes(SystemData* sd, int* fontHeight, float fontScale, int* fontHeightScaled, float windowScale) {
	sd->fontHeight = getSystemFontHeight(sd->windowHandle);

	*fontHeight = roundInt(sd->fontHeight * fontScale);
	*fontHeightScaled = roundInt(sd->fontHeight * windowScale);
}

char* GetHResultErrorMessageText(HRESULT hr) {
	DWORD errorCode = HRESULT_CODE(hr);

	WCHAR wszMsgBuff[512];
	char string[512];
	DWORD dwChars;

	dwChars = FormatMessageW( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorCode, 0, wszMsgBuff, 512, NULL );

	int len = wcstombs(string, wszMsgBuff, sizeof(wszMsgBuff));

	char* result = fString("Error Code %d: %s", errorCode, dwChars ? string : "Error message not found.");
	return result;
}

#if 0
#include <psapi.h>

void printMemoryUsage() {
	PROCESS_MEMORY_COUNTERS_EX pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));

	// SIZE_T commited = pmc.PrivateUsage;
	// SIZE_T workingSet = pmc.WorkingSetSize;

	float PeakWorkingSetSize = pmc.PeakWorkingSetSize/1024.0f/1024.0f;
	float WorkingSetSize = pmc.WorkingSetSize/1024.0f/1024.0f;
	float QuotaPeakPagedPoolUsage = pmc.QuotaPeakPagedPoolUsage/1024.0f/1024.0f;
	float QuotaPagedPoolUsage = pmc.QuotaPagedPoolUsage/1024.0f/1024.0f;
	float QuotaPeakNonPagedPoolUsage = pmc.QuotaPeakNonPagedPoolUsage/1024.0f/1024.0f;
	float QuotaNonPagedPoolUsage = pmc.QuotaNonPagedPoolUsage/1024.0f/1024.0f;
	float PagefileUsage = pmc.PagefileUsage/1024.0f/1024.0f;
	float PeakPagefileUsage = pmc.PeakPagefileUsage/1024.0f/1024.0f;
	float PrivateUsage = pmc.PrivateUsage/1024.0f/1024.0f;

	// printf("%f %f\n", commited/1024.0f/1024.0f, workingSet/1024.0f/1024.0f);
	printf("%f %f %f %f %f %f %f %f %f\n",
	       PeakWorkingSetSize,
	       WorkingSetSize,
	       QuotaPeakPagedPoolUsage,
	       QuotaPagedPoolUsage,
	       QuotaPeakNonPagedPoolUsage,
	       QuotaNonPagedPoolUsage,
	       PagefileUsage,
	       PeakPagefileUsage,
	       PrivateUsage
	      );
}
#endif