
struct DXTimer {
	ID3D11DeviceContext* d3ddc;

	ID3D11Query* queryStart[5];
	ID3D11Query* queryStop[5];
	ID3D11Query* queryDisjoint[5];

	int queryCount;
	int queryIndex;

	bool initialized;

	f64 dt;

	void init(ID3D11Device* d3dDevice, ID3D11DeviceContext* d3ddc) {
		this->d3ddc = d3ddc;

		D3D11_QUERY_DESC queryDesc;

		for(int i = 0; i < arrayCount(queryStart); i++) {
			queryDesc = {D3D11_QUERY_TIMESTAMP};
			d3dDevice->CreateQuery(&queryDesc, &queryStop[i]);
		}

		for(int i = 0; i < arrayCount(queryStart); i++) {
			queryDesc = {D3D11_QUERY_TIMESTAMP};
			d3dDevice->CreateQuery(&queryDesc, &queryStart[i]);
		}

		for(int i = 0; i < arrayCount(queryStart); i++) {
			queryDesc = {D3D11_QUERY_TIMESTAMP_DISJOINT};
			d3dDevice->CreateQuery(&queryDesc, &queryDisjoint[i]);
		}

		queryCount = arrayCount(queryStart);
		queryIndex = 0;
	}

	void start() {
		d3ddc->Begin(queryDisjoint[queryIndex]);
		d3ddc->End(queryStart[queryIndex]);
	}

	bool stop() {
		d3ddc->End(queryStop[queryIndex]);
		d3ddc->End(queryDisjoint[queryIndex]);

		// Wait for one full roundtrip.
		if(!initialized && queryIndex == queryCount-1) {
			initialized = true;
		}

		bool result = false;

		int nextIndex = (queryIndex+1)%queryCount;
		if(initialized) {

			D3D11_QUERY_DATA_TIMESTAMP_DISJOINT queryDataDisjoint;
			UINT64 queryDataStart;
			UINT64 queryDataEnd;

			while(S_OK != d3ddc->GetData(queryDisjoint[nextIndex], &queryDataDisjoint, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0)) {
				sleep(1);
			}

			bool result1 = (S_OK != d3ddc->GetData(queryStart[nextIndex], &queryDataStart, sizeof(UINT64), 0));
			bool result2 = (S_OK != d3ddc->GetData(queryStop[nextIndex], &queryDataEnd, sizeof(UINT64), 0));

			// 	if(queryDataDisjoint.Disjoint) {
			// 		waiting = false;
			// 		return false;
			// 	}

			dt = (queryDataEnd - queryDataStart) / (f64)queryDataDisjoint.Frequency;
			dt *= 1000; // To ms.

			result = true;
		}

		queryIndex = nextIndex;

		return result;
	}
};

struct GraphicsState {

	IDXGISwapChain* swapChain;
	ID3D11Device* d3dDevice;
	ID3D11DeviceContext* d3ddc;

	//

	Texture* textures;
	int textureCount;
	int textureCountMax;
	Texture* textureWhite;
	Texture* textureCircle;

	ID3D11Texture2D* cubeMapTexture;
	ID3D11ShaderResourceView* cubeMapView;
	int cubeMapSize;

	int shadowMapSize;

	ID3D11RenderTargetView* backBufferView;
	FrameBuffer* frameBuffers;
	int frameBufferCount;
	int frameBufferCountMax;

	Font fonts[10][20];
	int fontsCount;
	char* fontFolders[10];
	int fontFolderCount;

	Mesh* meshes;
	int meshCount;

	Material* materials;
	int materialCount;

	//

	Shader* shaders;
	int shaderCount;

	ID3D11InputLayout* primitiveInputLayout;
	ID3D11InputLayout* mainInputLayout;

	ID3D11Buffer* primitiveVertexBuffer;
	int primitiveVertexBufferMaxCount;

	ID3D11Buffer* shaderSamplesBuffer;

	//

	D3D11_RASTERIZER_DESC rasterizerState;
	D3D11_DEPTH_STENCIL_DESC depthStencilState;
	ID3D11SamplerState* sampler;
	ID3D11SamplerState* samplerCmp;

	ID3D11BlendState* blendStates[10];
	int blendStateCount;

	//

	DXTimer timer;

	//

	int currentShader;

	PrimitiveVertex* pVertexArray;
	int pVertexCount;

	Vec2i screenRes;
	Rect screenRect;

	float zLevel;
};

extern GraphicsState* theGState;

// @Shader.

#define dxGetShaderVars(name) ((name##ShaderVars*)theGState->shaders[Shader_##name].varsData)

Shader* dxGetShader(int shaderId) {
	return theGState->shaders + shaderId;
}

void dxLoadShader(int type, char* shaderCode, char* name, ID3D11VertexShader** vertexShader, ID3D11PixelShader** pixelShader, ID3DBlob** shaderBlob = 0) {
	enum {
		SHADER_TYPE_VERTEX = 0,
		SHADER_TYPE_PIXEL,
	};

	bool vShader = type == SHADER_TYPE_VERTEX;

	if(vShader) {
		if(*vertexShader != 0) (*vertexShader)->Release();
	} else {
		if(*pixelShader != 0) (*pixelShader)->Release();
	}

	#if SHADER_DEBUG
	uint flags1 = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	#else
	uint flags1 = 0;
	#endif

	ID3DBlob* blobError;

	char* version[] = {"vs_4_0", "ps_4_0"};

	ID3DBlob* blob;
	D3DCompile(shaderCode, strlen(shaderCode), NULL, NULL, NULL, name, version[type], flags1, 0, &blob, &blobError);
	{
		if (blobError != nullptr)
			printf((char*)blobError->GetBufferPointer());
		if (blobError) blobError->Release();
	}

	if(vShader) 
		theGState->d3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), 0, vertexShader);
	else 
		theGState->d3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), 0, pixelShader);

	if(shaderBlob) *shaderBlob = blob;
}

