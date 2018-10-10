
// @Platform

bool pfGetRectScissor(Rect* scissor, Rect r) {
	*scissor = rectIntersect(*scissor, r);
	if(scissor->empty() || scissor->w()==0 || scissor->h()==0) return false;
	return true;
}

Rect pfGetRectScissor(Rect scissor, Rect r) {
	return rectIntersect(scissor, r);
}

bool pfTestRectScissor(Rect scissor, Rect r) {
	return pfGetRectScissor(&scissor, r);
}

void pfScissorTestScreen(Rect r) {
	if(r.w() < 0 || r.h() < 0) r = rect(0,0,0,0);

	dxScissor(round(r));
}

//

void pfSetScissor(bool enable = true) {
	dxScissorState(enable);
}

void pfLineWidth(int w) {
	// dcState(STATE_LINEWIDTH, w);
}

void pfDrawLine(Vec2 a, Vec2 b, Vec4 color) {
	dxDrawLine(a, b, color);
}

void pfDrawTriangle(Vec2 p, float size, Vec2 dir, Vec4 color) {
	dxDrawTriangle(p, size, color, dir);
}

void pfDrawRect(Rect r, Vec4 color) {
	dxDrawRect(r, color);
}

void pfDrawRectRounded(Rect r, Vec4 color, float size) {
	dxDrawRectRounded(r, color, size);
}

void pfDrawRectOutline(Rect r, Vec4 color) {
	dxDrawRectOutline(r, color);
}

void pfDrawRectRoundedOutline(Rect r, Vec4 color, float size) {
	dxDrawRectRoundedOutline(r, color, size);
}

void pfDrawRectRoundedGradient(Rect r, Vec4 color, float size, Vec4 off) {
	dxDrawRectRoundedGradient(r, color, size, off);
}

void pfDrawText(char* text, Vec2 startPos, Vec2i align, TextSettings s, int wrapWidth = 0) {
	::drawText(text, startPos, align, wrapWidth, s);
}

void pfDrawTextSelection(char* text, Font* font, Vec2 startPos, int index1, int index2, Vec4 color, Vec2i align = vec2i(-1,1), int wrapWidth = 0) {
	drawTextSelection(text, font, startPos, index1, index2, color, align, wrapWidth);
}

void pfDrawLineH(Vec2 p0, Vec2 p1, Vec4 color, bool roundUp = true) {
	dxDrawLineH(p0, p1, color, roundUp);
}
void pfDrawLineV(Vec2 p0, Vec2 p1, Vec4 color, bool roundUp = true) {
	dxDrawLineV(p0, p1, color, roundUp);
}

// @Settings.

BoxSettings boxSettings(Vec4 color, float roundedCorner, Vec4 borderColor) {
	return {color, roundedCorner, borderColor};
}
BoxSettings boxSettings(Vec4 color, float roundedCorner) {
	return {color, roundedCorner, 0};
}
BoxSettings boxSettings(Vec4 color) {
	return {color, 0};
}
BoxSettings boxSettings() {
	return {0};
}

CheckBoxSettings checkBoxSettings(BoxSettings boxSettings, Vec4 color, float sizeMod) {
	return {boxSettings, color, sizeMod};
}

TextBoxSettings textBoxSettings(TextSettings textSettings, BoxSettings boxSettings, float padding) {
	return {textSettings, boxSettings, padding};
}

TextBoxSettings textBoxSettings(TextSettings textSettings, BoxSettings boxSettings) {
	return {textSettings, boxSettings, 0};
}

TextEditSettings textEditSettings(TextBoxSettings textSettings, Vec4 defaultTextColor, char* textBuffer, int flags, float cursorWidth, float cursorHeightMod, Vec4 colorSelection, Vec4 colorCursor, float cursorFlashingSpeed, float textOffset) {
	return {textSettings, defaultTextColor, textBuffer, flags, cursorWidth, cursorHeightMod, "", colorSelection, colorCursor, cursorFlashingSpeed, textOffset};
}

SliderSettings sliderSettings(TextBoxSettings textBoxSettings, float size, float minSize, float lineWidth, float rounding, float heightOffset, Vec4 color, Vec4 lineColor) {
	return {textBoxSettings, size, minSize, lineWidth, rounding, heightOffset, color, lineColor, 1,1,false,0};
}

ScrollRegionSettings scrollRegionSettings(BoxSettings boxSettings, int flags, Vec2 border, float scrollBarWidth, float scrollBarPadding, Vec2 sliderMargin, float sliderRounding, float sliderSize, float sliderSizeMin, float scrollAmount, Vec4 sliderColor, Vec4 scrollBarColor) {

	ScrollRegionSettings s = {};
	s.boxSettings = boxSettings;
	s.flags = flags;
	s.border = border;
	s.scrollBarWidth = scrollBarWidth;
	s.scrollBarPadding = scrollBarPadding;
	s.sliderMargin = sliderMargin;
	s.sliderRounding = sliderRounding;
	s.sliderSize = sliderSize;
	s.sliderSizeMin = sliderSizeMin;
	s.scrollAmount = scrollAmount;
	s.sliderColor = sliderColor;
	s.scrollBarColor = scrollBarColor;
	return s;
}

LayoutData layoutData(Rect region, float lineHeightMedium, Vec2 paddingMedium) {
	LayoutData ld = {};
	ld.region = region;
	ld.pos = region.tl();
	ld.lineHeight = lineHeightMedium;
	ld.padding = paddingMedium;

	ld.lineHeightSmall = lineHeightMedium*0.75f;
	ld.lineHeightMedium = lineHeightMedium;
	ld.lineHeightLarge = lineHeightMedium*2.0f;

	ld.paddingSmall = vec2(0,0);
	ld.paddingMedium = paddingMedium;
	ld.paddingLarge = paddingMedium * 2.5f;

	return ld;
}

//

void Gui::defaultSettings(Font* font) {
	AppColors c = {};

	int fontHeight = font->height;

	Vec4 cEditCursor = c.text;
	Vec4 cEditSelection = vec4(hslToRgbf(0.6f,0.4f,0.4f),1);

	float buttonRounding = fontHeight * 0.3f;
	float textPadding = fontHeight * 0.4f;

	BoxSettings bs = boxSettings(c.background, 0, c.outline);
	TextSettings ts = textSettings(font, c.text);
	TextSettings ts2 = textSettings(font, c.text, TEXTSHADOW_MODE_SHADOW, vec2(1,-1), fontHeight*0.15f, c.outline);

	BoxSettings bous = boxSettings(c.button, buttonRounding, c.outline);
	// bous.vertGradientOffset = 0.04f;
	bous.vertGradientOffset = 0.05f;
	TextBoxSettings bus = textBoxSettings(ts, bous, fontHeight * 0.5f);

	sText = ts;
	sText2 = ts2;
	sBox = bs;
	sTextBox = {ts, bs, fontHeight * 0.5f};
	sTextBox2 = {ts2, bs, fontHeight * 0.5f};
	sButton = bus;

	TextBoxSettings etbs = sTextBox;
	etbs.boxSettings.color = c.edit;
	etbs.boxSettings.roundedCorner = 0;
	sEdit = textEditSettings(etbs, vec4(0,0,0,0), editText, ESETTINGS_SINGLE_LINE | ESETTINGS_START_RIGHT, 1, 1.1f, cEditSelection, cEditCursor, 6, textPadding);

	float sw = fontHeight*1.0f;
	sSlider = sliderSettings(etbs, sw, sw, 0, 0, fontHeight*0.20f, c.button, vec4(0,0,0,0));
	sSlider.borderColor = c.outline;
	sSlider.resetDistance = fontHeight*5;
	sSlider.notifyWhenActive = false;

	sPopup = boxSettings(c.background, 0, c.outline);

	sComboBox = textBoxSettings(sText, boxSettings(c.edit, 0, c.outline), textPadding);

	BoxSettings cbs = boxSettings(c.edit, buttonRounding, c.outline);
	sCheckBox = checkBoxSettings(cbs, c.button, 0.70f);

	int scrollFlags = SCROLL_SLIDER | SCROLL_MOUSE_WHEEL | SCROLL_DYNAMIC_HEIGHT;
	sScroll = scrollRegionSettings({}, scrollFlags, vec2(0,0), fontHeight*0.8f, 0, vec2(1), 0, 0, fontHeight*2, fontHeight, c.button, c.edit);
}

void Gui::begin(Input* input, WindowSettings* ws, float dt, bool mouseInClient) {
	int voidId = 0;

	id = 1;
	if(!forceNoClear) {
		gotActiveId = voidId;
		wasActiveId = voidId;
	}

	forceNoClear = false;

	for(int i = 0; i < Gui_Focus_Size; i++) {
		contenderId[i] = voidId;
		contenderIdZ[i] = voidId;
	}

	if(disable) {
		for(int i = 0; i < Gui_Focus_Size; i++) hotId[i] = voidId;
	}

	// if(activeId != 0) {
	// 	for(int i = 0; i < Gui_Focus_Size; i++) hotId[i] = voidId;
	// }

	zLevel = 0;

	colorModHot = vec4(0.08f, 0);
	colorModActive = vec4(0.17f, 0);

	this->input = input;
	windowSettings = ws;
	this->dt = dt;

	hotDecayTime = 0.2f;

	this->mouseInClient = mouseInClient;

	currentCursor = IDC_ARROW;

	scissor = rectCenDim(0,0,10000000,10000000);
	scissorStack[0] = scissor;
	scissorStackIndex = 0;
	layoutStackIndex = 0;

	pfScissorTestScreen(scissor);
	pfSetScissor();

	scissorPush(scissor);

	// LayoutData ld = {rect(0,0,0,0), 0, 0, 0};
	// layoutPush(ld);
}

