
#include <assert.h>

struct MemoryArray {
	bool initialized;

	char * data;
	int index;
	int size;
};

MemoryArray* theMemoryArray;

void initMemoryArray(MemoryArray * memory, int slotSize, void* baseAddress = 0) {
    if(baseAddress) {
	    memory->data = (char*)VirtualAlloc(baseAddress, slotSize, MEM_COMMIT, PAGE_READWRITE);
	    // memory->data = (char*)malloc(slotSize);
	    int errorCode = GetLastError();
    } else memory->data = (char*)malloc(slotSize);

    assert(memory->data);

    memory->index = 0;
    memory->size = slotSize;
    memory->initialized = true;
}

void* getMemoryArray(int size, MemoryArray * memory = 0) {
    if(!memory) memory = theMemoryArray;
    assert(memory->index + size <= memory->size);

    void * location = memory->data + memory->index;
    memory->index += size;

    return location;
}

void freeMemoryArray(int size, MemoryArray * memory = 0) {
    if(!memory) memory = theMemoryArray;
    assert(memory->size >= memory->index);

    memory->index -= size;
}

void clearMemoryArray(MemoryArray* memory = 0) {
    if(!memory) memory = theMemoryArray;
	memory->index = 0;
}

void* getBaseMemoryArray(MemoryArray* ma) {
	void* base = ma->data;
	return base;
}

//

struct ExtendibleMemoryArray {
	void* startAddress;
	int slotSize;
	int allocGranularity;
	MemoryArray arrays[32];
	int index;
};

ExtendibleMemoryArray* theExtendibleMemoryArray;

void initExtendibleMemoryArray(ExtendibleMemoryArray* memory, int slotSize, int allocGranularity, void* baseAddress = 0) {
	memory->startAddress = baseAddress;
	memory->index = 0;
	memory->allocGranularity = allocGranularity;

	// roundModUp.
	// memory->slotSize = roundModUp(slotSize, memory->allocGranularity);
	memory->slotSize = ceil(slotSize/(double)memory->allocGranularity)*memory->allocGranularity;

	initMemoryArray(memory->arrays, memory->slotSize, memory->startAddress);
}

void* getExtendibleMemoryArray(int size, ExtendibleMemoryArray* memory = 0) {
	if(!memory) memory = theExtendibleMemoryArray;
	assert(size <= memory->slotSize);

	MemoryArray* currentArray = memory->arrays + memory->index;
	if(currentArray->index + size > currentArray->size) {
		memory->index++;
		assert(memory->index < arrayCount(memory->arrays));
		i64 baseOffset = (i64)memory->index*(i64)memory->slotSize;

		MemoryArray* mArray = memory->arrays + memory->index;
		if(!mArray->initialized)
			initMemoryArray(&memory->arrays[memory->index], memory->slotSize, (char*)memory->startAddress + baseOffset);
		else
			mArray->index = 0;

		currentArray = memory->arrays + memory->index;
	}

	void* location = getMemoryArray(size, currentArray);
	return location;
}

void* getBaseExtendibleMemoryArray(ExtendibleMemoryArray* ema) {
	void* base = ema->arrays[0].data;
	return base;
}

//

struct BucketMemory {
	int pageSize;
	int count;
	int useCount;
	char* data;
	bool* used;
};

BucketMemory* theBucketMemory;

// slotSize has to be dividable by pageSize
void initBucketMemory(BucketMemory* memory, int pageSize, int slotSize, void* baseAddress = 0) {
	memory->pageSize = pageSize;
	memory->count = slotSize / pageSize;
	memory->useCount = 0;

	if(baseAddress) {
		memory->data = (char*)VirtualAlloc(baseAddress, slotSize + memory->count, MEM_COMMIT, PAGE_READWRITE);
		// memory->data = (char*)malloc(slotSize + memory->count);
	}
	else memory->data = (char*)malloc(slotSize + memory->count);
	assert(memory->data);

	memory->used = (bool*)memory->data + slotSize;
	memset(memory->used, 0, memory->count);
}

void deleteBucketMemory(BucketMemory* memory) {
	VirtualFree(memory->data, 0, MEM_RELEASE);
	// free(memory->data);
}

