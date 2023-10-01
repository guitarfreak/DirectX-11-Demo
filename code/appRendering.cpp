
void addFrameBuffers(int* msaaSamples) {
	// DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	DXGI_FORMAT dFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DXGI_FORMAT fFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	
	DXGI_FORMAT typelessFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;

	dxAddFrameBuffer("Sky",         fFormat, true,  true,  false);
	dxAddFrameBuffer("3dMsaa",      fFormat,  true,  false, false);
	dxAddFrameBuffer("3dNoMsaa",    fFormat,  true, true,  false);
	dxAddFrameBuffer("2dMsaa",      fFormat,  true,  true, false); // TODO: Remove shader resource view.

	dxAddFrameBuffer("2dNoMsaa",    fFormat,  true,  true,  false);
	dxAddFrameBuffer("2dTemp",      fFormat,  false, false, false);

	dxAddFrameBuffer("DebugMsaa",   typelessFormat,  true,  false, false); // TODO: Remove 
	dxAddFrameBuffer("DebugNoMsaa", format,  false, true,  false);
	// dxAddFrameBuffer("ds3d",        dFormat, false, false, true );
	dxAddFrameBuffer("ds3d", DXGI_FORMAT_R24G8_TYPELESS, false, true, true );
	dxGetFrameBuffer("ds3d")->makeDepthView = true;

	dxAddFrameBuffer("ds3dTemp", DXGI_FORMAT_R24G8_TYPELESS, false, true, true );
	dxGetFrameBuffer("ds3dTemp")->makeDepthView = true;

	dxAddFrameBuffer("ds",          dFormat, false, false, true );

	dxAddFrameBuffer("Shadow",   DXGI_FORMAT_R32_TYPELESS, false, true, true );
	dxGetFrameBuffer("Shadow")->isShadow = true;

	for(int i = 0; i < 5; i++) {
		dxAddFrameBuffer(fString("Bloom_%i", i),       DXGI_FORMAT_R16G16B16A16_FLOAT,  true, true, false);
		dxAddFrameBuffer(fString("Bloom2_%i", i),       DXGI_FORMAT_R16G16B16A16_FLOAT,  true, true, false);
	}

	dxAddFrameBuffer("BloomHelper", DXGI_FORMAT_R16G16B16A16_FLOAT,  true, true, false);

	dxAddFrameBuffer("MenuBackground",       DXGI_FORMAT_R16G16B16A16_FLOAT,  true, true, false);

	// dxAddFrameBuffer("Test", DXGI_FORMAT_R8G8B8A8_TYPELESS, true, true, false);
	dxAddFrameBuffer("FontTemp", fFormat, true, true, false);

	//

	{
		DXGI_FORMAT formatsThatUseMSAA[] = { fFormat, DXGI_FORMAT_R24G8_TYPELESS, dFormat, typelessFormat };
		char* msaaBuffers[] = {"3dMsaa", "2dMsaa", "ds3d", "ds3dTemp", "ds", "FontTemp", "DebugMsaa"};
		for(int i = 0; i < arrayCount(msaaBuffers); i++) {
			FrameBuffer* fb = dxGetFrameBuffer(msaaBuffers[i]);

			uint numQualityLevels;
			HRESULT result = theGState->d3dDevice->CheckMultisampleQualityLevels(fb->format, *msaaSamples, &numQualityLevels);
			if(result != 0 || numQualityLevels < (*msaaSamples))
				*msaaSamples = max((int)numQualityLevels, 1);
		}
	}
}

void resizeFrameBuffers(Vec2i res3d, Vec2i res2d, int msaaSamples) {
	int m = msaaSamples;

	dxSetFrameBuffer("Sky",      res3d, 1);
	dxSetFrameBuffer("3dMsaa",   res3d, m);
	dxSetFrameBuffer("3dNoMsaa", res3d, 1);
	dxSetFrameBuffer("2dMsaa",   res2d, m);
	dxSetFrameBuffer("2dNoMsaa", res2d, 1);
	dxSetFrameBuffer("2dTemp",   res2d, 1);
	dxSetFrameBuffer("ds3d",     res3d, m);
	dxSetFrameBuffer("ds3dTemp", res3d, m);
	dxSetFrameBuffer("ds",       res2d, m);

	dxSetFrameBuffer("Shadow",   vec2i(theGState->shadowMapSize), 1);

	for(int i = 0; i < 5; i++) {
		dxSetFrameBuffer(fString("Bloom_%i", i),  res3d/(pow(2.0f,i)), 1);
		dxSetFrameBuffer(fString("Bloom2_%i", i), res3d/(pow(2.0f,i)), 1);
	}

	dxSetFrameBuffer("BloomHelper", res3d, 1);

	// dxSetFrameBuffer("Test", res2d, 1);
	dxSetFrameBuffer("FontTemp", res2d, m);
}