void dxLoadShaders() {
	for(int i = 0; i < theGState->shaderCount; i++) {
		Shader* shader = theGState->shaders + i;
		ShaderInfo* info = shaderInfos + shader->id;

		dxLoadShader(0, info->code, "vertexShader", &shader->vertexShader, 0, &shader->vertexBlob);
		dxLoadShader(1, info->code, "pixelShader", 0, &shader->pixelShader, 0);
	}
}

void dxSetShader(int shaderId) {
	GraphicsState* gs = theGState;
	Shader* shader = gs->shaders + shaderId;

	if(shader->id == gs->currentShader) return;

	gs->currentShader = shader->id;

	{
		gs->d3ddc->VSSetShader(shader->vertexShader, 0, 0);
		gs->d3ddc->PSSetShader(shader->pixelShader, 0, 0);
		
		gs->d3ddc->VSSetConstantBuffers(0, 1, &shader->constantBuffer);
		gs->d3ddc->PSSetConstantBuffers(0, 1, &shader->constantBuffer);

		if(shader->inputLayout) {
			gs->d3ddc->IASetInputLayout(shader->inputLayout);
		}
	}

	if(shader->id == Shader_Primitive) {
		UINT stride = sizeof(PrimitiveVertex);
		UINT offset = 0;
		gs->d3ddc->IASetVertexBuffers( 0, 1, &gs->primitiveVertexBuffer, &stride, &offset );
	}
}

void dxPushShaderConstants(int shaderId) {
	GraphicsState* gs = theGState;
	Shader* shader = gs->shaders + shaderId;

	D3D11_MAPPED_SUBRESOURCE sub;
	gs->d3ddc->Map(shader->constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub);

	memcpy(sub.pData, shader->varsData, shader->varsSize);

	gs->d3ddc->Unmap(shader->constantBuffer, 0);
}

inline void dxSetShaderAndPushConstants(int shaderId) {
	dxSetShader(shaderId);
	dxPushShaderConstants(shaderId);
}

// int dxSetVertexBuffer(ID3D11Buffer* buffer) {
// 	D3D11_BUFFER_DESC desc;
// 	buffer->GetDesc(&desc);
// 	int vertexCount = desc.ByteWidth / sizeof(MeshVertex);

// 	uint stride = sizeof(MeshVertex);
// 	uint offset = 0;
// 	theGState->d3ddc->IASetVertexBuffers( 0, 1, &buffer, &stride, &offset );

// 	return vertexCount;
// }

//

PrimitiveVertex* dxBeginPrimitive(uint topology = -1) {
	GraphicsState* gs = theGState;

	if(topology != -1) 
		gs->d3ddc->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)topology);

	D3D11_MAPPED_SUBRESOURCE sub;
	gs->d3ddc->Map(gs->primitiveVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub);

	gs->pVertexArray = (PrimitiveVertex*)sub.pData;
	gs->pVertexCount = 0;

	return gs->pVertexArray;
}

PrimitiveVertex* dxBeginPrimitiveColored(Vec4 color, uint topology = -1) {
	GraphicsState* gs = theGState;

	gs->d3ddc->PSSetShaderResources(0, 1, &gs->textureWhite->view);
	gs->d3ddc->PSSetSamplers(0, 1, &gs->sampler);

	dxGetShaderVars(Primitive)->color = color;
	dxPushShaderConstants(Shader_Primitive);

	return dxBeginPrimitive(topology);
}
PrimitiveVertex* dxBeginPrimitiveColored(uint topology) {
	return dxBeginPrimitiveColored(vec4(1), topology);
}

void dxEndPrimitive(int vertexCount = theGState->pVertexCount) {
	GraphicsState* gs = theGState;

	gs->d3ddc->Unmap(gs->primitiveVertexBuffer, 0);
	gs->d3ddc->Draw(vertexCount, 0);
}

//

void dxSetRasterizer() {
	GraphicsState* gs = theGState;

	ID3D11RasterizerState* pRS;
	gs->d3dDevice->CreateRasterizerState(&gs->rasterizerState, &pRS);

	gs->d3ddc->RSSetState(pRS);

	pRS->Release();
}

void dxCullState(bool value) {
	D3D11_CULL_MODE mode = value ? D3D11_CULL_BACK : D3D11_CULL_NONE;
	if(theGState->rasterizerState.CullMode != mode) {
		theGState->rasterizerState.CullMode = mode;
		dxSetRasterizer();
	}
}

void dxFillWireFrame(bool value) {
	D3D11_FILL_MODE mode = value ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
   if(theGState->rasterizerState.FillMode != mode) {
		theGState->rasterizerState.FillMode = mode;
		dxSetRasterizer();
   }
}

void dxFrontCCW(bool value) {
   if(theGState->rasterizerState.FrontCounterClockwise != (BOOL)value) {
		theGState->rasterizerState.FrontCounterClockwise = (BOOL)value;
		dxSetRasterizer();
   }
}

void dxScissorState(bool value) {
   if(theGState->rasterizerState.ScissorEnable != (BOOL)value) {
		theGState->rasterizerState.ScissorEnable = (BOOL)value;
		dxSetRasterizer();
   }
}

void dxScissor(Rect r) {
	D3D11_RECT dRect = {r.left, -r.top, r.right, -r.bottom};
	theGState->d3ddc->RSSetScissorRects(1, &dRect);
}

