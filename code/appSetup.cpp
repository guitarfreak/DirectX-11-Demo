
void handleReload(SystemData* sd, WindowSettings* ws) {
	TIMER_BLOCK();

	SetWindowLongPtr(sd->windowHandle, GWLP_WNDPROC, (LONG_PTR)mainWindowCallBack);
	SetWindowLongPtr(sd->windowHandle, GWLP_USERDATA, (LONG_PTR)sd);

	#if USE_FIBERS
	DeleteFiber(sd->messageFiber);
	sd->messageFiber = CreateFiber(0, (PFIBER_START_ROUTINE)updateInput, sd);
	#endif

	theGState->screenRes = ws->currentRes;
	theGState->screenRect = getScreenRect(ws);

	if(HOTRELOAD_SHADERS) {
		dxLoadShaders();
	}

	// Bad news.
	for(int i = 0; i < arrayCount(theGState->fonts); i++) {
		for(int j = 0; j < arrayCount(theGState->fonts[0]); j++) {
			Font* font = &theGState->fonts[i][j];
			if(font->heightIndex != 0) {
				freeFont(font);
				getFont(font->file, font->heightIndex, font->boldFont ? font->boldFont->file : 0, font->italicFont ? font->italicFont->file : 0);

			} else break;
		}
	}
}

void updateTimers(DebugState* ds, GraphicsState* gs, int refreshRate, double* dt, double* time, int* frameCount, bool init) {
	ds->debugTimer.start();
	gs->timer.start();

	if(init) {
		ds->frameTimer.start();
		ds->dt = 1/(float)refreshRate;
		ds->clockStamp = __rdtsc();
		ds->clockConvertStat.begin();

	} else {
		ds->dt = ds->frameTimer.update();
		ds->time += ds->dt;

		ds->fpsTime += ds->dt;
		ds->fpsCounter++;
		if(ds->fpsTime >= 1) {
			ds->avgFps = 1 / (ds->fpsTime / (double)ds->fpsCounter);
			ds->fpsTime = 0;
			ds->fpsCounter = 0;
		}

		u64 stamp = __rdtsc() - ds->clockStamp;
		ds->clockStamp = __rdtsc();
		double clockToTime = (ds->dt*1000) / stamp;
		ds->clockConvertStat.update(clockToTime);
		ds->clockStampToTime = ds->clockConvertStat.getAvg();
	}

	*dt = ds->dt;
	*time = ds->time;
	*frameCount++;
}

void handleInput(DebugState* ds, Input* input, void* messageFiber, SystemData* sd) {
	TIMER_BLOCK();

	inputPrepare(ds->input);

	#if USE_FIBERS
	SwitchToFiber(messageFiber);
	#else
	updateInput(sd);
	#endif

	// Beware, changes to ad->input have no effect on the next frame, 
	// because we override it every time.
	*input = *ds->input;

	// Stop debug gui game interaction.
	{
		bool blockInput = false;
		bool blockMouse = false;

		if(ds->gui.someoneActive()) {
			blockInput = true;
			blockMouse = true;
		}

		if(ds->gui.hotId[Gui_Focus_MLeft] || ds->gui.hotId[Gui_Focus_MRight] ||
		   ds->gui.hotId[Gui_Focus_MMiddle], ds->gui.hotId[Gui_Focus_MWheel]) {
			blockMouse = true;
		}

		// if(ds->gui.hotId[Gui_Focus_ArrowKeys]) {
		// 	blockInput = true;
		// }

		Console* con = &ds->console;
		if(pointInRect(ds->input->mousePosNegative, con->consoleRect)) blockMouse = true;
		if(con->isActive) blockInput = true;

		if(blockMouse) {
			input->mousePos = vec2(-1,-1);
			input->mousePosNegative = vec2(-1,-1);
			input->mousePosScreen = vec2(-1,-1);
			input->mousePosNegativeScreen = vec2(-1,-1);
		}

		if(blockInput) {
			memset(input->keysPressed, 0, sizeof(input->keysPressed));
			memset(input->keysDown, 0, sizeof(input->keysDown));
		}
	}
}

