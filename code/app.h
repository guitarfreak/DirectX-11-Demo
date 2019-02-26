
Meta_Parse_Struct(0);
struct MouseEvents {
	bool debugMouse;
	bool captureMouseKeepCenter;
	bool captureMouse;
	bool fpsMode;
	bool fpsModeFixed;
	bool lostFocusWhileCaptured;
};

Meta_Parse_Struct(0);
struct AppData {
	
	// General.

	SystemData systemData;
	Input input;
	WindowSettings wSettings;
	GraphicsState GraphicsState;
	AudioState audioState;

	double dt;
	double time;
	int frameCount;

	int maxFrameRate;
	int frameRateCap;

	bool updateFrameBuffers;
	bool updateDebugFrameBuffer;
	float mouseSensitivity;

	// 

	MouseEvents mouseEvents;
	bool toggleFullscreen;
	
	GraphicsSettings gSettings;

	Rect3 sceneBoundingBox;
	bool redrawSkyBox;

	// Game.

	int newGameMode;
	int gameMode;
	MainMenu menu;

	bool levelEdit;
	// bool freeCam, pause, leveledit

	bool playerMode;

	bool loading;
	bool newGame;
	bool saveGame;

	GameSettings settings;
	float volumeFootsteps;
	float volumeGeneral;
	float volumeMenu;

	//

	EntityManager entityManager;
	Entity* player;
	Entity* camera;
	Entity* sky;

	//

	Entity* figure;

	bool showSkeleton;
	Mesh* figureMesh; // @Ignore
	int figureAnimation;
	float figureSpeed;

	//

	bool firstWalk;
	float footstepSoundValue;
	int lastFootstepSoundId;

	//

	Mat4 viewProjLight;
};
