
void debugInit(DebugState* ds) {
	*ds = {};

	setMemory(true);
	defer { setMemory(false); };

	ds->recState.init(600, theMemory->pMemory);
	
	// ds->profiler.init(10000, 1000);
	ds->profiler.init(100, 10, 3);
	theSampler = &ds->profiler.sampler;

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

	ds->logger.init(getPString(Log_File), LOGGING_ENABLED);
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

	// {
	// 	dxDrawRect(rectTLDim(0,0,1000,1000), vec4(0,1));
	// 	dxDrawRect(rectTLDim(0,-60,1000,1000), vec4(1,1));

	// 	int fh = 12*1.3f;
	// 	TextSettings ts = {getFont("LiberationSans-Regular.ttf", fh), vec4(1.0f,1)};
	// 	TextSettings ts2 = {getFont("LiberationSans-Regular.ttf", fh), vec4(0.0f,1)};

	// 	drawText("this is some text!!!", vec2(20,-40), vec2i(-1,1), ts); 
	// 	drawText("this is some text!!!", vec2(20,-80), vec2i(-1,1), ts2); 
	// }

	{
		if(false)
		{
			Font* font = getFont("LiberationSans-Regular.ttf", 18);

			bool isSubPixel = font->isSubpixel;

			TextSettings ts = textSettings(font, vec4(1.0f, isSubPixel ? -1 : 1));
			TextSettings tsb = textSettings(font, vec4(0.0f, isSubPixel ? -1 : 1));

			Texture* t = &font->tex;
			Vec2 p = vec2(100,-100);
			Rect r0 = rectTLDim(p, vec2(180,40));
			Rect r1 = rectTLDim(p - vec2(0,50), vec2(180,40));
			dxDrawRect(r0, vec4(0,1));	
			dxDrawRect(r1, vec4(1,1));	

			if(true)
			{
				char* name = "DebugMsaa";
				char* nameDepthStencil = 0;
				// ID3D11RenderTargetView* rtv = name ? dxGetFrameBuffer(name)->renderTargetView : 0;
				ID3D11RenderTargetView* rtv = name ? dxGetFrameBuffer(name)->renderTargetViewSRGB : 0;
				ID3D11DepthStencilView* dsv = nameDepthStencil ? dxGetFrameBuffer(nameDepthStencil)->depthStencilView : 0;

				int count = (rtv == 0 && dsv == 0) ? 0 : 1;

				theGState->d3ddc->OMSetRenderTargets(count, &rtv, dsv);
			}

			// 0 0.7 -> a 0.5 -> 0.35

			if(isSubPixel)
				dxSetBlendState(Blend_State_Subpixel_Font);

			drawText("This is Some Text wVMK", vec2(110,-110), vec2i(-1,1), ts);
			drawText("This is Some Text wVMK", vec2(110,-160), vec2i(-1,1), tsb);
		}

		if(false)
		{
			// void drawText(char* text, Vec2 startPos, Vec2i align, int wrapWidth, TextSettings settings) {

			Font* font = getFont("LiberationSans-Regular.ttf", 18);
			// bool isSubpixel = font->isSubpixel;
			bool isSubpixel = font->isSubpixel;

			TextSettings ts = textSettings(font, vec4(1.0f , isSubpixel ? -1 : 1));
			TextSettings tsb = textSettings(font, vec4(0.0f, isSubpixel ? -1 : 1));
			// drawText("This is Some Text", vec2(100,-50), vec2i(-1,1), ts);
			// drawText("T", vec2(100,-50), vec2i(-1,1), ts);

			Texture* t = &font->tex;
			{
				Texture* t = &font->tex;
				dxDrawRect(rectTLDim(vec2(100,-40), vec2(t->dim)), vec4(0.1f,1));
				dxDrawRect(rectTLDim(vec2(100,-40), vec2(t->dim)), vec4(1.0f,1), t->view);
			}

			Vec2 p = vec2(100,-100);
			Rect r0 = rectTLDim(p, vec2(t->dim));
			Rect r1 = rectTLDim(p - vec2(0,50), vec2(t->dim));
			dxDrawRect(r0, vec4(0.0f,1));	
			dxDrawRect(r1, vec4(1,1));	

			if(isSubpixel)
				dxSetBlendState(Blend_State_Subpixel_Font);
			drawText("This is Some Text", vec2(110,-110), vec2i(-1,1), ts);
			drawText("This is Some Text", vec2(110,-160), vec2i(-1,1), tsb);
		}

		if(false)
		{
			{
				// dxBindFrameBuffer("FontTemp");

				Font* font = getFont("LiberationSans-Regular.ttf", 28);
				TextSettings ts = textSettings(font, vec4(1.0f ,  -1));
				TextSettings tsb = textSettings(font, vec4(0.0f , -1));

				Vec2 pos = vec2(110,-160);
				Rect r = rectTLDim(pos, vec2(150,40));
				dxDrawRect(r, vec4(0.0f,1));
				drawText("This is Some Text", pos, vec2i(-1,1), ts);

				r += vec2(0,-25);
				dxDrawRect(r, vec4(1.0f,1));
				drawText("This is Some Text", pos+vec2(0,-30), vec2i(-1,1), tsb);



				D3D11_BOX sourceRegion;
				sourceRegion.left = 0;
				sourceRegion.right = 500;
				sourceRegion.top = 0;
				sourceRegion.bottom = 500;
				sourceRegion.front = 0;
				sourceRegion.back = 1;

				FrameBuffer* fb = dxGetFrameBuffer("FontTemp");
				FrameBuffer* fbs = dxGetFrameBuffer("2dMsaa");

				dxBindFrameBuffer("FontTemp");

				// Rect cr = rectTLDim(0,0,200,300);
				Rect cr = theGState->screenRect;
				// cr.right -= 100;
				// cr.bottom += 300;
				dxDrawRect(cr, vec4(1,1), fbs->shaderResourceView);
				// dxDrawRect(cr, vec4(1,1), dxGetTexture("misc\\test.dds")->view);

				// dxCopyFrameBuffer("2dMsaa", "FontTemp");
				// dxCopyFrameBuffer("FontTemp", "2dMsaa");

				// gs->d3ddc->CopySubresourceRegion(fb->texture, 0, 10, 20, 0, fbs->texture, 0, &sourceRegion);
				// gs->d3ddc->CopySubresourceRegion(fbs->texture, 0, 0, 0, 0, fb->texture, 0, &sourceRegion);
			}
		}
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