void clearFrameBuffers() {
	dxClearFrameBuffer("Sky",         vec4(0.0f, 1.0f));
	dxClearFrameBuffer("3dMsaa",      vec4(0.0f, 1.0f));
	dxClearFrameBuffer("2dMsaa",      vec4(0.0f));
	dxClearFrameBuffer("DebugNoMsaa", vec4(0.0f));
	dxClearFrameBuffer("ds3d");
	dxClearFrameBuffer("ds3dTemp");
	dxClearFrameBuffer("ds");

	for(int i = 0; i < 5; i++) {
		dxClearFrameBuffer(fString("Bloom_%i", i));
		dxClearFrameBuffer(fString("Bloom2_%i", i));
	}

	dxClearFrameBuffer("BloomHelper", vec4(0,1));

	// dxClearFrameBuffer("Test", vec4(0.0f, 1.0f));
	dxClearFrameBuffer("FontTemp", vec4(0.0f, 1.0f));
}

void updateFrameBuffersAndScreenRes(GraphicsSettings* gSettings, float aspectRatio, Vec2i currentRes, bool init) {
	TIMER_BLOCK();

	GraphicsState* gs = theGState;

	gSettings->aspectRatio = aspectRatio;
	
	gs->screenRes = currentRes;
	gs->screenRect = getScreenRect(currentRes);

	gSettings->cur3dBufferRes = currentRes;
	if(gSettings->resolutionScale < 1.0f) {
		gSettings->cur3dBufferRes.w = roundInt(gSettings->cur3dBufferRes.w * gSettings->resolutionScale);
		gSettings->cur3dBufferRes.h = roundInt(gSettings->cur3dBufferRes.h * gSettings->resolutionScale);
	}

	Vec2i res3d = gSettings->cur3dBufferRes;
	Vec2i res2d = currentRes;

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

	resizeFrameBuffers(res3d, res2d, gSettings->msaaSamples);
	if(init) {
		dxSetFrameBuffer("DebugMsaa",   res2d, gSettings->msaaSamples);
		dxSetFrameBuffer("DebugNoMsaa", res2d, 1);
	}
}

void updateDebugFrameBuffer(bool* update) {
	*update = false;

	Vec2i res2d = theGState->screenRes;
	int m = theGState->gSettings->msaaSamples;
	dxSetFrameBuffer("DebugMsaa",   res2d, m);
	dxSetFrameBuffer("DebugNoMsaa", res2d, 1);
}

void setupGraphics(GraphicsState* gs, GraphicsSettings gSettings, Camera activeCam, bool init, bool reload) {
	TIMER_BLOCK();

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
		rasterizerState.MultisampleEnable = false;
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

	dxScissor(getScreenRect(gs->screenRes));
	dxScissorState(true);

	gs->currentShader = -1;
	gs->zLevel = 0;

	{
		Vec4 color = vec4(0.0f,1.0f);
		gs->d3ddc->ClearRenderTargetView(gs->backBufferView, color.e);

		clearFrameBuffers();
	}

	dxViewPort(gs->screenRes);
	dxBindFrameBuffer("3dMsaa", "ds3d");

	gs->d3ddc->PSSetSamplers(1, 1, &gs->samplerCmp);

	{
		GraphicsSettings* gSettings = gs->gSettings;
			
		GraphicsMatrices* gMats = &gs->gMats;
		Rect sr = getScreenRect(gs->screenRes);
		gMats->view2d = identityMatrix();
		gMats->ortho  = orthoMatrixZ01(0, sr.bottom, sr.right, 0, 10, -10);

		gMats->view = viewMatrix(gs->activeCam.pos, -gs->activeCam.look, gs->activeCam.up, gs->activeCam.right);
		gMats->proj = projMatrixZ01(degreeToRadian(gSettings->fieldOfView), gSettings->aspectRatio, gSettings->nearPlane, gSettings->farPlane);

		gMats->viewInv = viewMatrixInv(gs->activeCam.pos, -gs->activeCam.look, gs->activeCam.up, gs->activeCam.right);
		gMats->projInv = projMatrixZ01Inv(gMats->proj);
	}
}

