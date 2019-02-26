
#define FRAME_BUFFER_SIZE 120

struct Timings {
	u64 cycles;
	int hits;
	u64 cyclesOverHits;
};

struct GraphSlot {
	char threadIndex;
	char timerIndex;
	char stackIndex;
	u64 cycles;
	uint size;
};

struct Profiler {
	ProfilerTimer* timer;
	// Stats for one frame.
	Timings timings[FRAME_BUFFER_SIZE][TIMER_INFO_SIZE];
	// Averages over multiple frames.
	Statistic statistics[FRAME_BUFFER_SIZE][TIMER_INFO_SIZE];

	int frameIndex;
	int currentFrameIndex;
	int lastFrameIndex;

	GraphSlot* slotBuffer;
	int slotBufferIndex;
	int slotBufferMax;
	int slotBufferCount;

	// Helpers for building slotBuffer and timings
	// over multiple frames.
	GraphSlot tempSlots[16][8]; // threads, stackIndex
	int tempSlotCount[16];

	bool noCollating;

	//

	void init(int frameSlotCount, int totalSlotBufferCount);
	void setPlay();
	void setPause();
	void update(int timerInfoCount, ThreadQueue* threadQueue);
};

void Profiler::init(int frameSlotCount, int totalSlotBufferCount) {
	timer = getPStruct(ProfilerTimer);
	timer->init(frameSlotCount);

	slotBufferMax = totalSlotBufferCount;
	slotBufferIndex = 0;
	slotBufferCount = 0;
	slotBuffer = getPArray(GraphSlot, slotBufferMax);
}

void Profiler::setPlay() {
	frameIndex = lastFrameIndex;
}

void Profiler::setPause() {
	lastFrameIndex = frameIndex;
	frameIndex = mod(frameIndex-1, arrayCount(timings));
}

void Profiler::update(int timerInfoCount, ThreadQueue* threadQueue) {
	timer->timerInfoCount = timerInfoCount;
	timer->update();

	currentFrameIndex = frameIndex;

	if(!noCollating) {
		frameIndex = (frameIndex + 1)%FRAME_BUFFER_SIZE;

		Timings* currentTimings = timings[currentFrameIndex];
		Statistic* currentStatistics = statistics[currentFrameIndex];

		zeroMemory(currentTimings, timer->timerInfoCount*sizeof(Timings));
		zeroMemory(currentStatistics, timer->timerInfoCount*sizeof(Statistic));

		// Collate timing buffer.
		// We take the timer slots that the app emits every frame and transform/extract 
		// information from them.

		if(timer->lastBufferIndex == 0) {
			for(int i = 0; i < arrayCount(tempSlots); i++) {
				tempSlotCount[i] = 0;
				memset(tempSlots + i, 0, sizeof(GraphSlot) * arrayCount(tempSlots[0]));
			}
		}

		for(int i = timer->lastBufferIndex; i < timer->bufferIndex; ++i) {
			TimerSlot* slot = timer->timerBuffer + i;
			
			int threadIndex = threadIdToIndex(threadQueue, slot->threadId);

			if(slot->type == TIMER_TYPE_BEGIN) {
				int index = tempSlotCount[threadIndex];

				GraphSlot graphSlot;
				graphSlot.threadIndex = threadIndex;
				graphSlot.timerIndex = slot->timerIndex;
				graphSlot.stackIndex = index;
				graphSlot.cycles = slot->cycles;
				tempSlots[threadIndex][index] = graphSlot;

				// tempSlotCount[threadIndex]++;

				tempSlotCount[threadIndex] = min(tempSlotCount[threadIndex]+1, (int)arrayCount(tempSlots[0]));

			} else {
				tempSlotCount[threadIndex] = max(tempSlotCount[threadIndex]-1, 0);
				// tempSlotCount[threadIndex]--;

				int index = tempSlotCount[threadIndex];
				if(index < 0) index = 0; // @Hack, to keep things running.

				tempSlots[threadIndex][index].size = slot->cycles - tempSlots[threadIndex][index].cycles;

				slotBuffer[slotBufferIndex] = tempSlots[threadIndex][index];
				slotBufferIndex = (slotBufferIndex+1)%slotBufferMax;
				slotBufferCount = clampMax(slotBufferCount + 1, slotBufferMax);

				Timings* timing = currentTimings + tempSlots[threadIndex][index].timerIndex;
				timing->cycles += tempSlots[threadIndex][index].size;
				timing->hits++;
			}
		}

		for(int i = 0; i < timer->timerInfoCount; i++) {
			Timings* t = currentTimings + i;
			t->cyclesOverHits = t->hits > 0 ? (u64)(t->cycles/t->hits) : 0; 
		}

		for(int timerIndex = 0; timerIndex < timer->timerInfoCount; timerIndex++) {
			Statistic* stat = currentStatistics + timerIndex;
			stat->begin();

			for(int i = 0; i < arrayCount(timings); i++) {
				Timings* t = &timings[i][timerIndex];
				if(t->hits == 0) continue;

				stat->update(t->cyclesOverHits);
			}

			stat->end();
			if(stat->count == 0) stat->avg = 0;
		}
	}

	timer->lastBufferIndex = timer->bufferIndex;

	if(threadQueueFinished(threadQueue)) {
		timer->bufferIndex = 0;
		timer->lastBufferIndex = 0;
	}

	assert(timer->bufferIndex < timer->bufferSize);
}
