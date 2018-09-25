/*

ToDo:
- For shipping mode, pack all data into one file
- Fix linewidth everywhere.
- Make true fullscreen work.
- Audio debug screen.
- Clean up obj loader.
- Input layout change component sizes to be more reasonable.
- Mark objects with alpha in them.

- Draw little island with ground/heightmap, shore, ocean, horizon.

- Movement a la CS.
- Collision GJK.

- Interaction.

- Load assets better.
- Hot reloading for all assets.

- DDS file generation automation.

- Animation doesn't use scaling yet.

Bugs:
- Black shadow spot where sun hits material with displacement map.

*/

// External.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <D3D11.h>
#include <D3Dcompiler.h>

#define _WIN32_WINNT 0x0600
#include "external\textureLoader\DDSTextureLoader.cpp"
#include "external\textureLoader\DDSTextureLoader.h"

// Stb.

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "external\stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "external\stb_image_write.h"

#define STB_VORBIS_NO_PUSHDATA_API
#include "external\stb_vorbis.c"

//

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_PARAMETER_TAGS_H
#include FT_MODULE_H

//

struct ThreadQueue;
struct GraphicsState;
struct AudioState;
struct MemoryBlock;
struct DebugState;
struct Timer;
ThreadQueue*   theThreadQueue;
GraphicsState* theGState;
AudioState*    theAudioState;
MemoryBlock*   theMemory;
DebugState*    theDebugState;
Timer*         theTimer;

// Internal.

#include "types.cpp"
#include "misc.cpp"
#include "string.cpp"
#include "memory.cpp"
#include "appMemory.cpp"
#include "fileIO.cpp"
#include "random.cpp"
#include "mathBasic.cpp"
#include "math.cpp"
#include "color.cpp"
#include "timer.cpp"
#include "interpolation.cpp"
#include "sort.cpp"
#include "container.cpp"
#include "misc2.cpp"
#include "hotload.cpp"
#include "threadQueue.cpp"
#include "platformWin32.cpp"
#include "input.cpp"

#include "animation.h"
#include "rendering.h"
#include "font.h"

#include "userSettings.cpp"

#include "shader.cpp"
#include "objLoader.cpp"
#include "animation.cpp"
#include "rendering.cpp"
#include "font.cpp"
#include "newGui.cpp"
#include "console.cpp"

#include "entity.cpp"
#include "audio.cpp"

#include "menu.cpp"

#include "introspection.cpp"

//

struct AppData {
	// General.

	SystemData systemData;
	Input input;
	WindowSettings wSettings;
	GraphicsState GraphicsState;
	AudioState audioState;

	f64 dt;
	f64 time;
	int frameCount;

	int maxFrameRate;
	int frameRateCap;

	bool updateFrameBuffers;
	bool updateDebugFrameBuffer;
	float mouseSensitivity;

	// 

	bool debugMouse;
	bool captureMouseKeepCenter;
	bool captureMouse;
	bool fpsMode;
	bool fpsModeFixed;
	bool lostFocusWhileCaptured;

	Camera activeCam;

	Vec2i cur3dBufferRes;
	int msaaSamples;
	float resolutionScale;

	Mat4 view2d;
	Mat4 ortho;
	Mat4 view;
	Mat4 proj;
	Mat4 viewInv;
	Mat4 projInv;

	float aspectRatio;
	int fieldOfView;
	float nearPlane;
	float farPlane;

	bool redrawSkyBox;
	Vec2 sunAngles;

	// Game.

	int gameMode;
	MainMenu menu;

	bool loading;
	bool newGame;
	bool saveGame;

	GameSettings settings;

	float volumeFootsteps;
	float volumeGeneral;
	float volumeMenu;

	//

	EntityList entityList;
	Entity* player;
	Entity* cameraEntity;

	//

	Particle* particleLists[50];
	bool particleListUsage[50];
	int particleListsSize;

	//

	bool playerMode;

	//

	bool showSkeleton;
	Mat4 figureModel;
	Mesh* figureMesh;
	int figureAnimation;
	float figureSpeed;

	//

	bool firstWalk;
	float footstepSoundValue;
	int lastFootstepSoundId;
};

#include "debug.cpp"



#ifdef FULL_OPTIMIZE
#pragma optimize( "", on )
#else 
#pragma optimize( "", off )
#endif