void Gui::end() {

	if(currentCursor != IDC_ARROW && mouseInClient) {
		setCursorIcon(windowSettings, currentCursor);
	}

	// layoutPop();

	for(int i = 0; i < Gui_Focus_Size; i++) {
		hotId[i] = contenderId[i];
	}

	if(!mouseInClient) {
		for(int i = 0; i < Gui_Focus_Size; i++) {
			hotId[i] = 0;
		}
	}

	for(int i = 0; i < hotQueueCount; i++) {
		Gui::Timestamp* stamp = hotQueue + i;
		stamp->dt -= dt;
		if(stamp->dt <= 0 || stamp->id == wasActiveId) {
			hotQueue[i] = hotQueue[hotQueueCount-1];
			hotQueueCount--;
			i--;
		}
	}

	if(hotId[Gui_Focus_MLeft]) {
		bool alreadyInQueue = false;
		int newId = hotId[Gui_Focus_MLeft];

		for(int i = 0; i < hotQueueCount; i++) {
			Gui::Timestamp* stamp = hotQueue + i;
			if(stamp->id == newId) {
				alreadyInQueue = true;
				stamp->dt = hotDecayTime;
				break;
			}
		}

		if(!alreadyInQueue) {
			if(hotQueueCount < arrayCount(hotQueue)) {
				hotQueue[hotQueueCount++] = {hotId[Gui_Focus_MLeft], hotDecayTime};
			}
		}
	}

	pfSetScissor(false);

	scissorPop();
}

int Gui::advanceId() {
	return id++;
}

// "Increment" doesn't make sense anymore, but too lazy to change the name.
int Gui::incrementId() {
	if(storedIdIndex < storedIdCount) {
		return storedIds[storedIdIndex++];

	} else {
		return advanceId();
	}
}

void Gui::storeIds(int count) {
	storedIdIndex = 0;
	storedIdCount = count;
	for(int i = 0; i < count; i++) {
		storedIds[i] = advanceId();
	}
}

void Gui::clearStoredIds() {
	storedIdIndex = 0;
	storedIdCount = 0;
}

int Gui::currentId() {
	if(storedIdIndex < storedIdCount) {
		return storedIds[storedIdIndex-1];
	} else {
		return id-1;
	}
}

bool Gui::isHot(int id, int focus) {
	return (id==0?currentId():id) == hotId[focus];
}

bool Gui::isActive(int id) {
	return (id==0?currentId():id) == activeId;
}

bool Gui::isHotOrActive(int id, int focus) {
	return isHot(id, focus) || isActive(id);
}

bool Gui::gotActive(int id) {
	return (id==0?currentId():id) == gotActiveId;
}

bool Gui::wasActive(int id) {
	return (id==0?currentId():id) == wasActiveId;
}

bool Gui::isWasHotOrActive(int id, int focus) {
	return isHot(id, focus) || isActive(id) || wasActive(id);
}

void Gui::clearActive() {
	activeId = 0;
}

void Gui::setNotActive(int id) {
	clearActive();
	wasActiveId = id;
}

bool Gui::someoneActive() {
	return activeId != 0;
}

bool Gui::someoneHot() {
	for(int i = 0; i < arrayCount(hotId); i++) {
		if(hotId[i] != 0) return true;
	}
	return false;
}

void Gui::setNotActiveWhenActive(int id) {
	if(isActive(id)) clearActive();
}

void Gui::setActive(int id, bool input, int focus) {
	bool setActive = false;

	if(id == activeSignalId) {
		setActive = true;
		activeSignalId = 0;

	} else {
		if(!isActive(id)) {
			if(input) {
				int stop = 234;
			}
			if(input && isHot(id, focus)) {
				if(someoneActive()) { 
					activeSignalId = id;
					
				} else {
					setActive = true;
				}
			}
		}
	}

	if(setActive) {
		activeId = id;
		gotActiveId = id;
	}
}

bool Gui::focusCanBeHot(int focus) {
	// Input* input = input;
	// bool result = true;
	// switch(focus) {
	// 	case Gui_Focus_MLeft:   if(input->mouseButtonDown[0]) result = false; break;
	// 	case Gui_Focus_MRight:  if(input->mouseButtonDown[1]) result = false; break;
	// 	case Gui_Focus_MMiddle: if(input->mouseButtonDown[2]) result = false; break;
	// }
	
	bool result = true;
	return result;
}

void Gui::setHot(int id, float z, int focus) {
	// if(!newGuiSomeoneActive() && newGuiFocusCanBeHot(focus)) {
	if(focusCanBeHot(focus) && !setInactiveX) {
		if(z > contenderIdZ[focus]) {
			contenderId[focus] = id;
			contenderIdZ[focus] = z;

		} else {
			if(z == contenderIdZ[focus]) {
				contenderId[focus] = max(id, contenderId[focus]);
			}
		}
	}
}
void Gui::setHot(int id, int focus) {
	return setHot(id, zLevel, focus);
}

void Gui::setHotAll(int id, float z) {
	for(int i = 0; i < Gui_Focus_Size; i++) {
		setHot(id, z, i);
	}
}

void Gui::setHotAll(float z) {
	return setHotAll(incrementId(), z);
}

void Gui::setHotAllMouseOver(int id, Rect r, float z) {
	if(pointInRectEx(input->mousePosNegative, r)) return setHotAll(id, z);
}

void Gui::setHotAllMouseOver(Rect r, float z) {
	return setHotAllMouseOver(incrementId(), r, z);
}

void Gui::setHotMouseOver(int id, Vec2 mousePos, Rect r, float z, int focus) {
	if(pointInRectEx(mousePos, r) && !r.empty()) {
		setHot(id, z, focus);
	}
}

void Gui::clearHot() {
	for(int i = 0; i < Gui_Focus_Size; i++) {
		hotId[i] = 0;
	}
}

void Gui::forceActive(int id) {
	activeId = menuId;
	gotActiveId = menuId;
	forceNoClear = true;
}

int Gui::inputFromFocus(int focus, bool press) {
	switch(focus) {
		case Gui_Focus_MLeft: return press?input->mouseButtonPressed[0]:input->mouseButtonReleased[0];
		case Gui_Focus_MRight: return press?input->mouseButtonPressed[1]:input->mouseButtonReleased[1];
		case Gui_Focus_MMiddle: return press?input->mouseButtonPressed[2]:input->mouseButtonReleased[2];

		case Gui_Focus_MWheel: return input->mouseWheel != 0;
		default: return -1;
	}
}

void Gui::setCursor(LPCSTR cursorType) {
	currentCursor = cursorType;
}

void Gui::setInactive(bool stayInactive) {
	setInactiveX = true;
	stayInactive = stayInactive;
}

void Gui::setActive() {
	setInactiveX = false;
	stayInactive = false;
}

void Gui::handleInactive() {
	if(setInactiveX) {
		if(!stayInactive) setInactiveX = false;

		for(int i = 0; i < hotQueueCount; i++) {
			Gui::Timestamp* stamp = hotQueue + i;
			if(stamp->id == currentId()) {
				hotQueue[i] = hotQueue[hotQueueCount-1];
				hotQueueCount--;
				break;
			}
		}
	}
}

//

int Gui::buttonAction(int id, Rect r, float z, Vec2 mousePos, bool inp, int focus) {
	setActive(id, inp, focus);
	if(isActive(id)) setNotActive(id);
	setHotMouseOver(id, mousePos, r, z, focus);

	return id;
}
int Gui::buttonAction(Rect r, float z, Vec2 mousePos, bool inp, int focus) {
	return buttonAction(incrementId(), r, z, mousePos, inp, focus);
}
int Gui::buttonAction(int id, Rect r, float z, bool inp, int focus) {
	return buttonAction(id, r, z, input->mousePosNegative, inp, focus);
}
int Gui::buttonAction(int id, Rect r, float z, int focus) {
	bool inp = inputFromFocus(focus, true);
	return buttonAction(id, r, z, input->mousePosNegative, inp, focus);
}
int Gui::buttonAction(Rect r, float z, int focus) {
	return buttonAction(incrementId(), r, z, focus);
}
int Gui::buttonAction(Rect r, float z, bool inp, int focus) {
	return buttonAction(incrementId(), r, z, input->mousePosNegative, inp, focus);
}

//

bool Gui::goButtonAction(int id, Rect r, float z, bool inp, int focus) {
	return gotActive(buttonAction(id, r, z, inp, focus));
}

bool Gui::goButtonAction(int id, Rect r, float z, int focus) {
	bool inp = inputFromFocus(focus, true);
	return gotActive(buttonAction(id, r, z, inp, focus));
}
bool Gui::goButtonAction(Rect r, float z, int focus) {
	return goButtonAction(incrementId(), r, z, focus);
}
bool Gui::goButtonAction(Rect r, float z, bool inp, int focus) {
	return goButtonAction(incrementId(), r, z, inp, focus);
}
bool Gui::goButtonAction(Rect r, int focus) {
	return goButtonAction(incrementId(), r, zLevel, focus);
}

//

int Gui::dragAction(int id, Rect r, float z, Vec2 mousePos, bool inp, bool inputRelease, int focus) {
	setActive(id, inp, focus);
	if(isActive(id) && (inputRelease || activeSignalId)) {
		setNotActive(id);
	}
	setHotMouseOver(id, mousePos, r, z, focus);

	return id;
}
int Gui::dragAction(Rect r, float z, Vec2 mousePos, bool inp, bool inputRelease, int focus) {
	return dragAction(incrementId(), r, z, mousePos, inp, inputRelease, focus);
}
int Gui::dragAction(Rect r, float z, int focus) {
	bool inp = inputFromFocus(focus, true);
	bool inputRelease = inputFromFocus(focus, false);
	return dragAction(incrementId(), r, z, input->mousePosNegative, inp, inputRelease, focus);
}
int Gui::dragAction(bool isHot, int focus) {
	bool inp = inputFromFocus(focus, true);
	bool inputRelease = inputFromFocus(focus, false);

	int id = incrementId();
	Vec2 mousePos = input->mousePosNegative;

	setActive(id, inp);
	if(isActive(id) && inputRelease) setNotActive(id);

	if(isHot) setHot(id, focus);
	
	return id;
}

int Gui::goDragAction(Rect r, float z, bool inp, bool inputRelease, int focus, bool screenMouse) {
	Vec2 mousePos = screenMouse ? input->mousePosNegativeScreen : input->mousePosNegative;
	int id = dragAction(r, z, mousePos, inp, inputRelease, focus);
	if(gotActive(id)) return 1;
	if(isActive(id)) return 2;
	if(wasActive(id)) return 3;
	
	return 0;
}
int Gui::goDragAction(Rect r, float z, int focus, bool screenMouse) {
	bool inp = inputFromFocus(focus, true);
	bool inputRelease = inputFromFocus(focus, false);
	return goDragAction(r, z, inp, inputRelease, focus, screenMouse);
}
int Gui::goDragAction(Rect r, int focus, bool screenMouse) {
	bool inp = inputFromFocus(focus, true);
	bool inputRelease = inputFromFocus(focus, false);
	return goDragAction(r, zLevel, inp, inputRelease, focus, screenMouse);
}

