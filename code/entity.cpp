
float camDistanceFromFOVandWidth(float fovInDegrees, float w) {
	float angle = degreeToRadian(fovInDegrees);
	float sideAngle = ((M_PI-angle)/2.0f);
	float side = w/sin(angle) * sin(sideAngle);
	float h = side*sin(sideAngle);
	
	return h;
}

Camera getCamData(Vec3 pos, Vec2 rot, Vec3 offset = vec3(0,0,0), Vec3 up = vec3(0,0,1), Vec3 forward = vec3(0,1,0)) {
	Camera c;
	c.pos = pos + offset;
	c.look = forward;
	rotate(&c.look, rot.x, up);
	rotate(&c.look, rot.y, norm(cross(up, c.look)));
	c.up = norm(cross(c.look, norm(cross(up, c.look))));
	c.right = norm(cross(up, c.look));
	c.look = -c.look;

	// c.look  = rot * forward;
	// c.up    = rot * up;
	// c.right = norm(cross(c.look, c.up));

	return c;
}

Vec2 getCamDim(float aspect, float fieldOfView, float nearPlane) {
	float camRight = 1 / (aspect*tan(degreeToRadian(fieldOfView)*0.5f));
	float camTop   = 1 / (tan(degreeToRadian(fieldOfView)*0.5f));
	float camHeight = nearPlane*(2*tan(degreeToRadian(fieldOfView) / 2.0f));
	float camWidth = camHeight * aspect;

	return vec2(camWidth, camHeight);
}

Camera getCamDataLook(Vec3 pos, Vec3 look) {
	Camera c;
	c.pos = pos;
	c.look = norm(look);
	c.right = norm(cross(vec3(0,0,1), look));
	c.up = norm(cross(look, c.right));

	return c;
}

Camera getCamDataLook(Vec3 pos, Vec3 look, Vec3 right) {
	Camera c;
	c.pos = pos;
	c.look = norm(look);
	c.right = norm(right);
	c.up = norm(cross(look, c.right));

	return c;
}

Mat4 viewMatrix(Camera cam) {
	return viewMatrix(cam.pos, -cam.look, cam.up, cam.right);
}

Vec3 vectorToCam(Vec3 pos, Camera* cam) {
	Vec3 intersection;
	float distance = linePlaneIntersection(pos, -cam->look, cam->pos, cam->look, &intersection);
	if(distance != -1) {
		return intersection - pos;
	}

	return vec3(0,0,0);
}

//

Entity entity(int type, char* name = "", bool temp = false) {
	Entity e = {};
	e.type = type;
	e.name = temp ? getPString(name) : getTString(name);
	e.groupId = 0;
	e.mountParentId = 0;

	e.mesh = temp ? getPString("") : getTString(name);
	e.material = temp ? getPString("") : getTString(name);

	e.xfBlocker = xForm();

	return e;
}

Entity entity(int type, XForm xf, bool temp, char* name = 0, char* mesh = 0, char* material = 0, Vec4 color = vec4(1,1)) {
	Entity e = {};
	e.type = type;
	e.xf = xf;
	e.name = temp ? (name ? getTString(name) : getTString("")) :
	                (name ? getPString(name) : getPString(""));
	e.groupId = 0;
	e.mountParentId = 0;

	e.mesh = temp ? (mesh ? getTString(mesh) : getTString("")) :
	                (mesh ? getPString(mesh) : getPString(""));
	e.material = temp ? (material ? getTString(material) : getTString("")) :
	                    (material ? getPString(material) : getPString(""));
	e.color = color;

	e.xfBlocker = xForm();

	return e;
}

Entity getDefaultEntity(int type, Vec3 pos, bool temp = false) {
	Entity e;
	switch(type) {
		case ET_Player: e = entity(type, xForm(pos), temp); break;
		case ET_Camera: e = entity(type, xForm(pos), temp); break;
		case ET_Sky:    e = entity(type, xForm(pos), temp); break;
		case ET_Object: e = entity(type, xForm(pos), temp, 0, "sphere\\obj.obj", "Matte\\mat.mtl", vec4(1,1,1,1)); break;
		case ET_ParticleEffect: {
			e = entity(type, xForm(pos), temp, 0, "entityUI_particle\\obj.obj", "Matte\\mat.mtl", hslToRgbf(0.8f,0.5f,0.5f,1));
			e.particleEmitter.init();
		} break;
		// case ET_Group:  e = entity(type, xForm(pos), 0, "entityUI_group\\obj.obj", "Matte\\mat.mtl", hslToRgbf(0.8f,0.5f,0.5f,1)); break;
		// case ET_Group:  e = entity(type, xForm(pos), 0, "entityUI_group\\obj.obj", 0, hslToRgbf(0.8f,0.5f,0.5f,1)); break;
		case ET_Group:  e = entity(type, xForm(pos), temp, 0, "entityUI_group\\obj.obj", 0, vec4(1,1,1,1)); break;
		case ET_Sound:  e = entity(type, xForm(pos), temp); break;
	};
	return e;
}


