
EnumInfo Entity_Type_EnumInfos[] = {
	{ ET_Player, "ET_Player", "Player" },
	{ ET_Camera, "ET_Camera", "Camera" },
	{ ET_Sky, "ET_Sky", "Sky" },
	{ ET_Object, "ET_Object", "Object" },
	{ ET_ParticleEffect, "ET_ParticleEffect", "ParticleEffect" },
	{ ET_Group, "ET_Group", "Group" },
	{ ET_Sound, "ET_Sound", "Sound" },
};

EnumInfo Particle_Textures_EnumInfos[] = {
	{ PARTICLE_TEXTURE_Circle, "PARTICLE_TEXTURE_Circle", "Circle" },
	{ PARTICLE_TEXTURE_Square, "PARTICLE_TEXTURE_Square", "Square" },
	{ PARTICLE_TEXTURE_Triangle, "PARTICLE_TEXTURE_Triangle", "Triangle" },
	{ PARTICLE_TEXTURE_Cloud, "PARTICLE_TEXTURE_Cloud", "Cloud" },
	{ PARTICLE_TEXTURE_Ring, "PARTICLE_TEXTURE_Ring", "Ring" },
	{ PARTICLE_TEXTURE_Bubble1, "PARTICLE_TEXTURE_Bubble1", "Bubble1" },
	{ PARTICLE_TEXTURE_Bubble2, "PARTICLE_TEXTURE_Bubble2", "Bubble2" },
	{ PARTICLE_TEXTURE_Point, "PARTICLE_TEXTURE_Point", "Point" },
	{ PARTICLE_TEXTURE_Star, "PARTICLE_TEXTURE_Star", "Star" },
	{ PARTICLE_TEXTURE_Smoke1, "PARTICLE_TEXTURE_Smoke1", "Smoke1" },
	{ PARTICLE_TEXTURE_Smoke2, "PARTICLE_TEXTURE_Smoke2", "Smoke2" },
	{ PARTICLE_TEXTURE_Fire, "PARTICLE_TEXTURE_Fire", "Fire" },
};

EnumInfo SpawnRegionType_EnumInfos[] = {
	{ SPAWN_REGION_SPHERE, "SPAWN_REGION_SPHERE", "Sphere" },
	{ SPAWN_REGION_CYLINDER, "SPAWN_REGION_CYLINDER", "Cylinder" },
	{ SPAWN_REGION_BOX, "SPAWN_REGION_BOX", "Box" },
};

EnumInfo FunctionType_EnumInfos[] = {
	{ FUNC_CONST, "FUNC_CONST", "Const" },
	{ FUNC_LINEAR, "FUNC_LINEAR", "Linear" },
	{ FUNC_QUADRATIC, "FUNC_QUADRATIC", "Quadratic" },
	{ FUNC_CUBIC, "FUNC_CUBIC", "Cubic" },
	{ FUNC_QUARTIC, "FUNC_QUARTIC", "Quartic" },
	{ FUNC_TWO_DICE, "FUNC_TWO_DICE", "Two_Dice" },
	{ FUNC_THREE_DICE, "FUNC_THREE_DICE", "Three_Dice" },
	{ FUNC_FOUR_DICE, "FUNC_FOUR_DICE", "Four_Dice" },
	{ FUNC_TWO_DICE_SQUARED, "FUNC_TWO_DICE_SQUARED", "Two_Dice_Squared" },
	{ FUNC_THREE_DICE_SQUARED, "FUNC_THREE_DICE_SQUARED", "Three_Dice_Squared" },
	{ FUNC_FOUR_DICE_SQUARED, "FUNC_FOUR_DICE_SQUARED", "Four_Dice_Squared" },
	{ FUNC_EXTREMES, "FUNC_EXTREMES", "Extremes" },
};

EnumInfo BlockerType_EnumInfos[] = {
	{ BLOCKERTYPE_NONE, "BLOCKERTYPE_NONE", "None" },
	{ BLOCKERTYPE_RECT, "BLOCKERTYPE_RECT", "Rect" },
	{ BLOCKERTYPE_CIRCLE, "BLOCKERTYPE_CIRCLE", "Circle" },
	{ BLOCKERTYPE_LINE, "BLOCKERTYPE_LINE", "Line" },
};

enum StructType {
	TYPE_u64 = 0,
	TYPE_i64,
	TYPE_int,
	TYPE_uint,
	TYPE_short,
	TYPE_ushort,
	TYPE_char,
	TYPE_uchar,
	TYPE_float,
	TYPE_double,
	TYPE_bool,
	TYPE_string,
	TYPE_MouseEvents,
	TYPE_AppData,
	TYPE_Audio,
	TYPE_Track,
	TYPE_Sound,
	TYPE_AudioState,
	TYPE_Camera,
	TYPE_DArray_int,
	TYPE_DArray_Entity,
	TYPE_Entity,
	TYPE_EntityManager,
	TYPE_Font,
	TYPE_GraphicsState,
	TYPE_Input,
	TYPE_Vec2,
	TYPE_Vec2i,
	TYPE_Vec3,
	TYPE_Vec3i,
	TYPE_Vec4,
	TYPE_Mat4,
	TYPE_Quat,
	TYPE_XForm,
	TYPE_Line2,
	TYPE_Line3,
	TYPE_Rect,
	TYPE_Recti,
	TYPE_Rect3,
	TYPE_Rect3i,
	TYPE_GameSettings,
	TYPE_MainMenu,
	TYPE_Object,
	TYPE_SpawnRegion,
	TYPE_ValueRange_float,
	TYPE_ValueRange_Vec4,
	TYPE_ParticleSettings,
	TYPE_ParticleEmitter,
	TYPE_MonitorData,
	TYPE_WindowSettings,
	TYPE_SystemData,
	TYPE_Texture,
	TYPE_FrameBuffer,
	TYPE_GraphicsSettings,
	TYPE_GraphicsMatrices,
	TYPE_WalkManifoldSettings,
	TYPE_SkySettings,

	TYPE_SIZE, 
};

