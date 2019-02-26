
enum EntitySelectionMode {
	ENTITYUI_MODE_TRANSLATION = 0,
	ENTITYUI_MODE_ROTATION,
	ENTITYUI_MODE_SCALE,
	ENTITYUI_MODE_DRAG,

	ENTITYUI_MODE_SIZE,
};

enum EntityUIState {
	ENTITYUI_INACTIVE = 0,
	ENTITYUI_HOT,
	ENTITYUI_ACTIVE,
};

enum EntityTransformMode {
	TRANSFORM_MODE_AXIS = 0,
	TRANSFORM_MODE_PLANE,
	TRANSFORM_MODE_CENTER,
};

enum CommandType {
	COMMAND_TYPE_EDIT = 0,
	COMMAND_TYPE_INSERT,
	COMMAND_TYPE_REMOVE,
	COMMAND_TYPE_SELECTION,

	COMMAND_TYPE_SIZE,
};

struct HistoryCommand {
	int type;
	int count;
};

struct HistoryData {
	int index;
	DArray<char> buffer;
	DArray<int> offsets;

	// Temp.
	DArray<Entity> objectsPreMod;
	DArray<Entity> objectsPostMod;
	DArray<Entity> tempObjects;
	DArray<int> tempSelected;

	DArray<int> previousSelection;
};

void historyReset(HistoryData* hd) {
	hd->index = 0;
	hd->buffer.clear();
	hd->offsets.clear();
	hd->previousSelection.clear();
}

struct EntityUI {
	HistoryData history;
	bool objectNoticeableChange;

	DArray<int> selectedObjects;
	DArray<Entity> objectCopies;
	DArray<Entity> objectTempArray;
	bool objectsEdited;

	// For multiple selection.
	DArray<Vec3> objectCenterOffsets;
	Entity multiChangeObject;

	Vec2 dragSelectionStart;
	bool dragSelectionActive;
	bool multipleSelectionMode;

	int selectionMode;
	int selectionState;
	bool gotActive;
	int hotId;

	bool selectionChanged;

	bool guiHasFocus;

	float selectionAnimState;
	bool localMode;
	bool snappingEnabled;
	int snapGridSize;
	float snapGridDim;

	int axisIndex;
	Vec3 axis;
	Vec3 objectDistanceVector;

	// Translation mode.

	Vec3 startPos;
	Vec3 centerOffset;
	float centerDistanceToCam;
	int transformMode;
	Vec3 axes[3];

	// Rotation mode.

	Quat startRot;
	float currentRotationAngle;

	// Scale mode.

	Vec3 startOffset;
	Vec3 startDim;
	bool enableScaleEqually;
	float currentScalePercent;

	// For resetting state.
	
	Vec3 currentObjectDistanceVector;
	Vec3 currentPos;

	//

	Entity temp;
	bool changedGeomType;
	bool mouseOverScene;
};

bool keyPressed(Gui* gui, Input* input, int keycode) {
	if(gui->activeId != 0) return false;
	else return input->keysPressed[keycode];
}

bool keyDown(Gui* gui, Input* input, int keycode) {
	if(gui->activeId != 0) return false;
	else return input->keysDown[keycode];
}

int mouseWheel(Gui* gui, Input* input) {
	if(gui->hotId[Gui_Focus_MWheel] != 0 || gui->activeId != 0) return false;
	else return input->mouseWheel; 
}

int mouseButtonPressedLeft(Gui* gui, Input* input) {
	if(gui->hotId[Gui_Focus_MLeft] != 0 || gui->activeId != 0 ||
	   (gui->popupStackCount > 0 && (gui->popupStack[0].type == POPUP_TYPE_COLOR_PICKER ||
	                                 gui->popupStack[0].type == POPUP_TYPE_ALPHA_PICKER))) return false;
	else return input->mouseButtonPressed[0]; 
}

int mouseButtonPressedRight(Gui* gui, Input* input) {
	if(gui->hotId[Gui_Focus_MRight] != 0 || gui->activeId != 0) return false;
	else return input->mouseButtonPressed[1]; 
}

int mouseButtonReleasedRight(Gui* gui, Input* input) {
	if(gui->hotId[Gui_Focus_MRight] != 0 || gui->activeId != 0) return false;
	else return input->mouseButtonReleased[1]; 
}

int mouseButtonPressedMiddle(Gui* gui, Input* input) {
	if(gui->hotId[Gui_Focus_MMiddle] != 0 || gui->activeId != 0) return false;
	else return input->mouseButtonPressed[2]; 
}

int mouseButtonReleasedMiddle(Gui* gui, Input* input) {
	if(gui->activeId != 0) return false;
	else return input->mouseButtonReleased[2]; 
}

bool guiHotMouseClick(Gui* gui) {
	return gui->hotId[Gui_Focus_MLeft] != 0;
}

bool guiPopupOpen(Gui* gui) {
	return gui->popupStackCount > 0;
}