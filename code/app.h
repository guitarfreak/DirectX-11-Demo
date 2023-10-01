
Meta_Parse_Struct(0);
struct MouseEvents {
	bool debugMouse;
	bool debugMouseFixed;
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
	bool freeCam;

	float playerHeight;

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

	WalkManifold manifold; // @Ignore
	int manifoldGridRadius;
	Vec2 playerMoveDir;
	WalkLayer* currentWalkLayer; // @Ignore

	//

	Mat4 viewProjLight;
};

struct AppSessionSettings {
	Rect windowRect;
	Vec3 camPos;
	Vec2 camRot;

	void load(char* filePath) {
		readDataFromFile((char*)this, filePath);
	}

	void save(char* filePath) {
		writeDataToFile((char*)this, sizeof(AppSessionSettings), filePath);
	}
};
