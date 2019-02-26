
enum FileType {
	FILE_TYPE_FILE = 0,
	FILE_TYPE_FOLDER,
};

struct FolderSearchData {
	WIN32_FIND_DATAA findData;
	HANDLE folderHandle;

	char* folder;

	bool isFolder;

	char* file;
	char* fileName;
	char* fileExtension;
	char* filePath;
	int type;
};

struct RecursiveFolderSearchData {
	FolderSearchData data[10];

	int index;
	char* startFolder;
	int strSizes[10];
	char folder[200];

	char* filePath;
	char* fileName;
};

bool folderSearchStart(FolderSearchData* fd, char* fileFolder) {	
	// Remember, for searching folder add "*" at the end of path

	fd->folder = fileFolder;
	char* folderWithStarAtEnd = fString("%s*", fileFolder);

	fd->folderHandle = FindFirstFileA(folderWithStarAtEnd, &fd->findData);

	if(fd->folderHandle != INVALID_HANDLE_VALUE) return true;
	else return false;
}

bool folderSearchNextFile(FolderSearchData* fd) {
	if(FindNextFileA(fd->folderHandle, &fd->findData) == 0) {
		FindClose(fd->folderHandle);
		return false;
	}

	if(strCompare(fd->findData.cFileName, "..")) {
		return folderSearchNextFile(fd); // Skip ".."
	}

	if(flagGet(fd->findData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
		fd->type = FILE_TYPE_FOLDER;
	} else {
		fd->type = FILE_TYPE_FILE;
	}

	fd->file = fd->findData.cFileName;

	int nameSize = getFileName(fd->file);
	if(nameSize == 0) {
		fd->isFolder = true;
	} else {
		fd->isFolder = false;

		fd->fileName = getTString(fd->file, nameSize);
		fd->fileExtension = getFileExtension(fd->file);
		fd->filePath = fString("%s%s", fd->folder, fd->file);
	}

	return true;
}

void recursiveFolderSearchStart(RecursiveFolderSearchData* rfd, char* folder) {

	rfd->index = 0;
	rfd->startFolder = folder;
	strClear(rfd->folder);

	char* folderPath = fString("%s*", folder);
	folderSearchStart(rfd->data + rfd->index, folderPath);
}

bool recursiveFolderSearchNext(RecursiveFolderSearchData* rfd) {

	for(;;) {
		FolderSearchData* fd = rfd->data + rfd->index;

		for(;;) {
			bool result = folderSearchNextFile(fd);
			if(!result) {
				if(rfd->index == 0) return false;
				else {
					// Pop stack.
					rfd->index--;
					int index = rfd->strSizes[rfd->index];
					index = strLen(rfd->folder) - index;
					if(rfd->index > 0) index--;
					rfd->folder[index] = '\0';

					fd = rfd->data + rfd->index;
				}
			} else {
				break;
			}
		}

		if(fd->type == FILE_TYPE_FOLDER) {
			// Push stack.

			rfd->strSizes[rfd->index] = strLen(fd->file);
			if(rfd->index > 0) strAppend(rfd->folder, "\\");
			strAppend(rfd->folder, fd->file);

			rfd->index++;

			char* folderPath = fString("%s%s\\*", rfd->startFolder, rfd->folder);
			folderSearchStart(rfd->data + rfd->index, folderPath);

		} else {
			rfd->filePath = fString("%s%s%s%s", rfd->startFolder, rfd->folder, strLen(rfd->folder)?"\\":"", fd->file);
			rfd->fileName = fString("%s%s%s", rfd->folder, strLen(rfd->folder)?"\\":"", fd->file);

			break;
		}
	}

	return true;
}

int folderFileCount(char* folder) {
	FolderSearchData fd;
	folderSearchStart(&fd, folder);
	int count = 0;
	while(folderSearchNextFile(&fd)) {
		if(fd.type == FILE_TYPE_FILE) count++;
	}

	return count;
}