MemberInfo MouseEvents_MemberInfos[] = {
	{ TYPE_bool, "debugMouse", offsetof(MouseEvents, debugMouse), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "debugMouseFixed", offsetof(MouseEvents, debugMouseFixed), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "captureMouseKeepCenter", offsetof(MouseEvents, captureMouseKeepCenter), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "captureMouse", offsetof(MouseEvents, captureMouse), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "fpsMode", offsetof(MouseEvents, fpsMode), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "fpsModeFixed", offsetof(MouseEvents, fpsModeFixed), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "lostFocusWhileCaptured", offsetof(MouseEvents, lostFocusWhileCaptured), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo AppData_MemberInfos[] = {
	{ TYPE_SystemData, "systemData", offsetof(AppData, systemData), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Input, "input", offsetof(AppData, input), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_WindowSettings, "wSettings", offsetof(AppData, wSettings), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_GraphicsState, "GraphicsState", offsetof(AppData, GraphicsState), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_AudioState, "audioState", offsetof(AppData, audioState), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_double, "dt", offsetof(AppData, dt), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_double, "time", offsetof(AppData, time), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "frameCount", offsetof(AppData, frameCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "maxFrameRate", offsetof(AppData, maxFrameRate), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "frameRateCap", offsetof(AppData, frameRateCap), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "updateFrameBuffers", offsetof(AppData, updateFrameBuffers), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "updateDebugFrameBuffer", offsetof(AppData, updateDebugFrameBuffer), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "mouseSensitivity", offsetof(AppData, mouseSensitivity), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_MouseEvents, "mouseEvents", offsetof(AppData, mouseEvents), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "toggleFullscreen", offsetof(AppData, toggleFullscreen), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_GraphicsSettings, "gSettings", offsetof(AppData, gSettings), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Rect3, "sceneBoundingBox", offsetof(AppData, sceneBoundingBox), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "redrawSkyBox", offsetof(AppData, redrawSkyBox), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "newGameMode", offsetof(AppData, newGameMode), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "gameMode", offsetof(AppData, gameMode), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_MainMenu, "menu", offsetof(AppData, menu), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "levelEdit", offsetof(AppData, levelEdit), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "freeCam", offsetof(AppData, freeCam), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "playerHeight", offsetof(AppData, playerHeight), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "loading", offsetof(AppData, loading), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "newGame", offsetof(AppData, newGame), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "saveGame", offsetof(AppData, saveGame), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_GameSettings, "settings", offsetof(AppData, settings), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "volumeFootsteps", offsetof(AppData, volumeFootsteps), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "volumeGeneral", offsetof(AppData, volumeGeneral), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "volumeMenu", offsetof(AppData, volumeMenu), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_EntityManager, "entityManager", offsetof(AppData, entityManager), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Entity, "player", offsetof(AppData, player), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_CONST, 1, sizeof(AppData)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Entity, "camera", offsetof(AppData, camera), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_CONST, 1, sizeof(AppData)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Entity, "sky", offsetof(AppData, sky), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_CONST, 1, sizeof(AppData)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Entity, "figure", offsetof(AppData, figure), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_CONST, 1, sizeof(AppData)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "showSkeleton", offsetof(AppData, showSkeleton), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "figureAnimation", offsetof(AppData, figureAnimation), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "figureSpeed", offsetof(AppData, figureSpeed), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "firstWalk", offsetof(AppData, firstWalk), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "footstepSoundValue", offsetof(AppData, footstepSoundValue), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "lastFootstepSoundId", offsetof(AppData, lastFootstepSoundId), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "manifoldGridRadius", offsetof(AppData, manifoldGridRadius), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2, "playerMoveDir", offsetof(AppData, playerMoveDir), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Mat4, "viewProjLight", offsetof(AppData, viewProjLight), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo Audio_MemberInfos[] = {
	{ TYPE_string, "name", offsetof(Audio, name), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_string, "file", offsetof(Audio, file), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "channels", offsetof(Audio, channels), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "sampleRate", offsetof(Audio, sampleRate), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "bitsPerSample", offsetof(Audio, bitsPerSample), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_char, "vorbisFile", offsetof(Audio, vorbisFile), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_CONST, 1, sizeof(Audio)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "decodeIndex", offsetof(Audio, decodeIndex), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "frameCount", offsetof(Audio, frameCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "totalLength", offsetof(Audio, totalLength), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo Track_MemberInfos[] = {
	{ TYPE_bool, "used", offsetof(Track, used), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "playing", offsetof(Track, playing), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "paused", offsetof(Track, paused), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "index", offsetof(Track, index), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_i64, "startTime", offsetof(Track, startTime), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Audio, "audio", offsetof(Track, audio), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_CONST, 1, sizeof(Track)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "type", offsetof(Track, type), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "volume", offsetof(Track, volume), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "speed", offsetof(Track, speed), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "isSpatial", offsetof(Track, isSpatial), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec3, "pos", offsetof(Track, pos), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "minDistance", offsetof(Track, minDistance), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "maxDistance", offsetof(Track, maxDistance), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo Sound_MemberInfos[] = {
	{ TYPE_string, "name", offsetof(Sound, name), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "type", offsetof(Sound, type), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Audio, "audioArray", offsetof(Sound, audioArray), 2, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_VAR, offsetof(Sound, audioCount), sizeof(char*)}, {ARRAYTYPE_DYNAMIC, ARRAYSIZE_VAR, offsetof(Sound, audioCount), sizeof(Sound)}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "audioCount", offsetof(Sound, audioCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "volume", offsetof(Sound, volume), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "volumeMod", offsetof(Sound, volumeMod), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "pitchOffset", offsetof(Sound, pitchOffset), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "pitchMod", offsetof(Sound, pitchMod), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "minDistance", offsetof(Sound, minDistance), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "maxDistance", offsetof(Sound, maxDistance), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo AudioState_MemberInfos[] = {
	{ TYPE_float, "latencyInSeconds", offsetof(AudioState, latencyInSeconds), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_uint, "bufferFrameCount", offsetof(AudioState, bufferFrameCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "latency", offsetof(AudioState, latency), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "channelCount", offsetof(AudioState, channelCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "sampleRate", offsetof(AudioState, sampleRate), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Audio, "files", offsetof(AudioState, files), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_VAR, offsetof(AudioState, fileCount), sizeof(AudioState)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "fileCount", offsetof(AudioState, fileCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "fileCountMax", offsetof(AudioState, fileCountMax), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "masterVolume", offsetof(AudioState, masterVolume), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "effectVolume", offsetof(AudioState, effectVolume), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "musicVolume", offsetof(AudioState, musicVolume), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Track, "tracks", offsetof(AudioState, tracks), 1, { {ARRAYTYPE_CONSTANT, ARRAYSIZE_CONST, memberArrayCount(AudioState, tracks), memberSize(AudioState, tracks[0])}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Sound, "sounds", offsetof(AudioState, sounds), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_VAR, offsetof(AudioState, soundCount), sizeof(AudioState)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "soundCount", offsetof(AudioState, soundCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo Camera_MemberInfos[] = {
	{ TYPE_Vec3, "pos", offsetof(Camera, pos), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec3, "look", offsetof(Camera, look), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec3, "up", offsetof(Camera, up), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec3, "right", offsetof(Camera, right), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2, "dim", offsetof(Camera, dim), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2, "rot", offsetof(Camera, rot), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo DArray_int_MemberInfos[] = {
	{ TYPE_int, "data", offsetof(DArray<int>, data), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_VAR, offsetof(DArray<int>, count), sizeof(DArray<int>)}, {-1}}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_int, "count", offsetof(DArray<int>, count), 0, {}, {-1}, 0, 0, 1, 0, {}, 0, -1 },
	{ TYPE_int, "reserved", offsetof(DArray<int>, reserved), 0, {}, {-1}, 0, 0, 1, 0, {}, 0, -1 },
	{ TYPE_int, "startSize", offsetof(DArray<int>, startSize), 0, {}, {-1}, 0, 0, 1, 0, {}, 0, -1 },
	{ TYPE_bool, "customAlloc", offsetof(DArray<int>, customAlloc), 0, {}, {-1}, 0, 0, 1, 0, {}, 0, -1 },
};

MemberInfo DArray_Entity_MemberInfos[] = {
	{ TYPE_Entity, "data", offsetof(DArray<Entity>, data), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_VAR, offsetof(DArray<Entity>, count), sizeof(DArray<Entity>)}, {-1}}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_int, "count", offsetof(DArray<Entity>, count), 0, {}, {-1}, 0, 0, 1, 0, {}, 0, -1 },
	{ TYPE_int, "reserved", offsetof(DArray<Entity>, reserved), 0, {}, {-1}, 0, 0, 1, 0, {}, 0, -1 },
	{ TYPE_int, "startSize", offsetof(DArray<Entity>, startSize), 0, {}, {-1}, 0, 0, 1, 0, {}, 0, -1 },
	{ TYPE_bool, "customAlloc", offsetof(DArray<Entity>, customAlloc), 0, {}, {-1}, 0, 0, 1, 0, {}, 0, -1 },
};

MemberInfo Entity_MemberInfos[] = {
	{ TYPE_int, "type", offsetof(Entity, type), 0, {}, {-1}, arrayCount(Entity_Type_EnumInfos), Entity_Type_EnumInfos, 1, 0, {}, 0, -1 },
	{ TYPE_int, "id", offsetof(Entity, id), 0, {}, {-1}, 0, 0, 1, 0, {}, 0, -1 },
	{ TYPE_XForm, "xf", offsetof(Entity, xf), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_string, "name", offsetof(Entity, name), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_int, "groupId", offsetof(Entity, groupId), 0, {}, {-1}, 0, 0, 0, 1, {{"Id", "Group"}}, 0, -1 },
	{ TYPE_int, "groupZ", offsetof(Entity, groupZ), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_int, "mountParentId", offsetof(Entity, mountParentId), 0, {}, {-1}, 0, 0, 0, 1, {{"Id", "Entity"}}, 0, -1 },
	{ TYPE_XForm, "xfMount", offsetof(Entity, xfMount), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_string, "mesh", offsetof(Entity, mesh), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_Vec3, "meshoffset", offsetof(Entity, meshoffset), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_string, "material", offsetof(Entity, material), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_Vec4, "color", offsetof(Entity, color), 0, {}, {-1}, 0, 0, 0, 1, {{"Color"}}, 0, -1 },
	{ TYPE_bool, "noRender", offsetof(Entity, noRender), 0, {}, {-1}, 0, 0, 0, 0, {}, 3, -1 },
	{ TYPE_Vec3, "vel", offsetof(Entity, vel), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_Vec3, "acc", offsetof(Entity, acc), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_int, "blockerType", offsetof(Entity, blockerType), 0, {}, {-1}, arrayCount(BlockerType_EnumInfos), BlockerType_EnumInfos, 0, 0, {}, 2, -1 },
	{ TYPE_XForm, "xfBlocker", offsetof(Entity, xfBlocker), 0, {}, {-1}, 0, 0, 0, 0, {}, 2, -1 },
	{ TYPE_Rect3, "aabb", offsetof(Entity, aabb), 0, {}, {-1}, 0, 0, 1, 0, {}, -1, -1 },
	{ TYPE_Mat4, "modelInv", offsetof(Entity, modelInv), 0, {}, {-1}, 0, 0, 1, 0, {}, -1, -1 },
	{ TYPE_bool, "deleted", offsetof(Entity, deleted), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_Vec2, "camRot", offsetof(Entity, camRot), 0, {}, {ET_Camera, offsetof(Entity, type), "type"}, 0, 0, 0, 1, {{"Range", "0", "6.2831", "-6.2831", "6.2831"}}, 0, -1 },
	{ TYPE_SkySettings, "skySettings", offsetof(Entity, skySettings), 0, {}, {ET_Sky, offsetof(Entity, type), "type"}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_int, "trackIndex", offsetof(Entity, trackIndex), 0, {}, {ET_Sound, offsetof(Entity, type), "type"}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_ParticleEmitter, "particleEmitter", offsetof(Entity, particleEmitter), 0, {}, {ET_ParticleEffect, offsetof(Entity, type), "type"}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_bool, "handlesParticles", offsetof(Entity, handlesParticles), 0, {}, {ET_Group, offsetof(Entity, type), "type"}, 0, 0, 0, 0, {}, 0, -1 },
};

MemberInfo EntityManager_MemberInfos[] = {
	{ TYPE_DArray_int, "indices", offsetof(EntityManager, indices), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_DArray_Entity, "entities", offsetof(EntityManager, entities), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_char, "currentMap", offsetof(EntityManager, currentMap), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_CONST, 1, sizeof(EntityManager)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo Font_MemberInfos[] = {
	{ TYPE_char, "file", offsetof(Font, file), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_CONST, 1, sizeof(Font)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "id", offsetof(Font, id), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "heightIndex", offsetof(Font, heightIndex), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "pixelScale", offsetof(Font, pixelScale), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2i, "glyphRanges", offsetof(Font, glyphRanges), 1, { {ARRAYTYPE_CONSTANT, ARRAYSIZE_VAR, offsetof(Font, glyphRangeCount), memberSize(Font, glyphRanges[0])}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "glyphRangeCount", offsetof(Font, glyphRangeCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "totalGlyphCount", offsetof(Font, totalGlyphCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "height", offsetof(Font, height), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "baseOffset", offsetof(Font, baseOffset), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "lineSpacing", offsetof(Font, lineSpacing), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Font, "boldFont", offsetof(Font, boldFont), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_CONST, 1, sizeof(Font)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Font, "italicFont", offsetof(Font, italicFont), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_CONST, 1, sizeof(Font)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "pixelAlign", offsetof(Font, pixelAlign), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Texture, "tex", offsetof(Font, tex), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo GraphicsState_MemberInfos[] = {
	{ TYPE_Texture, "textures", offsetof(GraphicsState, textures), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_VAR, offsetof(GraphicsState, textureCount), sizeof(GraphicsState)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "textureCount", offsetof(GraphicsState, textureCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "textureCountMax", offsetof(GraphicsState, textureCountMax), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Texture, "textureWhite", offsetof(GraphicsState, textureWhite), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_CONST, 1, sizeof(GraphicsState)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Texture, "textureCircle", offsetof(GraphicsState, textureCircle), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_CONST, 1, sizeof(GraphicsState)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "cubeMapSize", offsetof(GraphicsState, cubeMapSize), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "shadowMapSize", offsetof(GraphicsState, shadowMapSize), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_FrameBuffer, "frameBuffers", offsetof(GraphicsState, frameBuffers), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_VAR, offsetof(GraphicsState, frameBufferCount), sizeof(GraphicsState)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "frameBufferCount", offsetof(GraphicsState, frameBufferCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "frameBufferCountMax", offsetof(GraphicsState, frameBufferCountMax), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Font, "fonts", offsetof(GraphicsState, fonts), 2, { {ARRAYTYPE_CONSTANT, ARRAYSIZE_CONST, memberArrayCount(GraphicsState, fonts), memberSize(GraphicsState, fonts[0])}, {ARRAYTYPE_CONSTANT, ARRAYSIZE_VAR, offsetof(GraphicsState, fontsCount), memberSize(GraphicsState, fonts[0][0])}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "fontsCount", offsetof(GraphicsState, fontsCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_string, "fontFolders", offsetof(GraphicsState, fontFolders), 1, { {ARRAYTYPE_CONSTANT, ARRAYSIZE_VAR, offsetof(GraphicsState, fontFolderCount), memberSize(GraphicsState, fontFolders[0])}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "fontFolderCount", offsetof(GraphicsState, fontFolderCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_GraphicsMatrices, "gMats", offsetof(GraphicsState, gMats), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_GraphicsSettings, "gSettings", offsetof(GraphicsState, gSettings), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_CONST, 1, sizeof(GraphicsState)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Camera, "activeCam", offsetof(GraphicsState, activeCam), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "currentShader", offsetof(GraphicsState, currentShader), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_uint, "currentTopology", offsetof(GraphicsState, currentTopology), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2i, "screenRes", offsetof(GraphicsState, screenRes), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Rect, "screenRect", offsetof(GraphicsState, screenRect), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "zLevel", offsetof(GraphicsState, zLevel), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo Input_MemberInfos[] = {
	{ TYPE_bool, "firstFrame", offsetof(Input, firstFrame), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2, "mousePos", offsetof(Input, mousePos), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2, "mousePosNegative", offsetof(Input, mousePosNegative), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2, "mousePosScreen", offsetof(Input, mousePosScreen), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2, "mousePosNegativeScreen", offsetof(Input, mousePosNegativeScreen), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2, "mouseDelta", offsetof(Input, mouseDelta), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "mouseWheel", offsetof(Input, mouseWheel), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "mouseButtonPressed", offsetof(Input, mouseButtonPressed), 1, { {ARRAYTYPE_CONSTANT, ARRAYSIZE_CONST, memberArrayCount(Input, mouseButtonPressed), memberSize(Input, mouseButtonPressed[0])}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "mouseButtonDown", offsetof(Input, mouseButtonDown), 1, { {ARRAYTYPE_CONSTANT, ARRAYSIZE_CONST, memberArrayCount(Input, mouseButtonDown), memberSize(Input, mouseButtonDown[0])}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "mouseButtonReleased", offsetof(Input, mouseButtonReleased), 1, { {ARRAYTYPE_CONSTANT, ARRAYSIZE_CONST, memberArrayCount(Input, mouseButtonReleased), memberSize(Input, mouseButtonReleased[0])}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "doubleClick", offsetof(Input, doubleClick), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2, "doubleClickPos", offsetof(Input, doubleClickPos), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2, "lastMousePos", offsetof(Input, lastMousePos), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "keysDown", offsetof(Input, keysDown), 1, { {ARRAYTYPE_CONSTANT, ARRAYSIZE_CONST, memberArrayCount(Input, keysDown), memberSize(Input, keysDown[0])}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "keysPressed", offsetof(Input, keysPressed), 1, { {ARRAYTYPE_CONSTANT, ARRAYSIZE_CONST, memberArrayCount(Input, keysPressed), memberSize(Input, keysPressed[0])}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_char, "inputCharacters", offsetof(Input, inputCharacters), 1, { {ARRAYTYPE_CONSTANT, ARRAYSIZE_CONST, memberArrayCount(Input, inputCharacters), memberSize(Input, inputCharacters[0])}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "inputCharacterCount", offsetof(Input, inputCharacterCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "anyKey", offsetof(Input, anyKey), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "closeWindow", offsetof(Input, closeWindow), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "maximizeWindow", offsetof(Input, maximizeWindow), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "minimizeWindow", offsetof(Input, minimizeWindow), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "resize", offsetof(Input, resize), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "altEnter", offsetof(Input, altEnter), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo Vec2_MemberInfos[] = {
	{ TYPE_float, "x", offsetof(Vec2, x), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "y", offsetof(Vec2, y), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
};

MemberInfo Vec2i_MemberInfos[] = {
	{ TYPE_int, "x", offsetof(Vec2i, x), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_int, "y", offsetof(Vec2i, y), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
};

MemberInfo Vec3_MemberInfos[] = {
	{ TYPE_float, "x", offsetof(Vec3, x), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "y", offsetof(Vec3, y), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "z", offsetof(Vec3, z), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
};

MemberInfo Vec3i_MemberInfos[] = {
	{ TYPE_int, "x", offsetof(Vec3i, x), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_int, "y", offsetof(Vec3i, y), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_int, "z", offsetof(Vec3i, z), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
};

MemberInfo Vec4_MemberInfos[] = {
	{ TYPE_float, "x", offsetof(Vec4, x), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "y", offsetof(Vec4, y), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "z", offsetof(Vec4, z), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "w", offsetof(Vec4, w), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
};

MemberInfo Mat4_MemberInfos[] = {
	{ TYPE_float, "xa", offsetof(Mat4, xa), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "xb", offsetof(Mat4, xb), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "xc", offsetof(Mat4, xc), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "xd", offsetof(Mat4, xd), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "ya", offsetof(Mat4, ya), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "yb", offsetof(Mat4, yb), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "yc", offsetof(Mat4, yc), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "yd", offsetof(Mat4, yd), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "za", offsetof(Mat4, za), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "zb", offsetof(Mat4, zb), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "zc", offsetof(Mat4, zc), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "zd", offsetof(Mat4, zd), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "wa", offsetof(Mat4, wa), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "wb", offsetof(Mat4, wb), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "wc", offsetof(Mat4, wc), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "wd", offsetof(Mat4, wd), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo Quat_MemberInfos[] = {
	{ TYPE_float, "w", offsetof(Quat, w), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "x", offsetof(Quat, x), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "y", offsetof(Quat, y), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "z", offsetof(Quat, z), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
};

MemberInfo XForm_MemberInfos[] = {
	{ TYPE_Vec3, "trans", offsetof(XForm, trans), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_Quat, "rot", offsetof(XForm, rot), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_Vec3, "scale", offsetof(XForm, scale), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
};

MemberInfo Line2_MemberInfos[] = {
	{ TYPE_Vec2, "a", offsetof(Line2, a), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2, "b", offsetof(Line2, b), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo Line3_MemberInfos[] = {
	{ TYPE_Vec3, "a", offsetof(Line3, a), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec3, "b", offsetof(Line3, b), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo Rect_MemberInfos[] = {
	{ TYPE_float, "left", offsetof(Rect, left), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "bottom", offsetof(Rect, bottom), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "right", offsetof(Rect, right), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "top", offsetof(Rect, top), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
};

MemberInfo Recti_MemberInfos[] = {
	{ TYPE_int, "left", offsetof(Recti, left), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_int, "bottom", offsetof(Recti, bottom), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_int, "right", offsetof(Recti, right), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_int, "top", offsetof(Recti, top), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
};

MemberInfo Rect3_MemberInfos[] = {
	{ TYPE_Vec3, "min", offsetof(Rect3, min), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_Vec3, "max", offsetof(Rect3, max), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
};

MemberInfo Rect3i_MemberInfos[] = {
	{ TYPE_Vec3i, "min", offsetof(Rect3i, min), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_Vec3i, "max", offsetof(Rect3i, max), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
};

MemberInfo GameSettings_MemberInfos[] = {
	{ TYPE_bool, "fullscreen", offsetof(GameSettings, fullscreen), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "vsync", offsetof(GameSettings, vsync), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "frameRateCap", offsetof(GameSettings, frameRateCap), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "resolutionScale", offsetof(GameSettings, resolutionScale), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "volume", offsetof(GameSettings, volume), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "mouseSensitivity", offsetof(GameSettings, mouseSensitivity), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "fieldOfView", offsetof(GameSettings, fieldOfView), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo MainMenu_MemberInfos[] = {
	{ TYPE_bool, "gameRunning", offsetof(MainMenu, gameRunning), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "volume", offsetof(MainMenu, volume), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "screen", offsetof(MainMenu, screen), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "activeId", offsetof(MainMenu, activeId), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "currentId", offsetof(MainMenu, currentId), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "buttonAnimState", offsetof(MainMenu, buttonAnimState), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Font, "font", offsetof(MainMenu, font), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_CONST, 1, sizeof(MainMenu)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec4, "cOption", offsetof(MainMenu, cOption), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec4, "cOptionActive", offsetof(MainMenu, cOptionActive), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec4, "cOptionShadow", offsetof(MainMenu, cOptionShadow), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec4, "cOptionShadowActive", offsetof(MainMenu, cOptionShadowActive), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "optionShadowSize", offsetof(MainMenu, optionShadowSize), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "pressedEnter", offsetof(MainMenu, pressedEnter), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "pressedEscape", offsetof(MainMenu, pressedEscape), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "pressedBackspace", offsetof(MainMenu, pressedBackspace), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "pressedUp", offsetof(MainMenu, pressedUp), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "pressedDown", offsetof(MainMenu, pressedDown), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "pressedLeft", offsetof(MainMenu, pressedLeft), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "pressedRight", offsetof(MainMenu, pressedRight), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo Object_MemberInfos[] = {
	{ TYPE_char, "name", offsetof(Object, name), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_CONST, 1, sizeof(Object)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec3, "pos", offsetof(Object, pos), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec3, "dim", offsetof(Object, dim), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec4, "color", offsetof(Object, color), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Quat, "rot", offsetof(Object, rot), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_char, "material", offsetof(Object, material), 1, { {ARRAYTYPE_DYNAMIC, ARRAYSIZE_CONST, 1, sizeof(Object)}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "hasAlpha", offsetof(Object, hasAlpha), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo SpawnRegion_MemberInfos[] = {
	{ TYPE_int, "type", offsetof(SpawnRegion, type), 0, {}, {-1}, arrayCount(SpawnRegionType_EnumInfos), SpawnRegionType_EnumInfos, 0, 0, {}, 0, -1 },
	{ TYPE_float, "radiusMin", offsetof(SpawnRegion, radiusMin), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "radius", offsetof(SpawnRegion, radius), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "height", offsetof(SpawnRegion, height), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec3, "dim", offsetof(SpawnRegion, dim), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
};

MemberInfo ValueRange_float_MemberInfos[] = {
	{ TYPE_float, "min", offsetof(ValueRange<float>, min), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "max", offsetof(ValueRange<float>, max), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_int, "functionType", offsetof(ValueRange<float>, functionType), 0, {}, {-1}, arrayCount(FunctionType_EnumInfos), FunctionType_EnumInfos, 0, 0, {}, 0, -1 },
};

MemberInfo ValueRange_Vec4_MemberInfos[] = {
	{ TYPE_Vec4, "min", offsetof(ValueRange<Vec4>, min), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_Vec4, "max", offsetof(ValueRange<Vec4>, max), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_int, "functionType", offsetof(ValueRange<Vec4>, functionType), 0, {}, {-1}, arrayCount(FunctionType_EnumInfos), FunctionType_EnumInfos, 0, 0, {}, 0, -1 },
};

MemberInfo ParticleSettings_MemberInfos[] = {
	{ TYPE_int, "spriteIndex", offsetof(ParticleSettings, spriteIndex), 0, {}, {-1}, arrayCount(Particle_Textures_EnumInfos), Particle_Textures_EnumInfos, 0, 0, {}, 0, -1 },
	{ TYPE_SpawnRegion, "region", offsetof(ParticleSettings, region), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "spawnRate", offsetof(ParticleSettings, spawnRate), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "lifeTime", offsetof(ParticleSettings, lifeTime), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "startTime", offsetof(ParticleSettings, startTime), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "restTime", offsetof(ParticleSettings, restTime), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_ValueRange_float, "lifeTimeP", offsetof(ParticleSettings, lifeTimeP), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "fadeInTime", offsetof(ParticleSettings, fadeInTime), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "fadeOutTime", offsetof(ParticleSettings, fadeOutTime), 0, {}, {-1}, 0, 0, 0, 1, {{"Section"}}, 0, -1 },
	{ TYPE_ValueRange_float, "size", offsetof(ParticleSettings, size), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_ValueRange_float, "velSize", offsetof(ParticleSettings, velSize), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_ValueRange_float, "speed", offsetof(ParticleSettings, speed), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_ValueRange_float, "angleV", offsetof(ParticleSettings, angleV), 0, {}, {-1}, 0, 0, 0, 1, {{"Range", "-90", "90"}}, 0, -1 },
	{ TYPE_ValueRange_float, "angleH", offsetof(ParticleSettings, angleH), 0, {}, {-1}, 0, 0, 0, 1, {{"Range", "0", "360"}}, 0, -1 },
	{ TYPE_Vec3, "gravity", offsetof(ParticleSettings, gravity), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_ValueRange_float, "drag", offsetof(ParticleSettings, drag), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_bool, "stretch", offsetof(ParticleSettings, stretch), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_ValueRange_float, "stretchMod", offsetof(ParticleSettings, stretchMod), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_ValueRange_float, "rotation", offsetof(ParticleSettings, rotation), 0, {}, {-1}, 0, 0, 0, 1, {{"Range", "-90", "90"}}, 0, -1 },
	{ TYPE_ValueRange_float, "velRot", offsetof(ParticleSettings, velRot), 0, {}, {-1}, 0, 0, 0, 1, {{"Section"}}, 0, -1 },
	{ TYPE_float, "colorT", offsetof(ParticleSettings, colorT), 0, {}, {-1}, 0, 0, 0, 1, {{"Range", "0", "1"}}, 0, -1 },
	{ TYPE_ValueRange_Vec4, "color", offsetof(ParticleSettings, color), 1, { {ARRAYTYPE_CONSTANT, ARRAYSIZE_CONST, memberArrayCount(ParticleSettings, color), memberSize(ParticleSettings, color[0])}, {-1}}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "alpha", offsetof(ParticleSettings, alpha), 0, {}, {-1}, 0, 0, 0, 1, {{"Range", "0", "1"}}, 0, -1 },
	{ TYPE_float, "brightness", offsetof(ParticleSettings, brightness), 0, {}, {-1}, 0, 0, 0, 1, {{"Range", "0", "5"}}, 0, -1 },
};

MemberInfo ParticleEmitter_MemberInfos[] = {
	{ TYPE_float, "time", offsetof(ParticleEmitter, time), 0, {}, {-1}, 0, 0, 1, 0, {}, -1, -1 },
	{ TYPE_float, "spawnTime", offsetof(ParticleEmitter, spawnTime), 0, {}, {-1}, 0, 0, 1, 0, {}, -1, -1 },
	{ TYPE_float, "startTime", offsetof(ParticleEmitter, startTime), 0, {}, {-1}, 0, 0, 1, 0, {}, -1, -1 },
	{ TYPE_float, "finishTime", offsetof(ParticleEmitter, finishTime), 0, {}, {-1}, 0, 0, 1, 0, {}, -1, -1 },
	{ TYPE_bool, "finished", offsetof(ParticleEmitter, finished), 0, {}, {-1}, 0, 0, 1, 0, {}, -1, -1 },
	{ TYPE_bool, "loop", offsetof(ParticleEmitter, loop), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_bool, "localSpace", offsetof(ParticleEmitter, localSpace), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "fadeDistance", offsetof(ParticleEmitter, fadeDistance), 0, {}, {-1}, 0, 0, 0, 0, {}, 1, -1 },
	{ TYPE_float, "fadeContrast", offsetof(ParticleEmitter, fadeContrast), 0, {}, {-1}, 0, 0, 0, 0, {}, 1, -1 },
	{ TYPE_ParticleSettings, "settings", offsetof(ParticleEmitter, settings), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
};

MemberInfo MonitorData_MemberInfos[] = {
	{ TYPE_Rect, "fullRect", offsetof(MonitorData, fullRect), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Rect, "workRect", offsetof(MonitorData, workRect), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo WindowSettings_MemberInfos[] = {
	{ TYPE_Vec2i, "res", offsetof(WindowSettings, res), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2i, "fullRes", offsetof(WindowSettings, fullRes), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "fullscreen", offsetof(WindowSettings, fullscreen), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_uint, "style", offsetof(WindowSettings, style), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Rect, "previousWindowRect", offsetof(WindowSettings, previousWindowRect), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_MonitorData, "monitors", offsetof(WindowSettings, monitors), 1, { {ARRAYTYPE_CONSTANT, ARRAYSIZE_CONST, memberArrayCount(WindowSettings, monitors), memberSize(WindowSettings, monitors[0])}, {-1}}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "monitorCount", offsetof(WindowSettings, monitorCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2i, "biggestMonitorSize", offsetof(WindowSettings, biggestMonitorSize), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "refreshRate", offsetof(WindowSettings, refreshRate), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2i, "currentRes", offsetof(WindowSettings, currentRes), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "aspectRatio", offsetof(WindowSettings, aspectRatio), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "windowScale", offsetof(WindowSettings, windowScale), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "dontUpdateCursor", offsetof(WindowSettings, dontUpdateCursor), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "customCursor", offsetof(WindowSettings, customCursor), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "vsync", offsetof(WindowSettings, vsync), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "frameRate", offsetof(WindowSettings, frameRate), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo SystemData_MemberInfos[] = {
	{ TYPE_int, "coreCount", offsetof(SystemData, coreCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "fontHeight", offsetof(SystemData, fontHeight), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "maximized", offsetof(SystemData, maximized), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "killedFocus", offsetof(SystemData, killedFocus), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "setFocus", offsetof(SystemData, setFocus), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "windowIsFocused", offsetof(SystemData, windowIsFocused), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "vsyncTempTurnOff", offsetof(SystemData, vsyncTempTurnOff), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo Texture_MemberInfos[] = {
	{ TYPE_string, "name", offsetof(Texture, name), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_string, "file", offsetof(Texture, file), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_uint, "type", offsetof(Texture, type), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2i, "dim", offsetof(Texture, dim), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_uint, "format", offsetof(Texture, format), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "spriteSheet", offsetof(Texture, spriteSheet), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "spriteCount", offsetof(Texture, spriteCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2i, "cellDim", offsetof(Texture, cellDim), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "hasAlpha", offsetof(Texture, hasAlpha), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo FrameBuffer_MemberInfos[] = {
	{ TYPE_string, "name", offsetof(FrameBuffer, name), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec2i, "dim", offsetof(FrameBuffer, dim), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "hasRenderTargetView", offsetof(FrameBuffer, hasRenderTargetView), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "hasShaderResourceView", offsetof(FrameBuffer, hasShaderResourceView), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "hasDepthStencilView", offsetof(FrameBuffer, hasDepthStencilView), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "isShadow", offsetof(FrameBuffer, isShadow), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "makeDepthView", offsetof(FrameBuffer, makeDepthView), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo GraphicsSettings_MemberInfos[] = {
	{ TYPE_Vec2i, "cur3dBufferRes", offsetof(GraphicsSettings, cur3dBufferRes), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "msaaSamples", offsetof(GraphicsSettings, msaaSamples), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "resolutionScale", offsetof(GraphicsSettings, resolutionScale), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "aspectRatio", offsetof(GraphicsSettings, aspectRatio), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_int, "fieldOfView", offsetof(GraphicsSettings, fieldOfView), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "nearPlane", offsetof(GraphicsSettings, nearPlane), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "farPlane", offsetof(GraphicsSettings, farPlane), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo GraphicsMatrices_MemberInfos[] = {
	{ TYPE_Mat4, "view2d", offsetof(GraphicsMatrices, view2d), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Mat4, "ortho", offsetof(GraphicsMatrices, ortho), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Mat4, "view", offsetof(GraphicsMatrices, view), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Mat4, "proj", offsetof(GraphicsMatrices, proj), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Mat4, "viewInv", offsetof(GraphicsMatrices, viewInv), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Mat4, "projInv", offsetof(GraphicsMatrices, projInv), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo WalkManifoldSettings_MemberInfos[] = {
	{ TYPE_float, "playerRadius", offsetof(WalkManifoldSettings, playerRadius), 0, {}, {-1}, 0, 0, 0, 1, {{"Range", "0.1", "1.5"}}, -1, -1 },
	{ TYPE_float, "legHeight", offsetof(WalkManifoldSettings, legHeight), 0, {}, {-1}, 0, 0, 0, 1, {{"Range", "0.1", "1"}}, -1, -1 },
	{ TYPE_float, "playerHeight", offsetof(WalkManifoldSettings, playerHeight), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_float, "cellSize", offsetof(WalkManifoldSettings, cellSize), 0, {}, {-1}, 0, 0, 0, 1, {{"Range", "0.05", "1"}}, -1, -1 },
	{ TYPE_int, "edgeSearchIterations", offsetof(WalkManifoldSettings, edgeSearchIterations), 0, {}, {-1}, 0, 0, 0, 1, {{"Range", "1", "20"}}, -1, -1 },
	{ TYPE_float, "lineFlattenPercent", offsetof(WalkManifoldSettings, lineFlattenPercent), 0, {}, {-1}, 0, 0, 0, 1, {{"Range", "0", "0.5f"}}, -1, -1 },
	{ TYPE_int, "maxLineCollisionCount", offsetof(WalkManifoldSettings, maxLineCollisionCount), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "depthTest", offsetof(WalkManifoldSettings, depthTest), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "drawWalkEdges", offsetof(WalkManifoldSettings, drawWalkEdges), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "drawOutsideGrid", offsetof(WalkManifoldSettings, drawOutsideGrid), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_bool, "drawLinesNoCleanup", offsetof(WalkManifoldSettings, drawLinesNoCleanup), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec4, "cGrid", offsetof(WalkManifoldSettings, cGrid), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec4, "cGridOutside", offsetof(WalkManifoldSettings, cGridOutside), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec4, "cBlockerLine", offsetof(WalkManifoldSettings, cBlockerLine), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec4, "cBlockerLineNoCleanup", offsetof(WalkManifoldSettings, cBlockerLineNoCleanup), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
	{ TYPE_Vec4, "cWalkEdge", offsetof(WalkManifoldSettings, cWalkEdge), 0, {}, {-1}, 0, 0, 0, 0, {}, -1, -1 },
};

MemberInfo SkySettings_MemberInfos[] = {
	{ TYPE_Vec2, "sunAngles", offsetof(SkySettings, sunAngles), 0, {}, {-1}, 0, 0, 0, 1, {{"Range", "0", "360", "-90", "90"}}, 0, -1 },
	{ TYPE_float, "spotBrightness", offsetof(SkySettings, spotBrightness), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "mieBrightness", offsetof(SkySettings, mieBrightness), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "mieDistribution", offsetof(SkySettings, mieDistribution), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "mieStrength", offsetof(SkySettings, mieStrength), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "mieCollectionPower", offsetof(SkySettings, mieCollectionPower), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "rayleighBrightness", offsetof(SkySettings, rayleighBrightness), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "rayleighStrength", offsetof(SkySettings, rayleighStrength), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "rayleighCollectionPower", offsetof(SkySettings, rayleighCollectionPower), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "scatterStrength", offsetof(SkySettings, scatterStrength), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "surfaceHeight", offsetof(SkySettings, surfaceHeight), 0, {}, {-1}, 0, 0, 0, 1, {{"Range", "0", "1"}}, 0, -1 },
	{ TYPE_float, "intensity", offsetof(SkySettings, intensity), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_int, "stepCount", offsetof(SkySettings, stepCount), 0, {}, {-1}, 0, 0, 0, 0, {}, 0, -1 },
	{ TYPE_float, "horizonOffset", offsetof(SkySettings, horizonOffset), 0, {}, {-1}, 0, 0, 0, 1, {{"Range", "0", "1"}}, 0, -1 },
	{ TYPE_float, "sunOffset", offsetof(SkySettings, sunOffset), 0, {}, {-1}, 0, 0, 0, 1, {{"Range", "0", "1"}}, 0, -1 },
};

StructInfo structInfos[] = {
	{ "u64", sizeof(u64), 0, 0, -1, 0, },
	{ "i64", sizeof(i64), 0, 0, -1, 0, },
	{ "int", sizeof(int), 0, 0, -1, 0, },
	{ "uint", sizeof(uint), 0, 0, -1, 0, },
	{ "short", sizeof(short), 0, 0, -1, 0, },
	{ "ushort", sizeof(ushort), 0, 0, -1, 0, },
	{ "char", sizeof(char), 0, 0, -1, 0, },
	{ "uchar", sizeof(uchar), 0, 0, -1, 0, },
	{ "float", sizeof(float), 0, 0, -1, 0, },
	{ "double", sizeof(double), 0, 0, -1, 0, },
	{ "bool", sizeof(bool), 0, 0, -1, 0, },
	{ "string", sizeof(char*), 0, 0, -1, 0, },
	{ "MouseEvents", sizeof(MouseEvents), 7, MouseEvents_MemberInfos, -1, 0, },
	{ "AppData", sizeof(AppData), 45, AppData_MemberInfos, -1, 0, },
	{ "Audio", sizeof(Audio), 9, Audio_MemberInfos, -1, 0, },
	{ "Track", sizeof(Track), 13, Track_MemberInfos, offsetof(Track, used), 0, },
	{ "Sound", sizeof(Sound), 10, Sound_MemberInfos, -1, 0, },
	{ "AudioState", sizeof(AudioState), 14, AudioState_MemberInfos, -1, 0, },
	{ "Camera", sizeof(Camera), 6, Camera_MemberInfos, -1, 0, },
	{ "DArray_int", sizeof(DArray<int>), 5, DArray_int_MemberInfos, -1, 0, },
	{ "DArray_Entity", sizeof(DArray<Entity>), 5, DArray_Entity_MemberInfos, -1, 0, },
	{ "Entity", sizeof(Entity), 25, Entity_MemberInfos, -1, 3, },
	{ "EntityManager", sizeof(EntityManager), 3, EntityManager_MemberInfos, -1, 0, },
	{ "Font", sizeof(Font), 14, Font_MemberInfos, -1, 0, },
	{ "GraphicsState", sizeof(GraphicsState), 22, GraphicsState_MemberInfos, -1, 0, },
	{ "Input", sizeof(Input), 23, Input_MemberInfos, -1, 0, },
	{ "Vec2", sizeof(Vec2), 2, Vec2_MemberInfos, -1, 0, },
	{ "Vec2i", sizeof(Vec2i), 2, Vec2i_MemberInfos, -1, 0, },
	{ "Vec3", sizeof(Vec3), 3, Vec3_MemberInfos, -1, 0, },
	{ "Vec3i", sizeof(Vec3i), 3, Vec3i_MemberInfos, -1, 0, },
	{ "Vec4", sizeof(Vec4), 4, Vec4_MemberInfos, -1, 0, },
	{ "Mat4", sizeof(Mat4), 16, Mat4_MemberInfos, -1, 0, },
	{ "Quat", sizeof(Quat), 4, Quat_MemberInfos, -1, 0, },
	{ "XForm", sizeof(XForm), 3, XForm_MemberInfos, -1, 0, },
	{ "Line2", sizeof(Line2), 2, Line2_MemberInfos, -1, 0, },
	{ "Line3", sizeof(Line3), 2, Line3_MemberInfos, -1, 0, },
	{ "Rect", sizeof(Rect), 4, Rect_MemberInfos, -1, 0, },
	{ "Recti", sizeof(Recti), 4, Recti_MemberInfos, -1, 0, },
	{ "Rect3", sizeof(Rect3), 2, Rect3_MemberInfos, -1, 0, },
	{ "Rect3i", sizeof(Rect3i), 2, Rect3i_MemberInfos, -1, 0, },
	{ "GameSettings", sizeof(GameSettings), 7, GameSettings_MemberInfos, -1, 0, },
	{ "MainMenu", sizeof(MainMenu), 19, MainMenu_MemberInfos, -1, 0, },
	{ "Object", sizeof(Object), 7, Object_MemberInfos, -1, 0, },
	{ "SpawnRegion", sizeof(SpawnRegion), 5, SpawnRegion_MemberInfos, -1, 0, },
	{ "ValueRange_float", sizeof(ValueRange<float>), 3, ValueRange_float_MemberInfos, -1, 0, },
	{ "ValueRange_Vec4", sizeof(ValueRange<Vec4>), 3, ValueRange_Vec4_MemberInfos, -1, 0, },
	{ "ParticleSettings", sizeof(ParticleSettings), 24, ParticleSettings_MemberInfos, -1, 0, },
	{ "ParticleEmitter", sizeof(ParticleEmitter), 10, ParticleEmitter_MemberInfos, -1, 1, },
	{ "MonitorData", sizeof(MonitorData), 2, MonitorData_MemberInfos, -1, 0, },
	{ "WindowSettings", sizeof(WindowSettings), 16, WindowSettings_MemberInfos, -1, 0, },
	{ "SystemData", sizeof(SystemData), 7, SystemData_MemberInfos, -1, 0, },
	{ "Texture", sizeof(Texture), 9, Texture_MemberInfos, -1, 0, },
	{ "FrameBuffer", sizeof(FrameBuffer), 7, FrameBuffer_MemberInfos, -1, 0, },
	{ "GraphicsSettings", sizeof(GraphicsSettings), 7, GraphicsSettings_MemberInfos, -1, 0, },
	{ "GraphicsMatrices", sizeof(GraphicsMatrices), 6, GraphicsMatrices_MemberInfos, -1, 0, },
	{ "WalkManifoldSettings", sizeof(WalkManifoldSettings), 16, WalkManifoldSettings_MemberInfos, -1, 0, },
	{ "SkySettings", sizeof(SkySettings), 15, SkySettings_MemberInfos, -1, 0, },
};

