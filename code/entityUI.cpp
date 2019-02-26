
// #define HistoryDebugStrings

void historyChange(int type, HistoryData* hd, DArray<Entity>* list, DArray<int>* selected) {
	#ifdef HistoryDebugStrings
	printf("%*s%i Edit\n", hd->index, "", hd->index);
	#endif

	// Reset buffers to index position;
	hd->offsets.count = hd->index;
	hd->buffer.count = hd->index == 0 ? 0 : hd->offsets[hd->index-1];

	int* dType = (int*)hd->buffer.retrieve(sizeof(int));
	*dType = type;

	int totalSize;
	if(type == COMMAND_TYPE_EDIT) {
		int count = selected->count;
		totalSize = sizeof(int) + sizeof(Entity)*count*2;

		char* d = hd->buffer.retrieve(totalSize);
		int* dCount = (int*)d; d += sizeof(int);
		Entity* dObjects = (Entity*)d;

		*dCount = count;
		for(int i = 0; i < count; i++) {
			Entity objPre = hd->objectsPreMod[i];
			objPre.id = selected->at(i);
			dObjects[i*2 + 0] = objPre;

			Entity objPost = hd->objectsPostMod[i];
			objPost.id = selected->at(i);
			dObjects[i*2 + 1] = objPost;
		}

	} else if(type == COMMAND_TYPE_INSERT || type == COMMAND_TYPE_REMOVE) {
		int count = list->count;
		totalSize = sizeof(int) + sizeof(Entity)*count;

		char* d = hd->buffer.retrieve(totalSize);
		int* dCount = (int*)d; d += sizeof(int);
		Entity* dObjects = (Entity*)d; d += sizeof(Entity) * count;

		*dCount = count;
		for(int i = 0; i < count; i++) {
			Entity obj = list->at(i);
			dObjects[i] = obj;
		}

	} else if(type == COMMAND_TYPE_SELECTION) {
		if(hd->index == 0) hd->previousSelection.clear();

		int pCount = hd->previousSelection.count;
		int count = selected->count;
		totalSize = sizeof(int) + sizeof(int)*pCount + sizeof(int) + sizeof(int)*count;

		char* d = hd->buffer.retrieve(totalSize);

		int* dPrevCount = (int*)d; d += sizeof(int);
		int* dPrevSelected = (int*)d; d += sizeof(int) * pCount;
		*dPrevCount = pCount;
		copyArray(dPrevSelected, hd->previousSelection.data, int, pCount);

		int* dCount = (int*)d; d += sizeof(int);
		int* dSelected = (int*)d; d += sizeof(int) * count;
		*dCount = count;
		copyArray(dSelected, selected->data, int, count);

		hd->previousSelection.clear();
		hd->previousSelection.push(selected);
	}

	int offsetSize = sizeof(int) + totalSize;
	int offsetBefore = hd->offsets.count == 0 ? 0 : hd->offsets.last();
	hd->offsets.push(offsetBefore + offsetSize);

	hd->index++;
}

void historyChange(HistoryData* hd, EntityManager* em, DArray<int>* selected, bool undo = true) {

	int currentIndex;
	if(undo) {
		if(hd->index == 0) return;

		currentIndex = hd->index-1;
		hd->index--;
	} else {
		if(hd->index == hd->offsets.count) return;

		currentIndex = hd->index;
		hd->index++;
	}

	int offset, size;
	if(currentIndex == 0) {
		offset = 0;
		size = hd->offsets[currentIndex];
	} else {
		offset = hd->offsets[currentIndex-1];
		size = hd->offsets[currentIndex] - offset;
	}

	char* d = hd->buffer.data + offset;
	int type = *((int*)d); d += sizeof(int);


	#ifdef HistoryDebugStrings
	char* typeString;
	switch(type) {
		case 0: typeString = "Edit"; break;
		case 1: typeString = "Insert"; break;
		case 2: typeString = "Remove"; break;
		case 3: typeString = "Selection"; break;
	}
	printf("%*s%i %s %s\n", hd->index, "", hd->index, undo?"Undo":"Redo", typeString);
	#endif

	switch(type) {
		case COMMAND_TYPE_EDIT: {
			int count = *((int*)d); d += sizeof(int);
			Entity* objs = (Entity*)d;

			for(int i = 0; i < count; i++) {
				Entity objPre  = objs[i*2 + 0];
				Entity objPost = objs[i*2 + 1];
				int index = objPre.id;

				if(undo) *getEntity(em, index) = objPre;
				else     *getEntity(em, index) = objPost;
			}
		} break;

		case COMMAND_TYPE_INSERT: {
			int count = *((int*)d); d += sizeof(int);
			Entity* objs = (Entity*)d;

			if(undo) {
				for(int i = 0; i < count; i++) 
					removeEntity(em, objs[i].id);
			} else {
				for(int i = 0; i < count; i++) 
					addEntity(em, objs + i);
			}
		} break;

		case COMMAND_TYPE_REMOVE: {
			int count = *((int*)d); d += sizeof(int);
			Entity* objs = (Entity*)d; d += sizeof(Entity)*count;

			if(undo) {
				// Insert at indexes where they got deleted.
				selected->clear();
				for(int i = 0; i < count; i++) {
					Entity obj = objs[i];
					addEntity(em, &obj, true);
					selected->push(obj.id);
				}

				hd->previousSelection.copy(selected);
			} else {

				// Code duplication with deleteObjects().
				for(int i = 0; i < selected->count; i++) {
					Entity* e = getEntity(em, selected->at(i));
					removeEntity(em, e);
				}

				selected->clear();
				hd->previousSelection.copy(selected);
			}
		} break;

		case COMMAND_TYPE_SELECTION: {
			int pCount = *((int*)d); d += sizeof(int);
			int* dPrevSelected = (int*)d; d += sizeof(int)*pCount;
			int count = *((int*)d); d += sizeof(int);
			int* dSelected = (int*)d; d += sizeof(int)*count;

			selected->clear();
			if(undo) selected->push(dPrevSelected, pCount);
			else     selected->push(dSelected, count);
			hd->previousSelection.copy(selected);

		} break;
	}
}

void copyObjects(EntityManager* em, DArray<Entity>* copies, DArray<int>* selected) {
	copies->clear();
	for(int i = 0; i < selected->count; i++) {
		Entity e = copyEntity(getEntity(em, selected->at(i)));
		copies->push(e);
	}
}

