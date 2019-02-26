
enum GuiFocus {
	Gui_Focus_MLeft = 0,
	Gui_Focus_MRight,
	Gui_Focus_MMiddle,
	Gui_Focus_MWheel,
	Gui_Focus_MPos,
	Gui_Focus_ArrowKeys,

	Gui_Focus_Size,
};

struct TextEditVars {
	int cursorIndex;
	int markerIndex;
	Vec2 scrollOffset;

	bool cursorChanged;

	float dt;
	float cursorTimer;

	bool wordSelectionMode;
	int wordSelectionStartIndex;
};

struct BoxSettings {
	Vec4 color;
	float roundedCorner;
	Vec4 borderColor;
	float borderSize; // Not used yet.
	Vec2 padding;

	float vertGradientOffset;
	// char* texture;
};

struct CheckBoxSettings {
	BoxSettings boxSettings;
	Vec4 color;
	float sizeMod;
};

struct TextBoxSettings {
	TextSettings textSettings;
	BoxSettings boxSettings;
	float sideAlignPadding;
};

enum {
	ESETTINGS_WRAPPING    = 1 << 0,
	ESETTINGS_SINGLE_LINE = 1 << 1,
	ESETTINGS_START_RIGHT = 1 << 2,
};

struct TextEditSettings {
	TextBoxSettings textBoxSettings;
	Vec4 defaultTextColor;

	char* textBuffer;

	int flags;

	float cursorWidth;
	float cursorHeightMod;

	char* defaultText;

	Vec4 colorSelection;
	Vec4 colorCursor;
	float cursorFlashingSpeed;

	float textOffset;

	int floatPrecision;
	bool dontCopy;
};

struct SliderSettings {
	TextBoxSettings textBoxSettings;

	float size;
	float minSize;
	float lineWidth;
	float rounding;
	float heightOffset;

	Vec4 color;
	Vec4 lineColor;

	int mouseWheelModInt;
	float mouseWheelModFloat;

	bool useDefaultValue;
	float defaultvalue;

	float resetDistance;
	
	bool applyAfter;
	bool notifyWhenActive;

	Vec4 borderColor;
};

struct ComboBoxData {
	int index;
	DArray<char> strBuilder;
	DArray<char*> strings;
	bool change;
};

enum {
	SCROLL_BACKGROUND     = 1 << 0,
	SCROLL_SLIDER         = 1 << 1,
	SCROLL_MOUSE_WHEEL    = 1 << 2,
	SCROLL_DYNAMIC_HEIGHT = 1 << 3,
};

struct ScrollRegionSettings {
	BoxSettings boxSettings;

	int flags;

	Vec2 border;
	float scrollBarWidth;
	float scrollBarPadding;
	Vec2 sliderMargin;
	float sliderRounding;

	// 0 means dynamic size.
	float sliderSize;
	float sliderSizeMin;

	float scrollAmount;

	Vec4 sliderColor;
	Vec4 scrollBarColor;
};

enum LayoutMod {
	LAYOUT_MOD_SMALL = 0,
	LAYOUT_MOD_MEDIUM,
	LAYOUT_MOD_LARGE,
	LAYOUT_MOD_SIZE,

	LAYOUT_MOD_CUSTOM,
};

struct LayoutData {
	Rect region;
	Vec2 pos;

	float lineHeight;
	Vec2 padding;

	int columnCount;
	int columnIndex;
	float columns[10];

	union {
		struct {
			float lineHeightSmall;
			float lineHeightMedium;
			float lineHeightLarge;

			Vec2 paddingSmall;
			Vec2 paddingMedium;
			Vec2 paddingLarge;
		};
		
		struct {
			float lineHeights[LAYOUT_MOD_SIZE];
			Vec2 paddings[LAYOUT_MOD_SIZE];
		};
	};
};

enum Popup_Type {
	POPUP_TYPE_COMBO_BOX = 0,
	POPUP_TYPE_COLOR_PICKER,
	POPUP_TYPE_ALPHA_PICKER,
	POPUP_TYPE_OTHER,
};

struct PopupData {
	int type;
	int id;
	char name[30];
	Rect rSource;
	Rect r;
	Rect rCheck;

	Vec2 p;
	float width;

