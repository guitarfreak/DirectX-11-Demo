
void initDebugUI(DebugState* ds, bool init) {
	ds->panelInit = false;

	if(!fileExists(Gui_Session_File)) {
		DebugSessionSettings settings = {};

		float fh = ds->fontHeight;
		Rect sr = theGState->screenRect;

		Set_Init_Panel_Properties();

		settings.profilerPanelActiveSection = 0;

		for(int i = 0; i < Panel_Type_Size; i++) settings.panelHierarchy[i] = i;

		settings.drawGrid = true;
		settings.drawGroupHandles = true;
		settings.drawParticleHandles = true;

		debugWriteSessionSettings(&settings);
	}

	DebugSessionSettings settings;
	debugReadSessionSettings(&settings);

	for(int i = 0; i < Panel_Type_Size; i++) {
		ds->panelHierarchy[i] = settings.panelHierarchy[i];
		ds->panels[i] = settings.panels[i];
	}
	
	{
		IntrospectionPanelInfo* pi = &ds->introspectionPanelInfo;
		pi->scrollHeight = 0;
		pi->scrollValue = 0;
		pi->expansionArray = getPArray(ExpansionIndex, 1000);
		float widths[] = { 0.0f, 0.3, 0.7f, 1.0f };
		for(int i = 0; i < arrayCount(widths); i++) pi->widths[i] = widths[i];
	}

	{
		ProfilerPanelInfo* pi = &ds->profilerPanelInfo;
		pi->activeSection = settings.profilerPanelActiveSection;
	}

	{
		MapPanelInfo* pi = &ds->mapPanelInfo;
		*pi = {};
		pi->maps.init();
		refreshMaps(&pi->maps);
		pi->spawnEntityType = ET_Object;
	}

	{
		EntityPanelInfo* pi = &ds->entityPanelInfo;
		*pi = {};
		pi->filterEnabled = false;
		pi->activeSection = 1;
		pi->typeFilterIndex = -1;
		strCpy(pi->nameFilter, "");
	}

	ds->drawGrid = settings.drawGrid;
	ds->drawGroupHandles = settings.drawGroupHandles;
	ds->drawParticleHandles = settings.drawParticleHandles;
}

void updateDebugUI(DebugState* ds, AppData* ad, WindowSettings* ws) {

	if(ds->panelInit) initDebugUI(ds, true);

	ThreadQueue* threadQueue = theThreadQueue;
	int panelCount = Panel_Type_Size;

	Gui* gui = &ds->gui;
	Font* font = gui->sText.font;

	{
		// Panel selection row.
		defer {
			gui->zLevel = Panel_Type_Size;
			float textPad = font->height * 1.5f;
			Vec2 p = vec2(0,0);
			Rect r;
			float eh = font->height * 1.5f;

			float alpha = 0.7f;
			float iconListGap = eh * 0.5;
			Vec3 baseColor = vec3(0.05f,0.4f,0.5f);
			Vec4 cActive = hslToRgbf(baseColor,alpha);
			Vec4 cInactive = hslToRgbf(baseColor + vec3(0,-0.1f,-0.2f),alpha);

			TextBoxSettings tbsInactive = textBoxSettings(gui->sText2, boxSettings(cInactive));
			TextBoxSettings tbsActive   = textBoxSettings(gui->sText2, boxSettings(cActive));

			for(int i = 0; i < panelCount; i++) {
				bool active = ds->panels[i].isActive;

				char* text = panelStrings[i];
				float labelWidth = getTextDim(text, font).w + textPad;

				r = rectTLDim(p, vec2(labelWidth, eh)); p.x += labelWidth;

				TextBoxSettings* settings = active ? &tbsActive : &tbsInactive;
				if(gui->qPButton(r, text, settings)) { 

					// Set active exclusively.
					if(ds->input->keysDown[KEYCODE_CTRL]) {
						for(int j = 0; j < panelCount; j++) ds->panels[j].isActive = false;

						ds->panels[i].isActive = true;
						ds->panelActivityIndex = (i+1);

					} else {
						if(!ds->panels[i].isActive && ds->input->keysDown[KEYCODE_CTRL]) {
							for(int i = 0; i < panelCount; i++) ds->panels[i].isActive = false;
						}

						ds->panels[i].isActive = !ds->panels[i].isActive;

						if(ds->panels[i].isActive) ds->panelActivityIndex =  (i+1);
						else                       ds->panelActivityIndex = -(i+1);
					}
				}
			}

			// Icons.
			p.x += iconListGap;

			QLayout ql = qLayout(p, vec2(0,eh), vec2(0,0));
			// ql.row(eh, eh, eh);

			// TextBoxSettings tbsInactive = textBoxSettings(gui->sText2, boxSettings(cInactive));
			// TextBoxSettings tbsActive   = textBoxSettings(gui->sText2, boxSettings(cActive));
			TextBoxSettings tbs = textBoxSettings(gui->sText2, boxSettings(vec4(0,0,0,0)));

			bool* switches[] = {&ds->drawGrid, &ds->drawParticleHandles, &ds->drawGroupHandles};
			char* textures[] = {"icons\\grid.dds", "icons\\particles.dds", "icons\\groups.dds"};

			for(auto& it : switches) {
				bool* var = it;

				// @Hack.
				Rect r = ql.getRectW(eh);
				if(gui->qPButton(r, "", vec2i(0,0), &tbs)) *var = !(*var);

				Vec4 color = *var ? vec4(1,1) : vec4(0.5f,1);
				color += gui->colorMod();

				int i = &it - &switches[0];
				dxDrawRect(r, color, dxGetTexture(textures[i])->view);
			}

			// r = rectTLDim(p, vec2(eh, eh)); p.x += eh;
			// if(gui->qPButton(ql.getRectW(eh), "")) ds->drawGrid = !ds->drawGrid;
			// if(gui->qPButton(ql.getRectW(eh), "")) ds->drawParticleHandles = !ds->drawParticleHandles;
			// if(gui->qPButton(ql.getRectW(eh), "")) ds->drawGroupHandles = !ds->drawGroupHandles;
			// dxDrawRect(r, vec4(1,0,0,1));
			// dxDrawRect(r, vec4(0,1,0,1));
			// dxDrawRect(r, vec4(0,0,1,1));
		};

		{
			PanelData* panels = ds->panels;

			// Update Hierarchy if new panel selected.
			if(ds->panelActivityIndex != 0) {
				bool setActive = ds->panelActivityIndex > 0;
				int panelIndex = abs(ds->panelActivityIndex) - 1;

				// Find hierarchy index.
				int index = -1;
				for(int i = 0; i < panelCount; i++) {
					if(ds->panelHierarchy[i] == panelIndex) {
						index = i;
						break;
					}
				}

				bool ignore = false;
				if(( setActive && (index == panelCount - 1)) ||
				   (!setActive && (index == 0)))
				   ignore = true;

				if(!ignore) {
					if(setActive) {
						moveArray(ds->panelHierarchy + index, ds->panelHierarchy + index+1, int, panelCount-1-index);
						ds->panelHierarchy[panelCount-1] = panelIndex;
					} else {
						moveArray(ds->panelHierarchy + 1, ds->panelHierarchy + 0, int, index);
						ds->panelHierarchy[0] = panelIndex;
					}

					// gui->clearHot();
				}

				ds->panelActivityIndex = 0;
			}

			int activeCount = 0;
			for(int i = 0; i < panelCount; i++) {
				if(panels[i].isActive) activeCount++;
			}

			int maxUiElements = 10000;
			int startId = gui->id;

			int activeIndex = -1;

			for(int i = 0; i < panelCount; i++) {
				int panelIndex = ds->panelHierarchy[i];

				gui->zLevel = i;
				gui->id = startId + maxUiElements*panelIndex;

				char* panelTitle = fString("<b>%s<b>", panelStrings[panelIndex]);
				Rect* panelRect = &panels[panelIndex].r;
				Vec2i* panelAlign = &panels[panelIndex].align;
				bool* panelActive = &panels[panelIndex].isActive;

				if(*panelActive)
				{
					activeIndex++;

					float panelMargin = roundf(font->height*0.3f);
					float panelBorder = roundf(font->height*0.5f);

					{
						gui->setHotAllMouseOver(*panelRect, gui->zLevel);

						Rect clampRect = rectTLDim(0,0,gui->windowSettings->currentRes.w,gui->windowSettings->currentRes.h);
						GuiWindowSettings windowSettings = {panelBorder, panelBorder*3, vec2(font->height*10,font->height*2.5), 
							                                vec2(0,0), clampRect, true, true, true, false};
						guiWindow(gui, panelRect, windowSettings);

						if((*panelAlign) != vec2i(0,0)) {
							Rect sr = theGState->screenRect;
							*panelRect = rectAlignDim(rectAlign(sr, *panelAlign), *panelAlign, panelRect->dim());
						}
					}

					// @color
					// gui->sBox.color = cProfilerPanel;
					// gui->sTextBox.boxSettings.color = cProfilerPanel;

					bool onTop = (activeIndex == activeCount-1);

					if(onTop) pfDrawRect(*panelRect + vec2(1,-1)*font->height*0.3f, vec4(0,0.7f));

					// Draw panel background.
					{
						BoxSettings bs = gui->sBox;
						if(onTop) bs.borderColor.rgb = hslToRgbf(0.6,0.5f,0.5f);

						gui->qBox(*panelRect, &bs);
					}

					gui->scissorPush(*panelRect);
					defer { gui->scissorPop(); };

					//

					PanelSettings pd = {};
					{
						float fontHeight = gui->sText.font->height;

						pd.pri = panelRect->expand(-vec2(panelBorder*2));
						pd.p = pd.pri.tl();
						pd.dim = vec2(pd.pri.w(), roundf(fontHeight * 1.5f));
						pd.pad = vec2(panelMargin-1, panelMargin-1);
						pd.textPad = roundf(fontHeight * 1.5f);

						pd.headerHeight = roundf(pd.dim.h * 1.0f);
						pd.separatorHeight = roundf(fontHeight * 0.3f);

						Vec4 bcLinear = gammaToLinear(gui->sBox.color);
						pd.cSeparatorDark = linearToGamma(bcLinear - vec4(0.1f,0));
						pd.cSeparatorBright = linearToGamma(bcLinear + vec4(0.1f,0));
					}

					// Header.
					{
						Vec4 headerColor = hslToRgbf(0.55f,0.3f,0.3f,1);

						TextSettings headerTextSettings = textSettings(gui->sText.font, gui->sText.color, TEXTSHADOW_MODE_SHADOW, 1.0f, gui->sBox.borderColor);
						TextBoxSettings headerSettings = textBoxSettings(headerTextSettings, boxSettings());

						BoxSettings bs = boxSettings(headerColor);
						Rect hr = rectTLDim(panelRect->tl() + vec2(1,-1), vec2(panelRect->w()-2, -2 + pd.headerHeight + panelBorder*2));
						hr = round(hr);

						{
							gui->qBox(hr, &bs);
							pfDrawLineH(hr.bl(), hr.br(), pd.cSeparatorDark);
						}

						{
							Rect r = rectTLDim(pd.p, vec2(pd.dim.w, pd.headerHeight));

							gui->qTextBox(r, panelTitle, vec2i(0,0), &headerSettings);

							// Close button.
							{
								TextBoxSettings tbs = gui->sTextBox; 
								tbs.boxSettings = bs;
								Rect rClose = r.rSetL(r.h());
								if(gui->qButton(rClose, "", &tbs)) {
									*panelActive = false;
									ds->panelActivityIndex = -(panelIndex+1);
								}

								Vec4 c = gui->sText.color;
								rClose = round(rClose);
								rClose = rectCenDim(rClose.c(), vec2(font->height * 0.5f));
								dxDrawCross(rClose.c(), rClose.w(), rClose.w()*0.3f, c);
							}
						}

						// pd.p.y -= panelBorder*2;
						// pd.p.y -= panelBorder*2;

						pd.p.y -= hr.h();
					}

					switch(panelIndex) {
						Select_Panel_Function();
					}

					if(ds->panelActivityIndex == 0) {
						if(gui->gotActiveId) {
							ds->panelActivityIndex = panelIndex+1;
						}
					}

				}
			}

			gui->id = startId + panelCount*maxUiElements;
			gui->zLevel = 0;
		}
	}

	gui->popupSetup();

	ds->mouseOverPanel = false;
	for(int i = 0; i < Panel_Type_Size; i++) {
		PanelData* pd = ds->panels + i;
		if(!pd->isActive) continue;
		if(pointInRect(ds->input->mousePosNegative, pd->r)) {
			ds->mouseOverPanel = true;
			break;
		}
	}
}