void deleteObjects(EntityManager* em, DArray<int>* selected, HistoryData* hd, bool switchSelected = true) {

	for(auto it : *selected) {
		Entity* group = getEntity(em, it);
		if(group->type != ET_Group) continue;

		DArray<Entity*>* members = getGroupMembers(em, group->id);
		if(!members) continue;

		for(auto member : *members) {
			if(!selected->find(member->id)) {
				selected->push(member->id);
			}
		}
	}

	// Push object removal to history.
	hd->tempObjects.clear();

	// Sort selected objects for undo remove.
	auto cmp = [](int* a, int* b) { return a < b; };
	mergeSort<int>(selected->data, selected->count, cmp);

	for(int i = 0; i < selected->count; i++) {
		Entity* obj = getEntity(em, selected->at(i));
		hd->tempObjects.push(*obj);
	}
	historyChange(COMMAND_TYPE_REMOVE, hd, &hd->tempObjects, 0);

	// Do the actual removal.
	for(int i = 0; i < selected->count; i++) {
		removeEntity(em, selected->at(i));
	}

	selected->clear();
	hd->previousSelection.clear();

	// Ignoring the switch for now.

	// if(selected->count > 1 || !switchSelected || objects->empty()) {
	// 	selected->clear();
	// } else if(!objects->empty()) {
	// 	selected->at(0) = mod(selected->at(0), objects->count);
	// }
}

void selectionChanged(EntityUI* eui) {
	eui->selectionAnimState = 0;
	eui->selectionChanged = true;
}

int insertObjects(EntityManager* em, EntityUI* eui, bool keepPosition = true) {
	DArray<Entity>* copies = &eui->objectCopies;
	HistoryData* hd = &eui->history;

	if(copies->empty()) return 0;

	if(copies->count == 1 && !keepPosition) {
		Camera* cam = &theGState->activeCam;
		float spawnDistance = cam->dim.w*10;
		copies->atr(0)->xf.trans = cam->pos + cam->look * spawnDistance;
	}

	bool hasGroups = false;
	for(int i = 0; i < copies->count; i++) {
		if(copies->at(i).type == ET_Group) {
			hasGroups = true;
			break;
		}
	}

	DArray<Entity>* cs = copies;

	if(hasGroups) {
		cs = &eui->objectTempArray;
		cs->clear();
		cs->copy(copies);

		for(int i = 0; i < cs->count; i++) {
			Entity* group = cs->data + i;
			if(group->type != ET_Group) continue;

			DArray<Entity*>* members = getGroupMembers(em, group->id);
			if(!members) continue;

			int newGroupId = em->entities.count + i+1;
			for(int mi = 0; mi < members->count; mi++) {
				cs->push(members->at(mi));
				Entity* p = cs->data + cs->count-1;
				p->groupId = newGroupId;

				// If mountParentId is the same as the group id change that as well.
				if(p->mountParentId == group->id) {
					p->mountParentId = newGroupId;
				}
			}
		}
	}

	DArray<Entity> addedList = addEntities(em, cs->data, cs->count);

	historyChange(COMMAND_TYPE_INSERT, hd, &addedList, 0);

	return addedList.at(0).id;
}

bool isObjectSelected(EntityUI* eui, int index) {
	bool selected = false;
	for(int i = 0; i < eui->selectedObjects.count; i++) {
		if(index == eui->selectedObjects[i]) {
			selected = true;
			break;
		}
	}

	return selected;
}

Vec3 selectedObjectsGetCenter(EntityManager* em, DArray<int>* selected) {
	if(selected->count == 1) return getEntity(em, selected->first())->xf.trans;

	Vec3 pMin = vec3(FLT_MAX);
	Vec3 pMax = vec3(-FLT_MAX);
	for(int i = 0; i < selected->count; i++) {
		Entity* obj = getEntity(em, selected->at(i));

		pMin.x = min(pMin.x, obj->xf.trans.x);
		pMin.y = min(pMin.y, obj->xf.trans.y);
		pMin.z = min(pMin.z, obj->xf.trans.z);
		pMax.x = max(pMax.x, obj->xf.trans.x);
		pMax.y = max(pMax.y, obj->xf.trans.y);
		pMax.z = max(pMax.z, obj->xf.trans.z);
	}
	Vec3 result = pMin + (pMax - pMin)/2;

	return result;
}

void pushSelectedObjectsPreMod(EntityManager* em, EntityUI* eui) {
	eui->history.objectsPreMod.clear();
	for(auto it : eui->selectedObjects) {
		Entity* e = getEntity(em, it);
		eui->history.objectsPreMod.push(*e);
	}
}