//

bool Gui::goMousePosAction(Rect r, float z) {
	setHotMouseOver(incrementId(), input->mousePosNegative, r, z, Gui_Focus_MPos);
	bool hot = isHot(currentId(), Gui_Focus_MPos);
	return hot;
}

//

char* textSelectionToString(char* text, int index1, int index2) {
	myAssert(index1 >= 0 && index2 >= 0);

	int range = abs(index1 - index2);
	char* str = getTString(range + 1);
	strCpy(str, text + min(index1, index2), range);
	return str;
}

int textWordSearch(char* text, int startIndex, bool left) {
	if(left) {
		int index = startIndex;
		if(text[index] == ' ' && index != 0) index--;
		while(text[index] != ' ' && index != 0) index--;
		if(text[index] == ' ') index++;

		return index;
	} else {
		int index = startIndex;
		if(text[index] == ' ' && index != 0) index--;
		while(text[index] != ' ' && text[index] != '\0') index++;

		return index;
	}
}

void textEditBox(char* text, int textMaxSize, Font* font, Rect textRect, Input* input, Vec2i align, TextEditSettings tes, TextEditVars* tev) {

	bool wrapping = flagGet(tes.flags, ESETTINGS_WRAPPING);
	bool singleLine = flagGet(tes.flags, ESETTINGS_SINGLE_LINE);

	if(singleLine) wrapping = false;

	// Vec2 startPos = rectTL(textRect) + tev->scrollOffset;
	Vec2 startPos = textRect.tl() + tev->scrollOffset + vec2(tes.textOffset,0);
	int wrapWidth = wrapping ? textRect.dim().w : 0;

	int cursorIndex = tev->cursorIndex;
	int markerIndex = tev->markerIndex;

	// Bug?!, with the font that we.
	Vec2 off = vec2(2,0);
	Vec2 mp = input->mousePosNegative + off;
	if(singleLine) mp.y = textRect.cy(); // Lock mouse y axis.

	int mouseIndex = textMouseToIndex(text, font, startPos, mp, align, wrapWidth);

	if(input->doubleClick) {
		tev->wordSelectionMode = true;
		cursorIndex = 0;
		markerIndex = strLen(text);

		tev->wordSelectionStartIndex = mouseIndex;
	}

	if(input->mouseButtonReleased[0]) {
		tev->wordSelectionMode = false;
	}

	if(!tev->wordSelectionMode) {
		if(input->mouseButtonPressed[0]) {
			if(pointInRect(input->mousePosNegative, textRect)) {
				markerIndex = mouseIndex;
			}
		}

		if(input->mouseButtonDown[0]) {
			cursorIndex = mouseIndex;
		}

	} else {
		if(input->mouseButtonDown[0]) {
			if(tev->wordSelectionStartIndex != mouseIndex) {
				if(mouseIndex < tev->wordSelectionStartIndex) {
					markerIndex = textWordSearch(text, tev->wordSelectionStartIndex, false);
					cursorIndex = textWordSearch(text, mouseIndex, true);
				} else {
					markerIndex = textWordSearch(text, tev->wordSelectionStartIndex, true);
					cursorIndex = textWordSearch(text, mouseIndex, false);
				}
			} else {
				markerIndex = textWordSearch(text, mouseIndex, true);
				cursorIndex = textWordSearch(text, mouseIndex, false);
			}
		}
	}

	bool left = input->keysPressed[KEYCODE_LEFT];
	bool right = input->keysPressed[KEYCODE_RIGHT];
	bool up = input->keysPressed[KEYCODE_UP];
	bool down = input->keysPressed[KEYCODE_DOWN];

	bool a = input->keysPressed[KEYCODE_A];
	bool x = input->keysPressed[KEYCODE_X];
	bool c = input->keysPressed[KEYCODE_C];
	bool v = input->keysPressed[KEYCODE_V];
	
	bool home = input->keysPressed[KEYCODE_HOME];
	bool end = input->keysPressed[KEYCODE_END];
	bool backspace = input->keysPressed[KEYCODE_BACKSPACE];
	bool del = input->keysPressed[KEYCODE_DEL];
	bool enter = input->keysPressed[KEYCODE_RETURN];
	bool tab = input->keysPressed[KEYCODE_TAB];

	bool ctrl = input->keysDown[KEYCODE_CTRL];
	bool shift = input->keysDown[KEYCODE_SHIFT];

	// Main navigation and things.

	int startCursorIndex = cursorIndex;

	if(ctrl && backspace) {
		shift = true;
		left = true;
	}

	if(ctrl && del) {
		shift = true;
		right = true;
	}

	if(!singleLine) {
		if(up || down) {
			float cursorYOffset;
			if(up) cursorYOffset = font->height;
			else if(down) cursorYOffset = -font->height;

			Vec2 cPos = textIndexToPos(text, font, startPos, cursorIndex, align, wrapWidth);
			cPos.y += cursorYOffset;
			int newCursorIndex = textMouseToIndex(text, font, startPos, cPos, align, wrapWidth);
			cursorIndex = newCursorIndex;
		}
	}


	if(left) {
		if(ctrl) {
			if(cursorIndex > 0) {
				while(text[cursorIndex-1] == ' ') cursorIndex--;

				if(cursorIndex > 0)
					cursorIndex = strFindBackwards(text, ' ', cursorIndex-1);
			}
		} else {
			bool isSelected = cursorIndex != markerIndex;
			if(isSelected && !shift) {
				if(cursorIndex < markerIndex) {
					markerIndex = cursorIndex;
				} else {
					cursorIndex = markerIndex;
				}
			} else {
				if(cursorIndex > 0) cursorIndex--;
			}
		}
	}

	if(right) {
		if(ctrl) {
			while(text[cursorIndex] == ' ') cursorIndex++;
			if(cursorIndex <= strLen(text)) {
				cursorIndex = strFindOrEnd(text, ' ', cursorIndex+1);
				if(cursorIndex != strLen(text)) cursorIndex--;
			}
		} else {
			bool isSelected = cursorIndex != markerIndex;
			if(isSelected && !shift) {
				if(cursorIndex > markerIndex) {
					markerIndex = cursorIndex;
				} else {
					cursorIndex = markerIndex;
				}
			} else {
				if(cursorIndex < strLen(text)) cursorIndex++;
			}
		}
	}

	if(singleLine) {
		if(home) {
			cursorIndex = 0;
		}

		if(end) {
			cursorIndex = strLen(text);
		}
	}

	if((startCursorIndex != cursorIndex) && !shift) {
		markerIndex = cursorIndex;
	}

	if(ctrl && a) {
		cursorIndex = strLen(text);
		markerIndex = 0;
	}

	bool isSelected = cursorIndex != markerIndex;

	if((ctrl && x) && isSelected) {
		c = true;
		del = true;
	}

	if((ctrl && c) && isSelected) {
		char* selection = textSelectionToString(text, cursorIndex, markerIndex);
		setClipboard(selection);
	}

	if(enter) {
		if(singleLine) {
			// strClear(text);
			// cursorIndex = 0;
			// markerIndex = 0;
		} else {
			input->inputCharacters[input->inputCharacterCount++] = '\n';
		}
	}

	if(backspace || del || (input->inputCharacterCount > 0) || (ctrl && v)) {
		if(isSelected) {
			int delIndex = min(cursorIndex, markerIndex);
			int delAmount = abs(cursorIndex - markerIndex);
			strRemoveX(text, delIndex, delAmount);
			cursorIndex = delIndex;
		}

		markerIndex = cursorIndex;
	}

	if(ctrl && v) {
		char* clipboard = (char*)getClipboard();
		if(clipboard) {
			int clipboardSize = strLen(clipboard);
			if(clipboardSize + strLen(text) <= textMaxSize) {
				strInsert(text, cursorIndex, clipboard);
				cursorIndex += clipboardSize;
				markerIndex = cursorIndex;
			}
			closeClipboard();
		}
	}

	// Add input characters to input buffer.
	if(input->inputCharacterCount > 0) {
		if(input->inputCharacterCount + strLen(text) <= textMaxSize) {
			strInsert(text, cursorIndex, input->inputCharacters, input->inputCharacterCount);
			cursorIndex += input->inputCharacterCount;
			markerIndex = cursorIndex;
		}
	}

	if(backspace && !isSelected) {
		if(cursorIndex > 0) {
			strRemove(text, cursorIndex);
			cursorIndex--;
		}
		markerIndex = cursorIndex;
	}

	if(del && !isSelected) {
		if(cursorIndex+1 <= strLen(text)) {
			strRemove(text, cursorIndex+1);
		}
		markerIndex = cursorIndex;
	}

	// Scrolling.
	{
		Vec2 cursorPos = textIndexToPos(text, font, startPos, cursorIndex, align, wrapWidth);

		float leftEnd = textRect.left + tes.textOffset;
		float rightEnd = textRect.right - tes.textOffset;
		if(		cursorPos.x < leftEnd) tev->scrollOffset.x += leftEnd - cursorPos.x;
		else if(cursorPos.x > rightEnd) tev->scrollOffset.x += rightEnd - cursorPos.x;

		clampMax(&tev->scrollOffset.x, 0.0f);
		
		float ch = font->height;
		if(!singleLine) {
			if(		cursorPos.y - ch/2 < textRect.min.y) tev->scrollOffset.y += textRect.min.y - (cursorPos.y - ch/2);
			else if(cursorPos.y + ch/2 > textRect.max.y) tev->scrollOffset.y += textRect.max.y - (cursorPos.y + ch/2);

			clampMin(&tev->scrollOffset.y, 0.0f);
		}
	}

	tev->cursorChanged = (tev->cursorIndex != cursorIndex || tev->markerIndex != markerIndex);

	tev->cursorIndex = cursorIndex;
	tev->markerIndex = markerIndex;

	tev->cursorTimer += tev->dt*tes.cursorFlashingSpeed;
	if(tev->cursorChanged || input->mouseButtonPressed[0]) tev->cursorTimer = 0;
}