void setupShaders(GraphicsSettings gSettings, Camera activeCam, bool init, bool reload) {
	GraphicsState* gs = theGState;
	GraphicsMatrices* gMats = &gs->gMats;

	PrimitiveShaderVars* primitiveVars = dxGetShaderVars(Primitive);
	{
		*primitiveVars = {};
		primitiveVars->viewProj = gMats->ortho * gMats->view2d;
		primitiveVars->gammaGradient = false;
	}

	BloomShaderVars* bloomVars = dxGetShaderVars(Bloom);
	{
		*bloomVars = {};
		bloomVars->viewProj = gMats->ortho * gMats->view2d;
	}

	SkyShaderVars* skyVars = dxGetShaderVars(Sky);
	{
		skyVars->viewInv = gMats->viewInv;
		skyVars->projInv = gMats->projInv;
	}

	MainShaderVars* mainVars = dxGetShaderVars(Main);
	{
		*mainVars = {};
		mainVars->mvp.viewProj = gMats->proj * gMats->view;
		mainVars->camPos = activeCam.pos;
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
	}

	GradientShaderVars* gradientVars = dxGetShaderVars(Gradient);
	{
		*gradientVars = {};
		gradientVars->view = gMats->view2d;
		gradientVars->proj = gMats->ortho;
	}

	ShadowShaderVars* shadowVars = dxGetShaderVars(Shadow);
	{
		*shadowVars = {};
		shadowVars->camPos = activeCam.pos;
		shadowVars->farTessDistance = 20.0f;
		shadowVars->closeTessDistance = 1.0f;
		shadowVars->farTessFactor = 1;
		shadowVars->closeTessFactor = 10;
	}

	ParticleShaderVars* particleVars = dxGetShaderVars(Particle);
	{
		*particleVars = {};
		particleVars->viewProj = gMats->proj * gMats->view;
		particleVars->nearPlane = gSettings.nearPlane;
		particleVars->farPlane  = gSettings.farPlane;
		particleVars->msaaSamples  = gSettings.msaaSamples;

		particleVars->ambient = mainVars->ambient;
		particleVars->light = mainVars->light;
		particleVars->lightCount = mainVars->lightCount;
		particleVars->shadowMapSize = vec2(gs->shadowMapSize);
	}
}