void updateEntityUI(DebugState* ds, EntityManager* em) {
	Input* input = ds->input;
	Gui* gui = &ds->gui;
	EntityUI* eui = &ds->entityUI;
	Camera* cam = &theGState->activeCam;

	// Panel fade animation.
	{
		bool inc = eui->selectionState == ENTITYUI_ACTIVE && ds->mouseOverPanel;

		float speed = 1.0f;

		if(!inc) speed *= -2.5f;
		ds->panelAlphaFadeState += ds->dt * speed;

		ds->panelAlphaFadeState = clamp(ds->panelAlphaFadeState, 1-ds->guiAlphaMax, 0.8f);
		ds->guiAlpha = 1-pow(ds->panelAlphaFadeState,2);
	}

	if(mouseWheel(gui, input)) { 
		if(eui->selectionState != ENTITYUI_ACTIVE) {
			eui->selectionMode = mod(eui->selectionMode - mouseWheel(gui, input), ENTITYUI_MODE_SCALE+1);
		} else {
			if(eui->selectionMode == ENTITYUI_MODE_TRANSLATION && eui->transformMode == TRANSFORM_MODE_CENTER) {
				float wheelObjectCenterSpeed = 0.4f;
				eui->centerDistanceToCam += mouseWheel(gui, input) * cam->dim.w*wheelObjectCenterSpeed;
			}
		}
	}

	if(eui->selectedObjects.count > 1) {
		if(eui->selectionMode == ENTITYUI_MODE_ROTATION || 
		   eui->selectionMode == ENTITYUI_MODE_SCALE) {
			eui->selectionMode = ENTITYUI_MODE_TRANSLATION;
		}
	}

	if(input->keysDown[KEYCODE_SHIFT]) {
		eui->snappingEnabled = true;
	} else {
		eui->snappingEnabled = false;
	}

	float animSpeed = 3;
	eui->selectionAnimState += ds->dt * animSpeed;

	eui->multipleSelectionMode = input->keysDown[KEYCODE_CTRL];

	if(mouseButtonPressedMiddle(gui, input)) {
		eui->dragSelectionStart = input->mousePosNegative;
		eui->dragSelectionActive = true;
		eui->selectionMode = ENTITYUI_MODE_DRAG;
		eui->selectionState = ENTITYUI_ACTIVE;
	}

	if(eui->selectionMode == ENTITYUI_MODE_DRAG && 
	   mouseButtonReleasedMiddle(gui, input)) {
		eui->dragSelectionActive = false;
		eui->selectionChanged = true;
		eui->selectionState = ENTITYUI_INACTIVE;
		eui->selectionMode = ENTITYUI_MODE_TRANSLATION;
	}

	if(eui->selectedObjects.count == 0 && eui->selectionState == ENTITYUI_HOT) {
		eui->selectionState = ENTITYUI_INACTIVE;
	}

	// @Select.
	if(mouseButtonPressedLeft(gui, input) && eui->selectionState == ENTITYUI_INACTIVE) {
		Vec3 rayDir = mouseRayCast(theGState->screenRect, input->mousePosNegative, cam, theGState->gSettings->nearPlane);

		int objectIndex = -1;
		float shortestDistance = FLT_MAX;
		for(int entityIndex = 0; entityIndex < em->entities.count; entityIndex++) {
			Entity* e = em->entities.data + entityIndex;
			if(!entityIsValid(e)) continue;

			Mesh* mesh = dxGetMesh(e->mesh);
			if(!mesh) continue;

			XForm xf = e->xf;
			if(e->type == ET_ParticleEffect && !ds->drawParticleHandles) continue;
			if(e->type == ET_Group && !ds->drawGroupHandles) continue;

			if(e->type == ET_ParticleEffect || e->type == ET_Group) xf.scale = vec3(1);

			float dist = raycastMesh(cam->pos, rayDir, mesh, xf);
			if(dist != -1) {
				if(dist < shortestDistance) {
					shortestDistance = dist;
					objectIndex = e->id;
				}
			}
		}

		if(objectIndex != -1) {
			if(eui->multipleSelectionMode) {
				if(isObjectSelected(eui, objectIndex)) {
					// Deselect.
					int i = eui->selectedObjects.find(objectIndex)-1;
					eui->selectedObjects.remove(i);
				} else {
					// Select.
					eui->selectedObjects.push(objectIndex);
					eui->selectionAnimState = 0;
				}
			} else {
				eui->selectedObjects.clear();
				eui->selectedObjects.push(objectIndex);
				eui->selectionAnimState = 0;
			}

			eui->selectionChanged = true;
		} else {
			if(!eui->multipleSelectionMode) {
				eui->selectedObjects.clear();
				eui->selectionState = ENTITYUI_INACTIVE;
				eui->selectionChanged = true;
			}
		}
	}

	// // Select all.
	// if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_A)) {
	// 	eui->selectedObjects.clear();
	// 	for(int i = 0; i < objects->count; i++) {
	// 		eui->selectedObjects.push(i);
	// 	}

	// 	eui->selectionState = ENTITYUI_INACTIVE;
	// 	eui->selectionChanged = true;
	// }

	// if(keyPressed(gui, input, KEYCODE_ESCAPE) && eui->selectedObjects.count && ad->drawSceneWired) {
	// 	eui->selectedObjects.clear();
	// 	eui->selectionState = ENTITYUI_INACTIVE;
	// 	eui->selectionChanged = true;

	// 	// Hack.
	// 	gui->popupStackCount = 0;
	// }

	if(eui->selectionChanged) {
		// Compare current selection with previous and only push when different.
		if(eui->selectedObjects != eui->history.previousSelection)
			historyChange(COMMAND_TYPE_SELECTION, &eui->history, 0, &eui->selectedObjects);

		eui->selectionChanged = false;
	}

	bool paste = false;
	if(!guiPopupOpen(gui)) {
		if(eui->selectedObjects.count) {
			// Delete.
			if(keyPressed(gui, input, KEYCODE_DEL) && eui->selectionState != ENTITYUI_ACTIVE) {
				deleteObjects(em, &eui->selectedObjects, &eui->history, false);
			}

			// Copy.
			if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_C)) {
				copyObjects(em, &eui->objectCopies, &eui->selectedObjects);
			}

			// Cut.
			if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_X) && eui->selectionState != ENTITYUI_ACTIVE) {
				// Copy and delete.
				copyObjects(em, &eui->objectCopies, &eui->selectedObjects);
				deleteObjects(em, &eui->selectedObjects, &eui->history, false);
			}
		}

		// Paste.
		if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_V) && 
		   !guiPopupOpen(gui)) {
			paste = true;
			eui->objectsEdited = true;
		}
	}

	// @GroupHotkey.
	// If one group plus others are selected then put all entities in that group,
	// else create new group for all entities.
	if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_G)) {
		Entity* group = 0;
		for(auto it : eui->selectedObjects) {
			Entity* e = getEntity(em, it);
			if(e->type == ET_Group) {
				if(group) {
					group = 0;
					break;
				}
				group = e;
			}
		}

		Entity newGroup;
		if(!group) {
			// Create new group.
			Vec3 center = selectedObjectsGetCenter(em, &eui->selectedObjects);
			newGroup = getDefaultEntity(ET_Group, center);

			eui->objectCopies.clear();
			eui->objectCopies.push(newGroup);
			int groupId = insertObjects(em, eui, true);

			newGroup.id = groupId;
			group = &newGroup;
		}

		pushSelectedObjectsPreMod(em, eui);
		eui->objectNoticeableChange = true;

		for(auto it : eui->selectedObjects) {
			Entity* e = getEntity(em, it);
			if(e->type == ET_Group) continue;

			e->groupId = group->id;
			e->mountParentId = group->id;
			e->xfMount = inverse(group->xf) * e->xf;
		}

		eui->objectsEdited = true;
	}

	if(keyPressed(gui, input, KEYCODE_R)) eui->localMode = !eui->localMode;

	if(keyPressed(gui, input, KEYCODE_1)) eui->selectionMode = ENTITYUI_MODE_TRANSLATION;
	if(keyPressed(gui, input, KEYCODE_2)) eui->selectionMode = ENTITYUI_MODE_ROTATION;
	if(keyPressed(gui, input, KEYCODE_3)) eui->selectionMode = ENTITYUI_MODE_SCALE;

	if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_Y) && eui->selectionState != ENTITYUI_ACTIVE) {
		historyChange(&eui->history, em, &eui->selectedObjects);
	}

	if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_Z) && eui->selectionState != ENTITYUI_ACTIVE) {
		historyChange(&eui->history, em, &eui->selectedObjects, false);
	}

	// if((input->mouseButtonReleased[0] || paste) && eui->selectionState == ENTITYUI_ACTIVE) {
	if((input->mouseButtonReleased[0]) && eui->selectionState == ENTITYUI_ACTIVE) {
		eui->objectsEdited = true;
	}

	// // if((input->mouseButtonReleased[0] || paste) && eui->selectionState == ENTITYUI_ACTIVE) {
	if(eui->objectsEdited) {
		eui->objectsEdited = false;

		for(auto id : eui->selectedObjects) {
			Entity* e = getEntity(em, id);

			if(e->mountParentId) {
				Entity* parentEntity = getEntity(em, e->mountParentId);
				if(eui->selectionMode == ENTITYUI_MODE_TRANSLATION)
					e->xfMount.trans = (inverse(parentEntity->xf) * e->xf).trans;
				else if(eui->selectionMode == ENTITYUI_MODE_ROTATION)
					e->xfMount.rot = (inverse(parentEntity->xf) * e->xf).rot;
				else if(eui->selectionMode == ENTITYUI_MODE_SCALE)
					e->xfMount.scale = (inverse(parentEntity->xf) * e->xf).scale;
			}
		}

		if(!paste) {
			eui->selectionState = ENTITYUI_INACTIVE;
		}
		if(eui->objectNoticeableChange) {
			eui->history.objectsPostMod.clear();
			for(int i = 0; i < eui->history.objectsPreMod.count; i++) {
				Entity* objPostMod = getEntity(em, eui->selectedObjects[i]);
				// eui->history.objectsPreMod[i] = entitySub(eui->history.objectsPreMod[i], objAfterMod);

				eui->history.objectsPostMod.push(*objPostMod);
			}

			historyChange(COMMAND_TYPE_EDIT, &eui->history, 0, &eui->selectedObjects);
		}

		eui->objectNoticeableChange = false;
	}

	if(paste) {
		// Copy paste if active.
		if(eui->selectionState == ENTITYUI_ACTIVE) {
			copyObjects(em, &eui->objectCopies, &eui->selectedObjects);
		}

		insertObjects(em, eui, true);

		// Reset edit state. A paste should be an edit + paste.
		if(paste && eui->selectionState == ENTITYUI_ACTIVE) {

			// Code duplication with entityui code.

			eui->history.objectsPreMod.clear();
			for(int i = 0; i < eui->selectedObjects.count; i++) {
				Entity* obj = getEntity(em, eui->selectedObjects[i]);
				eui->history.objectsPreMod.push(obj);
			}

			Entity* obj = getEntity(em, eui->selectedObjects[0]);

			if(eui->selectionMode == ENTITYUI_MODE_TRANSLATION) {
				eui->startPos = eui->currentPos;
			} else if(eui->selectionMode == ENTITYUI_MODE_ROTATION) {
				eui->startRot = obj->xf.rot;
				eui->objectDistanceVector = eui->currentObjectDistanceVector;
			} else if(eui->selectionMode == ENTITYUI_MODE_SCALE) {
				eui->startDim = obj->xf.scale;
				eui->objectDistanceVector = eui->currentObjectDistanceVector;
			}
		}
	}

	if(eui->selectionState != ENTITYUI_ACTIVE) {
		eui->enableScaleEqually = keyDown(gui, input, KEYCODE_CTRL);
	}

	// @UpdateWidgets

	eui->gotActive = false;

	if(mouseButtonPressedLeft(gui, input) && eui->selectionState == ENTITYUI_HOT) {
		eui->selectionState = ENTITYUI_ACTIVE;
		eui->gotActive = true;
	}

	if(eui->selectionState == ENTITYUI_ACTIVE && eui->selectionMode != ENTITYUI_MODE_DRAG) {

		Vec3 rayPos = cam->pos;
		Vec3 rayDir = mouseRayCast(theGState->screenRect, input->mousePosNegative, cam, theGState->gSettings->nearPlane);

		if(eui->gotActive) {
			pushSelectedObjectsPreMod(em, eui);
		}

		if(eui->selectionMode == ENTITYUI_MODE_TRANSLATION) {
			Vec3 pos = selectedObjectsGetCenter(em, &eui->selectedObjects);

			// Get offset from all selected objects to center.
			if(eui->gotActive) {
				eui->objectCenterOffsets.clear();

				for(int i = 0; i < eui->selectedObjects.count; i++) {
					Entity* obj = getEntity(em, eui->selectedObjects[i]);
					Vec3 offset = obj->xf.trans - pos;

					eui->objectCenterOffsets.push(offset);
				}
			}

			if(eui->transformMode == TRANSFORM_MODE_AXIS) {
				Vec3 cameraOnAxis = projectPointOnLine(pos, eui->axis, cam->pos);
				Vec3 planeNormal = norm(cam->pos - cameraOnAxis);

				Vec3 planeIntersection;
				float distance = linePlaneIntersection(rayPos, rayDir, pos, planeNormal, &planeIntersection);
				if(distance != -1) {
					Vec3 linePointOnAxis = projectPointOnLine(pos, eui->axis, planeIntersection);

					if(eui->gotActive) {
						eui->objectDistanceVector = pos - linePointOnAxis;
						eui->startPos = pos;
					}

					pos = linePointOnAxis + eui->objectDistanceVector;
				}

			} else if(eui->transformMode == TRANSFORM_MODE_PLANE) {
				Vec3 planeIntersection;
				float distance = linePlaneIntersection(rayPos, rayDir, pos, eui->axis, &planeIntersection);
				if(distance != -1) {

					if(eui->gotActive) {
						eui->objectDistanceVector = pos - planeIntersection;
						eui->startPos = pos;
					}

					pos = planeIntersection + eui->objectDistanceVector;
				}
				
			} else if(eui->transformMode == TRANSFORM_MODE_CENTER) {
				Vec3 pp = cam->pos + cam->look * eui->centerDistanceToCam + eui->centerOffset;

				Vec3 planeIntersection;
				float distance = linePlaneIntersection(rayPos, rayDir, pp, -cam->look, &planeIntersection);
				if(distance != -1) {

					if(eui->gotActive) {
						eui->objectDistanceVector = eui->centerOffset;
						eui->startPos = pos;
					}

					pos = planeIntersection - eui->objectDistanceVector;
				}
			}

			if(eui->snappingEnabled) {
				// We always snap every axis to keep it simple.
				for(int i = 0; i < 3; i++) {
					Vec3 p = projectPointOnLine(eui->startPos, eui->axes[i], pos);
					float length = len(p - eui->startPos);
					float snappedLength = roundMod(length, roundf(eui->snapGridSize));
					float lengthDiff = length - snappedLength;

					if(dot(p - eui->startPos, eui->axes[i]) > 0) lengthDiff *= -1;

					pos += eui->axes[i]*lengthDiff;
				}
			}

			for(int i = 0; i < eui->selectedObjects.count; i++) {
				Entity* obj = getEntity(em, eui->selectedObjects[i]);
				obj->xf.trans = pos + eui->objectCenterOffsets[i];
			}

			eui->currentPos = pos;
			eui->objectNoticeableChange = len(eui->startPos - pos) > 0.00001f;

		} else {
			Entity* obj = getEntity(em, eui->selectedObjects.first());
			Vec3 pos = obj->xf.trans;

			if(eui->selectionMode == ENTITYUI_MODE_ROTATION) {
				Vec3 intersection;
				if(linePlaneIntersection(cam->pos, rayDir, pos, eui->axis, &intersection) != -1) {
					if(eui->gotActive) {
						eui->startRot = obj->xf.rot;
						eui->objectDistanceVector = norm(intersection - pos);
					}

					Vec3 start = eui->objectDistanceVector;
					Vec3 end = norm(intersection - pos);

					Vec3 a = cross(start, end);
					float length = len(a);
					if(dot(norm(a), eui->axis) < 0) length *= -1;
					float angle = asin(length);
					if(dot(start, end) < 0) {
						if(angle > 0) angle = M_PI_2 + M_PI_2-angle;
						else angle = -M_PI_2 - (M_PI_2-abs(angle));
					}

					if(eui->snappingEnabled) angle = roundMod(angle, M_PI_4);

					obj->xf.rot = quat(angle, eui->axis)*eui->startRot;

					eui->currentRotationAngle = angle;
					eui->objectNoticeableChange = eui->currentRotationAngle != 0;
				}
			}

			if(eui->selectionMode == ENTITYUI_MODE_SCALE) {
				float smallestScale = 0.001f;

				if(eui->transformMode == TRANSFORM_MODE_AXIS) {
					Vec3 cameraOnAxis = projectPointOnLine(pos, eui->axis, cam->pos);
					Vec3 planeNormal = norm(cam->pos - cameraOnAxis);

					Vec3 planeIntersection;
					if(linePlaneIntersection(rayPos, rayDir, pos, planeNormal, &planeIntersection) != -1) {
						Vec3 linePointOnAxis = projectPointOnLine(pos, eui->axis, planeIntersection);

						if(eui->gotActive) {
							eui->objectDistanceVector = pos - linePointOnAxis;
							eui->startDim = obj->xf.scale;
						}

						float scalePercent = (len(pos - linePointOnAxis) / len(eui->objectDistanceVector));
						if(dot(pos - linePointOnAxis, eui->axis) > 0) scalePercent *= -1;

						if(eui->enableScaleEqually) {
							for(int i = 0; i < 3; i++) obj->xf.scale.e[i] = scalePercent * eui->startDim.e[i];
							eui->currentScalePercent = scalePercent;

						} else {
							obj->xf.scale.e[eui->axisIndex-1] = max(scalePercent * eui->startDim.e[eui->axisIndex-1], smallestScale);
							eui->currentScalePercent = max(scalePercent,0.0f);
						}
					}

				} else if(eui->transformMode == TRANSFORM_MODE_PLANE) {
					Vec3 planeIntersection;
					if(linePlaneIntersection(rayPos, rayDir, pos, eui->axis, &planeIntersection) != -1) {
						int i = eui->axisIndex-1;
						Vec3 axis = eui->axes[(i+1)%3] + eui->axes[(i+2)%3];

						Vec3 linePointOnAxis = projectPointOnLine(pos, axis, planeIntersection);

						if(eui->gotActive) {
							eui->objectDistanceVector = pos - linePointOnAxis;
							eui->startDim = obj->xf.scale;
						}

						float scalePercent = (len(pos - linePointOnAxis) / len(eui->objectDistanceVector));
						if(dot(pos - linePointOnAxis, axis) > 0) scalePercent *= -1;

						obj->xf.scale.e[(i+1)%3] = max(scalePercent * eui->startDim.e[(i+1)%3], smallestScale);
						obj->xf.scale.e[(i+2)%3] = max(scalePercent * eui->startDim.e[(i+2)%3], smallestScale);

						eui->currentScalePercent = max(scalePercent,0.0f);
					}

				} else if(eui->transformMode == TRANSFORM_MODE_CENTER) {
					Vec3 pp = cam->pos + cam->look * len(vectorToCam(pos, cam));

					Vec3 planeIntersection;
					if(linePlaneIntersection(rayPos, rayDir, pp, -cam->look, &planeIntersection) != -1) {
						Vec3 linePointOnAxis = projectPointOnLine(pos, cam->right, planeIntersection);

						if(eui->gotActive) {
							eui->objectDistanceVector = linePointOnAxis;
							eui->startDim = obj->xf.scale;
						}

						float length = len(linePointOnAxis - eui->objectDistanceVector);
						if(dot(linePointOnAxis - eui->objectDistanceVector, cam->right) < 0) length *= -1;

						obj->xf.scale = eui->startDim + eui->startDim*length*0.5f;
						eui->currentScalePercent = obj->xf.scale.x / eui->startDim.x;
					}
				}

				eui->objectNoticeableChange = obj->xf.scale != eui->startDim;
			}
		}
	}

	// Multiple selection.
	{
		Vec4 cDragOutline = vec4(0,1);
		Vec4 cDrag = vec4(0,0.8f,0.8f,0.2f);

		float nearPlane = theGState->gSettings->nearPlane;

		if(eui->dragSelectionActive) {
			Vec2 start = eui->dragSelectionStart;
			Vec2 end = input->mousePosNegative;

			float left = min(start.x, end.x);
			float right = max(start.x, end.x);
			float bottom = min(start.y, end.y);
			float top = max(start.y, end.y);

			Rect r = round(rect(left, bottom, right, top));
			dxDrawRect(r, cDrag);
			dxDrawRectOutline(r, cDragOutline);

			Rect sr = theGState->screenRect;
			Vec3 rayDirLeft = mouseRayCast(sr, r.l(), cam, nearPlane);
			Vec3 coneLeftDir = norm(cross(cam->up, rayDirLeft));

			Vec3 rayDirRight = mouseRayCast(sr, r.r(), cam, nearPlane);
			Vec3 coneRightDir = norm(-cross(cam->up, rayDirRight));

			Vec3 rayDirBottom = mouseRayCast(sr, r.b(), cam, nearPlane);
			Vec3 coneBottomDir = norm(-cross(cam->right, rayDirBottom));

			Vec3 rayDirTop = mouseRayCast(sr, r.t(), cam, nearPlane);
			Vec3 coneTopDir = norm(cross(cam->right, rayDirTop));

			if(!keyDown(gui, input, KEYCODE_CTRL)) eui->selectedObjects.clear();

			for(int i = 0; i < em->entities.count; i++) {
				Entity* obj = em->entities.data + i;
				if(!entityIsValid(obj)) continue;
				if(!obj->mesh || !dxGetMesh(obj->mesh)) continue;
				if(obj->type == ET_ParticleEffect && !ds->drawParticleHandles) continue;
				if(obj->type == ET_Group && !ds->drawGroupHandles) continue;

				Vec3 objDir = norm(obj->xf.trans - cam->pos);

				bool objectNotInCone = false;
				if(dot(objDir, coneLeftDir) > 0 || 
				   dot(objDir, coneRightDir) > 0 || 
				   dot(objDir, coneBottomDir) > 0 || 
				   dot(objDir, coneTopDir) > 0) {
					objectNotInCone = true;
				}

				if(!objectNotInCone && !eui->selectedObjects.find(obj->id)) 
					eui->selectedObjects.push(obj->id);
			}
		}
	}
	
}

