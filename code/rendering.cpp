
// @Misc.

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
void dxLineAA(int mode = 2) {
	theGState->rasterizerState.MultisampleEnable = mode == 2 ? true : false;
	theGState->rasterizerState.AntialiasedLineEnable = mode == 0 ? false : true;
	dxSetRasterizer();
}

void dxScissor(Rect r) {
	D3D11_RECT dRect = {r.left, -r.top, r.right, -r.bottom};
	theGState->d3ddc->RSSetScissorRects(1, &dRect);
}

void dxViewPort(Vec2i res) {
	D3D11_VIEWPORT viewPort = {0, 0, res.w, res.h, 0.0f, 1.0f};
	theGState->d3ddc->RSSetViewports(1, &viewPort);
}

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

// @Blending.

ID3D11BlendState* dxCreateBlendState(D3D11_BLEND src, D3D11_BLEND dst, D3D11_BLEND_OP op, D3D11_BLEND srcA, D3D11_BLEND dstA, D3D11_BLEND_OP opA, bool alphaCoverage = false, bool asdf = true) {
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = alphaCoverage;
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

// @Shader.

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
	fb->name = getPString(name);
	fb->format = format;

	fb->hasRenderTargetView = renderTarget;
	fb->hasShaderResourceView = shaderResource;
	fb->hasDepthStencilView = depthStencil;
}

void dxReleaseFrameBuffer(FrameBuffer* fb);
void dxSetFrameBuffer(char* name, Vec2i dim, int msaaSamples) {
	GraphicsState* gs = theGState;
	FrameBuffer* fb = dxGetFrameBuffer(name);

	static int index = 0;

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

		} else if(fb->makeDepthView) {
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
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

		} else if(fb->makeDepthView) {
			D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
			descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
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
	} 
	if(fb->hasDepthStencilView) {
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
// @Samplers.
// 

ID3D11SamplerState* createSampler(uint filter, uint address, int anisotropy, uint compareFunc) {
	D3D11_SAMPLER_DESC samplerDesc;
	
	samplerDesc.Filter = (D3D11_FILTER)filter;
	samplerDesc.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)address;
	samplerDesc.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)address;
	samplerDesc.AddressW = (D3D11_TEXTURE_ADDRESS_MODE)address;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = anisotropy;
	samplerDesc.ComparisonFunc = (D3D11_COMPARISON_FUNC)compareFunc;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	ID3D11SamplerState* sampler;
	theGState->d3dDevice->CreateSamplerState(&samplerDesc, &sampler);

	return sampler;
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

	D3D11_TEXTURE2D_DESC desc;
	texResource->GetDesc(&desc);
	tex->dim = vec2i(desc.Width, desc.Height);

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

void dxSetTexture(ID3D11ShaderResourceView* view, int index) {
	theGState->d3ddc->PSSetShaderResources(index, 1, &view);
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
	dxGetShaderVars(Main)->material.hasDispMap = m->disp.file ? true : false;

	// Check if material uses texture.
	{
		theGState->d3ddc->PSSetShaderResources(0, 1, &m->map_Ka.view);
		theGState->d3ddc->PSSetShaderResources(1, 1, &m->map_Kd.view);
		if(dxGetShaderVars(Main)->material.hasBumpMap) 
			theGState->d3ddc->PSSetShaderResources(2, 1, &m->bump.view);
		theGState->d3ddc->PSSetShaderResources(3, 1, &m->map_Ks.view);
		if(dxGetShaderVars(Main)->material.hasDispMap) {
			theGState->d3ddc->DSSetShaderResources(4, 1, &m->disp.view);
			dxGetShaderVars(Main)->material.heightScale = m->heightScale;
		}
	}
}

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