
struct GameSettings {
	bool fullscreen;
	bool vsync;
	int frameRateCap;
	float resolutionScale;
	float volume;
	float mouseSensitivity;
	int fieldOfView;
};

enum Game_Mode {
	GAME_MODE_MENU,
	GAME_MODE_LOAD,
	GAME_MODE_MAIN,
	GAME_MODE_TEST,
};

enum Menu_Screen {
	MENU_SCREEN_MAIN,
	MENU_SCREEN_SETTINGS,
};

struct MainMenu {
	bool gameRunning;

	float volume;

	int screen;
	int activeId;
	int currentId;

	float buttonAnimState;

	Font* font;
	Vec4 cOption;
	Vec4 cOptionActive;
	Vec4 cOptionShadow;
	Vec4 cOptionShadowActive;

	float optionShadowSize;

	bool pressedEnter;
	bool pressedEscape;
	bool pressedBackspace;
	bool pressedUp;
	bool pressedDown;
	bool pressedLeft;
	bool pressedRight;
};

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

	char* str = getTString(20);
	floatToStr(str, *value, precision);

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

	char* str = fillString("%i", *value);

	menuOptionDraw(menu, str, pos, vec2i(1,0), isSelected);

	if(active) {
		menu->buttonAnimState = 0;
		playSound("menuOption");
	}

	return active;
}

