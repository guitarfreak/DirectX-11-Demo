
enum {
	REC_STATE_INACTIVE = 0,
	REC_STATE_RECORDING,
	REC_STATE_PLAYING,
};

struct StateRecording {
	Input* inputArray;
	int capacity;
	int recordIndex;

	int state;

	char* snapshotMemory[8];
	int snapshotCount;
	int snapshotMemoryIndex;

	int playbackIndex;
	bool playbackPaused;
	bool justPaused;

	bool activeBreakPoint;
	int breakPointIndex;

	ExtendibleMemoryArray* memory;
	bool reloadMemory;

	//

	void init(int capacity, ExtendibleMemoryArray* memory);
	void update(Input* mainInput);
	void updateReloadMemory();
	void startRecording();
	void startPlayback();
	void playbackBreak();
	void playbackStep();
	void stop();
};

void StateRecording::init(int capacity, ExtendibleMemoryArray* memory) {
	*this = {};

	this->capacity = capacity;
	this->memory = memory;

	inputArray = getPArray(Input, capacity);
}

void StateRecording::update(Input* mainInput) {
	if(state == REC_STATE_RECORDING) {
		inputArray[recordIndex++] = *mainInput;

		if(recordIndex >= capacity) state = REC_STATE_INACTIVE;

	} else if(state == REC_STATE_PLAYING && !playbackPaused) {
		*mainInput = inputArray[playbackIndex];
		playbackIndex = (playbackIndex+1)%recordIndex;

		if(playbackIndex == 0) reloadMemory = true;

		if(activeBreakPoint && breakPointIndex == playbackIndex) {
			playbackPaused = true;
			activeBreakPoint = false;
			justPaused = true;
		}
	}

	// If pause, jump to end of main. (Before blitting.)
}

void StateRecording::updateReloadMemory() {
	if(reloadMemory) {
		theThreadQueue->complete();

		memory->index = snapshotCount-1;
		memory->arrays[memory->index].index = snapshotMemoryIndex;

		for(int i = 0; i < snapshotCount; i++) {
			memcpy(memory->arrays[i].data, snapshotMemory[i], memory->slotSize);
		}

		reloadMemory = false;
	}
}

void StateRecording::startRecording() {
	if(state != REC_STATE_INACTIVE || !theThreadQueue->finished()) return;

	snapshotCount = memory->index+1;
	snapshotMemoryIndex = memory->arrays[memory->index].index;
	for(int i = 0; i < snapshotCount; i++) {
		if(snapshotMemory[i] == 0) 
			snapshotMemory[i] = (char*)mallocX(memory->slotSize);

		memcpy(snapshotMemory[i], memory->arrays[i].data, memory->slotSize);
	}

	state = REC_STATE_RECORDING;
	recordIndex = 0;
}

void StateRecording::startPlayback() {
	if(state == REC_STATE_INACTIVE && !theThreadQueue->finished()) return;

	playbackIndex = 0;

	memory->index = snapshotCount-1;
	memory->arrays[memory->index].index = snapshotMemoryIndex;

	for(int i = 0; i < snapshotCount; i++) {
		memcpy(memory->arrays[i].data, snapshotMemory[i], memory->slotSize);
	}

	state = REC_STATE_PLAYING;
}

void StateRecording::playbackBreak() {
	if(state != REC_STATE_PLAYING) return;

	activeBreakPoint = true;
}

void StateRecording::playbackStep() {
	if(state != REC_STATE_PLAYING) return; 

	activeBreakPoint = true;

	playbackPaused = false;
	breakPointIndex = (playbackIndex + 1)%recordIndex;
}

void StateRecording::stop() { 
	state = REC_STATE_INACTIVE; 

	activeBreakPoint = false;
	playbackPaused = false;
	playbackIndex = 0;
};