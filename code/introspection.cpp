
enum ArrayType {
	ARRAYTYPE_CONSTANT, 
	ARRAYTYPE_DYNAMIC, 
};

enum ArraySize {
	ARRAYSIZE_CONST,
	ARRAYSIZE_VAR,
	ARRAYSIZE_STRING,
};

struct ArrayInfo {
	int type;
	int sizeMode;

	union {
		int size;
		int offset;
	};

	int constSize;
};

struct StructMemberInfo {
	char* name;
	int type;
	int offset;
	int arrayCount;
	ArrayInfo arrays[2];
};

struct StructInfo {
	char* name;
	int size;
	int memberCount;
	StructMemberInfo* list;
};

StructMemberInfo initMemberInfo(char* name, int type, int offset) {
	StructMemberInfo info;
	info.name = name;
	info.type = type;
	info.offset = offset;
	info.arrayCount = 0;

	return info;
}

StructMemberInfo initMemberInfo(char* name, int type, int offset, ArrayInfo aInfo1, ArrayInfo aInfo2 = {-1}) {
	StructMemberInfo info = initMemberInfo(name, type, offset);
	info.arrayCount = 1;
	if(aInfo2.type != -1) info.arrayCount = 2;

	info.arrays[0] = aInfo1;
	info.arrays[1] = aInfo2;

	return info;
}

//

struct TestStruct {
	int a;
	float b;
	Vec3 c;
	int d[2];
	float* e;
	int f;
	int* g[2];
	int h;
};

#define ATOMTYPELIST \
	TYPEFUNC(int)    \
	TYPEFUNC(float)  \
	TYPEFUNC(char)   \
	TYPEFUNC(uchar)  \
	TYPEFUNC(bool)

#define TYPELIST  \
	TYPEFUNC(Vec2i) \
	TYPEFUNC(Vec3) \
	TYPEFUNC(Vec4) \
	TYPEFUNC(Quat) \
	TYPEFUNC(Entity) \
	TYPEFUNC(TestStruct)

#define TYPEFUNC(type) TYPE_##type,
enum StructType {
	FIRST_TYPE = -1, // Fantastic.

	ATOMTYPELIST
	TYPELIST

	TYPE_SIZE,
};
#undef TYPEFUNC