	BoxSettings settings;
	Vec2 border;

	bool finished;
};

//

struct Gui {
	struct Timestamp {
		int id;
		float dt;
	};

	int id;

	int activeId;
	int gotActiveId;
	int wasActiveId;
	
	int hotId[Gui_Focus_Size];
	int contenderId[Gui_Focus_Size];
	int contenderIdZ[Gui_Focus_Size];

	// Decay effect.
	float hotDecayTime;
	Timestamp hotQueue[30];
	int hotQueueCount;

	// Used for text edits right now.
	int activeSignalId;

	int storedIds[20];
	int storedIdIndex;
	int storedIdCount;

	Input* input;
	WindowSettings* windowSettings;
	float dt;

	Vec4 colorModHot;
	Vec4 colorModActive;

	int menuId;
	bool menuActive;

	bool forceNoClear;

	bool disable;
	bool setInactiveX;
	bool discolorInactive;
	bool stayInactive;

	bool mouseInClient;

	int test;

	Vec2i tabIdRange;

	// Temp vars for convenience.

	Vec2 mouseAnchor, mouseAnchor2;
	
	int mode;

	char editText[100];
	int editInt;
	float editFloat;
	int editMaxSize;

	int sliderId;

	TextEditVars editVars;

	PopupData popupStack[10];
	int popupStackCount;
	int popupStartId;

	int* comboBoxIndex;
	ComboBoxData comboBoxData;

	Vec4 colorPickerColorCopy;
	Vec4 colorPickerColorStart;
	Vec4 colorPickerColor;

	//

	TextSettings sText;
	TextSettings sText2;
	BoxSettings sBox;
	TextBoxSettings sTextBox;
	TextBoxSettings sTextBox2;
	TextBoxSettings sButton;
	TextEditSettings sEdit;
	SliderSettings sSlider;
	ScrollRegionSettings sScroll;
	BoxSettings sPopup;
	TextBoxSettings sComboBox;
	CheckBoxSettings sCheckBox;

	int zLevel;

	Rect scissor;
	Rect scissorStack[10];
	int scissorStackIndex;

	LayoutData* ld;
	LayoutData layoutStack[10];
	int layoutStackIndex;

	LPCSTR currentCursor;

	//

	struct AppColors {
		Vec4 text = vec4(1,1);
		Vec4 button = vec4(0.42f,1);
		Vec4 background = vec4(0.33f,1);
		Vec4 menu = vec4(0.27f,1);
		Vec4 edit = vec4(0.23f,1);
		Vec4 border = vec4(0.21f,1);
		Vec4 outline = vec4(0.19f,1);
		Vec4 outlineBright = vec4(0.41f,1);
		Vec4 ledgeDark = vec4(0.16f,1);
		Vec4 ledgeBright = vec4(0.42f,1);
	};

	void defaultSettings(Font* font);

	void begin(Input* input, WindowSettings* ws, float dt = 0, bool mouseInClient = true);
	void end();
	int  advanceId();
	int  incrementId(); // "Increment" doesn't make sense anymore, but too lazy to change the name.
	void storeIds(int count);
	void clearStoredIds();
	int  currentId();
	bool isHot(int id = 0, int focus = 0);
	bool isActive(int id = 0);
	bool isHotOrActive(int id = 0, int focus = 0);
	bool gotActive(int id = 0);
	bool wasActive(int id = 0);
	bool isWasHotOrActive(int id = 0, int focus = 0);
	void clearActive();
	void setNotActive(int id);
	bool someoneActive();
	bool someoneHot();
	void setNotActiveWhenActive(int id);
	void setActive(int id, bool input, int focus = 0, bool ignoreHot = false);
	bool focusCanBeHot(int focus);
	void setHot(int id, float z, int focus = 0);
	void setHot(int id, int focus = 0);
	void setHotAll(int id, float z);
	void setHotAll(float z);
	void setHotAllMouseOver(int id, Rect r, float z);
	void setHotAllMouseOver(Rect r, float z);
	void setHotAllMouseOver(Rect r);
	void setHotMouseOver(int id, Vec2 mousePos, Rect r, float z, int focus = 0);
	void clearHot();
	void forceActive(int id);
	int  inputFromFocus(int focus, bool press = true);
	void setCursor(LPCSTR cursorType);
	void setInactive(bool discolor = true, bool stayInactive = false);
	bool isInactive();
	void setActive();
	void handleInactive();

