
int createFileAndOverwrite(char* fileName, int fileSize) {
	FILE* file = fopen(fileName, "wb");
	if(file == 0) return -1;

	fileSize += 1;

	char* tempBuffer = (char*)mallocX(fileSize);
	zeroMemory(tempBuffer, fileSize);
	fwrite(tempBuffer, fileSize, 1, file);
	free(tempBuffer);

	fclose(file);

	return 0;
}

int fileSize(char* fileName) {
	FILE* file = fopen(fileName, "rb");

	if(file == 0) return -1;

	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fclose(file);

	return size;
}

inline int fileSize(FILE* file) {
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);

	return size;
}

bool fileExists(char* fileName) {
	bool result = false;
	FILE* file = fopen(fileName, "rb");

	if(file) {
		result = true;
		fclose(file);
	}
	
	return result;
}

int readFileToBuffer(char* buffer, char* fileName) {
	FILE* file = fopen(fileName, "rb");
	if(file == 0) return -1;

	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);

	fread(buffer, size, 1, file);

	fclose(file);
	return size;
}

int readFileToBufferMalloc(char* fileName, char** buffer) {
	int size = fileSize(fileName);
	char* buf = (char*)mallocX(sizeof(char)*size);

	readFileToBuffer(buf, fileName);

	*buffer = buf;

	return size;
}

char* readFileToBufferZeroTerminated(char* fileName, void* (*alloc) (int) = 0) {
	FILE* file = fopen(fileName, "rb");
	if(file == 0) return 0;

	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* buffer = alloc ? (char*)alloc(size+1) : (char*)mallocX(size+1);

	fread(buffer, size, 1, file);

	buffer[size] = '\0';

	fclose(file);
	return buffer;
}

int readFileSectionToBuffer(char* buffer, char* fileName, int offsetInBytes, int sizeInBytes) {
	FILE* file = fopen(fileName, "r+b");
	if(file == 0) return -1;

	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);

	myAssert(offsetInBytes+sizeInBytes < size);

	fseek(file, offsetInBytes, SEEK_SET);
	fread(buffer, sizeInBytes, 1, file);

	fclose(file);
	return size;
}

int writeBufferToFile(char* buffer, char* fileName, int size = -1) {
	if(size == -1) size = strLen(buffer);

	FILE* file = fopen(fileName, "wb");
	if(!file) {
		printf("Could not open file!\n");
		return -1;
	} 

	fwrite(buffer, size, 1, file);

	fclose(file);
	return size;
}

int writeBufferSectionToFile(char* buffer, char* fileName, int offsetInBytes, int sizeInBytes) {
	FILE* file = fopen(fileName, "r+b");
	if(!file) {
		printf("Could not open file!\n");
		return -1;
	} 

	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);

	myAssert(offsetInBytes+sizeInBytes < size);

	fseek(file, offsetInBytes, SEEK_SET);
	fwrite(buffer, sizeInBytes, 1, file);

	fclose(file);
	return size;
}

void writeDataToFile(char* data, int size, char* fileName) {
	FILE* file = fopen(fileName, "wb");
	fwrite(data, size, 1, file);
	fclose(file);
}

bool readDataFromFile(char* data, char* fileName) {
	FILE* file = fopen(fileName, "rb");

	if(!file) return false;

	int size = fileSize(file);
	fread(data, size, 1, file);
	fclose(file);

	return true;
}

int getFileName(char* file) {
	char* ext = strrchr(file, '.');
	if(!ext) return 0;

	return ext - file;
}

char* getFileExtension(char* file) {
	char* ext = strrchr(file, '.');
	if(!ext) return 0;
	else ext++;

	return ext;
}