bool entityIsValid(Entity* e) {
	return !e->deleted;
}

Entity* getEntity(EntityManager* em, int id) {
	if(!id) return 0;
	return em->entities + em->indices[id-1];
}

Entity* findEntity(EntityManager* em, char* name, int* index = 0) {
	if(!strLen(name)) return 0;
	for(int i = 0; i < em->entities.count; i++) {
		Entity* e = em->entities.data + i;
		if(entityIsValid(e) && strCompare(e->name, name)) {
			if(index) *index = i;
			return e;
		}
	}
	return 0;
}

Entity* findEntity(EntityManager* em, int type, char* name, int* index = 0) {
	if(!strLen(name)) return 0;
	for(int i = 0; i < em->entities.count; i++) {
		Entity* e = em->entities.data + i;
		if(entityIsValid(e) && e->type == type && strCompare(e->name, name)) {
			if(index) *index = i;
			return e;
		}
	}
	return 0;
}

DArray<int> getNextEntityIds(EntityManager* em, int count) {
	DArray<int> ids = dArray<int>(5, getTMemory);
	for(auto& it : em->indices) {
		if(it == -1) {
			int id = (&it - em->indices.data) + 1;
			ids.push(id);
		}
	}
	if(ids.count < count) {
		int restCount = count - ids.count;
		for(int i = 0; i < restCount; i++) {
			int id = em->indices.count + 1 + i;
			ids.push(id);
		}
	}

	return ids;
}

int getNextEntityId(EntityManager* em, int index = 0) {
	int currentIndex = 0;
	for(auto& it : em->indices) {
		if(it == -1) {
			if(currentIndex == index) return (&it - em->indices.data) + 1;
			currentIndex++;
		}
	}

	int temp = 0;
	while(true) {
		if(currentIndex == index) return em->indices.count + 1 + temp;
		currentIndex++;
		temp++;
	}
}

Entity* addEntity(EntityManager* em, Entity* e, bool keepIndex = false) {
	// Init.
	if(e->type == ET_ParticleEffect) {
		e->particleEmitter.reset();
	}

	int id = 0;
	if(!keepIndex) {
		for(int i = 0; i < em->indices.count; i++) {
			if(em->indices[i] == -1) {
				em->indices[i] = em->entities.count;
				id = i+1;
				break;
			}
		}

		if(!id) {
			id = em->indices.count + 1;
			em->indices.push(em->entities.count);
		}
	} else {
		id = e->id;
		em->indices[e->id - 1] = em->entities.count;
	}

	Entity entity = *e;
	entity.id = id;
	em->entities.push(entity);

	return em->entities.data + em->entities.count-1;
}

Entity* addEntity(EntityManager* em, Entity e) {
	return addEntity(em, &e);
}

DArray<Entity> addEntities(EntityManager* em, Entity* list, int count) {
	DArray<Entity> addedList = dArray<Entity>(getTMemory);
	for(int i = 0; i < count; i++) {
		Entity* e = addEntity(em, list + i);
		addedList.push(e);
	}

	return addedList;
}

void removeEntity(EntityManager* em, int entityId) {
	// Free memory.
	Entity* e = getEntity(em, entityId);
	if(e) {
		if(e->type == ET_ParticleEffect) {
			if(e->particleEmitter.particles.data) {
				e->particleEmitter.particles.free();
			}
		}
	}

	int index = em->indices[entityId-1];
	int movedId = em->entities.last().id;
	em->indices[movedId-1] = index;
	em->entities.remove(index);
	em->indices[entityId-1] = -1;
}

void removeEntity(EntityManager* em, Entity* e) {
	removeEntity(em, e->id);
}

