
#define FRAME_BUFFER_SIZE 120
#define DEBUG_NOTE_LENGTH 50
#define DEBUG_NOTE_DURATION 3

//

struct TimerInterval {
	f64 totalTime = 0;
	int count = 0;
	f64 stamp = 0;

	f64 resultTime;

	bool update(f64 dt, f64 time, float intervalSeconds = 1) {
		totalTime += dt;
		count++;
		stamp += time;

		if(totalTime >= intervalSeconds) {
			resultTime = stamp/count;

			totalTime = 0;
			count = 0;
			stamp = 0;

			return true;
		}

		return false;
	}
};

// @AssetHotloading.

void initWatchFolders(SystemData* sd, char** folders, int folderCount) {
	for(int i = 0; i < folderCount; i++) {
		char* folder = folders[i];
		HANDLE fileChangeHandle = FindFirstChangeNotification(folder, true, FILE_NOTIFY_CHANGE_LAST_WRITE);
		if(fileChangeHandle == INVALID_HANDLE_VALUE) {
			printf("Could not set folder change notification.\n");
		}
		sd->folderHandles[i] = fileChangeHandle;
	}
	sd->folderHandleCount = folderCount;
}

void reloadChangedFiles(SystemData* sd) {
	// Todo: Get ReadDirectoryChangesW to work instead of FindNextChangeNotification.
	// Right now we check every single file in the folder for change.

	GraphicsState* gs = theGState;

	DWORD fileStatus = WaitForMultipleObjects(sd->folderHandleCount, &sd->folderHandles[0], false, 0);
	if(between(fileStatus, WAIT_OBJECT_0, WAIT_OBJECT_0 + sd->folderHandleCount)) {

		// Note: WAIT_OBJECT_0 is defined as 0, so we can use it as an index.
		int folderIndex = fileStatus;

		FindNextChangeNotification(sd->folderHandles[folderIndex]);

		if(folderIndex == 0) {
			for(int i = 0; i < gs->textureCount; i++) {
				Texture* tex = gs->textures + i;

				FILETIME newWriteTime = getLastWriteTime(tex->file);
				if(CompareFileTime(&tex->assetInfo.lastWriteTime, &newWriteTime) != 0) {

					dxLoadAndCreateTexture(tex);

					tex->assetInfo.lastWriteTime = newWriteTime;
				}
			}
		} else if(folderIndex == 1) {

			for(int i = 0; i < gs->meshCount; i++) {
				Mesh* m = gs->meshes + i;

				char* files[] = {m->file, m->mtlFile};
				for(int j = 0; j < arrayCount(m->assetInfo); j++) {
					FILETIME newWriteTime = getLastWriteTime(files[j]);

					if(CompareFileTime(&m->assetInfo[j].lastWriteTime, &newWriteTime) != 0) {

						ObjLoader parser;
						dxLoadMesh(m, &parser);
						parser.free();

						m->assetInfo[j].lastWriteTime = newWriteTime;
					}
				}
			}
		}
	}
}

//

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

	inputArray = getPArrayDebug(Input, capacity);
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
		threadQueueComplete(theThreadQueue);

		memory->index = snapshotCount-1;
		memory->arrays[memory->index].index = snapshotMemoryIndex;

		for(int i = 0; i < snapshotCount; i++) {
			memcpy(memory->arrays[i].data, snapshotMemory[i], memory->slotSize);
		}

		reloadMemory = false;
	}
}

void StateRecording::startRecording() {
	if(state != REC_STATE_INACTIVE || !threadQueueFinished(theThreadQueue)) return;

	snapshotCount = memory->index+1;
	snapshotMemoryIndex = memory->arrays[memory->index].index;
	for(int i = 0; i < snapshotCount; i++) {
		if(snapshotMemory[i] == 0) 
			snapshotMemory[i] = (char*)malloc(memory->slotSize);

		memcpy(snapshotMemory[i], memory->arrays[i].data, memory->slotSize);
	}

	state = REC_STATE_RECORDING;
	recordIndex = 0;
}