//

void dxViewPort(Vec2i res) {
	D3D11_VIEWPORT viewPort = {0, 0, res.w, res.h, 0.0f, 1.0f};
	theGState->d3ddc->RSSetViewports(1, &viewPort);
}

//

void dxSetDepthStencil() {
	GraphicsState* gs = theGState;

	ID3D11DepthStencilState* pDSState;
	gs->d3dDevice->CreateDepthStencilState(&gs->depthStencilState, &pDSState);

	gs->d3ddc->OMSetDepthStencilState(pDSState, 1);

	pDSState->Release();
}

void dxDepthTest(bool value) {
	if(theGState->depthStencilState.DepthEnable != (BOOL)value) {
		theGState->depthStencilState.DepthEnable = (BOOL)value;
		dxSetDepthStencil();
	}
}

//

ID3D11BlendState* dxCreateBlendState(D3D11_BLEND src, D3D11_BLEND dst, D3D11_BLEND_OP op, D3D11_BLEND srcA, D3D11_BLEND dstA, D3D11_BLEND_OP opA, bool alphaCoverage = false, bool asdf = true) {
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = alphaCoverage;
	// blendDesc.AlphaToCoverageEnable = true;
	blendDesc.IndependentBlendEnable = false;
	blendDesc.RenderTarget[0].BlendEnable = asdf;
	blendDesc.RenderTarget[0].SrcBlend = src;
	blendDesc.RenderTarget[0].DestBlend = dst;
	blendDesc.RenderTarget[0].BlendOp = op;
	blendDesc.RenderTarget[0].SrcBlendAlpha = srcA;
	blendDesc.RenderTarget[0].DestBlendAlpha = dstA;
	blendDesc.RenderTarget[0].BlendOpAlpha = opA;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	ID3D11BlendState* blendState;
	theGState->d3dDevice->CreateBlendState(&blendDesc, &blendState);

	return blendState;
}

ID3D11BlendState* dxCreateBlendState(D3D11_BLEND src, D3D11_BLEND dst, D3D11_BLEND_OP op, bool alphaCoverage = false, bool asdf = true) {
	return dxCreateBlendState(src, dst, op, src, dst, op, alphaCoverage, asdf);
}

void dxSetBlendState(int blendState) {
	theGState->d3ddc->OMSetBlendState(theGState->blendStates[blendState], 0, 0xffffffff);
}


//
// @Framebuffers.
//

FrameBuffer* dxGetFrameBuffer(char* name) {
	GraphicsState* gs = theGState;

	for(int i = 0; i < gs->frameBufferCount; i++) {
		if(strCompare(gs->frameBuffers[i].name, name)) {
			return gs->frameBuffers + i;
		}
	}

	return 0;
}

void dxAddFrameBuffer(char* name, DXGI_FORMAT format, bool renderTarget, bool shaderResource, bool depthStencil = false) {
	FrameBuffer* fb = theGState->frameBuffers + theGState->frameBufferCount++;

	assert(theGState->frameBufferCount < theGState->frameBufferCountMax);

	*fb = {};
	fb->name = getPStringCpy(name);
	fb->format = format;

	fb->hasRenderTargetView = renderTarget;
	fb->hasShaderResourceView = shaderResource;
	fb->hasDepthStencilView = depthStencil;
}

void dxReleaseFrameBuffer(FrameBuffer* fb);
void dxSetFrameBuffer(char* name, Vec2i dim, int msaaSamples) {
	GraphicsState* gs = theGState;

	FrameBuffer* fb = dxGetFrameBuffer(name);

	if(fb->texture != 0) {
		dxReleaseFrameBuffer(fb);
	}

	fb->dim = dim;

	bool hasMsaa = msaaSamples > 1;

	uint bindFlags = 0;
	if(fb->hasRenderTargetView)   bindFlags |= D3D11_BIND_RENDER_TARGET;
	if(fb->hasShaderResourceView) bindFlags |= D3D11_BIND_SHADER_RESOURCE;
	if(fb->hasDepthStencilView)   bindFlags |= D3D11_BIND_DEPTH_STENCIL;

	D3D11_TEXTURE2D_DESC descTex;
	descTex.Width  = dim.w;
	descTex.Height = dim.h;
	descTex.MipLevels = 1;
	descTex.ArraySize = 1;
	descTex.Format = fb->format;
	descTex.SampleDesc.Count = msaaSamples;
	descTex.SampleDesc.Quality = hasMsaa ? 1 : 0;
	descTex.Usage = D3D11_USAGE_DEFAULT;
	descTex.BindFlags = bindFlags;
	descTex.CPUAccessFlags = 0;
	descTex.MiscFlags = 0;

	gs->d3dDevice->CreateTexture2D(&descTex, NULL, &fb->texture);

	if(fb->hasRenderTargetView) {
		gs->d3dDevice->CreateRenderTargetView(fb->texture, NULL, &fb->renderTargetView);
	}
	 
	if(fb->hasShaderResourceView) {
		if(fb->isShadow) {
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
			srvDesc.ViewDimension = hasMsaa ? D3D11_SRV_DIMENSION_TEXTURE2DMS : 
			                                  D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = descTex.MipLevels;
			srvDesc.Texture2D.MostDetailedMip = 0;

			gs->d3dDevice->CreateShaderResourceView(fb->texture, &srvDesc, &fb->shaderResourceView);

		} else {
			gs->d3dDevice->CreateShaderResourceView(fb->texture, NULL, &fb->shaderResourceView);
		}
	}

	if(fb->hasDepthStencilView) {
		if(fb->isShadow) {
			D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
			descDSV.Format = DXGI_FORMAT_D32_FLOAT;
			descDSV.ViewDimension = hasMsaa ? D3D11_DSV_DIMENSION_TEXTURE2DMS : 
			                                  D3D11_DSV_DIMENSION_TEXTURE2D;
			descDSV.Texture2D.MipSlice = 0;

			gs->d3dDevice->CreateDepthStencilView(fb->texture, &descDSV, &fb->depthStencilView);

		} else {
			gs->d3dDevice->CreateDepthStencilView(fb->texture, NULL, &fb->depthStencilView);
		}
	}
}

