
void defaultMap(EntityManager* em) {
	em->indices.clear();
	em->entities.clear();

	// Init player.
	{
		Entity player = entity(ET_Player, xForm(vec3(20,20,40), vec3(1)), "Player");
		addEntity(em, &player);
	}

	// Debug cam.
	{
		Entity freeCam = entity(ET_Camera, xForm(vec3(-2,-10,2.5f), vec3(0.0f)), "Camera");
		freeCam.camRot = vec2(M_PI, 0.0f);
		addEntity(em, &freeCam);
	}

	{
		Entity e = entity(ET_Sky);

		e.skySettings.sunAngles               = vec2(radianToDegree(5.0f), radianToDegree(0.5f));
		e.skySettings.spotBrightness          = 4.0f;
		e.skySettings.mieBrightness           = 0.3f;
		e.skySettings.mieDistribution         = 0.10f;
		e.skySettings.mieStrength             = 0.2f;
		e.skySettings.mieCollectionPower      = 0.1f;
		e.skySettings.rayleighBrightness      = 0.7f;
		e.skySettings.rayleighStrength        = 0.5f;
		e.skySettings.rayleighCollectionPower = 0.25f;
		e.skySettings.scatterStrength         = 0.1f;
		e.skySettings.surfaceHeight           = 0.99;
		e.skySettings.intensity               = 1.8;
		e.skySettings.stepCount               = 16;
		e.skySettings.horizonOffset           = 0.5f;
		e.skySettings.sunOffset               = 0.35f;

		addEntity(em, &e);
	}

	{
		Entity platform = entity(ET_Object, xForm(vec3(-2.5,0,-0.05f), vec3(15,10,0.1f)), false, 0, "cube\\obj.obj", "Matte\\mat.mtl", vec4(1.0f));
		addEntity(em, platform);
	}
}

void saveMap(EntityManager* em, char* mapName) {
	DArray<char> buffer;
	buffer.init();
	
	SData sData = {};
	serializeData(&sData, getType(DArray_Entity), (char*)&em->entities, "EntityList");
	writeSData(&sData, &buffer);

	char* filePath = fString("%s%s%s", App_Map_Folder, mapName, Map_File_Extension);
	writeBufferToFile(buffer.data, filePath);
}

void loadMap(EntityManager* em, char* mapName) {
	TIMER_BLOCK();

	char* filePath = fString("%s%s%s", App_Map_Folder, mapName, Map_File_Extension);

	SData sData;
	readSDataFromFile(&sData, filePath);
	versionConversion(&sData);

	int entityCount = *((int*)sData.members.data[1].aData);

	em->entities.clear();
	em->entities.resize(entityCount);
	em->indices.resize(entityCount);
	em->indices.count = entityCount;

	serializeData(&sData, getType(DArray_Entity), (char*)&em->entities, "EntityList", 0, 0, 0, true);

	for(int i = 0; i < em->entities.count; i++) {
		Entity* e = em->entities.data + i;
		em->indices[e->id-1] = i;
	}

	// Reset entities.
	for(auto& e : em->entities) {
		if(e.type == ET_ParticleEffect) {
			e.particleEmitter.particles = {};
			e.particleEmitter.reset();
		}
	}

	em->currentMap = getPString(mapName);
}

void loadMap(EntityManager* em, char* mapName, HistoryData* hd) {
	loadMap(em, mapName);
	historyReset(hd);
}

void refreshMaps(DArray<char*>* maps) {
	maps->clear();

	FolderSearchData fd;
	folderSearchStart(&fd, App_Map_Folder);
	while(folderSearchNextFile(&fd)) {
		maps->push(getPString(fd.fileName));
	}
}

int nextEntity(EntityManager* em, int index, bool previous = false) {
	int i = index;
	while(true) {
		i = !previous ? i+1 : i-1;
		i = mod(i, em->entities.count);

		if(entityIsValid(em->entities.data + i)) return i;
		if(i == index) return index;
	}
}