void drawEntityUI(DebugState* ds, EntityManager* em, Vec2 mousePos, WindowSettings* ws, bool playerMode) {
	TIMER_BLOCK();

	EntityUI* eui = &ds->entityUI;
	GraphicsState* gs = theGState;
	Camera* cam = &gs->activeCam;

	{
		dxSetShader(Shader_Main);
		dxBindFrameBuffer("3dMsaa", "ds3d");

		MainShaderVars* vars = dxGetShaderVars(Main);
		vars->disableLighting = true;
		dxPushShaderConstants(Shader_Main);
		defer { 
			vars->disableLighting = false;
			dxPushShaderConstants(Shader_Main);
		};

		dxFillWireFrame(true);
		defer { dxFillWireFrame(false); };

		D3D11_RASTERIZER_DESC oldRasterizerState = gs->rasterizerState;
		{
			gs->rasterizerState.DepthBias = -1000;
			gs->rasterizerState.DepthBiasClamp = 0;
			gs->rasterizerState.SlopeScaledDepthBias = 0;
		}
		dxSetRasterizer(); 
		defer{
			gs->rasterizerState = oldRasterizerState;
			dxSetRasterizer(); 
		};

		Vec4 color = vec4(1,1,1,1);

		// Color fading animation on selection.
		{
			float percent = (cos(eui->selectionAnimState)+1)/2.0f;

			Vec3 faceColors[] = { vec3(0,0,0), vec3(1,0,1), vec3(1,1,1) };
			float timings[] = { 0.0f, 0.4f, 1.0f };

			int index = 0;
			for(int i = 0; i < arrayCount(timings)-1; i++) {
				if(between(percent, timings[i], timings[i+1])) index = i;
			}

			float l = mapRange(percent, timings[index], timings[index+1], 0.0f, 1.0f);
			Vec3 ce = lerp(l, faceColors[index], faceColors[index+1]);
			color = vec4(gammaToLinear(ce), 1);
		}

		for(int i = 0; i < eui->selectedObjects.count; i++) {
			Entity* e = getEntity(em, eui->selectedObjects[i]);

			if(e->type == ET_Player && playerMode) continue;

			XForm xf = e->xf;
			if(e->type == ET_ParticleEffect ||
			   e->type == ET_Group) {
				if(e->type == ET_ParticleEffect && !ds->drawParticleHandles) continue;
				if(e->type == ET_Group && !ds->drawGroupHandles) continue;

				xf.scale = vec3(1);
			}

			dxDrawObject(xf, color, e->mesh, "Empty\\material.mtl", false);
		}

		dxCullState(true);
		dxSetBlendState(Blend_State_Blend);
	}

	//

	{
		dxSetShader(Shader_Primitive);
		dxCullState(false);
		defer { dxCullState(true); };

		Entity* object = getEntity(em, eui->selectedObjects.first());
		Vec3 pos = object->xf.trans;
		Quat rot = object->xf.rot;
		Vec3 dim = object->xf.scale;

		bool multipleSelection = eui->selectedObjects.count > 1;
		if(multipleSelection) {
			pos = selectedObjectsGetCenter(em, &eui->selectedObjects);
		}

		float uiAlpha = 1.0f;

		Vec3 axisColor[3] = {vec3(1,0,0), vec3(0,1,0), vec3(0,0,1)};
		Vec3 cCenter = vec3(0,1,1);
		Vec3 cHot = vec3(1.00,0.83,0.58);
		Vec3 cActive = vec3(1,1,0);
		Vec3 cScaleRange = vec3(0.5f);
		Vec4 cRotationSegment = vec4(0,1,1,0.5f);
		Vec4 cTranslationVector = vec4(0,1,1,1);

		// Scale ui widgets with window size.
		float screenMod = (float)ws->biggestMonitorSize.h / (float)theGState->screenRes.h;
		screenMod = powf(screenMod, 0.7f);
		float distToCam = dot(cam->look, pos - cam->pos) * screenMod;

		float radius = distToCam * 0.15f;
		float tArrowWidth = distToCam * 0.005f;
		float tArrowActWidth = tArrowWidth * 3;
		float tVectorWidth = tArrowWidth*0.5f;
		float tPlaneSize = radius * 0.2f;
		float tCenterBoxSize = distToCam * 0.025f;
		float tArrowOff = tCenterBoxSize*1.2f;

		float rRingWidth = tArrowWidth;
		float rRingActWidth = tArrowActWidth;
		float ringTesselationError = -50;

		float sBoxDim = distToCam * 0.015f;
		float sArrowWidth = distToCam * 0.002f;
		float sPlaneSize = tPlaneSize;
		float sArrowOff = tArrowOff;

		Mat4 vm = gs->gMats.view;	

		//

		Vec3 rayDir = mouseRayCast(theGState->screenRect, mousePos, cam, gs->gSettings->nearPlane);
		Vec3 rayPos = cam->pos + rayDir * gs->gSettings->nearPlane;

		Vec3 vToCam = cam->pos - pos;
		Vec3 axis[3];
		for(int i = 0; i < 3; i++) {
			Vec3 a = vec3(0,0,0);
			a.e[i] = 1;

			if(eui->localMode || eui->selectionMode == ENTITYUI_MODE_SCALE) 
				a = rot * a;
			if(dot(a, vToCam) < 0) a *= -1;
			axis[i] = a;
		}

		int contenderId = 0;
		int widgetId = 0;

		bool active       = eui->selectionState == ENTITYUI_ACTIVE;
		bool axisActive   = active && eui->transformMode == TRANSFORM_MODE_AXIS;
		bool planeActive  = active && eui->transformMode == TRANSFORM_MODE_PLANE;
		bool centerActive = active && 
		                    (eui->transformMode == TRANSFORM_MODE_CENTER || 
		                    (eui->enableScaleEqually && eui->transformMode == TRANSFORM_MODE_AXIS));
		int currentAxis = eui->axisIndex-1;

		if(eui->selectionMode == ENTITYUI_MODE_TRANSLATION) {
			dxDepthTest(false);
			defer { dxDepthTest(true); };

			int axisIndex = 0;
			for(int i = 0; i < 3; i++) {
				widgetId++;

				float al = radius - tArrowOff;
				Vec2 pd = vec2(tArrowActWidth,al);
				Vec3 pp = pos + axis[i]*(radius - al*0.5f);
				Vec3 pn = norm(cross(cross(axis[i], vToCam), axis[i]));

				Vec3 c = axisColor[i];
				if(linePlaneIntersection(rayPos, rayDir, pp, pn, axis[i], pd) != -1) {
					axisIndex = i+1;
					contenderId = widgetId;
					if(!eui->guiHasFocus && eui->hotId == contenderId) c = cHot;
				}

				bool ignore = (axisActive && i != currentAxis) || (planeActive && i == currentAxis);
				if(!ignore) {
					if(axisActive || (planeActive && currentAxis != i) || centerActive) c = cActive;
					dxDrawArrow(pos + axis[i]*tArrowOff, pos + axis[i]*radius, vToCam, tArrowWidth, vec4(c, uiAlpha));
				}
			}

			int planeIndex = 0;
			for(int i = 0; i < 3; i++) {
				widgetId++;

				float l = radius*0.5f + tPlaneSize*0.5f;
				Vec3 p = pos + axis[(i+1)%3]*l + axis[(i+2)%3]*l;

				Vec3 c = vec3(1);
				if(linePlaneIntersection(rayPos, rayDir, p, axis[i], axis[(i+1)%3], vec2(tPlaneSize)) != -1) {
					planeIndex = i+1;
					contenderId = widgetId;
					if(!eui->guiHasFocus && eui->hotId == contenderId) c = cHot;
				}

				bool ignore = axisActive || (planeActive && currentAxis != i) || centerActive;
				if(!ignore) {
					if(planeActive) c = cActive;
					dxDrawPlane(p, axis[i], axis[(i+1)%3], vec2(tPlaneSize), vec4(c,uiAlpha), dxGetTexture("misc\\roundedSquare.dds")->view);
				}
			}

			int centerIndex = 0;
			{
				widgetId++;

				Vec3 c = cCenter;
				Vec3 intersection;
				if(linePlaneIntersection(rayPos, rayDir, pos, -cam->look, cam->up, vec2(tCenterBoxSize), &intersection) != -1) {
					eui->centerOffset = intersection - pos;
					centerIndex = 1;
					contenderId = widgetId;
					if(!eui->guiHasFocus && eui->hotId == contenderId) c = cHot;
				}

				if(centerActive) c = cActive;

				dxLineAA(0);
				defer { dxLineAA(1); };
				dxDrawPlaneOutline(pos, -cam->look, cam->up, vec2(tCenterBoxSize), vec4(c,uiAlpha));
			}

			// Translation vector.
			if(active) {
				// dxDepthTest(true);
				// defer { dxDepthTest(false); };

				dxDrawArrow(eui->startPos, pos, vToCam, tVectorWidth, cTranslationVector);

				{
					Vec3 globalAxis[] = {vec3(1,0,0), vec3(0,1,0), vec3(0,0,1)};
					float l[3];
					for(int i = 0; i < 3; i++) {
						if(dot(globalAxis[i], eui->startPos-pos) > 0) globalAxis[i] *= -1;

						l[i] = len(projectPointOnLine(eui->startPos, globalAxis[i], pos) - eui->startPos);		
					}

					Vec3 p = eui->startPos;
					for(int i = 0; i < 3; i++) {
						dxDrawArrow(p, p + globalAxis[i]*l[i], vToCam, tVectorWidth, cTranslationVector);
						p = p + globalAxis[i]*l[i];
					}
				}
			}

			if(!active) {
				if(axisIndex || planeIndex || centerIndex) {
					if(centerIndex) {
						eui->axis = cam->pos - pos;
						eui->centerDistanceToCam = len(vectorToCam(pos, cam));
						eui->transformMode = TRANSFORM_MODE_CENTER;
					} else if(planeIndex) {
						eui->axis = axis[planeIndex-1];
						eui->axisIndex = planeIndex;
						eui->transformMode = TRANSFORM_MODE_PLANE;
					} else if(axisIndex) {
						eui->axis = axis[axisIndex-1];
						eui->axisIndex = axisIndex;
						eui->transformMode = TRANSFORM_MODE_AXIS;
					}

					eui->selectionState = ENTITYUI_HOT;
					for(int i = 0; i < 3; i++) eui->axes[i] = axis[i];

				} else eui->selectionState = ENTITYUI_INACTIVE;
			}

			eui->hotId = contenderId;
		}

		if(eui->selectionMode == ENTITYUI_MODE_ROTATION && !multipleSelection) {
			dxDepthTest(true);

			// Find closest ring to camera.
			float closestDistance = FLT_MAX;
			int axisIndex = 0;
			if(!active) {
				for(int i = 0; i < 3; i++) {
					Vec3 intersection;
					if(linePlaneIntersection(rayPos, rayDir, pos, axis[i], &intersection) != -1) {
						float distToObj = len(intersection - pos);
						float r = radius - rRingWidth/2;
						if(between(distToObj, r-rRingActWidth/2, r+rRingActWidth/2)) {

							float distToCam = -(vm*vec4(intersection,0)).z;
							if(distToCam < closestDistance) {
								closestDistance = distToCam;
								axisIndex = i+1;
							}
						}
					}
				}
			} else axisIndex = eui->axisIndex;
		
			// Also check closest distance to occlusion sphere.
			if(!active) {
				Vec3 intersection;
				if(lineSphereIntersection(rayPos, rayDir, pos, radius-rRingActWidth*0.5f, &intersection) != -1) {
					float distToCam = -(vm*vec4(intersection,0)).z;
					if(distToCam < closestDistance) axisIndex = 0;
				}
			}

			dxBindFrameBuffer("3dMsaa", "ds3dTemp");
			defer { dxBindFrameBuffer("3dMsaa", "ds3dTemp"); };

			// Occlude rings with sphere.
			dxDrawSphere(pos, (radius*2)-rRingWidth*2, vec4(0,0,0,0));

			// Draw rings.
			if(!active) {
				for(int i = 0; i < 3; i++) {
					dxDrawRing(pos, -axis[i], radius, rRingWidth, vec4(axisColor[i],uiAlpha), ringTesselationError);
				}
			}

			if(axisIndex && !eui->guiHasFocus) {
				dxDrawRing(pos, -axis[axisIndex-1], radius, rRingWidth, vec4(cHot,1), ringTesselationError);
			}

			if(active) {
				dxDepthTest(false);
				defer { dxDepthTest(true); };

				int i = axisIndex-1;
				dxDrawRing(pos, -axis[i], radius, rRingWidth, vec4(cActive,1), ringTesselationError);

				Vec3 left = pos + norm(eui->objectDistanceVector)*radius;
				dxDrawTriangleFan(pos, left, eui->currentRotationAngle, axis[i], cRotationSegment, ringTesselationError);
			}

			if(!active) {
				if(axisIndex) {
					eui->axisIndex = axisIndex;
					eui->axis = axis[axisIndex-1];
					eui->selectionState = ENTITYUI_HOT;
				} else eui->selectionState = ENTITYUI_INACTIVE;
			}
		}

		if(eui->selectionMode == ENTITYUI_MODE_SCALE && !multipleSelection) {
			dxDepthTest(false);
			defer { dxDepthTest(true); };

			auto drawCubeLine = [rot, sArrowWidth] (Vec3 a, Vec3 b, int axis, Vec4 color) {
				Vec3 d = vec3(sArrowWidth);
				d.e[axis] = len(a-b);
				dxDrawCube(xForm((a+b)*0.5f, rot, d), color);
			};

			int axisIndex = 0;
			for(int i = 0; i < 3; i++) {
				widgetId++;

				Vec3 bp = pos + axis[i] * (radius - sBoxDim*0.5f);

				Vec3 c = axisColor[i];
				if(boxRaycastRotated(rayPos, rayDir, xForm(bp, rot, vec3(sBoxDim))) != -1) {
					axisIndex = i+1;
					contenderId = widgetId;
					if(!eui->guiHasFocus && eui->hotId == contenderId) c = cHot;
				}

				bool isActive = (axisActive && currentAxis == i) || (planeActive && currentAxis != i) || centerActive;
				if(isActive) c = cActive;

				if(!active) {
					drawCubeLine(pos + axis[i]*sArrowOff, bp, i, vec4(c,1));
					dxDrawCube(xForm(bp, rot, vec3(sBoxDim)), vec4(c, 1));

				} else if(isActive) {
					float l = radius-sBoxDim*0.5f;

					drawCubeLine(pos + axis[i]*l, pos + axis[i]*l*eui->currentScalePercent, i, vec4(c,1));

					{
						dxDepthTest(true);
						defer { dxDepthTest(false); };
						dxBindFrameBuffer("3dMsaa", "ds3dTemp");
						defer { dxBindFrameBuffer("3dMsaa", "ds3dTemp"); };

						drawCubeLine(pos, pos + axis[i]*l, i, vec4(cScaleRange, 1));

						dxDrawCube(xForm(pos + axis[i]*l, rot, vec3(sBoxDim*0.5f)), vec4(cScaleRange, 1));
						dxDrawCube(xForm(pos, rot, vec3(sBoxDim*0.5f)), vec4(cScaleRange, 1));

						Vec3 p = pos + axis[i]*l*eui->currentScalePercent;
						dxDrawCube(xForm(p, rot, vec3(sBoxDim)), vec4(c, 1));
					}
				}
			}

			int planeIndex = 0;
			for(int i = 0; i < 3; i++) {
				widgetId++;

				float pd = sPlaneSize;
				float l = radius*0.5f + pd*0.5f;
				Vec3 pp = axis[(i+1)%3]*l + axis[(i+2)%3]*l;
				Vec3 p = pos + pp;

				Vec3 c = axisColor[i];
				if(linePlaneIntersection(rayPos, rayDir, p, axis[i], axis[(i+1)%3], vec2(pd)) != -1) {
					planeIndex = i+1;
					contenderId = widgetId;
					if(!eui->guiHasFocus && eui->hotId == contenderId) c = cHot;
				}

				if(planeActive && currentAxis == i) c = cActive;

				bool ignore = axisActive || (planeActive && currentAxis != i) || centerActive;
				if(!ignore) {
					Vec3 p = pos + pp * (planeActive ? eui->currentScalePercent : 1);
					dxDrawPlane(p, axis[i], axis[(i+1)%3], vec2(pd), vec4(c,uiAlpha), dxGetTexture("misc\\roundedSquare.dds")->view);
				}
			}

			int centerIndex = 0;
			{
				widgetId++;

				Vec3 c = cCenter;
				if(boxRaycastRotated(rayPos, rayDir, xForm(pos, rot, vec3(sBoxDim))) != -1) {
					centerIndex = 1;
					contenderId = widgetId;
					if(!eui->guiHasFocus && eui->hotId == contenderId) c = cHot;
				}

				if(centerActive) c = cActive;

				if(!active) {
					dxDrawCube(xForm(pos, rot, vec3(sBoxDim)), vec4(c,uiAlpha));
				}
			}

			if(!active) {
				if(axisIndex || planeIndex || centerIndex) {
					if(centerIndex) {
						eui->axis = cam->pos - pos;
						eui->centerDistanceToCam = len(vectorToCam(pos, cam));
						eui->transformMode = TRANSFORM_MODE_CENTER;
					} else if(planeIndex) {
						eui->axis = axis[planeIndex-1];
						eui->axisIndex = planeIndex;
						eui->transformMode = TRANSFORM_MODE_PLANE;
					} else if(axisIndex) {
						eui->axis = axis[axisIndex-1];
						eui->axisIndex = axisIndex;
						eui->transformMode = TRANSFORM_MODE_AXIS;
					}
					eui->selectionState = ENTITYUI_HOT;
					for(int i = 0; i < 3; i++) eui->axes[i] = axis[i];

				} else eui->selectionState = ENTITYUI_INACTIVE;
			}

			eui->hotId = contenderId;
		}
	}

}