enum {
	EDIT_MODE_CHAR = 0,
	EDIT_MODE_INT,
	EDIT_MODE_FLOAT,
};

int Gui::goTextEdit(Rect textRect, float z, void* var, int mode, TextEditSettings editSettings, TextEditVars* editVars, int maxTextSize, bool doubleClick) {

	if(maxTextSize == 0) maxTextSize = arrayCount(editText);
	maxTextSize = min(maxTextSize, (int)arrayCount(editText));

	bool leftMouse = input->mouseButtonPressed[0] && !pointInRectEx(input->mousePosNegative, textRect);
	bool enter = input->keysPressed[KEYCODE_RETURN];
	bool escape = input->keysPressed[KEYCODE_ESCAPE];

	bool releaseEvent = leftMouse || enter || escape;

	int event = goDragAction(textRect, z, doubleClick?input->doubleClick:input->mouseButtonPressed[0], releaseEvent, Gui_Focus_MLeft);

	editVars->dt = dt;

	if(event == 1) {
		editVars->scrollOffset = vec2(0,0);
		if(mode == EDIT_MODE_CHAR) strCpy(editText, (char*)var);
		else if(mode == EDIT_MODE_INT) strCpy(editText, fillString("%i", *((int*)var)));
		else if(mode == EDIT_MODE_FLOAT) strCpy(editText, fillString("%f", *((float*)var)));
		if(flagGet(editSettings.flags, ESETTINGS_START_RIGHT)) {
			editVars->cursorIndex = editVars->markerIndex = strlen(editText);
		}
	}

	if(event == 3 && (leftMouse || enter)) {
		if(mode == 0)      strCpy((char*)var, editText);
		else if(mode == 1) *((int*)var) = strToInt(editText);
		else if(mode == 2) *((float*)var) = strToFloat(editText);
	}

	if(event == 1 || event == 2) {
		if(event == 1) editVars->cursorTimer = 0;
		textEditBox(editText, maxTextSize, editSettings.textBoxSettings.textSettings.font, textRect, input, vec2i(-1,1), editSettings, editVars);
	}

	if(event == 3 && (escape)) event = 4;

	return event;
}
int Gui::goTextEdit(Rect textRect, float z, char* text, TextEditSettings editSettings, int maxTextSize) {
	return goTextEdit(textRect, z, text, EDIT_MODE_CHAR, editSettings, &editVars, maxTextSize);
}
int Gui::goTextEdit(Rect textRect, float z, int* number, TextEditSettings editSettings) {
	return goTextEdit(textRect, z, number, EDIT_MODE_INT, editSettings, &editVars, 0);
}
int Gui::goTextEdit(Rect textRect, float z, float* number, TextEditSettings editSettings) {
	return goTextEdit(textRect, z, number, EDIT_MODE_FLOAT, editSettings, &editVars, 0);
}

Rect Gui::calcSlider(float value, Rect br, float size, float min, float max, bool horizontal) {
	if(horizontal) {
		float sliderPos = mapRange(value, min, max, br.left + size/2, br.right - size/2);
		Rect slider = rectCenDim(sliderPos, br.cy(), size, br.h());
		return slider;
	} else {
		float sliderPos = mapRange(value, min, max, br.bottom + size/2, br.top - size/2);
		Rect slider = rectCenDim(br.cx(), sliderPos, br.w(), size);
		return slider;
	}
}

float Gui::sliderGetValue(Vec2 sliderCenter, Rect br, float size, float min, float max, bool horizontal) {
	if(horizontal) {
		float sliderValue = mapRangeClamp(sliderCenter.x, br.left + size/2, br.right - size/2, min, max);
		return sliderValue;
	} else {
		float sliderValue = mapRangeClamp(sliderCenter.y, br.bottom + size/2, br.top - size/2, min, max);
		return sliderValue;
	}
}

Rect Gui::calcSliderScroll(float value, Rect br, float size, float min, float max, bool horizontal) {
	Rect slider;

	if(horizontal) {
		float sliderPos = mapRange(value, min, max, -(br.w() - size), 0.0f);
		slider = rectTDim(vec2(br.left + sliderPos, br.cy()), vec2(size, br.h()));

	} else {
		float sliderPos = mapRange(value, min, max, -(br.h() - size), 0.0f);
		slider = rectTDim(vec2(br.cx(), br.top + sliderPos), vec2(br.w(), size));
	}

	return slider;
}

float Gui::sliderGetValueScroll(Vec2 sliderCenter, Rect br, float size, float min, float max, bool horizontal) {
	float sliderValue;

	if(horizontal) {
		sliderValue = mapRange(-(br.left - sliderCenter.x), -(br.w() - size), 0.0f, min, 0.0f);
	} else {
		sliderValue = mapRange(-(br.top - sliderCenter.y), -(br.h() - size), 0.0f, min, 0.0f);
	}

	return sliderValue;
}


//

bool Gui::isHotQueued(int id, float* decay) {
	for(int i = 0; i < hotQueueCount; i++) {
		if(hotQueue[i].id == id) {
			*decay = hotQueue[i].dt / hotDecayTime;
			return true;
		}
	}
	*decay = 1.0f;
	return false;
}

Vec4 Gui::hotActiveColorMod(bool isHot, bool isActive, bool inactive, float hotDecay) {
	Vec4 colorMod = vec4(0,0,0,0);
	if(isHot) {
		float hotMod = 0.08f;
		if(hotDecay == 1.0f) {
			colorMod = vec4(hotMod,0); 
		} else {
			colorMod = vec4(hotMod*hotDecay,0);
		}
	}
	if(isActive) colorMod = vec4(0.17f,0); 

	if(inactive) colorMod.a = -0.5f;

	return colorMod;
}
Vec4 Gui::inactiveColorMod(bool inactive) {
	Vec4 colorMod = !inactive ? vec4(0,0) : vec4(-0.1f,0);
	return colorMod;
}
// We assume you got an id first before calling this.
Vec4 Gui::colorModId(int id, int focus) {
	float hotDecay;
	bool hot = isHotQueued(id, &hotDecay) || isHot(id, focus);
	return hotActiveColorMod(hot, isActive(id), setInactiveX, hotDecay);
}
Vec4 Gui::colorModBId(int id, int focus) {
	float hotDecay;
	bool hot = isHotQueued(id, &hotDecay) || isHot(id, focus);
	return hotActiveColorMod(hot, gotActive(id), setInactiveX, hotDecay);
}

Vec4 Gui::colorMod(int focus) {
	return colorModId(currentId(), focus);
}
Vec4 Gui::colorModB(int focus) {
	return colorModBId(currentId(), focus);
}

// @GuiAutomation

void Gui::scissorPush(Rect sciss) {
	scissor = rectIntersect(scissor, sciss);
	scissorStack[scissorStackIndex+1] = scissor;
	scissorStackIndex++;

	pfScissorTestScreen(scissor);
}
void Gui::scissorPop() {
	scissorStackIndex--;
	scissor = scissorStack[scissorStackIndex];

	pfScissorTestScreen(scissor);
}

void Gui::popupPush(PopupData data) {
	popupStack[popupStackCount++] = data;
}
void Gui::popupPop() {
	popupStackCount--;
}

//

LayoutData* Gui::layoutPush(LayoutData layoutData) {
	layoutStackIndex++;
	layoutStack[layoutStackIndex] = layoutData;
	ld = &layoutStack[layoutStackIndex];

	return ld;
}
LayoutData* Gui::layoutPush(Rect region) {
	LayoutData newLd = *ld;
	newLd.region = region;
	newLd.pos = newLd.region.tl();

	return layoutPush(newLd);
}
LayoutData* Gui::layoutPush() {
	return layoutPush(*ld);
}

LayoutData* Gui::layoutPop(bool updateY) {

	float lastRegionHeight = ld->region.h();
	layoutStackIndex--;
	ld = &layoutStack[layoutStackIndex];

	if(updateY) {
		// ld->pos.y -= lastRegionHeight + ld->padding.y*2;
	}
	return ld;
}

void Gui::layoutNewLine(int flag, float height) {
	float padding = ld->padding.y;
	if(flag != -1) {
		if(flag == LAYOUT_MOD_CUSTOM) padding = height;
		else padding = ld->paddings[flag].y;
	}

	ld->pos.y -= ld->lineHeight + padding;
	ld->pos.x = ld->region.left;

	ld->columnCount = 0;
}

void Gui::layoutRowArray(float* columns, int count) {
	assert(count <= arrayCount(ld->columns));

	ld->columnCount = count;
	ld->columnIndex = 0;

	Layout lay = layout(rect(0,0,0,0), false, vec2i(-1,0), vec2(ld->padding.x, 0));

	Rect r = rectTLDim(ld->pos, vec2(ld->region.w(), ld->lineHeight));
	Layout* node = layoutQuickRowArray(&lay, r, columns, count);

	for(int i = 0; i < count; i++) {
		ld->columns[i] = layoutInc(&node).w();
	}
}

void Gui::layoutRow(float s0, float s1, float s2, float s3) {
	float cols[4];
	cols[0] = s0;
	int count = 1;
	if(s1 != -1) { cols[1] = s1; count = 2; }
	if(s2 != -1) { cols[2] = s2; count = 3; }
	if(s3 != -1) { cols[3] = s3; count = 4; }

	layoutRowArray(cols, count);
}

Rect Gui::layoutGet(float width) {
	Rect r = rectTLDim(ld->pos, vec2(width, ld->lineHeight));
	ld->pos.x += width + ld->padding.w;

	return r;
}
Rect Gui::layoutGet() {
	if(ld->columnCount > 0) {
		float width = ld->columns[ld->columnIndex++];

		Rect r = layoutGet(width);

		if(ld->columnIndex > ld->columnCount-1) {
			ld->columnCount = 0;
			layoutNewLine();
		}

		return r;
	}

	Rect r = layoutGet(ld->region.w());
	layoutNewLine();

	return r;
}
Rect Gui::layoutGetAll(float height) {
	float oldLineHeight = ld->lineHeight;
	ld->lineHeight = height;

	Rect r = layoutGet();

	ld->lineHeight = oldLineHeight;
	return r;
}

