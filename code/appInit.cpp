
void initMemory(AppMemory* appMemory) {

	SYSTEM_INFO info;
	GetSystemInfo(&info);

	// char* baseAddress = (char*)gigaBytes(8);
	// VirtualAlloc(baseAddress, gigaBytes(40), MEM_RESERVE, PAGE_READWRITE);

	ExtendibleMemoryArray* pMemory = &appMemory->extendibleMemoryArrays[appMemory->extendibleMemoryArrayCount++];
	// initExtendibleMemoryArray(pMemory, megaBytes(512), info.dwAllocationGranularity, baseAddress);
	initExtendibleMemoryArray(pMemory, megaBytes(512), info.dwAllocationGranularity);

	ExtendibleBucketMemory* dMemory = &appMemory->extendibleBucketMemories[appMemory->extendibleBucketMemoryCount++];
	// initExtendibleBucketMemory(dMemory, megaBytes(1), megaBytes(512), info.dwAllocationGranularity, baseAddress + gigaBytes(16));
	initExtendibleBucketMemory(dMemory, megaBytes(1), megaBytes(512), info.dwAllocationGranularity);

	MemoryArray* tMemoryDebug = &appMemory->memoryArrays[appMemory->memoryArrayCount++];
	// initMemoryArray(tMemoryDebug, megaBytes(30), baseAddress + gigaBytes(33));
	initMemoryArray(tMemoryDebug, megaBytes(30));

	//

	ExtendibleMemoryArray* pDebugMemory = &appMemory->extendibleMemoryArrays[appMemory->extendibleMemoryArrayCount++];
	initExtendibleMemoryArray(pDebugMemory, megaBytes(50), info.dwAllocationGranularity);

	MemoryArray* tMemory = &appMemory->memoryArrays[appMemory->memoryArrayCount++];
	initMemoryArray(tMemory, megaBytes(30), 0);
}

void setupMemoryAndGlobals(AppMemory* appMemory, ThreadQueue* threadQueue, MemoryBlock* gMemory, AppData** appData, DebugState** debugState, bool init) {

	if(init) initMemory(appMemory);

	*gMemory = {};
	{
		gMemory->pMemory = &appMemory->extendibleMemoryArrays[0];
		gMemory->tMemory = &appMemory->memoryArrays[0];
		gMemory->dMemory = &appMemory->extendibleBucketMemories[0];

		gMemory->pMemoryDebug = &appMemory->extendibleMemoryArrays[1];
		gMemory->tMemoryDebug = &appMemory->memoryArrays[1];
	}

	DebugState* ds = (DebugState*)getBaseExtendibleMemoryArray(gMemory->pMemoryDebug);
	AppData* ad = (AppData*)getBaseExtendibleMemoryArray(gMemory->pMemory);

	{
		theMemory = gMemory;
		theThreadQueue = threadQueue;
		theGState = &ad->GraphicsState;
		theAudioState = &ad->audioState;
		theDebugState = ds;
		theTimer = ds->profiler.timer;
	}

	*appData = ad;
	*debugState = ds;

	if(init) {
		setMemory(true);
		getPMemory(sizeof(DebugState));

		setMemory();
		getPMemory(sizeof(AppData));
	}
}

void systemInit(AppData* ad, DebugState* ds, SystemData* sd, WindowSettings* ws, WindowsData windowsData) {
	
	*ad = {};

	int windowStyle = WS_OVERLAPPEDWINDOW;
	// int windowStyle = WS_OVERLAPPEDWINDOW & ~WS_SYSMENU;
	initSystem(sd, ws, windowsData, vec2i(1920*0.85f, 1080*0.85f), windowStyle, 1);

	sd->messageFiber = CreateFiber(0, (PFIBER_START_ROUTINE)updateInput, sd);

	HWND windowHandle = sd->windowHandle;
	SetWindowText(windowHandle, APP_NAME);

	ws->vsync = true;
	ws->frameRate = ws->refreshRate;
	ad->maxFrameRate = 200;

	sd->input = ds->input;

	#ifndef SHIPPING_MODE
	#if WINDOW_TOPMOST_DEBUG
	if(!IsDebuggerPresent()) {
		makeWindowTopmost(sd);
	}
	#endif
	#endif

	ws->lastMousePosition = {0,0};

	ad->gSettings.fieldOfView = 60;
	ad->gSettings.msaaSamples = 4;
	ad->gSettings.resolutionScale = 1;
	ad->gSettings.nearPlane = 0.2f;
	ad->gSettings.farPlane = 50;
	ad->dt = 1/(float)ws->frameRate;

	pcg32_srandom(0, __rdtsc());
	
}