bool entityCheck(Entity* e, int type, char* name) {
	return e->type == type && (e->name ? strCompare(e->name, name) : false);
}

Entity copyEntity(Entity* e) {
	Entity ce = *e;
	switch(e->type) {
		case ET_ParticleEffect: {
			ce.particleEmitter.particles = {};
		}
	}
	return ce;
}

DArray<Entity*>* getGroupMembers(EntityManager* em, int groupId) {
	DArray<Entity*>* groups = em->byType + ET_Group;
	for(int i = 0; i < groups->count; i++) {
		if(groups->data[i]->id == groupId) {
			return em->groupMembers + i;
		}
	}

	return 0;
}

void entityXFormToLocal(EntityManager* em, Entity* e) {
	Entity* mountEntity = getEntity(em,e->mountParentId);
	e->xfMount = inverse(mountEntity->xf)*e->xf;
}

void collectByType(EntityManager* em) {
	for(int i = 0; i < ET_Size; i++) em->byType[i].init(getTMemory);
	for(int i = 0; i < em->entities.count; i++) {
		Entity* e = em->entities.data + i;
		if(!entityIsValid(e)) continue;

		em->byType[e->type].push(e);
	}

	int groupCount = em->byType[ET_Group].count;
	em->groupMembers = getTArray(DArray<Entity*>, groupCount);
	for(int i = 0; i < groupCount; i++) em->groupMembers[i] = dArray<Entity*>(getTMemory);

	DArray<Entity*>* groups = em->byType + ET_Group;
	for(int i = 0; i < em->entities.count; i++) {
		Entity* e = em->entities.data + i;
		if(!entityIsValid(e)) continue;

		int groupIndex = 0;
		if(e->groupId) {
			for(int i = 0; i < groupCount; i++) {
				if(groups->data[i]->id == e->groupId) {
					em->groupMembers[i].push(e);
				}
			}
		}
	}

	// Sort groups by group z.
	auto cmp = [](const void* a, const void* b) -> int { 
		return (*((Entity**)a))->groupZ < (*((Entity**)b))->groupZ ? -1 : 1;
	};
	for(int i = 0; i < em->byType[ET_Group].count; i++) {
		Entity* group = em->byType[ET_Group].data[i];

		if(group->handlesParticles) {
			DArray<Entity*>* groupList = em->groupMembers + i;
			qsort(groupList->data, groupList->count, sizeof(Entity*), cmp);
		}
	}
}

//

void entityMouseLook(Entity* e, Input* input, float mouseSensitivity, bool clampLegs = false) {
	float turnRate = mouseSensitivity * 0.01f;

	e->camRot.y -= turnRate * input->mouseDelta.y;
	e->camRot.x -= turnRate * input->mouseDelta.x;

	float margin = 0.05f;
	float clampBottom = clampLegs ? -1.0 : (float)-M_PI_2+margin;
	clamp(&e->camRot.y, clampBottom, (float)M_PI_2-margin);
	e->camRot.x = modf(e->camRot.x, (float)M_PI*2);

	// // Swapping roll and pitch because of handedness.
	// float pitch, roll, yaw;
	// quatToEulerAngles(e->xf.rot, &roll, &pitch, &yaw);

	// Vec2 md = input->mouseDelta * turnRate;

	// yaw   -= md.x;
	// pitch -= md.y;

	// float margin = 0.05f;
	// clamp(&pitch, (float)-M_PI_2+margin, (float)M_PI_2-margin);

	// e->xf.rot = eulerAnglesToQuat(roll, pitch, yaw);
}

Vec2 entityKeyboardControl(Entity* e, Input* input, Vec2 rot) {
	Vec3 up = vec3(0,0,1);
	Camera cam = getCamData(e->xf.trans, rot);
	cam.look = cross(up, cam.right);

	Vec3 dir = vec3(0,0,0);
	if(input->keysDown[KEYCODE_W]) dir +=  norm(cam.look);
	if(input->keysDown[KEYCODE_S]) dir += -norm(cam.look);
	if(input->keysDown[KEYCODE_D]) dir +=  norm(cam.right);
	if(input->keysDown[KEYCODE_A]) dir += -norm(cam.right);

	if(dir.xy != vec2(0,0)) dir.xy = norm(dir.xy);

	return dir.xy;
}

