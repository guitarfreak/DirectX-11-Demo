
void debugInit(DebugState* ds) {
	*ds = {};

	setMemory(true);
	defer { setMemory(false); };

	ds->recState.init(600, theMemory->pMemory);
	
	ds->profiler.init(10000, 1000);
	theTimer = ds->profiler.timer;

	ds->input = getPStruct(Input);
	initInput(ds->input);

	// ds->showUI = false;
	ds->showUI = true;
	ds->showConsole = false;
	ds->showHud = false;
	ds->guiAlphaMax = 0.90f;
	ds->guiAlpha = ds->guiAlphaMax;

	for(int i = 0; i < arrayCount(ds->noteData.stack); i++) {
		ds->noteData.stack[i] = getPArray(char, DEBUG_NOTE_LENGTH+1);
	}

	ds->fontScale = 1.0f;

	ds->console.init();

	ds->swapTimer.init();
	ds->frameTimer.init();
	ds->debugTimer.init();

	ds->panelActivityIndex = 0;

	ds->panelInit = true;

	ds->entityUI.selectionMode = ENTITYUI_MODE_TRANSLATION;
	ds->entityUI.localMode = false;
	ds->entityUI.snapGridSize = 1;
	ds->entityUI.snapGridDim = 100;
}

void debugMain(DebugState* ds, AppMemory* appMemory, AppData* ad, bool reload, bool* isRunning, bool init, int timerInfoCount, bool mouseInClient) {

	ds->debugTimer.start();

	Input* input = ds->input;
	WindowSettings* ws = &ad->wSettings;

	setMemory(true);
	clearTMemory();

	//

	if(input->keysPressed[KEYCODE_F5]) {
		if(!input->keysDown[KEYCODE_CTRL]) ds->console.smallExtensionButtonPressed = true;
		else ds->console.bigExtensionButtonPressed = true;
	}
	if(input->keysPressed[KEYCODE_F6]) ds->showUI = !ds->showUI;
	if(input->keysPressed[KEYCODE_F8]) ds->showHud = !ds->showHud;

	//

	ds->recState.updateReloadMemory();
	ds->profiler.update(timerInfoCount, theThreadQueue);

	//

	{
		GraphicsState* gs = theGState;
		dxSetBlendState(Blend_State_DrawOverlay);
		// dxSetBlendState(gs->bsBlend);

		dxScissorState(false);

		dxClearFrameBuffer("DebugMsaa", vec4(0.0f));
		dxClearFrameBuffer("ds");
		dxBindFrameBuffer("DebugMsaa", "ds");

		dxLineAA(1);
	}

	{
		int mode = ds->timeMode;
		if(input->keysPressed[KEYCODE_PAGEUP]) ds->timeMode++;
		if(input->keysPressed[KEYCODE_PAGEDOWN]) ds->timeMode--;
		if(input->keysPressed[KEYCODE_END]) ds->timeMode = 0;
		if(input->keysPressed[KEYCODE_PAUSE]) ds->timeStop = !ds->timeStop;

		clamp(&ds->timeMode, -8, 3);
	}

	//

	updateEntityUI(ds, &ad->entityManager, ad->levelEdit);

	{
		Font* font = getFont("LiberationSans-Regular.ttf", ds->fontHeight, "LiberationSans-Bold.ttf", "LiberationSans-Italic.ttf");

		Gui* gui = &ds->gui;
		gui->defaultSettings(font);
		gui->begin(ds->input, ws, ds->dt, mouseInClient);

		if(ds->showUI) {
			updateDebugUI(ds, ad, ws);
		}

		updateConsole(ds, ds->dt, isRunning);

		gui->end();
	}

	updateNotifications(&ds->noteData, ds->fontHeight, ds->dt);

	if(ds->showHud) {
		updateHud(ds, ad, ws);
	}

	//

	if(*isRunning == false) {
		DebugSessionSettings settings = {};

		for(int i = 0; i < Panel_Type_Size; i++) {
			settings.panelHierarchy[i] = ds->panelHierarchy[i];
			settings.panels[i] = ds->panels[i];
		}

		settings.profilerPanelActiveSection = ds->profilerPanelInfo.activeSection;

		settings.drawGrid = ds->drawGrid;
		settings.drawGroupHandles = ds->drawGroupHandles;
		settings.drawParticleHandles = ds->drawParticleHandles;
		settings.drawSelection = ds->drawSelection;
		settings.drawManifold = ds->drawManifold;
		settings.drawBlockers = ds->drawBlockers;

		debugWriteSessionSettings(&settings);
	}

	if(ds->debugInterval.update(ds->dt, ds->debugTimer.stop())) {
		ds->debugTime = ds->debugInterval.resultTime * 1000;
	}
}