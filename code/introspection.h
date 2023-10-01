
#define Meta_Parse_Struct(ver)
#define Meta_Parse_Enum()

enum ArrayType {
	ARRAYTYPE_CONSTANT = 0,
	ARRAYTYPE_DYNAMIC, 
};

enum ArraySize {
	ARRAYSIZE_CONST = 0,
	ARRAYSIZE_VAR,
	ARRAYSIZE_STRING,
};

char* arrayTypeStrings[] = {
	"ARRAYTYPE_CONSTANT", 
	"ARRAYTYPE_DYNAMIC", 	
};

char* arraySizeStrings[] = {
	"ARRAYSIZE_CONST",
	"ARRAYSIZE_VAR",
	"ARRAYSIZE_STRING",
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

struct EnumInfo {
	int value;
	char* name;
	char* nameWithoutTag;
};

struct UnionInfo {
	int type;
	int offset;
	char* varName;
};

struct StructInfo* getStructInfo(int type);

struct MemberInfo {
	int type;
	char* name;
	int offset;

	int arrayCount;
	ArrayInfo arrays[2];

	UnionInfo uInfo;

	int enumCount;
	EnumInfo* enumInfos;

	bool hide;

	int tagCount;
	char* tags[2][5];

	int vMin;
	int vMax;

	struct StructInfo* structInfo() { return getStructInfo(type); }
	bool isPrimitive();
	// MemberInfo* get(int memberIndex) { return getStructInfo(type)->list + memberIndex; };
};

struct StructInfo {
	char* name;
	int size;
	int memberCount;
	MemberInfo* list;

	int initBoolOffset;
	int version;

	bool isPrimitive() { return memberCount == 0; }
};

struct MemberInfoData {
	MemberInfo mInfo;
	char* data;
	bool noHistoryChange;
};

//

enum TraverseType {
	TRAVERSETYPE_PRIMITIVE = 0,
	TRAVERSETYPE_PUSH,
	TRAVERSETYPE_POP,
};

struct TypeTraverseStackElement {
	int type;
	MemberInfo* mInfo;

	char* data;
	char* structBase;
	int arrayIndex;

	int memberIndex;
	int memberCount;

	bool firstMember;
};

struct TypeTraverse {
	TypeTraverseStackElement stack[20];
	TypeTraverseStackElement* e;
	int stackCount;

	MemberInfo startMemberInfo;

	bool ignoreHiddenMembers;
	int temp;

	int savedStackCount;
	int savedMemberIndex;

	//
	
	MemberInfo* mInfo;
	int memberType;

	int type;
	char* data;

	//

	void start(int type, void* data, char* name);
	void start(MemberInfo* mInfo, void* structBase, char* name = "");

	bool valid();
	bool validGet();
	bool end();
	
	void get();
	bool insideArray();
	char* typeName();
	char* memberName();

	bool next();
	void getNext();
	void push();
	bool pop();
	bool popNext();
	void skip();

	void save();
	void load();
};

struct ExpansionIndex {
	int indices[10];
	int count;

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

// struct NewMemberThing {
// 	int type;
// 	void* data;

// 	NewMemberThing(char* typeName, void* data) {
// 		this->type = getTypeByName(typeName);
// 		this->data = data;
// 	}
// };

// int getTypeByName(char* typeName) {