void* getBucketMemory(BucketMemory* memory = 0) {
	if(memory == 0) memory = theBucketMemory;
	assert(memory);

	if(memory->useCount == memory->count) return 0;

	char* address = 0;
	int index;
	for(int i = 0; i < memory->count; i++) {
		if(memory->used[i] == 0) {
			address = memory->data + i*memory->pageSize;
			index = i;
			break;
		}
	}

	assert(address);

	if(address) {
		memory->used[index] = true;

		memory->useCount++;
		assert(memory->useCount <= memory->count);

		return address;
	}

	return 0;
}

void freeBucketMemory(void* address, BucketMemory* memory = 0) {
	if(memory == 0) memory = theBucketMemory;
	assert(memory);

	memory->useCount--;
	assert(memory->useCount >= 0);

	int dataOffset = ((char*)address - memory->data) / memory->pageSize;
	memory->used[dataOffset] = false;
}

//

struct ExtendibleBucketMemory {
	void* startAddress;
	int slotSize;
	int fullSize;
	int allocGranularity;
	BucketMemory arrays[32];
	bool allocated[32];

	int pageSize;
};

ExtendibleBucketMemory* globalExtendibleBucketMemory;

void initExtendibleBucketMemory(ExtendibleBucketMemory* memory, int pageSize, int slotSize, int allocGranularity, void* baseAddress = 0) {
	memory->startAddress = baseAddress;
	memory->allocGranularity = allocGranularity;
	memory->slotSize = slotSize;

	// roundModUp.
	// memory->fullSize = roundModUp(slotSize + (slotSize / pageSize), memory->allocGranularity);
	int v = slotSize + (slotSize / pageSize);
	memory->fullSize = ceil(v/(double)memory->allocGranularity)*memory->allocGranularity;

	memory->pageSize = pageSize;

	memset(memory->allocated, 0, arrayCount(memory->arrays));
}

void* getExtendibleBucketMemory(ExtendibleBucketMemory* memory = 0) {
	if(!memory) memory = globalExtendibleBucketMemory;

	// check all allocated arrays for a free slot
	BucketMemory* availableBucket = 0;
	for(int i = 0; i < arrayCount(memory->arrays); i++) {
		if(memory->allocated[i] && (memory->arrays[i].useCount < memory->arrays[i].count)) {
			availableBucket = memory->arrays + i;
			break;
		}
	}

	// allocate array
	if(!availableBucket) {
		// get first array that is not allocated
		int arrayIndex = -1;
		for(int i = 0; i < arrayCount(memory->allocated); i++) {
			if(!memory->allocated[i]) {
				availableBucket = memory->arrays + i;
				arrayIndex = i;
				break;
			}
		}

		assert(availableBucket);

		int slotSize = memory->slotSize;
		i64 baseOffset = arrayIndex * memory->fullSize;
		initBucketMemory(availableBucket, memory->pageSize, memory->slotSize, (char*)memory->startAddress + baseOffset);

		memory->allocated[arrayIndex] = true;
	}

	void* location = getBucketMemory(availableBucket);
	return location;
}

void freeExtendibleBucketMemory(void* address, ExtendibleBucketMemory* memory = 0) {
	if(!memory) memory = globalExtendibleBucketMemory;

	// calculate array index with address
	int arrayIndex = ((char*)address - (char*)memory->startAddress) / memory->fullSize;
	BucketMemory* bMemory = memory->arrays + arrayIndex;
	freeBucketMemory(address, bMemory);

	if(bMemory->useCount == 0) {
		deleteBucketMemory(bMemory);
		memory->allocated[arrayIndex] = false;
	}
}

//

struct AppMemory {
	MemoryArray memoryArrays[4];
	int memoryArrayCount;
	
	ExtendibleMemoryArray extendibleMemoryArrays[4];
	int extendibleMemoryArrayCount;

	BucketMemory bucketMemories[4];
	int bucketMemoryCount;

	ExtendibleBucketMemory extendibleBucketMemories[4];
	int extendibleBucketMemoryCount;
};