void handleWindowEvents(Input* debugInput, Input* input, WindowSettings* ws, HWND windowHandle, bool maximized, bool init, bool* toggleFullscreen, bool* updateFrameBuffers, bool* isRunning) {
	if(mouseInClientArea(windowHandle)) updateCursorIcon(ws);

	if((*toggleFullscreen || input->keysPressed[KEYCODE_F11] || input->altEnter) && !maximized) {
		if(*toggleFullscreen) *toggleFullscreen = false;
		// bool goFullscreen = !ws->fullscreen;

 		// gs->swapChain->SetFullscreenState(goFullscreen, 0);
 		// ws->fullscreen = !ws->fullscreen;

		if(ws->fullscreen) setWindowMode(windowHandle, ws, WINDOW_MODE_WINDOWED);
		else setWindowMode(windowHandle, ws, WINDOW_MODE_FULLBORDERLESS);
	}

	if(debugInput->resize || init) {
		if(!windowIsMinimized(windowHandle)) {
			updateResolution(windowHandle, ws);
			*updateFrameBuffers = true;
		}
		debugInput->resize = false;
	}

	if(debugInput->closeWindow) *isRunning = false;
}

void handleMouseCapturing(Input* input, MouseEvents* events, bool mouseOverPanel, Gui* gui, POINT* lastMousePosition, HWND windowHandle, bool killedFocus) {

	if(mouseButtonPressedRight(gui, input) && !mouseOverPanel) {
		events->debugMouse = false;

	}
	if(mouseButtonReleasedRight(gui, input)) {
		events->debugMouse = true;
	}

	//

	if(killedFocus && events->captureMouse) {
		events->captureMouse = false;
		events->lostFocusWhileCaptured = true;
	}

	if(events->lostFocusWhileCaptured && windowHasFocus(windowHandle)) {
		if(input->mouseButtonPressed[0] && mouseInClientArea(windowHandle)) {
			events->captureMouse = true;
			input->mouseButtonPressed[0] = false;

			events->lostFocusWhileCaptured = false;
		}
	}

	if(input->keysPressed[KEYCODE_F3]) {
		events->debugMouseFixed = !events->debugMouseFixed;
		if(!events->debugMouseFixed) events->debugMouse = true;
	}

	if(events->debugMouseFixed) events->debugMouse = false;

	bool showMouse = events->debugMouse;

	if(!events->captureMouse) {
		if(!showMouse) {
			input->mouseButtonPressed[1] = false;
			events->captureMouse = true;

			GetCursorPos(lastMousePosition);
		}

	} else {
		if(showMouse) {
			events->captureMouse = false;

			if(lastMousePosition->x == 0 && lastMousePosition->y == 0) {
				int w,h;
				Vec2i wPos;
				getWindowProperties(windowHandle, &w, &h, 0, 0, &wPos.x, &wPos.y);
				lastMousePosition->x = wPos.x + w/2;
				lastMousePosition->y = wPos.y + h/2;
			}

			SetCursorPos(lastMousePosition->x, lastMousePosition->y);
		}
	}

	events->fpsMode = events->captureMouse && windowHasFocus(windowHandle);
	if(events->fpsMode) {
		int w,h;
		Vec2i wPos;
		getWindowProperties(windowHandle, &w, &h, 0, 0, &wPos.x, &wPos.y);

		// SetCursorPos(wPos.x + w/2, wPos.y + h/2);
		
		SetCursorPos(lastMousePosition->x, lastMousePosition->y);
		input->lastMousePos = getMousePos(windowHandle,false);

		showCursor(false);
	} else {
		showCursor(true);
	}
}

// This needs to be put somewhere else.
void saveAppSessionAndSettings(AppData* ad, WindowSettings* ws, SystemData* sd) {

	// Game settings.
	{
		GameSettings settings = {};
		settings.fullscreen = ws->fullscreen;
		settings.frameRateCap = ad->frameRateCap;
		settings.vsync = ws->vsync;
		settings.resolutionScale = ad->gSettings.resolutionScale;
		settings.volume = ad->audioState.masterVolume;
		settings.mouseSensitivity = ad->mouseSensitivity;
		settings.fieldOfView = ad->gSettings.fieldOfView;

		char* file = Game_Settings_File;
		if(fileExists(file)) {
			writeDataToFile((char*)&settings, sizeof(GameSettings), file);
		}
	}

	// Session data.
	{
		Rect windowRect = getWindowWindowRect(sd->windowHandle);
		if(ws->fullscreen) windowRect = ws->previousWindowRect;

		AppSessionSettings at = {};

		at.windowRect = windowRect;

		at.camPos = ad->camera->xf.trans;
		// at.camRot = ad->camera->camRot;

		at.save(App_Session_File);
	}
}