extern "C" APPMAINFUNCTION(appMain) {

	if(init) {

		// Init memory.

		SYSTEM_INFO info;
		GetSystemInfo(&info);

		char* baseAddress = (char*)gigaBytes(8);
		VirtualAlloc(baseAddress, gigaBytes(40), MEM_RESERVE, PAGE_READWRITE);

		ExtendibleMemoryArray* pMemory = &appMemory->extendibleMemoryArrays[appMemory->extendibleMemoryArrayCount++];
		initExtendibleMemoryArray(pMemory, megaBytes(512), info.dwAllocationGranularity, baseAddress);

		ExtendibleBucketMemory* dMemory = &appMemory->extendibleBucketMemories[appMemory->extendibleBucketMemoryCount++];
		initExtendibleBucketMemory(dMemory, megaBytes(1), megaBytes(512), info.dwAllocationGranularity, baseAddress + gigaBytes(16));

		MemoryArray* tMemoryDebug = &appMemory->memoryArrays[appMemory->memoryArrayCount++];
		initMemoryArray(tMemoryDebug, megaBytes(30), baseAddress + gigaBytes(33));

		//

		MemoryArray* pDebugMemory = &appMemory->memoryArrays[appMemory->memoryArrayCount++];
		initMemoryArray(pDebugMemory, megaBytes(50), 0);

		MemoryArray* tMemory = &appMemory->memoryArrays[appMemory->memoryArrayCount++];
		initMemoryArray(tMemory, megaBytes(30), 0);
	}

	// Setup memory and globals.

	MemoryBlock gMemory = {};
	{
		gMemory.pMemory = &appMemory->extendibleMemoryArrays[0];
		gMemory.tMemory = &appMemory->memoryArrays[0];
		gMemory.dMemory = &appMemory->extendibleBucketMemories[0];
		gMemory.pMemoryDebug = &appMemory->memoryArrays[1];
		gMemory.tMemoryDebug = &appMemory->memoryArrays[2];
	}

	DebugState* ds = (DebugState*)getBaseMemoryArray(gMemory.pMemoryDebug);
	AppData* ad = (AppData*)getBaseExtendibleMemoryArray(gMemory.pMemory);

	{
		theMemory = &gMemory;
		theThreadQueue = threadQueue;
		theGState = &ad->GraphicsState;
		theAudioState = &ad->audioState;
		theDebugState = ds;
		theTimer = ds->profiler.timer;
	}

	Input* input = &ad->input;
	SystemData* sd = &ad->systemData;
	WindowSettings* ws = &ad->wSettings;
	GraphicsState* gs = &ad->GraphicsState;
	HWND windowHandle = sd->windowHandle;

	// @Init.

	if(init) {

		// @DebugInit.
		{
			getPMemoryDebug(sizeof(DebugState));
			*ds = {};
			clearTMemoryDebug();

			ds->recState.init(600, theMemory->pMemory);
			
			ds->profiler.init(10000, 200);
			theTimer = ds->profiler.timer;

			ds->input = getPStructDebug(Input);
			initInput(ds->input);

			ds->showMenu = false;
			ds->showProfiler = false;
			ds->showConsole = false;
			ds->showHud = false;
			ds->guiAlpha = 0.99f;

			for(int i = 0; i < arrayCount(ds->notificationStack); i++) {
				ds->notificationStack[i] = getPStringDebug(DEBUG_NOTE_LENGTH+1);
			}

			ds->fontScale = 1.0f;

			ds->console.init();

			ds->swapTimer.init();
			ds->frameTimer.init();
			ds->debugTimer.init();

			ds->expansionArray = getPArrayDebug(ExpansionIndex, 1000);

			ds->panelGotActiveIndex = -1;
		}

		TIMER_BLOCK_NAMED("Init");

		// @AppInit.
		{
			getPMemory(sizeof(AppData));
			*ad = {};
			clearTMemory();
			
			int windowStyle = WS_OVERLAPPEDWINDOW;
			// int windowStyle = WS_OVERLAPPEDWINDOW & ~WS_SYSMENU;
			initSystem(sd, ws, windowsData, vec2i(1920*0.85f, 1080*0.85f), windowStyle, 1);

			sd->messageFiber = CreateFiber(0, (PFIBER_START_ROUTINE)updateInput, sd);

			windowHandle = sd->windowHandle;
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

			ad->fieldOfView = 60;
			ad->msaaSamples = 4;
			ad->resolutionScale = 1;
			ad->nearPlane = 0.1f;
			ad->farPlane = 100;
			ad->dt = 1/(float)ws->frameRate;

			pcg32_srandom(0, __rdtsc());
		}

		// @AudioInit.
		{
			AudioState* as = &ad->audioState;
			(*as) = {};
			as->masterVolume = 0.5f;
			as->effectVolume = 0.8f;
			as->musicVolume = 1.0f;

			audioDeviceInit(as, ws->frameRate);

			as->fileCountMax = 100;
			as->files = getPArray(Audio, as->fileCountMax);

			{
				RecursiveFolderSearchData fd;
				recursiveFolderSearchStart(&fd, App_Audio_Folder);
				while(recursiveFolderSearchNext(&fd)) {
					addAudio(as, fd.filePath, fd.fileName);
				}
			}

			// Setup sounds.
			{
				initSoundTable(as);
			}
		}

		// @Direct3d
		{
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
					swapChainDesc.OutputWindow = windowHandle;
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

				pIDXGIFactory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER);
			}

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

				// @InputLayouts.
				{
					{
						Shader* shader = dxGetShader(Shader_Primitive);
						if ( FAILED( d3dDevice->CreateInputLayout(primitiveInputLayout, arrayCount(primitiveInputLayout), shader->vertexBlob->GetBufferPointer(), shader->vertexBlob->GetBufferSize(), &gs->primitiveInputLayout) ) ) 
							printf("Could not create Input Layout!");
					}

					{
						Shader* shader = dxGetShader(Shader_Main);
						if ( FAILED( d3dDevice->CreateInputLayout(mainShaderInputLayout, arrayCount(mainShaderInputLayout), shader->vertexBlob->GetBufferPointer(), shader->vertexBlob->GetBufferSize(), &gs->mainInputLayout) ) ) 
							printf("Could not create Input Layout!");
					}

					dxGetShader(Shader_Primitive)->inputLayout = gs->primitiveInputLayout;
					dxGetShader(Shader_Main)->inputLayout = gs->mainInputLayout;
					dxGetShader(Shader_Sky)->inputLayout = 0;
					dxGetShader(Shader_Gradient)->inputLayout = gs->primitiveInputLayout;
					dxGetShader(Shader_Cube)->inputLayout = gs->mainInputLayout;
					dxGetShader(Shader_Shadow)->inputLayout = gs->mainInputLayout;
				}
			}

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
						tex.name = getPStringCpy(fd.fileName);
						tex.file = getPStringCpy(fd.filePath);
						tex.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

						dxLoadAndCreateTextureDDS(&tex, true);

						gs->textures[gs->textureCount++] = tex;
					}

					gs->textureWhite  = dxGetTexture("misc\\white.dds");
					gs->textureCircle = dxGetTexture("misc\\circle.dds");
				}
			}

			// @Meshes.
			{
				gs->meshCount = 0;
				gs->meshes = getPArray(Mesh, 20);

				{
					ObjLoader parser;

					int counter = 0;
					Mesh m;

					RecursiveFolderSearchData fd;
					recursiveFolderSearchStart(&fd, App_Mesh_Folder);
					while(recursiveFolderSearchNext(&fd)) {

						// if(strFind(fd.fileName, ".obj") == -1) continue;

						if(strFind(fd.fileName, ".obj")  == -1 &&
						   strFind(fd.fileName, ".mesh") == -1) continue;

						m.name = getPStringCpy(fd.fileName);
						m.file = getPStringCpy(fd.filePath);

						{
							m.swapWinding = true;

							dxLoadMesh(&m, &parser);

							// m.vertices.copy(parser.vertexBuffer);

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

			// @Material.
			{
				gs->materialCount = 0;
				gs->materials = getPArray(Material, 20);

				{
					ObjLoader parser;
					parser.materialArray.reserve(1);

					int counter = 0;
					Mesh m;

					RecursiveFolderSearchData fd;
					recursiveFolderSearchStart(&fd, App_Material_Folder);
					while(recursiveFolderSearchNext(&fd)) {

						if(strFind(fd.fileName, ".mtl") == -1) continue;

						Material m = {};
						m.name = getPStringCpy(fd.fileName);
						m.file = getPStringCpy(fd.filePath);

						parser.parseMtl(App_Material_Folder, fd.fileName);
						defer{parser.clear();};

						dxLoadMaterial(&m, parser.materialArray.atr(0), App_Material_Folder);

						gs->materials[gs->materialCount++] = m;
					}

					parser.free();
				}
			}

			// @Samplers.
			{
				{
					D3D11_SAMPLER_DESC samplerDesc;
					// samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
					samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
					samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
					samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
					samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
					samplerDesc.MipLODBias = 0;
					samplerDesc.MaxAnisotropy = 16;
					samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
					samplerDesc.BorderColor[0] = 0;
					samplerDesc.BorderColor[1] = 0;
					samplerDesc.BorderColor[2] = 0;
					samplerDesc.BorderColor[3] = 0;
					samplerDesc.MinLOD = 0;
					samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

					ID3D11SamplerState* sampler;
					d3dDevice->CreateSamplerState(&samplerDesc, &sampler);

					gs->sampler = sampler;
				}

				{
					D3D11_SAMPLER_DESC samplerDesc;
					samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
					samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
					samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
					samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
					samplerDesc.MipLODBias = 0;
					samplerDesc.MaxAnisotropy = 0;
					samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
					samplerDesc.BorderColor[0] = 0;
					samplerDesc.BorderColor[1] = 0;
					samplerDesc.BorderColor[2] = 0;
					samplerDesc.BorderColor[3] = 0;
					samplerDesc.MinLOD = 0;
					samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

					ID3D11SamplerState* sampler;
					d3dDevice->CreateSamplerState(&samplerDesc, &sampler);

					gs->samplerCmp = sampler;
				}
			}

			// @BlendStates.
			{
				gs->blendStateCount = 0;

				D3D11_BLEND_DESC blendDesc = {};
				blendDesc.RenderTarget[0] = {false, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL};
				d3dDevice->CreateBlendState(&blendDesc, &gs->blendStates[Blend_State_NoBlend]);

				gs->blendStates[Blend_State_Blend] = dxCreateBlendState(
				                                                        D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD);

				gs->blendStates[Blend_State_Blend] = dxCreateBlendState(
				                                                        D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD);

				gs->blendStates[Blend_State_DrawOverlay] = dxCreateBlendState(
				                                                              D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, 
				                                                              D3D11_BLEND_ONE,       D3D11_BLEND_ONE,           D3D11_BLEND_OP_ADD);

				gs->blendStates[Blend_State_BlitOverlay] = dxCreateBlendState(
				                                                              D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD); 

				// gs->blendStates[Blend_State_BlendAlphaCoverage] = dxCreateBlendState(
				// 	D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, true);

				gs->blendStates[Blend_State_BlendAlphaCoverage] = dxCreateBlendState(
				                                                                     D3D11_BLEND_ZERO, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, true, false);
			}

			// @FrameBuffers.
			{
				gs->frameBufferCountMax = 20;
				gs->frameBufferCount = 0;
				gs->frameBuffers = getPArray(FrameBuffer, gs->frameBufferCountMax);

				addFrameBuffers();
			}

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
				theGState->d3dDevice->CreateTexture2D(&texDesc, 0, &texture);
				ID3D11ShaderResourceView* view;
				theGState->d3dDevice->CreateShaderResourceView(texture, &SMViewDesc, &view);

				theGState->cubeMapTexture = texture;
				theGState->cubeMapView = view;
			}

			{
				// gs->shadowMapSize = 1024;
				// gs->shadowMapSize = 2048;
				gs->shadowMapSize = 4096;
			}
		}

		// @WatchFolders.
		{
			char* folders[] = {App_Texture_Folder, App_Mesh_Folder};
			initWatchFolders(sd, folders, arrayCount(folders));

			for(int i = 0; i < gs->textureCount; i++) {
				Texture* tex = gs->textures + i;
				char* path = tex->file;
				tex->assetInfo.lastWriteTime = getLastWriteTime(path);
			}

			for(int i = 0; i < gs->meshCount; i++) {
				Mesh* m = gs->meshes + i;
				char* path = m->file;
				m->assetInfo[0].lastWriteTime = getLastWriteTime(path);
				m->assetInfo[1].lastWriteTime = getLastWriteTime(m->mtlFile);
			}
		}

		{
			theGState->fontFolders[theGState->fontFolderCount++] = getPStringCpy(App_Font_Folder);
			char* windowsFontFolder = fillString("%s%s", getenv(Windows_Font_Path_Variable), Windows_Font_Folder);
			theGState->fontFolders[theGState->fontFolderCount++] = getPStringCpy(windowsFontFolder);

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

					appWriteSessionSettings(App_Session_File, &at);
				}

				// @AppSessionLoad
				{
					AppSessionSettings at = {};
					appReadSessionSettings(App_Session_File, &at);

					Recti r = rectiRound(at.windowRect);
					MoveWindow(windowHandle, r.left, r.top, r.right-r.left, r.bottom-r.top, true);

					updateResolution(windowHandle, ws);

					appSessionSettings = at;
				}
			}
		}

		// @AppInit.
		{
			ad->audioState.masterVolume = 0.5f;

			ad->gameMode = GAME_MODE_LOAD;
			ad->menu.activeId = 0;

			#if SHIPPING_MODE
			ad->captureMouse = true;
			#else 
			ad->captureMouse = false;
			ad->debugMouse = true;
			#endif

			ad->volumeFootsteps = 0.2f;
			ad->volumeGeneral = 0.5f;
			ad->volumeMenu = 0.7f;

			ad->mouseSensitivity = 0.2f;

			ad->redrawSkyBox = true;

			//

			ad->particleListsSize = 100;
			for(int i = 0; i < arrayCount(ad->particleLists); i++) {
				ad->particleLists[i] = getPArray(Particle, ad->particleListsSize);
				ad->particleListUsage[i] = false;
			}

			// Entity.

			ad->playerMode = false;

			ad->entityList.size = 1000;
			ad->entityList.e = (Entity*)getPMemory(sizeof(Entity)*ad->entityList.size);
			for(int i = 0; i < ad->entityList.size; i++) ad->entityList.e[i].init = false;

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

					if(settings.fullscreen) setWindowMode(windowHandle, ws, WINDOW_MODE_FULLBORDERLESS);
					else setWindowMode(windowHandle, ws, WINDOW_MODE_WINDOWED);

					ws->vsync = settings.vsync;
					ad->frameRateCap = settings.frameRateCap;
					ad->resolutionScale = settings.resolutionScale;
					ad->audioState.masterVolume = settings.volume;
					ad->mouseSensitivity = settings.mouseSensitivity;
					ad->fieldOfView = settings.fieldOfView;
				}
			}
		}
	}

	// @AppStart.

	if(reload) {
		TIMER_BLOCK_NAMED("Reload");

		SetWindowLongPtr(sd->windowHandle, GWLP_WNDPROC, (LONG_PTR)mainWindowCallBack);
		SetWindowLongPtr(sd->windowHandle, GWLP_USERDATA, (LONG_PTR)sd);

		DeleteFiber(sd->messageFiber);
		sd->messageFiber = CreateFiber(0, (PFIBER_START_ROUTINE)updateInput, sd);

		theGState->screenRes = ws->currentRes;
		theGState->screenRect = getScreenRect(ws);

		if(HOTRELOAD_SHADERS) {
			dxLoadShaders();
		}

		// Bad news.
		for(int i = 0; i < arrayCount(theGState->fonts); i++) {
			for(int j = 0; j < arrayCount(theGState->fonts[0]); j++) {
				Font* font = &theGState->fonts[i][j];
				if(font->heightIndex != 0) {
					freeFont(font);
				} else break;
			}
		}
	}


	// Update timer.
	{
		ds->debugTimer.start();
		gs->timer.start();

		if(init) {
			ds->frameTimer.start();
			ds->dt = 1/(float)ws->refreshRate;

		} else {
			ds->dt = ds->frameTimer.update();
			ds->time += ds->dt;

			ds->fpsTime += ds->dt;
			ds->fpsCounter++;
			if(ds->fpsTime >= 1) {
				ds->avgFps = 1 / (ds->fpsTime / (f64)ds->fpsCounter);
				ds->fpsTime = 0;
				ds->fpsCounter = 0;
			}
		}
	}

	clearTMemory();

	//

	// @HotRealoadAssets.
	{
		reloadChangedFiles(sd);
	}

	// Update input.
	{
		TIMER_BLOCK_NAMED("Input");

		inputPrepare(ds->input);
		SwitchToFiber(sd->messageFiber);

		// Beware, changes to ad->input have no effect on the next frame, 
		// because we override it every time.
		ad->input = *ds->input;
		if(ad->input.closeWindow) *isRunning = false;

		// Stop debug gui game interaction.
		{
			bool blockInput = false;
			bool blockMouse = false;

			if(newGuiSomeoneActive(&ds->gui) || newGuiSomeoneHot(&ds->gui)) {
				blockInput = true;
				blockMouse = true;
			}

			Console* con = &ds->console;
			if(pointInRect(ds->input->mousePosNegative, con->consoleRect)) blockMouse = true;
			if(con->isActive) blockInput = true;


			if(blockMouse) {
				input->mousePos = vec2(-1,-1);
				input->mousePosNegative = vec2(-1,-1);
				input->mousePosScreen = vec2(-1,-1);
				input->mousePosNegativeScreen = vec2(-1,-1);
			}

			if(blockInput) {
				memset(ad->input.keysPressed, 0, sizeof(ad->input.keysPressed));
				memset(ad->input.keysDown, 0, sizeof(ad->input.keysDown));
			}
		}

		ad->dt = ds->dt;
		ad->time = ds->time;

		ad->frameCount++;

		sd->fontHeight = getSystemFontHeight(sd->windowHandle);
		ds->fontHeight = roundInt(ds->fontScale*sd->fontHeight);
		ds->fontHeightScaled = roundInt(ds->fontHeight * ws->windowScale);

		if(mouseInClientArea(windowHandle)) updateCursorIcon(ws);
	}

	if((input->keysPressed[KEYCODE_F11] || input->altEnter) && !sd->maximized) {
		// bool goFullscreen = !ws->fullscreen;

 		// gs->swapChain->SetFullscreenState(goFullscreen, 0);
 		// ws->fullscreen = !ws->fullscreen;

		if(ws->fullscreen) setWindowMode(windowHandle, ws, WINDOW_MODE_WINDOWED);
		else setWindowMode(windowHandle, ws, WINDOW_MODE_FULLBORDERLESS);
	}


	if(ds->input->resize || init) {
		if(!windowIsMinimized(windowHandle)) {
			updateResolution(windowHandle, ws);
			ad->updateFrameBuffers = true;
		}
		ds->input->resize = false;
	}

	// @ResizeFramebuffers.
	if(ad->updateFrameBuffers) {
		TIMER_BLOCK_NAMED("Upd FBOs");

		GraphicsState* gs = theGState;

		ad->updateFrameBuffers = false;
		ad->aspectRatio = ws->aspectRatio;
		
		gs->screenRes = ws->currentRes;
		gs->screenRect = getScreenRect(ws);

		ad->cur3dBufferRes = ws->currentRes;
		if(ad->resolutionScale < 1.0f) {
			ad->cur3dBufferRes.w = roundInt(ad->cur3dBufferRes.w * ad->resolutionScale);
			ad->cur3dBufferRes.h = roundInt(ad->cur3dBufferRes.h * ad->resolutionScale);
		}

		Vec2i res3d = ad->cur3dBufferRes;
		Vec2i res2d = ws->currentRes;

		// Resize backbuffer.
		{
			if(gs->backBufferView != 0) {
				gs->d3ddc->OMSetRenderTargets(0, 0, 0);

				// gs->backBufferResource->Release();
				gs->backBufferView->Release();

				// gs->d3ddc->ClearState();
			}

			gs->swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

			ID3D11Texture2D* backBufferResource;
			gs->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)(&backBufferResource));

			gs->d3dDevice->CreateRenderTargetView(backBufferResource, NULL, &gs->backBufferView);

			backBufferResource->Release();
		}

		resizeFrameBuffers(res3d, res2d, ad->msaaSamples);
		if(init) {
			dxSetFrameBuffer("DebugMsaa",   res2d, ad->msaaSamples);
			dxSetFrameBuffer("DebugNoMsaa", res2d, 1);
		}

		ad->updateDebugFrameBuffer = true;
	}

	// Handle recording.
	{
		ds->recState.update(&ad->input);

		if(ds->recState.playbackPaused) {
			if(!ds->recState.justPaused) goto endOfMainLabel;
		}
	} 

	// Mouse capture.
	if(!init)
	{
		if(sd->killedFocus && ad->captureMouse) {
			ad->captureMouse = false;
			ad->lostFocusWhileCaptured = true;
		}

		if(ad->lostFocusWhileCaptured && windowHasFocus(windowHandle)) {
			if(input->mouseButtonPressed[0] && mouseInClientArea(windowHandle)) {
				ad->captureMouse = true;
				input->mouseButtonPressed[0] = false;

				ad->lostFocusWhileCaptured = false;
			}
		}

		if(input->keysPressed[KEYCODE_F3]) {
			ad->debugMouse = !ad->debugMouse;
		}

		bool showMouse = ad->debugMouse;

		if(!ad->captureMouse) {
			if(!showMouse) {
				input->mouseButtonPressed[1] = false;
				ad->captureMouse = true;

				GetCursorPos(&ws->lastMousePosition);
			}

		} else {
			if(showMouse) {
				ad->captureMouse = false;

				if(ws->lastMousePosition.x == 0 && ws->lastMousePosition.y == 0) {
					int w,h;
					Vec2i wPos;
					getWindowProperties(windowHandle, &w, &h, 0, 0, &wPos.x, &wPos.y);
					ws->lastMousePosition.x = wPos.x + w/2;
					ws->lastMousePosition.y = wPos.y + h/2;
				}

				SetCursorPos(ws->lastMousePosition.x, ws->lastMousePosition.y);
			}
		}

		ad->fpsMode = ad->captureMouse && windowHasFocus(windowHandle);
		if(ad->fpsMode) {
			int w,h;
			Vec2i wPos;
			getWindowProperties(windowHandle, &w, &h, 0, 0, &wPos.x, &wPos.y);

			SetCursorPos(wPos.x + w/2, wPos.y + h/2);
			input->lastMousePos = getMousePos(windowHandle,false);

			showCursor(false);
		} else {
			showCursor(true);
		}
	} 

	// @GraphicsSetup.
	{
		TIMER_BLOCK_NAMED("Graphics Setup")

		{
			D3D11_RASTERIZER_DESC rasterizerState;
			rasterizerState.CullMode = D3D11_CULL_BACK;
			rasterizerState.FillMode = D3D11_FILL_SOLID;
			rasterizerState.FrontCounterClockwise = false;
			rasterizerState.DepthBias = 0;
			rasterizerState.DepthBiasClamp = 0.0f;
			rasterizerState.SlopeScaledDepthBias = 0.0f;
			rasterizerState.DepthClipEnable = true;
			rasterizerState.ScissorEnable = false;
			rasterizerState.MultisampleEnable = true;
			rasterizerState.AntialiasedLineEnable = true;
			
			gs->rasterizerState = rasterizerState;

			dxSetRasterizer();
		}

		{
			D3D11_DEPTH_STENCIL_DESC dsDesc;

			dsDesc.DepthEnable = true;
			dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

			dsDesc.StencilEnable = false;
			dsDesc.StencilReadMask = 0xFF;
			dsDesc.StencilWriteMask = 0xFF;

			dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
			dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

			dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
			dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

			gs->depthStencilState = dsDesc;

			dxSetDepthStencil();
		}

		dxSetBlendState(Blend_State_Blend);

		dxScissor(getScreenRect(ws));
		dxScissorState(true);

		gs->currentShader = -1;
		gs->zLevel = 0;

		{
			Vec4 color = vec4(0.0f,1.0f);
			gs->d3ddc->ClearRenderTargetView(gs->backBufferView, color.e);

			clearFrameBuffers();
		}

		dxViewPort(ws->currentRes);
		dxBindFrameBuffer("3dMsaa", "ds3d");

		// @ShaderSetup.
		{
			Rect sr = getScreenRect(ws);
			ad->view2d = identityMatrix();
			ad->ortho  = orthoMatrixZ01(0, sr.bottom, sr.right, 0, 10, -10);

			ad->view = viewMatrix(ad->activeCam.pos, -ad->activeCam.look, ad->activeCam.up, ad->activeCam.right);
			ad->proj = projMatrixZ01(degreeToRadian(ad->fieldOfView), ad->aspectRatio, ad->nearPlane, ad->farPlane);

			ad->viewInv = viewMatrixInv(ad->activeCam.pos, -ad->activeCam.look, ad->activeCam.up, ad->activeCam.right);
			ad->projInv = projMatrixZ01Inv(ad->proj);

			{
				PrimitiveShaderVars* primitiveVars = dxGetShaderVars(Primitive);
				SkyShaderVars* skyVars = dxGetShaderVars(Sky);
				MainShaderVars* mainVars = dxGetShaderVars(Main);
				GradientShaderVars* gradientVars = dxGetShaderVars(Gradient);
				ShadowShaderVars* shadowVars = dxGetShaderVars(Shadow);

				*primitiveVars = {};
				primitiveVars->view = ad->view2d;
				primitiveVars->proj = ad->ortho;
				primitiveVars->gammaGradient = false;

				{
					if(init || reload) {
						*skyVars = {};
						// ad->sunAngles = vec2(4.75f, 0.5f);
						ad->sunAngles = vec2(5.0f, 0.5f);

						skyVars->spotBrightness          = 4.0f;
						skyVars->mieBrightness           = 0.3f;
						skyVars->mieDistribution         = 0.10f;
						skyVars->mieStrength             = 0.2f;
						skyVars->mieCollectionPower      = 0.1f;
						skyVars->rayleighBrightness      = 0.7f;
						skyVars->rayleighStrength        = 0.5f;
						skyVars->rayleighCollectionPower = 0.25f;
						skyVars->scatterStrength         = 0.1f;
						skyVars->surfaceHeight           = 0.99;
						skyVars->intensity               = 1.8;
						skyVars->stepCount               = 16;
						skyVars->horizonOffset           = 0.5f;
						skyVars->sunOffset               = 0.35f;
					}

					skyVars->viewInv = ad->viewInv;
					skyVars->projInv = ad->projInv;

					skyVars->sunDir = quat(ad->sunAngles.x, vec3(0,0,-1)) * quat(-ad->sunAngles.y, vec3(0,1,0)) * vec3(1,0,0);
					skyVars->sunDir = norm(skyVars->sunDir);
				}

				*mainVars = {};
				mainVars->mvp.viewProj = ad->proj * ad->view;
				mainVars->camPos = ad->activeCam.pos;
				// mainVars->ambient = vec3(0.005f);
				// mainVars->ambient = vec3(0.2f);

				mainVars->ambient = vec3(0.27f,0.55f,0.72f);
				mainVars->ambient = mainVars->ambient * 0.2f;
				// mainVars->ambient = vec3(0.2f);

				mainVars->light.dir = -skyVars->sunDir;
				// mainVars->light.color = vec3(1.0f);
				mainVars->light.color = vec3(1.0f);
				mainVars->lightCount = 1;
				mainVars->shadowMapSize = vec2(gs->shadowMapSize);
				mainVars->sharpenAlpha = 0;

				mainVars->farTessDistance = 20.0f;
				mainVars->closeTessDistance = 1.0f;
				mainVars->farTessFactor = 1;
				mainVars->closeTessFactor = 10;

				{
					Vec2 data[] = {
						{0.410875, -0.504685},
						{0.153835, -0.060955},
						{0.172805, -0.847635},
						{0.778885, -0.081865},
						{-0.063715, -0.477125},
						{-0.591845, -0.496635},
						{-0.443255, 0.005445},
						{-0.786705, -0.211425},
						{-0.448325, 0.364535},
						{0.027865, 0.299825},
						{-0.173285, 0.719705},
						{-0.747335, 0.200035},
						{0.560205, 0.704155},
						{0.170125, 0.730885},
						{0.433185, 0.316815},
						{0.913445, 0.267335},
					};

					int count = arrayCount(data);
					for(int i = 0; i < count; i++) {
						mainVars->samples.data[i].xy = data[i];
					}
					mainVars->samples.count = count;
				}

				*gradientVars = {};
				gradientVars->view = ad->view2d;
				gradientVars->proj = ad->ortho;

				*shadowVars = {};

				shadowVars->camPos = ad->activeCam.pos;
				shadowVars->farTessDistance = 20.0f;
				shadowVars->closeTessDistance = 1.0f;
				shadowVars->farTessFactor = 1;
				shadowVars->closeTessFactor = 10;
			}
		}
	}

	// @AppLoop.

	// @Menu.

	if(ad->gameMode == GAME_MODE_MENU) {

		dxViewPort(ws->currentRes);
		dxGetShaderVars(Primitive)->view = ad->view2d;
		dxGetShaderVars(Primitive)->proj = ad->ortho;
		dxSetShaderAndPushConstants(Shader_Primitive);
		dxBindFrameBuffer("2dMsaa");

		//

		Rect sr = getScreenRect(ws);
		Vec2 top = sr.t();
		float rHeight = sr.h();
		float rWidth = sr.w();

		int titleFontHeight = ds->fontHeightScaled * 6.0f;
		int optionFontHeight = titleFontHeight * 0.45f;
		Font* titleFont = getFont("Merriweather-Regular.ttf", titleFontHeight);
		Font* font = getFont("LiberationSans-Regular.ttf", optionFontHeight);

		Vec4 cBackground = vec4(hslToRgbf(0.93f,0.5f,0.13f),1);
		Vec4 cTitle = vec4(1,1);
		Vec4 cTitleShadow = vec4(0,0,0,1);
		Vec4 cOption = vec4(0.5f,1);
		Vec4 cOptionActive = vec4(0.9f,1);
		Vec4 cOptionShadow1 = vec4(0,1);
		Vec4 cOptionShadow2 = vec4(hslToRgbf(0.55f,0.5f,0.5f), 1);
		Vec4 cOptionShadow = vec4(0,1);

		float titleShadowSize = titleFontHeight * 0.07f;
		float optionShadowSize = optionFontHeight * 0.07f;

		float buttonAnimSpeed = 4;

		float optionOffset = optionFontHeight*1.2f;
		float settingsOffset = 0.30f;

		TextSettings tsTitle = textSettings(titleFont, cTitle, TEXTSHADOW_MODE_SHADOW, vec2(1,-1), titleShadowSize, cTitleShadow);


		MainMenu* menu = &ad->menu;
		menuSetInput(menu, input);
		menu->volume = ad->volumeMenu;

		bool selectionChange = false;

		if(input->keysPressed[KEYCODE_DOWN]) {
			playSound("menuSelect");
			menu->activeId++;
			selectionChange = true;
		}
		if(input->keysPressed[KEYCODE_UP]) {
			playSound("menuSelect");
			menu->activeId--;
			selectionChange = true;
		}

		if(menu->currentId > 0)
			menu->activeId = mod(menu->activeId, menu->currentId);

		{
			if(selectionChange) {
				menu->buttonAnimState = 0;
			}

			menu->buttonAnimState += ad->dt * buttonAnimSpeed;
			float anim = (cos(menu->buttonAnimState) + 1)/2.0f;
			anim = powf(anim, 0.5f);
			Vec4 cOptionShadowActive = vec4(0,1);
			cOptionShadowActive.rgb = lerp(anim, cOptionShadow1.rgb, cOptionShadow2.rgb);

			menu->currentId = 0;
			menu->font = font;
			menu->cOption = cOption;
			menu->cOptionActive = cOptionActive;
			menu->cOptionShadow = cOptionShadow;
			menu->cOptionShadowActive = cOptionShadowActive;
			menu->optionShadowSize = optionShadowSize;
		}

		dxDrawRect(sr, cBackground);

		if(menu->screen == MENU_SCREEN_MAIN) {

			Vec2 p = top - vec2(0, rHeight*0.2f);
			drawText("???", p, vec2i(0,0), tsTitle);

			bool gameRunning = menu->gameRunning;

			int optionCount = gameRunning ? 4 : 3;
			p.y = sr.c().y + ((optionCount-1)*optionOffset)/2;

			if(gameRunning) {
				if(menuOption(menu, "Resume", p, vec2i(0,0)) || 
				   input->keysPressed[KEYCODE_ESCAPE]) {
					input->keysPressed[KEYCODE_ESCAPE] = false;

					ad->gameMode = GAME_MODE_MAIN;
				}
				p.y -= optionOffset;
			}

			if(menuOption(menu, "New Game", p, vec2i(0,0))) {
				playSound("gameStart");
				ad->gameMode = GAME_MODE_LOAD;
				ad->newGame = true;
			}

			p.y -= optionOffset;
			if(menuOption(menu, "Settings", p, vec2i(0,0))) {
				playSound("menuPush");
				menu->screen = MENU_SCREEN_SETTINGS;
				menu->activeId = 0;
			}

			p.y -= optionOffset;
			if(menuOption(menu, "Exit", p, vec2i(0,0))) {
				*isRunning = false;
			}

		} else if(menu->screen == MENU_SCREEN_SETTINGS) {

			Vec2 p = top - vec2(0, rHeight*0.2f);
			Vec2 pos;
			float leftX = rWidth * settingsOffset;
			float rightX = rWidth * (1-settingsOffset);

			drawText("Settings", p, vec2i(0,0), tsTitle);

			int optionCount = 7;
			p.y = sr.c().y + ((optionCount-1)*optionOffset)/2;


				// List settings.

			GameSettings* settings = &ad->settings;


			p.x = leftX;
			menuOption(menu, "Fullscreen", p, vec2i(-1,0));

			bool tempBool = ws->fullscreen;
			if(menuOptionBool(menu, vec2(rightX, p.y), &tempBool)) {
				if(ws->fullscreen) setWindowMode(windowHandle, ws, WINDOW_MODE_WINDOWED);
				else setWindowMode(windowHandle, ws, WINDOW_MODE_FULLBORDERLESS);
			}

				//

			p.y -= optionOffset; p.x = leftX;
			menuOption(menu, "VSync", p, vec2i(-1,0));

			menuOptionBool(menu, vec2(rightX, p.y), &ws->vsync);

				//

			p.y -= optionOffset; p.x = leftX;
			menuOption(menu, "Max FPS", p, vec2i(-1,0));

			menuOptionSliderInt(menu, vec2(rightX, p.y), &ad->frameRateCap, ws->frameRate, ad->maxFrameRate, 1);

				//

			p.y -= optionOffset; p.x = leftX;
			menuOption(menu, "Resolution Scale", p, vec2i(-1,0));

			if(menuOptionSliderFloat(menu, vec2(rightX, p.y), &ad->resolutionScale, 0.2f, 1, 0.1f, 1)) {
				ad->updateFrameBuffers = true;
			}

				//

			p.y -= optionOffset; p.x = leftX;
			menuOption(menu, "Volume", p, vec2i(-1,0));

			menuOptionSliderFloat(menu, vec2(rightX, p.y), &ad->audioState.masterVolume, 0.0f, 1, 0.1f, 1);

				//

			p.y -= optionOffset; p.x = leftX;
			menuOption(menu, "Mouse Sensitivity", p, vec2i(-1,0));

			menuOptionSliderFloat(menu, vec2(rightX, p.y), &ad->mouseSensitivity, 0.01f, 2, 0.01f, 2);

				//

			p.y -= optionOffset; p.x = leftX;
			menuOption(menu, "Field of View", p, vec2i(-1,0));

			menuOptionSliderInt(menu, vec2(rightX, p.y), &ad->fieldOfView, 20, 90, 1);

				//

			p.y -= optionOffset;

				//

			p.y -= optionOffset;

			p.x = rWidth * 0.5f;
			p.y -= optionOffset;
			if(menuOption(menu, "Back", p, vec2i(0,0)) || 
			   input->keysPressed[KEYCODE_ESCAPE] ||
			   input->keysPressed[KEYCODE_BACKSPACE]) {
				playSound("menuPop");

				menu->screen = MENU_SCREEN_MAIN;
				menu->activeId = 0;
			}

		}
	}

	if(ad->gameMode == GAME_MODE_LOAD) {
		for(int i = 0; i < ad->entityList.size; i++) ad->entityList.e[i].init = false;

			// Init player.
		{
			float v = randomFloat(0,M_2PI);
			Vec3 startRot = vec3(v,0,0);

			Entity player = {};
			Vec3 playerDim = vec3(0.8f, 0.8f, 1.8f);
			float camOff = playerDim.z*0.5f - playerDim.x*0.25f;
				// initEntity(&player, ET_Player, vec3(0.5f,0.5f,40), playerDim);
			initEntity(&player, ET_Player, vec3(20,20,40), playerDim, vec2i(0,0));
			player.camOff = vec3(0,0,camOff);
			player.rot = startRot;
			player.onGround = false;
			strCpy(player.name, "Player");
			ad->footstepSoundValue = 0;

			ad->player = addEntity(&ad->entityList, &player);
		}

			// Debug cam.
		{
			Entity freeCam = {};
			initEntity(&freeCam, ET_Camera, vec3(-4,5,3), vec3(0,0,0), vec2i(0,0));
			freeCam.rot.xy = vec2(0.8f,-0.2f);
			strCpy(freeCam.name, "Camera");
			ad->cameraEntity = addEntity(&ad->entityList, &freeCam);
		}

		ad->gameMode = GAME_MODE_MAIN;

	} else if(ad->gameMode == GAME_MODE_MAIN) {
		TIMER_BLOCK_NAMED("Main Mode");

		if(input->keysPressed[KEYCODE_ESCAPE]) {
			ad->gameMode = GAME_MODE_MENU;
			ad->menu.gameRunning = true;
			ad->menu.activeId = 0;
		}

		{
			Entity* player = ad->player;
			Entity* camera = ad->cameraEntity;

			// if(input->keysPressed[KEYCODE_F4]) {
			// 	if(ad->playerMode) {
			// 		camera->pos = player->pos + player->camOff;
			// 		camera->dir = player->dir;
			// 		camera->rot = player->rot;
			// 		camera->rotAngle = player->rotAngle;
			// 	}
			// 	ad->playerMode = !ad->playerMode;
			// }

			if(ad->playerMode) camera->vel = vec3(0,0,0);
			else player->vel = vec3(0,0,0);

			// if(!ad->playerMode && input->keysPressed[KEYCODE_SPACE]) {
			// 	player->pos = camera->pos;
			// 	player->dir = camera->dir;
			// 	player->rot = camera->rot;
			// 	player->rotAngle = camera->rotAngle;
			// 	player->vel = camera->vel;
			// 	player->chunk = camera->chunk;
			// 	ad->playerMode = true;
			// 	input->keysPressed[KEYCODE_SPACE] = false;
			// 	input->keysDown[KEYCODE_SPACE] = false;
			// }
		}

		{
			dxViewPort(ad->cur3dBufferRes);

			// @RedrawCubeMap.
			// @Sky.
			bool cachedSky = true;
			if(cachedSky) {
				if(ad->redrawSkyBox || reload) {
					ad->redrawSkyBox = false;

					dxDepthTest(false); defer{dxDepthTest(true);};
					dxScissorState(false); defer{dxScissorState(true);};

					dxSetShader(Shader_Sky);

					dxViewPort(vec2i(gs->cubeMapSize)); defer{dxViewPort(ad->cur3dBufferRes);};

					gs->d3ddc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
					SkyShaderVars* skyVars = dxGetShaderVars(Sky);

					Mat4 proj = projMatrixZ01(M_PI_2, 1, 0.1f, 2);
					Mat4 projInv = projMatrixZ01Inv(proj);
					skyVars->projInv = projInv;

					Vec3 dirs[6][3] = { 
						{ { 1, 0, 0}, { 0, 0, 1}, { 0,-1, 0} },
						{ {-1, 0, 0}, { 0, 0, 1}, { 0, 1, 0} },
						{ { 0, 0, 1}, { 0,-1, 0}, { 1, 0, 0} },
						{ { 0, 0,-1}, { 0, 1, 0}, { 1, 0, 0} },
						{ { 0, 1, 0}, { 0, 0, 1}, { 1, 0, 0} },
						{ { 0,-1, 0}, { 0, 0, 1}, {-1, 0, 0} },
					};
					for(int i = 0; i < 6; i++) {
						D3D11_RENDER_TARGET_VIEW_DESC desc = {};
						desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
						desc.Texture2DArray.MipSlice = 0;
						desc.Texture2DArray.ArraySize = 1;
						desc.Texture2DArray.FirstArraySlice = i;

						ID3D11RenderTargetView* view; defer{view->Release();};
						gs->d3dDevice->CreateRenderTargetView(gs->cubeMapTexture, &desc, &view);

						gs->d3ddc->OMSetRenderTargets(1, &view, 0);

						skyVars->viewInv = viewMatrixInv(vec3(0.0f), -dirs[i][0], dirs[i][1], dirs[i][2]);
						dxPushShaderConstants(Shader_Sky);

						gs->d3ddc->Draw(4, 0);
					}
				}

			} else {
				dxDepthTest(false); defer{dxDepthTest(true);};
				dxScissorState(false); defer{dxScissorState(true);};

				dxBindFrameBuffer("Sky");
				dxSetShader(Shader_Sky);

				dxPushShaderConstants(Shader_Sky);
				gs->d3ddc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
				gs->d3ddc->Draw(4, 0);
			}

			dxBindFrameBuffer("3dMsaa", "ds3d");

			// @RenderCubeMap.
			if(cachedSky) 
			{
				GraphicsState* gs = theGState;
				Shader* shader = gs->shaders + Shader_Cube;

				dxSetShader(Shader_Cube);

				Mesh* m = dxGetMesh("cube\\obj.obj");
				dxSetMesh(m);

				gs->d3ddc->PSSetShaderResources(0, 1, &gs->cubeMapView);
				gs->d3ddc->PSSetSamplers(0, 1, &gs->sampler);

				{
					Vec3 skyBoxRot = ad->cameraEntity->rot;
					Camera skyBoxCam = getCamData(vec3(0,0,0), skyBoxRot, vec3(0,0,0), vec3(0,1,0), vec3(0,0,1));

					Mat4 view = viewMatrix(skyBoxCam.pos, -skyBoxCam.look, skyBoxCam.up, skyBoxCam.right);
					Mat4 proj = projMatrix(degreeToRadian(ad->fieldOfView), ad->aspectRatio, 0.0f, 2);

					dxGetShaderVars(Cube)->view = view;
					dxGetShaderVars(Cube)->proj = proj;
				}

				dxCullState(false); defer{dxCullState(true);};
				dxDepthTest(false); defer{dxDepthTest(true);};

				dxSetShaderAndPushConstants(Shader_Cube);
				gs->d3ddc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				gs->d3ddc->Draw(m->size, 0);

			} else {
				dxDepthTest(false); defer{dxDepthTest(true);};

				dxGetShaderVars(Primitive)->view = ad->view2d;
				dxGetShaderVars(Primitive)->proj = orthoMatrixZ01(0, 0, 1, 1, 10, -10);
				dxPushShaderConstants(Shader_Primitive);

				dxDrawRect(rectTLDim(0,1,1,1), vec4(1), dxGetFrameBuffer("Sky")->shaderResourceView);	
			}

			dxGetShaderVars(Primitive)->view = ad->view;
			dxGetShaderVars(Primitive)->proj = ad->proj;
			dxPushShaderConstants(Shader_Primitive);
		}

		{
			// if(input->keysPressed[KEYCODE_R]) {
			// 	Entity e = {};

			// 	e.init = true;
			// 	e.type = ET_Sound;
			// 	e.pos = vec3(0,0,10);

			// 	e.trackIndex = playSound("ping");

			// 	ad->entityList.e[2] = e;
			// }

			// if(input->keysPressed[KEYCODE_SPACE]) {
			// 	Entity e = {};

			// 	e.init = true;
			// 	e.type = ET_Sound;
			// 	e.pos = vec3(0,0,10);

			// 	e.trackIndex = playSound("music");

			// 	ad->entityList.e[3] = e;
			// }
		}

		// @Entities.
		{
			TIMER_BLOCK_NAMED("Upd Entities");

			for(int i = 0; i < ad->entityList.size; i++) {
				Entity* e = &ad->entityList.e[i];
				if(!e->init) continue;

				float dt = ad->dt;

				switch(e->type) {

					case ET_Camera: {
						if(ad->playerMode) continue;

						if((!ad->fpsMode && input->mouseButtonDown[1]) || ad->fpsMode)
							entityMouseLook(ad->cameraEntity, input, ad->mouseSensitivity);

						e->acc = vec3(0,0,0);
						// float speed = !input->keysDown[KEYCODE_T] ? 150 : 1000;
						float speed = !input->keysDown[KEYCODE_T] ? 25 : 250;
						entityKeyboardAcceleration(ad->cameraEntity, input, speed, 2.0f, true);

						e->vel = e->vel + e->acc*dt;
						float friction = 0.01f;
						e->vel = e->vel * pow(friction,dt);

						if(e->vel != vec3(0,0,0)) {
							e->pos = e->pos - 0.5f*e->acc*dt*dt + e->vel*dt;
						}

						ad->activeCam = getCamData(ad->cameraEntity->pos, ad->cameraEntity->rot, ad->cameraEntity->camOff);

					} break;

					case ET_Sound: {

						Track* track = theAudioState->tracks + e->trackIndex;

						if(!track->used) {
							e->init = false;
							break;
						}

						track->isSpatial = true;
						track->pos = e->pos;

						dxSetShader(Shader_Main);
						// dxDrawMesh(dxGetMesh("sphere\\sphere.obj"), vec3(0,0,10), vec3(0.25f), vec4(1,1,1,1));

					} break;

				}
			}
		}

		// @Animation.
		{
			Mesh* mesh = dxGetMesh("figure\\figure.mesh");
			AnimationPlayer* player = &mesh->animPlayer;

			if(!player->init) {
				player->init = true;

				player->mesh = mesh;

				player->time = 0;

				player->speed = 1;
				player->fps = 30;
				player->noInterp = false;
				player->noLocomotion = false;
				player->loop = false;

				player->setAnim("idle.anim");
				player->noLocomotion = false;

				ad->figureAnimation = 0;
				ad->figureMesh = mesh;
				ad->figureSpeed = 1;
			}

			player->animation = &player->mesh->animations[ad->figureAnimation];
			player->update(ad->dt * ad->figureSpeed);
		}

		// @Rendering.

		// @3d.
		{
			// @Grid.
			{
				dxSetShader(Shader_Primitive);

				float zOff = -0.1f;

				Vec4 lineColor = vec4(0.3f,1);
				float size = roundf(2);
				float count = roundf(40 / size);
				float dim = size*count;
				Vec3 start = vec3(-dim/2, -dim/2, zOff);

				for(int i = 0; i < count+1; i++) {
					Vec3 p = start + vec3(1,0,0) * i*size;
					dxDrawLine(p, p + vec3(0,dim,0), lineColor);
				}

				for(int i = 0; i < count+1; i++) {
					Vec3 p = start + vec3(0,1,0) * i*size;
					dxDrawLine(p, p + vec3(dim,0,0), lineColor);
				}

				dxDrawLine(vec3(-dim/2,0,zOff), vec3(dim/2,0,zOff), vec4(0.5f,0,0,1));
				dxDrawLine(vec3(0,-dim/2,zOff), vec3(0,dim/2,zOff), vec4(0,0.5f,0,1));
			}

			dxSetShader(Shader_Main);

			// @ShadowMap.
			{
				// Draw normals.
				#if 0
				{
					// Draw debug normals.
					// dxSetShader(Shader_Primitive);
					// dxBeginPrimitiveColored(vec4(1,1), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
					// for(int i = 0; i < m->vertices.count; i++) {
					// 	MeshVertex v = m->vertices[i];
						
					// 	dxPushLine(v.pos, v.pos + v.normal*0.1f, vec4(0,0,1,1));
					// 	dxPushLine(v.pos, v.pos + v.tangent*0.1f, vec4(0,1,0,1));
					// 	dxPushLine(v.pos, v.pos + v.bitangent*0.1f, vec4(1,0,0,1));
					// }
					// dxEndPrimitive();
				}
				#endif

				// Setup objects.

				Object objA[50];
				int objC = 0;

				{
					Vec3 size = vec3(1);
					float dist = 1.25f;
					Vec3 startPos = vec3(0,-3.5f,1);
					Quat rot = quat(ad->time*0.20f, vec3(0,0,1));

					{
						char* materials[] = {"Brick Wall", "Tiles", "Metal Plate", "Metal Weave", "Marble", "Metal Grill"};
						int matCount = arrayCount(materials);

						for(int i = 0; i < matCount; i++) {
							Material* m = dxGetMaterial(fillString("%s%s", materials[i], "\\material.mtl"));

							float p = -dist*(matCount-1)/2 + (i*dist);
							
							bool alpha = false;
							if(strFind(m->name, "Metal Grill") != -1) alpha = true;

							objA[objC++] = { "cube\\obj.obj",     startPos + vec3(p,0,0),    size, vec4(1,1), quat(), m->name, alpha };
							objA[objC++] = { "cylinder\\obj.obj", startPos + vec3(p,0,1.1), size, vec4(1,1), rot, m->name, alpha };
							objA[objC++] = { "sphere\\obj.obj",   startPos + vec3(p,0,2.2f), size, vec4(1,1), rot, m->name, alpha };
						}
					}

					{
						char* materials[] = {"Rock", "Canyon Rock", "Stone Wall", "Crystals", "Snow"};
						float heightScales[] = {0.05f, 0.2f, 0.05f, 0.2f, 0.05f};
						int matCount = arrayCount(materials);

						for(int i = 0; i < matCount; i++) {
							Material* m = dxGetMaterial(fillString("%s%s", materials[i], "\\material.mtl"));

							float p = -dist*(matCount-1)/2 + (i*dist);
							
							m->heightScale = heightScales[i];

							objA[objC++] = { "sphere\\obj.obj", startPos + vec3(p,0,3.3f), size, vec4(1,1), rot, m->name};
						}
					}
				}

				char* mat = "Matte\\mat.mtl";

				objA[objC++] = {"cube\\obj.obj", vec3(0,0,-0.05f + 0.0001f), vec3(10,10,0.1f), vec4(1,1), quat(), mat};
				objA[objC++] = {"cube\\obj.obj", vec3(-3,4,0.5f), vec3(1,1,1), hslToRgbf(0,0.5f,0.5f, 1), quat(-0.6f, vec3(0,0,1)), mat};

				Quat rot = quat(M_PI_2, vec3(1,0,0));
				objA[objC++] = {"treeRealistic\\Tree.obj", vec3(3,3,0), vec3(0.8f), vec4(1,1), quat(), 0, true};

				{
					mat = "Figure\\material.mtl";

					Vec3 pos = vec3(-4,0,0);
					Vec3 scale = vec3(0.6f);
					Quat rot = quat(degreeToRadian(135), vec3(0,0,1));

					ad->figureModel = modelMatrix(pos, scale, rot);

					objA[objC++] = {"figure\\figure.mesh", pos, scale, vec4(1,1), rot, mat};
				}

				//

				// Setup light view and projection.
				Mat4 viewLight = {};
				Mat4 projLight = {};
				{
					// Hardcoding values for now.
					float dist = 8;
					float size = 15;
					Vec3 sunDir = dxGetShaderVars(Sky)->sunDir;
					viewLight = viewMatrix(sunDir * dist, sunDir);
					projLight = orthoMatrixZ01(size, size, 0, 30);
				}

				// Render scene.
				for(int stage = 0; stage < 2; stage++) {

					// Render shadow maps.
					if(stage == 0) {
						dxDepthTest(true);
						dxScissorState(false);
						dxClearFrameBuffer("Shadow");

						// dxSetBlendState(Blend_State_BlendAlphaCoverage);
						// defer{dxSetBlendState(Blend_State_Blend);};

						#if 0
						D3D11_RASTERIZER_DESC oldRasterizerState = gs->rasterizerState;
						{
							// gs->rasterizerState.DepthBias = 50000;
							// gs->rasterizerState.DepthBiasClamp = 0;
							// gs->rasterizerState.SlopeScaledDepthBias = 2;

							gs->rasterizerState.DepthBias = 0;
							gs->rasterizerState.DepthBiasClamp = 0;
							gs->rasterizerState.SlopeScaledDepthBias = 5;
						}
						dxSetRasterizer(); 
						defer{
							gs->rasterizerState = oldRasterizerState;
							dxSetRasterizer(); 
						};
						#endif

						// gs->rasterizerState.FrontCounterClockwise = true;
						// dxSetRasterizer(); 
						// defer{
						// 	gs->rasterizerState.FrontCounterClockwise = false;
						// 	dxSetRasterizer(); 
						// };

						// Remove warning.
						ID3D11ShaderResourceView* srv = 0;
						gs->d3ddc->PSSetShaderResources(5, 1, &srv);
						dxBindFrameBuffer(0, "Shadow"); 

						Vec2i vp = vec2i(theGState->shadowMapSize);
						dxViewPort(vp);

						dxSetShader(Shader_Shadow);
						dxGetShaderVars(Shadow)->viewProj = projLight * viewLight;

					} else {
						dxBindFrameBuffer("3dMsaa", "ds3d");
						dxSetShader(Shader_Main);
						dxViewPort(ad->cur3dBufferRes);

						FrameBuffer* fb = dxGetFrameBuffer("Shadow");
						gs->d3ddc->PSSetShaderResources(5, 1, &fb->shaderResourceView);

						dxGetShaderVars(Main)->mvpShadow.viewProj = projLight * viewLight;
					}

					bool alphaMode = false;
					bool first = true;
					for(int i = 0; i < objC; i++) {
						Object* obj = objA + i;

						if(obj->hasAlpha) {
							if(!alphaMode || first) {
								dxCullState(false);
								dxSetBlendState(Blend_State_BlendAlphaCoverage);

								if(stage == 0) {
									dxGetShaderVars(Shadow)->sharpenAlpha = 1;
								} else {
									dxGetShaderVars(Main)->sharpenAlpha = 1;
								}

								alphaMode = true;
							}
						} else {
							if(alphaMode || first) {
								dxCullState(true);
								dxSetBlendState(Blend_State_Blend);

								if(stage == 0) {
									dxGetShaderVars(Shadow)->sharpenAlpha = 0;
								} else {
									dxGetShaderVars(Main)->sharpenAlpha = 0;
								}

								alphaMode = false;
							}
						}

						first = false;

						bool shadow = stage == 0;
						dxDrawObject(obj, shadow);
					}
				}

				dxCullState(true);
				dxSetBlendState(Blend_State_Blend);
			}
		}

		if(ad->showSkeleton) {
			dxSetShader(Shader_Primitive);

			dxDepthTest(false);
			defer{dxDepthTest(true);};

			Mesh* mesh = ad->figureMesh;
			drawSkeleton(mesh->animPlayer.bones, ad->figureModel, &mesh->boneTree);
		}

		{
			dxResolveFrameBuffer("3dMsaa", "3dNoMsaa");
			dxViewPort(ws->currentRes);
			dxBindFrameBuffer("2dMsaa", "ds");

			dxGetShaderVars(Primitive)->view = ad->view2d;
			dxGetShaderVars(Primitive)->proj = ad->ortho;
			dxSetShaderAndPushConstants(Shader_Primitive);

			dxDepthTest(false);
			dxDrawRect(getScreenRect(ws), vec4(1), dxGetFrameBuffer("3dNoMsaa")->shaderResourceView);
			dxDepthTest(true);
		}

		// @2d.
		{
			FrameBuffer* fb = dxGetFrameBuffer("Shadow");
			Rect sr = getScreenRect(ws);
			// dxDrawRect(rectTRDim(sr.tr(),vec2(50)), vec4(1,1,1,1), fb->shaderResourceView);
		}

	}

	// {
	// 	if(input->keysPressed[KEYCODE_1]) playSound("menuSelect");
	// 	if(input->keysPressed[KEYCODE_2]) playSound("menuOption");
	// 	if(input->keysPressed[KEYCODE_3]) playSound("gameStart");
	// 	if(input->keysPressed[KEYCODE_4]) playSound("menuPush");
	// 	if(input->keysPressed[KEYCODE_5]) playSound("menuPop");
	// 	if(input->keysPressed[KEYCODE_6]) playSound("song1");
	// }

	updateAudio(&ad->audioState, ad->dt, ad->activeCam);

	endOfMainLabel:

		// @Blit.
	{
		{
			TIMER_BLOCK_NAMED("Blit");

			dxSetShader(Shader_Primitive);

			dxResolveFrameBuffer("2dMsaa", "2dNoMsaa");

			if(ds->recState.state == REC_STATE_PLAYING) {
				if(!ds->recState.playbackPaused || ds->recState.justPaused) {
					if(ds->recState.justPaused) ds->recState.justPaused = false;
					dxCopyFrameBuffer("2dNoMsaa", "2dTemp");

				} else {
					dxCopyFrameBuffer("2dTemp", "2dNoMsaa");
				}
			}

			dxResolveFrameBuffer("DebugMsaa", "DebugNoMsaa");

			dxSetBlendState(Blend_State_BlitOverlay);

			dxBindFrameBuffer("2dNoMsaa");
			FrameBuffer* fb = dxGetFrameBuffer("DebugNoMsaa");
			dxDrawRect(rectTLDim(0,0,fb->dim.w,fb->dim.h), vec4(1,ds->guiAlpha), fb->shaderResourceView);		

				//

			dxSetBlendState(Blend_State_NoBlend);
			dxDepthTest(false);
			gs->d3ddc->OMSetRenderTargets(1, &gs->backBufferView, 0);
			dxDrawRect(getScreenRect(ws), vec4(1), dxGetFrameBuffer("2dNoMsaa")->shaderResourceView);
		}

		if(ds->cpuInterval.update(ds->dt, ds->debugTimer.stop())) {
			ds->cpuTime = ds->cpuInterval.resultTime * 1000;
		}

		{
			float dtGpu = 0;
			if(gs->timer.stop()) {
				dtGpu = gs->timer.dt;
			}

			if(ds->gpuInterval.update(ds->dt, dtGpu)) {
				ds->gpuTime = ds->gpuInterval.resultTime;
			}
		}

		{
			TIMER_BLOCK_NAMED("Swap");

			bool vsync = ws->vsync;
			{
				if(vsync && sd->vsyncTempTurnOff) vsync = false;
				sd->vsyncTempTurnOff = false;
			}

				// Sleep until monitor refresh.
			{
				double frameTime = ds->swapTimer.update();
				int sleepTimeMS = 0;
				if(!init && !vsync) {
					double fullFrameTime = (1.0f/ad->frameRateCap);

					bool first = true;
					while((fullFrameTime - frameTime) * 1000.0f > 0) {
						if(first) {
							first = false;
							gs->d3ddc->Flush();
						}

						Sleep(1);
						frameTime += ds->swapTimer.update();
					}
				}
				ds->swapTimer.start();
			}

			gs->swapChain->Present(vsync,0);

			if(init) {
				showWindow(windowHandle);
			}
		}
	}

		// We have to resize the debug framebuffer here or it will get destroyed 
		// too soon and not show up when resizing the window.
	if(ad->updateDebugFrameBuffer) {
		ad->updateDebugFrameBuffer = false;

		Vec2i res2d = ws->currentRes;
		int m = ad->msaaSamples;
		dxSetFrameBuffer("DebugMsaa",   res2d, m);
		dxSetFrameBuffer("DebugNoMsaa", res2d, 1);
	}

	debugMain(ds, appMemory, ad, reload, isRunning, init, threadQueue, __COUNTER__, mouseInClientArea(windowHandle));

		// Save game settings.
	if(*isRunning == false) {
		GameSettings settings = {};
		settings.fullscreen = ws->fullscreen;
		settings.frameRateCap = ad->frameRateCap;
		settings.vsync = ws->vsync;
		settings.resolutionScale = ad->resolutionScale;
		settings.volume = ad->audioState.masterVolume;
		settings.mouseSensitivity = ad->mouseSensitivity;
		settings.fieldOfView = ad->fieldOfView;

		char* file = Game_Settings_File;
		if(fileExists(file)) {
			writeDataToFile((char*)&settings, sizeof(GameSettings), file);
		}
	}

		// @AppSessionWrite
	if(*isRunning == false) {
		Rect windowRect = getWindowWindowRect(sd->windowHandle);
		if(ws->fullscreen) windowRect = ws->previousWindowRect;

		AppSessionSettings at = {};

		at.windowRect = windowRect;
		saveAppSettings(at);
	}

	// @AppEnd.
}

#pragma optimize( "", on ) 