
void watchFoldersInit(SystemData* sd, GraphicsState* gs) {
	char* folders[] = {App_Texture_Folder, App_Mesh_Folder};
	// initWatchFolders(sd, folders, arrayCount(folders));
	int folderCount = arrayCount(folders);

	{
		for(int i = 0; i < folderCount; i++) {
			char* folder = folders[i];
			HANDLE fileChangeHandle = FindFirstChangeNotification(folder, true, FILE_NOTIFY_CHANGE_LAST_WRITE);
			if(fileChangeHandle == INVALID_HANDLE_VALUE) {
				printf("Could not set folder change notification.\n");
			}
			sd->folderHandles[i] = fileChangeHandle;
		}
		sd->folderHandleCount = folderCount;
	}

	for(int i = 0; i < gs->textureCount; i++) {
		Texture* tex = gs->textures + i;
		char* path = tex->file;
		tex->assetInfo.lastWriteTime = getLastWriteTime(path);
	}

	for(int i = 0; i < gs->meshCount; i++) {
		Mesh* m = gs->meshes + i;
		char* path = m->file;
		m->assetInfo[0].lastWriteTime = getLastWriteTime(path);
		m->assetInfo[1].lastWriteTime = getLastWriteTime(m->mtlFile);
	}
}

void reloadChangedFiles(SystemData* sd) {
	// Todo: Get ReadDirectoryChangesW to work instead of FindNextChangeNotification.
	// Right now we check every single file in the folder for change.

	GraphicsState* gs = theGState;

	DWORD fileStatus = WaitForMultipleObjects(sd->folderHandleCount, &sd->folderHandles[0], false, 0);
	if(between(fileStatus, WAIT_OBJECT_0, WAIT_OBJECT_0 + sd->folderHandleCount)) {

		// Note: WAIT_OBJECT_0 is defined as 0, so we can use it as an index.
		int folderIndex = fileStatus;

		FindNextChangeNotification(sd->folderHandles[folderIndex]);

		if(folderIndex == 0) {
			for(int i = 0; i < gs->textureCount; i++) {
				Texture* tex = gs->textures + i;

				FILETIME newWriteTime = getLastWriteTime(tex->file);
				if(CompareFileTime(&tex->assetInfo.lastWriteTime, &newWriteTime) != 0) {

					dxLoadAndCreateTexture(tex);

					tex->assetInfo.lastWriteTime = newWriteTime;
				}
			}
		} else if(folderIndex == 1) {

			for(int i = 0; i < gs->meshCount; i++) {
				Mesh* m = gs->meshes + i;

				char* files[] = {m->file, m->mtlFile};
				for(int j = 0; j < arrayCount(m->assetInfo); j++) {
					FILETIME newWriteTime = getLastWriteTime(files[j]);

					if(CompareFileTime(&m->assetInfo[j].lastWriteTime, &newWriteTime) != 0) {

						ObjLoader parser;
						dxLoadMesh(m, &parser);
						parser.free();

						m->assetInfo[j].lastWriteTime = newWriteTime;
					}
				}
			}
		}
	}
}
