/*

ToDo:
- For shipping mode, pack all data into one file
- Make true fullscreen work?
- Audio debug screen.
- Clean up obj loader. Or replace with AssImp.
- Ground/heightmap, blended textures, grass, shore, ocean/water, horizon.
- Proximity grid.

- Lookup water on ShaderToy.

- Load assets better.
- Hot reloading for all assets.

- DDS file generation automation.
- Environment mapping, basic reflections.

- Make animation that demonstrates scaling.

- Frustum culling.
- Draw from back to front.

- Input recording not working right now.

- Clean up memberinfo and structinfo functions, make them member functions, and add them to typetraverse.
- Only push members that changed when editing entities.

- Make null strings work on entities instead of allocating empty strings everywhere.
- Storing only mesh name on Entity is wrong for animated meshes.

- Fix figure default pose.
- Particle brightness mipmap border problem.
- Manifold:
  - make cells go from 0 to 1 everywhere to get fixed precision.
  - Fix walk edges.

- Better/smoother movement: Acceleration, drag.
- Make Camera use transform rotation instead of "camRot".

Bugs:
- Black shadow spot where sun hits material with displacement map.
- Particle mipmaps blending dark alpha border.
  This works with pre multiplied alpha, but now things in front of particles are aliased.
- Stretched object lighting incorrect.
- Memory leak when resizing window. Something to do with framebuffers.

*/

// External.

#define WIN32_LEAN_bAND_MEAN
#define NOMINMAX
#include <windows.h>
#include <timeapi.h>

#define D3D11_IGNORE_SDK_LAYERS
#include <D3D11.h>
#include <D3Dcompiler.h>

#define _WIN32_WINNT 0x0600
#include "external\textureLoader\DDSTextureLoader.h"
#include "external\textureLoader\DDSTextureLoader.cpp"

// Stb.

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "external\stb_image.h"

// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #define STBI_ONLY_PNG
// #include "external\stb_image_write.h"

#define STB_VORBIS_NO_PUSHDATA_API
#include "external\stb_vorbis.c"

#define STB_SPRINTF_IMPLEMENTATION
#include "external\stb_sprintf.h"

#include "external\pcg_basic.c"

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
struct ProfilerTimer;
ThreadQueue*   theThreadQueue;
GraphicsState* theGState;
AudioState*    theAudioState;
MemoryBlock*   theMemory;
DebugState*    theDebugState;
ProfilerTimer* theTimer;

// Internal.

#include "userSettings.cpp"

#include "types.h"
#include "misc.cpp"
#include "string.cpp"
#include "memory.cpp"
#include "appMemory.cpp"
#include "fileIO.cpp"
#include "introspection.h"
#include "container.cpp"
#include "mathBasic.cpp"
#include "math.cpp"
#include "color.cpp"
#include "profilerTimer.cpp"
#include "interpolation.cpp"
#include "sort.cpp"
#include "misc2.cpp"
#include "miscMath.cpp"
#include "hotload.cpp"
#include "threadQueue.cpp"
#include "platformWin32.cpp"
#include "platformMisc.cpp"
#include "input.cpp"

#include "animation.h"
#include "rendering.h"
#include "mesh.h"
#include "font.h"

#include "objLoader.cpp"
#include "animation.cpp"

#include "camera.h"

#include "drawPrimitives.h"
#include "graphics.h"
#include "shader.cpp"
#include "rendering.cpp"
#include "drawPrimitives.cpp"
#include "mesh.cpp"
#include "font.cpp"
#include "gui.h"
#include "layout.cpp"
#include "gui.cpp"
#include "console.cpp"

#include "particles.cpp"
#include "entity.h"
#include "audio.cpp"
#include "walkManifold.h"
#include "gjk.cpp"
#include "walkManifold.cpp"

#include "assetReload.cpp"
#include "stateRecord.cpp"
#include "profiler.cpp"

//

#include "menu.h"
#include "app.h"

#include "serialization.h"
#include "introspection.cpp"
#include "serialization.cpp"

#include "entityUI.h"
#include "entity.cpp"
#include "maps.cpp"

#include "debug.h"

#include "entityUI.cpp"
#include "debugUI.cpp"
#include "debugMisc.cpp"
#include "debug.cpp"
#include "appInit.cpp"
#include "appSetup.cpp"

#include "appRendering.cpp"

#include "menu.cpp"
#include "appMain.cpp"

#include "versionConversion.cpp"

#ifdef FULL_OPTIMIZE
#pragma optimize( "", on )
#else 
#pragma optimize( "", off )
#endif

