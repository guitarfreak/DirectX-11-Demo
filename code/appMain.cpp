
void gameLoad(AppData* ad) {
	// loadMap(&ad->entityManager, "walkmanifoldtest");
	// loadMap(&ad->entityManager, "level");
	loadMap(&ad->entityManager, "showcase");

	ad->newGameMode = GAME_MODE_MAIN;
}

void gameLoop(AppData* ad, DebugState* ds, WindowSettings* ws, bool showUI, bool init, bool reload) {
	TIMER_BLOCK();

	GraphicsState* gs = theGState;
	Input* input = &ad->input;

	// Open menu.
	if(input->keysPressed[KEYCODE_ESCAPE]) {
		ad->newGameMode = GAME_MODE_MENU;
		ad->menu.gameRunning = true;
		ad->menu.activeId = 0;

		renderMenuBackground();
	}

	if(input->keysPressed[KEYCODE_F4]) {
		ad->playerMode = !ad->playerMode;
	}

	{
		EntityManager* em = &ad->entityManager;

		for(int i = 0; i < em->entities.count; i++) {
			Entity* e = em->entities.data + i;
			if(!entityIsValid(e)) continue;

			if(e->type == ET_Player) ad->player = e;
			else if(e->type == ET_Camera) ad->camera = e;
			else if(e->type == ET_Sky) ad->sky = e;

			if(entityCheck(e, ET_Object, "figure")) ad->figure = e; 
		}

		collectByType(em);
	}

	// @Entities.
	updateEntities(&ad->entityManager, input, ad->dt, ad->playerMode, ad->mouseEvents, ad->mouseSensitivity, &ds->entityUI);

	// @Cam.
	{
		Entity* e = ad->playerMode ? ad->player : ad->camera;
		theGState->activeCam = getCamData(e->xf.trans, e->camRot, vec3(0,0,0));

		GraphicsSettings* gs = theGState->gSettings;
		theGState->activeCam.dim = getCamDim(gs->aspectRatio, gs->fieldOfView, gs->nearPlane);

		// @Optimize.
		{
			TIMER_BLOCK_NAMED("SceneBoundingBox");
			ad->sceneBoundingBox = sceneBoundingBox(&ad->entityManager);
		}
	}

	// if(ad->sky) 
	{
		renderSky(&ad->redrawSkyBox, &ad->sky->skySettings, ad->camera->camRot, reload);
	}

	{
		dxBindFrameBuffer("3dMsaa", "ds3d");

		dxGetShaderVars(Primitive)->viewProj = gs->gMats.proj * gs->gMats.view;
		dxPushShaderConstants(Shader_Primitive);
	}

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

	{
		for(int i = 0; i < ad->entityManager.entities.count; i++) {
			Entity* e = ad->entityManager.entities.data + i;
			if(!entityIsValid(e) || !entityCheck(e, ET_Object, "Rotating Object")) continue;

			e->xf.rot = e->xf.rot * quat(ad->dt*0.2f, vec3(0,0,1));
		}
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

					if(e->type == ET_Player && ad->playerMode) continue;

					XForm xf = e->xf;
					if(e->type == ET_ParticleEffect ||
					   e->type == ET_Group) {
						if(e->type == ET_ParticleEffect && !ds->drawParticleHandles) continue;
						if(e->type == ET_Group && !ds->drawGroupHandles) continue;

						xf.scale = vec3(1);
					}

					dxDrawObject(xf, e->color, e->mesh, e->material, stage == 0);
				}
			}

			dxCullState(true);
			dxSetBlendState(Blend_State_Blend);
		}
	}

	// @Walkmanifold.
	if(false)
	{
		dxSetShader(Shader_Primitive);

		// dxDepthTest(false);
		// defer{ dxDepthTest(true); };			

		Vec3 off = vec3(0,0,0.001f);

		DArray<WalkMesh> walkMeshes = dArray<WalkMesh>(getTMemory);
		for(auto& e : ad->entityManager.entities) {
			if(!strCompare(e.name, "walkmesh")) continue;

			walkMeshes.push({e.xf});
		}

		DArray<Blocker> sceneBlockers = dArray<Blocker>(getTMemory);
		// for(auto& e : ad->entityManager.entities) {
		// 	if(!strCompare(e.name, "blocker")) continue;

		// 	sceneBlockers.push({ BLOCKERTYPE_RECT, e.xf });
		// }

		{
			Entity* ep = findEntity(&ad->entityManager, "playerPos");

			static Vec3 playerPos = ep->xf.trans;
			static Vec3 newPos = playerPos;
			static bool moveMode = false;
			static Vec2 mouseStartPos;

			WalkManifold mf;
			mf.init(playerPos, recti(-10,-10,10,10), 0.3f, 0.27f, 0.2f, 
			        walkMeshes.data, walkMeshes.count, sceneBlockers.data, sceneBlockers.count);

			mf.rasterize();
			mf.debugDraw();

			static bool start = true;
			if(start) {
				start = false;
				playerPos = mf.calcWalkCellPos(playerPos.xy);
			}

			#if 1
			{
				bool calcMove = false;
				if(input->mouseButtonPressed[0] || 
				   input->mouseButtonDown[2]) calcMove = true;

				{
					Camera* cam = &theGState->activeCam;
					Vec3 rayDir = mouseRayCast(gs->screenRect, input->mousePosNegative, cam, gs->gSettings->nearPlane);

					Vec3 intersection;
					bool result = mf.raycast(cam->pos, rayDir, &intersection);

					if(result) {
						newPos = intersection;

						// dxDrawCube(xForm(intersection, vec3(0.02f)), vec4(1,0.5f,0,1));
						// dxDrawLine(intersection, intersection + vec3(0,0,0.2f), vec4(1,0.8f,0,1));
					}
				}

				{
					dxDepthTest(false);
					dxDrawLine(playerPos+off, newPos+off, vec4(1,0,1,1));
					// dxDrawLine(playerPos+off, playerPos+vec3(0,0,1)+off, vec4(1,0,1,1));
					dxDrawRing(playerPos+off, vec3(0,0,-1), mf.playerRadius, 0.01f, vec4(1,1,0,1), 0.002f);
					dxDepthTest(true);
				}

				if(calcMove) {
					int stop = 234;
				}

				bool printOut = false;
				if(input->mouseButtonPressed[2]) printOut = true;

				if(playerPos.xy != newPos.xy) {
					Vec3 pos = mf.move(playerPos.xy, newPos.xy);

					dxDrawLine(pos, pos+off, vec4(0,1,0,1));
					dxDrawRing(pos+off, vec3(0,0,-1), mf.playerRadius, 0.01f, vec4(1,0.5,0,1), 0.002f);

					if(calcMove) {
						playerPos = pos;
					}
				}

				// {
				// 	bool isBlocked = false;
					
				// 	Vec2i cellIndex = mf.getWalkCell(newPos.xy);

				// 	// alkCell* cell = &cells[aIndex(cellsDim.w, cellCoord->x-grid.left, cellCoord->y-grid.bottom)];
				// 	WalkCell* cell = &mf.cells[aIndex(mf.cellsDim.w, cellIndex.x-mf.grid.left, cellIndex.y-mf.grid.bottom)];

				// 	if(cell->lineCount) {
				// 		for(int i = 0; i < cell->lineCount; i++) {
				// 			Line3 line = cell->lines[i];
				// 			Vec2 a = norm(line.b.xy - line.a.xy);
				// 			Vec2 normal = rotateRight(a);

				// 			float d = dot(normal, newPos.xy - line.a.xy);
				// 			if(d < 0) {
				// 				isBlocked = true;
				// 				break;
				// 			}
				// 		}

				// 	} else {
				// 		// if(cell->poles[i]->blockerCount) {
				// 		// 	isBlocked = true;
				// 		// } else {
				// 		// 	isBlocked = false;
				// 		// }
				// 	}

				// 	if(isBlocked) {
				// 		dxDrawLine(newPos, newPos + vec3(0,0,0.2f), vec4(1,0,0,1));
				// 	}
				// }

			}
			#endif

			if(true)
			{
				dxDepthTest(true);
				// defer{ dxDepthTest(true); };	

				// dxDrawLine(playerPos, playerPos + lineOff, vec4(1,1,0,1));

				#if 0

				dxDrawRect(getScreenRect(ws).expand(-20), vec4(0.2f,1));

				static Vec2 p = vec2(150,-150);

				// Vec2 points[] = {{100,-100}, {160,-140}, {200,-200}, {100,-200}, };
				Vec2 points[] = {{100,-80}, {160,-140}, {250,-200}, {170,-200}, };

				Line lines[100] = {};
				int lineCount = 0;

				for(int i = 0; i < arrayCount(points); i++) {
					lines[lineCount++] = {vec3(points[i],0), vec3(points[(i+1)%arrayCount(points)],0)};
				}

				for(int i = 0; i < lineCount; i++) {
					dxDrawLine(lines[i].a, lines[i].b, vec4(1,0,0,1));
				}

				float speed = 100;
				Vec2 vel = {};
				if(input->keysDown[KEYCODE_I]) vel.y += 1;
				if(input->keysDown[KEYCODE_K]) vel.y -= 1;
				if(input->keysDown[KEYCODE_L]) vel.x += 1;
				if(input->keysDown[KEYCODE_J]) vel.x -= 1;

				// if(vel != vec2(0,0)) 
				if(mouseInClientArea(windowHandle))
				{
					Vec2 oldPoint = p;

					// vel = norm(vel) * speed;
					// Vec2 np = p + vel*ad->dt;
					Vec2 np = input->mousePosNegative;
					Vec2 dir = p - np;

					int lastLineIndex = -1;

					Vec2 projectDir;

					int index = 0;
					while(true) {
						Vec2 cp;
						int lineIndex = lineCollision(p, np, lines, lineCount, lastLineIndex, &cp);

						if(lineIndex == -1) {
							p = np;
							break;
						}

						lastLineIndex = lineIndex;

						// Project onto line.

						{
							if(index == 0) {
								projectDir = np - cp;
							}

							Line l = lines[lineIndex];

							Vec2 a = norm(l.a.xy - l.b.xy);
							Vec2 b = projectDir;
							Vec2 pp = cp + a * dot(a, b);

							projectDir = norm(projectDir) * len(cp - pp);

							dxDrawLine(p, np, vec4(0.5f,1));

							// p = cp;
							// np = pp;

							dxDrawCircle(cp,  3, vec4(0,1,0,0.5f));
							dxDrawCircle(pp, 3, vec4(0,1,1,0.5f));

							Vec2 normal = norm(rotateRight(a));
							Vec2 mid = (l.a.xy + l.b.xy) / 2.0f;

							dxDrawLine(mid, mid + normal * 10, vec4(1,0.5f,0,1));

							float offset = 0.001f;
							// float offset = 20.0f;
							// float offset = 0;
							Vec2 op  = cp - normal * offset;
							Vec2 onp = pp - normal * offset;

							dxDrawCircle(op, 3, vec4(0,1,0,1));
							dxDrawCircle(onp, 3, vec4(0,1,1,1));

							{
								Vec2 ncp;
								int lineIndex = lineCollision(cp, op, lines, lineCount, lastLineIndex, &ncp);
								// printf("%i\n", lineIndex);
								if(lineIndex != -1) {
									Line ol = {vec3(l.a.xy - normal * offset,0), vec3(l.b.xy - normal * offset,0)};
									Vec2 ep;
									if(getLineIntersection(p, cp, ol.a.xy, ol.b.xy, &ep)) {
										p = ep;

										dxDrawCircleX(ep, 2, vec4(1,0,1,1));
									}

									break;
								}
							}

							p = op;
							np = onp;
						}

						if(index == 5) break;
						index++;
					}

					if(!(input->mouseButtonPressed[0] || (input->mouseButtonDown[0] && input->keysDown[KEYCODE_CTRL]))) {
						p = oldPoint;
					}

					// {
					// 	Vec2 cp;
					// 	int lineIndex = lineCollision(p, p, lines, lineCount, -1, &cp);
					// 	assert(lineIndex == -1);
					// }
				}

				// if(mouseInClientArea(windowHandle))
				// {
				// 	Line l0 = {p, input->mousePosNegative};
				// 	Line l1 = lines[0];
				// 	dxDrawLine(l0.a, l0.b, vec4(0.5f,1));

				// 	Vec2 cp;
				// 	if(getLineIntersection(l0.a,l0.b, l1.a,l1.b, &cp)) {
				// 		dxDrawCircleX(cp, 2, vec4(1,1,0,1));
				// 		// Line l2 = {cp, cp + (l1.a-l1.a)};

				// 		Vec2 a = l0.b - cp;
				// 		Vec2 b = norm(l1.a - cp);
				// 		Vec2 projectedPoint = cp + b * dot(b, a);

				// 		dxDrawCircleX(projectedPoint, 2, vec4(0,1,0,1));
				// 	}
				// }

				dxDrawCircleX(p, 1, vec4(0.8f,1));
				#endif
			}
		}
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
	drawParticles(&ad->entityManager, ad->viewProjLight);

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
			drawEntityUI(ds, &ad->entityManager, input->mousePosNegative, ws, ad->playerMode);
		}
	}

	renderBloom(gs->gMats);

	// @2d.
	{
		dxBindFrameBuffer("2dMsaa", "ds");
		dxSetShader(Shader_Primitive);
		dxLineAA(1);

		Rect sr = gs->screenRect;

		#if 0
		{
			float radius = 10;
			Rect r = rectTLDim(0,0,400,400);
			dxDrawRect(r, vec4(0.1f,1));
			Vec2 offset = vec2(200,-200);
			float scale = 100;

			static Vec2 pos = vec2(0);
			static bool start = true;
			if(start) {
				start = false;
				Entity* e = findEntity(&ad->entityManager, "start");
				pos = e->xf.trans.xy * 100 + offset;
			}

			Vec2 newPos = input->mousePosNegative;
			if(!pointInRect(newPos, gs->screenRect)) newPos = pos;

			// pos = input->mousePosNegative;

			DArray<Line2> lines = dArray<Line2>(getTMemory);
			for(auto e : ad->entityManager.entities) {
				if(strCompare(e.name, "line")) {
					Vec2 p0 = (e.xf * vec3(0,0,-0.5f)).xy;
					Vec2 p1 = (e.xf * vec3(0,0, 0.5f)).xy;
					p0 = p0 * scale + offset;
					p1 = p1 * scale + offset;
					lines.push({p0, p1});
				}
			}

			if(input->mouseButtonPressed[0]) {
				int stop = 234;
			}

			for(auto& it : lines) dxDrawLine(it.a, it.b, vec4(1,1,1,1));
			dxDrawCircle(pos, 2,    vec4(1,0,0,1));
			dxDrawRing(pos, radius, vec4(1,0,0,1));
			dxDrawCircle(newPos, 2, vec4(0,1,0,1));
			dxDrawLine(pos, newPos, vec4(0,1,1,1));

			Vec2 finalPos;
			{
				Vec2 p = pos;
				Vec2 fp = newPos;
				Vec2 np;

				float maxStepSize = radius*1.9f;
				int maxConsecutiveHits = 5;
				int maxTotalHits = 10;

				bool first = true;
				bool abort = false;
				int totalHits = -1;
				while(true) {
					if(abort) break;
					if(totalHits >= 10) break;

					float restLength = len(p - fp);
					if(restLength == 0) break;

					float stepSize = min(maxStepSize, restLength);
					np = p + norm(fp - p) * stepSize;

					int conscutiveHits = -1;
					while(true) {
						conscutiveHits++;
						if(conscutiveHits > 1) {
							printf("%i\n", conscutiveHits);
						}

						bool hit = false;
						float furthestLen = 0;
						Vec2 furthestVec = vec2(0.0f);

						for(auto it : lines) {
							Blocker b = {BLOCKERTYPE_LINE};
							b.linePoints[0] = vec3(it.a);
							b.linePoints[1] = vec3(it.b);

							Vec2 vec;
							bool result = gjkEpa({BLOCKERTYPE_CIRCLE, xForm(vec3(np,0), vec3(radius))}, p, b, &vec, true);

							if(result && first) {
								if(dot(vec, np - p) > 0) result = false;
							}

							if(result) {
								float l = len(vec);
								if(l > furthestLen) {
									furthestVec = vec;
									furthestLen = l;
									hit = true;
								}
							}
						}

						first = false;

						if(hit) {
							totalHits++;

							Vec2 cp = np + furthestVec;
							np = cp;

							if(conscutiveHits >= maxConsecutiveHits) {
								abort = true;
								p = np;
								break;
							}

							continue;
						}

						// dxDrawRing(np, radius, vec4(1,0,1,1));

						break;
					}

					p = np;
					dxDrawRing(p, radius, vec4(1,1,0,1));
				}

				finalPos = p;
			}

			if(input->mouseButtonPressed[0]) {
				pos = finalPos;
			}
		}
		#endif

		// GJK test.
		// {
		// 	// {
		// 	// 	Rect r = rectTLDim(10,-10,100,100);
		// 	// 	dxDrawRect(r, vec4(1,0,0,1));
		// 	// }

		// 	Blocker b  = {BLOCKERTYPE_RECT, xForm(vec3(100,-100,3), quatDeg(0,vec3(0,0,1)), vec3(60))};
		// 	// Blocker b2 = {BLOCKERTYPE_CIRCLE, xForm(vec3(160,-100,3), vec3(30))};
		// 	Blocker b2 = {BLOCKERTYPE_CIRCLE, xForm(vec3(input->mousePosNegative,3), vec3(30))};

		// 	// dxFillWireFrame(true);
		// 	// defer {dxFillWireFrame(false);};

		// 	Vec3 dir = quat(ad->figureSpeed*4, vec3(0,0,1)) * vec3(1,0,0);

		// 	// Vec2 fp = furthestPointInDirection(&b, dir.xy);
		// 	// Vec2 sp = support(&b, &b2, dir.xy);

		// 	bool result = gjk(&b, vec2(0,0), &b2);
		// 	// result = true;

		// 	dxDrawRect(rect(-b.form.scale.xy/2.0f, b.form.scale.xy/2.0f) + b.form.trans.xy, vec4(0,0,0,1));
		// 	dxDrawCircleX(b2.form.trans.xy, b2.form.scale.x, vec4(0.2f + ((int)result)*0.5f,1));

		// 	dxDrawLine(b.form.trans.xy, b.form.trans.xy+dir.xy * 100, vec4(1,1,1,1));

		// 	// dxDrawLine(b.form.trans.xy, fp, vec4(1,0,0,1));
		// 	// dxDrawLine(b.form.trans.xy, p, vec4(1,0,0,1));
		// 	// dxDrawLine(b.form.trans.xy, b.form.trans.xy + dir.xy * 0.5f, vec4(0,1,0,1));
		// 	// dxDrawLine(b.form.trans, vec3(sp, b.form.trans.z), vec4(1,0,1,1));
		// }

		#if 0
		{
			dxDrawRect(getScreenRect(ws).expand(-20), vec4(0.2f,1));

			static Vec2 p = vec2(150,-150);

			// Vec2 points[] = {{100,-100}, {160,-140}, {200,-200}, {100,-200}, };
			Vec2 points[] = {{100,-80}, {160,-140}, {250,-200}, {170,-200}, };

			Line lines[100] = {};
			int lineCount = 0;

			for(int i = 0; i < arrayCount(points); i++) {
				lines[lineCount++] = {vec3(points[i],0), vec3(points[(i+1)%arrayCount(points)],0)};
			}

			for(int i = 0; i < lineCount; i++) {
				dxDrawLine(lines[i].a, lines[i].b, vec4(1,0,0,1));
			}

			float speed = 100;
			Vec2 vel = {};
			if(input->keysDown[KEYCODE_I]) vel.y += 1;
			if(input->keysDown[KEYCODE_K]) vel.y -= 1;
			if(input->keysDown[KEYCODE_L]) vel.x += 1;
			if(input->keysDown[KEYCODE_J]) vel.x -= 1;

			// if(vel != vec2(0,0)) 
			if(mouseInClientArea(windowHandle))
			{
				Vec2 oldPoint = p;

				// vel = norm(vel) * speed;
				// Vec2 np = p + vel*ad->dt;
				Vec2 np = input->mousePosNegative;
				Vec2 dir = p - np;

				int lastLineIndex = -1;

				Vec2 projectDir;

				int index = 0;
				while(true) {
					Vec2 cp;
					int lineIndex = lineCollision(p, np, lines, lineCount, lastLineIndex, &cp);

					if(lineIndex == -1) {
						p = np;
						break;
					}

					lastLineIndex = lineIndex;

					// Project onto line.

					{
						if(index == 0) {
							projectDir = np - cp;
						}

						Line l = lines[lineIndex];

						Vec2 a = norm(l.a.xy - l.b.xy);
						Vec2 b = projectDir;
						Vec2 pp = cp + a * dot(a, b);

						projectDir = norm(projectDir) * len(cp - pp);

						dxDrawLine(p, np, vec4(0.5f,1));

						// p = cp;
						// np = pp;

						dxDrawCircle(cp,  3, vec4(0,1,0,0.5f));
						dxDrawCircle(pp, 3, vec4(0,1,1,0.5f));

						Vec2 normal = norm(rotateRight(a));
						Vec2 mid = (l.a.xy + l.b.xy) / 2.0f;

						dxDrawLine(mid, mid + normal * 10, vec4(1,0.5f,0,1));

						float offset = 0.001f;
						// float offset = 20.0f;
						// float offset = 0;
						Vec2 op  = cp - normal * offset;
						Vec2 onp = pp - normal * offset;

						dxDrawCircle(op, 3, vec4(0,1,0,1));
						dxDrawCircle(onp, 3, vec4(0,1,1,1));

						{
							Vec2 ncp;
							int lineIndex = lineCollision(cp, op, lines, lineCount, lastLineIndex, &ncp);
							// printf("%i\n", lineIndex);
							if(lineIndex != -1) {
								Line ol = {vec3(l.a.xy - normal * offset,0), vec3(l.b.xy - normal * offset,0)};
								Vec2 ep;
								if(getLineIntersection(p, cp, ol.a.xy, ol.b.xy, &ep)) {
									p = ep;

									dxDrawCircleX(ep, 2, vec4(1,0,1,1));
								}

								break;
							}
						}

						p = op;
						np = onp;
					}

					if(index == 5) break;
					index++;
				}

				if(!(input->mouseButtonPressed[0] || (input->mouseButtonDown[0] && input->keysDown[KEYCODE_CTRL]))) {
					p = oldPoint;
				}

				// {
				// 	Vec2 cp;
				// 	int lineIndex = lineCollision(p, p, lines, lineCount, -1, &cp);
				// 	assert(lineIndex == -1);
				// }
			}

			// if(mouseInClientArea(windowHandle))
			// {
			// 	Line l0 = {p, input->mousePosNegative};
			// 	Line l1 = lines[0];
			// 	dxDrawLine(l0.a, l0.b, vec4(0.5f,1));

			// 	Vec2 cp;
			// 	if(getLineIntersection(l0.a,l0.b, l1.a,l1.b, &cp)) {
			// 		dxDrawCircleX(cp, 2, vec4(1,1,0,1));
			// 		// Line l2 = {cp, cp + (l1.a-l1.a)};

			// 		Vec2 a = l0.b - cp;
			// 		Vec2 b = norm(l1.a - cp);
			// 		Vec2 projectedPoint = cp + b * dot(b, a);

			// 		dxDrawCircleX(projectedPoint, 2, vec4(0,1,0,1));
			// 	}
			// }

			dxDrawCircleX(p, 1, vec4(0.8f,1));
		}
		#endif

	}
}