	void setTabRange(int count = 0);

	//

	int buttonAction(int id, Rect r, float z, Vec2 mousePos, bool input, int focus = 0);
	int buttonAction(Rect r, float z, Vec2 mousePos, bool input, int focus = 0);
	int buttonAction(int id, Rect r, float z, bool input, int focus = 0);
	int buttonAction(int id, Rect r, float z, int focus = 0);
	int buttonAction(Rect r, float z, int focus = 0);
	int buttonAction(Rect r, float z, bool input, int focus = 0);

	bool goButtonAction(int id, Rect r, float z, bool input, int focus);
	bool goButtonAction(int id, Rect r, float z, int focus);
	bool goButtonAction(Rect r, float z, int focus = 0);
	bool goButtonAction(Rect r, float z, bool input, int focus = 0);
	bool goButtonAction(Rect r, int focus = 0);

	int dragAction(int id, Rect r, float z, Vec2 mousePos, bool input, bool inputRelease, int focus = 0);
	int dragAction(Rect r, float z, Vec2 mousePos, bool input, bool inputRelease, int focus = 0);
	int dragAction(Rect r, float z, int focus = 0);
	int dragAction(bool isHot, int focus = 0);

	int goDragAction(Rect r, float z, bool input, bool inputRelease, int focus = 0, bool screenMouse = false);
	int goDragAction(Rect r, float z, int focus = 0, bool screenMouse = false);
	int goDragAction(Rect r, int focus = 0, bool screenMouse = false);

	bool goMousePosAction(Rect r, float z);

	int goTextEdit(Rect textRect, float z, void* var, int mode, TextEditSettings editSettings, TextEditVars* editVars, int maxTextSize, bool doubleClick = false);
	int goTextEdit(Rect textRect, float z, char* text, TextEditSettings editSettings, int maxTextSize);
	int goTextEdit(Rect textRect, float z, int* number, TextEditSettings editSettings);
	int goTextEdit(Rect textRect, float z, float* number, TextEditSettings editSettings);

	Rect calcSlider(float value, Rect br, float size, float min, float max, bool horizontal = true);
	float sliderGetValue(Vec2 sliderCenter, Rect br, float size, float min, float max, bool horizontal = true);
	Rect calcSliderScroll(float value, Rect br, float size, float min, float max, bool horizontal = true);
	float sliderGetValueScroll(Vec2 sliderCenter, Rect br, float size, float min, float max, bool horizontal = true);
	//

	bool isHotQueued(int id, float* decay);

	Vec4 hotActiveColorMod(bool isHot, bool isActive, bool inactive, float hotDecay = 1.0f);
	Vec4 inactiveColorMod(bool inactive);
	Vec4 colorModId(int id, int focus = 0); // We assume you got an id first before calling this.
	Vec4 colorModBId(int id, int focus = 0);

	Vec4 colorMod(int focus = 0);
	Vec4 colorModB(int focus = 0);
	Vec4 inactiveColorMod();

	//

	void scissorPush(Rect scissor);
	void scissorPop();
	void popupPush(PopupData data);
	void popupPop();

	LayoutData* layoutPush(LayoutData layoutData);
	LayoutData* layoutPush(Rect region);
	LayoutData* layoutPush();
	LayoutData* layoutPop(bool updateY = true);
	void layoutNewLine(int flag = -1, float height = 0);
	void layoutRowArray(float* columns, int count);
	void layoutRow(float s0, float s1 = -1, float s2 = -1, float s3 = -1);
	Rect layoutGet(float width);
	Rect layoutGet();
	Rect layoutGetAll(float height);
	void layoutSetLineHeight(int flag, float height = 0);
	void layoutSetXPadding(int flag, float size = 0);
	void layoutSetYPadding(int flag, float size = 0);
	void layoutScissorPush(LayoutData ld, Rect scissor);
	void layoutScissorPush(Rect region, Rect scissor);
	void layoutScissorPop(bool updateY = true);

	//

