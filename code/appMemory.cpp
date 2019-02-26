
struct MemoryBlock {
	int markerStack[10];
	int markerStackIndex;

	ExtendibleMemoryArray* pMemory;
	MemoryArray* tMemory;
	ExtendibleBucketMemory* dMemory;

	ExtendibleMemoryArray* pMemoryDebug;
	MemoryArray* tMemoryDebug;
	ExtendibleBucketMemory* dMemoryDebug;

	ExtendibleMemoryArray* currentPMemory;
	MemoryArray* currentTMemory;
	ExtendibleBucketMemory* currentDMemory;
};

void setMemory(bool debug = false) {
	if(!debug) {
		theMemory->currentPMemory = theMemory->pMemory;
		theMemory->currentTMemory = theMemory->tMemory;
		theMemory->currentDMemory = theMemory->dMemory;
	} else {
		theMemory->currentPMemory = theMemory->pMemoryDebug;
		theMemory->currentTMemory = theMemory->tMemoryDebug;
		theMemory->currentDMemory = theMemory->dMemoryDebug;
	}
}

#define getPStruct(type)       (type*)(getPMemory(sizeof(type)))
#define getPArray(type, count) (type*)(getPMemory(sizeof(type) * count))
#define getTStruct(type)       (type*)(getTMemory(sizeof(type)))
#define getTArray(type, count) (type*)(getTMemory(sizeof(type) * count))
#define getDStruct(type)       (type*)(getDMemory(sizeof(type)))
#define getDArray(type, count) (type*)(getDMemory(sizeof(type) * count))

inline void* getPMemory(int size) {
	return getExtendibleMemoryArray(size, theMemory->currentPMemory);
}

inline char* getPString(int size) { 
	char* s = getPArray(char, size + 1);
	s[0] = '\0';
	return s;
}

inline char* getPString(char* str, int size = -1) {
	char* newStr = getPArray(char, (size == -1 ? strLen(str) : size) + 1);
	strCpy(newStr, str, size);
	return newStr;
}

//

inline void* getTMemory(int size) {
	return getMemoryArray(size, theMemory->currentTMemory);
}

// Empty function for containers.
inline void freeTMemory(void* data) {}

inline void clearTMemory() {
	clearMemoryArray(theMemory->currentTMemory);
}

inline void pushMarkerTMemory()  {
    theMemory->markerStack[theMemory->markerStackIndex++] = theMemory->currentTMemory->index;
}

inline void popMarkerTMemory()  {
    int storedIndex = theMemory->markerStack[--theMemory->markerStackIndex];
    theMemory->currentTMemory->index = storedIndex;
}

inline char* getTString(int size) { 
	char* s = getTArray(char, size + 1);
	s[0] = '\0';
	return s;
}

inline char* getTString(char* str, int size = -1) {
	char* newStr = getTArray(char, (size == -1 ? strLen(str) : size) + 1);
	strCpy(newStr, str, size);
	return newStr;
}

inline char** getTStringArray(char** strings, int count) {
	char** array = getTArray(char*, count);
	for(int i = 0; i < count; i++) {
		array[i] = getTString(strLen(strings[i]));
	}

	return array;
}

//

inline void* getDMemory(int size) {
	return getExtendibleBucketMemory(theMemory->currentDMemory);
}

inline void freeDMemory(void* address) {
	freeExtendibleBucketMemory(address, theMemory->currentDMemory);
}

