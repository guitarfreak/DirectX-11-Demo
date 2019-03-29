
struct MemoryArray {
	bool initialized;

	char* data;
	unsigned int index;
	int size;
};

struct ExtendibleMemoryArray {
	void* startAddress;
	int slotSize;
	int allocGranularity;
	MemoryArray arrays[32];
	int index;
};

struct BucketMemory {
	int pageSize;
	int count;
	int useCount;
	char* data;
	bool* used;
};

struct ExtendibleBucketMemory {
	void* startAddress;
	int slotSize;
	int fullSize;
	int allocGranularity;
	BucketMemory arrays[32];
	bool allocated[32];

	int pageSize;
};

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
