
#define MC_Parse      "Meta_Parse_Struct"
#define MC_Version    "@V"
#define MC_SizeString "@String"
#define MC_SizeVar    "@SizeVar"
#define MC_Union      "@Union"
#define MC_Ignore     "@Ignore"
#define MC_Enum       "@Enum"
#define MC_Hide       "@Hide"
#define MC_Init       "@Init"
#define MC_Tag        "@Tag"
#define MC_ParseEnum  "Meta_Parse_Enum"
#define MC_Size       "@Size"

char* metaTypeList[] = {
	"u64",   "i64",
	"int",   "uint",
	"short", "ushort",
	"char",  "uchar",
	"float", "double",
	"bool",  "string"
};

struct MetaMember;
struct MetaStruct {
	char* name;
	DArray<MetaMember> members;

	bool isTemplate;
	char* templateType;

	DArray<char*> usedTemplateTypes;
	DArray<MetaMember*> usedTemplateMembers;

	int version;

	char* initBoolOffsetString;
};

struct MetaMember {
	char* type;
	char* name;
	int pointerCount;
	int constArrayCount;

	bool arraySizeString;
	char* arraySizeVar;

	int minVersion, maxVersion;

	bool usesTemplate;
	char* templateType;
	char* typeWithUnderscores;
	char* typeWithoutTemplate;

	bool usesUnion;
	char* unionVar;
	char* unionType;

	bool isEnum;
	char* enumName;

	bool hide;

	int tagCount;
	char* tags[2][5];

	bool structIsInit;
};

struct MetaEnum {
	char* name;
	DArray<char*> members;
	char* sizeName;
};

void createMetaDataFromCodeFolder(char** folderPaths, int folderCount, char* genFile);
void metaParseFile(char* file, DArray<MetaStruct>* structs, DArray<MetaEnum>* enums);
MetaStruct metaParseStruct(char* str, bool skipVersionHeader = false);
MetaEnum metaParseEnum(char* str);
void buildMetaData(DArray<MetaStruct>* structs, DArray<MetaEnum>* enums, DArray<char>* fileString);
