
void menuSetInput(MainMenu* menu, Input* input) {
	menu->pressedEnter = input->keysPressed[KEYCODE_RETURN];
	menu->pressedEscape = input->keysPressed[KEYCODE_ESCAPE];
	menu->pressedBackspace = input->keysPressed[KEYCODE_BACKSPACE];
	menu->pressedUp = input->keysPressed[KEYCODE_UP];
	menu->pressedDown = input->keysPressed[KEYCODE_DOWN];
	menu->pressedLeft = input->keysPressed[KEYCODE_LEFT];
	menu->pressedRight = input->keysPressed[KEYCODE_RIGHT];
}

void menuOptionDraw(MainMenu* menu, char* text, Vec2 pos, Vec2i alignment, bool isActive) {
	Vec4 textColor = isActive ? menu->cOptionActive : menu->cOption;
	Vec4 shadowColor = isActive ? menu->cOptionShadowActive : menu->cOptionShadow;

	TextSettings ts = textSettings(menu->font, textColor, TEXTSHADOW_MODE_SHADOW, vec2(1,-1), menu->optionShadowSize, shadowColor);
	drawText(text, pos, alignment, ts);
}

bool menuOption(MainMenu* menu, char* text, Vec2 pos, Vec2i alignment) {

	bool isActive = menu->activeId == menu->currentId;
	menuOptionDraw(menu, text, pos, alignment, isActive);

	bool result = menu->pressedEnter && menu->activeId == menu->currentId;

	menu->currentId++;

	return result;
}

bool menuOptionBool(MainMenu* menu, Vec2 pos, bool* value) {
	bool isSelected = menu->activeId == menu->currentId-1;

	bool active = false;
	if(isSelected) {
		if(menu->pressedEnter) {
			*value = !(*value);
			active = true;
		}
		if(menu->pressedLeft) {
			if(*value == true) {
				*value = false;
				active = true;
			}
		}
		if(menu->pressedRight) {
			if(*value == false) {
				*value = true;
				active = true;
			}
		}
	}

	char* str;
	if((*value) == true) str = "True";
	else str = "False";

	menuOptionDraw(menu, str, pos, vec2i(1,0), isSelected);

	if(active) {
		menu->buttonAnimState = 0;
		playSound("menuOption");
	}

	return active;
}

bool menuOptionSliderFloat(MainMenu* menu, Vec2 pos, float* value, float rangeMin, float rangeMax, float step, float precision) {
	bool isSelected = menu->activeId == menu->currentId-1;

	bool active = false;

	float valueBefore = *value;
	if(isSelected) {
		if(menu->pressedLeft)  (*value) -= step;
		if(menu->pressedRight) (*value) += step;
		clamp(value, rangeMin, rangeMax);
	}

	if(valueBefore != (*value)) active = true;

	char* str = fString("%.*f", (int)precision, *value);
	menuOptionDraw(menu, str, pos, vec2i(1,0), isSelected);

	if(active) {
		menu->buttonAnimState = 0;
		playSound("menuOption");
	}

	return active;
}

bool menuOptionSliderInt(MainMenu* menu, Vec2 pos, int* value, int rangeMin, int rangeMax, int step) {
	bool isSelected = menu->activeId == menu->currentId-1;

	bool active = false;

	int valueBefore = *value;
	if(isSelected) {
		if(menu->pressedLeft)  (*value) -= step;
		if(menu->pressedRight) (*value) += step;
		clamp(value, rangeMin, rangeMax);
	}

	if(valueBefore != (*value)) active = true;

	char* str = fString("%i", *value);

	menuOptionDraw(menu, str, pos, vec2i(1,0), isSelected);

	if(active) {
		menu->buttonAnimState = 0;
		playSound("menuOption");
	}

	return active;
}

//

