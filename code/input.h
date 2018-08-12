#pragma once

enum Keycode {
	KEYCODE_CTRL = 0,
	KEYCODE_CTRL_RIGHT,
	KEYCODE_SHIFT,
	KEYCODE_SHIFT_RIGHT,
	KEYCODE_ALT,
	KEYCODE_CAPS,
	KEYCODE_TAB,
	KEYCODE_SPACE,
	KEYCODE_RETURN,
	KEYCODE_ESCAPE,
	KEYCODE_BACKSPACE,
	KEYCODE_DEL,
	KEYCODE_HOME,
	KEYCODE_END,
	KEYCODE_PAGEUP,
	KEYCODE_PAGEDOWN,
	KEYCODE_UP,
	KEYCODE_DOWN,
	KEYCODE_LEFT,
	KEYCODE_RIGHT,

	KEYCODE_A,
	KEYCODE_B,
	KEYCODE_C,
	KEYCODE_D,
	KEYCODE_E,
	KEYCODE_F,
	KEYCODE_G,
	KEYCODE_H,
	KEYCODE_I,
	KEYCODE_J,
	KEYCODE_K,
	KEYCODE_L,
	KEYCODE_M,
	KEYCODE_N,
	KEYCODE_O,
	KEYCODE_P,
	KEYCODE_Q,
	KEYCODE_R,
	KEYCODE_S,
	KEYCODE_T,
	KEYCODE_U,
	KEYCODE_V,
	KEYCODE_W,
	KEYCODE_X,
	KEYCODE_Y,
	KEYCODE_Z,

	KEYCODE_0,
	KEYCODE_1,
	KEYCODE_2,
	KEYCODE_3,
	KEYCODE_4,
	KEYCODE_5,
	KEYCODE_6,
	KEYCODE_7,
	KEYCODE_8,
	KEYCODE_9,

	KEYCODE_F1,
	KEYCODE_F2,
	KEYCODE_F3,
	KEYCODE_F4,
	KEYCODE_F5,
	KEYCODE_F6,
	KEYCODE_F7,
	KEYCODE_F8,
	KEYCODE_F9,
	KEYCODE_F10,
	KEYCODE_F11,
	KEYCODE_F12,

	KEYCODE_COUNT,
};

#define WIN_KEY_NUMERIC_START 0x30
#define WIN_KEY_NUMERIC_END 0x39
#define WIN_KEY_LETTERS_START 0x41
#define WIN_KEY_LETTERS_END 0x5a
#define WIN_KEY_F_START 0x70
#define WIN_KEY_F_END 0x7B

struct Input {
	bool firstFrame;
	Vec2 mousePos;
	Vec2 mousePosNegative;
	Vec2 mousePosScreen;
	Vec2 mousePosNegativeScreen;

	Vec2 mouseDelta;
	int mouseWheel;
	bool mouseButtonPressed[8];
	bool mouseButtonDown[8];
	bool mouseButtonReleased[8];
	bool doubleClick;
	Vec2 doubleClickPos;

	Vec2 lastMousePos;

	bool keysDown[KEYCODE_COUNT];
	bool keysPressed[KEYCODE_COUNT];
	char inputCharacters[32];
	int inputCharacterCount;

	bool anyKey;

	bool closeWindow;
	bool maximizeWindow;
	bool minimizeWindow;
	bool resize;

	bool altEnter;
};