void dxResolveFrameBuffer(char* nameSrc, char* nameDst) {
	GraphicsState* gs = theGState;
	FrameBuffer* fbSrc = dxGetFrameBuffer(nameSrc);
	FrameBuffer* fbDst = dxGetFrameBuffer(nameDst);

	gs->d3ddc->ResolveSubresource(fbDst->texture, 0, fbSrc->texture, 0, fbDst->format);
}

void dxCopyFrameBuffer(char* nameSrc, char* nameDst) {
	GraphicsState* gs = theGState;
	FrameBuffer* fbSrc = dxGetFrameBuffer(nameSrc);
	FrameBuffer* fbDst = dxGetFrameBuffer(nameDst);

	gs->d3ddc->CopyResource(fbDst->texture, fbSrc->texture);
}

void dxBindFrameBuffer(char* name, char* nameDepthStencil = 0) {
	GraphicsState* gs = theGState;

	ID3D11RenderTargetView* rtv = name ? dxGetFrameBuffer(name)->renderTargetView : 0;
	ID3D11DepthStencilView* dsv = nameDepthStencil ? dxGetFrameBuffer(nameDepthStencil)->depthStencilView : 0;

	int count = (rtv == 0 && dsv == 0) ? 0 : 1;

	gs->d3ddc->OMSetRenderTargets(count, &rtv, dsv);
}

