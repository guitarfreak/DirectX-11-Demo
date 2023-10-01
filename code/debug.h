
#include "debugUI.h"

struct DebugSessionSettings {
	int panelHierarchy[10];
	PanelData panels[10];
	int profilerPanelActiveSection;

	bool drawGrid;
	bool drawGroupHandles;
	bool drawParticleHandles;
	bool drawSelection;
	bool drawManifold;
	bool drawBlockers;
};

void debugWriteSessionSettings(DebugSessionSettings* at) {
	writeDataToFile((char*)at, sizeof(DebugSessionSettings), Gui_Session_File);
}

void debugReadSessionSettings(DebugSessionSettings* at) {
	readDataFromFile((char*)at, Gui_Session_File);
}

struct NotificationData {
	char* stack[10];
	float times[10];
	int count;
};

struct InfoData {
	char* stack[40];
	int count;
};

struct DebugState {
	Timer swapTimer;
	Timer frameTimer;
	Timer debugTimer;
	u64 clockStamp;
	Statistic clockConvertStat;
	double clockStampToTime;

	double dt;
	double time;

	int timeMode;
	bool timeStop;

	// Stats.

	TimerInterval cpuInterval, gpuInterval, debugInterval;
	double cpuTime, gpuTime, debugTime;

	double fpsTime;
	int fpsCounter;
	float avgFps;

	//

	Input* input;
	StateRecording recState;
	Profiler profiler;

	//

	bool showUI;
	bool showConsole;
	bool showHud;

	//

	int fontHeight;
	int fontHeightScaled;
	float fontScale;

	Gui gui;
	float guiAlpha;
	float guiAlphaMax;

	int panelInit;
	int panelActivityIndex;
	int panelHierarchy[10];
	PanelData panels[10];
	bool mouseOverPanel;
	float panelAlphaFadeState;

	IntrospectionPanelInfo introspectionPanelInfo;
	ProfilerPanelInfo profilerPanelInfo;
	MapPanelInfo mapPanelInfo;
	EntityPanelInfo entityPanelInfo;
	ManifoldPanelInfo manifoldPanelInfo;

	EntityUI entityUI;

	bool drawGrid;
	bool drawGroupHandles;
	bool drawParticleHandles;
	bool drawSelection;
	bool drawManifold;
	bool drawBlockers;

	//

	Console console;
	NotificationData noteData;
	InfoData infoData;

	Logger logger;
};