void Gui::layoutSetLineHeight(int flag, float height) {
	if(flag == LAYOUT_MOD_CUSTOM) ld->lineHeight = height;
	else ld->lineHeight = ld->lineHeights[flag];
}	
void Gui::layoutSetXPadding(int flag, float size) {
	if(flag == LAYOUT_MOD_CUSTOM) ld->padding.x = size;
	else ld->padding.x = ld->paddings[flag].x;
}
void Gui::layoutSetYPadding(int flag, float size) {
	if(flag == LAYOUT_MOD_CUSTOM) ld->padding.y = size;
	else ld->padding.y = ld->paddings[flag].y;
}

void Gui::layoutScissorPush(LayoutData ld, Rect scissor) {
	scissorPush(scissor);
	layoutPush(ld);
}
void Gui::layoutScissorPush(Rect region, Rect scissor) {
	scissorPush(scissor);
	layoutPush(region);
}

void Gui::layoutScissorPop(bool updateY) {
	scissorPop();
	layoutPop();
}

//

void Gui::drawText(Rect r, char* text, Vec2i align, Rect scissor, TextSettings settings, float borderSize) {
	Vec2 pos = r.c() + (r.dim()/2) * vec2(align);

	Rect scissorRect = pfGetRectScissor(r,scissor).expand(-vec2(borderSize*2));
	pfScissorTestScreen(scissorRect);
	
	int wrapWidth = settings.cull ? scissorRect.right - pos.x : 0;
	pfDrawText(text, pos, align, settings, wrapWidth);
	// guiDrawText(text, pos, align, settings);

	pfScissorTestScreen(scissor);
}

void Gui::drawBox(Rect r, Rect scissor, BoxSettings settings, bool rounding) {
	pfScissorTestScreen(scissor);

	if(rounding) r = round(r);

	bool hasBorder = settings.borderColor.a != 0 ? true : false;
	int borderSize = 1;

	if(settings.color.a != 0) {
		Rect br = r;
		if(hasBorder) br = br.expand( -vec2(borderSize));

		if(settings.vertGradientOffset) {
			Vec4 off = vec4(settings.vertGradientOffset, 0);
			pfDrawRectRoundedGradient(br, settings.color, settings.roundedCorner, off);

		} else {
			pfDrawRectRounded(br, settings.color, settings.roundedCorner);
		}
	}

	if(hasBorder) {
		pfLineWidth(1);
		pfDrawRectRoundedOutline(r, settings.borderColor, settings.roundedCorner);
	}
}

void Gui::drawTextBox(Rect r, char* text, Vec2i align, Rect scissor, TextBoxSettings settings) {
	// guiScissorTestScreen(scissor);
	drawBox(r, scissor, settings.boxSettings);

	if(align.x == -1) r.left += settings.sideAlignPadding;
	if(align.x == 1) r.right -= settings.sideAlignPadding;

	float borderSize = 0;
	if(settings.boxSettings.borderColor.a != 0) borderSize = 1;

	drawText(r, text, align, scissor, settings.textSettings, borderSize);
}

void Gui::drawTextEditBox(char* text, Rect textRect, bool active, Rect scissor, TextEditVars editVars, TextEditSettings editSettings) {

	BoxSettings* boSettings = &editSettings.textBoxSettings.boxSettings;
	TextSettings* textSettings = &editSettings.textBoxSettings.textSettings;

	Vec2 startPos = textRect.l() + vec2(editSettings.textOffset,0);
	if(active) startPos += editVars.scrollOffset;

	drawBox(textRect, scissor, *boSettings);

	// guiScissorTestScreen(rectExpand(scissor, vec2(-1,-1)));
	pfScissorTestScreen(pfGetRectScissor(textRect, scissor).expand(vec2(-2,-2)));
	// guiScissorTestScreen(getRectScissor(textRect, scissor));

	if(active) text = editSettings.textBuffer;

	Vec2i align = vec2i(-1,0);
	if(active) {
		// Selection.
		pfDrawTextSelection(text, textSettings->font, startPos, editVars.cursorIndex, editVars.markerIndex, editSettings.colorSelection, align);
	}

	if(!strEmpty(editSettings.defaultText) && strEmpty(text) && !active) {
		TextSettings defaultTextSettings = *textSettings;
		defaultTextSettings.color = editSettings.defaultTextColor;
		pfDrawText(editSettings.defaultText, textRect.c(), vec2i(0,0), defaultTextSettings);

	} else pfDrawText(text, startPos, align, *textSettings);

	if(active) {
		// Cursor.
		Vec2 cPos = textIndexToPos(text, textSettings->font, startPos, editVars.cursorIndex, align);
		Rect cRect = rectCenDim(cPos, vec2(editSettings.cursorWidth, textSettings->font->height* editSettings.cursorHeightMod));

		if(editVars.cursorIndex == strLen(text) && editVars.cursorIndex == editVars.markerIndex) {
			cRect = cRect.trans(vec2(2,0)); // Small offset for looks.
		}

		Vec4 cCursor = editSettings.colorCursor;
		cCursor.a = (cos(editVars.cursorTimer) + 1)/2.0f;

		drawBox(cRect, scissor, ::boxSettings(cCursor));
	}

	pfScissorTestScreen(scissor);
}

void Gui::drawTextEditBox(void* val, int mode, Rect textRect, bool active, Rect scissor, TextEditVars editVars, TextEditSettings editSettings) {
	return drawTextEditBox(mode == EDIT_MODE_INT ? fillString("%i", VDref(int, val)) : fillString("%f", VDref(float, val)), textRect, active, scissor, editVars, editSettings);
}
void Gui::drawTextEditBox(int number, Rect textRect, bool active, Rect scissor, TextEditVars editVars, TextEditSettings editSettings) {
	return drawTextEditBox(fillString("%i", number), textRect, active, scissor, editVars, editSettings);
}
void Gui::drawTextEditBox(float number, Rect textRect, bool active, Rect scissor, TextEditVars editVars, TextEditSettings editSettings) {
	return drawTextEditBox(fillString("%f", number), textRect, active, scissor, editVars, editSettings);
}

enum {
	SLIDER_TYPE_INT = 0,
	SLIDER_TYPE_FLOAT,
};

void Gui::drawSlider(void* val, bool type, Rect br, Rect sr, Rect scissor, SliderSettings settings) {
	scissor = pfGetRectScissor(br, scissor);

	BoxSettings* boSettings = &settings.textBoxSettings.boxSettings;
	TextSettings* textSettings = &settings.textBoxSettings.textSettings;

	// rectExpand(&sr, vec2(0,-settings.heightOffset*2));
	sr = sr.expand(vec2(-settings.heightOffset*2,-settings.heightOffset*2));

	drawBox(br, scissor, *boSettings);
	if(settings.lineColor.a > 0 && settings.lineWidth > 0) {
		pfLineWidth(settings.lineWidth);
		pfDrawLine(br.l(), br.r(), settings.lineColor);
	}

	BoxSettings sliderBoxSettings = ::boxSettings(settings.color, settings.rounding, settings.borderColor);
	int border = 1;
	scissor = scissor.expand(vec2(-border*2));

	drawBox(sr, scissor, sliderBoxSettings);

	char* text = type == SLIDER_TYPE_FLOAT ? fillString("%f", *((float*)val)) : fillString("%i", *((int*)val)) ;

	// Position text outside of slider rect.
	{
		int border = 1;
		br = br.expand(-vec2(border*2));
		Vec2 tp = br.c();

		float off = settings.heightOffset;
		Vec2 tDim = getTextDim(text, textSettings->font);
		float l = tp.x - tDim.w/2 - off;
		float r = tp.x + tDim.w/2 + off;

		Vec2i align = vec2i(0,0);

		// Test doesn't fit.
		if(tDim.w > br.dim().w-off*2) {
			tp = br.c();
			align = vec2i(0,0);
		} else {
			if(sr.right < l || sr.left > r) {
				// Text fits in middle.
				tp = br.c();
				align = vec2i(0,0);
			} else {
				Vec2 brRight = br.r() - vec2(off,0);
				Vec2 brLeft = br.l() + vec2(off,0);

				if(sr.cx()+0.1f < br.cx()) {
					Vec2 nextToSlider = sr.r() + vec2(off,0);
					// Clamp.
					if(brRight.x - nextToSlider.x < tDim.w) {
						tp = brRight;
						align = vec2i(1,0);
					} else {
						// Slider left of center.
						tp = nextToSlider;
						align = vec2i(-1,0);
					}
				} else {
					Vec2 nextToSlider = sr.l() - vec2(off,0);
					// Clamp.
					if(nextToSlider.x - brLeft.x < tDim.w) {
						tp = brLeft;
						align = vec2i(-1,0);
					} else {
						// Slider right of center.
						tp = nextToSlider;
						align = vec2i(1,0);
					}
				}
			}
		}

		pfDrawText(text, tp, align, *textSettings);
	}
}
void Gui::drawSlider(float val, Rect br, Rect sr, Rect scissor, SliderSettings settings) {
	return drawSlider(&val, SLIDER_TYPE_FLOAT, br, sr, scissor, settings);
}
void Gui::drawSlider(int val, Rect br, Rect sr, Rect scissor, SliderSettings settings) {
	return drawSlider(&val, SLIDER_TYPE_INT, br, sr, scissor, settings);
}

//
// @Widgets.
//

void Gui::qText(Rect r, char* t, Vec2i align, TextSettings* settings) {
	if(pfGetRectScissor(scissor, r).empty()) return;

	TextSettings set = settings == 0 ? sText : *settings;
	drawText(r, t, align, scissor, set);
}
void Gui::qText(Rect r, char* t, TextSettings* settings) {
	qText(r, t, vec2i(0,0), settings);
}

void Gui::qBox(Rect r, BoxSettings* settings) {
	if(pfGetRectScissor(scissor, r).empty()) return;

	BoxSettings set = settings == 0 ? sBox : *settings;
	drawBox(r, scissor, set);
}

void Gui::qTextBox(Rect r, char* t, Vec2i align, TextBoxSettings* settings) {
	if(pfGetRectScissor(scissor, r).empty()) return;

	TextBoxSettings set = settings == 0 ? sTextBox : *settings;
	drawTextBox(r, t, align, scissor, set);
}
void Gui::qTextBox(Rect r, char* t, TextBoxSettings* settings) {
	return qTextBox(r, t, vec2i(0,0), settings);
}

