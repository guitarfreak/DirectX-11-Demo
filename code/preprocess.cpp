
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
// #include <windows.h>
#include "external\win32\atomic.h"
#include "external\win32\fiber.h"
#include "external\win32\file.h"
#include "external\win32\io.h"

struct MemoryBlock;
MemoryBlock* theMemory;

#define STB_SPRINTF_IMPLEMENTATION
#include "external\stb_sprintf.h"

#define Code_Folder "..\\code\\"
#define Shader_Folder "..\\code\\"

#include "mathBasic.cpp"

#include "misc.cpp"
#include "fileIO.cpp"
#include "string.cpp"
#include "memory.cpp"
#include "appMemory.cpp"
#include "misc2.cpp"

#include "introspection.h"
#include "container.cpp"
#include "platformMisc.cpp"

#include "metaParser.cpp"

int main(int argc, char **argv) {

	MemoryArray tMemory = {};
	initMemoryArray(&tMemory, megaBytes(10));

	MemoryBlock gMemory = {};
	gMemory.tMemory = &tMemory;
	theMemory = &gMemory;
	setMemory();

	//

	char* folder = Code_Folder;
	char* genFilePath = Code_Folder "generated\\metadata.cpp";
	char* shaderFilePath = Shader_Folder "generated\\metadata.cpp";

	int folderCount = 0;
	char* folders[5];
	folders[folderCount++] = Code_Folder;
	folders[folderCount++] = Code_Folder "shader\\";

	createMetaDataFromCodeFolder(folders, folderCount, genFilePath);

	return 0;
}