#define MEMBER_INFO(type, name) \
	initMemberInfo(#name, TYPE_##type, offsetof(CURRENT_TYPE, name))

#define MEMBER_INFOACS(type, name) \
	initMemberInfo(#name, TYPE_##type, offsetof(CURRENT_TYPE, name), \
	               {ARRAYTYPE_CONSTANT, ARRAYSIZE_STRING, 0, memberArrayCount(CURRENT_TYPE, name)})

#define MEMBER_INFOADS(type, name) \
	initMemberInfo(#name, TYPE_##type, offsetof(CURRENT_TYPE, name), \
	               {ARRAYTYPE_DYNAMIC, ARRAYSIZE_STRING})

#define MEMBER_INFOACC(type, name) \
	initMemberInfo(#name, TYPE_##type, offsetof(CURRENT_TYPE, name), \
	               {ARRAYTYPE_CONSTANT, ARRAYSIZE_CONST, memberArrayCount(CURRENT_TYPE, name), memberArrayCount(CURRENT_TYPE, name)})

#define MEMBER_INFOADV(type, name, sizeVar) \
	initMemberInfo(#name, TYPE_##type, offsetof(CURRENT_TYPE, name), \
	               {ARRAYTYPE_DYNAMIC, ARRAYSIZE_VAR, offsetof(CURRENT_TYPE, sizeVar)})

#define MEMBER_INFOACCADV(type, name, sizeVar) \
	initMemberInfo(#name, TYPE_##type, offsetof(CURRENT_TYPE, name), \
	               {ARRAYTYPE_CONSTANT, ARRAYSIZE_CONST, memberArrayCount(CURRENT_TYPE, name), memberArrayCount(CURRENT_TYPE, name)}, \
	               {ARRAYTYPE_DYNAMIC, ARRAYSIZE_VAR, offsetof(CURRENT_TYPE, sizeVar)})

#define MAKE_MEMBER_ARRAY(type) StructMemberInfo (type##StructMemberInfos)[] 
#define MEMBER_ARRAY(type) MAKE_MEMBER_ARRAY(type)

//

#define CURRENT_TYPE Vec2i
MEMBER_ARRAY(CURRENT_TYPE) = {
	MEMBER_INFO( int, x ),
	MEMBER_INFO( int, y ),
};

#define CURRENT_TYPE Vec3
MEMBER_ARRAY(CURRENT_TYPE) = {
	MEMBER_INFO( float, x ),
	MEMBER_INFO( float, y ),
	MEMBER_INFO( float, z ),
};

#define CURRENT_TYPE Vec4
MEMBER_ARRAY(CURRENT_TYPE) = {
	MEMBER_INFO( float, x ),
	MEMBER_INFO( float, y ),
	MEMBER_INFO( float, z ),
	MEMBER_INFO( float, w ),
};

#define CURRENT_TYPE Quat
MEMBER_ARRAY(CURRENT_TYPE) = {
	MEMBER_INFO( float, x ),
	MEMBER_INFO( float, y ),
	MEMBER_INFO( float, z ),
	MEMBER_INFO( float, w ),
};

#define CURRENT_TYPE Entity
MEMBER_ARRAY(CURRENT_TYPE) = {
	MEMBER_INFO( bool, init ),
	MEMBER_INFO( int, type ),
	MEMBER_INFO( int, id ),
	MEMBER_INFOACS( char, name ),
	MEMBER_INFO( Vec3, pos ),
	MEMBER_INFO( Vec3, dir ),
	MEMBER_INFO( Vec3, rot ),
	MEMBER_INFO( float, rotAngle ),
	MEMBER_INFO( Vec3, dim ),
	MEMBER_INFO( Vec3, vel ),
	MEMBER_INFO( Vec3, acc ),
	MEMBER_INFO( int, movementType ),
	MEMBER_INFO( int, spatial ),
	MEMBER_INFO( bool, onGround ),
	MEMBER_INFO( bool, deleted ),
	MEMBER_INFO( bool, isMoving ),
	MEMBER_INFO( bool, isColliding ),
	MEMBER_INFO( Vec3, camOff ),
};

#define CURRENT_TYPE TestStruct
MEMBER_ARRAY(CURRENT_TYPE) = {
	MEMBER_INFO( int, a ),
	MEMBER_INFO( float, b ),
	MEMBER_INFO( Vec3, c ),
	MEMBER_INFOACC( int, d ),
	MEMBER_INFOADV( float, e, f ),
	MEMBER_INFO( int, f ),
	MEMBER_INFOACCADV( int, g, h ),
	MEMBER_INFO( int, h ),
};

#undef CURRENT_TYPE

StructInfo structInfos[] = {
	#define TYPEFUNC(type) { #type, sizeof(type), 0 },
	ATOMTYPELIST

	#define TYPEFUNC(type) { #type, sizeof(type), arrayCount(type##StructMemberInfos), type##StructMemberInfos },
	TYPELIST

	#undef TYPEFUNC
};

// #define getStructInfo(typeName) (structInfos + TYPE_##typeName)

#define getType(typeName) TYPE_##typeName

StructInfo* getStructInfo(int type) {
	return structInfos + type;
}

//

bool typeIsPrimitive(StructInfo* info) {
	return info->memberCount == 0;
}

bool typeIsPrimitive(int type) {
	return structInfos[type].memberCount == 0;
}

char* castTypeArray(char* base, ArrayInfo aInfo) {
	char* arrayBase = (aInfo.type == ARRAYTYPE_CONSTANT) ? base :*((char**)(base));
	return arrayBase;
}

int getTypeArraySize(char* structBase, ArrayInfo aInfo, char* arrayBase = 0) {
	int arraySize;
	if(aInfo.sizeMode == ARRAYSIZE_CONST) arraySize = aInfo.size;
	else if(aInfo.sizeMode == ARRAYSIZE_VAR) arraySize = *(int*)(structBase + aInfo.offset);
	else if(aInfo.sizeMode == ARRAYSIZE_STRING) arraySize = strLen(arrayBase);

	return arraySize;
}

//

enum TraverseType {
	TRAVERSETYPE_PRIMITIVE = 0,
	TRAVERSETYPE_PUSH,
	TRAVERSETYPE_POP,
};

struct TypeTraverseStackElement {
	int type;
	char* data;
	int index;

	bool isArray;
	int arraySize;
};

struct TypeTraverse {
	TypeTraverseStackElement stack[20];
	int stackCount;

	bool init;

	//

	int savedStackCount;
	int savedIndex;
	int savedAction;

	// These get set after every step.

	bool first;
	int action;
	bool isArray;

	char* typeName;
	char* memberName;

	char* data;
	int memberType;

	// For printing.
	bool firstMember;
	bool lastActionWasPush;

	//

	void start(int type, char* data, char* name);
	bool next();
	void saveState();
	void loadState();
};

void TypeTraverse::start(int type, char* data, char* name) { 
	*this = {}; 

	init = true;
	stack[stackCount++] = {type, data, 0, false};
	memberName = name;
};

bool TypeTraverse::next() {

	if(first) first = false;

	if(init) {
		init = false;
		first = true;

		TypeTraverseStackElement* e = stack + stackCount-1;

		action = TRAVERSETYPE_PUSH;
		isArray = false;
		typeName = getStructInfo(e->type)->name;

		return true;
	}

	if(stackCount == 0) return false;

	TypeTraverseStackElement* e = stack + stackCount-1;

	lastActionWasPush = action == TRAVERSETYPE_PUSH;

	isArray = e->isArray;

	int maxCount = isArray ? e->arraySize : getStructInfo(e->type)->memberCount;
	if(e->index < maxCount) {

		StructMemberInfo* structMemberInfo = getStructInfo(e->type)->list + e->index;

		StructMemberInfo smi;
		if(isArray) {
			smi = {fillString("[%i]", e->index), e->type, getStructInfo(e->type)->size * e->index, 0};
			structMemberInfo = &smi;
		}

		StructInfo* memberInfo = getStructInfo(structMemberInfo->type);

		bool memberIsPrimitive = typeIsPrimitive(structMemberInfo->type) && structMemberInfo->arrayCount == 0;
		if(memberIsPrimitive) {
			action = TRAVERSETYPE_PRIMITIVE;
			typeName = getStructInfo(structMemberInfo->type)->name;
			memberName = structMemberInfo->name;
			memberType = structMemberInfo->type;
			firstMember = e->index == 0;

			data = e->data + structMemberInfo->offset;

		} else {
			action = TRAVERSETYPE_PUSH;
			memberName = structMemberInfo->name;

			if(structMemberInfo->arrayCount == 0) {
				typeName = memberInfo->name;
				stack[stackCount++] = {structMemberInfo->type, e->data + structMemberInfo->offset, 0, false};

			} else {
				ArrayInfo aInfo = structMemberInfo->arrays[0];

				char* arrayBase = castTypeArray(e->data + structMemberInfo->offset, aInfo);
				int arraySize = getTypeArraySize(e->data, aInfo, arrayBase);

				if(aInfo.type == ARRAYTYPE_CONSTANT) {
					typeName = fillString("%s [%i]", memberInfo->name, aInfo.constSize);
				} else {
					typeName = fillString("%s *", memberInfo->name);
				}

				stack[stackCount++] = {structMemberInfo->type, arrayBase, 0, true, arraySize};
			}
		}

		e->index++;

	} else {
		action = TRAVERSETYPE_POP;
		stackCount--;
	}

	return true;
}

void TypeTraverse::saveState() {
	savedStackCount = stackCount;
	savedIndex = stack[stackCount-1].index;
	savedAction = action;
}

void TypeTraverse::loadState() {
	stackCount = savedStackCount;
	stack[stackCount-1].index = savedIndex;
	action = savedAction;
}

struct ExpansionIndex {
	int indices[10];
	int count;

	//

	bool operator==(ExpansionIndex b) { 
		if(this->count != b.count) return false;

		for(int i = 0; i < this->count; i++) {
			if(this->indices[i] != b.indices[i]) {
				return false;
			}
		}

		return true;
	};
};

//

void printTypeY(int type, char* data, int expandLevel = 0, char* name = "") {

	TypeTraverse info;
	info.start(type, data, name);

	if(strLen(name) != 0) printf("%s:\n", name);

	int level = expandLevel;
	int indentSize = 2;
	int indent = 0;

	while(info.next()) {
		if(info.action == TRAVERSETYPE_PUSH) {
			if(indent < level) {
				printf("%*s", indentSize*(indent), "");
				printf(!info.isArray ? "{" : "[");
				printf("\n");

			} else {
				if(indent == level) {
					printf("%*s", indentSize*(indent), "");
					printf(!info.isArray ? "{" : "[");

				} else {
					if(!info.lastActionWasPush && indent != level)
						printf(!info.isArray ? "," : ",");
					printf(!info.isArray ? " {" : " [");
				}
			}
			indent++;

		} else if(info.action == TRAVERSETYPE_POP) {
			indent--;

			if(indent < level) {
				printf("%*s", indentSize*(indent), "");
				printf(!info.isArray ? "}" : "]");
				printf("\n");

			} else {
				printf(!info.isArray ? " }" : " ]");
				if(indent == level) printf("\n");
			}

		} else if(info.action == TRAVERSETYPE_PRIMITIVE) {
			if(indent-1 < level) printf("%*s", indentSize*(indent), "");
			else printf(info.firstMember ? " " : ", ");

			if(!info.isArray) {
				if(indent-1 < level) printf(fillString("%s = ", info.memberName));
				else printf(fillString("%s=", info.memberName));
			}

			char* data = info.data;
			switch(info.memberType) {
				case TYPE_int:   printf("%i", *(int*)data); break;
				case TYPE_float: printf("%g", *(float*)data); break;
				case TYPE_char:  printf("'%c'", *data); break;
				case TYPE_uchar: printf("%u", *(uchar*)data); break;
				case TYPE_bool:  printf(*((bool*)data) ? "true" : "false"); break;
				default: break;
			};

			if(indent-1 < level) printf("\n");
		}
	}

	printf("\n");
}