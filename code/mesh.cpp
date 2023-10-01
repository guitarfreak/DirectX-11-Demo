
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
		char* maps[] = { objM->map_Ka, objM->map_Kd, objM->bump, objM->map_Ks, objM->disp };
		bool srgb[] = { false, true, false, false, false };
		for(int j = 0; j < arrayCount(maps); j++) {
			char* map = maps[j];

			if(map) {
				char* fName = fString("%s%s", folder, map);

				Texture tex = {};
				tex.file = fName;

				if(strFind(fName, ".dds") == -1) {
					tex.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
					dxLoadAndCreateTexture(&tex);

				} else {
					dxLoadAndCreateTextureDDS(&tex, srgb[j]);
				}

				     if(j == 0) m->map_Ka = tex;
				else if(j == 1) m->map_Kd = tex;
				else if(j == 2) m->bump   = tex;
				else if(j == 3) m->map_Ks = tex;
				else if(j == 4) m->disp   = tex;

			} else {
				     if(j == 0) m->map_Ka = *theGState->textureWhite;
				else if(j == 1) m->map_Kd = *theGState->textureWhite;
				else if(j == 3) m->map_Ks = *theGState->textureWhite;
			}
		}
	}
}

void dxLoadMesh(Mesh* m, ObjLoader* parser) {
	GraphicsState* gs = theGState;

	if(strFind(m->name, ".mesh") != -1) {
		parser->parseCustom(App_Mesh_Folder, m->name, m->swapWinding);
	} else {
		parser->parse(App_Mesh_Folder, m->name, m->swapWinding);
	}

	// if(strFind(m->name, "cube") != -1) 
	{
		m->vertices = {};
		m->vertices.copy(parser->vertexBuffer);
	}

	// Get bounding box.
	{
		Rect3 bb = { vec3(FLT_MAX), vec3(-FLT_MAX) };
		for(int i = 0; i < m->vertices.count; i++) {
			Vec3 p = m->vertices.data[i].pos;

			bb.min.x = min(bb.min.x, p.x);
			bb.min.y = min(bb.min.y, p.y);
			bb.min.z = min(bb.min.z, p.z);

			bb.max.x = max(bb.max.x, p.x);
			bb.max.y = max(bb.max.y, p.y);
			bb.max.z = max(bb.max.z, p.z);
		}
		
		m->aabb = bb;
	}

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
	m->groupCount = parser->objectArray.count;
	m->groups = getPArray(Mesh::Group, m->groupCount);

	for(int i = 0; i < parser->objectArray.count; i++) {
		Mesh::Group* g = m->groups + i;
		*g = {};
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

	if(parser->boneArray.count) {
		m->boneCount = parser->boneArray.count;
		m->bones = getPArray(Bone, m->boneCount);
		for(int i = 0; i < m->boneCount; i++) {
			m->bones[i].name = getPString(parser->boneArray[i].name);
			m->bones[i].depth = parser->boneArray[i].depth;
			m->bones[i].index = i;
		}

		m->basePose = getPArray(XForm, m->boneCount);
		for(int i = 0; i < m->boneCount; i++) {
			m->basePose[i].trans = parser->poseArray[i].trans;
			m->basePose[i].rot   = orientationToQuat(parser->poseArray[i].rot);
			m->basePose[i].scale = parser->poseArray[i].scale;
		}

		buildBoneTree(m->bones, m->boneCount, &m->boneTree);

		xFormsLocalToGlobal(m->basePose, &m->boneTree);

		m->animationCount = parser->animationCount;
		for(int animIndex = 0; animIndex < m->animationCount; animIndex++) {
			ObjLoader::Animation* pAnim = &parser->animations[animIndex];
			Animation* anim = &m->animations[animIndex];

			anim->name = getPString(pAnim->name);
			anim->startTime = pAnim->startTime;
			anim->endTime = pAnim->endTime;
			anim->speed = pAnim->speed;
			anim->fps = pAnim->fps;
			anim->playbackMode = pAnim->playbackMode;

			anim->boneCount = pAnim->boneCount;
			anim->bones = getPArray(Bone, anim->boneCount);
			for(int i = 0; i < anim->boneCount; i++) {
				anim->bones[i].index = i;
				anim->bones[i].name = getPString(pAnim->bones[i].name);
				anim->bones[i].depth = pAnim->bones[i].depth;
			}

			anim->frameCount = pAnim->frameCount;
			anim->frames = getPArray(XForm*, anim->frameCount);
			for(int frameIndex = 0; frameIndex < anim->frameCount; frameIndex++) {
				anim->frames[frameIndex] = getPArray(XForm, anim->boneCount);

				for(int i = 0; i < anim->boneCount; i++) {
					anim->frames[frameIndex][i].trans = pAnim->frames[frameIndex][i].trans;
					anim->frames[frameIndex][i].rot = orientationToQuat(pAnim->frames[frameIndex][i].rot);
					anim->frames[frameIndex][i].scale = pAnim->frames[frameIndex][i].scale;
				}
			}
		}

		// Get Bone bounding boxes.
		{
			m->boneBoundingBoxes = getPArray(Rect3, m->boneCount);
			for(int i = 0; i < m->boneCount; i++) {
				m->boneBoundingBoxes[i] = { vec3(FLT_MAX), vec3(-FLT_MAX) };
			}
			
			// @Fix.
			m->boneBoundingBoxes[0] = { vec3(0.0f), vec3(0.0f) };

			for(int i = 0; i < m->vertices.count; i++) {
				MeshVertex* v = m->vertices.data + i;

				for(int j = 0; j < 4; j++) {
					float weight = v->blendWeights.e[j];
					if(weight == 0.0f) break;

					int index = v->blendIndices[j];

					Rect3* bb = m->boneBoundingBoxes + index;
					Vec3 p = v->pos;

					bb->min.x = min(bb->min.x, p.x);
					bb->min.y = min(bb->min.y, p.y);
					bb->min.z = min(bb->min.z, p.z);
					bb->max.x = max(bb->max.x, p.x);
					bb->max.y = max(bb->max.y, p.y);
					bb->max.z = max(bb->max.z, p.z);
				}
			}
		}
	}
}

void dxSetMesh(Mesh* m) {
	GraphicsState* gs = theGState;

	UINT stride = sizeof(MeshVertex);
	UINT offset = 0;
	gs->d3ddc->IASetVertexBuffers( 0, 1, &m->vertexBuffer, &stride, &offset );
}

void dxDrawObject(XForm xForm, Vec4 color, char* meshName, char* materialName, bool shadow = false) {
	TIMER_BLOCK();
	
	GraphicsState* gs = theGState;

	Mesh* m = dxGetMesh(meshName);
	if(!m) return;

	gs->d3ddc->PSSetSamplers(0, 1, &gs->sampler);
	gs->d3ddc->DSSetSamplers(0, 1, &gs->sampler);

	Mat4 model = modelMatrix(xForm);
	if(shadow) {
		dxGetShaderVars(Shadow)->model = model;
		dxPushShaderConstants(Shader_Shadow);

	} else {
		gs->d3ddc->PSSetSamplers(1, 1, &gs->samplerCmp);

		dxGetShaderVars(Main)->mvp.model = model;
		dxGetShaderVars(Main)->color = color;
	}

	bool setAlpha = m->hasAlpha;
	bool setDoubleSided = m->doubleSided;

	{
		Material* mat = materialName ? dxGetMaterial(materialName) : 0;
		if(mat && mat->hasAlpha) {
			setAlpha = true;
			setDoubleSided = true;
		}
	}

	if(setAlpha) {
		if (shadow) {
			// @Note(2023): Doing Blend_State_BlendAlphaCoverage when drawing shadow doesn't work.
			// Not sure why.
			dxSetBlendState(Blend_State_Blend);
			dxGetShaderVars(Shadow)->sharpenAlpha = 1;
		} else {
			dxSetBlendState(Blend_State_BlendAlphaCoverage);
			dxGetShaderVars(Main)->sharpenAlpha = 1;
		}

	} else {
		dxSetBlendState(Blend_State_Blend);
		if(shadow) dxGetShaderVars(Shadow)->sharpenAlpha = 0;
		else       dxGetShaderVars(Main)->sharpenAlpha = 0;
	}

	if(setDoubleSided) dxCullState(false);
	else               dxCullState(true);

	{
		int* shaderBoneCount = shadow ? &dxGetShaderVars(Shadow)->boneCount : 
		                                &dxGetShaderVars(Main)->boneCount;

		if(m->animPlayer.init) {
			Mat4* shaderBoneMatrices = shadow ? dxGetShaderVars(Shadow)->boneMatrices : 
			                                    dxGetShaderVars(Main)->boneMatrices;

			int boneCount = m->animPlayer.animation->boneCount;
			*shaderBoneCount = boneCount;
			if(boneCount) {
				for(int i = 0; i < boneCount; i++) {
					shaderBoneMatrices[i] = m->animPlayer.mats[i];
				}
			}

		} else {
			*shaderBoneCount = 0;
		}
	}

	UINT stride = sizeof(MeshVertex);
	UINT offset = 0;
	gs->d3ddc->IASetVertexBuffers( 0, 1, &m->vertexBuffer, &stride, &offset );

	// dxFillWireFrame(true);
	// defer { dxFillWireFrame(false); };

	for(int i = 0; i < m->groupCount; i++) {
		Mesh::Group* g = m->groups + i;

		Material* mat = materialName ? dxGetMaterial(materialName) : &g->material;
		if(!mat) mat = &g->material;

		// Doing this for every material is slow.
		bool hasDisp = mat->disp.file ? true : false;
		if(!hasDisp) {
			gs->d3ddc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			gs->d3ddc->HSSetShader(0, 0, 0);
			gs->d3ddc->DSSetShader(0, 0, 0);

		} else {
			gs->d3ddc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

			Shader* shader = shadow ? dxGetShader(Shader_Shadow) : dxGetShader(Shader_Main);

			gs->d3ddc->HSSetShader(shader->hullShader, 0, 0);   // If the shaders are zero they will be unbound intentionally.
			gs->d3ddc->DSSetShader(shader->domainShader, 0, 0); // If the shaders are zero they will be unbound intentionally.
			
			if(shader->hullShader) gs->d3ddc->HSSetConstantBuffers(0, 1, &shader->constantBuffer);
			if(shader->domainShader) gs->d3ddc->DSSetConstantBuffers(0, 1, &shader->constantBuffer);
		}

		if(shadow) {
			gs->d3ddc->PSSetShaderResources(0, 1, &mat->map_Kd.view);
			dxGetShaderVars(Shadow)->hasDispMap = mat->disp.file ? true : false;

			if(dxGetShaderVars(Shadow)->hasDispMap) {
				dxGetShaderVars(Shadow)->heightScale = mat->heightScale;
				gs->d3ddc->DSSetShaderResources(4, 1, &mat->disp.view);
			}

			dxPushShaderConstants(Shader_Shadow);

		} else {
			dxGetShaderVars(Main)->material.smoothing = g->smoothing;

			dxSetMaterial(mat);

			dxPushShaderConstants(Shader_Main);
		}

		gs->d3ddc->Draw(g->size, g->offset);
	}
}

void dxDrawLine(Vec3 a, Vec3 b, Vec4 color);
void drawSkeleton(XForm* bones, Mat4 model, BoneNode* node, int depth = 0) {

	for(int i = 0; i < node->childCount; i++) {
		if(depth > 0) {
			XForm form = bones[node->data->index];
			XForm formChild = bones[node->children[i].data->index];

			dxDrawLine((model * vec4(form.trans,1)).xyz, (model * vec4(formChild.trans,1)).xyz, vec4(0.26f,1.00f,0.64f,1));
		}

		drawSkeleton(bones, model, node->children + i, depth+1);
	}
}

void drawAnimatedMeshSoftware(Mesh* mesh, XForm xf, Vec4 color) {
	Mat4 model = modelMatrix(xf);
	AnimationPlayer* player = &mesh->animPlayer;

	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	defer { dxEndPrimitive(); };

	for(int i = 0; i < mesh->vertices.count; i++) {
		MeshVertex* v = mesh->vertices.data + i;
		Vec3 p = {};

		for(int j = 0; j < 4; j++) {
			float weight = v->blendWeights.e[j];
			if(weight == 0.0f) break;

			int index = v->blendIndices[j];
			p += (weight * (player->mats[index] * vec4(v->pos, 1))).xyz;
		}

		p = (model * vec4(p,1)).xyz;
		dxPushVertex(pVertex(p));

		if(i%3 == 2) dxFlushIfFull();
	}
}