void entityKeyboardAcceleration(Entity* e, Input* input, Vec2 rot, float speed, float boost, bool freeForm) {
	Vec3 up = vec3(0,0,1);
	Camera cam = getCamData(e->xf.trans, rot);

	bool rightLock = !freeForm || input->keysDown[KEYCODE_CTRL];
	if(rightLock) cam.look = cross(up, cam.right);

	Vec3 acceleration = vec3(0,0,0);
	if(input->keysDown[KEYCODE_SHIFT]) speed *= boost;
	if(input->keysDown[KEYCODE_CTRL])  speed /= boost * 5;
	if(input->keysDown[KEYCODE_W]) acceleration +=  norm(cam.look);
	if(input->keysDown[KEYCODE_S]) acceleration += -norm(cam.look);
	if(input->keysDown[KEYCODE_D]) acceleration +=  norm(cam.right);
	if(input->keysDown[KEYCODE_A]) acceleration += -norm(cam.right);

	if(freeForm) {
		if(input->keysDown[KEYCODE_E]) acceleration +=  norm(up);
		if(input->keysDown[KEYCODE_Q]) acceleration += -norm(up);
	}

	if(acceleration != vec3(0,0,0)) {
		e->acc += norm(acceleration)*speed;
	}
}

//

Vec3 mouseRayCast(Rect tr, Vec2 mp, Camera* cam, float nearX) {
	Vec2 mousePercent = {};
	mousePercent.x = mapRange01(mp.x, tr.left, tr.right);
	mousePercent.y = mapRange01(mp.y, tr.bottom, tr.top);

	Vec3 camBottomLeft = cam->pos + cam->look*nearX + (-cam->right)*(cam->dim.w/2.0f) + (-cam->up)*(cam->dim.h/2.0f);

	Vec3 p = camBottomLeft;
	p += (cam->right*cam->dim.w) * mousePercent.x;
	p += (cam->up*cam->dim.h) * mousePercent.y;

	Vec3 rayDir = norm(p - cam->pos);

	return rayDir;
}

Rect3 sceneBoundingBox(EntityManager* em) {
	Rect3 aabb = rect3(vec3(FLT_MAX), vec3(-FLT_MAX));

	for(auto& e : em->entities) {
		if(!entityIsValid(&e)) continue;
		if(e.aabb == rect3(vec3(0.0f), vec3(0.0f))) continue;

		aabb.min = min(aabb.min, e.aabb.min);
		aabb.max = max(aabb.max, e.aabb.max);
	}

	return aabb;
}

//