bool Gui::_qButton(Rect r, char* text, Vec2i align, TextBoxSettings* settings, bool highlightOnActive) {
	r = round(r);

	Rect intersection = pfGetRectScissor(scissor, r);
	bool active = goButtonAction(intersection);
	if(!intersection.empty()) {
		TextBoxSettings set = settings == 0 ? sButton : *settings;
		set.boxSettings.color += highlightOnActive?colorModB():colorMod();
		// set.boxSettings.color += newGuiInactiveColorMod(setInactive);

		drawTextBox(r, text, align, scissor, set);
	}

	if(isHot()) {
		setCursor(IDC_HAND);
	}
	handleInactive();

	return active;
}

bool Gui::qButton(Rect r, char* text, Vec2i align, TextBoxSettings* settings) {
	return _qButton(r, text, align, settings == 0 ? &sButton : settings, true);
}
bool Gui::qButton(Rect r, char* text, TextBoxSettings* settings) {
	return qButton(r, text, vec2i(0,0), settings);
}
bool Gui::qButton(Rect r, TextBoxSettings* settings) {
	return qButton(r, "", vec2i(0,0), settings);
}

bool Gui::qPButton(Rect r, char* text, Vec2i align, TextBoxSettings* settings) {
	return _qButton(r, text, align, settings == 0 ? &sButton : settings, false);
}
bool Gui::qPButton(Rect r, char* text, TextBoxSettings* settings) {
	return qPButton(r, text, vec2i(0,0), settings);
}
bool Gui::qPButton(Rect r, TextBoxSettings* settings) {
	return qPButton(r, "", vec2i(0,0), settings);
}

bool Gui::qCheckBox(Rect r, bool* value, Vec2i align, CheckBoxSettings* settings) {
	Vec2 dim = vec2(min(r.w(), r.h()));
	Rect cr;
	     if(align.x ==  0) cr = rectCenDim(r.c(), dim);
	else if(align.x == -1) cr = rectLDim(r.l(), dim);
	else if(align.x ==  1) cr = rectRDim(r.r(), dim);

	Rect intersection = pfGetRectScissor(scissor, cr);

	bool active = goButtonAction(intersection);
	if(cr.empty()) return false;
	if(!intersection.empty()) {
		if(active) *value = !(*value);

		CheckBoxSettings set = settings == 0 ? sCheckBox : *settings;
		set.boxSettings.color += colorMod();

		drawBox(cr, scissor, set.boxSettings);

		if(*value) {
			cr = round(cr);
			drawBox(cr.expand(vec2(-cr.w()*(1 - set.sizeMod))), scissor, boxSettings(set.color, set.boxSettings.roundedCorner/2));
		}

		if(isHot()) setCursor(IDC_HAND);
	}

	handleInactive();

	return active;
}
bool Gui::qCheckBox(Rect r, bool* value, CheckBoxSettings* settings) {
	return qCheckBox(r, value, vec2i(-1,0), settings);
}

bool Gui::_qTextEdit(Rect r, void* data, int varType, int maxSize, TextEditSettings* editSettings) {
	Rect intersect = pfGetRectScissor(scissor, r);

	TextEditSettings set = editSettings == 0 ? sEdit : *editSettings;
	TextSettings* textSettings = &set.textBoxSettings.textSettings;

	char* charData = (char*)data;
	int* intData = (int*)data;
	float* floatData = (float*)data;

	int event;
	if(varType == EDIT_MODE_CHAR) event = goTextEdit(intersect, zLevel, charData, set, maxSize);
	else if(varType == EDIT_MODE_INT) event = goTextEdit(intersect, zLevel, intData, set);
	else if(varType == EDIT_MODE_FLOAT) event = goTextEdit(intersect, zLevel, floatData, set);

	if(intersect.empty()) return false;

	// if(event == 0) set.textBoxSettings.boxSettings.color += newGuiColorMod(gui);

	bool active = event == 1 || event == 2;
	if(varType == EDIT_MODE_CHAR) drawTextEditBox(charData, r, active, scissor, editVars, set);
	else if(varType == EDIT_MODE_INT) drawTextEditBox(*intData, r, active, scissor, editVars, set);
	else if(varType == EDIT_MODE_FLOAT) drawTextEditBox(*floatData, r, active, scissor, editVars, set);

	if(isHot() || (isActive() && pointInRectEx(input->mousePosNegative, intersect))) setCursor(IDC_IBEAM);

	if(event == 3) return true;

	return false;
}
bool Gui::qTextEdit(Rect r, char* data, int maxSize, TextEditSettings* editSettings) {
	return _qTextEdit(r, data, EDIT_MODE_CHAR, maxSize, editSettings);
}
bool Gui::qTextEdit(Rect r, int* data, TextEditSettings* editSettings) {
	return _qTextEdit(r, data, EDIT_MODE_INT, 0, editSettings);
}
bool Gui::qTextEdit(Rect r, float* data, TextEditSettings* editSettings) {
	return _qTextEdit(r, data, EDIT_MODE_FLOAT, 0, editSettings);
}

float Gui::sliderGetMod(int type, SliderSettings* settings, int mod) {
	float modifier = mod;
	if(input->keysDown[KEYCODE_SHIFT] && input->keysDown[KEYCODE_CTRL]) modifier *= 100;
	else if(input->keysDown[KEYCODE_SHIFT]) modifier *= 10;
	else if(input->keysDown[KEYCODE_CTRL]) modifier /= 10;
	
	if(type == SLIDER_TYPE_INT) {
		int mod = settings->mouseWheelModInt * modifier;
		return mod;

	} else {
		float mod = settings->mouseWheelModFloat * modifier;
		return mod;
	}
}

bool Gui::qSlider(Rect r, int type, void* val, void* min, void* max, SliderSettings* settings) {

	bool typeIsInt = type == SLIDER_TYPE_INT;
	SliderSettings set = settings == 0 ? sSlider : *settings;

	bool result = false;
	bool editMode = false;

	storeIds(10);

	// Double click text edit mode.
	{
		Rect intersect = pfGetRectScissor(scissor, r);

		TextEditSettings edSet = sEdit;

		int event;
		flagSet(&edSet.flags, ESETTINGS_START_RIGHT);
		event = goTextEdit(intersect, zLevel, val, typeIsInt?EDIT_MODE_INT:EDIT_MODE_FLOAT, edSet, &editVars, 0, true);
		if(event == 1 || event == 2) {
			drawTextEditBox(val, typeIsInt?EDIT_MODE_INT:EDIT_MODE_FLOAT, r, event > 0, scissor, editVars, edSet);
			editMode = true;

			handleInactive();
			clearStoredIds();
			return false;

		} else if(event == 3) {
			result = true;
		}
	}

	// Mouse Wheel / Arrow Keys.
	{
		int mod = 0;
		if(goButtonAction(r, zLevel, Gui_Focus_MWheel)) {
			mod = input->mouseWheel;
		}
		if(goButtonAction(r, zLevel, input->keysPressed[KEYCODE_LEFT] || input->keysPressed[KEYCODE_RIGHT], Gui_Focus_ArrowKeys)) {
			mod = input->keysPressed[KEYCODE_LEFT]?-1:1;
		}

		if(mod) {
			float modValue = sliderGetMod(type, &set, mod);
			if(typeIsInt) VDref(int, val) += roundInt(modValue);
			else VDref(float, val) += modValue;
			result = true;
		}
	}

	// For apply after.
	void* valuePointer = val;
	int sliderId = incrementId();

	if(set.applyAfter && sliderId == sliderId) {
		if(typeIsInt) val = &editInt;
		else val = &editFloat;
	}

	// Right now we just handle int as if it was a float.

	if(typeIsInt) clamp((int*)val, VDref(int, min), VDref(int, max));
	else clamp((float*)val, VDref(float, min), VDref(float, max));

	float floatVal = typeIsInt ? VDref(int, val) : VDref(float, val);
	float floatMin = typeIsInt ? VDref(int, min) : VDref(float, min);
	float floatMax = typeIsInt ? VDref(int, max) : VDref(float, max);

	float sliderSize;
	if(typeIsInt) {
		sliderSize = (r.w() / (float)(VDref(int, max) - VDref(int, min) + 1));
		sliderSize = clampMin(sliderSize, set.minSize);

	} else {
		sliderSize = set.size;
	}

	Rect slider = calcSlider(floatVal, r, sliderSize, floatMin, floatMax, true);

	int event = goDragAction(pfGetRectScissor(scissor, slider));
	if(!pfTestRectScissor(scissor, r)) {

		handleInactive();
		clearStoredIds();
		return false;
	}

	if(event == 1 && set.useDefaultValue && input->doubleClick) {
		setNotActive(currentId());
		result = true;

		if(typeIsInt) VDref(int, val) = roundInt(set.defaultvalue);
		else VDref(float, val) = set.defaultvalue;

	} else {
		if(event == 1) {
			if(typeIsInt) editInt = VDref(int, val);
			else editFloat = VDref(float, val);

			if(set.applyAfter) {
				sliderId = sliderId;
			}

			mouseAnchor = input->mousePosNegative - slider.c();
		}
		if(event > 0) {
			Vec2 pos = input->mousePosNegative - mouseAnchor;
			floatVal = sliderGetValue(pos, r, sliderSize, floatMin, floatMax, true);

			if(set.resetDistance > 0) {
				if(abs(r.cy() - input->mousePosNegative.y) > set.resetDistance) {
					if(typeIsInt) floatVal = editInt;
					else floatVal = editFloat;
				}
			}

			if(typeIsInt) floatVal = roundInt(floatVal);
			slider = calcSlider(floatVal, r, sliderSize, floatMin, floatMax, true);

			if(input->keysDown[KEYCODE_SHIFT]) floatVal = roundf(floatVal);

			if(set.notifyWhenActive) result = true;
		}

		if(typeIsInt) VDref(int, val) = floatVal;
		else VDref(float, val) = floatVal;
	}

	set.color += colorMod();
	if(!editMode) {
		scissorPush(r);
		drawSlider(val, type, r, slider, scissor, set);
		scissorPop();
	}

	if(event == 3) {
		if(set.applyAfter) {
			if(typeIsInt) VDref(int, valuePointer) = editInt;
			else VDref(float, valuePointer) = editFloat;

			sliderId = 0;
		}
	
		result = true;
	}

	clearStoredIds();

	handleInactive();

	return result;
}