void resolveFrameBuffersAndSwap(DebugState* ds, GraphicsState* gs, Vec2i currentRes, bool vsync, bool* vsyncTempTurnOff, int frameRateCap, bool windowMinimized, bool init) {

	if(!windowMinimized)
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
		dxDrawRect(rectTLDim(0,0,fb->dim.w,fb->dim.h), vec4(ds->guiAlpha,ds->guiAlpha), fb->shaderResourceView);		
		//

		dxSetBlendState(Blend_State_NoBlend);
		dxDepthTest(false);
		gs->d3ddc->OMSetRenderTargets(1, &gs->backBufferView, 0);
		dxDrawRect(theGState->screenRect, vec4(1), dxGetFrameBuffer("2dNoMsaa")->shaderResourceView);


		// dxSetBlendState(Blend_State_Blend);

		// dxBindFrameBuffer("Test");
		// // dxDrawRect(rectTLDim(0,0,100,100), vec4(0.5f,1));

		// {
		// 	dxDrawRect(rectTLDim(0,0,1000,1000), vec4(0,1));
		// 	dxDrawRect(rectTLDim(0,-60,1000,1000), vec4(1,1));

		// 	int fh = 12*1.3f;
		// 	TextSettings ts = {getFont("LiberationSans-Regular.ttf", fh), vec4(1.0f,1)};
		// 	TextSettings ts2 = {getFont("LiberationSans-Regular.ttf", fh), vec4(0.0f,1)};

		// 	drawText("this is some text!!!", vec2(20,-40), vec2i(-1,1), ts); 
		// 	drawText("this is some text!!!", vec2(20,-80), vec2i(-1,1), ts2); 
		// }

		// dxSetBlendState(Blend_State_NoBlend);

		// gs->d3ddc->OMSetRenderTargets(1, &gs->backBufferView, 0);
		// dxDrawRect(theGState->screenRect, vec4(1), dxGetFrameBuffer("Test")->shaderResourceView);
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
		TIMER_BLOCK_NAMED(SWAP_REGION_NAME);

		*vsyncTempTurnOff = false;

		{
			if(vsync && *vsyncTempTurnOff) vsync = false;
			*vsyncTempTurnOff = false;
		}

		// Sleep until monitor refresh.
		{
			if(windowMinimized) {
				frameRateCap = min(30, frameRateCap);
			}

			double frameTime = ds->swapTimer.update();
			int sleepTimeMS = 0;
			if(!init && (!vsync || windowMinimized)) {
				double fullFrameTime = (1.0f/frameRateCap);

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

		if(!windowMinimized)
			gs->swapChain->Present(vsync,0);
	}
	
}

//

void renderSky(bool* redrawSkyBox, SkySettings* skySettings, bool reload) {
	TIMER_BLOCK();

	GraphicsState* gs = theGState;

	{
		SkyShaderVars* skyVars = dxGetShaderVars(Sky);

		Vec2 angles = vec2(degreeToRadian(skySettings->sunAngles.x), 
		                   degreeToRadian(skySettings->sunAngles.y));
		clamp(&angles.y, (float)-M_PI_2 + 0.001f, (float)M_PI_2 - 0.001f);

		Quat sunRot = quat( angles.x, vec3(0,0,-1)) * 
		              quat(-angles.y, vec3(0,1, 0));
		skySettings->sunRot = sunRot;

		skyVars->sunDir = norm(sunRot * vec3(1,0,0));

		skyVars->spotBrightness          = skySettings->spotBrightness;
		skyVars->mieBrightness           = skySettings->mieBrightness;
		skyVars->mieDistribution         = skySettings->mieDistribution;
		skyVars->mieStrength             = skySettings->mieStrength;
		skyVars->mieCollectionPower      = skySettings->mieCollectionPower;
		skyVars->rayleighBrightness      = skySettings->rayleighBrightness;
		skyVars->rayleighStrength        = skySettings->rayleighStrength;
		skyVars->rayleighCollectionPower = skySettings->rayleighCollectionPower;
		skyVars->scatterStrength         = skySettings->scatterStrength;
		skyVars->surfaceHeight           = skySettings->surfaceHeight;
		skyVars->intensity               = skySettings->intensity;
		skyVars->stepCount               = skySettings->stepCount;
		skyVars->horizonOffset           = skySettings->horizonOffset;
		skyVars->sunOffset               = skySettings->sunOffset;
	}

	dxViewPort(gs->gSettings->cur3dBufferRes);

	// @RedrawCubeMap.
	// @Sky.
	bool cachedSky = true;
	if(cachedSky) {
		if(*redrawSkyBox || reload) {
			*redrawSkyBox = false;

			dxDepthTest(false); defer{dxDepthTest(true);};
			dxScissorState(false); defer{dxScissorState(true);};

			dxSetShader(Shader_Sky);

			dxViewPort(vec2i(gs->cubeMapSize)); defer{dxViewPort(gs->gSettings->cur3dBufferRes);};

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
		Shader* shader = gs->shaders + Shader_Cube;

		dxSetShader(Shader_Cube);

		Mesh* m = dxGetMesh("cube\\obj.obj");
		dxSetMesh(m);

		gs->d3ddc->PSSetShaderResources(0, 1, &gs->cubeMapView);
		gs->d3ddc->PSSetSamplers(0, 1, &gs->sampler);

		{
			Camera skyBoxCam = getCamData(vec3(0,0,0), theGState->activeCam.rot, vec3(0,0,0), vec3(0,1,0), vec3(0,0,1));

			Mat4 view = viewMatrix(skyBoxCam.pos, -skyBoxCam.look, skyBoxCam.up, skyBoxCam.right);
			Mat4 proj = projMatrix(degreeToRadian(gs->gSettings->fieldOfView), gs->gSettings->aspectRatio, 0.0f, 2);

			dxGetShaderVars(Cube)->viewProj = proj * view;
		}

		dxCullState(false); defer{dxCullState(true);};
		dxDepthTest(false); defer{dxDepthTest(true);};

		dxSetShaderAndPushConstants(Shader_Cube);
		gs->d3ddc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		gs->d3ddc->Draw(m->size, 0);

	} else {
		dxDepthTest(false); defer{dxDepthTest(true);};

		dxGetShaderVars(Primitive)->viewProj = orthoMatrixZ01(0, 0, 1, 1, 10, -10) * gs->gMats.view2d;
		dxPushShaderConstants(Shader_Primitive);

		dxDrawRect(rectTLDim(0,1,1,1), vec4(1), dxGetFrameBuffer("Sky")->shaderResourceView);	
	}
}

void renderBloom(GraphicsMatrices gMats) {
	TIMER_BLOCK();

	dxDepthTest(false);
	defer{ dxDepthTest(true); };

	Rect sr = theGState->screenRect;

	dxViewPort(theGState->screenRes);

	dxSetShader(Shader_Primitive);
	dxGetShaderVars(Primitive)->viewProj = gMats.ortho * gMats.view2d;
	dxPushShaderConstants(Shader_Primitive);

	dxResolveFrameBuffer("3dMsaa", "3dNoMsaa");
	dxBindFrameBuffer("2dMsaa", "ds");
	dxDrawRect(sr, vec4(1), dxGetFrameBuffer("3dNoMsaa")->shaderResourceView);

	int stageCount = 4;
	for(int i = 0; i < stageCount; i++) {
		char* cBuf = fString("Bloom_%i", i);
		char* oBuf = fString("Bloom2_%i", i);

		FrameBuffer* fb = dxGetFrameBuffer(cBuf);
		Vec2 dim = vec2(fb->dim);

		{
			dxBindFrameBuffer(cBuf, 0);

			dxSetShader(Shader_Bloom);
			dxGetShaderVars(Bloom)->mode = 0;
			dxPushShaderConstants(Shader_Bloom);

			dxSetBlendState(Blend_State_Blend);

			dxDrawRect(rectTLDim(vec2(0,0), dim), vec4(1,1,1,1), dxGetFrameBuffer("3dNoMsaa")->shaderResourceView);
		}

		if(i != 0) {
			dxSetShader(Shader_Bloom);

			int blurSteps = 1;
			for(int i = 0; i < blurSteps*2; i++) {
				dxBindFrameBuffer(oBuf, 0);

				dxGetShaderVars(Bloom)->mode = 1 + (i%2);
				dxPushShaderConstants(Shader_Bloom);

				Rect sr = theGState->screenRect;
				dxDrawRect(rectTLDim(vec2(0,0), dim), vec4(1,1,1,1), dxGetFrameBuffer(cBuf)->shaderResourceView, rect(0,1,1,0));

			 	swap(&cBuf, &oBuf);

			 	dxBindFrameBuffer(0, 0);
			 	theGState->d3ddc->PSSetShaderResources(0, 1, &dxGetFrameBuffer(cBuf)->shaderResourceView);
			}
		}

		dxSetShader(Shader_Primitive);

		dxSetBlendState(Blend_State_Add);
		defer{ dxSetBlendState(Blend_State_Blend); };

		dxBindFrameBuffer("BloomHelper");
		dxDrawRect(sr, vec4(pow(1.0f/stageCount, 1/2.2f),0.0f), dxGetFrameBuffer(cBuf)->shaderResourceView);
	}

	// Add last texture mip and blur everything again to 
	// smooth out pixelation from lower resolutions.
	{
		char* cBuf = "Bloom_0";
		char* oBuf = "Bloom2_0";

		dxClearFrameBuffer(cBuf);
		dxClearFrameBuffer(oBuf);

		dxCopyFrameBuffer("BloomHelper", cBuf);
		
		{
			dxSetShader(Shader_Bloom);
			Vec2 dim = vec2(dxGetFrameBuffer(cBuf)->dim);

			int blurSteps = 1;
			for(int i = 0; i < blurSteps*2; i++) {
				dxBindFrameBuffer(oBuf, 0);

				dxGetShaderVars(Bloom)->mode = 1 + (i%2);
				dxPushShaderConstants(Shader_Bloom);

				dxDrawRect(rectTLDim(vec2(0,0), dim), vec4(1,1,1,1), dxGetFrameBuffer(cBuf)->shaderResourceView, rect(0,1,1,0));

			 	swap(&cBuf, &oBuf);

			 	// Remove warnings.
				dxBindFrameBuffer(0, 0);
				theGState->d3ddc->PSSetShaderResources(0, 1, &dxGetFrameBuffer(cBuf)->shaderResourceView);
			}
		}

		dxSetShader(Shader_Primitive);

		// Copy again for menu background. (Menu uses 3dNoMsaa texture.)
		{
			dxBindFrameBuffer("3dNoMsaa");

			dxSetBlendState(Blend_State_Add);
			defer{ dxSetBlendState(Blend_State_Blend); };

			dxDrawRect(sr, vec4(1,0), dxGetFrameBuffer(cBuf)->shaderResourceView);
		}

		dxBindFrameBuffer("2dMsaa", "ds");

		dxSetBlendState(Blend_State_Add);
		defer{ dxSetBlendState(Blend_State_Blend); };

		dxDrawRect(sr, vec4(1,0), dxGetFrameBuffer(cBuf)->shaderResourceView);
	}
}

void drawParticles(EntityManager* em, Mat4 viewProjLight) {
	TIMER_BLOCK();

	GraphicsState* gs = theGState;

	FrameBuffer* fb = dxGetFrameBuffer("Shadow");
	gs->d3ddc->PSSetShaderResources(5, 1, &fb->shaderResourceView);
	dxGetShaderVars(Particle)->shadowViewProj = viewProjLight;

	{
		Camera* cam = &theGState->activeCam;

		// We collect particle emitters and groups that handle particle emitters.
		DArray<Entity*> pEntities;
		pEntities.init(getTMemory);

		for(int i = 0; i < em->byType[ET_ParticleEffect].count; i++) {
			Entity* e = em->byType[ET_ParticleEffect].data[i];
			if(e->groupId) continue;

			if(e->type == ET_ParticleEffect) pEntities.push(e);
		}

		for(int i = 0; i < em->byType[ET_Group].count; i++) {
			Entity* e = em->byType[ET_Group].data[i];
			if(e->handlesParticles) pEntities.push(e);
		}

		Pair<Entity*, float>* sortData = (Pair<Entity*, float>*)getTMemory(pEntities.count * sizeof(Pair<Entity*, float>));
		for(int i = 0; i < pEntities.count; i++) {
			Entity* e = pEntities.data[i];
			float dist = len(e->xf.trans - cam->pos);
			sortData[i] = {e, dist};
		}

		auto cmp = [](const void* a, const void* b) -> int { 
			return ((Pair<Entity*, float>*)a)->b > 
			       ((Pair<Entity*, float>*)b)->b ? -1 : 1;
		};
		qsort(sortData, pEntities.count, sizeof(Pair<Entity*, float>), cmp);

		//

		dxSetShader(Shader_Particle);
		dxSetBlendState(Blend_State_PreMultipliedAlpha);
		// // dxSetBlendState(Blend_State_Add);
		dxDepthTest(false); 
		dxBindFrameBuffer("3dMsaa", 0);
		dxSetTexture(dxGetFrameBuffer("ds3d")->shaderResourceView, 7);

		defer { 
			dxSetTexture(0, 7);
			dxSetBlendState(Blend_State_Blend); 
			dxDepthTest(true); 
			dxBindFrameBuffer("3dMsaa", "ds3d"); 
		};

		for(int i = 0; i < pEntities.count; i++) {
			Entity* e = sortData[i].a;

			if(e->type == ET_ParticleEffect) {
				if(e->culled) continue;

				e->particleEmitter.draw(e->xf, cam);

			} else if(e->type == ET_Group) {
				DArray<Entity*>* list = getGroupMembers(em, e->id);
				if(list) {
					for(int i = 0; i < list->count; i++) {
						Entity* e = list->data[i];
						if(e->type != ET_ParticleEffect) continue;
						if(e->culled) continue;

						e->particleEmitter.draw(e->xf, cam);
					}
				}
			}
		}
	}
}