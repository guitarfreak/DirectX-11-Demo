
struct InventorySlot {
	int type;
	int count;
};

struct Inventory {
	bool show;
	bool swapMouse;

	int maxStackCount;

	// Stored consecutively.
	InventorySlot slots[100];
	int slotCount;
	int quickSlotCount;

	//

	bool activeSlotDrag;
	int dragSlotIndex;
	InventorySlot dragSlot;
	Vec2 dragMouseOffset;

	//

	int quickSlotSelected;
};

bool inventoryAdd(Inventory* inv, int type) {

	int totalSlotCount = inv->slotCount + inv->quickSlotCount;

	// Look for existing item stack.
	int slotIndex = -1;
	for(int i = 0; i < totalSlotCount; i++) {
		InventorySlot* slot = inv->slots + i;

		if(slot->count == 0 && slotIndex == -1) slotIndex = i;

		if(slot->count && 
		   slot->type == type && slot->count < inv->maxStackCount) {
			slot->type = type;
			slot->count++;

			return true;
		}
	}

	// We didn't find an existing stack so we use a new empty one.

	if(slotIndex != -1) {
		inv->slots[slotIndex].type = type;
		inv->slots[slotIndex].count = 1;

		return true;
	}

	// There is no more space.

	return false;
}

int inventoryRemove(Inventory* inv, int index) {
	InventorySlot* slot = inv->slots + index;

	if(slot->count > 0) {
		slot->count--;
		return slot->type;
	}

	return 0;
}

int inventoryRemoveQuick(Inventory* inv) {
	return inventoryRemove(inv, inv->slotCount + inv->quickSlotSelected);
}

void inventorySwapQuick(Inventory* inv, int slotIndex, int quickSlotIndex) {
	quickSlotIndex += inv->slotCount;

	InventorySlot temp = inv->slots[slotIndex];
	inv->slots[slotIndex] = inv->slots[quickSlotIndex];
	inv->slots[quickSlotIndex] = temp;
}

void inventoryDrawIcon(InventorySlot slot, Rect r, Font* font, Vec4 cFont, Vec4 cShadow) {

	if(!slot.count) return;

	// Draw resource.

	float iconMargin = 0.5f;
	float iconSize = r.w() * iconMargin;

	Vec3 pos = vec3(r.c(), 0);

	Vec4 color = vec4(1,1);
	int colorPalleteIndex = blockColor[slot.type];
	color.r = colorPaletteCompact[colorPalleteIndex][0] / 255.0f;
	color.g = colorPaletteCompact[colorPalleteIndex][1] / 255.0f;
	color.b = colorPaletteCompact[colorPalleteIndex][2] / 255.0f;
	color = gammaToLinear(color);

	// Because we can't draw in 3d with commandList2d
	// we calculate it by hand for now.

	float angle1 = 0.25f;
	float angle2 = -0.5f;
	Quat rot = quat();
	rot = quat(M_PI_2, vec3(-1,0,0)) * rot;
	rot = quat(angle1, vec3(1,0,0)) * rot;
	rot = rot * quat(angle2,vec3(0,0,1));

	int voxelTextureId = getTexture("voxelTextures")->id;

	int dirIndex = 1;
	for(int faceIndex = 0; faceIndex < 6; faceIndex++) {

		Vec3 vs[4];
		getVoxelQuadFromFaceDir(pos, faceIndex, vs, iconSize);

		for(int i = 0; i < 4; i++) {
			vs[i] -= pos;
			vs[i] = rot * vs[i];
			vs[i] += pos;
		}

		int faceTextureId = texture1Faces[slot.type][faceIndex];
		dcQuad2d(vs[0].xy, vs[1].xy, vs[2].xy, vs[3].xy, color, voxelTextureId, rect(0,0,1,1), faceTextureId);
	}

	// Draw quantity.
	if(slot.count > 1) {
		Vec2 tOffset = font->height * (vec2(-0.25f,0) + vec2(0.0f)*vec2(-1,1));

		char* t = fillString("%i", slot.count);
		dcText(t, font, r.br() + tOffset, cFont, vec2i(1,-1), 0, 1, cShadow);
	}
}

void inventoryInitResource(Entity* e, int type) {
	e->blockType = type;

	e->dim = vec3(0.3f + randomOffset(0.02f));
}

void inventoryThrowAway(InventorySlot slot, EntityList* entityList, Entity* player, Camera* cam) {
	Entity e;
	initEntity(&e, ET_BlockResource, player->pos, vec3(1), player->chunk);

	Vec3 dir = cam->look;
	dir = norm(cross(vec3(0,0,1), cam->right));
	e.pos += dir * 1;
	e.vel = dir * 2 * randomFloat(1,1.5f);

	Vec3 p = e.pos;

	for(int i = 0; i < slot.count; i++) {
		inventoryInitResource(&e, slot.type);

		e.pos = p + randomUnitHalfSphereDirection(dir) * 0.2f;

		addEntity(entityList, &e);
	}
}