bool Gui::qSlider(Rect r, float* val, float min, float max, SliderSettings* settings) {
	return qSlider(r, SLIDER_TYPE_FLOAT, val, &min, &max, settings);
}
bool Gui::qSlider(Rect r, int* val, int min, int max, SliderSettings* settings) {
	return qSlider(r, SLIDER_TYPE_INT, val, &min, &max, settings);
}

void Gui::qScroll(Rect r, float height, float* scrollValue, float* yOffset, float* wOffset, ScrollRegionSettings* settings) {
	ScrollRegionSettings set = settings == 0 ? sScroll : *settings;
	int flags = set.flags;

	clampMin(&height, 0.0f);

	float scrollRegionHeight = r.h();

	float itemsHeight = height - scrollRegionHeight;
	clampMin(&itemsHeight, 0.0f);
	bool hasScrollbar = itemsHeight > 0;

	Rect scrollRegion = round(r);
	if(!hasScrollbar && flagGet(flags, SCROLL_DYNAMIC_HEIGHT)) {
		scrollRegion = scrollRegion.setB(scrollRegion.top - height);
	}
	float scrollBarWidth = hasScrollbar?set.scrollBarWidth:0;

	Rect itemRegion = scrollRegion.setR(scrollRegion.right - scrollBarWidth);
	Rect scrollBarRegion = scrollRegion.setL(scrollRegion.right - scrollBarWidth);
	scrollBarRegion = round(scrollBarRegion);

	pfScissorTestScreen(scissor.expand(vec2(0)));

	// if(!hasScrollbar) rectSetB(&scrollRegion, scrollRegion.top-height);
	pfDrawRect(scrollRegion, set.boxSettings.color);
	if(set.scrollBarColor.a != 0) pfDrawRect(scrollBarRegion, set.scrollBarColor);
	if(set.boxSettings.borderColor.a != 0) {
		pfLineWidth(1);
		pfDrawRectOutline(scrollRegion, set.boxSettings.borderColor);
	}

	float sliderSize = set.sliderSize;
	if(sliderSize == 0) sliderSize = (scrollBarRegion.h() / (scrollRegion.h()+itemsHeight)) * scrollBarRegion.h();
	clampMin(&sliderSize, min(set.sliderSizeMin, scrollBarRegion.h()));

	storeIds(3);
	defer { clearStoredIds(); };

	if(hasScrollbar) {
		// Scroll with mousewheel.
		if(flagGet(flags, SCROLL_MOUSE_WHEEL))
		{
			if(goButtonAction(pfGetRectScissor(scissor, scrollRegion), Gui_Focus_MWheel)) {
				float scrollAmount = set.scrollAmount;
				if(input->keysDown[KEYCODE_SHIFT]) scrollAmount *= 2;

				(*scrollValue) += input->mouseWheel*scrollAmount;
			}
		}

		// Scroll with background drag.
		if(flagGet(flags, SCROLL_BACKGROUND))
		{
			int event = goDragAction(pfGetRectScissor(scissor, itemRegion));
			if(event == 1) mouseAnchor.y = input->mousePos.y - (*scrollValue);
			if(event > 0) (*scrollValue) = input->mousePos.y - mouseAnchor.y;
		}

		int sliderId;
		// Scroll with handle.
		if(flagGet(flags, SCROLL_SLIDER))
		{
			Rect slider = calcSliderScroll((*scrollValue), scrollBarRegion, sliderSize, -itemsHeight, 0.0f, false);

			int event = goDragAction(pfGetRectScissor(scissor, slider));
			sliderId = currentId();
			if(event == 1) mouseAnchor = input->mousePosNegative - slider.t();
			if(event > 0) {
				(*scrollValue) = sliderGetValueScroll(input->mousePosNegative - mouseAnchor, scrollBarRegion, sliderSize, -itemsHeight, 0.0f, false);
			}
		}

		(*scrollValue) = clamp((*scrollValue), -itemsHeight, 0.0f);
		Rect slider = calcSliderScroll((*scrollValue), scrollBarRegion, sliderSize, -itemsHeight, 0.0f, false);

		slider = round(slider.expand(-set.sliderMargin*2));
		if(hasScrollbar) pfDrawRectRounded(slider, set.sliderColor + colorModId(sliderId), set.sliderRounding);

	} else {
		(*scrollValue) = 0;
	}

	set.border.x = clampMin(set.border.x, 1.0f);
	set.border.y = clampMin(set.border.y, 1.0f);

	if(hasScrollbar) itemRegion.right -= set.scrollBarPadding;
	itemRegion = itemRegion.expand(-set.border*2);

	Rect sciss = pfGetRectScissor(scissor, itemRegion);

	// Vec2 layoutStartPos = vec2(itemRegion.left, itemRegion.top + (*scrollValue)*itemsHeight);
	// layoutScissorPush(itemRegion, sciss);
	// ld->pos = layoutStartPos;

	scissorPush(round(sciss.expand(3)));

	*yOffset = -(*scrollValue);
	*wOffset = hasScrollbar ? scrollBarWidth + set.scrollBarPadding : 0;
}

void Gui::qScrollEnd() {
	// layoutScissorPop();
	scissorPop();
}

bool Gui::qComboBox(Rect r, int* index, char** strings, int stringCount, TextBoxSettings* settings) {

	Rect intersection = pfGetRectScissor(scissor, r);
	bool active = goButtonAction(intersection);
	if(intersection.empty()) return false;

	if(isHot()) setCursor(IDC_HAND);

	TextBoxSettings set = settings == 0 ? sComboBox : *settings;
	set.boxSettings.color += colorMod();

	bool updated = false;

	// Mouse Wheel / Arrow Keys.
	{
		int mod = 0;
		if(goButtonAction(r, zLevel, Gui_Focus_MWheel)) {
			mod = -input->mouseWheel;
		}
		bool keys = input->keysPressed[KEYCODE_LEFT] || input->keysPressed[KEYCODE_RIGHT] || input->keysPressed[KEYCODE_UP] || input->keysPressed[KEYCODE_DOWN];
		if(goButtonAction(r, zLevel, keys, Gui_Focus_ArrowKeys)) {
			mod = (input->keysPressed[KEYCODE_LEFT] || input->keysPressed[KEYCODE_UP])?-1:1;
		}
		if(mod) {
			*index += mod;
			if(*index < 0 || *index > stringCount-1) updated = false;
			else updated = true;

			clamp(index, 0, stringCount-1);
			comboBoxData.index = *index;
		}
	}

	if(active) {
		PopupData pd = {};
		pd.type = POPUP_TYPE_COMBO_BOX;
		pd.id = currentId();
		// pd.r = rectTLDim(rectBL(r), vec2(rectW(r), set.textSettings.font->height*cData.count));
		pd.r = rectTLDim(r.bl(), vec2(r.w(), 0));
		pd.settings = sBox;
		pd.border = vec2(5,5);

		popupPush(pd);

		comboBoxIndex = index;
		comboBoxData = {*index, strings, stringCount};
	}

	// Check if popup is active.
	bool popupActive = false;
	for(int i = 0; i < popupStackCount; i++) {
		if(currentId() == popupStack[i].id) {
			popupActive = true;
			break;
		}
	}

	if(comboBoxData.finished && popupActive) {
		comboBoxData.finished = false;
		popupPop();
		updated = true;
	}

	{
		drawBox(r, scissor, set.boxSettings);

		Rect mainRect = r.expand(vec2(-set.sideAlignPadding*2,0));
		float fontHeight = set.textSettings.font->height;

		float triangleSizeMod = 1.0f/4.0f;
		float triangleOffsetMod = 0.25f;

		float triangleSize = fontHeight * triangleSizeMod;
		float triangleOffset = fontHeight * triangleOffsetMod;
		float triangleRectWidth = fontHeight * 0.5f;

		char* text = strings[*index];

		#if 0
		// float textWidth = getTextDim(text, set.textSettings.font).w+4;
		// float fullWidth = textWidth + triangleRectWidth + triangleOffset;
		// if(rectW(mainRect) > fullWidth) rectSetW(&mainRect, fullWidth);
		#endif 

		float textRectWidth = mainRect.w()-triangleRectWidth-triangleOffset;

		Rect textRect = mainRect.rSetR(textRectWidth);
		Rect triangleRect = mainRect.rSetL(triangleRectWidth);

		scissorPush(r.expand(-2));

		drawText(textRect, text, vec2i(-1,0), scissor, set.textSettings);

		// Vec2 dir = popupActive ? vec2(0,-1) : vec2(1,0);
		Vec2 dir = vec2(0,-1);
		pfDrawTriangle(triangleRect.c(), triangleSize, dir, set.textSettings.color);

		scissorPop();
	}

	return updated;
}

bool Gui::qComboBox(Rect r, int* index, void* data, int memberSize, int count, char* (*getName) (void* a), TextBoxSettings* settings) {

	char** names = getTArray(char*, count);
	for(int i = 0; i < count; i++) {
		char* name = getName(((char*)data) + i*memberSize);
		names[i] = getTStringCpy(name);
	}

	return qComboBox(r, index, names, count, settings);
}

//

void Gui::popupSetup() {

	zLevel = 5;

	if(popupStackCount > 0) {

		TextBoxSettings s = sTextBox;
		s.boxSettings.color.a = 0;
		s.boxSettings.borderColor.a = 0;

		// Hack. We ignore the mouseclick that probably created the popup.
		if(test == 1 && input->mouseButtonPressed[0])
		{
			bool overPopup = false;
			for(int i = 0; i < popupStackCount; i++) {
				if(pointInRectEx(input->mousePosNegative, popupStack[i].rCheck)) {
					overPopup = true;
					break;
				}
			}
			if(!overPopup) {
				popupStackCount = 0;
			}
		}
		if(input->mouseButtonPressed[0]) test = 1;


		// if(goButtonAction(getScreenRect(windowSettings))) {
		// 	popupStackCount = 0;
		// }

		// // Capture all mouse clicks.
		// int id = incrementId();
		// newGuiSetHotAll(gui, id, zLevel);
		// int focus[] = {Gui_Focus_MLeft, Gui_Focus_MRight, Gui_Focus_MMiddle};
		// for(int i = 0; i < arrayCount(focus); i++) {
		// 	setActive(id, inputFromFocus(focus[i]), focus[i]);
		// 	newGuiSetNotActiveWhenActive(gui, id);			
		// }

		// if(gotActive(id)) popupStackCount = 0;
	} else {
		menuActive = false;
		test = 0;
	}

	updateComboBoxPopups();
}