	void drawText(Rect r, char* text, Vec2i align, Rect scissor, TextSettings settings, float borderSize = 0);
	void drawBox(Rect r, Rect scissor, BoxSettings settings, bool rounding = true);
	void drawTextBox(Rect r, char* text, Vec2i align, Rect scissor, TextBoxSettings settings);
	void drawTextEditBox(char* text, Rect textRect, bool active, Rect scissor, TextEditVars editVars, TextEditSettings editSettings);
	void drawTextEditBox(void* val, int mode, Rect textRect, bool active, Rect scissor, TextEditVars editVars, TextEditSettings editSettings);
	void drawTextEditBox(int number, Rect textRect, bool active, Rect scissor, TextEditVars editVars, TextEditSettings editSettings);
	void drawTextEditBox(float number, Rect textRect, bool active, Rect scissor, TextEditVars editVars, TextEditSettings editSettings);
	void drawSlider(void* val, bool type, Rect br, Rect sr, Rect scissor, SliderSettings settings);
	void drawSlider(float val, Rect br, Rect sr, Rect scissor, SliderSettings settings);
	void drawSlider(int val, Rect br, Rect sr, Rect scissor, SliderSettings settings);

	// Widgets.

	void qText(Rect r, char* t, Vec2i align, TextSettings* settings = 0);
	void qText(Rect r, char* t, TextSettings* settings = 0);

	void qBox(Rect r, BoxSettings* settings = 0);

	void qTextBox(Rect r, char* t, Vec2i align, TextBoxSettings* settings = 0);
	void qTextBox(Rect r, char* t, TextBoxSettings* settings = 0);

	bool _qButton(Rect r, char* text, Vec2i align, TextBoxSettings* settings, bool highlightOnActive);
	bool qButton(Rect r, char* text, Vec2i align, TextBoxSettings* settings = 0);
	bool qButton(Rect r, char* text, TextBoxSettings* settings = 0);
	bool qButton(Rect r, TextBoxSettings* settings = 0);

	bool qPButton(Rect r, char* text, Vec2i align, TextBoxSettings* settings = 0);
	bool qPButton(Rect r, char* text, TextBoxSettings* settings = 0);
	bool qPButton(Rect r, TextBoxSettings* settings = 0);

	bool qCheckBox(Rect r, bool* value, Vec2i align, CheckBoxSettings* settings = 0);
	bool qCheckBox(Rect r, bool* value, CheckBoxSettings* settings = 0);

	bool _qTextEdit(Rect r, void* data, int varType, int maxSize, TextEditSettings* editSettings = 0);
	bool qTextEdit(Rect r, char* data, int maxSize, TextEditSettings* editSettings = 0);
	bool qTextEdit(Rect r, int* data, TextEditSettings* editSettings = 0);
	bool qTextEdit(Rect r, float* data, TextEditSettings* editSettings = 0);

	float sliderGetMod(int type, SliderSettings* settings, int mod);
	int qSlider(Rect r, int type, void* val, void* min, void* max, SliderSettings* settings = 0);
	int qSlider(Rect r, float* val, float min, float max, SliderSettings* settings = 0);
	int qSlider(Rect r, int* val, int min, int max, SliderSettings* settings = 0);

	void qScroll(Rect r, float height, float* scrollValue, float* yOffset, float* wOffset, ScrollRegionSettings* settings = 0);
	void qScrollEnd();
	
	int qComboBox(Rect r, int* index, char** strings, int stringCount, TextBoxSettings* settings = 0);
	int qComboBox(Rect r, int* index, void* data, int memberSize, int count, char* (*getName) (void* a), TextBoxSettings* settings = 0);

	int qComboBox(Rect r, int* index, MemberInfo* mInfo, TextBoxSettings* settings = 0);
	int qComboBox(Rect r, int* index, EnumInfo* eInfo, int count, TextBoxSettings* settings = 0);

	int qColorPicker(Rect r, Vec4* color, Vec2i align = vec2i(-1,0));

	//

	void popupSetup();
	void handleColorPickerPopup();
	void updateComboBoxPopups();
};

//

struct GuiWindowSettings {
	float borderSize;
	float cornerSize;
	Vec2 minDim;
	Vec2 maxDim;
	Rect insideRect;

	bool movable;
	bool resizableX;
	bool resizableY;

	bool mouseScreenCoordinates;
};