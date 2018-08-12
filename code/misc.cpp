#pragma once 

#include "types.cpp"
#include <stdio.h>

#define arrayCount(array) (sizeof(array) / sizeof((array)[0]))
#define addPointer(ptr, int) ptr = (char*)ptr + (int)
#define memberSize(type, member) sizeof(((type *)0)->member)
#define memberArrayCount(type, member) arrayCount(((type *)0)->member)
#define memberOffsetSize(type, member) vec2i(offsetof(type, member), memberSize(type, member))
#define mallocArray(type, count) (type*)malloc(sizeof(type)*(count))
#define mallocStruct(type) (type*)malloc(sizeof(type))
#define mallocString(count) (char*)malloc(sizeof(char)*(count))

#define allocaArray(type, count) (type*)alloca(sizeof(type)*(count))
#define allocaStruct(type) (type*)alloca(sizeof(type))
#define allocaString(count) (char*)alloca(sizeof(char)*(count))

#define zeroStruct(s, structType) zeroMemory(s, sizeof(structType));
#define copyArray(dst, src, type, count) memcpy(dst, src, sizeof(type)*(count));
#define moveArray(dst, src, type, count) memmove(dst, src, sizeof(type)*(count));
#define PVEC2(v) v.x, v.y
#define PVEC3(v) v.x, v.y, v.z
#define PVEC4(v) v.x, v.y, v.z, v.w
#define PRECT(r) r.left, r.bottom, r.right, r.top

#define VDref(type, ptr) (*((type*)ptr))

#define For_Array(array, count, type) \
	for(type* it = array; (it-array) < count; it++)

#define arrayIndex(w, x, y) (y*w + x)
#define arrayIndex3D(w, h, x, y, z) (z*h*w + y*w + x)

#define writeTypeAndAdvance(buf, val, type) \
		(*(type*)buf) = val; buf += sizeof(type); 

#define readTypeAndAdvance(buf, type) \
		(*(type*)buf); buf += sizeof(type); 

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

void zeroMemory(void* memory, int size) {
	memset(memory, 0, size);
}

void freeAndSetNullSave(void* data) {
	if(data) {
		free(data);
		data = 0;
	}
}

#define reallocArraySave(type, ptr, count) \
	freeAndSetNullSave(ptr);               \
	ptr = mallocArray(type, (count));

#define reallocStructSave(type, ptr) \
	freeAndSetNullSave(ptr);         \
	ptr = mallocStruct(type);

#define reallocStringSave(ptr, count) \
	freeAndSetNullSave(ptr);          \
	ptr = mallocString((count));

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
