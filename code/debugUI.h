
enum PanelTypes {
	Panel_Type_Profiler = 0,
	Panel_Type_InputRecording,
	Panel_Type_Introspection,
	Panel_Type_Animation,
	Panel_Type_Map,
	Panel_Type_Entity,

	Panel_Type_Size,
};

char* panelStrings[] = {
	"Profiler", 
	"InputRecording",
	"Introspection", 
	"Animation", 
	"Map", 
	"Entity"
};

struct PanelSettings {
	Rect pri;
	Vec2 p;
	Vec2 dim;
	Vec2 pad;
	float textPad;

	float headerHeight;
	float separatorHeight;
	Vec4 cSeparatorDark;
	Vec4 cSeparatorBright;
};

struct PanelData {
	Rect r;
	Vec2i align;
	bool isActive;
};

void profilerPanel(DebugState* ds, Gui* gui, PanelSettings pd);
void inputRecordingPanel(DebugState* ds, Gui* gui, PanelSettings pd);
void introspectionPanel(AppData* ad, DebugState* ds, Gui* gui, PanelSettings pd);
void animationPanel(AppData* ad, DebugState* ds, Gui* gui, PanelSettings pd);
void mapPanel(AppData* ad, DebugState* ds, Gui* gui, PanelSettings pd);
void entityPanel(AppData* ad, DebugState* ds, Gui* gui, PanelSettings pd);

#define Select_Panel_Function() \
	case Panel_Type_Profiler:       profilerPanel(ds, gui, pd); break; \
	case Panel_Type_InputRecording: inputRecordingPanel(ds, gui, pd); break; \
	case Panel_Type_Introspection:  introspectionPanel(ad, ds, gui, pd); break; \
	case Panel_Type_Animation:      animationPanel(ad, ds, gui, pd); break; \
	case Panel_Type_Map:            mapPanel(ad, ds, gui, pd); break; \
	case Panel_Type_Entity:         entityPanel(ad, ds, gui, pd); break; \

#define Set_Init_Panel_Properties() \
	settings.panels[Panel_Type_Profiler].r       = { rectCenDim(sr.c(), vec2(fh*40, fh*29)) }; \
	settings.panels[Panel_Type_InputRecording].r = { rectBDim(sr.b(), vec2(fh*17, fh*12)) }; \
	settings.panels[Panel_Type_Introspection].r  = { rectLDim(sr.l(), vec2(fh*25, fh*30)) }; \
	settings.panels[Panel_Type_Animation].r      = { rectCenDim(sr.c(), vec2(fh*15, fh*12)) }; \
	settings.panels[Panel_Type_Map]              = { rectBLDim(sr.bl(), vec2(fh*15, min(fh*13, sr.h()))), vec2i(-1,-1) }; \
	settings.panels[Panel_Type_Entity]           = { rectBLDim(sr.tr(), vec2(fh*20, min(fh*35, sr.h()))), vec2i(1,1) }; \

struct IntrospectionPanelInfo {
	float scrollHeight;
	float scrollValue;

	ExpansionIndex* expansionArray;
	int expansionCount;
	float widths[4];
};

struct ProfilerPanelInfo {
	int activeSection;
	GraphCam graphCam;
	int statsSortingIndex;
};

struct MapPanelInfo {
	DArray<char*> maps;
	int spawnEntityType;
};

struct EntityPanelInfo {
	int activeSection;

	bool filterEnabled;
	int typeFilterIndex;
	char nameFilter[50];
	float camDistFilter;
};