void StateRecording::startPlayback() {
	if(state == REC_STATE_INACTIVE && !threadQueueFinished(theThreadQueue)) return;

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

//

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
	Timer* timer;
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
	timer = getPStructDebug(Timer);
	timer->init(frameSlotCount);

	slotBufferMax = totalSlotBufferCount;
	slotBufferIndex = 0;
	slotBufferCount = 0;
	slotBuffer = getPArrayDebug(GraphSlot, slotBufferMax);
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

//

struct DebugSessionSettings {
	Rect menuPanel;
	int menuPanelActiveSection;
	Rect profilerPanel;
	int profilerPanelActiveSection;
	int panelHierarchy[2];
};

void debugWriteSessionSettings(DebugSessionSettings* at) {
	writeDataToFile((char*)at, sizeof(DebugSessionSettings), Gui_Session_File);
}

void debugReadSessionSettings(DebugSessionSettings* at) {
	readDataFromFile((char*)at, Gui_Session_File);
}

//

struct DebugState {
	MSTimer swapTimer;
	MSTimer frameTimer;
	MSTimer debugTimer;

	f64 dt;
	f64 time;

	// Stats.

	TimerInterval cpuInterval, gpuInterval, debugInterval;
	f64 cpuTime, gpuTime, debugTime;

	f64 fpsTime;
	int fpsCounter;
	float avgFps;

	//

	Input* input;
	StateRecording recState;

	//

	bool showMenu;
	bool showProfiler;
	bool showConsole;
	bool showHud;

	//

	int fontHeight;
	int fontHeightScaled;
	float fontScale;

	NewGui gui;
	float guiAlpha;

	int panelGotActiveIndex;
	int panelHierarchy[2];

	Rect menuPanel;
	int menuPanelActiveSection;
	ExpansionIndex* expansionArray;
	int expansionCount;

	Rect profilerPanel;
	int profilerPanelActiveSection;
	GraphCam graphCam;
	Profiler profiler;
	int statsSortingIndex;

	Console console;

	//

	char* notificationStack[10];
	float notificationTimes[10];
	int notificationCount;

	char* infoStack[40];
	int infoStackCount;
};

//

void addDebugNote(char* string, float duration = DEBUG_NOTE_DURATION) {
	DebugState* ds = theDebugState;

	assert(strLen(string) < DEBUG_NOTE_LENGTH);
	if(ds->notificationCount >= arrayCount(ds->notificationStack)) return;

	int count = ds->notificationCount;
	strClear(ds->notificationStack[count]);
	ds->notificationTimes[count] = duration;
	strCpy(ds->notificationStack[count], string);
	ds->notificationCount++;
}

void addDebugInfo(char* string) {
	DebugState* ds = theDebugState;

	if(ds->infoStackCount >= arrayCount(ds->infoStack)) return;
	ds->infoStack[ds->infoStackCount++] = string;
}

//

void debugMain(DebugState* ds, AppMemory* appMemory, AppData* ad, bool reload, bool* isRunning, bool init, ThreadQueue* threadQueue, int timerInfoCount, bool mouseInClient) {

	ds->debugTimer.start();

	theMemory->debugMode = true;
	Input* input = ds->input;
	WindowSettings* ws = &ad->wSettings;

	clearTMemoryDebug();

	//

	if(input->keysPressed[KEYCODE_F5]) {
		if(!input->keysDown[KEYCODE_CTRL]) ds->console.smallExtensionButtonPressed = true;
		else ds->console.bigExtensionButtonPressed = true;
	}
	if(input->keysPressed[KEYCODE_F6]) ds->showMenu = !ds->showMenu;
	if(input->keysPressed[KEYCODE_F7]) ds->showProfiler = !ds->showProfiler;
	if(input->keysPressed[KEYCODE_F8]) ds->showHud = !ds->showHud;

	//

	ds->recState.updateReloadMemory();
	ds->profiler.update(timerInfoCount, threadQueue);

	//

	int fontHeight = ds->fontHeight;
	Font* font = getFont("LiberationSans-Regular.ttf", fontHeight, "LiberationSans-Bold.ttf", "LiberationSans-Italic.ttf");

	if(init) {
		if(!fileExists(Gui_Session_File)) {
			DebugSessionSettings settings;
			settings.menuPanel = rectTLDim(0, 0, ds->fontHeight*18, ds->fontHeight*25);
			settings.menuPanelActiveSection = 0;
			settings.profilerPanel = rectCenDim(vec2(ws->currentRes)/2 * vec2(1,-1), vec2(ds->fontHeight*40, ds->fontHeight*29));

			settings.profilerPanelActiveSection = 0;
			settings.panelHierarchy[0] = 0;
			settings.panelHierarchy[1] = 1;

			debugWriteSessionSettings(&settings);
		}

		DebugSessionSettings settings;
		debugReadSessionSettings(&settings);
		ds->menuPanel = settings.menuPanel;
		ds->menuPanelActiveSection = settings.menuPanelActiveSection;
		ds->profilerPanel = settings.profilerPanel;
		ds->profilerPanelActiveSection = settings.profilerPanelActiveSection;
		ds->panelHierarchy[0] = settings.panelHierarchy[0];
		ds->panelHierarchy[1] = settings.panelHierarchy[1];
	}

	//

	{
		GraphicsState* gs = theGState;
		dxSetBlendState(Blend_State_DrawOverlay);
		// dxSetBlendState(gs->bsBlend);

		dxScissorState(false);

		dxClearFrameBuffer("DebugMsaa", vec4(0.0f));
		dxClearFrameBuffer("ds");
		dxBindFrameBuffer("DebugMsaa", "ds");
	}

	//

	NewGui* gui = &ds->gui;

	newGuiDefaultSettings(gui, font);
	newGuiBegin(gui, input, ws, ds->dt, mouseInClient);

	// @Panels.
	{
		// Update Hierarchy.
		if(ds->panelGotActiveIndex != -1) 
		{
			Rect rects[] = {ds->menuPanel, ds->profilerPanel};
			int panelCount = arrayCount(ds->panelHierarchy);

			// Panel not on top.
			if(ds->panelGotActiveIndex != ds->panelHierarchy[panelCount-1]) {
				int panelIndex = ds->panelGotActiveIndex;

				for(int i = 0; i < panelCount; i++) {
					if(ds->panelHierarchy[i] == panelIndex) {
						moveArray(ds->panelHierarchy + i, ds->panelHierarchy + i+1, int, panelCount-1-i);
						ds->panelHierarchy[panelCount-1] = panelIndex;

						newGuiClearHot(gui);
						break;
					}
				}
			}

			ds->panelGotActiveIndex = -1;
		}

		// Vec4 cMenuPanel = hslToRgbf(0.6f,0.3f,0.33f,1);
		// Vec4 cProfilerPanel = hslToRgbf(0.0f,0.3f,0.33f,1);

		Rect* rects[] = {&ds->menuPanel, &ds->profilerPanel};
		char* titles[] = {"<b>Menu<b>", "<b>Profiler<b>"};
		bool* actives[] = {&ds->showMenu, &ds->showProfiler};

		int maxUiElements = 10000;
		int startId = gui->id;
		int panelCount = arrayCount(ds->panelHierarchy);

		for(int i = 0; i < panelCount; i++) {
			int panelIndex = ds->panelHierarchy[i];

			gui->zLevel = i;
			gui->id = startId + maxUiElements*panelIndex;

			char* panelTitle = titles[panelIndex];
			Rect* panelRect = rects[panelIndex];
			bool* panelActive = actives[panelIndex];

			if(*panelActive)
			{
				float panelMargin = roundf(fontHeight*0.3f);
				float panelBorder = roundf(fontHeight*0.5f);

				newGuiSetHotAllMouseOver(gui, *panelRect, gui->zLevel);

				Rect clampRect = rectTLDim(0,0,ws->currentRes.w,ws->currentRes.h);
				GuiWindowSettings windowSettings = {panelBorder, panelBorder*3, vec2(fontHeight*10,fontHeight*2.5), 
					                                vec2(0,0), clampRect, true, true, true, false};
				newGuiWindowUpdate(gui, panelRect, windowSettings);

				Rect rPanel = *panelRect;

				//

				// @color
				// gui->boxSettings.color = cProfilerPanel;
				// gui->textBoxSettings.boxSettings.color = cProfilerPanel;
				newGuiQuickBox(gui, rPanel);

				Rect pri = rPanel.expand(-vec2(panelBorder*2));
				newGuiScissorPush(gui, pri);
				defer { newGuiScissorPop(gui); };

				//

				Font* font = gui->textSettings.font;

				float textPad = font->height * 1.5f;

				float elementHeight = font->height * 1.5f;
				float elementWidth = pri.w();
				Vec2 padding = vec2(panelMargin-1, panelMargin-1);

				Vec2 p = pri.tl();
				float eh = elementHeight;
				float ew = elementWidth;
				Vec2 pad = padding;

				//

				float headerHeight = eh * 1.2f;
				float separatorHeight = font->height * 0.3f;

				Vec4 bcLinear = gammaToLinear(gui->boxSettings.color);
				Vec4 cSeparatorDark = linearToGamma(bcLinear - vec4(0.1f,0));
				Vec4 cSeparatorBright = linearToGamma(bcLinear + vec4(0.1f,0));

				Rect r;
				char* s;
				QuickRow qr;

				// Header.
				{
					TextSettings headerTextSettings = textSettings(gui->textSettings.font, gui->textSettings.color, TEXTSHADOW_MODE_SHADOW, 1.0f, gui->boxSettings.borderColor);
					TextBoxSettings headerSettings = textBoxSettings(headerTextSettings, boxSettings());

					r = rectTLDim(p, vec2(ew, headerHeight)); p.y -= headerHeight+pad.y;
					newGuiQuickTextBox(gui, r, panelTitle, vec2i(0,0), &headerSettings);
				}

				// Close button.
				{
					TextBoxSettings tbs = gui->textBoxSettings; 
					tbs.boxSettings.borderColor = vec4(0);
					Rect rClose = r.rSetL(r.h());
					if(newGuiQuickButton(gui, rClose, "", &tbs)) *panelActive = false;

					Vec4 c = gui->textSettings.color;
					rClose = round(rClose);
					rClose = rectCenDim(rClose.c(), vec2(font->height * 0.5f));
					dxDrawCross(rClose.c(), rClose.w(), rClose.w()*0.3f, c);
				}

				// @Menu
				if(panelIndex == 0)
				{
					{
						TextBoxSettings tbs = textBoxSettings(gui->textSettings, boxSettings(gui->boxSettings.color));
						TextBoxSettings tbsActive = tbs;
						tbsActive.boxSettings.borderColor = gui->boxSettings.borderColor;

						char* labels[] = {"InputRecording", "Introspection", "Sun"};
						for(int i = 0; i < arrayCount(labels); i++) {
							bool active = ds->menuPanelActiveSection == i;

							char* text = labels[i];
							float labelWidth = getTextDim(text, font).w + textPad;

							r = rectTLDim(p, vec2(labelWidth, eh)); p.x += labelWidth;

							if(active) gui->setInactive = true;

							TextBoxSettings* settings = active ? &tbsActive : &tbs;
							if(newGuiQuickPButton(gui, r, text, settings)) ds->menuPanelActiveSection = i;
						}
						p.y -= eh+pad.y;
						p.x = pri.left;

						{
							p.y -= separatorHeight/2;
							guiDrawLineH(p+vec2(0,0.5f), p+vec2(ew,0)+vec2(0,0.5f), cSeparatorDark);
							guiDrawLineH(p+vec2(0,-0.5f), p+vec2(ew,0)+vec2(0,-0.5f), cSeparatorBright);
							p.y -= separatorHeight/2+pad.y;
						}
					}

					//

					switch(ds->menuPanelActiveSection) {

					// @RecordingSection
					case 0: {
						StateRecording* rec = &ds->recState;

						// rec, stop rec, play, not play, pause, resume, step, set breakpoint

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						newGuiQuickText(gui, r, fillString("Active Threads: %b.", !threadQueueFinished(threadQueue)), vec2i(-1,0));

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						newGuiQuickText(gui, r, fillString("Recorded Frames: %i of %i..", rec->recordIndex, rec->capacity), vec2i(-1,0));

						// rec, stop, pause, play
						{
							float h = eh*1.5f;
							float w = h;
							r = rectTLDim(p, vec2(ew, h)); p.y -= h+pad.y;
							
							float cols[] = {0, w,w,w,w, w, 0};
							qr = quickRow(r, pad.x, cols, arrayCount(cols));

							bool recordInactive = rec->state == REC_STATE_PLAYING;
							bool stopInactive = rec->state == REC_STATE_INACTIVE;
							bool pausePlayInactive = rec->state == REC_STATE_RECORDING || rec->recordIndex == 0;

							//

							quickRowNext(&qr);

							Vec4 c = gui->editSettings.textBoxSettings.boxSettings.color;
							Vec4 recColor = c;
							if(rec->state == REC_STATE_RECORDING) recColor = vec4(0.5f,0,0,1);

							if(recordInactive) gui->setInactive = true;
							r = round(quickRowNext(&qr));
							bool record = newGuiQuickButton(gui, r, "", vec2i(0,0));
							r = round(r.expand(-h*0.5f));
							dxDrawRect(r, recColor, dxGetTexture("misc\\circle.dds")->view);

							if(stopInactive) gui->setInactive = true;
							r = round(quickRowNext(&qr));
							bool stop = newGuiQuickButton(gui, r, "", vec2i(0,0));
							r = round(r.expand(-h*0.5f));
							dxDrawRect(r, c);

							if(pausePlayInactive) gui->setInactive = true;
							r = round(quickRowNext(&qr));
							bool pausePlay = newGuiQuickButton(gui, r, "", vec2i(0,0));
							r = round(r.expand(-h*0.5f));
							if(rec->state == REC_STATE_PLAYING && !rec->playbackPaused) {
								dxDrawRect(round(r.rSetR(r.w()/3)), c);
								dxDrawRect(round(r.rSetL(r.w()/3)), c);

							} else {
								dxDrawTriangle(r.bl(), r.tl(), r.r(), c);
							}

							if(pausePlayInactive) gui->setInactive = true;
							r = round(quickRowNext(&qr));
							bool step = newGuiQuickButton(gui, r, "", vec2i(0,0));
							r = round(r.expand(-h*0.5f));
							dxDrawTriangle(r.bl(), r.tl(), r.c(), c);
							dxDrawTriangle(r.b(), r.t(), r.r(), c);

							if(pausePlayInactive) gui->setInactive = true;
							newGuiQuickCheckBox(gui, quickRowNext(&qr), &rec->activeBreakPoint);

							quickRowNext(&qr);

							//

							if(rec->recordIndex > 0 && rec->state != REC_STATE_RECORDING) {
								r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
								int playbackIndex = rec->playbackIndex;
								if(newGuiQuickSlider(gui, r, &playbackIndex, 0, rec->recordIndex - 1)) {
									rec->breakPointIndex = playbackIndex;
									rec->activeBreakPoint = true;
								}

								SliderSettings ss = gui->sliderSettings;
								ss.textBoxSettings.textSettings.color = vec4(0,0);
								ss.textBoxSettings.boxSettings.color = vec4(0,0);
								ss.textBoxSettings.boxSettings.borderColor = vec4(0,0);
								ss.color = vec4(0,1);
								ss.borderColor = vec4(0,0);

								if(rec->activeBreakPoint) {
									gui->setInactive = true;
									newGuiQuickSlider(gui, r, &rec->breakPointIndex, 0, rec->recordIndex - 1, &ss);
								}
							}

							//

							bool availableFrames = rec->recordIndex > 0;
							if(rec->state == REC_STATE_INACTIVE) {
								if(record) rec->startRecording();

								if(availableFrames) {
									if(pausePlay) rec->startPlayback();
									if(step) {
										rec->startPlayback();
										rec->playbackStep();
									}
								}

							} else if(rec->state == REC_STATE_RECORDING) {
								if(record || stop) rec->stop();

							} else if(rec->state == REC_STATE_PLAYING) {
								if(stop) rec->stop();
								else if(pausePlay) rec->playbackPaused = !rec->playbackPaused;
								else if(step) rec->playbackStep();
							}

						}

					} break;

					// @Introspection
					case 1: {

						Vec4 cGrid = gui->editSettings.textBoxSettings.boxSettings.color;

						eh = roundInt(font->height*1.2f);
						pad.y = roundInt(font->height*0.2f);

						float buttonWidth = eh;
						int padding = eh/2;
						float seperatorWidth     = font->height*0.3f;
						float memberSectionWidth = 0.2f;
						float valueSectionWidth  = 0.0f;
						float typeSectionWidth = 0;
						for(int i = 0; i < arrayCount(structInfos); i++) {
							StructInfo* info = structInfos + i;
							typeSectionWidth = max(typeSectionWidth, getTextDim(info->name, font).w);
						}
						typeSectionWidth += seperatorWidth*2;

						float currentOffset = 0;

						TextEditSettings tes = gui->editSettings;
						tes.textBoxSettings.boxSettings.borderSize = 0;
						tes.textBoxSettings.boxSettings.borderColor.a = 0;
						tes.textBoxSettings.boxSettings.color.a = 0;
						tes.textOffset = 1;

						int strMaxSize = 1000;
						char* buffer = getTStringDebug(strMaxSize + 1);

						//

						// TestStruct s = {};
						// {
						// 	s.a = 1;
						// 	s.b = 2.3f;
						// 	s.c = vec3(1,2,3);
						// 	s.d[0] = 5;
						// 	s.d[1] = 6;
						// 	s.f = 3;
						// 	s.e = getTArray(float, s.f);
						// 	s.e[0] = 6;
						// 	s.e[1] = 7;
						// 	s.e[2] = 8;

						// 	s.h = 2;
						// 	s.g[0] = getTArray(int, s.h);
						// 	s.g[1] = getTArray(int, s.h);

						// 	s.g[0][0] = 1;
						// 	s.g[0][1] = 2;
						// 	s.g[1][0] = 3;
						// 	s.g[1][1] = 4;
						// }

						// TypeTraverse info;
						// info.start(getType(TestStruct), (char*)&s, "Test");

						TypeTraverse info;
						info.start(getType(Entity), (char*)ad->cameraEntity, "CameraEntity");

						// TypeTraverse info;
						// info.start(getType(Entity), (char*)ad->player, "player");

						while(info.next()) {
							Rect r;
							QuickRow qr;
							r = rectTLDim(p, vec2(ew, eh)); 
							float cols[] = {memberSectionWidth, seperatorWidth, valueSectionWidth, seperatorWidth, typeSectionWidth};
							qr = quickRow(r, pad.x, cols, arrayCount(cols));

							Rect rSeperators[2];
							Rect rMember   = quickRowNext(&qr);
							rSeperators[0] = quickRowNext(&qr);
							Rect rValue    = quickRowNext(&qr);
							rSeperators[1] = quickRowNext(&qr);
							Rect rType     = quickRowNext(&qr);

							// Ugh.
							if(info.action == TRAVERSETYPE_PUSH || info.action == TRAVERSETYPE_PRIMITIVE) {
								float y = roundf(rMember.bottom - pad.y/2)-0.5f;
								float x[] = {rMember.left, rSeperators[0].cx(), rSeperators[1].cx(), rType.right-1};
								for(int i = 0; i < arrayCount(x); i++) x[i] = roundf(x[i]) + 0.5f;

								for(int i = 0; i < arrayCount(x); i++) {
									dxDrawLine(vec2(x[i],y), vec2(x[i],y + eh+pad.y), cGrid);
								}
								rMember.left += 1;
								rType.right -= 1;

								if(info.first) {
									float yt = y + eh+pad.y;
									dxDrawLine(vec2(x[0],yt), vec2(x[arrayCount(x)-1],yt), cGrid);
								}

								dxDrawLine(vec2(x[0],y + 1), vec2(x[arrayCount(x)-1],y + 1), cGrid);

								p.y -= eh+pad.y;
							}

							if(info.action == TRAVERSETYPE_PUSH) {
								char* memberLabel = info.memberName;
								char* typeLabel = info.typeName;

								r = rMember;
								r = r.addL(currentOffset);
								Rect rButton = r.rSetR(buttonWidth);
								r = r.addL(buttonWidth + pad.x);

								bool expanded = false;
								{
									ExpansionIndex currentIndex = {};
									for(int i = 0; i < info.stackCount; i++) {
										currentIndex.indices[currentIndex.count++] = info.stack[i].index;
									}

									// Check if current index is expanded.
									int indexIndex = -1; // Yes.
									for(int i = 0; i < ds->expansionCount; i++) {
										if(currentIndex == ds->expansionArray[i]) {
											expanded = true;
											indexIndex = i;
											break;
										}
									}

									{
										TextBoxSettings tbs = gui->textBoxSettings; 
										tbs.boxSettings.borderColor = vec4(0);
										if(newGuiQuickButton(gui, rButton, "", &tbs)) {
											if(!expanded) {
												ds->expansionArray[ds->expansionCount++] = currentIndex;

											} else {
												ds->expansionArray[indexIndex] = ds->expansionArray[--ds->expansionCount];
											}
										}

										dxDrawTriangle(rButton.c(), rButton.w()/4, gui->textSettings.color, expanded ? vec2(0,-1) : vec2(1,0));
									}
								}

								char* str = buffer;
								defer { strClear(str); };

								// Collect content preview string.
								{
									info.saveState();

									strClear(str);
									strAppend(str, !info.isArray ? "{" : "[");

									while(info.next()) {
										if(info.action == TRAVERSETYPE_POP && info.savedStackCount == info.stackCount+1) break;

										if(info.action == TRAVERSETYPE_PUSH) {
											if(!info.lastActionWasPush) strAppend(str, ",");

											strAppend(str, !info.isArray ? " {" : " [");

										} else if(info.action == TRAVERSETYPE_POP) {
											strAppend(str, !info.isArray ? " }" : " ]");

										} else if(info.action == TRAVERSETYPE_PRIMITIVE) {
											char* data = info.data;

											strAppend(str, info.firstMember ? " " : ", ");
											if(!info.isArray) strAppend(str, fillString("%s=", info.memberName));

											char* s;
											switch(info.memberType) {
												case TYPE_int:   s = fillString("%i", *((int*)data)); break;
												case TYPE_float: s = fillString("%f", *((float*)data)); break;
												case TYPE_bool:  s = fillString("%bU", *((bool*)data)); break;
												case TYPE_char:  s = fillString("'%c'", *((char*)data)); break;
												default: break;
											};

											strAppend(str, s);
										}
									}

									strAppend(str, !info.isArray ? " }" : " ]");

									if(expanded) info.loadState();
								}

								newGuiQuickText(gui, r, memberLabel, vec2i(-1,0));
								newGuiQuickText(gui, rValue, getTStringCpyDebug(str), vec2i(-1,0));
								newGuiQuickText(gui, rType, typeLabel, vec2i(-1,0));

								if(expanded) {
									currentOffset += padding;
								}

							} else if(info.action == TRAVERSETYPE_PRIMITIVE) {

								r = rMember;
								r = r.addL(currentOffset + buttonWidth + pad.x);
								newGuiQuickText(gui, r, info.memberName, vec2i(-1,0));

								r = rValue;
								char* data = info.data;
								switch(info.memberType) {
									case TYPE_int:   newGuiQuickTextEdit(gui, r, (int*)data, &tes); break;
									case TYPE_float: newGuiQuickTextEdit(gui, r, (float*)data, &tes); break;
									case TYPE_bool:  {
										TextBoxSettings tbs = gui->textBoxSettings; 
										tbs.boxSettings.borderColor = vec4(0);
										tbs.sideAlignPadding = 0;

										bool* b = (bool*)data;
										char* label = fillString("%bU", *b);
										float w = getTextDim(label, font).w;
										if(newGuiQuickButton(gui, r.rSetR(w), label, vec2i(-1,0), &tbs)) *b = !(*b);
									} break;
									case TYPE_char:  {
										char* temp = getTStringCpyDebug((char*)data, 1);
										newGuiQuickTextEdit(gui, r, temp, 1, &tes);
										data[0] = temp[0];
									} break;
									case TYPE_uchar: {
										char c = *((uchar*)data);
										char* temp = getTStringDebug(2);
										temp[0] = c;
										temp[1] = '\0';
										newGuiQuickText(gui, r, temp); break;
									}
									default: break;
								};

								char* typeLabel = info.typeName;
								newGuiQuickText(gui, rType, typeLabel, vec2i(-1,0));

							} else if(info.action == TRAVERSETYPE_POP) {
								currentOffset -= padding;
							}
						}
					} break;

					// @Sun
					case 2: {
						SkyShaderVars* skyVars = dxGetShaderVars(Sky);

						int li = 0;
						char* labels[] = { "spotBrightness", "mieBrightness", "mieDistribution", "mieStrength", "mieCollectionPower", "rayleighBrightness", "rayleighStrength", "rayleighCollectionPower", "scatterStrength", "surfaceHeight", "intensity", "stepCount", "horizonOffset", "sunOffset" };

						float labelMaxWidth = getTextMaxWidth(labels, arrayCount(labels), font) + padding.x;

						int types[] = {0,0,0,0,0,0,0,0,0,0,0,1,0,0}; // 0 float, 1 int
						Vec2 ranges[] = {{0,10}, {0,2}, {0,2}, {0,2}, {0,2}, {0,2}, {0,2}, {0,2}, {0,2}, {0, 1}, {0, 5}, {1,30}, {0,1}, {0,1}};

						void* ptrs[] = {
							&skyVars->spotBrightness, &skyVars->mieBrightness, &skyVars->mieDistribution, &skyVars->mieStrength, &skyVars->mieCollectionPower, &skyVars->rayleighBrightness, &skyVars->rayleighStrength, &skyVars->rayleighCollectionPower, &skyVars->scatterStrength, &skyVars->surfaceHeight, &skyVars->intensity, &skyVars->stepCount, &skyVars->horizonOffset, &skyVars->sunOffset,
						};

						gui->sliderSettings.notifyWhenActive = true;
						defer{gui->sliderSettings.notifyWhenActive = false;};

						char* label = "sunDir";
						for(int i = 0; i < 2; i++) {
							r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
							qr = quickRow(r, pad.x, labelMaxWidth, 0.0f);

							if(i == 0) {
								newGuiQuickText(gui, quickRowNext(&qr), label, vec2i(-1,0));
							} else {
								quickRowNext(&qr);
							}

							Vec2 range = i == 0 ? vec2(0, M_2PI) : vec2(-M_PI_2 + 0.0001f, M_PI_2 - 0.0001f);
							if(newGuiQuickSlider(gui, quickRowNext(&qr), ad->sunAngles.e + i, range.x, range.y)) {
								ad->redrawSkyBox = true;
							}
						}

						for(int i = 0; i < arrayCount(labels); i++) {
							r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
							qr = quickRow(r, pad.x, labelMaxWidth, 0.0f);
							newGuiQuickText(gui, quickRowNext(&qr), labels[li++], vec2i(-1,0));

							void* ptr = ptrs[i];
							Vec2 range = ranges[i];

							bool active = false;
							if(types[i] == 0) {
								if(newGuiQuickSlider(gui, quickRowNext(&qr), (float*)ptr, range.x, range.y)) {
									active = true;
								}
							} else {
								if(newGuiQuickSlider(gui, quickRowNext(&qr), (int*)ptr, (int)range.x, (int)range.y)) {
									active = true;
								}
							}

							if(active) {
								ad->redrawSkyBox = true;
							}
						}

					} break;

					}
				}

				// @Profiler
				if(panelIndex == 1)
				{
					{
						TextBoxSettings tbs = textBoxSettings(gui->textSettings, boxSettings(gui->boxSettings.color));
						TextBoxSettings tbsActive = tbs;
						tbsActive.boxSettings.borderColor = gui->boxSettings.borderColor;

						char* labels[] = {"Stats", "Graph"};
						for(int i = 0; i < arrayCount(labels); i++) {
							bool active = ds->profilerPanelActiveSection == i;

							char* text = labels[i];
							float labelWidth = getTextDim(text, font).w + textPad;

							r = rectTLDim(p, vec2(labelWidth, eh)); p.x += labelWidth;

							if(active) gui->setInactive = true;

							TextBoxSettings* settings = active ? &tbsActive : &tbs;
							if(newGuiQuickPButton(gui, r, text, settings)) ds->profilerPanelActiveSection = i;
						}

						{
							char* label = fillString("%fms", ds->debugTime);
							r = rectTLDim(p, vec2(pri.right - p.x,eh));
							qr = quickRow(r, pad.x, 0, getTextDim(label, font).w + textPad/2, 1.01f);

							quickRowNext(&qr);
							newGuiQuickText(gui, quickRowNext(&qr), label, vec2i(1,0));
							quickRowNext(&qr);
						}

						p.y -= eh+pad.y;
						p.x = pri.left;

						{
							Profiler* prof = &ds->profiler;

							char* labels[] = {"Pause", "Resume"};
							r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
							qr = quickRow(r, pad.x, getTextDim(labels[1], font).w + textPad, 0);

							if(newGuiQuickButton(gui, quickRowNext(&qr), prof->noCollating ? labels[1] : labels[0])) {
								prof->noCollating = !prof->noCollating;

								if(prof->noCollating) {
									ds->graphCam.init = false;
									prof->setPause();

								} else prof->setPlay();
							}

							newGuiQuickSlider(gui, quickRowNext(&qr), &prof->frameIndex, 0, FRAME_BUFFER_SIZE-1);
						}

						{
							p.y -= separatorHeight/2;
							guiDrawLineH(p+vec2(0,0.5f), p+vec2(ew,0)+vec2(0,0.5f), cSeparatorDark);
							guiDrawLineH(p+vec2(0,-0.5f), p+vec2(ew,0)+vec2(0,-0.5f), cSeparatorBright);
							p.y -= separatorHeight/2+pad.y;
						}
					}

					//

					Profiler* prof = &ds->profiler;
					Timer* timer = prof->timer;
					Timings* timings = prof->timings[prof->currentFrameIndex];
					Statistic* statistics = prof->statistics[prof->currentFrameIndex];

					//

					static int highlightedIndex = -1;
					Vec4 highlightColor = vec4(1,1,1,0.05f);

					float cyclesPerFrame = (float)((3.5f*((float)1/60))*1000*1000*1000);
					int fontSize = ds->fontHeight;
					Vec2 textPos = vec2(550, -ds->fontHeight);
					int infoCount = timer->timerInfoCount;

					//

					switch(ds->profilerPanelActiveSection) {

					// @Stats
					case 0: {
						int barWidth = 1;
						int barCount = arrayCount(prof->timings);

						float cols[] = {0,0,0,0,0,0,0,0, barWidth*barCount};
						char* labels[] = {"File", "Function", "Description", "Cycles", "Hits", "C/H", "Avg. Cycl.", "Total Time", "Graphs"};

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, cols, arrayCount(cols));

						for(int i = 0; i < arrayCount(cols); i++) {
							TextBoxSettings tbs = gui->textBoxSettings; 
							tbs.boxSettings.borderColor = vec4(0);
							tbs.textSettings = gui->textSettings2;
							if(newGuiQuickButton(gui, quickRowNext(&qr), fillString("<b>%s<b>", labels[i]), &tbs)) {
								if(abs(ds->statsSortingIndex) == i) ds->statsSortingIndex *= -1;
								else ds->statsSortingIndex = i;
							}
						}

						int* sortIndices = getTArrayDebug(int, infoCount);
						{
							int count = infoCount;
							for(int i = 0; i < count; i++) sortIndices[i] = i;

							int si = abs(ds->statsSortingIndex);

							if(between(si, 0, 2)) {
								char** data = getTArrayDebug(char*, count);

								TimerInfo* timerInfos = timer->timerInfos;
								for(int i = 0; i < count; i++) {
									TimerInfo* tInfo = timerInfos + i;
									if(!tInfo->initialised) data[i] = getTStringCpyDebug("");
									else {
										     if(si == 0) data[i] = getTStringCpyDebug(timerInfos[i].file);						
										else if(si == 1) data[i] = getTStringCpyDebug(timerInfos[i].function);						
										else if(si == 2) data[i] = getTStringCpyDebug(timerInfos[i].name);						
									}
								}

								auto cmp = [](void* a, void* b) { return sortFuncString(VDref(SortPair<char*>, a).key, VDref(SortPair<char*>, b).key); };
								bubbleSort(data, sortIndices, count, cmp);

							} else if(between(si, 3, 7)) {
								double* data = getTArrayDebug(double, count);

								     if(si == 3) for(int i = 0; i < count; i++) data[i] = timings[i].cycles;
								else if(si == 4) for(int i = 0; i < count; i++) data[i] = timings[i].hits;
								else if(si == 5) for(int i = 0; i < count; i++) data[i] = timings[i].cyclesOverHits;
								else if(si == 6) for(int i = 0; i < count; i++) data[i] = statistics[i].avg;
								else if(si == 7) for(int i = 0; i < count; i++) data[i] = timings[i].cycles/cyclesPerFrame;

								auto cmp = [](void* a, void* b) { return VDref(SortPair<double>, a).key < VDref(SortPair<double>, b).key; };
								bubbleSort(data, sortIndices, count, cmp);
						   	}

						   	bool sortDirection = ds->statsSortingIndex < 0 ? false : true;
						   	if(sortDirection) {
						   		for(int i = 0; i < count/2; i++) swap(sortIndices + i, sortIndices + (count-1-i));
						   	}
						}

						for(int index = 0; index < infoCount; index++) {
							int i = sortIndices[index];

							TimerInfo* tInfo = timer->timerInfos + i;
							Timings* timing = timings + i;

							if(!tInfo->initialised) continue;

							r = rectTLDim(p, vec2(ew, eh)); p.y -= eh;
							qr = quickRow(r, pad.x, cols, arrayCount(cols));

							newGuiQuickText(gui, quickRowNext(&qr), fillString("%s", tInfo->file + 21), vec2i(-1,0));

							TextBoxSettings tbs = gui->textBoxSettings; tbs.boxSettings.borderColor = vec4(0);
							if(newGuiQuickButton(gui, quickRowNext(&qr), fillString("%s", tInfo->function), vec2i(-1,0), &tbs)) {
								char* command = fillString("%s %s:%i", Editor_Executable_Path, tInfo->file, tInfo->line);
								shellExecuteNoWindow(command);
							}
							newGuiQuickText(gui, quickRowNext(&qr), fillString("%s", tInfo->name), vec2i(-1,0));
							newGuiQuickText(gui, quickRowNext(&qr), fillString("%i64.c", timing->cycles), vec2i(-1,0));
							newGuiQuickText(gui, quickRowNext(&qr), fillString("%i64.", timing->hits), vec2i(-1,0));
							newGuiQuickText(gui, quickRowNext(&qr), fillString("%i64.c", timing->cyclesOverHits), vec2i(-1,0));
							newGuiQuickText(gui, quickRowNext(&qr), fillString("%i64.c", (i64)statistics[i].avg), vec2i(-1,0)); // Not a i64 but whatever.
							newGuiQuickText(gui, quickRowNext(&qr), fillString("%.3f%%", ((float)timing->cycles/cyclesPerFrame)*100), vec2i(-1,0));

							// Bar graph.

							// dcState(STATE_LINEWIDTH, barWidth);

							Rect r = quickRowNext(&qr);
							r.left = round(r.left) + 0.5f;
							r.right = round(r.right) + 0.5f;

							float rheight = r.h();
							float fontBaseOffset = 4;

							dxBeginPrimitiveColored(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

							float xOffset = 0;
							for(int statIndex = 0; statIndex < barCount; statIndex++) {
								Statistic* stat = statistics + i;
								u64 coh = prof->timings[statIndex][i].cyclesOverHits;

								float height = mapRangeClamp(coh, stat->min, stat->max, 1, rheight);
								Vec2 rmin = r.min + vec2(xOffset, fontBaseOffset);
								float colorOffset = mapRange01((double)coh, stat->min, stat->max);

								dxPushLine(rmin, rmin + vec2(0,height), vec4(colorOffset,1-colorOffset,0,1));

								xOffset += barWidth;
							}

							dxEndPrimitive();
						}

					} break;

					// @Graph
					case 1: {
						Vec4 bgColor = vec4(0,0.15f);
						Vec4 cThreadLine = vec4(0,0.5f);
						Vec4 cSwapBoundary = vec4(0,0.5f);
						Vec4 cMinimap = vec4(0,0.25f);
						TextBoxSettings slotSettings = gui->textBoxSettings2;
						slotSettings.textSettings.cull = true;

						//

						// Rect cyclesRect = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						Rect headerRect = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;

						float lineHeightOffset = 1.2;
						float lineHeight = ds->fontHeight * lineHeightOffset;

						int stackCountMain = 3;
						int stackCountThreads = 2;

						eh = stackCountMain*lineHeight + stackCountThreads*lineHeight*(threadQueue->threadCount);
						Rect bgRect = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						eh = elementHeight;

						//

						int swapTimerIndex = 0;
						for(int i = 0; i < timer->timerInfoCount; i++) {
							if(!timer->timerInfos[i].initialised) continue;

							if(strCompare(timer->timerInfos[i].name, "Swap")) {
								swapTimerIndex = i;
								break;
							}
						}

						//

						int lastIndex = mod(prof->slotBufferIndex-1, prof->slotBufferMax);
						int firstIndex = mod(prof->slotBufferIndex - prof->slotBufferCount, prof->slotBufferMax);
						GraphSlot recentSlot = prof->slotBuffer[lastIndex];
						GraphSlot oldSlot = prof->slotBuffer[firstIndex];
						double cyclesLeft = oldSlot.cycles;
						double cyclesRight = recentSlot.cycles + recentSlot.size;

						//

						GraphCam* cam = &ds->graphCam;
						if(!cam->init) {
							graphCamInit(cam, cyclesLeft, cyclesRight, 0, 1);
							cam->init = true;
						}

						// Update cam.
						{
							Input* input = gui->input;
							double maxZoomLevel = 1000;

							// Translate.

							Vec2 offset = vec2(0,0);
							int dragEvent = newGuiGoDragAction(gui, bgRect);	
							if(dragEvent == 1) {
								gui->mouseAnchor = input->mousePosNegative;

							} else if(dragEvent == 2) {
								offset = gui->mouseAnchor - input->mousePosNegative;
								gui->mouseAnchor = input->mousePosNegative;
							}

							// Scale.

							bool scale = false;
							float zoom = !input->keysDown[KEYCODE_CTRL]  ? 0.9f : 
							             !input->keysDown[KEYCODE_SHIFT] ? 0.8f : 0.6f;
							if(dragEvent == 0) {
								if(newGuiGoButtonAction(gui, bgRect, Gui_Focus_MWheel)) scale = true;
							}

							//

							graphCamSetViewPort(cam, bgRect);
							graphCamUpdateSides(cam);
							if(scale) graphCamScaleToPos(cam, input->mouseWheel, zoom, maxZoomLevel, 0, 0, 0, input->	mousePosNegative);
							if(offset != vec2(0,0)) graphCamTrans(cam, offset.x, offset.y);
							graphCamSizeClamp(cam, maxZoomLevel, 0);
							graphCamPosClamp(cam);
						}

						// Header.
						{
							dxDrawRect(graphCamMiniMap(cam, headerRect), cMinimap);

							Vec4 lineColor = vec4(0.7f, 1);
							float heightMod = 0.0f;
							double div = 4;
							double divMod = (1/div) + 0.05f;

							double timelineSection = div;
							while(timelineSection < cam->w*divMod*(ws->currentRes.h/(bgRect.w()))) {
								timelineSection *= div;
								heightMod += 0.1f;
							}

							clampMax(&heightMod, 1.0f);

							// dcState(STATE_LINEWIDTH, 3);
							double startPos = roundMod(cam->left, timelineSection) - timelineSection;
							double pos = startPos;
							while(pos < cam->right + timelineSection) {

								float p = graphCamMapX(cam, pos);

								// Big line.
								{
									float h = headerRect.h() * heightMod;
									dxDrawLine(vec2(p,headerRect.min.y), vec2(p,headerRect.min.y + h), lineColor);
								}

								// // Text
								// {
								// 	Vec2 textPos = vec2(p,headerRect.bottom + cyclesDim.h/2);
								// 	float percent = mapRange(pos, cyclesLeft, cyclesRight, 0.0, 100.0);
								// 	int percentInterval = mapRange(timelineSection, 0.0, cyclesSize, 0.0, 100.0);

								// 	char* s;
								// 	if(percentInterval > 10) s = fillString("%i%%", (int)percent);
								// 	else if(percentInterval > 1) s = fillString("%.1f%%", percent);
								// 	else if(percentInterval > 0.1) s = fillString("%.2f%%", percent);
								// 	else s = fillString("%.3f%%", percent);

								// 	float tw = getTextDim(s, gui->font).w;
								// 	if(between(bgRect.min.x, textPos.x - tw/2, textPos.x + tw/2)) textPos.x = bgRect.min.x + tw/2;
								// 	if(between(bgRect.max.x, textPos.x - tw/2, textPos.x + tw/2)) textPos.x = bgRect.max.x - tw/2;

								// 	drawText(s, gui->font, textPos, gui->colors.textColor, vec2i(0,0), 0, 1, gui->colors.shadowColor);
								// }

								pos += timelineSection;
							}
							// dcState(STATE_LINEWIDTH, 1);

							pos = startPos;
							timelineSection /= div;
							heightMod *= 0.6f;
							int index = 0;
							while(pos < cam->right + timelineSection) {

								// Small line.
								if((index%(int)div) != 0) {
									Vec2 p = vec2(graphCamMapX(cam, pos), headerRect.bottom);
									dxDrawLine(p, p + vec2(0, headerRect.h()*heightMod), lineColor);
								}

								// Cycle text.
								{
									float pMid = mapRange(pos - timelineSection/2.0, cam->left, cam->right, (double)bgRect.left, (double)bgRect.right);
									Vec2 textPos = vec2(pMid,headerRect.bottom + headerRect.h()/3);

									double cycles = timelineSection;
									char* s;
									if(cycles < 1000) s = fillString("%ic", (int)cycles);
									else if(cycles < 1000000) s = fillString("%.1fkc", cycles/1000);
									else if(cycles < 1000000000) s = fillString("%.1fmc", cycles/1000000);
									else if(cycles < 1000000000000) s = fillString("%.1fbc", cycles/1000000000);
									else s = fillString("INF");

									drawText(s, textPos, vec2i(0,0), gui->textSettings2);
								}

								pos += timelineSection;
								index++;
							}
						}

						// Highlighted slot.

						bool mouseHighlight = false;
						Rect hRect;
						char* hText;
						Vec4 hc;

						//

						// dcState(STATE_LINEWIDTH, 1);

						BoxSettings bs = gui->boxSettings;
						bs.color = bgColor;
						newGuiQuickBox(gui, bgRect, &bs);

						Vec2 pos = bgRect.tl();
						for(int threadIndex = 0; threadIndex < threadQueue->threadCount+1; threadIndex++) {

							// Horizontal lines to distinguish thread bars.
							if(threadIndex > 0) {
								dxDrawLine(pos, vec2(bgRect.right, pos.y), cThreadLine);
							}

							int bufferCount = prof->slotBufferCount;
							for(int i = 0; i < bufferCount; ++i) {
								GraphSlot* slot = prof->slotBuffer + ((firstIndex+i)%prof->slotBufferMax);
								if(slot->threadIndex != threadIndex) continue;

								if(slot->cycles + slot->size < cam->left || slot->cycles > cam->right) continue;

								//

								float left  = graphCamMapX(cam, slot->cycles);
								float right = graphCamMapX(cam, slot->cycles + slot->size);

								// Draw vertical line at swap boundaries.
								if(slot->timerIndex == swapTimerIndex) {
									dxDrawLine(vec2(right, bgRect.bottom), vec2(right, bgRect.top), cSwapBoundary);
								}

								float y = pos.y - (slot->stackIndex*lineHeight);
								Rect r = rect(left, y - lineHeight, right, y);

								bool visible = right - left >= 1;
								if(visible) {
									TimerInfo* tInfo = timer->timerInfos + slot->timerIndex;

									Vec4 color = vec4(tInfo->color, 1);
									char* text = fillString("%s %s (%i.c)", tInfo->function, tInfo->name, slot->size);
									r.left = max(r.left, bgRect.left);

									if(pointInRect(gui->input->mousePosNegative, r)) {
										mouseHighlight = true;
										hRect = r;
										hc = color;
										hText = text;

									} else {
										slotSettings.boxSettings.color = color;
										newGuiQuickTextBox(gui, r, text, vec2i(-1,0), &slotSettings);
									}

								} else {
									dxDrawLine(r.b(), r.t(), slotSettings.boxSettings.borderColor);
								}
							}

							if(threadIndex == 0) pos.y -= stackCountMain*lineHeight;
							else pos.y -= stackCountThreads*lineHeight;
						}

						if(mouseHighlight) {
							float tw = getTextDim(hText, slotSettings.textSettings.font).w + 
							                      slotSettings.sideAlignPadding*2;
							hRect.right = max(hRect.right, hRect.left + tw);

							slotSettings.boxSettings.color = hc;
							newGuiQuickTextBox(gui, hRect, hText, vec2i(-1,0), &slotSettings);
						}

					} break;

					}
				}

				if(ds->panelGotActiveIndex == -1) {
					if(gui->gotActiveId) {
						ds->panelGotActiveIndex = panelIndex;
					}
				}

			}
		}

		gui->id = startId + panelCount*maxUiElements;
		gui->zLevel = 0;
	}

	newGuiPopupSetup(gui);
	newGuiEnd(gui);

	// @Console.
	// Not actually usefull right now. More of a proof of concept...
	{
		Console* con = &ds->console;

		con->update(ds->input, vec2(ws->currentRes), ds->fontHeight, ad->dt);

		// Execute commands.

		if(con->commandAvailable) {
			con->commandAvailable = false;

			char* comName = con->comName;
			char** args = con->comArgs;
			char* resultString = "";
			bool pushResult = true;

			if(strCompare(comName, "add")) {
				int a = strToInt(args[0]);
				int b = strToInt(args[1]);

				resultString = fillString("%i + %i = %i.", a, b, a+b);

			} else if(strCompare(comName, "addFloat")) {
				float a = strToFloat(args[0]);
				float b = strToFloat(args[1]);

				resultString = fillString("%f + %f = %f.", a, b, a+b);

			} else if(strCompare(comName, "print")) {
				resultString = fillString("\"%s\"", args[0]);

			} else if(strCompare(comName, "cls")) {
				con->clearMainBuffer();
				pushResult = false;

			} else if(strCompare(comName, "doNothing")) {

			} else if(strCompare(comName, "setGuiAlpha")) {
				ds->guiAlpha = strToFloat(args[0]);

			} else if(strCompare(comName, "exit")) {
				*isRunning = false;

			}
			if(pushResult) con->pushToMainBuffer(resultString);
		}

		con->updateBody();
	}

	// @Notifications.
	{
		// Update notes.
		int deletionCount = 0;
		for(int i = 0; i < ds->notificationCount; i++) {
			ds->notificationTimes[i] -= ds->dt;
			if(ds->notificationTimes[i] <= 0) {
				deletionCount++;
			}
		}

		// Delete expired notes.
		if(deletionCount > 0) {
			for(int i = 0; i < ds->notificationCount-deletionCount; i++) {
				ds->notificationStack[i] = ds->notificationStack[i+deletionCount];
				ds->notificationTimes[i] = ds->notificationTimes[i+deletionCount];
			}
			ds->notificationCount -= deletionCount;
		}

		// Draw notes.
		int fontHeight = ds->fontHeight*1.3f;
		TextSettings ts = {getFont("LiberationSans-Regular.ttf", fontHeight), vec4(1.0f,0.4f,0.0f,1), 
		                   TEXTSHADOW_MODE_SHADOW, vec2(1,-1), fontHeight*0.10f, vec4(0,1)};

		float y = -fontHeight * 0.2f;
		for(int i = 0; i < ds->notificationCount; i++) {
			drawText(ds->notificationStack[i], vec2(ws->currentRes.w/2, y), vec2i(0,1), ts); 
			y -= fontHeight;
		}
	}

	// @Hud
	if(ds->showHud) {
		int fontHeight = ds->fontHeight*1.1f;
		TextSettings ts = {getFont("consola.ttf", fontHeight), vec4(1.0f,0.4f,0.0f,1), 
		                   TEXTSHADOW_MODE_SHADOW, vec2(1,-1), fontHeight*0.10f, vec4(0,1)};

		// Vec2 tp = vec2(ad->wSettings.currentRes.x, 0) - fontHeight*0.3f;
		// Vec2i align = vec2i(1,1);

		float off = fontHeight*0.3f;
		Vec2 tp = vec2(0, 0) + vec2(off, -off);
		Vec2i align = vec2i(-1,1);

		f64 msPerFrame = (1/(f64)ws->refreshRate) * 1000;
		float cpuPercent = (float)(ds->cpuTime/msPerFrame)*100;
		float gpuPercent = (float)(ds->gpuTime/msPerFrame)*100;
		float debugPercent = (float)(ds->debugTime/msPerFrame)*100;

		addDebugInfo( fillString("Fps: %i",              roundInt(ds->avgFps)));
		addDebugInfo( fillString("Cpu time:   %fms (%f%%)", ds->cpuTime, cpuPercent ));
		addDebugInfo( fillString("Gpu time:   %fms (%f%%)", ds->gpuTime, gpuPercent ));
		addDebugInfo( fillString("Debug time: %fms (%f%%)", ds->debugTime, debugPercent ));

		addDebugInfo( fillString("Cam pos: (%f,%f,%f)", PVEC3(ad->activeCam.pos) ));
		addDebugInfo( fillString("Cam dir: (%f,%f,%f)", PVEC3(ad->activeCam.look) ));

		addDebugInfo( fillString("Window size: (%i,%i)", PVEC2(ws->currentRes) ));

		addDebugInfo( fillString("BufferIndex: %i",      ds->profiler.timer->bufferIndex ));
		addDebugInfo( fillString("LastBufferIndex: %i",  ds->profiler.timer->lastBufferIndex ));

		for(int i = 0; i < ds->infoStackCount; i++) {
			drawText(fillString("%s", ds->infoStack[i]), tp, align, ts); 
			tp.y -= fontHeight;
		}
		ds->infoStackCount = 0;
	}

	//

	if(*isRunning == false) {
		DebugSessionSettings settings = {
			ds->menuPanel, ds->menuPanelActiveSection, 
			ds->profilerPanel, ds->profilerPanelActiveSection, 
			ds->panelHierarchy[0], ds->panelHierarchy[1],
		};
		debugWriteSessionSettings(&settings);
	}

	if(ds->debugInterval.update(ds->dt, ds->debugTimer.stop())) {
		ds->debugTime = ds->debugInterval.resultTime * 1000;
	}
}