void addFrameBuffers();
void graphicsInit(GraphicsState* gs, WindowSettings* ws, SystemData *sd, SectionTimer* st) {

	*gs = {};

	// @SwapChain.
	{
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		{
			DXGI_MODE_DESC bufferDesc = {};
			bufferDesc.Width = 0;
			bufferDesc.Height = 0;
			bufferDesc.RefreshRate = {ws->refreshRate, 1};
			bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			// bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

			DXGI_SAMPLE_DESC sampleDesc = {};
			sampleDesc.Count = 1;
			sampleDesc.Quality = 0;

			swapChainDesc.BufferDesc = bufferDesc;
			swapChainDesc.SampleDesc = sampleDesc;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.BufferCount = 1;
			swapChainDesc.OutputWindow = sd->windowHandle;
			swapChainDesc.Windowed = true;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
			swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		}

		D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};

		IDXGISwapChain* swapChain;
		ID3D11Device* d3dDevice;

		#if SHADER_DEBUG
		uint flags = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG;
		#else
		uint flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
		#endif

		if ( FAILED( D3D11CreateDeviceAndSwapChain( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, featureLevels, arrayCount(featureLevels), D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &d3dDevice, 0, 0) )) 
			printf("D3D device creation failed");

		gs->swapChain = swapChain;
		gs->d3dDevice = d3dDevice;
		gs->d3dDevice->GetImmediateContext(&gs->d3ddc);

		// Disable alt + enter.
		IDXGIDevice * pDXGIDevice;
		d3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice);
		IDXGIAdapter * pDXGIAdapter;
		pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pDXGIAdapter);
		IDXGIFactory * pIDXGIFactory;
		pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&pIDXGIFactory);

		pIDXGIFactory->MakeWindowAssociation(sd->windowHandle, DXGI_MWA_NO_ALT_ENTER);
	}

	st->add("d3d11 init");

	IDXGISwapChain* swapChain = gs->swapChain;
	ID3D11Device* d3dDevice = gs->d3dDevice;
	ID3D11DeviceContext* d3ddc = gs->d3ddc;

	// uint count = 0;
	// sd->d3dDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 8, &count);

	// @Shader.
	{
		gs->shaderCount = 0;
		gs->shaders = getPArray(Shader, 10);

		for(int i = 0; i < arrayCount(shaderInfos); i++) {
			ShaderInfo* info = shaderInfos + i;
			
			Shader shader = {};
			shader.id = i;
			shader.varsSize = info->varsSize;
			shader.varsData = (char*)getPMemory(shader.varsSize);

			shader.inputLayout = 0;

			{
				D3D11_BUFFER_DESC bufferDesc = {};
				bufferDesc.ByteWidth = info->varsSize;
				bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
				bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
				bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
				bufferDesc.MiscFlags = 0;
				bufferDesc.StructureByteStride = 0;

				d3dDevice->CreateBuffer(&bufferDesc, 0, &shader.constantBuffer);
			}

			gs->shaders[gs->shaderCount++] = shader;
		}

		dxLoadShaders();
		setupInputLayouts();
	}

	st->add("shader");

	// @Textures.
	{
		gs->textureCount = 0;
		gs->textureCountMax = 100;
		gs->textures = getPArray(Texture, gs->textureCountMax);

		{
			RecursiveFolderSearchData fd;
			recursiveFolderSearchStart(&fd, App_Texture_Folder);
			while(recursiveFolderSearchNext(&fd)) {

				if(strFind(fd.fileName, ".dds") == -1) continue;

				Texture tex = {};
				tex.name = getPString(fd.fileName);
				tex.file = getPString(fd.filePath);
				tex.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
				
				if(strFind(fd.fileName, "particles.dds") != -1) {
					tex.spriteSheet = true;
					tex.spriteCount = 12;
					tex.cellDim = vec2i(8,8);
				}

				dxLoadAndCreateTextureDDS(&tex, true);

				gs->textures[gs->textureCount++] = tex;
			}

			gs->textureWhite  = dxGetTexture("misc\\white.dds");
			gs->textureCircle = dxGetTexture("misc\\circle.dds");
		}
	}

	st->add("textures");

	// @Meshes.
	{
		char* meshesWithAlpha[] = {"entityUI_particle", "treeRealistic"};
		char* meshesDoubleSided[] = {"entityUI_particle", "treeRealistic"};

		gs->meshCount = 0;
		gs->meshes = getPArray(Mesh, 20);

		{
			ObjLoader parser;

			int counter = 0;
			Mesh m = {};

			RecursiveFolderSearchData fd;
			recursiveFolderSearchStart(&fd, App_Mesh_Folder);
			while(recursiveFolderSearchNext(&fd)) {

				// if(strFind(fd.fileName, ".obj") == -1) continue;

				if(strFind(fd.fileName, ".obj")  == -1 &&
				   strFind(fd.fileName, ".mesh") == -1) continue;

				m.name = getPString(fd.fileName);
				m.file = getPString(fd.filePath);

				{
					m.swapWinding = true;

					dxLoadMesh(&m, &parser);

					m.hasAlpha = false;
					for(int i = 0; i < arrayCount(meshesWithAlpha); i++) {
						if(strFind(m.name, meshesWithAlpha[i]) != -1) {
							m.hasAlpha = true;
							break;
						}
					}

					m.doubleSided = false;
					for(int i = 0; i < arrayCount(meshesDoubleSided); i++) {
						if(strFind(m.name, meshesDoubleSided[i]) != -1) {
							m.doubleSided = true;
							break;
						}
					}

					gs->meshes[gs->meshCount++] = m;
				}
			}

			parser.free();
		}

		{
			gs->primitiveVertexBufferMaxCount = 1000;

			D3D11_BUFFER_DESC bd;
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.ByteWidth = sizeof(PrimitiveVertex) * gs->primitiveVertexBufferMaxCount;
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bd.MiscFlags = 0;

			d3dDevice->CreateBuffer(&bd, NULL, &gs->primitiveVertexBuffer);
		}
	}

	st->add("meshes");

	// @Material.
	{
		char* materialsWithAlpha[] = {"Metal Grill"};

		gs->materialCount = 0;
		gs->materials = getPArray(Material, 20);

		{
			ObjLoader parser = {};
			parser.materialArray.reserve(1);

			RecursiveFolderSearchData fd;
			recursiveFolderSearchStart(&fd, App_Material_Folder);
			while(recursiveFolderSearchNext(&fd)) {

				if(strFind(fd.fileName, ".mtl") == -1) continue;

				Material m = {};
				m.name = getPString(fd.fileName);
				m.file = getPString(fd.filePath);

				parser.parseMtl(App_Material_Folder, fd.fileName);
				defer{parser.clear();};

				dxLoadMaterial(&m, parser.materialArray.atr(0), App_Material_Folder);

				for(int i = 0; i < arrayCount(materialsWithAlpha); i++) {
					if(strFind(fd.folder, materialsWithAlpha[i]) != -1) {
						m.hasAlpha = true;
						break;
					}
				}

				gs->materials[gs->materialCount++] = m;
			}

			parser.free();
		}

		char* materials[] = {"Rock", "Canyon Rock", "Stone Wall", "Crystals", "Snow"};
		float heightScales[] = {0.05f, 0.2f, 0.05f, 0.2f, 0.05f};
		int matCount = arrayCount(materials);

		for(int i = 0; i < matCount; i++) {
			Material* m = dxGetMaterial(fString("%s%s", materials[i], "\\material.mtl"));
			m->heightScale = heightScales[i];
		}
	}

	st->add("materials");

	// @Samplers.
	{
		gs->sampler      = createSampler(D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_WRAP, 16, D3D11_COMPARISON_LESS);
		gs->samplerClamp = createSampler(D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_CLAMP, 16, D3D11_COMPARISON_LESS);
		// gs->samplerClamp = createSampler(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, 16, D3D11_COMPARISON_LESS);
		gs->samplerCmp   = createSampler(D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_MIRROR, 0, D3D11_COMPARISON_LESS_EQUAL);
	}

	// @BlendStates.
	{
		gs->blendStateCount = 0;

		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.RenderTarget[0] = {false, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, 
			                                 D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, 
			                                 D3D11_COLOR_WRITE_ENABLE_ALL};
		d3dDevice->CreateBlendState(&blendDesc, &gs->blendStates[Blend_State_NoBlend]);

		gs->blendStates[Blend_State_Blend] = 
			dxCreateBlendState(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD);

		gs->blendStates[Blend_State_DrawOverlay] = 
			dxCreateBlendState(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, 
		                      D3D11_BLEND_ONE,       D3D11_BLEND_ONE,           D3D11_BLEND_OP_ADD);

		gs->blendStates[Blend_State_BlitOverlay] = 
			dxCreateBlendState(D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD); 

		// gs->blendStates[Blend_State_BlendAlphaCoverage] = dxCreateBlendState(
		// 	D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, true);

		gs->blendStates[Blend_State_BlendAlphaCoverage] = 
			dxCreateBlendState(D3D11_BLEND_ZERO, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, true, false);

		gs->blendStates[Blend_State_PreMultipliedAlpha] = 
			dxCreateBlendState(D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD);

		gs->blendStates[Blend_State_Add] = 
			dxCreateBlendState(D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD);
	}

	st->add("sampler, blending");

	// @FrameBuffers.
	{
		gs->frameBufferCountMax = 30;
		gs->frameBufferCount = 0;
		gs->frameBuffers = getPArray(FrameBuffer, gs->frameBufferCountMax);

		addFrameBuffers();
	}

	st->add("framebuffers");

	// @Queries.
	{
		gs->timer.init(gs->d3dDevice, gs->d3ddc);
	}

	// @InitCubeMap.
	{
		gs->cubeMapSize = 1024;
		Vec2i dim = vec2i(gs->cubeMapSize);

		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = dim.w;
		texDesc.Height = dim.h;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 6;
		texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		texDesc.CPUAccessFlags = 0;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		D3D11_SHADER_RESOURCE_VIEW_DESC SMViewDesc;
		SMViewDesc.Format = texDesc.Format;
		SMViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		SMViewDesc.TextureCube.MipLevels =  texDesc.MipLevels;
		SMViewDesc.TextureCube.MostDetailedMip = 0;

		ID3D11Texture2D* texture;
		gs->d3dDevice->CreateTexture2D(&texDesc, 0, &texture);
		ID3D11ShaderResourceView* view;
		gs->d3dDevice->CreateShaderResourceView(texture, &SMViewDesc, &view);

		gs->cubeMapTexture = texture;
		gs->cubeMapView = view;
	}

	{
		// gs->shadowMapSize = 1024;
		// gs->shadowMapSize = 2048;
		gs->shadowMapSize = 4096;
	}

	// Fonts
	{
		gs->fontFolders[gs->fontFolderCount++] = getPString(App_Font_Folder);
		char* windowsFontFolder = fString("%s%s", getenv(Windows_Font_Path_Variable), Windows_Font_Folder);
		gs->fontFolders[gs->fontFolderCount++] = getPString(windowsFontFolder);
	}

	st->add("fonts");
}

