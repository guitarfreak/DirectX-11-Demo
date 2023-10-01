#pragma once 

#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define arrayCount(array) (sizeof(array) / sizeof((array)[0]))
#define addPointer(ptr, int) ptr = (char*)ptr + (int)
#define memberSize(type, member) sizeof(((type *)0)->member)
#define memberArrayCount(type, member) arrayCount(((type *)0)->member)
#define memberOffsetSize(type, member) vec2i(offsetof(type, member), memberSize(type, member))
#define mallocArray(type, count) (type*)mallocX(sizeof(type)*(count))
#define mallocStruct(type) (type*)mallocX(sizeof(type))
#define mallocString(count) (char*)mallocX(sizeof(char)*(count))

#define allocaArray(type, count) (type*)alloca(sizeof(type)*(count))
#define allocaStruct(type) (type*)alloca(sizeof(type))
#define allocaString(count) (char*)alloca(sizeof(char)*(count))

#define zeroStruct(s, structType) zeroMemory(s, sizeof(structType));
#define copyArray(dst, src, type, count) memcpy(dst, src, sizeof(type)*(count));
#define copyStaticArray(dst, src, type) memcpy(dst, src, sizeof(type)*(arrayCount(src)));
#define moveArray(dst, src, type, count) memmove(dst, src, sizeof(type)*(count));
#define PVEC2(v) v.x, v.y
#define PVEC3(v) v.x, v.y, v.z
#define PVEC4(v) v.x, v.y, v.z, v.w
#define PRECT(r) r.left, r.bottom, r.right, r.top

#define VDref(type, ptr) (*((type*)ptr))

#define For_Array(array, count, type) \
	for(type* it = array; (it-array) < count; it++)

#define aIndex(w, x, y) ((y)*(w) + (x))
#define aIndex3D(w, h, x, y, z) ((z)*(h)*(w) + (y)*(w) + (x))
void aCoord(int w, int i, int* x, int* y) {
	*x = i % w;
	*y = i / w;
};

#define writeTypeAndAdvance(buf, val, type) \
		(*(type*)buf) = val; buf += sizeof(type); 

#define readTypeAndAdvance(buf, type) \
		(*(type*)buf); buf += sizeof(type); 

void assertPrint(bool check, char* message) {
	if(!check) {
		printf(message);
		if(IsDebuggerPresent()) {
			__debugbreak();
		}
		exit(1);
	}
}

int myAssert(bool check) {
	if(!check) {

		printf("Assert fired!\n");
		if(IsDebuggerPresent()) {
			__debugbreak();
		}
		exit(1);
	}
	return -1;
}

// So we can debug break here and see who's taking memory.
inline void* mallocX(size_t size) {
	return malloc(size);
}

inline void zeroMemory(void* memory, int size) {
	memset(memory, 0, size);
}

inline void freeAndSetNullSave(void* data) {
	if(data) {
		free(data);
		data = 0;
	}
}

// #define mallocArrayResizeSave(type, ptr, count) \
// 	freeAndSetNullSave(ptr);               \
// 	(ptr) = mallocArray(type, (count));

// #define mallocStructResizeSave(type, ptr) \
// 	freeAndSetNullSave(ptr);         \
// 	(ptr) = mallocStruct(type);

// #define mallocStringResizeSave(ptr, count) \
// 	freeAndSetNullSave(ptr);          \
// 	(ptr) = mallocString((count));

inline void _mallocArrayResize(void** data, int* count, int newCount) {
	if(!(*data)) return;
	free(*data);
	*data = malloc(newCount);
	*count = newCount;
}

#define mallocArrayResize(type, data, count, newCount) \
	_mallocArrayResize((void**)data, count, sizeof(type) * (newCount)); \
	*count /= sizeof(type);

inline void _resizeRingBuffer(void** data, int *count, int *index, int newCount, int elementSize) {
	void* newData = malloc(newCount * elementSize);
	
	// Copy first part of ring buffer to front.
	memcpy(newData, *data, (*index) * elementSize);

	// Copy second part of ring buffer to end of new buffer.
	int endSize = (*count) - (*index);
	memcpy((char*)newData + (newCount - endSize) * elementSize, (char*)(*data) + (*index) * elementSize, endSize * elementSize);

	free(*data);
	*data = newData;
	*count = newCount;
}

#define resizeRingBuffer(type, data, count, index, newCount) \
	_resizeRingBuffer((void**)(data), count, index, newCount, sizeof(type));

void freeZero(void* data) {
	if(data) {
		free(data);
		data = 0;
	}
}

inline int strLen(char* str) {	
	int len = 0;
	while(str[len] != '\0') len++;

	return len;
}

//

int timeToSeconds(int year = 0, int month = 0, int day = 0, int hour = 0, int minute = 0, int seconds = 0) {
	// year is 20XX, only works up to ~2060
	return (year * 31556926 + month * 2629743.83 + day * 86400 +
		hour * 3600 + minute * 60 + seconds);
};

inline i64 terraBytes(i64 count) { return count * 1024 * 1024 * 1024 * 1024; }
inline i64 gigaBytes(i64 count)  { return count * 1024 * 1024 * 1024; }
inline int megaBytes(int count)  { return count * 1024 * 1024; }
inline int kiloBytes(int count)  { return count * 1024; }

inline int flagSet(int flags, int flagType) { return flags | flagType; }
inline int flagRemove(int flags, int flagType) { return flags &= ~flagType; }
inline void flagSet(int* flags, int flagType) { (*flags) |= flagType; }
inline void flagRemove(int* flags, int flagType) { (*flags) &= ~flagType; }
inline bool flagGet(int flags, int flagType) { return (flags | flagType) == flags; }

//

struct defer_dummy {};
template <class T> struct deferrer { 
	T f; 
	bool callFunction;
	deferrer(T f) : f(f), callFunction(true) {}
	~deferrer() { if(callFunction) f(); } 
};
template <class T> deferrer<T> operator*(defer_dummy, T f) { return {f}; }
#define DEFER_(LINE) zz_defer##LINE
#define DEFER(LINE) DEFER_(LINE)
#define defer auto DEFER(__LINE__) = defer_dummy{} *[&]()

//

enum LogPriority {
	Log_Note,
	Log_Warning,
	Log_Error,
};

struct Logger {
	char* filePath;
	bool enabled;
	void init(char* filePath, bool enabled);
	void log(char* category, LogPriority priority, char* txt, bool print = false);
	void assertLog(bool check, char* category, LogPriority priority, char* txt, bool print = false);
};

void Logger::init(char* filePath, bool enabled) {
	if(!enabled) return;

	this->filePath = filePath;
	this->enabled = enabled;

	FILE* file = fopen(filePath, "wb");
	assertPrint(file, "Error: Could not create log file.\n");
	fclose(file);
}

void Logger::log(char* category, LogPriority priority, char* txt, bool print) {
	if(!enabled) return;

	static char* priorityStrings[] = {"", "Warning: ", "Error: "};

	// Have to always close and reopen to actually save the contents in case the program crashes.
	FILE* file = fopen(filePath, "ab");
	myAssert(file);
	fprintf(file, "%s: %s%s\r\n", category, priorityStrings[priority], txt);
	fclose(file);

	if(print || priority == Log_Error) printf("%s: %s%s\r\n", category, priorityStrings[priority], txt);
}

void Logger::assertLog(bool check, char* category, LogPriority priority, char* txt, bool print) {
	if(!check) {
		log(category, priority, txt); 
		assertPrint(check, txt); 
	}
}
