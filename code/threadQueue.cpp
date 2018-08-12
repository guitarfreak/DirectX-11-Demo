
struct ThreadJob {
    void (*function)(void* data);
    void* data;
    int dataType;
    char dataBuffer[150];
};

struct ThreadQueue {
    volatile uint completionGoal;
    volatile uint completionCount;
    volatile uint writeIndex;
    volatile uint readIndex;
    HANDLE semaphore;

    int threadIds[16];
    int threadCount;

    ThreadJob* jobs;
    int jobCount;
};

int getThreadQueueId(ThreadQueue* queue) {
	int currentId = GetCurrentThreadId();

	int id = -1;
	for(int i = 0; i < arrayCount(queue->threadIds); i++) {
		if(queue->threadIds[i] == currentId) {
			id = i;
			break;
		}
	}

	myAssert(id != -1);
	return id;
}

bool doNextThreadJob(ThreadQueue* queue) {
    bool shouldSleep = false;

    volatile uint currentReadIndex = queue->readIndex;
    volatile uint newReadIndex = (currentReadIndex + 1) % queue->jobCount;

    if(currentReadIndex != queue->writeIndex) {
        LONG oldValue = InterlockedCompareExchange((LONG volatile*)&queue->readIndex, newReadIndex, currentReadIndex);
        // if(newReadIndex == queue->readIndex) {
        if(oldValue == currentReadIndex) { // Holy Shit.
            ThreadJob job = queue->jobs[currentReadIndex];
            if(job.dataType == 0) job.function(job.data);
            else job.function(job.dataBuffer);
            InterlockedIncrement((LONG volatile*)&queue->completionCount);
            // printf("COUNT: %i \n", queue->completionCount);
        }
    } else {
        shouldSleep = true;
    }

    return shouldSleep;
}

DWORD WINAPI threadProcess(LPVOID data) {
    ThreadQueue* queue = (ThreadQueue*)data;
    for(;;) {
        if(doNextThreadJob(queue)) {
            WaitForSingleObjectEx(queue->semaphore, INFINITE, FALSE);
        }
    }

    return 0;
}

void threadInit(ThreadQueue* queue, int numOfThreads, int jobCount) {
    queue->completionGoal = 0;
    queue->completionCount = 0;
    queue->writeIndex = 0;
    queue->readIndex = 0;
    // queue->semaphore = CreateSemaphore(0, 0, 255, "Semaphore");
    queue->semaphore = CreateSemaphoreEx(0, 0, 255, "Semaphore", 0, SEMAPHORE_ALL_ACCESS);

    queue->threadCount = numOfThreads;

    queue->jobCount = jobCount;
    queue->jobs = mallocArray(ThreadJob, jobCount);

    int id = GetCurrentThreadId();
    queue->threadIds[0] = id;

    HANDLE handle = GetCurrentThread();
    // SetThreadPriority(handle, 2);
    SetThreadPriority(handle, 1);

    for(int i = 0; i < numOfThreads; i++) {
        HANDLE thread = CreateThread(0, 0, threadProcess, (void*)(queue), 0, 0);

        // SetThreadPriority(thread, -2);
        SetThreadPriority(thread, -1);

        int id = GetThreadId(thread);
        queue->threadIds[i+1] = id;

        if(!thread) printf("Could not create thread\n");
        CloseHandle(thread);
    }
}

bool threadQueueAdd(ThreadQueue* queue, void (*function)(void*), void* data, int dataSize = 0, bool skipIfFull = false) {
    int newWriteIndex = (queue->writeIndex + 1) % queue->jobCount;

	// if(skipIfFull) {
	// 	if(newWriteIndex == queue->readIndex) return false;
	// } 
	// else {
	// 	myAssert(newWriteIndex != queue->readIndex);
	// }

    ThreadJob* job = queue->jobs + queue->writeIndex;
    job->function = function;
    if(dataSize == 0) {
    	job->dataType = 0;
	    job->data = data;
    } else {
    	job->dataType = 1;
	    memcpy(job->dataBuffer, data, dataSize);
    }

    InterlockedIncrement(&queue->completionGoal);
    // printf("GOAL: %i \n", queue->completionCount);

    _ReadWriteBarrier(); // doesn't work on 32 bit?
    InterlockedExchange(&queue->writeIndex, newWriteIndex);

    ReleaseSemaphore(queue->semaphore, 1, 0);

    return true;
}

bool threadQueueFull(ThreadQueue* queue) {
	int newWriteIndex = (queue->writeIndex + 1) % queue->jobCount;
    bool result = newWriteIndex == queue->readIndex;

    return result;
}

bool threadQueueFinished(ThreadQueue* queue) {
	return queue->completionCount == queue->completionGoal;
}

void threadQueueComplete(ThreadQueue* queue) {
    while(threadQueueFinished(queue) == false) {
        doNextThreadJob(queue);
    }

    // if(queue->readIndex != queue->writeIndex) 
    // 	printf("Threadqueue copmletion error.\n");

    InterlockedExchange(&queue->completionGoal, 0);
    InterlockedExchange(&queue->completionCount, 0);
}

int threadQueueOpenJobs(ThreadQueue* queue) {
	int result = queue->completionGoal - queue->completionCount;
	return result;
}

int threadIdToIndex(ThreadQueue* queue, int id) {
	int threadCount = queue->threadCount;
	for(int i = 0; i < threadCount; i++) {
		if(id == queue->threadIds[i]) return i;
	}

	return -1;
}