template <class T> void drawProbabilityGraph(Gui* gui, Rect r, ValueRange<T>* vr);

void profilerPanel(DebugState* ds, Gui* gui, PanelSettings pd) {
	ProfilerPanelInfo* pInfo = &ds->profilerPanelInfo;

	ThreadQueue* threadQueue = theThreadQueue;
	Font* font = gui->sText.font;

	Rect pri = pd.pri;
	Vec2 p   = pd.p;
	float eh = pd.dim.h;
	float ew = pd.dim.w;
	Vec2 pad = pd.pad;
	Rect r;
	QuickRow qr;

	{
		TextBoxSettings tbs = textBoxSettings(gui->sText, boxSettings(gui->sBox.color));
		TextBoxSettings tbsActive = tbs;
		tbsActive.boxSettings.borderColor = gui->sBox.borderColor;

		char* labels[] = {"Stats", "Graph"};
		for(int i = 0; i < arrayCount(labels); i++) {
			bool active = pInfo->activeSection == i;

			char* text = labels[i];
			float labelWidth = getTextDim(text, font).w + pd.textPad;

			r = rectTLDim(p, vec2(labelWidth, eh)); p.x += labelWidth;

			if(active) gui->setInactive(false);

			TextBoxSettings* settings = active ? &tbsActive : &tbs;
			if(gui->qPButton(r, text, settings)) pInfo->activeSection = i;
		}

		{
			char* label = fString("%.2fms", ds->debugTime);
			r = rectTLDim(p, vec2(pri.right - p.x,eh));
			qr = quickRow(r, pad.x, 0, getTextDim(label, font).w + pd.textPad/2, 1.01f);

			qr.next();
			gui->qText(qr.next(), label, vec2i(1,0));
			qr.next();
		}

		p.y -= eh+pad.y;
		p.x = pri.left;

		{
			Profiler* prof = &ds->profiler;

			char* labels[] = {"Pause", "Resume"};
			r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
			qr = quickRow(r, pad.x, getTextDim(labels[1], font).w + pd.textPad, 0);

			if(gui->qButton(qr.next(), prof->noCollating ? labels[1] : labels[0])) {
				prof->noCollating = !prof->noCollating;

				if(prof->noCollating) {
					pInfo->graphCam.init = false;
					prof->setPause();

				} else prof->setPlay();
			}

			gui->qSlider(qr.next(), &prof->frameIndex, 0, FRAME_BUFFER_SIZE-1);
		}

		{
			p.y -= pd.separatorHeight/2;
			pfDrawLineH(p+vec2(0,0.5f), p+vec2(ew,0)+vec2(0,0.5f), pd.cSeparatorDark);
			pfDrawLineH(p+vec2(0,-0.5f), p+vec2(ew,0)+vec2(0,-0.5f), pd.cSeparatorBright);
			p.y -= pd.separatorHeight/2+pad.y;
		}
	}

	//

	Profiler* prof = &ds->profiler;
	ProfilerTimer* timer = prof->timer;
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

	switch(pInfo->activeSection) {

	// @Stats
	case 0: {
		static float scrollHeight = 0;
		float yOffset = 0;
		float wOffset = 0;
		{
			static float scrollValue = 0;
			Rect pr = pri.setT(p.y);

			gui->sScroll.scrollBarPadding = pad.x;
			gui->sScroll.scrollAmount = eh + pad.y;
			gui->qScroll(pr, scrollHeight, &scrollValue, &yOffset, &wOffset);
		}

		QLayout ql = qLayout(p + vec2(0,yOffset), vec2(ew - wOffset,eh), pad);
		float startY = ql.pos.y;

		defer { gui->qScrollEnd(); };
		defer { scrollHeight = startY - ql.pos.y - pad.y; };

		int barWidth = 1;
		int barCount = arrayCount(prof->timings);

		float cols[] = {0,0,0,0,0,0,0,0, barWidth*barCount};
		char* labels[] = {"File", "Function", "Description", "Cycles", "Hits", "C/H", "Avg. Cycl.", "Total Time", "Graphs"};

		qr = ql.row(cols, arrayCount(cols));

		for(int i = 0; i < arrayCount(cols); i++) {
			TextBoxSettings tbs = gui->sTextBox; 
			tbs.boxSettings.borderColor = vec4(0);
			tbs.textSettings = gui->sText2;
			if(gui->qButton(qr.next(), fString("<b>%s<b>", labels[i]), &tbs)) {
				if(abs(pInfo->statsSortingIndex) == i) pInfo->statsSortingIndex *= -1;
				else pInfo->statsSortingIndex = i;
			}
		}

		int* sortIndices = getTArray(int, infoCount);
		{
			int count = infoCount;
			for(int i = 0; i < count; i++) sortIndices[i] = i;

			int si = abs(pInfo->statsSortingIndex);

			if(between(si, 0, 2)) {
				char** data = getTArray(char*, count);

				TimerInfo* timerInfos = timer->timerInfos;
				for(int i = 0; i < count; i++) {
					TimerInfo* tInfo = timerInfos + i;
					if(!tInfo->initialised) data[i] = getTString("");
					else {
						     if(si == 0) data[i] = getTString(timerInfos[i].file);						
						else if(si == 1) data[i] = getTString(timerInfos[i].function);						
						else if(si == 2) data[i] = getTString(timerInfos[i].name);						
					}
				}

				auto cmp = [](void* a, void* b) { return sortFuncString(VDref(SortPair<char*>, a).key, VDref(SortPair<char*>, b).key); };
				bubbleSort(data, sortIndices, count, cmp);

			} else if(between(si, 3, 7)) {
				double* data = getTArray(double, count);

				     if(si == 3) for(int i = 0; i < count; i++) data[i] = timings[i].cycles;
				else if(si == 4) for(int i = 0; i < count; i++) data[i] = timings[i].hits;
				else if(si == 5) for(int i = 0; i < count; i++) data[i] = timings[i].cyclesOverHits;
				else if(si == 6) for(int i = 0; i < count; i++) data[i] = statistics[i].avg;
				else if(si == 7) for(int i = 0; i < count; i++) data[i] = timings[i].cycles/cyclesPerFrame;

				auto cmp = [](void* a, void* b) { return VDref(SortPair<double>, a).key < VDref(SortPair<double>, b).key; };
				bubbleSort(data, sortIndices, count, cmp);
	   	}

	   	bool sortDirection = pInfo->statsSortingIndex < 0 ? false : true;
	   	if(sortDirection) {
	   		for(int i = 0; i < count/2; i++) swap(sortIndices + i, sortIndices + (count-1-i));
	   	}
		}

		for(int index = 0; index < infoCount; index++) {
			int i = sortIndices[index];

			TimerInfo* tInfo = timer->timerInfos + i;
			Timings* timing = timings + i;

			if(!tInfo->initialised) continue;

			ql.pad.y = 0;
			qr = ql.row(cols, arrayCount(cols));

			gui->qText(qr.next(), fString("%s", tInfo->file + 21), vec2i(-1,0));

			TextBoxSettings tbs = gui->sTextBox; tbs.boxSettings.borderColor = vec4(0);
			if(gui->qButton(qr.next(), fString("%s", tInfo->function), vec2i(-1,0), &tbs)) {
				char* command = fString("%s %s:%i", Editor_Executable_Path, tInfo->file, tInfo->line);
				shellExecuteNoWindow(command);
			}
			gui->qText(qr.next(), fString("%s", tInfo->name), vec2i(-1,0));
			gui->qText(qr.next(), fString("%'I64dc", timing->cycles), vec2i(-1,0));
			gui->qText(qr.next(), fString("%'I64d", timing->hits), vec2i(-1,0));
			gui->qText(qr.next(), fString("%'I64dc", timing->cyclesOverHits), vec2i(-1,0));
			gui->qText(qr.next(), fString("%'I64dc", (i64)statistics[i].avg), vec2i(-1,0)); // Not a i64 but whatever.
			gui->qText(qr.next(), fString("%.3f%%", ((float)timing->cycles/cyclesPerFrame)*100), vec2i(-1,0));

			// Bar graph.

			// dcState(STATE_LINEWIDTH, barWidth);

			Rect r = qr.next();
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
				xOffset += barWidth;

				dxPushLine(rmin, rmin + vec2(0,height), vec4(colorOffset,1-colorOffset,0,1));
				dxFlushIfFull();
			}

			dxEndPrimitive();
		}

	} break;

	// @Graph
	case 1: {
		static float scrollHeight = 0;
		float yOffset = 0;
		float wOffset = 0;
		{
			static float scrollValue = 0;
			Rect pr = pri.setT(p.y);

			gui->sScroll.scrollBarPadding = pad.x;
			gui->sScroll.scrollAmount = eh + pad.y;
			gui->qScroll(pr, scrollHeight, &scrollValue, &yOffset, &wOffset);
		}

		QLayout ql = qLayout(p + vec2(0,yOffset), vec2(ew - wOffset,eh), pad);
		float startY = ql.pos.y;

		defer { gui->qScrollEnd(); };
		defer { scrollHeight = startY - ql.pos.y - pad.y; };

		Vec4 bgColor = vec4(0,0.15f);
		Vec4 cThreadLine = vec4(0,0.5f);
		Vec4 cSwapBoundary = vec4(0,0.5f);
		Vec4 cMinimap = vec4(0,0.25f);
		TextBoxSettings slotSettings = gui->sTextBox2;
		slotSettings.textSettings.cull = true;

		//

		// Rect cyclesRect = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
		Rect headerRect = ql.getRect();

		float lineHeightOffset = 1.2;
		float lineHeight = ds->fontHeight * lineHeightOffset;

		int stackCountMain = 6;
		int stackCountThreads = 2;

		ql.dim.h = stackCountMain*lineHeight + stackCountThreads*lineHeight*(threadQueue->threadCount);
		Rect bgRect = ql.getRect();
		ql.dim.h = eh;

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

		GraphCam* cam = &pInfo->graphCam;
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
			int dragEvent = gui->goDragAction(pfGetRectScissor(gui->scissor, bgRect));	
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
				if(gui->goButtonAction(bgRect, Gui_Focus_MWheel)) scale = true;
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
			// while(timelineSection < cam->w*divMod*(gui->windowSettings->currentRes.w/(bgRect.w()))) {
			while(timelineSection < cam->w*divMod*(650/(bgRect.w()))) {
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
					dxDrawLine(vec2(p,headerRect.bottom), vec2(p,headerRect.bottom + h), lineColor);
				}

				// // Text
				// {
				// 	Vec2 textPos = vec2(p,headerRect.bottom + cyclesDim.h/2);
				// 	float percent = mapRange(pos, cyclesLeft, cyclesRight, 0.0, 100.0);
				// 	int percentInterval = mapRange(timelineSection, 0.0, cyclesSize, 0.0, 100.0);

				// 	char* s;
				// 	if(percentInterval > 10) s = fString("%i%%", (int)percent);
				// 	else if(percentInterval > 1) s = fString("%.1f%%", percent);
				// 	else if(percentInterval > 0.1) s = fString("%.2f%%", percent);
				// 	else s = fString("%.3f%%", percent);

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
					drawText(fString("%$_.1fc", cycles), textPos, vec2i(0,0), gui->sText2);
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

		BoxSettings bs = gui->sBox;
		bs.color = bgColor;
		bgRect.bottom += 1;
		gui->qBox(round(bgRect), &bs);

		Vec2 pos = bgRect.tl();
		for(int threadIndex = 0; threadIndex < threadQueue->threadCount+1; threadIndex++) {

			// Horizontal lines to distinguish thread bars.
			if(threadIndex > 0) {
				dxDrawLineH(pos, vec2(bgRect.right, pos.y), cThreadLine);
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
					dxDrawLineV(vec2(right, bgRect.bottom), vec2(right, bgRect.top), cSwapBoundary);
				}

				float y = pos.y - (slot->stackIndex*lineHeight);
				Rect r = rect(left, y - lineHeight, right, y);

				bool visible = right - left >= 1;
				if(visible) {
					TimerInfo* tInfo = timer->timerInfos + slot->timerIndex;

					Vec4 color = vec4(tInfo->color, 1);
					char* text = fString("%s %s (%$_.1ic)", tInfo->function, tInfo->name, slot->size);
					r.left = max(r.left, bgRect.left);

					if(pointInRect(gui->input->mousePosNegative, r)) {
						mouseHighlight = true;
						hRect = r;
						hc = color;
						hText = text;

					} else {
						slotSettings.boxSettings.color = color;
						gui->qTextBox(r, text, vec2i(-1,0), &slotSettings);
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
			gui->qTextBox(hRect, hText, vec2i(-1,0), &slotSettings);
		}

	} break;

	}
}

void inputRecordingPanel(DebugState* ds, Gui* gui, PanelSettings pd) {
	ThreadQueue* threadQueue = theThreadQueue;
	Font* font = gui->sText.font;

	Rect pri = pd.pri;
	Vec2 p   = pd.p;
	float eh = pd.dim.h;
	float ew = pd.dim.w;
	Vec2 pad = pd.pad;
	Rect r;
	QuickRow qr;

	//

	StateRecording* rec = &ds->recState;

	static float scrollHeight = 0;
	float yOffset = 0;
	float wOffset = 0;
	{
		static float scrollValue = 0;
		Rect pr = pri.setT(p.y);

		gui->sScroll.scrollBarPadding = pad.x;
		gui->sScroll.scrollAmount = eh + pad.y;
		gui->qScroll(pr, scrollHeight, &scrollValue, &yOffset, &wOffset);
	}

	QLayout ql = qLayout(p + vec2(0,yOffset), vec2(ew - wOffset,eh), pad);
	float startY = ql.pos.y;

	defer { gui->qScrollEnd(); };
	defer { scrollHeight = startY - ql.pos.y - pad.y; };

	// rec, stop rec, play, not play, pause, resume, step, set breakpoint

	gui->qText(ql.getRect(), fString("Active Threads: %b.", !threadQueueFinished(threadQueue)), vec2i(-1,0));
	gui->qText(ql.getRect(), fString("Recorded Frames: %i of %i.", rec->recordIndex, rec->capacity), vec2i(-1,0));

	// rec, stop, pause, play
	{
		ql.dim.h = eh*1.5f;

		float h = ql.dim.h;
		float cols[] = {0, h,h,h,h,h, 0};
		qr = ql.row(cols, arrayCount(cols));

		bool recordInactive = rec->state == REC_STATE_PLAYING;
		bool stopInactive = rec->state == REC_STATE_INACTIVE;
		bool pausePlayInactive = rec->state == REC_STATE_RECORDING || rec->recordIndex == 0;

		//

		qr.next();

		Vec4 c = gui->sEdit.textBoxSettings.boxSettings.color;
		Vec4 recColor = c;
		if(rec->state == REC_STATE_RECORDING) recColor = vec4(0.5f,0,0,1);

		if(recordInactive) gui->setInactiveX = true;
		r = round(qr.next());
		bool record = gui->qButton(r, "", vec2i(0,0));
		r = round(r.expand(-h*0.5f));
		dxDrawRect(r, recColor, dxGetTexture("misc\\circle.dds")->view);

		if(stopInactive) gui->setInactiveX = true;
		r = round(qr.next());
		bool stop = gui->qButton(r, "", vec2i(0,0));
		r = round(r.expand(-h*0.5f));
		dxDrawRect(r, c);

		if(pausePlayInactive) gui->setInactiveX = true;
		r = round(qr.next());
		bool pausePlay = gui->qButton(r, "", vec2i(0,0));
		r = round(r.expand(-h*0.5f));
		if(rec->state == REC_STATE_PLAYING && !rec->playbackPaused) {
			dxDrawRect(round(r.rSetR(r.w()/3)), c);
			dxDrawRect(round(r.rSetL(r.w()/3)), c);

		} else {
			dxDrawTriangle(r.bl(), r.tl(), r.r(), c);
		}

		if(pausePlayInactive) gui->setInactiveX = true;
		r = round(qr.next());
		bool step = gui->qButton(r, "", vec2i(0,0));
		r = round(r.expand(-h*0.5f));
		dxDrawTriangle(r.bl(), r.tl(), r.c(), c);
		dxDrawTriangle(r.b(), r.t(), r.r(), c);

		if(pausePlayInactive) gui->setInactiveX = true;
		gui->qCheckBox(qr.next(), &rec->activeBreakPoint);

		qr.next();

		//

		ql.dim.h = eh;
		if(rec->recordIndex > 0 && rec->state != REC_STATE_RECORDING) {
			r = ql.getRect();
			int playbackIndex = rec->playbackIndex;
			if(gui->qSlider(r, &playbackIndex, 0, rec->recordIndex - 1)) {
				rec->breakPointIndex = playbackIndex;
				rec->activeBreakPoint = true;
			}

			SliderSettings ss = gui->sSlider;
			ss.textBoxSettings.textSettings.color = vec4(0,0);
			ss.textBoxSettings.boxSettings.color = vec4(0,0);
			ss.textBoxSettings.boxSettings.borderColor = vec4(0,0);
			ss.color = vec4(0,1);
			ss.borderColor = vec4(0,0);

			if(rec->activeBreakPoint) {
				gui->setInactiveX = true;
				gui->qSlider(r, &rec->breakPointIndex, 0, rec->recordIndex - 1, &ss);
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
}

void introspectionPanel(AppData* ad, DebugState* ds, Gui* gui, PanelSettings pd) {
	IntrospectionPanelInfo* pi = &ds->introspectionPanelInfo;

	Font* font = gui->sText.font;

	Rect pri = pd.pri;
	Vec2 p   = pd.p;
	float eh = pd.dim.h;
	float ew = pd.dim.w;
	Vec2 pad = pd.pad;
	Rect r;
	QuickRow qr;

	//

	eh = roundInt(font->height*1.2f);
	pad.y = roundInt(font->height*0.2f);

	Vec4 cGrid = gui->sEdit.textBoxSettings.boxSettings.color;

	float buttonWidth = eh;
	int padding = eh;
	float seperatorWidth     = font->height*0.3f*3;
	float pSep               = seperatorWidth/ew;
	float memberSectionWidth = 0.3f;
	float valueSectionWidth  = 0.0f;
	float typeSectionWidth = 0;
	for(int i = 0; i < arrayCount(structInfos); i++) {
		StructInfo* info = structInfos + i;
		typeSectionWidth = max(typeSectionWidth, getTextDim(info->name, font).w);
	}
	typeSectionWidth += seperatorWidth*2;

	float currentOffset = 0;

	//

	Rect rHeader = rectTLDim(p, vec2(ew, eh*1.3f)); 
	p.y -= eh*1.3f;

	//

	static float wOffset = 0;

	Rect* rects;

	// Table header.
	{
		Rect r = rHeader;
		r.right -= wOffset;
		
		float cols[] = {pi->widths[1]-pSep/2, seperatorWidth, pi->widths[2]-pi->widths[1]-pSep, seperatorWidth, pi->widths[3]-pi->widths[2]-pSep/2};
		QuickRow qr = quickRow(r, 0, cols, arrayCount(cols));

		Rect rSep[2]; qr.next(); rSep[0] = qr.next(); qr.next(); rSep[1] = qr.next();

		for(int i = 0; i < 2; i++) {
			int event = gui->goDragAction(rSep[i], Gui_Focus_MLeft);
			if(gui->isHot() || event) gui->setCursor(IDC_SIZEWE);
			if(event == 1) gui->mouseAnchor.x = gui->input->mousePos.x - rSep[i].cx();
			if(event > 0) {
				float x =  gui->input->mousePos.x - r.left - gui->mouseAnchor.x;
				float p = x / r.w();
				pi->widths[i+1] = p;

				int id = i+1;
				pi->widths[id] = clamp(pi->widths[id], pi->widths[id-1]+pSep, pi->widths[id+1]-pSep);
			}
		}

		{
			float cols[] = {pi->widths[1]-pSep/2, seperatorWidth, pi->widths[2]-pi->widths[1]-pSep, seperatorWidth, pi->widths[3]-pi->widths[2]-pSep/2};
			qr = quickRow(r, 0, cols, arrayCount(cols));

			Rect rs[] = { qr.next(), qr.next(), qr.next(), qr.next(), qr.next() };
			rects = qr.rects;

			pfDrawRect(round(rect(rs[0].min, rs[4].max)), hslToRgbf(0.55f,0.3f,0.3f,1));

			char* labels[] = {"Members", "Values", "Types"};
			for(int i = 0; i < arrayCount(rs); i++) {
				if(i % 2 == 0) {
					rs[i].left += pad.x;
					gui->qText(rs[i], labels[i/2], vec2i(-1,0));
				} else {
					pfDrawLineV(rs[i].t(), rs[i].b(), cGrid);
				}
			}
		}
	}

	//

	float yOffset = 0;
	Rect pr = pri.setT(p.y);
	{
		gui->sScroll.scrollBarPadding = pad.x;
		gui->sScroll.scrollAmount = eh + pad.y;
		gui->qScroll(pr, pi->scrollHeight, &pi->scrollValue, &yOffset, &wOffset);
	}

	p.y += yOffset;
	ew -= wOffset;
	float startY = p.y;

	defer { gui->qScrollEnd(); };
	defer { pi->scrollHeight = startY - p.y - pad.y; };

	Vec2 startPos = p;

	//

	TypeTraverse tt;
	tt.start(getType(AppData), ad, "AppData");

	while(true) {
		Rect r;
		QuickRow qr;
		r = rectTLDim(p, vec2(ew, eh)); 
		float cols[] = {pi->widths[1]-pSep/2, seperatorWidth, pi->widths[2]-pi->widths[1]-pSep, seperatorWidth, pi->widths[3]-pi->widths[2]-pSep/2};
		qr = quickRow(r, 0, cols, arrayCount(cols));

		Rect rSeperators[2];
		Rect rMember   = qr.next();
		rSeperators[0] = qr.next();
		Rect rValue    = qr.next();
		rSeperators[1] = qr.next();
		Rect rType     = qr.next();

		if(tt.valid()) {
			tt.get();

			p.y -= eh+pad.y;

			if(tt.type == SData_ATOM) {
				r = rMember;
				r = r.addL(currentOffset + buttonWidth + pad.x);
				gui->qText(r,      tt.memberName(), vec2i(-1,0));
				gui->qText(rValue, typeToStr(tt.memberType, tt.data), vec2i(-1,0));
				gui->qText(rType,  tt.typeName(), vec2i(-1,0));

				tt.next();

			} else {
				char* memberLabel = tt.memberName();
				char* typeLabel = tt.typeName();

				r = rMember;
				r = r.addL(currentOffset);
				Rect rButton = r.rSetR(buttonWidth);
				r = r.addL(buttonWidth + pad.x);

				bool expanded = false;
				// bool expanded = true;
				{
					gui->scissorPush(rMember);
					defer { gui->scissorPop(); };
					
					ExpansionIndex currentIndex = {};
					for(int i = 0; i < tt.stackCount; i++) {
						currentIndex.indices[currentIndex.count++] = tt.stack[i].memberIndex;
					}

					// Check if current index is expanded.
					int indexIndex = -1; // Yes.
					for(int i = 0; i < pi->expansionCount; i++) {
						if(currentIndex == pi->expansionArray[i]) {
							expanded = true;
							indexIndex = i;
							break;
						}
					}

					{
						TextBoxSettings tbs = gui->sTextBox; 
						tbs.boxSettings.borderColor = vec4(0);
						if(gui->qButton(rButton, "", &tbs)) {
							if(!expanded) {
								pi->expansionArray[pi->expansionCount++] = currentIndex;

							} else {
								pi->expansionArray[indexIndex] = pi->expansionArray[--pi->expansionCount];
							}
						}

						dxDrawTriangle(rButton.c(), rButton.w()/4, gui->sText.color, expanded ? vec2(0,-1) : vec2(1,0));
					}
				}

				gui->qText(r, memberLabel, vec2i(-1,0));
				
				char* string = memberToStr(tt.mInfo, tt.e->structBase, 0, "", 0, 0, rValue.w(), font);
				gui->qText(rValue, getTString(string), vec2i(-1,0));

				gui->qText(rType, typeLabel, vec2i(-1,0));

				if(expanded) {
					currentOffset += padding;
					tt.push();
				} else {
					tt.next();
				}
			}

		} else {
			if(!tt.pop()) break;
			tt.next();

			currentOffset -= padding;
		}
	}

	{
		{
			dxBeginPrimitiveColored(cGrid, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
			Vec2 pp = startPos;
			while(pp.y > p.y) {
				float y = round(pp.y)-0.5f;
				dxPushVertex(pVertex(vec2(pp.x, y))); 
				dxPushVertex(pVertex(vec2(pp.x + ew, y)));
				dxFlushIfFull();

				pp.y -= eh+pad.y;
			}
			dxEndPrimitive();
		}

		Rect r = round(rectTLDim(pr.tl(), vec2(ew, pr.top - p.y - 1)));
		if(r.h() > pr.h()) r = r.rSetB(pr.h());
		pfDrawRectOutline(r, cGrid);
		pfDrawLineV(vec2(rects[1].cx(), r.top), vec2(rects[1].cx(), r.bottom), cGrid);
		pfDrawLineV(vec2(rects[3].cx(), r.top), vec2(rects[3].cx(), r.bottom), cGrid);
	}
}

void animationPanel(AppData* ad, DebugState* ds, Gui* gui, PanelSettings pd) {
	ThreadQueue* threadQueue = theThreadQueue;
	Font* font = gui->sText.font;

	Rect pri = pd.pri;
	Vec2 p   = pd.p;
	float eh = pd.dim.h;
	float ew = pd.dim.w;
	Vec2 pad = pd.pad;
	Rect r;
	QuickRow qr;

	//

	Mesh* mesh = ad->figureMesh;
	AnimationPlayer* player = &mesh->animPlayer;

	if(!mesh) return;

	static float scrollHeight = 0;
	float yOffset = 0;
	float wOffset = 0;
	{
		static float scrollValue = 0;
		Rect pr = pri.setT(p.y);

		gui->sScroll.scrollBarPadding = pad.x;
		gui->sScroll.scrollAmount = eh + pad.y;
		gui->qScroll(pr, scrollHeight, &scrollValue, &yOffset, &wOffset);
	}

	QLayout ql = qLayout(p + vec2(0,yOffset), vec2(ew - wOffset,eh), pad);
	float startY = ql.pos.y;

	defer { gui->qScrollEnd(); };
	defer { scrollHeight = startY - ql.pos.y - pad.y; };

	int li = 0;
	char* labels[] = { "Animation", "Speed", "ShowSkeleton", "NoLocomotion", "NoInterp" };
	float labelMaxWidth = getTextMaxWidth(labels, arrayCount(labels), font) + pad.x;

	qr = ql.row(labelMaxWidth, 0.0f);
	gui->qText(qr.next(), labels[li++], vec2i(-1,0));
	auto getName = [](void* a) { return ((Animation*)a)->name; };
	if(gui->qComboBox(qr.next(), &ad->figureAnimation, mesh->animations, sizeof(mesh->animations[0]), mesh->animationCount, getName)) {
		ad->figureMesh->animPlayer.time = 0;
	};

	qr = ql.row(labelMaxWidth, 0.0f);
	gui->qText(qr.next(), labels[li++], vec2i(-1,0));
	gui->qSlider(qr.next(), &ad->figureSpeed, 0, 2);

	qr = ql.row(labelMaxWidth, 0.0f);
	gui->qText(qr.next(), labels[li++], vec2i(-1,0));
	gui->qCheckBox(qr.next(), &ad->showSkeleton);

	qr = ql.row(labelMaxWidth, 0.0f);
	gui->qText(qr.next(), labels[li++], vec2i(-1,0));
	gui->qCheckBox(qr.next(), &ad->figureMesh->animPlayer.noLocomotion);

	qr = ql.row(labelMaxWidth, 0.0f);
	gui->qText(qr.next(), labels[li++], vec2i(-1,0));
	gui->qCheckBox(qr.next(), &ad->figureMesh->animPlayer.noInterp);
}

void mapPanel(AppData* ad, DebugState* ds, Gui* gui, PanelSettings pd) {
	MapPanelInfo* pi = &ds->mapPanelInfo;

	static float scrollHeight = 0;
	float yOffset, wOffset;
	{
		static float scrollValue = 0;
		Rect pr = pd.pri.setT(pd.p.y);

		gui->sScroll.scrollBarPadding = pd.pad.x;
		gui->sScroll.scrollAmount = pd.dim.h + pd.pad.y;
		gui->qScroll(pr, scrollHeight, &scrollValue, &yOffset, &wOffset);
	}

	QLayout ql = qLayout(pd.p + vec2(0,yOffset), vec2(pd.dim.w - wOffset,pd.dim.h), pd.pad, pd.textPad, gui->sText.font);

	float startY = ql.pos.y;
	defer { gui->qScrollEnd(); };
	defer { scrollHeight = startY - ql.pos.y - pd.pad.y; };

	//

	{
		EntityUI* eui = &ds->entityUI;
		EntityManager* em = &ad->entityManager;
		bool loadedMap = false;

		int mapIndex = pi->maps.findStr(em->currentMap)-1;

		{
			ql.row(ql.text("Refresh"), 0);

			if(gui->qButton(ql.next(), ql.nextText())) refreshMaps(&pi->maps);

			if(gui->qComboBox(ql.next(), &mapIndex, pi->maps.data, pi->maps.count)) {
				loadMap(em, pi->maps[mapIndex]);
				loadedMap = true;
			}
		}

		{
			ql.row(ql.textW("Refresh"), 0);

			gui->qText(ql.next(), "Rename");

			TextEditSettings set = gui->sEdit;
			set.dontCopy = true;

			if(gui->qTextEdit(ql.next(), pi->maps.data[mapIndex], 50, &set)) {
				char* newName = getPString(gui->editText);
				char* oldName = pi->maps.at(mapIndex);
				pi->maps.at(mapIndex) = newName;
				rename(fString("%s%s%s", App_Map_Folder, oldName, Map_File_Extension), 
				       fString("%s%s%s", App_Map_Folder, newName, Map_File_Extension));
				em->currentMap = getPString(newName);
			}
		}

		{
			ql.row(0.0f,0.0f);

			if(gui->qButton(ql.next(), "Load")) {
				if(pi->maps.count) {
					loadMap(em, pi->maps[mapIndex]);
					loadedMap = true;
				}
			}

			if(gui->qButton(ql.next(), "Save")) {
				if(pi->maps.count)
					saveMap(em, pi->maps[mapIndex]);
			}
		}

		{
			ql.row(0.0f,0.0f);

			if(gui->qButton(ql.next(), "New")) {
				EntityManager emTmp = {};
				emTmp.entities.init();

				defaultMap(&emTmp);

				char* fileName = "new";
				saveMap(&emTmp, fileName);
				refreshMaps(&pi->maps);
				loadMap(em, fileName);
				loadedMap = true;

				for(int i = 0; i < pi->maps.count; i++) {
					if(strCompare(pi->maps[i], fileName)) {
						mapIndex = i;
						break;
					}
				}
			}

			if(gui->qButton(ql.next(), "Delete")) {
				char* filePath = fString("%s%s%s", App_Map_Folder, pi->maps[mapIndex], Map_File_Extension);
				remove(filePath);
				refreshMaps(&pi->maps);

				mapIndex = clamp(mapIndex, 0, pi->maps.count-1);
				loadMap(em, pi->maps[mapIndex]);
				loadedMap = true;
			}
		}

		{
			ql.row(0.0f,0.0f);
			if(gui->qButton(ql.next(), "Copy")) {
				char* newName = fString("%s_copy", pi->maps[mapIndex]);
				saveMap(em, newName);
				refreshMaps(&pi->maps);
				loadMap(em, newName);
				loadedMap = true;

				for(int i = 0; i < pi->maps.count; i++) {
					if(strCompare(pi->maps[i], newName)) {
						mapIndex = i;
						break;
					}
				}
			}
		}

		ql.seperator(pd.separatorHeight, pd.cSeparatorDark, pd.cSeparatorBright);

		{
			ql.row(ql.text("Insert Entity", 1), 0);
			gui->qText(ql.next(), ql.nextText(), vec2i(-1,0));

			int objectType = ET_Object;
			int event = gui->qComboBox(ql.next(), &objectType, getMember(getStructInfo(getType(Entity)), "type"));
			if(event == 1) {
				eui->objectCopies.clear();
				eui->objectCopies.push(getDefaultEntity(objectType, vec3(0.0)));
				int id = insertObjects(em, eui, false);

				eui->selectedObjects.clear();
				eui->selectedObjects.push(id);
				selectionChanged(eui);
			}
		}

		if(loadedMap) {
			eui->selectedObjects.clear();
			eui->selectionState = ENTITYUI_INACTIVE;
			selectionChanged(eui);

			historyReset(&eui->history);
		}
	}
}

struct MemberCompareData {
	EntityManager* em;
	DArray<int> selected;
	Entity* defaultEntity;
	Entity* tempEntity;
};

int memberEqual(MemberCompareData* mcd, MemberInfoData mInfoData, bool clearIfMultipleDifferent = false) {
	MemberInfo* mInfo = &mInfoData.mInfo;
	char* data = mInfoData.data;

	int dataOffset = (data - (char*)mcd->tempEntity);

	if(mcd->selected.count == 1) {
		bool isDefault = compareMemberInfoData(mInfo, data, (char*)(mcd->defaultEntity) + dataOffset);
		return isDefault ? 1 : 0;
	} else {
		bool allSameValue = true;
		bool allDefaultValue = true;

		for(int i = 1; i < mcd->selected.count; i++) {
			Entity* se = getEntity(mcd->em, mcd->selected.data[i]);
			bool equal = compareMemberInfoData(mInfo, data, (char*)se + dataOffset);
			if(!equal) {
				allSameValue = false;
				allDefaultValue = false;
				break;
			}
		}

		if(allSameValue) {
			bool isDefault = compareMemberInfoData(mInfo, data, (char*)(mcd->defaultEntity) + dataOffset);
			if(!isDefault) allDefaultValue = false;
		}

		if(allDefaultValue) return 1;
		if(allSameValue) return 0;

		// if(clearIfMultipleDifferent)
			// memcpy(mInfoData.data, (char*)(mcd->defaultEntity) + dataOffset, getStructInfo(mInfo->type)->size);

		return 2;
	}

	return 0;
}

struct MemberWidgetSettings {
	TextSettings sText[3];
	TextEditSettings sEdit[3];
	SliderSettings sSlider[3];
	TextBoxSettings sComboBox[3];
	CheckBoxSettings sCheckBox[3];
};

int memberWidgetTypes[] = { 
	TYPE_Vec2, TYPE_Vec3, TYPE_Vec4, TYPE_Quat,
	TYPE_SpawnRegion, TYPE_ValueRange_float, TYPE_ValueRange_Vec4,
};
char* specialTags[] = {"Color", "Range", "Id"};
void memberGuiWidget(Gui* gui, MemberCompareData* mCompareData, EntityManager* em, EntityUI* eui, DArray<MemberInfoData>* mInfoChanged, MemberWidgetSettings* set, QLayout ql, Rect r, MemberInfoData mInfoData) {
	MemberInfo* mInfo = &mInfoData.mInfo;
	char* data = mInfoData.data;

	if(memberGetTag(mInfo, "NoMod")) {
		gui->setInactive(false);
	}

	if(mInfo->enumCount) {
		TextBoxSettings* s = set->sComboBox + memberEqual(mCompareData, mInfoData, true);
		if(gui->qComboBox(r, (int*)data, mInfo, s)) mInfoChanged->push(mInfoData);

		return;
	}

	bool useTag = false;
	bool special = false;
	for(auto it : specialTags) {
		if(memberGetTag(mInfo, it)) {
			special = true;
			break;
		}
	}
	if(special) useTag = true;

	// @Hack
	if(memberGetTag(mInfo, "Range") && getStructInfo(mInfo->type)->memberCount > 2) useTag = false;

	if(useTag) {
		if(memberHasTag(mInfo, "Color")) {
			int result = gui->qColorPicker(r, (Vec4*)data);
			if(result) {
				mInfoChanged->push({mInfoData.mInfo, mInfoData.data, result == 1 ? true : false});
			}

		} else if(memberHasTag(mInfo, "Range")) {
			int memberCount = getStructInfo(mInfo->type)->memberCount;

			if(memberCount == 0) {
				SliderSettings* s = set->sSlider + memberEqual(mCompareData, mInfoData, true);
				char** tags = memberGetTag(mInfo, "Range");

				int result = 0;
				if(mInfo->type == TYPE_float) {
					result = gui->qSlider(r, (float*)data, strToFloat(tags[0]), strToFloat(tags[1]), s);
				} else if(mInfo->type == TYPE_int) {
					result = gui->qSlider(r, (int*)data, strToInt(tags[0]), strToInt(tags[1]), s);
				}

				if(result) {
					mInfoChanged->push({mInfoData.mInfo, mInfoData.data, result == 1 ? true : false});
				}
			}

			if(memberCount > 1) {
				float cols[] = {0,0}; 
				ql.row(r, cols, memberCount);

				for(int i = 0; i < memberCount; i++) {
					MemberInfoData mid = getMember(mInfo, data, i);

					int index = i;
					if(!memberGetTag(mInfo, "Range")[0 + i*2]) index = 0;

					mid.mInfo.tags[mid.mInfo.tagCount  ][0] = "Range";
					mid.mInfo.tags[mid.mInfo.tagCount  ][1] = memberGetTag(mInfo, "Range")[0 + index*2];
					mid.mInfo.tags[mid.mInfo.tagCount++][2] = memberGetTag(mInfo, "Range")[1 + index*2];

					memberGuiWidget(gui, mCompareData, em, eui, mInfoChanged, set, ql, ql.next(), mid);
				}
			}

		} else if(memberHasTag(mInfo, "Id")) {
			int* id = (int*)mInfoData.data;

			bool group = strCompare(memberGetTag(mInfo, "Id")[0], "Group");

			Entity* e = getEntity(em, *id);
			ql.row(r, 0.0f, ql.dim.h*2.5f, ql.dim.h*1.0f);

			TextEditSettings s = *(set->sEdit + memberEqual(mCompareData, mInfoData, true));
			s.dontCopy = true;

			int newId = *id;
			bool setId = false;

			char* str = e ? e->name : "";
			if(gui->qTextEdit(ql.next(), str, 50, &s)) {
				char* newName = gui->editText;
				Entity* e = findEntity(em, newName);

				setId = true;

				if(group) {
					if(e) newId = e->id;
					else if(strLen(newName)) {
						// Should spawn this in the center of all selected entities.
						Entity newGroup = getDefaultEntity(ET_Group, mCompareData->tempEntity->xf.trans);
						newGroup.name = getPString(newName);

						eui->objectCopies.clear();
						eui->objectCopies.push(newGroup);
						int id = insertObjects(em, eui, true);

						newId = id;
					} else newId = 0;

				} else {
					if(e) newId = e->id;
					else if(strLen(newName)) setId = false;
					else newId = 0;
				}
			}

			int oldId = *id;
			MemberInfoData mid = mInfoData;
			mid.mInfo.tagCount = 0;
			memberGuiWidget(gui, mCompareData, em, eui, mInfoChanged, set, ql, ql.next(), mid);

			if(*id != oldId) {
				setId = true;
				newId = *id;
			}

			if(setId) {
				*id = newId;
				mInfoChanged->push(mInfoData);
			}

			if(!e) gui->setInactive(true);
			if(gui->qButton(ql.next(), "►")) {
				eui->selectedObjects.clear();
				eui->selectedObjects.push(*id);
				selectionChanged(eui);
			}
		}
	} else {
		switch(mInfo->type) {
			case TYPE_bool: {
				CheckBoxSettings* s = set->sCheckBox + memberEqual(mCompareData, mInfoData, true);
				if(gui->qCheckBox(r, (bool*)data, s)) mInfoChanged->push(mInfoData);
			} break;
			
			case TYPE_int: {
				TextEditSettings* s = set->sEdit + memberEqual(mCompareData, mInfoData, true);
				if(gui->qTextEdit(r, (int*)data, s)) mInfoChanged->push(mInfoData);
			} break;

			case TYPE_float: {
				TextEditSettings* s = set->sEdit + memberEqual(mCompareData, mInfoData, true);
				if(gui->qTextEdit(r, (float*)data, s)) mInfoChanged->push(mInfoData);
			} break;

			case TYPE_char: {
				TextEditSettings* s = set->sEdit + memberEqual(mCompareData, mInfoData, true);
				if(gui->qTextEdit(r, (char*)data, 50, s)) mInfoChanged->push(mInfoData);
			} break;

			case TYPE_string: {
				TextEditSettings s = *(set->sEdit + memberEqual(mCompareData, mInfoData, true));
				s.dontCopy = true;

				char* t;
				char** d = (char**)data;
				t = !(*d) ? getTString("") : *d;

				if(gui->qTextEdit(r, t, 50, &s)) {
					*d = getPString(gui->editText);
					mInfoChanged->push(mInfoData);
				}

			} break;

			case TYPE_Vec2: 
			case TYPE_Vec3: 
			case TYPE_Vec4: 
			case TYPE_Quat: {
				int memberCount = getStructInfo(mInfo->type)->memberCount;
				float cols[4] = {0,0,0,0}; 
				ql.row(r, cols, memberCount);

				gui->setTabRange(memberCount);
				defer { gui->setTabRange(); };

				for(int i = 0; i < memberCount; i++) {
					memberGuiWidget(gui, mCompareData, em, eui, mInfoChanged, set, ql, ql.next(), getMember(mInfo, data, i)); 
				}
			} break;

			case TYPE_SpawnRegion: {
				int memberCount = *((int*)data) == SPAWN_REGION_SPHERE ? 3 : 4;
				float cols[4] = {0.4f,0,0,0}; 
				ql.row(r, cols, memberCount);

				memberGuiWidget(gui, mCompareData, em, eui, mInfoChanged, set, ql, ql.next(), getMember(mInfo, data, 0));

				for(int i = 0; i < memberCount; i++) {
					memberGuiWidget(gui, mCompareData, em, eui, mInfoChanged, set, ql, ql.next(), getMember(mInfo, data, i+1));
				}
			} break;

			case TYPE_ValueRange_float: {
				ql.row(r, 0, 0, 0.35f, ql.dim.h);

				MemberInfoData mid0 = getMember(mInfo, data, 0);
				MemberInfoData mid1 = getMember(mInfo, data, 1);
				memberCopyTags(mInfo, &mid0.mInfo);
				memberCopyTags(mInfo, &mid1.mInfo);

				{
					gui->setTabRange(2);
					defer { gui->setTabRange(); };

					memberGuiWidget(gui, mCompareData, em, eui, mInfoChanged, set, ql, ql.next(), mid0);
					memberGuiWidget(gui, mCompareData, em, eui, mInfoChanged, set, ql, ql.next(), mid1);
				}

				memberGuiWidget(gui, mCompareData, em, eui, mInfoChanged, set, ql, ql.next(), getMember(mInfo, data, 2));
				drawProbabilityGraph(gui, ql.next(), (ValueRange<float>*)data);
			} break;

			case TYPE_ValueRange_Vec4: {
				MemberInfoData mid0 = getMember(mInfo, data, 0);
				MemberInfoData mid1 = getMember(mInfo, data, 1);
				mid0.mInfo.tags[mid0.mInfo.tagCount++][0] = "Color";
				mid1.mInfo.tags[mid1.mInfo.tagCount++][0] = "Color";

				ql.row(r, ql.dim.h*1.5f, ql.dim.h*1.5f, 0, ql.dim.h);
				memberGuiWidget(gui, mCompareData, em, eui, mInfoChanged, set, ql, ql.next(), mid0);
				memberGuiWidget(gui, mCompareData, em, eui, mInfoChanged, set, ql, ql.next(), mid1);
				memberGuiWidget(gui, mCompareData, em, eui, mInfoChanged, set, ql, ql.next(), getMember(mInfo, data, 2));
				drawProbabilityGraph(gui, ql.next(), (ValueRange<Vec4>*)data);
			} break;

			default: gui->qText(r, "");
		}
	}
}

void entityPanel(AppData* ad, DebugState* ds, Gui* gui, PanelSettings pd) {
	EntityPanelInfo* pi = &ds->entityPanelInfo;

	if(ad->gameMode != GAME_MODE_MAIN) return;

	Font* font = gui->sText.font;
	QuickRow qr;
	Rect r;

	EntityUI* eui = &ds->entityUI;
	EntityManager* em = &ad->entityManager;

	int unionMemberIndex = 0;
	MemberInfo* unionMemberInfo = 0;
	{
		StructInfo* sInfo = getStructInfo(getType(Entity));
		for(int i = 0; i < sInfo->memberCount; i++) {
			MemberInfo* mInfo = sInfo->list + i;
			if(mInfo->uInfo.type != -1) break;

			unionMemberInfo = mInfo;
			unionMemberIndex = i;
		}
	}

	bool allSelectedSameType = true;
	for(int i = 1; i < eui->selectedObjects.count; i++) {
		Entity* e = getEntity(em, eui->selectedObjects[i]);
		if(e->type != getEntity(em, eui->selectedObjects[0])->type) {
			allSelectedSameType = false;
			break;
		}
	}

	QLayout ql = qLayout(pd.p, vec2(pd.dim.w, pd.dim.h), pd.pad, pd.textPad, gui->sText.font);

	{
		{
			bool filterChange = false;
			{
				EnumInfo* eInfos = Entity_Type_EnumInfos;
				int eCount = arrayCount(Entity_Type_EnumInfos);

				char* name = "Filter:";
				ql.row(ql.text(name, 1), ql.dim.h, 0.0f,0.0f);

				gui->qText(ql.next(), ql.nextText(), vec2i(-1,0));
				if(gui->qCheckBox(ql.next(), &pi->filterEnabled, vec2i(-1,0))) filterChange = true;

				{
					EnumInfo* eInfo = getEnumInfo(Entity_Type);
					int count = getEnumInfoCount(Entity_Type);

					char** strs = getTArray(char*, count+1);
					strs[0] = "";
					for(int i = 1; i < count+1; i++) {
						strs[i] = eInfo[i-1].nameWithoutTag;
					}
					pi->typeFilterIndex += 1;
					if(gui->qComboBox(ql.next(), &pi->typeFilterIndex, strs, eCount+1)) {
						filterChange = true;
					}
					pi->typeFilterIndex -= 1;
				}

				TextEditSettings tes = gui->sEdit;
				tes.defaultText = "<Name>";
				if(gui->qTextEdit(ql.next(), pi->nameFilter, arrayCount(pi->nameFilter), &tes)) filterChange = true;

				ql.row(ql.textW(name, true), ql.text("Dist to Cam:", 2), 0);
				ql.next();
				gui->qText(ql.next(), ql.nextText(), vec2i(-1,0));
				if(gui->qTextEdit(ql.next(), &pi->camDistFilter)) filterChange = true;

			}
			if(filterChange && !pi->filterEnabled) filterChange = false;

			DArray<int> filteredEntities;
			filteredEntities.init(getTMemory);

			bool selectedEntitiesBefore = eui->selectedObjects.count;
			bool updatedSelectedEntities = false;

			for(int i = 0; i < em->entities.count; i++) {
				Entity* e = em->entities.data + i;
				if(!entityIsValid(e)) continue;

				int id = e->id;

				bool filtered = false;
				if(pi->filterEnabled) {
					if(pi->typeFilterIndex != -1 && e->type != pi->typeFilterIndex) filtered = true;
					else if(strLen(pi->nameFilter)) {
						if(strLen(e->name)) {
							if(strFind(e->name, pi->nameFilter) == -1) filtered = true;
						} else filtered = true;
					} else if(pi->camDistFilter != 0) {
						float distToCam = len(e->xf.trans - theGState->activeCam.pos);
						if(distToCam > pi->camDistFilter) filtered = true;
					}
				}

				if(filtered) {
					if(eui->selectedObjects.count) {
						int index = eui->selectedObjects.find(id);
						if(index) {
							eui->selectedObjects.remove(index-1);
							updatedSelectedEntities = true;
						}
					}
					continue;
				}

				filteredEntities.push(id);
			}

			if(updatedSelectedEntities || filterChange) {
				if((selectedEntitiesBefore && !eui->selectedObjects.count) || 
				   filterChange) {
					if(filteredEntities.count) {
						eui->selectedObjects.clear();
						eui->selectedObjects.push(filteredEntities.first());
					}
				}
				selectionChanged(eui);
			}

			EnumInfo* ei = Entity_Type_EnumInfos;
			int eIndex = 0;
			if(eui->selectedObjects.count) {
				int firstSelected = eui->selectedObjects.first();
				for(int i = 0; i < filteredEntities.count; i++) {
					if(filteredEntities.data[i] == firstSelected) {
						eIndex = i;
						break;
					}
				}
			}

			char** strs = getTArray(char*, filteredEntities.count);
			for(int i = 0; i < filteredEntities.count; i++) {
				int index = filteredEntities.data[i];

				Entity* e = getEntity(em, index);
				int eType = e->type;
				int typeIndex = -1;

				DArray<Entity*>* typeList = &em->byType[eType];
				for(int j = 0; j < typeList->count; j++) {
					if(typeList->data[j]->id == index) {
						typeIndex = j;
						break;
					}
				}

				strs[i] = fString("%i. %s %i", index, ei[eType].nameWithoutTag, typeIndex+1);
				if(e->name && strLen(e->name)) {
					strs[i] = fString("%s, %s", strs[i], fString("\"%s\"", e->name));
				}
			}

			{
				ql.row(ql.dim.h*1.5f, 0, ql.dim.h*1.5f);

				if(eui->selectedObjects.count != 1) gui->setInactive(true);
				if(gui->qButton(ql.next(), "◄")) {
					eIndex = mod(eIndex-1, filteredEntities.count);
					eui->selectedObjects[0] = filteredEntities.data[eIndex];
					selectionChanged(eui);
				}

				if(gui->qComboBox(ql.next(), &eIndex, strs, filteredEntities.count)) {
					// @Todo: Need to put this into a function.
					eui->selectedObjects.clear();
					eui->selectedObjects.push(filteredEntities.data[gui->comboBoxData.index]);
					selectionChanged(eui);
				}

				if(eui->selectedObjects.count != 1) gui->setInactive(true);
				if(gui->qButton(ql.next(), "►")) {
					eIndex = mod(eIndex+1, filteredEntities.count);
					eui->selectedObjects[0] = filteredEntities.data[eIndex];
					selectionChanged(eui);
				}
			}
		}

		ql.seperator(pd.separatorHeight, pd.cSeparatorDark, pd.cSeparatorBright);

		if(eui->selectedObjects.count == 0) return;

		// Header.
		if(eui->selectedObjects.count == 1) {
			EnumInfo* ei = Entity_Type_EnumInfos;
			int eId = eui->selectedObjects.first();
			int eType = getEntity(em, eId)->type;
			int typeIndex = -1;
			int typeCount = em->byType[eType].count;

			DArray<Entity*>* typeList = &em->byType[eType];
			for(int i = 0; i < typeList->count; i++) {
				if(typeList->data[i]->id == eId) {
					typeIndex = i;
					break;
				}
			}

			char* c = "93bdff";
			char* cIndex     = fString("<c>%s%i<c>", c, eId);
			char* cIndexType = fString("<c>%s%i<c>", c, typeIndex+1);
			char* cCountType = fString("<c>%s%i<c>", c, typeCount);

			char* headerString = fString("<b>%s. (%s %s of %s)<b>", cIndex, ei[eType].nameWithoutTag, cIndexType, cCountType);
			gui->qText(ql.getRect(), headerString, vec2i(0,0));

		} else {
			char* c = "93bdff";
			
			char* headerString = "";
			{
				int etCount[ET_Size-1] = {};
				for(int i = 0; i < eui->selectedObjects.count; i++) {
					Entity* se = getEntity(em, eui->selectedObjects[i]);
					etCount[se->type]++;
				}

				EnumInfo* ei = Entity_Type_EnumInfos;

				char* typeStr = "";
				bool first = true;
				for(int i = 0; i < ET_Size-1; i++) {
					if(etCount[i] == 0) continue;
					char* comma = ", ";
					if(first) {
						first = false;
						comma = "";
					}
					char* cCount = fString("<c>%s%i<c>", c, etCount[i]);
					typeStr = fString("%s%s%s %s%s", typeStr, comma, cCount, ei[i].nameWithoutTag, etCount[i] > 1 ? "s" : "");
				}

				char* cCount = fString("<c>%s%i<c>", c, eui->selectedObjects.count);
				headerString = fString("<b>%s Entities (%s)<b>", cCount, typeStr);
			}

			gui->qText(ql.getRect(), headerString, vec2i(0,0));
		}

		// Sections.
		{
			TextBoxSettings tbs = textBoxSettings(gui->sText, boxSettings(gui->sBox.color));
			TextBoxSettings tbsActive = tbs;
			tbsActive.boxSettings.borderColor = gui->sBox.borderColor;

			float oldPad = ql.pad.w;
			ql.pad.w = 0;
			defer { ql.pad.w = oldPad; };

			char* labels[] = {"Flags", "Common", "Main"};
			qr = ql.row(ql.text(labels[0]), ql.text(labels[1]), ql.text(labels[2]));

			bool noMainSection = unionMemberInfo == 0 || !allSelectedSameType;

			int sectionCount = arrayCount(labels);
			if(noMainSection) sectionCount--;
			clamp(&pi->activeSection, 0, sectionCount-1);

			for(int i = 0; i < sectionCount; i++) {
				bool active = pi->activeSection == i;

				char* text = labels[i];
				float labelWidth = getTextDim(text, font).w + pd.textPad;

				if(active) gui->setInactive(false);

				TextBoxSettings* settings = active ? &tbsActive : &tbs;
				if(gui->qPButton(qr.next(), text, settings)) pi->activeSection = i;
			}
		}

		ql.seperator(pd.separatorHeight, pd.cSeparatorDark, pd.cSeparatorBright);
	}

	static float scrollHeight = 0;
	float yOffset, wOffset;
	{
		static float scrollValue = 0;
		Rect pr = pd.pri.setT(ql.pos.y);

		gui->sScroll.scrollBarPadding = pd.pad.x;
		gui->sScroll.scrollAmount = pd.dim.h + pd.pad.y;
		gui->qScroll(pr, scrollHeight, &scrollValue, &yOffset, &wOffset);
	}

	ql = qLayout(ql.pos + vec2(0,yOffset), vec2(pd.dim.w - wOffset,pd.dim.h), pd.pad);
	ql.font = font;
	ql.textPad = pd.textPad*0.5f;

	float startY = ql.pos.y;

	defer { gui->qScrollEnd(); };
	defer { scrollHeight = startY - ql.pos.y - pd.pad.y; };
	
	// 
	
	{
		ql.dim.h = round(pd.dim.h * 0.9f);
		ql.pad.h = round(pd.pad.h * 0.9f);
		ql.pad.w = round(pd.pad.w * 0.9f);

		int selectedCount = eui->selectedObjects.count;

		if(pi->activeSection == 0) return;

		Entity tempEntity = *getEntity(em, eui->selectedObjects[0]);
		Entity defaultEntity = getDefaultEntity(tempEntity.type, vec3(0,0,0), true);

		if(eui->selectionState != ENTITYUI_ACTIVE) {
			eui->history.objectsPreMod.clear();
			for(int i = 0; i < eui->selectedObjects.count; i++) {
				Entity obj = *getEntity(em, eui->selectedObjects[i]);
				eui->history.objectsPreMod.push(obj);
			}
		}

		DArray<MemberInfoData> mInfoChanged;
		mInfoChanged.init(getTMemory);

		MemberCompareData mCompareData = {em, eui->selectedObjects, &defaultEntity, &tempEntity};
		MemberWidgetSettings settings;
		{
			for(int i = 0; i < 3; i ++) settings.sText[i] = gui->sText;
			for(int i = 0; i < 3; i ++) settings.sEdit[i] = gui->sEdit;
			for(int i = 0; i < 3; i ++) settings.sComboBox[i] = gui->sComboBox;
			for(int i = 0; i < 3; i ++) settings.sCheckBox[i] = gui->sCheckBox;
			for(int i = 0; i < 3; i ++) {
				settings.sSlider[i] = gui->sSlider;
				settings.sSlider[i].notifyWhenActive = true;
			}
			
			float alpha = 0.2f;
			Vec4 color = hslToRgbf(0.85f,1,0.7f, 1);

			settings.sText[1].color.a = alpha;
			settings.sText[2].color = color;
			settings.sEdit[1].textBoxSettings.textSettings.color.a = alpha;
			settings.sEdit[2].textBoxSettings.textSettings.color = color;
			settings.sSlider[1].textBoxSettings.textSettings.color.a = alpha;
			settings.sSlider[2].textBoxSettings.textSettings.color = color;
			settings.sComboBox[1].textSettings.color.a = alpha;
			settings.sComboBox[2].textSettings.color = color;
			settings.sCheckBox[1].color.a = alpha;
			settings.sCheckBox[2].color = color;
		}

		float maxTextWidth = 0;
		for(int stage = 0; stage < 2; stage++) {
			TypeTraverse tt;
			tt.start(getType(Entity), (char*)&tempEntity, "Entity");
			tt.ignoreHiddenMembers = true;

			while(true) {
				if(tt.validGet()) {

					bool handleStructAsAtom = false;
					if(tt.type == SData_STRUCT) {
						for(int i = 0; i < arrayCount(memberWidgetTypes); i++) {
							if(memberWidgetTypes[i] == tt.memberType) {
								handleStructAsAtom = true;
								break;
							}
						}
					}

					if(tt.type == SData_ATOM || handleStructAsAtom) {
						if(stage == 0) {
							maxTextWidth = max(maxTextWidth, ql.textW(tt.mInfo->name));
						} else {
							MemberInfoData mInfoData = {*tt.mInfo, tt.data};
							QuickRow qr = ql.row(maxTextWidth, 0.0f);

							TextSettings s = *(settings.sText + memberEqual(&mCompareData, mInfoData));
							char* name = tt.memberName();
							if(tt.insideArray()) name = fString("%s%s", tt.mInfo->name, name); 
							gui->qText(qr.next(), name, vec2i(-1,0), &s);

							memberGuiWidget(gui, &mCompareData, em, eui, &mInfoChanged, &settings, ql, qr.next(), mInfoData);

							if(memberHasTag(tt.mInfo, "Section"))
								ql.seperator(pd.separatorHeight, pd.cSeparatorDark, pd.cSeparatorBright);
						}

						tt.next();

					} else {
						tt.push();

						if(pi->activeSection == 2 && tt.stackCount == 2) {
							tt.e->memberIndex = unionMemberIndex+1;
							tt.skip();
						}
					}

					if(pi->activeSection == 1 && tt.stackCount == 2 && tt.mInfo == unionMemberInfo) break;

				} else if(!tt.popNext()) break;
			}
		}

		if(eui->selectionState != ENTITYUI_ACTIVE && mInfoChanged.count) {
			bool historyAdd = true;
			for(int i = 0; i < mInfoChanged.count; i++) {
				MemberInfoData* mInfoData = mInfoChanged.data + i;

				if(mInfoData->noHistoryChange) historyAdd = false;

				int dataOffset = (mInfoData->data - (char*)&tempEntity);
				for(auto it : eui->selectedObjects) {
					Entity* se = getEntity(em, it);
					copyMemberInfoData(&mInfoData->mInfo, mInfoData->data, (char*)se + dataOffset);

					// @Hack.

					if(strCompare(mInfoData->mInfo.name, "mountParentId")) {
						if(se->mountParentId) entityXFormToLocal(em, se);
						else se->xfMount = xForm();
					}

					if(se->mountParentId) {
						Entity* parentEntity = getEntity(em, se->mountParentId);

						StructInfo* sInfo = getStructInfo(getType(Entity));
						MemberInfo* mInfoXForm = getMember(sInfo, "xf");

						// @Fix this....
						int offsets[4] = { 
							mInfoXForm->offset + getMember(mInfoXForm, "trans")->offset, 
							mInfoXForm->offset + getMember(mInfoXForm, "rot")->offset, 
							mInfoXForm->offset + getMember(mInfoXForm, "scale")->offset 
						};
						offsets[3] = offsets[2] + getStructInfo(getMember(mInfoXForm, "scale")->type)->size;

						// @Duplication with updateEntityUI.
						if(dataOffset >= offsets[0] && dataOffset < offsets[1])
							se->xfMount.trans = (inverse(parentEntity->xf) * se->xf).trans;
						if(dataOffset >= offsets[1] && dataOffset < offsets[2])
							se->xfMount.rot = (inverse(parentEntity->xf) * se->xf).rot;
						if(dataOffset >= offsets[2] && dataOffset < offsets[3])
							se->xfMount.scale = (inverse(parentEntity->xf) * se->xf).scale;
					}
				}

				// @Hack, for widgets that keep updating when they are active we have to hand place 
				// the old values from the gui to the pre mod entities when they are active.
				// The widget code should probably handle this.
				if(memberHasTag(&mInfoData->mInfo, "Color") ||
				   memberHasTag(&mInfoData->mInfo, "Range")) {
					char* data;
					if(memberHasTag(&mInfoData->mInfo, "Color"))
						data = (char*)&gui->colorPickerColorStart;
					else if(memberHasTag(&mInfoData->mInfo, "Range")) {
						data = mInfoData->mInfo.type == TYPE_int ? (char*)&gui->editInt : (char*)&gui->editFloat;
					}

					for(int i = 0; i < eui->selectedObjects.count; i++) {
						Entity* obj = eui->history.objectsPreMod.data + i;
						copyMemberInfoData(&mInfoData->mInfo, data, (char*)obj + dataOffset);
					}
				}
			}

			if(getEntity(em, eui->selectedObjects[0])->type == ET_Sky) {
				ad->redrawSkyBox = true;
			}

			if(historyAdd) {
				eui->objectsEdited = true;
				eui->objectNoticeableChange = true;
			}
		}
	}
}

//

template <class T> 
void drawProbabilityGraph(Gui* gui, Rect r, ValueRange<T>* vr) {
	r = round(r);
	Vec4 color = vec4(hslToRgbf(0.5f,0.6f,0.3f),1);

	int sampleCount = 1000;

	const int valueCount = 10;
	int values[valueCount] = {};
	for(int i = 0; i < sampleCount; i++) {
		int index = vr->getSample()*(arrayCount(values));
		index = min((int)index, (int)arrayCount(values)-1);
		values[index]++;
	}

	float normalize = 0;
	for(int i = 0; i < arrayCount(values); i++) normalize = max((float)values[i], normalize);

	gui->scissorPush(r);
	defer { gui->scissorPop(); };

	Vec2 points[valueCount] = {};
	Vec2 lp = {};
	for(int i = 0; i < arrayCount(values); i++) {
		float x = r.left   + r.w() * ((float)i / (arrayCount(values)-1));
		float y = r.bottom + r.h() * (values[i] / normalize);
		Vec2 p = vec2(x,y);

		points[i] = p;
	}

	{
		dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		defer { dxEndPrimitive(); };

		for(int i = 0; i < arrayCount(values); i++) {
			Vec2 p = points[i];

			dxPushVertex( pVertex(vec2(p.x, r.bottom)) );
			dxPushVertex( pVertex(p) );
		}
	}

	{
		dxBeginPrimitiveColored(vec4(0.9f,1), D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
		defer { dxEndPrimitive(); };

		for(int i = 0; i < arrayCount(values); i++) {
			dxPushVertex( pVertex(points[i]) );
		}
	}
}