void dxClearFrameBuffer(char* name, Vec4 color = vec4(0,1)) {
	GraphicsState* gs = theGState;
	FrameBuffer* fb = dxGetFrameBuffer(name);

	if(fb->hasRenderTargetView) {
		gs->d3ddc->ClearRenderTargetView(fb->renderTargetView, color.e);

	} else if(fb->hasDepthStencilView) {
		gs->d3ddc->ClearDepthStencilView(fb->depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
	}
}

void dxReleaseFrameBuffer(FrameBuffer* fb) {
	fb->texture->Release();
	if(fb->hasShaderResourceView) fb->shaderResourceView->Release();
	if(fb->hasRenderTargetView)   fb->renderTargetView->Release();
	if(fb->hasDepthStencilView)   fb->depthStencilView->Release();
}

//
// @Textures.
// 

Texture* dxGetTexture(char* name) {
	GraphicsState* gs = theGState;
	for(int i = 0; i < gs->textureCount; i++) {
		if(strCompare(gs->textures[i].name, name)) {
			return gs->textures + i;
		}
	}

	return 0;
}

void dxCreateTexture(Texture* tex, char* data) {
	// Expects you to fill out: dim, format.

	bool autoGen = true;

	if(tex->resource != 0) tex->resource->Release();
	if(tex->view != 0) tex->view->Release();

	DXGI_SAMPLE_DESC sampleDesc;
	sampleDesc.Count = 1;
	sampleDesc.Quality = 0;

	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = tex->dim.w;
	texDesc.Height = tex->dim.h;
	texDesc.MipLevels = autoGen ? 0 : 1;
	texDesc.ArraySize = 1;
	texDesc.Format = (DXGI_FORMAT)tex->format;
	texDesc.SampleDesc = sampleDesc;
	texDesc.Usage = autoGen ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = autoGen ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

	D3D11_SUBRESOURCE_DATA resourceData;
	resourceData.pSysMem = data;
	resourceData.SysMemPitch = tex->dim.w*sizeof(uint);
	resourceData.SysMemSlicePitch = tex->dim.w*tex->dim.h*sizeof(uint);

	ID3D11Texture2D* texture;
	if(autoGen) {
		theGState->d3dDevice->CreateTexture2D(&texDesc, 0, &texture);
	} else {
		theGState->d3dDevice->CreateTexture2D(&texDesc, &resourceData, &texture);
	}
	defer{texture->Release();};

	D3D11_TEX2D_SRV srv;
	srv.MostDetailedMip = 0;
	srv.MipLevels = -1;
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = (DXGI_FORMAT)tex->format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D = srv;

	ID3D11ShaderResourceView* view;
	theGState->d3dDevice->CreateShaderResourceView(texture, &viewDesc, &view);

	if(autoGen) {
		theGState->d3ddc->UpdateSubresource(texture, 0, nullptr, resourceData.pSysMem, resourceData.SysMemPitch, resourceData.SysMemSlicePitch);
		theGState->d3ddc->GenerateMips(view);
	}

	//

	tex->type = 0;
	tex->desc = texDesc;
	tex->resource = texture;
	tex->view = view;
}

void dxLoadAndCreateTexture(Texture* tex) {
	// Expects you to fill out: file, format.

	int x,y,n;
	unsigned char* stbData = stbi_load(tex->file, &x, &y, &n, 0);
	defer { stbi_image_free(stbData); };

	tex->dim = vec2i(x,y);

	dxCreateTexture(tex, (char*)stbData);
}

void dxLoadAndCreateTextureDDS(Texture* tex, bool srgb) {
	GraphicsState* gs = theGState;

	int size = fileSize(tex->file);
	uchar* buffer = mallocArray(uchar, size); defer{free(buffer);};
	readFileToBuffer((char*)buffer, tex->file);

	ID3D11Resource* resource;
	ID3D11ShaderResourceView* view;

	DirectX::CreateDDSTextureFromMemoryEx(gs->d3dDevice, gs->d3ddc, buffer, size, 0, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0, 0, srgb, &resource, &view);

	ID3D11Texture2D* texResource;
	resource->QueryInterface(__uuidof(ID3D11Texture2D), (void **) &texResource);

	tex->resource = texResource;
	tex->view = view;
}

void dxReleaseTexture(Texture* tex) {
	tex->resource->Release();
	tex->resource = 0;
	tex->view->Release();
	tex->view = 0;
}

// #ifdef STB_IMAGE_WRITE_IMPLEMENTATION
// void writeTextureToFile(Texture* tex, char* path) {
// 	int w = tex->dim.w;
// 	int h = tex->dim.h;

// 	int bufSize = w*h*4;
// 	char* buffer = mallocArray(char, bufSize); 
// 	defer {free(buffer);};

// 	int texId = tex->id;
// 	glGetTextureImage(texId, 0, GL_RGBA, GL_UNSIGNED_BYTE, bufSize, buffer);

// 	int result = stbi_write_png(path, w, h, 4, buffer, w*4);
// }
// #endif

//
// @Mesh
//

Mesh* dxGetMesh(char* name) {
	GraphicsState* gs = theGState;
	for(int i = 0; i < gs->meshCount; i++) {
		if(strCompare(gs->meshes[i].name, name)) {
			return gs->meshes + i;
		}
	}

	return 0;
}

void dxLoadMaterial(Material* m, ObjLoader::ObjectMaterial* objM, char* folder) {
	m->Ka = objM->Ka;
	m->Kd = objM->Kd;
	m->Ks = objM->Ks;
	m->Ns = objM->Ns;
	m->Ke = objM->Ke;
	m->Ni = objM->Ni;
	m->d  = objM->d;
	m->illum = objM->illum;

	{
		char* maps[] = { objM->map_Kd, objM->bump, objM->map_Ks };
		bool srgb[] = { true, false, false };
		for(int j = 0; j < 3; j++) {
			char* map = maps[j];

			if(map) {
				char* fName = fillString("%s%s", folder, map);

				Texture tex = {};
				tex.file = fName;

				if(strFind(fName, ".dds") == -1) {
					tex.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
					dxLoadAndCreateTexture(&tex);

				} else {
					dxLoadAndCreateTextureDDS(&tex, srgb[j]);
				}

				     if(j == 0) m->map_Kd = tex;
				else if(j == 1) m->bump   = tex;
				else if(j == 2) m->map_Ks = tex;

			} else {
				     if(j == 0) m->map_Kd = *theGState->textureWhite;
				else if(j == 2) m->map_Ks = *theGState->textureWhite;
			}
		}
	}
}

void dxLoadMesh(Mesh* m, ObjLoader* parser) {
	GraphicsState* gs = theGState;

	parser->parse(App_Mesh_Folder, m->name, m->swapWinding);

	int size = parser->vertexBuffer.count;
	int sizeInBytes = size * sizeof(MeshVertex);
	char* data = (char*)parser->vertexBuffer.data;
	ID3D11Buffer* buffer;
	{
		D3D11_BUFFER_DESC bd;
		bd.ByteWidth = sizeInBytes;
		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA subresourceData;
		subresourceData.pSysMem = data;
		subresourceData.SysMemPitch = 0;
		subresourceData.SysMemSlicePitch = 0;

		theGState->d3dDevice->CreateBuffer(&bd, &subresourceData, &buffer);
	}

	m->vertexBuffer = buffer;
	m->size = size;

	m->groupCount = parser->objectArray.count;
	m->groups = getPArray(Mesh::Group, m->groupCount);

	for(int i = 0; i < parser->objectArray.count; i++) {
		Mesh::Group* g = m->groups + i;
		g->offset = parser->objectArray[i].offset;
		g->size = parser->objectArray[i].size;
		g->smoothing = parser->objectArray[i].smoothing;
	
		int mId = parser->objectArray[i].material;

		if(mId == -1) {
			g->material = {"", "", vec3(0.1f), vec3(0.5f), vec3(0.5f), 90, vec3(0.0f), 0, 1.0f, 2};

		} else {
			Material* m = &g->material;
			ObjLoader::ObjectMaterial* objM = parser->materialArray + mId;

			dxLoadMaterial(m, objM, App_Mesh_Folder);
		}
	}
}

void dxSetMesh(Mesh* m) {
	GraphicsState* gs = theGState;

	UINT stride = sizeof(MeshVertex);
	UINT offset = 0;
	gs->d3ddc->IASetVertexBuffers( 0, 1, &m->vertexBuffer, &stride, &offset );
}

//
// @Materials.
//

Material* dxGetMaterial(char* name) {
	GraphicsState* gs = theGState;
	for(int i = 0; i < gs->materialCount; i++) {
		if(strCompare(gs->materials[i].name, name)) {
			return gs->materials + i;
		}
	}

	return 0;
}

// @2dDraw

PrimitiveVertex pVertex(Vec2 p, float z = theGState->zLevel) {
	return { p.x, p.y, z, 1,1,1,1, 0,0 };
}

PrimitiveVertex pVertex(Vec2 p, Vec2 uv, float z = theGState->zLevel) {
	return { p.x, p.y, z, 1,1,1,1, uv.x, uv.y };
}

PrimitiveVertex pVertex(Vec2 p, Vec4 c, float z = theGState->zLevel) {
	return { p.x, p.y, z, c };
}

PrimitiveVertex pVertex(Vec2 p, Vec4 c, Vec2 uv, float z = theGState->zLevel) {
	return { p.x, p.y, z, c, uv };
}

PrimitiveVertex pVertex(Vec3 p) {
	return { p, 1,1,1,1 };
}

PrimitiveVertex pVertex(Vec3 p, Vec4 c) {
	return { p, c };
}

void dxPushVertex(PrimitiveVertex v) {
	theGState->pVertexArray[theGState->pVertexCount++] = v;
}

void dxPushLine(Vec2 a, Vec2 b, Vec4 c) {
	theGState->pVertexArray[theGState->pVertexCount++] = pVertex(a, c);
	theGState->pVertexArray[theGState->pVertexCount++] = pVertex(b, c);
}

void dxPushTriangle(Vec2 a, Vec2 b, Vec2 c) {
	theGState->pVertexArray[theGState->pVertexCount++] = pVertex(a);
	theGState->pVertexArray[theGState->pVertexCount++] = pVertex(b);
	theGState->pVertexArray[theGState->pVertexCount++] = pVertex(c);
}

void dxPushRect(Rect r, Rect uv) {
	GraphicsState* gs = theGState;
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.bl(), uv.tl());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.tl(), uv.bl());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.br(), uv.tr());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.br(), uv.tr());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.tl(), uv.bl());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.tr(), uv.br());
}