void appSessionInit(AppData* ad, SystemData* sd, WindowSettings* ws) {

	// Setup app temp settings.
	AppSessionSettings appSessionSettings = {};
	{
		// @AppSessionDefaults
		if(!fileExists(App_Session_File)) {
			AppSessionSettings at = {};

			Rect r = ws->monitors[0].workRect;
			Vec2 center = vec2(r.cx(), (r.top - r.bottom)/2);
			Vec2 dim = vec2(r.w(), -r.h());
			at.windowRect = rectCenDim(center, dim*0.85f);

			at.camPos = vec3(-4,5,3);
			at.camRot = vec2(0.8f,-0.2f);

			appWriteSessionSettings(App_Session_File, &at);
		}

		// @AppSessionLoad
		{
			AppSessionSettings at = {};
			appReadSessionSettings(App_Session_File, &at);

			Recti r = rectiRound(at.windowRect);
			MoveWindow(sd->windowHandle, r.left, r.top, r.right-r.left, r.bottom-r.top, true);

			updateResolution(sd->windowHandle, ws);

			appSessionSettings = at;
		}
	}
}

void appInit(AppData* ad, SystemData* sd, WindowSettings* ws) {

	ad->audioState.masterVolume = 0.5f;

	ad->gameMode = GAME_MODE_LOAD;
	ad->newGameMode = -1;
	ad->menu.activeId = 0;

	// #if SHIPPING_MODE
	// ad->captureMouse = true;
	// #else 
	ad->mouseEvents.captureMouse = false;
	ad->mouseEvents.debugMouse = true;
	// #endif

	ad->volumeFootsteps = 0.2f;
	ad->volumeGeneral = 0.5f;
	ad->volumeMenu = 0.7f;

	ad->mouseSensitivity = 0.2f;

	ad->redrawSkyBox = true;

	// Entity.

	ad->playerMode = false;

	// Load game settings.
	{
		// Init default.
		if(!fileExists(Game_Settings_File)) {
			GameSettings settings = {};

			settings.fullscreen = true;
			settings.vsync = true;
			settings.frameRateCap = ad->maxFrameRate;
			settings.resolutionScale = 1.0f;
			settings.volume = 0.5f;
			settings.mouseSensitivity = 0.2f;
			settings.fieldOfView = 60;

			writeDataToFile((char*)&settings, sizeof(GameSettings), Game_Settings_File);
		}

		// Load Gamesettings.
		{
			GameSettings settings = {};

			readDataFromFile((char*)&settings, Game_Settings_File);

			if(settings.fullscreen) setWindowMode(sd->windowHandle, ws, WINDOW_MODE_FULLBORDERLESS);
			else setWindowMode(sd->windowHandle, ws, WINDOW_MODE_WINDOWED);

			ws->vsync = settings.vsync;
			ad->frameRateCap = settings.frameRateCap;
			ad->gSettings.resolutionScale = settings.resolutionScale;
			ad->audioState.masterVolume = settings.volume;
			ad->mouseSensitivity = settings.mouseSensitivity;
			ad->gSettings.fieldOfView = settings.fieldOfView;
		}
	}

	theGState->gSettings = &ad->gSettings;
}