void renderMenuBackground() {
	dxSetFrameBuffer("MenuBackground", theGState->gSettings->cur3dBufferRes, 1);

	dxSetShader(Shader_Primitive);
	dxGetShaderVars(Primitive)->viewProj = theGState->gMats.ortho * theGState->gMats.view2d;
	dxPushShaderConstants(Shader_Primitive);

	Rect sr = theGState->screenRect;

	int resolutionIndex = 3;
	char* cBuf = fString("Bloom_%i", resolutionIndex);
	char* oBuf = fString("Bloom2_%i", resolutionIndex);

	Vec2 dim = vec2(dxGetFrameBuffer(cBuf)->dim);

	{
		dxBindFrameBuffer(cBuf, 0);
		dxDrawRect(rectTLDim(vec2(0,0), dim), vec4(1,1,1,1), dxGetFrameBuffer("3dNoMsaa")->shaderResourceView);
	}

	{
		dxSetShader(Shader_Bloom);

		int blurSteps = 1;
		for(int i = 0; i < blurSteps*2; i++) {
			dxBindFrameBuffer(oBuf, 0);

			dxGetShaderVars(Bloom)->mode = 1 + (i%2);
			dxPushShaderConstants(Shader_Bloom);

			dxDrawRect(rectTLDim(vec2(0,0), dim), vec4(1,1,1,1), dxGetFrameBuffer(cBuf)->shaderResourceView);

		 	swap(&cBuf, &oBuf);
		}
	}

	{
		resolutionIndex = 0;
		char* oldCBuf = cBuf;
		cBuf = fString("Bloom_%i", resolutionIndex);
		oBuf = fString("Bloom2_%i", resolutionIndex);

		dxSetShader(Shader_Primitive);
		dxBindFrameBuffer(cBuf);
		dxDrawRect(sr, vec4(1), dxGetFrameBuffer(oldCBuf)->shaderResourceView);
	}
	
	{
		dxSetShader(Shader_Bloom);

		int blurSteps = 3;
		for(int i = 0; i < blurSteps*2; i++) {
			dxBindFrameBuffer(oBuf, 0);

			dxGetShaderVars(Bloom)->mode = 1 + (i%2);
			dxPushShaderConstants(Shader_Bloom);

			dxDrawRect(sr, vec4(1,1,1,1), dxGetFrameBuffer(cBuf)->shaderResourceView);

		 	swap(&cBuf, &oBuf);
		}
	}

	dxSetShader(Shader_Primitive);
	dxBindFrameBuffer("MenuBackground");

	float darkenMod = 0.4f;
	// dxDrawRect(sr, vec4(darkenMod,1), dxGetFrameBuffer(cBuf)->shaderResourceView);
	dxDrawRect(sr, vec4(vec3(1,0.5f,0.5f)*darkenMod,1), dxGetFrameBuffer(cBuf)->shaderResourceView);
}

//

