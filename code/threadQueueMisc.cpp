
struct ThreadHeader {
	void* data;

	// These are for when we want to spread the thread tasks evenly.
	// Usefull when we expect every task to roughly take the same time. 
	int index;
	int count;

	// These are for when we expect the tasks to differ strongly in execution time.
	int arrayCount;

	volatile int* stackIndex;
	bool batchRunning;
	int currentBatchIndex;
	int currentBatchEndIndex;

	inline bool incStackIndex(int* _index) {
		volatile int addedIndex = InterlockedIncrement((LONG volatile*)stackIndex);
		*_index = addedIndex - 1;
		return !((*_index) >= arrayCount);
	}

	inline bool addStackIndex(int* startIndex, int* endIndex, int amount) {
		volatile int addedIndex = InterlockedAdd((LONG volatile*)stackIndex, amount);
		*startIndex = addedIndex - amount;
		*endIndex = min(addedIndex, arrayCount);

		return !((*startIndex) >= arrayCount);
	}

	inline bool addStackIndex(int* _index, int amount) {
		if(!batchRunning) {
			volatile int addedIndex = InterlockedAdd((LONG volatile*)stackIndex, amount);
			int startIndex = addedIndex - amount;

			if(startIndex >= arrayCount) return false;

			batchRunning = true;
			currentBatchIndex = startIndex;
			currentBatchEndIndex = min(addedIndex, arrayCount) - 1;
		}

		*_index = currentBatchIndex;

		if(currentBatchIndex == currentBatchEndIndex) {
			batchRunning = false;
			currentBatchIndex = 0;
		}

		currentBatchIndex++;

		return true;
	}
};

void splitThreadTask(int count, void* data, void (*function)(void*), int threadCount = 0, Vec2i** _ranges = 0) {
	if(!count) return;

	volatile int stackIndex = 0;

	// Disable threading for debuggin purposes.
	if(threadCount == -1) {
		ThreadHeader th = {data, 0, count, count, &stackIndex};
		function(&th);

		return;
	}

	if(!threadCount) threadCount = theThreadQueue->threadCount+1;
	DArray<ThreadHeader> threadData = dArray<ThreadHeader>(threadCount, getTMemory);

	Vec2i* ranges = arrayDivideRanges(count, threadCount);
	if(ranges) {
		for(int i = 0; i < threadCount; i++) {
			ThreadHeader th = {data, ranges[i].x, ranges[i].y, count, &stackIndex};
			threadData.push(th);
		}

		theThreadQueue->add(function, threadData.data, sizeof(ThreadHeader), threadCount);
		theThreadQueue->complete();
	}

	if(_ranges) *_ranges = ranges;
}