void dxPushRect(Rect r, Vec4 c, Rect uv) {
	GraphicsState* gs = theGState;
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.bl(), c, uv.tl());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.tl(), c, uv.bl());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.br(), c, uv.tr());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.br(), c, uv.tr());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.tl(), c, uv.bl());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.tr(), c, uv.br());
}

void dxPushLine(Vec3 a, Vec3 b, Vec4 c) {
	theGState->pVertexArray[theGState->pVertexCount++] = pVertex(a, c);
	theGState->pVertexArray[theGState->pVertexCount++] = pVertex(b, c);
}

//

void dxDrawLine(Vec2 a, Vec2 b, Vec4 color) {
	PrimitiveVertex* v = dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	v[0] = pVertex(a);
	v[1] = pVertex(b);

	dxEndPrimitive(2);
}

void dxDrawLineH(Vec2 a, Vec2 b, Vec4 color, bool roundUp = false) {
	float off = roundUp ? 0.5f : -0.5f;
	dxDrawLine(round(a) + vec2(0, off), round(b) + vec2(0, off), color);
}

void dxDrawLineV(Vec2 a, Vec2 b, Vec4 color, bool roundUp = false) {
	float off = roundUp ? 0.5f : -0.5f;
	dxDrawLine(round(a) + vec2(off, 0), round(b) + vec2(off, 0), color);
}