void menuUpdate(AppData* ad, WindowSettings* ws, float fontHeightScaled, bool* toggleFullscreen, bool* isRunning) {
	TIMER_BLOCK();

	dxViewPort(ws->currentRes);
	dxGetShaderVars(Primitive)->viewProj = theGState->gMats.ortho * theGState->gMats.view2d;
	dxSetShaderAndPushConstants(Shader_Primitive);
	dxBindFrameBuffer("2dMsaa");

	//

	Rect sr = getScreenRect(ws);
	Vec2 top = sr.t();
	float rHeight = sr.h();
	float rWidth = sr.w();

	int titleFontHeight = fontHeightScaled * 6.0f;
	int optionFontHeight = titleFontHeight * 0.45f;
	Font* titleFont = getFont("Merriweather-Regular.ttf", titleFontHeight);
	Font* font = getFont("LiberationSans-Regular.ttf", optionFontHeight);

	Vec4 cBackground = vec4(hslToRgbf(0.93f,0.5f,0.13f),1);
	Vec4 cTitle = vec4(1,1);
	Vec4 cTitleShadow = vec4(0,0,0,1);
	Vec4 cOption = vec4(0.7f,1);
	Vec4 cOptionActive = vec4(0.95f,1);
	Vec4 cOptionShadow1 = vec4(0,1);
	Vec4 cOptionShadow2 = vec4(hslToRgbf(0.55f,0.5f,0.5f), 1);
	Vec4 cOptionShadow = vec4(0,1);

	float titleShadowSize = titleFontHeight * 0.07f;
	float optionShadowSize = optionFontHeight * 0.07f;

	float buttonAnimSpeed = 4;

	float optionOffset = optionFontHeight*1.2f;
	float titleOffset = 0.2f;
	float settingsOffset = optionFontHeight * 6;

	TextSettings tsTitle = textSettings(titleFont, cTitle, TEXTSHADOW_MODE_SHADOW, vec2(1,-1), titleShadowSize, cTitleShadow);


	MainMenu* menu = &ad->menu;
	Input* input = &ad->input;

	menuSetInput(menu, input);
	menu->volume = ad->volumeMenu;

	bool selectionChange = false;

	if(input->keysPressed[KEYCODE_DOWN]) {
		playSound("menuSelect");
		menu->activeId++;
		selectionChange = true;
	}
	if(input->keysPressed[KEYCODE_UP]) {
		playSound("menuSelect");
		menu->activeId--;
		selectionChange = true;
	}

	if(menu->currentId > 0)
		menu->activeId = mod(menu->activeId, menu->currentId);

	{
		if(selectionChange) {
			menu->buttonAnimState = 0;
		}

		menu->buttonAnimState += ad->dt * buttonAnimSpeed;
		float anim = (cos(menu->buttonAnimState) + 1)/2.0f;
		anim = powf(anim, 0.5f);
		Vec4 cOptionShadowActive = vec4(0,1);
		cOptionShadowActive.rgb = lerp(anim, cOptionShadow1.rgb, cOptionShadow2.rgb);

		menu->currentId = 0;
		menu->font = font;
		menu->cOption = cOption;
		menu->cOptionActive = cOptionActive;
		menu->cOptionShadow = cOptionShadow;
		menu->cOptionShadowActive = cOptionShadowActive;
		menu->optionShadowSize = optionShadowSize;
	}

	// Draw blurred Background.
	{
		FrameBuffer* fb = dxGetFrameBuffer("MenuBackground");

		Vec2 filledDim = fitDim(sr.dim(), vec2(fb->dim));
		dxDrawRect(rectCenDim(sr.c(), filledDim), vec4(1,1), fb->shaderResourceView);
	}

	if(menu->screen == MENU_SCREEN_MAIN) {

		Vec2 p = top - vec2(0, rHeight*titleOffset);
		drawText("???", p, vec2i(0,0), tsTitle);

		bool gameRunning = menu->gameRunning;

		int optionCount = gameRunning ? 4 : 3;
		p.y = sr.c().y + ((optionCount-1)*optionOffset)/2;

		if(gameRunning) {
			if(menuOption(menu, "Resume", p, vec2i(0,0)) || 
			   input->keysPressed[KEYCODE_ESCAPE]) {
				input->keysPressed[KEYCODE_ESCAPE] = false;

				ad->newGameMode = GAME_MODE_MAIN;
			}
			p.y -= optionOffset;
		}

		if(menuOption(menu, "New Game", p, vec2i(0,0))) {
			playSound("gameStart");
			ad->newGameMode = GAME_MODE_LOAD;
			ad->newGame = true;
		}

		p.y -= optionOffset;
		if(menuOption(menu, "Settings", p, vec2i(0,0))) {
			playSound("menuPush");
			menu->screen = MENU_SCREEN_SETTINGS;
			menu->activeId = 0;
		}

		p.y -= optionOffset;
		if(menuOption(menu, "Exit", p, vec2i(0,0))) {
			*isRunning = false;
		}

	} else if(menu->screen == MENU_SCREEN_SETTINGS) {

		Vec2 p = top - vec2(0, rHeight*titleOffset);
		float leftX = sr.cx() - settingsOffset;
		float rightX = sr.cx() + settingsOffset;

		drawText("Settings", p, vec2i(0,0), tsTitle);

		int optionCount = 7;
		p.y = sr.c().y + ((optionCount-1)*optionOffset)/2;


		// List settings.

		GameSettings* settings = &ad->settings;


		p.x = leftX;
		menuOption(menu, "Fullscreen", p, vec2i(-1,0));

		bool tempBool = ws->fullscreen;
		if(menuOptionBool(menu, vec2(rightX, p.y), &tempBool)) {
			*toggleFullscreen = true;
		}

		//

		p.y -= optionOffset; p.x = leftX;
		menuOption(menu, "VSync", p, vec2i(-1,0));

		menuOptionBool(menu, vec2(rightX, p.y), &ws->vsync);

		//

		p.y -= optionOffset; p.x = leftX;
		menuOption(menu, "Max FPS", p, vec2i(-1,0));

		menuOptionSliderInt(menu, vec2(rightX, p.y), &ad->frameRateCap, ws->frameRate, ad->maxFrameRate, 1);

		//

		p.y -= optionOffset; p.x = leftX;
		menuOption(menu, "Resolution Scale", p, vec2i(-1,0));

		if(menuOptionSliderFloat(menu, vec2(rightX, p.y), &ad->gSettings.resolutionScale, 0.2f, 1, 0.1f, 1)) {
			ad->updateFrameBuffers = true;
		}

		//

		p.y -= optionOffset; p.x = leftX;
		menuOption(menu, "Volume", p, vec2i(-1,0));

		menuOptionSliderFloat(menu, vec2(rightX, p.y), &ad->audioState.masterVolume, 0.0f, 1, 0.1f, 1);

		//

		p.y -= optionOffset; p.x = leftX;
		menuOption(menu, "Mouse Sensitivity", p, vec2i(-1,0));

		menuOptionSliderFloat(menu, vec2(rightX, p.y), &ad->mouseSensitivity, 0.01f, 2, 0.01f, 2);

		//

		p.y -= optionOffset; p.x = leftX;
		menuOption(menu, "Field of View", p, vec2i(-1,0));

		menuOptionSliderInt(menu, vec2(rightX, p.y), &ad->gSettings.fieldOfView, 20, 90, 1);

		//

		p.y -= optionOffset;

		//

		p.y -= optionOffset;

		p.x = rWidth * 0.5f;
		p.y -= optionOffset;
		if(menuOption(menu, "Back", p, vec2i(0,0)) || 
		   input->keysPressed[KEYCODE_ESCAPE] ||
		   input->keysPressed[KEYCODE_BACKSPACE]) {
			playSound("menuPop");

			menu->screen = MENU_SCREEN_MAIN;
			menu->activeId = 0;
		}

	}
	
}

//

