
struct ThreadQueue {
	struct Job {
		void (*function)(void* data);
		void* data;
	};

	struct Settings {
		ThreadQueue* queue;
		HANDLE releaseSemaphore;
		bool setSemaphore;
	};

	Settings* settings;

	int* threadIds;
	int threadCount;

	HANDLE semaphore;

	Job* jobs;
	int jobCount;
	volatile uint completionGoal;
	volatile uint completionCount;
	volatile uint writeIndex;
	volatile uint readIndex;

	void init(int threadCount, int jobCount, bool setAffinity = false);
	void doNextJob();
	bool add(void (*function)(void*), void* data, int dataSize = 0, bool skipIfFull = false);
	void add(void (*function)(void*), void* data, int dataSize, int count);
	bool full();
	bool finished();
	int openJobs();
	void complete();
	int threadIdToIndex(int id);
	int getThreadId();
};

DWORD WINAPI threadProcess(LPVOID data) {
	ThreadQueue* queue = (ThreadQueue*)data;

	while(true) {
		int state = WaitForSingleObjectEx(queue->semaphore, INFINITE, FALSE);
		if(state == WAIT_OBJECT_0) queue->doNextJob();
	}

	return 0;
}

void ThreadQueue::init(int threadCount, int jobCount, bool setAffinity) {
	completionGoal = 0;
	completionCount = 0;
	writeIndex = 0;
	readIndex = 0;

	// DWORD_PTR mask;
	// DWORD_PTR sMask;
	// GetProcessAffinityMask(GetCurrentProcess(), &mask, &sMask);

	this->threadCount = threadCount;
	this->jobCount = jobCount;
	jobs = mallocArray(ThreadQueue::Job, jobCount);

	threadIds = mallocArray(int, threadCount);
	threadIds[0] = GetCurrentThreadId();

	semaphore = CreateSemaphoreExA(0, 0, jobCount, 0, 0, SEMAPHORE_ALL_ACCESS);

	HANDLE mainThread = GetCurrentThread();
	// SetThreadPriority(mainThread, 1);

	if(setAffinity) {
		__int64 threadMask = SetThreadAffinityMask(mainThread, 1);
	}

	settings = mallocArray(Settings, threadCount);

	for(int i = 0; i < threadCount; i++) {
		int id;
		HANDLE thread = CreateThread(0, 0, threadProcess, (void*)(this), 0, (LPDWORD)&id);
		threadIds[i+1] = id;

		if(setAffinity) {
			SetThreadAffinityMask(thread, 1<<(i+1));
		}

		CloseHandle(thread);
	}
}

void ThreadQueue::doNextJob() {
	volatile uint currentReadIndex;
	while(true) {
		currentReadIndex = readIndex;
		volatile uint newReadIndex = (currentReadIndex + 1) % jobCount;

		if(currentReadIndex != writeIndex) {
			LONG oldValue = InterlockedCompareExchange((LONG volatile*)&readIndex, newReadIndex, currentReadIndex);
			if(oldValue == currentReadIndex) break;
		}
	}

	ThreadQueue::Job job = jobs[currentReadIndex];
	job.function(job.data);

	InterlockedIncrement((LONG volatile*)&completionCount);
}

bool ThreadQueue::add(void (*function)(void*), void* data, int dataSize, bool skipIfFull) {
	int newWriteIndex = (writeIndex + 1) % jobCount;

	ThreadQueue::Job* job = jobs + writeIndex;
	job->function = function;
	job->data = data;

	InterlockedIncrement((long*)&completionGoal);

	_ReadWriteBarrier();
	InterlockedExchange((long*)&writeIndex, newWriteIndex);
	int result = ReleaseSemaphore(semaphore, 1, 0);
	myAssert(result);

	return true;
}

void ThreadQueue::add(void (*function)(void*), void* data, int dataSize, int count) {
	for(int i = 0; i < count; i++) {
		ThreadQueue::Job* job = jobs + (writeIndex + i) % jobCount;
		job->function = function;
		job->data = ((char*)data) + dataSize*i;
	}

	InterlockedExchange((long*)&completionGoal, completionGoal + count);

	_ReadWriteBarrier();
	int newWriteIndex = (writeIndex + count) % jobCount;

	InterlockedExchange((long*)&writeIndex, newWriteIndex);
	int result = ReleaseSemaphore(semaphore, count, 0);
	myAssert(result);
}

bool ThreadQueue::full() {
	int newWriteIndex = (writeIndex + 1) % jobCount;
	bool result = newWriteIndex == readIndex;

	return result;
}

bool ThreadQueue::finished() {
	return completionCount == completionGoal;
}

int ThreadQueue::openJobs() {
	return completionGoal - completionCount;
}

void ThreadQueue::complete() {
	while(true) {
		short state = WaitForSingleObjectEx(semaphore, 0, FALSE);
		if(state == WAIT_OBJECT_0) doNextJob();
		else break;
	}

	while(!finished());

	InterlockedExchange((long*)&completionGoal,  0);
	InterlockedExchange((long*)&completionCount, 0);
}

int ThreadQueue::threadIdToIndex(int id) {
	for(int i = 0; i < threadCount+1; i++) {
		if(id == threadIds[i]) return i;
	}

	return -1;
}

int ThreadQueue::getThreadId() {
	return threadIdToIndex(GetCurrentThreadId());
}

//

struct ThreadHeader {
	int index;
	int count;
	void* data;
};

void splitThreadTask(int count, void* data, void (*function)(void*), int threadCount = 0) {
	if(!count) return;

	if(!threadCount) threadCount = theThreadQueue->threadCount+1;
	DArray<ThreadHeader> threadData = dArray<ThreadHeader>(threadCount, getTMemory);

	Vec2i* ranges = arrayDivideRanges(count, threadCount);
	if(ranges) {
		for(int i = 0; i < threadCount; i++) {
			threadData.push({ranges[i].x, ranges[i].y, data});
		}

		theThreadQueue->add(function, threadData.data, sizeof(ThreadHeader), threadCount);
		theThreadQueue->complete();
	}
}