void updateEntities(EntityManager* em, Input* input, float dt, bool freeCam, MouseEvents mouseEvents, float mouseSensitivity, EntityUI* eui, Entity* playerCam, bool levelEdit = false) {
	TIMER_BLOCK();

	dxSetShader(Shader_Primitive);

	for(int i = 0; i < em->entities.count; i++) {
		Entity* e = em->entities.data + i;
		if(!entityIsValid(e)) continue;

		if(e->mountParentId) {
			if(!(eui->selectedObjects.find(e->id) && (eui->selectionState == ENTITYUI_ACTIVE))) {
				Entity* ep = getEntity(em, e->mountParentId);
				e->xf = ep->xf * e->xfMount;
			}
		}

		e->modelInv = modelMatrixInv(e->xf);

		// Calc bounding box.
		{
			if(!strLen(e->mesh) || !dxGetMesh(e->mesh)) 
				e->aabb = {};
			else {
				Rect3 aabb;
				Mesh* mesh = dxGetMesh(e->mesh);
				if(!mesh->animPlayer.init) {
					e->aabb = transformBoundingBox(mesh->aabb, modelMatrix(e->xf));

				} else {
					Rect3 aabb = rect3(vec3(FLT_MAX), vec3(-FLT_MAX));
					AnimationPlayer* player = &mesh->animPlayer;
					for(int i = 0; i < mesh->boneCount; i++) {
						Mat4 mat = modelMatrix(e->xf) * player->mats[i];
						Rect3 bbr = transformBoundingBox(mesh->boneBoundingBoxes[i], mat);

						aabb.min = min(aabb.min, bbr.min);
						aabb.max = max(aabb.max, bbr.max);
					}

					e->aabb = aabb;
				}
			}
		}

		if(levelEdit) {
			if((e->type != ET_ParticleEffect) &&
			   (e->type != ET_Camera) &&
			  !(e->type == ET_Group && e->handlesParticles)) continue;
		}

		if(e->groupId) continue;

		switch(e->type) {

			case ET_Player: {
				Vec2 dir = entityKeyboardControl(e, input, playerCam->camRot);

				if(freeCam) continue;
			} break;

			case ET_Object: {

			} break;

			case ET_ParticleEffect: {
				// e->particleEmitter.update(dt, e->xf, &theGState->activeCam);
				e->particleEmitter.markedForUpdate = true;
			} break;

			case ET_Sound: {
				Track* track = theAudioState->tracks + e->trackIndex;

				if(!track->used) {
					e->deleted = true;
					break;
				}

				track->isSpatial = true;
				track->pos = e->xf.trans;
			} break;

			case ET_Group: {
				if(e->handlesParticles) {
					DArray<Entity*>* list = getGroupMembers(em, e->id);
					if(!list) break;

					// Temporarily transform from local space to world space.
					// XForm oldForms[10];
					// for(int i = 0; i < emitterCount; i++) {
					// 	oldForms[i] = emitters[i].xForm;

					// 	emitters[i].xForm = xFormCombine(xForm, emitters[i].xForm);
					// }
					// defer {
					// 	for(int i = 0; i < emitterCount; i++) emitters[i].xForm = oldForms[i];
					// };

					int finishedCount = 0;
					for(int i = 0; i < list->count; i++) {
						Entity* e = list->data[i];
						if(e->particleEmitter.finished) finishedCount++; 
					}
					bool everyoneFinished = finishedCount == list->count;

					for(int i = 0; i < list->count; i++) {
						Entity* e = list->data[i];
						if(!e->particleEmitter.finished || everyoneFinished) 
							// e->particleEmitter.update(dt, e->xf, &theGState->activeCam);
							e->particleEmitter.markedForUpdate = true;

					}
				}
			} break;
		}
	}
}

//

float raycastEntity(Vec3 rayPos, Vec3 rayDir, Entity* e, Mesh* mesh) {
	AnimationPlayer* player = &mesh->animPlayer;

	float dist;
	if(!player->init) {
		dist = lineBoxIntersection(rayPos, rayDir, e->aabb.c(), e->aabb.dim());

	} else {
		for(int i = 0; i < mesh->boneCount; i++) {
			Rect3 bb = mesh->boneBoundingBoxes[i];
			XForm bbxf = xForm(bb.c(), bb.dim());
			XForm bbBone = player->xforms[i];
			Mat4 mat = player->mats[i];

			bbxf = player->xforms[i] * bbxf;
			bbxf = e->xf * bbxf;

			dist = boxRaycastRotated(rayPos, rayDir, bbxf);
			if(dist != -1) break;
		}
	}

	if(dist == -1) return -1;

	float shortestDistance = FLT_MAX;
	Vec3 intersectionPos;

	{
		DArray<MeshVertex> verts = mesh->vertices;

		Vec3 rayPosT = (e->modelInv * vec4(rayPos,1)).xyz;
		Vec3 rayDirT = (e->modelInv * vec4(rayDir,0)).xyz;

		for(int i = 0; i < verts.count/3; i++) {
			Vec3 ps[3];

			for(int j = 0; j < 3; j++) {
				MeshVertex* v = mesh->vertices.data + (i*3 + j);
				Vec3 p;

				if(player->init) {
					p = {};
					for(int k = 0; k < 4; k++) {
						float weight = v->blendWeights.e[k];
						if(weight == 0.0f) break;

						int index = v->blendIndices[k];
						p += (weight * (player->mats[index] * vec4(v->pos, 1))).xyz;
					}
				} else p = v->pos;

				ps[j] = p;
			}

			Vec3 intersection;
			bool result = lineTriangleIntersection(rayPosT, rayDirT, ps[0], ps[1], ps[2], &intersection, false);
			if(result) {
				float dist = len(intersection - rayPosT);
				if(dist < shortestDistance) {
					shortestDistance = dist;
					intersectionPos = intersection;
				}
			}
		}
	}

	if(shortestDistance == FLT_MAX) return -1;

	intersectionPos = e->xf * intersectionPos;
	shortestDistance = len(intersectionPos - rayPos);

	return shortestDistance;
}