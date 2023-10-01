
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
	SampleData sampler;

	Timings      timings[FRAME_BUFFER_SIZE][TIMER_INFO_SIZE]; // Stats for one frame.
	Statistic statistics[FRAME_BUFFER_SIZE][TIMER_INFO_SIZE]; // Averages over multiple frames.

	int frameIndex;
	int currentFrameIndex;
	int lastFrameIndex;

	GraphSlot* slotBuffer;
	int slotBufferIndex;
	int slotBufferMax;
	int slotBufferCount;

	int savedFramesCount;

	// Helpers for building slotBuffer and timings over multiple frames.
	DArray<DArray<GraphSlot>> slotStacks; // [threads][stack]

	bool noCollating;

	//

	void init(int startFrameSlotCount, int startSlotBufferCount, int savedFramesCount);
	void setPlay();
	void setPause();
	void update(int timerInfoCount, ThreadQueue* threadQueue);
};

void Profiler::init(int startSampleBufferSize, int startGraphSlotCount, int savedFramesCount) {
	sampler.init(startSampleBufferSize);

	slotBufferMax = startGraphSlotCount;
	slotBufferIndex = 0;
	slotBufferCount = 0;
	slotBuffer = mallocArray(GraphSlot, slotBufferMax);

	this->savedFramesCount = savedFramesCount;

	int maxStackSize = 50;
	int threadCount = theThreadQueue->threadCount+1;
	slotStacks.initResize(threadCount, getPMemory, true);
	for(auto& it : slotStacks) it.init(maxStackSize, getPMemory);
}

void Profiler::setPlay() {
	frameIndex = lastFrameIndex;
}

void Profiler::setPause() {
	lastFrameIndex = frameIndex;
	frameIndex = mod(frameIndex-1, arrayCount(timings));
}

void Profiler::update(int timerInfoCount, ThreadQueue* threadQueue) {
	myAssert(timerInfoCount <= sampler.infos.reserved);
	sampler.infos.count = timerInfoCount;
	sampler.update();

	currentFrameIndex = frameIndex;

	// if(!noCollating && !sampler.bufferOverrunCount) {
	if(!noCollating) {
		frameIndex = (frameIndex + 1)%FRAME_BUFFER_SIZE;

		Timings* currentTimings = timings[currentFrameIndex];
		Statistic* currentStatistics = statistics[currentFrameIndex];

		zeroMemory(currentTimings, sampler.infos.count*sizeof(Timings));
		zeroMemory(currentStatistics, sampler.infos.count*sizeof(Statistic));

		// Collate timing buffer.
		// We take the timer slots that the app emits every frame and transform/extract 
		// information from them.

		if(sampler.lastBufferIndex == 0) {
			for(auto& it : slotStacks) {
				it.zeroMemoryReserved();
				it.count = 0;
			}
		}

		for(int i = sampler.lastBufferIndex; i < sampler.bufferIndex; ++i) {
			Sample* sample = sampler.buffer + i;
			
			int threadIndex = threadQueue->threadIdToIndex(sample->threadId);
			DArray<GraphSlot>& stack = slotStacks[threadIndex];

			if(sample->type == SAMPLE_TYPE_BEGIN) {
				int index = stack.count;
				stack.count = min(stack.count+1, stack.reserved-1);
				stack[index] = {(char)threadIndex, (char)sample->timerIndex, (char)index, sample->cycles};

			} else {
				stack.count = max(stack.count-1, 0);
				int index = stack.count;

				GraphSlot* slot = &stack[index];
				slot->size = sample->cycles - slot->cycles;

				slotBuffer[slotBufferIndex] = *slot;
				slotBufferIndex = (slotBufferIndex+1)%slotBufferMax;
				slotBufferCount = clampMax(slotBufferCount + 1, slotBufferMax);

				Timings* timing = currentTimings + slot->timerIndex;
				timing->cycles += slot->size;
				timing->hits++;
			}
		}

		// Count swaps in slot ring buffer and resize if there are not enough enough frames.
		if(slotBufferCount == slotBufferMax && sampler.swapInfoIndex != -1) {
			int swapCount = 0;
			for(int i = 0; i < slotBufferMax; ++i) {
				if(slotBuffer[i].timerIndex == sampler.swapInfoIndex) swapCount++;
			}

			// Resize ring buffer:
			if(swapCount < savedFramesCount+1) {
				int newSize = swapCount ? slotBufferMax + ((slotBufferMax / swapCount) * ((savedFramesCount+1) - swapCount)) * 1.25f : slotBufferMax * 2;

				resizeRingBuffer(GraphSlot, &slotBuffer, &slotBufferMax, &slotBufferIndex, newSize);
			}
		}

		for(int i = 0; i < sampler.infos.count; i++) {
			Timings* t = currentTimings + i;
			t->cyclesOverHits = t->hits > 0 ? (u64)(t->cycles/t->hits) : 0; 
		}

		for(int timerIndex = 0; timerIndex < sampler.infos.count; timerIndex++) {
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

	sampler.lastBufferIndex = sampler.bufferIndex;

	if(threadQueue->finished()) {
		sampler.bufferIndex = 0;
		sampler.lastBufferIndex = 0;

		// Grow buffer on overrun.
		if(sampler.bufferOverrunCount) {
			mallocArrayResize(Sample, &sampler.buffer, &sampler.bufferSize, (sampler.bufferSize + sampler.bufferOverrunCount) * 1.25f);
			sampler.bufferOverrunCount = 0;
		}
	}

	assert(sampler.bufferIndex < sampler.bufferSize);
}
