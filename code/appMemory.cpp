
struct MemoryBlock {
	bool debugMode;
	
	int markerStack[10];
	int markerStackIndex;

	ExtendibleMemoryArray* pMemory;
	MemoryArray* tMemory;
	ExtendibleBucketMemory* dMemory;

	MemoryArray* pMemoryDebug;
	MemoryArray* tMemoryDebug;
};

//

#define getPStruct(type)       (type*)(getPMemoryMain(sizeof(type)))
#define getPArray(type, count) (type*)(getPMemoryMain(sizeof(type) * count))
#define getPString(count)      (char*)(getPMemoryMain(count))
#define getTStruct(type)       (type*)(getTMemoryMain(sizeof(type)))
#define getTArray(type, count) (type*)(getTMemoryMain(sizeof(type) * count))
#define getTString(size)       (char*)(getTMemoryMain(size))
#define getDStruct(type)       (type*)(getDMemoryMain(sizeof(type)))
#define getDArray(type, count) (type*)(getDMemoryMain(sizeof(type) * count))5

void* getPMemoryMain(int size, MemoryBlock* memory) {
	return getExtendibleMemoryArray(size, memory->pMemory);
}
void* getPMemoryMain(int size) {
	return getExtendibleMemoryArray(size, theMemory->pMemory);
}

void* getTMemoryMain(int size, MemoryBlock* memory) {
	return getMemoryArray(size, memory->tMemory);
}
void* getTMemoryMain(int size) {
	return getMemoryArray(size, theMemory->tMemory);
}

void clearTMemory(MemoryBlock* memory = 0) {
	clearMemoryArray(memory ? memory->tMemory : theMemory->tMemory);
}

void* getDMemoryMain(int size, MemoryBlock* memory = 0) {
	return getExtendibleBucketMemory(memory ? memory->dMemory : theMemory->dMemory);
}
void freeDMemoryMain(void* address, MemoryBlock* memory = 0) {
	freeExtendibleBucketMemory(address, memory ? memory->dMemory : theMemory->dMemory);
}

void pushMarkerTMemoryMain(MemoryBlock* memory = 0)  {
    if(!memory) memory = theMemory;
    memory->markerStack[memory->markerStackIndex++] = memory->tMemory->index;
}

void popMarkerTMemoryMain(MemoryBlock* memory = 0)  {
    if(!memory) memory = theMemory;
    int storedIndex = memory->markerStack[--memory->markerStackIndex];
    memory->tMemory->index = storedIndex;
}



inline char* getPStringCpy(char* str, int size = -1) {
	char* newStr = getPString((size == -1 ? strLen(str) : size) + 1);
	strCpy(newStr, str, size);
	return newStr;
}

inline char* getTStringCpy(char* str, int size = -1) {
	char* newStr = getTString((size == -1 ? strLen(str) : size) + 1);
	strCpy(newStr, str, size);
	return newStr;
}

inline char* getPStringClr(int size) { 
	char* s = getPString(size);
	s[0] = '\0';
	return s;
}

inline char* getTStringClr(int size) { 
	char* s = getTString(size);
	s[0] = '\0';
	return s;
}

char** getTStringArray(char** strings, int count) {
	char** array = getTArray(char*, count);
	for(int i = 0; i < count; i++) {
		array[i] = getTString(strLen(strings[i]));
	}

	return array;
}

//

#define getPStructDebug(type)       (type*)(getPMemoryDebug(sizeof(type)))
#define getPArrayDebug(type, count) (type*)(getPMemoryDebug(sizeof(type) * count))
#define getPStringDebug(count)      (char*)(getPMemoryDebug(count))
#define getTStructDebug(type)       (type*)(getTMemoryDebug(sizeof(type)))
#define getTArrayDebug(type, count) (type*)(getTMemoryDebug(sizeof(type) * count))
#define getTStringDebug(size)       (char*)(getTMemoryDebug(size))

void* getPMemoryDebug(int size, MemoryBlock* memory) {
	return getMemoryArray(size, memory->pMemoryDebug);
}
void* getPMemoryDebug(int size) {
	return getMemoryArray(size, theMemory->pMemoryDebug);
}

void* getTMemoryDebug(int size, MemoryBlock* memory) {
	return getMemoryArray(size, memory->tMemoryDebug);
}
void* getTMemoryDebug(int size) {
	return getMemoryArray(size, theMemory->tMemoryDebug);
}

void clearTMemoryDebug(MemoryBlock* memory = 0) {
	clearMemoryArray(memory ? memory->tMemoryDebug : theMemory->tMemoryDebug);
}

void pushMarkerTMemoryDebug(MemoryBlock* memory = 0)  {
    if(!memory) memory = theMemory;
    memory->markerStack[memory->markerStackIndex++] = memory->tMemoryDebug->index;
}

void popMarkerTMemoryDebug(MemoryBlock* memory = 0)  {
    if(!memory) memory = theMemory;
    int storedIndex = memory->markerStack[--memory->markerStackIndex];
    memory->tMemoryDebug->index = storedIndex;
}

//

inline char* getPStringCpyDebug(char* str, int size = -1) {
	char* newStr = getPString((size == -1 ? strLen(str) : size) + 1);
	strCpy(newStr, str, size);
	return newStr;
}

inline char* getTStringCpyDebug(char* str, int size = -1) {
	char* newStr = getTString((size == -1 ? strLen(str) : size) + 1);
	strCpy(newStr, str, size);
	return newStr;
}

//

// Choose right function according to memory mode that's set globally.

#define getPStructX(type)       (type*)(getPMemory(sizeof(type)))
#define getPArrayX(type, count) (type*)(getPMemory(sizeof(type) * count))
#define getPStringX(count)      (char*)(getPMemory(count))
#define getTStructX(type)       (type*)(getTMemory(sizeof(type)))
#define getTArrayX(type, count) (type*)(getTMemory(sizeof(type) * count))
#define getTStringX(size)       (char*)(getTMemory(size))

void* getPMemory(int size, MemoryBlock* memory = 0) {
	if(memory == 0) memory = theMemory;

	if(!memory->debugMode) return getPMemoryMain(size, memory);
	else return getPMemoryDebug(size, memory);
}

void* getTMemory(int size, MemoryBlock* memory = 0) {
	if(memory == 0) memory = theMemory;

	if(!memory->debugMode) return getTMemoryMain(size, memory);
	else return getTMemoryDebug(size, memory);
}