void Gui::updateComboBoxPopups() {
	float popupMinWidth = 80;
	float popupMaxWidth = 300;

	for(int i = 0; i < popupStackCount; i++) {
		PopupData pd = popupStack[i];
		int popupIndex = i;

		if(pd.type == POPUP_TYPE_COMBO_BOX) {
			ComboBoxData cData = comboBoxData;
			BoxSettings ps = sPopup;

			float comboboxPopupTopOffset = 2;

			Font* font = sText.font;
			float fontHeight = font->height;
			float padding = sComboBox.sideAlignPadding;
			// float topBottomPadding = padding*0.3f;
			float topBottomPadding = 0;

			float eh = fontHeight * 1.2f;

			float border = 0;
			if(ps.borderColor.a > 0) border = 1;

			float maxWidth = 0;
			for(int i = 0; i < cData.count; i++) {
				float w = getTextDim(cData.strings[i], font).w;
				maxWidth = max(maxWidth, w);
			}
			maxWidth += padding*2 + 4;
			clampMin(&maxWidth, popupMinWidth);

			float popupWidth = max(maxWidth, pd.r.w());
			float popupHeight = eh * cData.count + border*2 + topBottomPadding*2;

			Rect r = rectTDim(pd.r.t()-vec2(0,comboboxPopupTopOffset), vec2(popupWidth, popupHeight));

			// Clamp rect to window rect.
			{
				Vec2 offset = rectInsideRectClamp(r, getScreenRect(windowSettings));
				r = r.trans(offset);
			}
			
			popupStack[popupIndex].rCheck = r;

			// Shadow.
			Rect shadowRect = r.trans(vec2(1,-1)*padding*0.5f);
			pfDrawRect(shadowRect, vec4(0,0.8f));

			// Background.
			drawBox(r, scissor, sPopup);


			Rect lr = r.expand(vec2(-border*2));
			Vec2 p = lr.tl() + vec2(0,-topBottomPadding);

			float ew = lr.w();

			Vec4 cButton = sPopup.color;
			TextBoxSettings tbs = textBoxSettings(sComboBox.textSettings, boxSettings());
			tbs.sideAlignPadding = padding - border;

			BoxSettings bs = boxSettings(cButton);
			BoxSettings bsSelected = sEdit.textBoxSettings.boxSettings;

			for(int i = 0; i < cData.count; i++) {
				bool selected = cData.index == i;
				tbs.boxSettings = selected ? bsSelected : bs;

				Rect br = rectTLDim(p, vec2(ew, eh)); p.y -= eh;
				// if(selected) br = rectExpand(br, vec2(-padding*0.5f,0));

				if(qButton(br, cData.strings[i], vec2i(-1,0), &tbs)) {
					comboBoxData.index = i;
					comboBoxData.finished = true;

					*comboBoxIndex = comboBoxData.index;
				}
			}
		}
	}
}

//

bool guiWindow(Gui* gui, Rect* r, GuiWindowSettings settings) {
	Rect region = *r;
	Rect sr = settings.insideRect;
	bool insideClamp = !sr.empty();
	if(!insideClamp) sr = rectCenDim(vec2(0,0), vec2(FLT_MAX, FLT_MAX)); // So we don't clamp.

	if(settings.maxDim == vec2(0,0)) settings.maxDim = vec2(FLT_MAX,FLT_MAX);

	bool screenMouse = settings.mouseScreenCoordinates;

	float w = settings.borderSize;
	Vec2 p = screenMouse?gui->input->mousePosNegativeScreen:gui->input->mousePosNegative;

	int uiEvent = 0;
	bool changeCursor = false;
	Vec2i resizeAlign;

	bool move = false;
	bool somebodyActive = false;

	if((settings.resizableY || settings.resizableX) || settings.movable) {
		int eventLeftClick  = gui->goDragAction(region, gui->zLevel, Gui_Focus_MLeft, screenMouse);
		// int eventRightClick = gui->goDragAction(region, z, Gui_Focus_MRight, screenMouse);
		int eventRightClick = 0;

		int event = max(eventLeftClick, eventRightClick);
		if(event == 1) {
			gui->mode = gui->input->keysDown[KEYCODE_CTRL];

			if(!gui->mode) {
				POINT p; 
				GetCursorPos(&p);
				Vec2 mp = vec2(p.x, -p.y);

				gui->mouseAnchor = mp - region.tl();
				gui->mouseAnchor2 = region.dim();
			}
		}

		if(event > 0) {
			if(gui->mode) {
				uiEvent = event;
				resizeAlign = vec2i(1,-1);
				changeCursor = true;

			} else {
				move = true;

				POINT p; 
				GetCursorPos(&p);
				Vec2 mp = vec2(p.x, -p.y);

				Vec2 pos = mp - gui->mouseAnchor;

				if(insideClamp) {
					clamp(&pos.x, sr.left, sr.right - gui->mouseAnchor2.w);
					clamp(&pos.y, sr.bottom + gui->mouseAnchor2.h, sr.top);
				}
				region = rectTLDim(pos, gui->mouseAnchor2);
			}

			somebodyActive = true;
		}
	}

	float cornerSize = settings.cornerSize;
	for(int x = -1; x < 2; x++) {
		for(int y = -1; y < 2; y++) {
			if(x == 0 && y == 0) continue;

			Vec2i align = vec2i(x,y);
			Vec2 dim = vec2(align.x==0?region.w()-cornerSize*2+2:w+1, align.y==0?region.h()-cornerSize*2+2:w+1);
			Rect r = rectAlignDim(region, align, dim);

			int event;
			bool corner = abs(x) == 1 && abs(y) == 1;
			if(corner) {
				r = rectAlignDim(region, align, vec2(w+1,cornerSize));
				int cornerEvent1 = gui->goDragAction(r, gui->zLevel, Gui_Focus_MLeft, screenMouse);
				r = rectAlignDim(region, align, vec2(cornerSize,w+1));
				gui->id--;
				int cornerEvent2 = gui->goDragAction(r, gui->zLevel, Gui_Focus_MLeft, screenMouse);
				event = max(cornerEvent1, cornerEvent2);

			} else {
				event = gui->goDragAction(r, gui->zLevel, Gui_Focus_MLeft, screenMouse);
			}

			if(event > 0) {
				uiEvent = event;
				resizeAlign = align;
				somebodyActive = true;
			}

			if(event > 0 || (gui->isHot() && !somebodyActive)) {
				changeCursor = true;
				resizeAlign = align;
			}
		}
	}

	if(!move) {
		if(uiEvent == 1) {
			if(resizeAlign.x == -1) gui->mouseAnchor.x = p.x - region.left;
			if(resizeAlign.x ==  1) gui->mouseAnchor.x = p.x - region.right;
			if(resizeAlign.y ==  1) gui->mouseAnchor.y = p.y - region.top;
			if(resizeAlign.y == -1) gui->mouseAnchor.y = p.y - region.bottom;
		}

		if(uiEvent > 0) {
			if(settings.resizableX) {
				     if(resizeAlign.x == -1) region.left  = (p - gui->mouseAnchor).x;
				else if(resizeAlign.x ==  1) region.right = (p - gui->mouseAnchor).x;
			}

			if(settings.resizableY) {
				     if(resizeAlign.y == -1) region.bottom = (p - gui->mouseAnchor).y;
				else if(resizeAlign.y ==  1) region.top    = (p - gui->mouseAnchor).y;
			}
		}

		if(changeCursor) {
			if(resizeAlign == vec2i(-1,-1) || resizeAlign == vec2i(1, 1)) gui->setCursor(IDC_SIZENESW);
			if(resizeAlign == vec2i(-1, 1) || resizeAlign == vec2i(1,-1)) gui->setCursor(IDC_SIZENWSE);
			if(resizeAlign == vec2i(-1, 0) || resizeAlign == vec2i(1, 0)) gui->setCursor(IDC_SIZEWE);
			if(resizeAlign == vec2i( 0,-1) || resizeAlign == vec2i(0, 1)) gui->setCursor(IDC_SIZENS);
		}

    	// if(uiEvent > 0 && insideClamp) {
		if(uiEvent > 0) {
			Vec2 minDim = settings.minDim;
			Vec2 maxDim = settings.maxDim;

			if(settings.resizableX) {
				if(resizeAlign.x == -1) region.left = clamp(region.left, max(sr.left, region.right - maxDim.x), region.right - minDim.x);
				if(resizeAlign.x ==  1) region.right = clamp(region.right, region.left + minDim.x, min(sr.right, region.left + maxDim.x));
			}

			if(settings.resizableY) {
				if(resizeAlign.y == -1) region.bottom = clamp(region.bottom, max(sr.bottom, region.top - maxDim.y), region.top - minDim.y);
				if(resizeAlign.y ==  1) region.top = clamp(region.top, region.bottom + minDim.y, min(sr.top, region.bottom + maxDim.y));
			}
		}
	}

    // If window is resizing clamp panel.
	if(insideClamp) {
		Vec2 dim = region.dim();
		if(region.left   < sr.left)   region = region.trans( vec2(sr.left - region.left, 0) );
		if(region.right  > sr.right)  region = region.trans( vec2(sr.right - region.right, 0) );
		if(region.bottom < sr.bottom) region = region.trans( vec2(0, sr.bottom - region.bottom) );
		if(region.top    > sr.top)    region = region.trans( vec2(0, sr.top - region.top) );

		if(region.h() > sr.h()) {
			region.top = sr.top;
			region.bottom = sr.bottom;
		}
		if(region.w() > sr.w()) {
			region.left = sr.left;
			region.right = sr.right;
		}
	}

	*r = region;

	if(move || uiEvent) return true;

	return false;
}
