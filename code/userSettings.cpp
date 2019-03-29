
#define STRINGIFY(n) #n
#define STRINGIFY_MACRO(n) STRINGIFY(n)
#define APP_NAME STRINGIFY_MACRO(A_NAME)

#define SHADER_DEBUG 0
#define HOTRELOAD_SHADERS 0
#define WINDOW_TOPMOST_DEBUG 1

#define USE_FIBERS 1

#define Editor_Executable_Path "C:\\Program Files\\Sublime Text 3\\sublime_text.exe"
#define Windows_Font_Folder "\\Fonts\\"
#define Windows_Font_Path_Variable "windir"
	
#define App_Session_File   ".\\session.tmp"
#define Game_Settings_File ".\\settings.tmp"
#define Gui_Session_File   ".\\guiSettings.tmp"
#define Saves_Folder       ".\\saves\\"
#define Save_State1        "saveState1.sav"

#ifdef SHIPPING_MODE
#define DATA_FOLDER(str)  ".\\data\\" str
#else 
#define DATA_FOLDER(str) "..\\data\\" str
#endif

#define App_Font_Folder     DATA_FOLDER("Fonts\\")
#define App_Audio_Folder    DATA_FOLDER("Audio\\")
#define App_Texture_Folder  DATA_FOLDER("Textures\\")
#define App_Material_Folder DATA_FOLDER("Materials\\")
#define App_Mesh_Folder     DATA_FOLDER("Meshes\\")
#define App_Map_Folder      DATA_FOLDER("Maps\\")

#define Map_File_Extension  ".map"
