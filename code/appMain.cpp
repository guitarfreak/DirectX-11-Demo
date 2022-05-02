
void gameLoad(AppData* ad) {
	EntityManager* em = &ad->entityManager;

	if(!em->currentMap) {
		loadMap(em, "showcase");
		// loadMap(em, "levelTest");

	} else {
		// loadMap(em, em->currentMap);
	}

	if(!ad->levelEdit) {
		for(auto& e : em->entities) {
			if(entityIsValid(&e) && e.type == ET_Player) {
				Entity playerCam = getDefaultEntity(ET_Camera, vec3(0,0,0));
				playerCam.name = getPString("PlayerCam");
				playerCam.mountParentId = e.id;

				Vec3 mountPos = vec3(0,0,ad->playerHeight);
				playerCam.xfMount = xForm(mountPos, vec3(1.0f));

				{
					float angle0, angle1;
					quatToEulerAngles(e.xf.rot, 0, &angle1, &angle0);
					playerCam.camRot.x = angle0;
					playerCam.camRot.y = -angle1;
				}

				addEntity(em, playerCam);
			}
		}
	}

	// Caught previous bug, leaving this for now.
	for(int i = 0; i < em->entities.count-1; i++) {
		for(int j = i+1; j < em->entities.count; j++) {
			Entity* e0 = em->entities.data + i;
			Entity* e1 = em->entities.data + j;
			assert(e0->id != e1->id);
		}
	}

	ad->newGameMode = GAME_MODE_MAIN;
}