void dxDrawRect(Rect r, Vec4 color, ID3D11ShaderResourceView* view = 0, Rect uv = rect(0,1,1,0)) {
	GraphicsState* gs = theGState;

	if(!view) view = gs->textureWhite->view;
	gs->d3ddc->PSSetShaderResources(0, 1, &view);
	gs->d3ddc->PSSetSamplers(0, 1, &gs->sampler);

	dxGetShaderVars(Primitive)->color = color;
	dxPushShaderConstants(Shader_Primitive);

	PrimitiveVertex* v = dxBeginPrimitive(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	v[0] = pVertex(r.bl(), uv.bl());
	v[1] = pVertex(r.tl(), uv.tl());
	v[2] = pVertex(r.br(), uv.br());
	v[3] = pVertex(r.tr(), uv.tr());

	dxEndPrimitive(4);
}

void dxDrawRectOutline(Rect r, Vec4 color) {	
	r = r.expand(-1);
	dxBeginPrimitiveColored(vec4(1), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	dxPushLine(r.bl() + vec2(0,0.5f),  r.tl() + vec2(0,0.5f),  color);
	dxPushLine(r.tl() + vec2(0.5f,0),  r.tr() + vec2(0.5f,0),  color);
	dxPushLine(r.tr() + vec2(0,-0.5f), r.br() + vec2(0,-0.5f), color);
	dxPushLine(r.br() + vec2(-0.5f,0), r.bl() + vec2(-0.5f,0), color);

	dxEndPrimitive();
}

void dxDrawRectOutlined(Rect r, Vec4 color, Vec4 colorOutline) {	
	dxDrawRect(r.expand(-2), color);
	dxDrawRectOutline(r, colorOutline);
}

void dxDrawRectGradientH(Rect r, Vec4 c0, Vec4 c1) {	
	PrimitiveVertex* v = dxBeginPrimitiveColored(vec4(1), D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	v[0] = pVertex(r.bl(), c0);
	v[1] = pVertex(r.tl(), c1);
	v[2] = pVertex(r.br(), c0);
	v[3] = pVertex(r.tr(), c1);

	dxEndPrimitive(4);
}

void dxDrawRectRounded(Rect r, Vec4 color, float size, bool outline = false) {
	if(!outline) {
		if(size == 0) return dxDrawRect(r, color);

	} else {
		if(size == 0) return dxDrawRectOutline(r, color);

		r = r.expand(-1);
	}

	float roundingMod = 1.0f/2.0f;
	int steps = roundInt(M_PI_2 * size * roundingMod);
	size = min(size, r.w()/2, r.h()/2);

	if(!outline) {
		dxDrawRect(r.expand(vec2(-size*2, 0)), color);
		dxDrawRect(r.expand(vec2(0, -size*2)), color);
	}

	//

	uint topology = outline ? D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP : 
	                          D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	PrimitiveVertex* v = dxBeginPrimitiveColored(color, topology);
	int vc = 0;

	Rect rc = r.expand(-size*2);
	Vec2 corners[] = { rc.tr(), rc.br(), rc.bl(), rc.tl() };
	for(int cornerIndex = 0; cornerIndex < 4; cornerIndex++) {
		Vec2 corner = corners[cornerIndex];
		float startAngle = M_PI_2*cornerIndex;

		int st = outline ? steps : steps-1;
		for(int i = 0; i < st; i++) {
			float a = startAngle + i*(M_PI_2/(steps-1));
			Vec2 d = vec2(sin(a), cos(a));
			v[vc++] = pVertex(corner + d*size);

			if(!outline) {
				a = startAngle + (i+1)*(M_PI_2/(steps-1));
				d = vec2(sin(a), cos(a));
				v[vc++] = pVertex(corner + d*size);

				v[vc++] = pVertex(corner);
			}
		}
	}
	if(outline) v[vc++] = pVertex(vec2(rc.right, rc.top + size));

	dxEndPrimitive(vc);
};
void dxDrawRectRoundedOutline(Rect r, Vec4 color, float size) {
	return dxDrawRectRounded(r, color, size, true);
}

void dxDrawRectRoundedOutlined(Rect r, Vec4 color, Vec4 colorOutline, float size) {
	dxDrawRectRounded(r.expand(-2), color, size);
	dxDrawRectRounded(r, colorOutline, size, true);
};

void dxDrawRectRoundedGradient(Rect r, Vec4 color, int size, Vec4 off) {	
	GraphicsState* gs = theGState;

	gs->depthStencilState.StencilEnable = true;
	gs->depthStencilState.StencilReadMask = 0xFF;
	gs->depthStencilState.StencilWriteMask = 0xFF;
	gs->depthStencilState.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	gs->depthStencilState.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	gs->depthStencilState.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	gs->depthStencilState.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dxSetDepthStencil();

	{
		dxDrawRectRounded(r, color, size);
	}

	gs->depthStencilState.StencilWriteMask = 0x0;
	gs->depthStencilState.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dxSetDepthStencil();

	{
		dxGetShaderVars(Primitive)->gammaGradient = true;
		dxDrawRectGradientH(r, color - off, color + off);
		dxGetShaderVars(Primitive)->gammaGradient = false;
	}

	gs->depthStencilState.StencilEnable = false;
	dxSetDepthStencil();
}

void dxDrawCircle(Vec2 pos, float d, Vec4 color) {
	return dxDrawRect(rectCenDim(pos, vec2(d)), color, theGState->textureCircle->view);
}

void dxDrawTriangle(Vec2 p, float r, Vec4 color, Vec2 dir) {
	PrimitiveVertex* v = dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	dir = norm(dir) * r;
	v[0] = pVertex( p + dir                       );
	v[1] = pVertex( p + rotate(dir, M_2PI*1/3.0f) );
	v[2] = pVertex( p + rotate(dir, M_2PI*2/3.0f) );

	dxEndPrimitive(3);
}

void dxDrawTriangle(Vec2 a, Vec2 b, Vec2 c, Vec4 color) {
	PrimitiveVertex* v = dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	v[0] = pVertex( a );
	v[1] = pVertex( b );
	v[2] = pVertex( c );

	dxEndPrimitive(3);
}

void dxDrawQuad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec4 color) {
	PrimitiveVertex* v = dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	v[0] = pVertex( a );
	v[1] = pVertex( b );
	v[2] = pVertex( d );
	v[3] = pVertex( c );

	dxEndPrimitive(4);
}

void dxDrawCross(Vec2 p, float size, float size2, Vec4 color) {

	PrimitiveVertex* v = dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	int vc = 0;

	for(int i = 0; i < 2; i++) {
		Vec2 p0 = p + (i == 0 ? vec2(-1,-1) : rotateRight(vec2(-1,-1))) * size/2;
		Vec2 p1 = p - (i == 0 ? vec2(-1,-1) : rotateRight(vec2(-1,-1))) * size/2;
		Vec2 d0 =     (i == 0 ? vec2( 0, 1) : rotateRight(vec2( 0, 1))) * size2/2;
		Vec2 d1 =     (i == 0 ? vec2( 1, 0) : rotateRight(vec2( 1, 0))) * size2/2;

		v[vc++] = pVertex( p0 );
		v[vc++] = pVertex( p0 + d0 );
		v[vc++] = pVertex( p0 + d1 );
		v[vc++] = pVertex( p1 - d1 );
		v[vc++] = pVertex( p1 - d0 );
		v[vc++] = pVertex( p1 );
	}

	dxEndPrimitive(vc);
}

// @3dDraw

void dxDrawLine(Vec3 a, Vec3 b, Vec4 color) {
	PrimitiveVertex* v = dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	v[0] = pVertex( a );
	v[1] = pVertex( b );

	dxEndPrimitive(2);
}

// @MainShader

void dxSetMaterial(Material* m) {
	dxGetShaderVars(Main)->material.Ka         = m->Ka;
	dxGetShaderVars(Main)->material.Kd         = m->Kd;
	dxGetShaderVars(Main)->material.Ks         = m->Ks;
	dxGetShaderVars(Main)->material.Ns         = m->Ns;
	dxGetShaderVars(Main)->material.Ke         = m->Ke;
	dxGetShaderVars(Main)->material.Ni         = m->Ni;
	dxGetShaderVars(Main)->material.d          = m->d;
	dxGetShaderVars(Main)->material.illum      = m->illum;
	dxGetShaderVars(Main)->material.hasBumpMap = m->bump.file ? true : false;

	// Check if material uses texture.
	{
		theGState->d3ddc->PSSetShaderResources(0, 1, &m->map_Kd.view);
		theGState->d3ddc->PSSetShaderResources(2, 1, &m->map_Ks.view);

		if(dxGetShaderVars(Main)->material.hasBumpMap) 
			theGState->d3ddc->PSSetShaderResources(1, 1, &m->bump.view);
	}
}

void dxDrawMesh(Mesh* m, Vec3 pos, Vec3 dim, Vec4 color, Quat rot = quat()) {
	GraphicsState* gs = theGState;

	gs->d3ddc->PSSetSamplers(0, 1, &gs->sampler);
	gs->d3ddc->PSSetSamplers(1, 1, &gs->samplerCmp);

	dxGetShaderVars(Main)->mvp.model = modelMatrix(pos, dim, rot);
	dxGetShaderVars(Main)->color = color;

	UINT stride = sizeof(MeshVertex);
	UINT offset = 0;
	gs->d3ddc->IASetVertexBuffers( 0, 1, &m->vertexBuffer, &stride, &offset );

	gs->d3ddc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for(int i = 0; i < m->groupCount; i++) {
		Mesh::Group* g = m->groups + i;

		dxGetShaderVars(Main)->material.smoothing  = g->smoothing;
		dxSetMaterial(&g->material);
		dxPushShaderConstants(Shader_Main);

		gs->d3ddc->Draw(g->size, g->offset);
	}
}

void dxDrawMeshShadow(Mesh* m, Vec3 pos, Vec3 dim, Vec4 color, Quat rot = quat()) {
	GraphicsState* gs = theGState;

	gs->d3ddc->PSSetSamplers(0, 1, &gs->sampler);

	dxGetShaderVars(Shadow)->model = modelMatrix(pos, dim, rot);
	dxPushShaderConstants(Shader_Shadow);

	UINT stride = sizeof(MeshVertex);
	UINT offset = 0;
	gs->d3ddc->IASetVertexBuffers( 0, 1, &m->vertexBuffer, &stride, &offset );

	gs->d3ddc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for(int i = 0; i < m->groupCount; i++) {
		Mesh::Group* g = m->groups + i;
		
		gs->d3ddc->PSSetShaderResources(0, 1, &g->material.map_Kd.view);

		gs->d3ddc->Draw(g->size, g->offset);
	}
}

void dxDrawObject(Object* obj, bool shadow = false) {
	GraphicsState* gs = theGState;

	Mesh* m = dxGetMesh(obj->name);

	gs->d3ddc->PSSetSamplers(0, 1, &gs->sampler);

	Mat4 model = modelMatrix(obj->pos, obj->dim, obj->rot);
	if(shadow) {
		dxGetShaderVars(Shadow)->model = model;

		dxPushShaderConstants(Shader_Shadow);

	} else {
		gs->d3ddc->PSSetSamplers(1, 1, &gs->samplerCmp);

		dxGetShaderVars(Main)->mvp.model = model;
		dxGetShaderVars(Main)->color = obj->color;
	}

	UINT stride = sizeof(MeshVertex);
	UINT offset = 0;
	gs->d3ddc->IASetVertexBuffers( 0, 1, &m->vertexBuffer, &stride, &offset );

	gs->d3ddc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for(int i = 0; i < m->groupCount; i++) {
		Mesh::Group* g = m->groups + i;

		Material* mat = obj->material ? dxGetMaterial(obj->material) : &g->material;

		if(shadow) {
			gs->d3ddc->PSSetShaderResources(0, 1, &mat->map_Kd.view);

		} else {
			dxGetShaderVars(Main)->material.smoothing  = g->smoothing;

			dxSetMaterial(mat);

			dxPushShaderConstants(Shader_Main);
		}

		gs->d3ddc->Draw(g->size, g->offset);
	}
}

//
// @App
//

void addFrameBuffers() {
	// DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	DXGI_FORMAT dFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// DXGI_FORMAT fFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	DXGI_FORMAT fFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	
	dxAddFrameBuffer("Sky",         fFormat, true,  true,  false);
	dxAddFrameBuffer("3dMsaa",      format,  true,  false, false);
	dxAddFrameBuffer("3dNoMsaa",    format,  true, true,  false);
	dxAddFrameBuffer("2dMsaa",      format,  true,  false, false);
	dxAddFrameBuffer("2dNoMsaa",    format,  true,  true,  false);
	dxAddFrameBuffer("2dTemp",      format,  false, false, false);
	dxAddFrameBuffer("DebugMsaa",   format,  true,  false, false);
	dxAddFrameBuffer("DebugNoMsaa", format,  false, true,  false);
	dxAddFrameBuffer("ds3d",        dFormat, false, false, true );
	dxAddFrameBuffer("ds",          dFormat, false, false, true );

	dxAddFrameBuffer("Shadow",   DXGI_FORMAT_R32_TYPELESS, false, true, true );
	dxGetFrameBuffer("Shadow")->isShadow = true;
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
	dxSetFrameBuffer("ds",       res2d, m);

	dxSetFrameBuffer("Shadow",   vec2i(theGState->shadowMapSize), 1);
}

void clearFrameBuffers() {
	dxClearFrameBuffer("Sky",         vec4(0.0f, 1.0f));
	dxClearFrameBuffer("3dMsaa",      vec4(0.0f, 1.0f));
	dxClearFrameBuffer("2dMsaa",      vec4(0.0f));
	dxClearFrameBuffer("DebugNoMsaa", vec4(0.0f));
	dxClearFrameBuffer("ds3d");
	dxClearFrameBuffer("ds");
}
