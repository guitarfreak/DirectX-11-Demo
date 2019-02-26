
void updateConsole(DebugState* ds, float dt, bool* isRunning) {
	// Not actually usefull right now. More of a proof of concept...

	Input* input = ds->input;
	Gui* gui = &ds->gui;

	Console* con = &ds->console;
	con->update(input, vec2(theGState->screenRes), ds->fontHeight, dt);

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

			resultString = fString("%i + %i = %i.", a, b, a+b);

		} else if(strCompare(comName, "addFloat")) {
			float a = strToFloat(args[0]);
			float b = strToFloat(args[1]);

			resultString = fString("%g + %g = %g.", a, b, a+b);

		} else if(strCompare(comName, "print")) {
			resultString = fString("\"%s\"", args[0]);

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

	gui->setHotAllMouseOver(con->consoleRect);
}

//

#define DEBUG_NOTE_LENGTH 50

void addDebugNote(char* string, float duration) {
	NotificationData* nd = &theDebugState->noteData;

	assert(strLen(string) < DEBUG_NOTE_LENGTH);
	if(nd->count >= arrayCount(nd->stack)) return;

	int count = nd->count;
	strClear(nd->stack[count]);
	nd->times[count] = duration;
	strCpy(nd->stack[count], string);
	nd->count++;
}

void updateNotifications(NotificationData* noteData, float fontHeight, float dt) {
	NotificationData* nd = &theDebugState->noteData;

	// Update notes.
	int deletionCount = 0;
	for(int i = 0; i < nd->count; i++) {
		nd->times[i] -= dt;
		if(nd->times[i] <= 0) {
			deletionCount++;
		}
	}

	// Delete expired notes.
	if(deletionCount > 0) {
		for(int i = 0; i < nd->count-deletionCount; i++) {
			nd->stack[i] = nd->stack[i+deletionCount];
			nd->times[i] = nd->times[i+deletionCount];
		}
		nd->count -= deletionCount;
	}

	// Draw notes.
	int fh = fontHeight*1.3f;
	TextSettings ts = {getFont("LiberationSans-Regular.ttf", fh), vec4(1.0f,0.4f,0.0f,1), 
	                   TEXTSHADOW_MODE_SHADOW, vec2(1,-1), fh*0.10f, vec4(0,1)};

	float y = -fh * 0.2f;
	for(int i = 0; i < nd->count; i++) {
		drawText(nd->stack[i], vec2(theGState->screenRes.w/2, y), vec2i(0,1), ts); 
		y -= fh;
	}
}

//

void addDebugInfo(char* string) {
	InfoData* id = &theDebugState->infoData;

	if(id->count >= arrayCount(id->stack)) return;
	id->stack[id->count++] = string;
}

void updateHud(DebugState* ds, AppData* ad, WindowSettings* ws) {
	int fontHeight = ds->fontHeight*1.1f;
	TextSettings ts = {getFont("consola.ttf", fontHeight), vec4(1.0f,0.4f,0.0f,1), 
	                   TEXTSHADOW_MODE_SHADOW, vec2(1,-1), fontHeight*0.10f, vec4(0,1)};

	// Vec2 tp = vec2(ad->wSettings.currentRes.x, 0) - fontHeight*0.3f;
	// Vec2i align = vec2i(1,1);

	float off = fontHeight*0.3f;
	Vec2 tp = vec2(0, 0) + vec2(off, -off);

	if(ds->showUI) {
		tp.y -= fontHeight*1.2f;
	}

	double msPerFrame = (1/(double)ws->refreshRate) * 1000;
	float cpuPercent = (float)(ds->cpuTime/msPerFrame)*100;
	float gpuPercent = (float)(ds->gpuTime/msPerFrame)*100;
	float debugPercent = (float)(ds->debugTime/msPerFrame)*100;

	addDebugInfo( fString("Fps: %i",              roundInt(ds->avgFps)));
	addDebugInfo( fString("Cpu time:   %.2fms (%.2f%%)", ds->cpuTime, cpuPercent ));
	addDebugInfo( fString("Gpu time:   %.2fms (%.2f%%)", ds->gpuTime, gpuPercent ));
	addDebugInfo( fString("Debug time: %.2fms (%.2f%%)", ds->debugTime, debugPercent ));

	addDebugInfo( fString("Cam pos: (%.2f,%.2f,%.2f)", PVEC3(theGState->activeCam.pos) ));
	addDebugInfo( fString("Cam dir: (%.2f,%.2f,%.2f)", PVEC3(theGState->activeCam.look) ));

	addDebugInfo( fString("Window size: (%i,%i)", PVEC2(theGState->screenRes) ));

	addDebugInfo( fString("BufferIndex: %i",      ds->profiler.timer->bufferIndex ));
	addDebugInfo( fString("LastBufferIndex: %i",  ds->profiler.timer->lastBufferIndex ));

	InfoData* id = &ds->infoData;
	for(int i = 0; i < id->count; i++) {
		drawText(fString("%s", id->stack[i]), tp, vec2i(-1,1), ts); 
		tp.y -= fontHeight;
	}
	id->count = 0;
}