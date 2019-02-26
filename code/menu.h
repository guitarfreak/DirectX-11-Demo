
Meta_Parse_Struct(0);
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

Meta_Parse_Struct(0);
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