void gameLoop(AppData* ad, DebugState* ds, WindowSettings* ws, bool showUI, bool init, bool reload) {
	TIMER_BLOCK();

	GraphicsState* gs = theGState;
	Input* input = &ad->input;
	EntityManager* em = &ad->entityManager;

	// Open menu.
	if(input->keysPressed[KEYCODE_ESCAPE]) {
		ad->newGameMode = GAME_MODE_MENU;
		ad->menu.gameRunning = true;
		ad->menu.activeId = 0;

		renderMenuBackground();
	} 

	if(input->keysPressed[KEYCODE_F1]) {
		ad->levelEdit = !ad->levelEdit;
		if(ad->levelEdit) {
			loadMap(em, em->currentMap);
			ad->freeCam = true;
		} else {
			ad->newGameMode = GAME_MODE_LOAD;
			ad->freeCam = false;
			return;
		}
	}

	if(input->keysPressed[KEYCODE_F4] && !ad->levelEdit) {
		ad->freeCam = !ad->freeCam;
		if(ad->freeCam) {
			Entity* cam = findEntity(&ad->entityManager, "Camera");
			Entity* pCam = findEntity(&ad->entityManager, "PlayerCam");
			cam->xf = pCam->xf;
			cam->camRot = pCam->camRot;
			cam->vel = vec3(0,0,0);
			cam->acc = vec3(0,0,0);
		}
	}

	// @Entities.
	{
		{
			for(int i = 0; i < em->entities.count; i++) {
				Entity* e = em->entities.data + i;
				if(!entityIsValid(e)) continue;

				if(e->type == ET_Player) ad->player = e;
				else if(e->type == ET_Camera && !strCompare(e->name, "PlayerCam")) ad->camera = e;
				else if(e->type == ET_Sky) ad->sky = e;

				if(entityCheck(e, ET_Object, "figure")) ad->figure = e; 
			}

			collectByType(em);
		}

		Entity* playerCam = findEntity(em, ET_Camera, "PlayerCam");

		if(ad->freeCam) {
			float dt = ds->dt;

			Entity* e = ad->camera;
		   if(ad->mouseEvents.fpsMode)
		   	entityMouseLook(e, input, ad->mouseSensitivity);

		   e->acc = vec3(0,0,0);
		   float speed = !input->keysDown[KEYCODE_T] ? 25 : 250;
		   if(input->keysDown[KEYCODE_BACKSPACE]) speed = 0.1f;
		   entityKeyboardAcceleration(e, input, e->camRot, speed, 2.0f, true);

		   e->vel = e->vel + e->acc*dt;
		   float friction = 0.01f;
		   e->vel = e->vel * pow(friction,dt);

		   if(e->acc == vec3(0,0,0) && 
		      between(e->vel.x + e->vel.y + e->vel.z, -0.0001f, 0.0001f)) {
		   	e->vel = vec3(0.0f);
		   }

		   if(e->vel != vec3(0,0,0)) {
		   	e->xf.trans = e->xf.trans - 0.5f*e->acc*dt*dt + e->vel*dt;
		   }
		}

		{
			if(!ad->freeCam && ad->mouseEvents.fpsMode) entityMouseLook(playerCam, input, ad->mouseSensitivity, true);

			updateEntities(em, input, ad->dt, ad->freeCam, ad->mouseEvents, ad->mouseSensitivity, &ds->entityUI, playerCam, ad->levelEdit);

			{
				DArray<WalkMesh> walkMeshes = dArray<WalkMesh>(getTMemory);
				for(auto& e : ad->entityManager.entities) {
					if(!strCompare(e.name, "walkmesh")) continue;
					walkMeshes.push({ e.xf, inverseSplit(e.xf), e.aabb });
				}

				DArray<Blocker> sceneBlockers = dArray<Blocker>(getTMemory);
				for(auto& e : ad->entityManager.entities) {
					if(e.blockerType) {
						Blocker b;
						switch(e.blockerType) {
							case BLOCKERTYPE_RECT:   b = {BLOCKERTYPE_RECT, e.xf * e.xfBlocker}; break;
							case BLOCKERTYPE_CIRCLE: b = {BLOCKERTYPE_CIRCLE, e.xf * e.xfBlocker}; break;
							case BLOCKERTYPE_LINE: {
								b = {BLOCKERTYPE_LINE};
								XForm xf = e.xf * e.xfBlocker;
								b.linePoints[0] = xf * vec3(-0.5f);
								b.linePoints[1] = xf * vec3( 0.5f);
							} break;
						}

						sceneBlockers.push(b);
					}
				}

				WalkManifold& wm = ad->manifold;

				if(!ad->freeCam)
				{
					Entity* player = ad->player;
					Vec2 dir = entityKeyboardControl(player, input, playerCam->camRot);
					float speed = 2.0f * ad->dt;
					if(input->keysDown[KEYCODE_SHIFT]) speed *= 3;
					if(input->keysDown[KEYCODE_CTRL]) speed *= 0.2f;
					if(input->keysDown[KEYCODE_T]) speed *= 6;

					Vec3 playerPos = player->xf.trans;
					Vec2 newPos = playerPos.xy + dir * speed;
					
					ad->playerMoveDir	= newPos - playerPos.xy;
					{
						int gridRadius = ad->manifoldGridRadius;
						float dist = (dir != vec2(0,0)) ? speed : 0;
						if(!ds->showUI || !ds->drawManifold) {
							gridRadius = ceil((dist / wm.settings.cellSize) + 2);
						}

						wm.setup(playerPos, gridRadius, walkMeshes.data, walkMeshes.count, sceneBlockers.data, sceneBlockers.count);
						defer { wm.free(); };

						wm.rasterize();

						playerPos = wm.calcWalkCellPos(playerPos.xy, playerPos.z);
						player->xf.trans.z = playerPos.z;

						if(wm.outside(playerPos.xy, playerPos.z)) {
							playerPos = wm.pushInside(playerPos.xy, playerPos.z);
						}
						Vec3 movedPos = wm.move(playerPos.xy, playerPos.z, newPos);
						movedPos.z = wm.calcWalkCellZ(movedPos.xy, movedPos.z, wm.getCell(movedPos.xy));

						playerPos = movedPos;

						player->xf.trans = playerPos;
					}
					
				} else {
					wm.setup(ad->camera->xf.trans, ad->manifoldGridRadius, walkMeshes.data, walkMeshes.count, sceneBlockers.data, sceneBlockers.count);
					defer { wm.free(); };

					wm.rasterize();
				}

				Vec3 pos = ad->freeCam ? ad->camera->xf.trans : ad->player->xf.trans;
				WalkCell* cell = wm.getCell(pos.xy);
				ad->currentWalkLayer = wm.getWalkLayer(cell, pos.z);
			}

			{
				TIMER_BLOCK_NAMED("ParticlesUpdate");

				DArray<Entity*> particlesToUpdate = dArray<Entity*>(getTMemory);
				for(auto it : em->byType[ET_ParticleEffect]) {
					if(it->particleEmitter.markedForUpdate) {
						it->particleEmitter.markedForUpdate = false;
						particlesToUpdate.push(it);
					}
				}

				struct ThreadData {
					Entity** effects;
					float dt;
				};

				auto threadFunc = [](void* data) {
					ThreadHeader* h = (ThreadHeader*)data;
					ThreadData* d = (ThreadData*)h->data;

					for(int i = 0; i < h->count; i++) {
						Entity* e = d->effects[i + h->index];
						e->particleEmitter.update(d->dt, e->xf, &theGState->activeCam);
					}
				};

				ThreadData td = {particlesToUpdate.data, ad->dt};
				splitThreadTask(particlesToUpdate.count, &td, threadFunc);
			}
		}
	}

	// @Logic.
	if(ad->levelEdit) {
		Mesh* mesh = dxGetMesh("figure\\figure.mesh");
		AnimationPlayer* player = &mesh->animPlayer;
		player->init = false;
	}

	if(!ad->levelEdit)
	{
		// @AnimationTest.
		{
			TIMER_BLOCK_NAMED("Animation");

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

		for(auto& e : ad->entityManager.entities) {
			if(!entityIsValid(&e)) continue;
			if(entityCheck(&e, ET_Object, "Rotating Object")) {
				e.xf.rot = e.xf.rot * quat(ad->dt*0.2f, vec3(0,0,1));
			} else if(entityCheck(&e, ET_Group, "rotator")) {
				e.xf.rot = e.xf.rot * quat(ad->dt, vec3(0,0,1));
			}
		}

		{
			Entity* platformGroup = findEntity(em, "movingPlatform");
			Entity* pathPoint0 = findEntity(em, "pathPoint0");
			Entity* pathPoint1 = findEntity(em, "pathPoint1");

			if(platformGroup) {
				DArray<Entity*>* members = getGroupMembers(em, platformGroup->id);
				Entity* platform;
				for(auto& it : *members) {
					if(strCompare(it->name, "walkmesh")) {
						platform = it;
					}
				}

				float speed = 1.0f;
				Vec3 move = vec3(0,0,0);

				static int mode = 1;
				static float wait2 = 2.0f;
				if(wait2 > 0) wait2 -= ad->dt;
				else {
					if(mode == 0) {
						if(platformGroup->xf.trans.y < pathPoint0->xf.trans.y) 
							move = vec3(0,ad->dt*speed,0);
						else {
							mode = 1;
							wait2 = 2;
						}
					} else {
						if(platformGroup->xf.trans.y > pathPoint1->xf.trans.y) 
							move = vec3(0,-ad->dt*speed,0);
						else {
							mode = 0;
							wait2 = 2;
						}
					}
				}

				platformGroup->xf.trans += move;

				Entity* p = ad->player;
				Mesh* mesh = dxGetMesh(platform->mesh);
				float dist = raycastEntity(p->xf.trans + vec3(0,0,0.1), vec3(0,0,-1), platform, mesh);
				if(dist != -1) {
					if(dist < 0.5) {
						p->xf.trans += move;
					}
				}
			}
		}

		{
			Entity* elevator = findEntity(em, "elevator");
			Entity* elevatorPoint0 = findEntity(em, "elevatorPoint0");
			Entity* elevatorPoint1 = findEntity(em, "elevatorPoint1");

			if(elevator) {
				DArray<Entity*>* members = getGroupMembers(em, elevator->id);
				Entity* platform;
				for(auto& it : *members) {
					if(strCompare(it->name, "walkmesh")) {
						platform = it;
					}
				}

				float speed = 2.0f;
				Vec3 move = vec3(0,0,0);

				static int mode = 0;
				static float wait = 2.0f;
				if(wait > 0) wait -= ad->dt;
				else {
					if(mode == 0) {
						if(elevator->xf.trans.z < elevatorPoint1->xf.trans.z) 
							move = vec3(0,0,ad->dt*speed);
						else {
							mode = 1;
							wait = 2;
						}
					} else {
						if(elevator->xf.trans.z > elevatorPoint0->xf.trans.z) 
							move = vec3(0,0,-ad->dt*speed);
						else {
							mode = 0;
							wait = 2;
						}
					}
				}

				elevator->xf.trans += move;

				Entity* p = ad->player;
				Mesh* mesh = dxGetMesh(platform->mesh);
				float dist = raycastEntity(p->xf.trans + vec3(0,0,0.1), vec3(0,0,-1), platform, mesh);
				if(dist != -1) {
					if(dist < 0.5f) {
						p->xf.trans += move;
					}
				}
			}
		}

		{
			Entity* platform = findEntity(em, "rotatingPlatform");

			if(platform) {
				Entity* walkMesh = getGroupMembers(em, platform->id)->first();

				float speed = 1.0f;
				float rotAmount = -ad->dt * speed;
				Quat rotation = quat(rotAmount, vec3(0,0,1));
				platform->xf.rot = rotation * platform->xf.rot;

				Entity* p = ad->player;
				Mesh* mesh = dxGetMesh(walkMesh->mesh);
				float dist = raycastEntity(p->xf.trans + vec3(0,0,0.1), vec3(0,0,-1), walkMesh, mesh);
				if(dist != -1) {
					if(dist < 0.5f) {
						Entity* playerCam = findEntity(em, ET_Camera, "PlayerCam");
						playerCam->camRot.x = playerCam->camRot.x + rotAmount;

						rotateAround(&p->xf.trans, rotation, platform->xf.trans);
					}
				}
			}
		}
	}

	// @Cam.
	{
		Entity* playerCam = findEntity(em, ET_Camera, "PlayerCam");
		Entity* cam = playerCam;
		if(ad->freeCam) {
			cam = ad->camera;
		}

		theGState->activeCam = getCamData(cam->xf.trans, cam->camRot, vec3(0,0,0));

		GraphicsSettings* gs = theGState->gSettings;
		theGState->activeCam.dim = getCamDim(gs->aspectRatio, gs->fieldOfView, gs->nearPlane);
		theGState->activeCam.rot = cam->camRot;

		// @Optimize.
		{
			TIMER_BLOCK_NAMED("SceneBoundingBox");
			ad->sceneBoundingBox = sceneBoundingBox(em);
		}
	}

	// Redundant with initial app setup, but needed for game load.
	{
		GraphicsState* gs = theGState;
		GraphicsSettings* gSettings = gs->gSettings;

		{
			GraphicsMatrices* gMats = &gs->gMats;
			Rect sr = getScreenRect(gs->screenRes);
			gMats->view2d = identityMatrix();
			gMats->ortho  = orthoMatrixZ01(0, sr.bottom, sr.right, 0, 10, -10);

			gMats->view = viewMatrix(gs->activeCam.pos, -gs->activeCam.look, gs->activeCam.up, gs->activeCam.right);
			gMats->proj = projMatrixZ01(degreeToRadian(gSettings->fieldOfView), gSettings->aspectRatio, gSettings->nearPlane, gSettings->farPlane);

			gMats->viewInv = viewMatrixInv(gs->activeCam.pos, -gs->activeCam.look, gs->activeCam.up, gs->activeCam.right);
			gMats->projInv = projMatrixZ01Inv(gMats->proj);
		}

		setupShaders(*gSettings, gs->activeCam, false, false);
	}

	// if(ad->sky) 
	{
		renderSky(&ad->redrawSkyBox, &ad->sky->skySettings, reload);
	}

	{
		dxBindFrameBuffer("3dMsaa", "ds3d");

		dxGetShaderVars(Primitive)->viewProj = gs->gMats.proj * gs->gMats.view;
		dxPushShaderConstants(Shader_Primitive);
	}

	// @Rendering.

	// @3d.
	{
		TIMER_BLOCK_NAMED("3dScene");
		dxLineAA(2);

		// @Grid.
		if(ds->drawGrid) {
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

		{
			// Setup light view and projection.
			{
				XForm rbb = aabbToObb(ad->sceneBoundingBox.xf(), ad->sky->skySettings.sunRot);
				Mat4 viewLight = viewMatrix(rbb.trans, dxGetShaderVars(Sky)->sunDir);
				Mat4 projLight = orthoMatrixZ01(rbb.scale.y, rbb.scale.z, -rbb.scale.x*0.5f, rbb.scale.x*0.5f);

				ad->viewProjLight = projLight * viewLight;
			}

			Mat4 viewProjLight = ad->viewProjLight;

			// Render scene.
			for(int stage = 0; stage < 2; stage++) {

				// Render shadow maps.
				if(stage == 0) {
					dxDepthTest(true);
					dxScissorState(false);
					dxClearFrameBuffer("Shadow");

					// Removes warning.
					ID3D11ShaderResourceView* srv = 0;
					gs->d3ddc->PSSetShaderResources(5, 1, &srv);

					dxBindFrameBuffer(0, "Shadow"); 

					Vec2i vp = vec2i(gs->shadowMapSize);
					dxViewPort(vp);

					dxSetShader(Shader_Shadow);
					dxGetShaderVars(Shadow)->viewProj = viewProjLight;

				} else {
					dxBindFrameBuffer("3dMsaa", "ds3d");
					dxSetShader(Shader_Main);
					dxViewPort(theGState->gSettings->cur3dBufferRes);

					FrameBuffer* fb = dxGetFrameBuffer("Shadow");
					gs->d3ddc->PSSetShaderResources(5, 1, &fb->shaderResourceView);

					dxGetShaderVars(Main)->mvpShadow.viewProj = viewProjLight;
				}

				for(int i = 0; i < ad->entityManager.entities.count; i++) {
					Entity* e = ad->entityManager.entities.data + i;
					if(!entityIsValid(e)) continue;

					if(e->type == ET_Player && !ad->freeCam) continue;

					if(e->noRender) continue;

					XForm xf = e->xf;
					if(e->type == ET_ParticleEffect ||
					   e->type == ET_Group) {
						if(e->type == ET_ParticleEffect && !ds->drawParticleHandles) continue;
						if(e->type == ET_Group && !ds->drawGroupHandles) continue;

						xf.scale = vec3(1);
					}

					// Hack, fix the default figure mesh.
					if(ad->levelEdit && entityCheck(e, ET_Object, "figure")) {
						xf.rot = xf.rot * quatDeg(90, vec3(1,0,0));
					}

					dxDrawObject(xf, e->color, e->mesh, e->material, stage == 0);
				}
			}

			dxCullState(true);
			dxSetBlendState(Blend_State_Blend);
		}

		if(ds->drawBlockers) {
			MainShaderVars* vars = dxGetShaderVars(Main);
			dxSetShader(Shader_Main);

			vars->disableLighting = true;
			dxPushShaderConstants(Shader_Main);
			defer { 
				vars->disableLighting = false;
				dxPushShaderConstants(Shader_Main);
			};

			dxFillWireFrame(true); defer { dxFillWireFrame(false); };
			dxDepthTest(false); defer { dxDepthTest(true); };
			dxCullState(false); defer { dxCullState(true); };

			Vec4 cBlocker = vec4(1,1,0,1);
			for(auto& e : em->entities) {
				if(!entityIsValid(&e) || !e.blockerType) continue;

				XForm xf = e.xf * e.xfBlocker;

				switch(e.blockerType) {
					case BLOCKERTYPE_RECT:   dxDrawObject(xf, cBlocker, "cube\\obj.obj", "Empty\\material.mtl", false); break;
					case BLOCKERTYPE_CIRCLE: dxDrawObject(xf, cBlocker, "cylinder\\obj.obj", "Empty\\material.mtl", false); break;
					case BLOCKERTYPE_LINE:   {
						dxSetShader(Shader_Primitive);
						dxDrawLine(xf * vec3(-0.5f), xf * vec3(0.5f), cBlocker); break;
						dxSetShader(Shader_Main);
					}
				}
			}
		}
	}

	// @Walkmanifold.
	if(ds->showUI && ds->drawManifold) {
		dxSetShader(Shader_Primitive);
		ad->manifold.debugDraw();
	}

	if(ad->showSkeleton) {
		dxSetShader(Shader_Primitive);

		dxDepthTest(false);
		defer{dxDepthTest(true);};

		Mesh* mesh = ad->figureMesh;

		Entity* figure = ad->figure;
		Mat4 model = modelMatrix(figure->xf);

		drawSkeleton(mesh->animPlayer.bones, model, &mesh->boneTree);
	}

	// @Particles.
	drawParticles(em, ad->viewProjLight);

	{
		EntityUI* eui = &ds->entityUI;
		if(eui->dragSelectionActive) {
			dxSetShader(Shader_Primitive);
			dxDepthTest(false);
			defer { dxDepthTest(true); };

			for(auto& e : ad->entityManager.entities) {
				if(!entityIsValid(&e)) continue;
				if(!e.mesh || !dxGetMesh(e.mesh)) continue;
				if(e.type == ET_ParticleEffect && !ds->drawParticleHandles) continue;
				if(e.type == ET_Group && !ds->drawGroupHandles) continue;

				XForm xf = e.xf;
				xf.scale = vec3(0.05f);
				dxDrawCube(xf, vec4(0,1,1,1));
			}
		}

		// Debug highlight entities that are selected.
		if(showUI && eui->selectedObjects.count) {
			drawEntityUI(ds, em, input->mousePosNegative, ws, ad->freeCam, ad->levelEdit);
		}
	}

	renderBloom(gs->gMats);

	// @2d.
	{
		dxBindFrameBuffer("2dMsaa", "ds");
		dxSetShader(Shader_Primitive);
		dxLineAA(1);

		Rect sr = gs->screenRect;
	}
}