extern "C" APPMAINFUNCTION(appMain) {
	MemoryBlock gMemory;
	AppData* ad;
	DebugState* ds;
	setupMemoryAndGlobals(appMemory, threadQueue, &gMemory, &ad, &ds, init);

	Input* input = &ad->input;
	SystemData* sd = &ad->systemData;
	WindowSettings* ws = &ad->wSettings;
	GraphicsState* gs = &ad->GraphicsState;
	HWND windowHandle = sd->windowHandle;

	setMemory();
	clearTMemory();

	// @Init.
	if(init) {
		debugInit(ds);

		TIMER_BLOCK_NAMED("Init");

		SectionTimer st;
		st.start(&ds->debugTimer);

		systemInit(ad, ds, sd, ws, windowsData);
		st.add("systemInit");

		audioInit(&ad->audioState, ws->frameRate);
		st.add("audioInit");
		
		graphicsInit(gs, ws, sd, &st);
		// st.add("graphicsInit");
		
		watchFoldersInit(sd, gs);
		st.add("watchFoldersInit");
		
		appSessionInit(ad, sd, ws);
		st.add("appSessionInit");
		
		appInit(ad, sd, ws);
		windowHandle = sd->windowHandle;
		st.add("appInit");

		st.stopAndPrint();
	}

	// @AppSetup.
	{
		TIMER_BLOCK_NAMED("AppSetup");

		if(reload) handleReload(sd, ws);

		updateTimers(ds, gs, ws->refreshRate, &ad->dt, &ad->time, &ad->frameCount, init);
		reloadChangedFiles(sd);
		updateFontSizes(sd, &ds->fontHeight, ds->fontScale, &ds->fontHeightScaled, ws->windowScale);

		handleInput(ds, &ad->input, sd->messageFiber, sd);
		handleWindowEvents(ds->input, input, ws, windowHandle, sd->maximized, init, &ad->toggleFullscreen, &ad->updateFrameBuffers, isRunning);

		if(ad->updateFrameBuffers) {
			ad->updateFrameBuffers = false;
			ad->updateDebugFrameBuffer = true;

			updateFrameBuffersAndScreenRes(&ad->gSettings, ws->aspectRatio, ws->currentRes, init);
		}

		// Handle input recording.
		{
			ds->recState.update(&ad->input);
			if(ds->recState.playbackPaused && !ds->recState.justPaused) goto endOfMainLabel; // @Hack
		}

		if(!init) {
			handleMouseCapturing(input, &ad->mouseEvents, ds->mouseOverPanel, &ds->gui, &ws->lastMousePosition, windowHandle, sd->killedFocus);
		}

		setupGraphics(gs, ad->gSettings, gs->activeCam, init, reload);

		if(!init) {
			setupShaders(ad->gSettings, gs->activeCam, init, reload);
		}
	}

	// @AppLoop.
	if(ad->newGameMode != -1) {
		ad->gameMode = ad->newGameMode;
		ad->newGameMode = -1;
	}

	if(ad->gameMode == GAME_MODE_LOAD) {
		gameLoad(ad);
		historyReset(&ds->entityUI.history);

		// No need for loading screen right now so we switch immediately to main game mode.
		ad->gameMode = GAME_MODE_MAIN;
	}

	if(ad->gameMode == GAME_MODE_MENU) {
		menuUpdate(ad, ws, ds->fontHeightScaled, &ad->toggleFullscreen, isRunning);

	} else if(ad->gameMode == GAME_MODE_MAIN) {
		if(ds->timeMode != 0) {
			float t = pow(2.0f, abs(ds->timeMode));
			if(ds->timeMode < 0) t = 1.0f/t;
			ad->dt *= t;
		}

		if(ds->timeStop) ad->dt = 0;

		gameLoop(ad, ds, ws, ds->showUI, init, reload);
	}

	audioUpdate(&ad->audioState, ad->dt);

	endOfMainLabel:

	{
		bool windowMinimized = windowIsMinimized(windowHandle);
		resolveFrameBuffersAndSwap(ds, gs, ws->currentRes, ws->vsync, &sd->vsyncTempTurnOff, ad->frameRateCap, windowMinimized, init);
	}

	// Show window after we rendered one frame to the backbuffer.
	if(init) showWindow(sd->windowHandle);

	if(!(*isRunning)) saveAppSessionAndSettings(ad, ws, sd);

	// We have to resize the debug framebuffer here or it will get destroyed 
	// too soon and not show up when resizing the window.
	if(ad->updateDebugFrameBuffer) 
		updateDebugFrameBuffer(&ad->updateDebugFrameBuffer);

	debugMain(ds, appMemory, ad, reload, isRunning, init, __COUNTER__, mouseInClientArea(windowHandle));
}

#pragma optimize( "", on ) 