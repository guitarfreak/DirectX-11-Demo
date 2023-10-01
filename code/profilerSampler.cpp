#pragma once 

#define TIMER_INFO_SIZE 100
#define SWAP_REGION_NAME "Swap"

struct SampleZoneInfo {
	bool initialised;
	bool stringsSaved;

	char* file;
	char* function;
	char* name;
	int line;
	Vec3 color;

	void init(char* file, char* function, int line, char* name);
};

void SampleZoneInfo::init(char* file, char* function, int line, char* name) {
	this->file = file;
	this->function = function;
	this->line = line;
	this->name = name;
	this->initialised = true;
}

//

enum SampleType {
	SAMPLE_TYPE_BEGIN,
	SAMPLE_TYPE_END,
};

#pragma pack(push,1)
struct Sample {
	char type;
	uint threadId;
	char timerIndex;

	u64 cycles;
};
#pragma pack(pop)

struct SampleData {
	DArray<SampleZoneInfo> infos;
	int swapInfoIndex;

	Sample* buffer;
	int bufferIndex;
	int bufferSize;
	uint bufferOverrunCount;
	
	int lastBufferIndex;

	void init(int bufferSize);
	void setInfo(int infoIndex, char* file, char* function, int line, char* name = "");
	void addSlot(int infoIndex, int type);
	void addSlotAndInfo(int infoIndex, int type, char* file, char* function, int line, char* name = "");
	void update();
};

void SampleData::init(int bufferSize) {
	infos = dArray<SampleZoneInfo>(TIMER_INFO_SIZE, getPMemory);

	swapInfoIndex = -1;

	this->bufferSize = bufferSize;
	buffer = (Sample*)malloc(sizeof(Sample) * bufferSize);
}

void SampleData::addSlot(int infoIndex, int type) {
	// -10 so we have some room to patch things up at the end
	// if we run out of space.
	// Also we if we don't have enough space here so we can grow the 
	// buffer at a different place when we know all the threads are done.
	if(bufferIndex > bufferSize-10) {
		InterlockedIncrement(&bufferOverrunCount);
		return; 
	}

	int nextSlotIndex = InterlockedIncrement((LONG*)(&bufferIndex));
	Sample* slot = buffer + nextSlotIndex-1;

	_mm_lfence();
	slot->cycles = __rdtsc();
	_mm_lfence();

	slot->type = type;
	slot->threadId = GetCurrentThreadId();
	slot->timerIndex = infoIndex;
}

void SampleData::addSlotAndInfo(int infoIndex, int type, char* file, char* function, int line, char* name) {
	if(!infos[infoIndex].initialised) infos[infoIndex].init(file, function, line, name);

	addSlot(infoIndex, type);
}

void SampleData::update() {
	int infoCount = infos.count;
	for(int i = 0; i < infoCount; i++) {
		SampleZoneInfo* info = infos + i;

		// Set colors.
		float ss = i%(infoCount/2) / ((float)infoCount/2);
		float h = i < infoCount/2 ? 0.1f : -0.1f;

		float hf = 360*ss;
		float li = 0.6f+h;
		{
			float brightMod[] = {0.299, 0.587, 0.114};
			     if(hf >= 0   && hf < 120) li *= 1-mapRange(hf, 0.0f,   120.0f, brightMod[0], brightMod[1]);
			else if(hf >= 120 && hf < 270) li *= 1-mapRange(hf, 120.0f, 270.0f, brightMod[1], brightMod[2]);
			else if(hf >= 270 && hf < 360) li *= 1-mapRange(hf, 270.0f, 360.0f, brightMod[2], brightMod[0]);
		}
		info->color = hslToRgb(hf, 0.6f, li);

		if(!info->initialised || info->stringsSaved) continue;

		info->file = getPString(info->file);
		info->function = getPString(info->function);
		int lambdaPos = strFindRight(info->function, "lambda_");
		if(lambdaPos != -1) {
			info->function[lambdaPos-1] = '>';
			info->function[lambdaPos] = '\0';
		}
		info->name = getPString(info->name);

		if(strCompare(info->name, SWAP_REGION_NAME)) swapInfoIndex = i;
		
		info->stringsSaved = true;
	}
}

//

extern SampleData* theSampler;
void addSample(int infoIndex, int type) { return theSampler->addSlot(infoIndex, type); }
void addSampleAndInfo(int index, int type, char* file, char* function, int line, char* name = "") {
	return theSampler->addSlotAndInfo(index, type, file, function, line, name);
}

struct TimerBlock {
	int infoIndex;

	TimerBlock(int infoIndex, char* file, char* function, int line, char* name = "") {
		this->infoIndex = infoIndex;
		addSampleAndInfo(infoIndex, SAMPLE_TYPE_BEGIN, file, function, line, name);
	}

	~TimerBlock() {
		addSample(infoIndex, SAMPLE_TYPE_END);
	}
};

#define TIMER_BLOCK() \
	TimerBlock timerBlock##__LINE__(__COUNTER__, __FILE__, __FUNCTION__, __LINE__);

#define TIMER_BLOCK_NAMED(name) \
	TimerBlock timerBlock##__LINE__(__COUNTER__, __FILE__, __FUNCTION__, __LINE__, name);

#define TIMER_BLOCK_BEGIN(ID) \
	const int timerCounter##ID = __COUNTER__; \
	addSampleAndInfo(timerCounter##ID, SAMPLE_TYPE_BEGIN, __FILE__, __FUNCTION__, __LINE__); 

#define TIMER_BLOCK_BEGIN_NAMED(ID, name) \
	const int timerCounter##ID = __COUNTER__; \
	addSampleAndInfo(timerCounter##ID, SAMPLE_TYPE_BEGIN, __FILE__, __FUNCTION__, __LINE__, name); 

#define TIMER_BLOCK_END(ID) \
	addSample(timerCounter##ID, SAMPLE_TYPE_END);
