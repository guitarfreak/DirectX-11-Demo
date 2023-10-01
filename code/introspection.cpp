
#include "generated\metadata.cpp"

#define getType(typeName) TYPE_##typeName
#define getEnumInfo(enumName) enumName##_EnumInfos
#define getEnumInfoCount(enumName) arrayCount(enumName##_EnumInfos)

StructInfo* getStructInfo(int type) {
	return structInfos + type;
}

bool typeIsPrimitive(int type) {
	return structInfos[type].memberCount == 0;
}

int getTypeByName(char* typeName) {
	for(int i = 0; i < arrayCount(structInfos); i++) {
		if(strCompare(structInfos[i].name, typeName)) return i;
	}

	return -1;
}

//

bool MemberInfo::isPrimitive() { 
	return structInfo()->isPrimitive(); 
}

//

bool typeIsPrimitive(MemberInfo* mInfo) {
	return mInfo->structInfo()->isPrimitive();
}

bool memberIsVersioned(MemberInfo* mInfo, int version) {
	return mInfo->vMin != -1 &&  version >= mInfo->vMin && 
	     ((mInfo->vMax != -1) ? (version <= mInfo->vMax) : true);
};

char* castTypeArray(char* base, ArrayInfo* aInfo) {
	char* arrayBase = (aInfo->type == ARRAYTYPE_CONSTANT) ? base :*((char**)(base));
	return arrayBase;
}

int getTypeArraySize(char* structBase, ArrayInfo aInfo, char* arrayBase = 0) {
	int arraySize;
	     if(aInfo.sizeMode == ARRAYSIZE_CONST) arraySize = aInfo.size;
	else if(aInfo.sizeMode == ARRAYSIZE_VAR) arraySize = *(int*)(structBase + aInfo.offset);
	else if(aInfo.sizeMode == ARRAYSIZE_STRING) arraySize = strLen(arrayBase);

	return arraySize;
}
int getArrayMemberOffset(MemberInfo* mInfo, int arrayLevel) {
	ArrayInfo* aInfo = mInfo->arrays + arrayLevel;

	if(aInfo->type == ARRAYTYPE_CONSTANT) return aInfo->constSize;
	else {
		if(arrayLevel == mInfo->arrayCount-1) {
			return getStructInfo(mInfo->type)->size;
		} else {
			return sizeof(char*);
		};
	}
}

bool memberIsString(MemberInfo* mInfo, int arrayLevel) {
	if(mInfo->type == getType(char) && mInfo->arrayCount > 0 && arrayLevel == mInfo->arrayCount-1) {
		ArrayInfo* aInfo = mInfo->arrays + arrayLevel;
		if(aInfo->type == ARRAYTYPE_DYNAMIC && aInfo->sizeMode == ARRAYSIZE_STRING) {
			return true;
		}
	}

	return false;
}

bool memberIsUnionized(MemberInfo* mInfo, char* structBase) {
	if(mInfo->uInfo.type == -1) return true;

	int structUnionType = *(int*)(structBase + mInfo->uInfo.offset);
	return structUnionType == mInfo->uInfo.type;
}

MemberInfoData getMember(StructInfo* sInfo, char* data, int index) {
	MemberInfo* mi = sInfo->list + index;
	return {*mi, data + mi->offset};
}

MemberInfoData getMember(MemberInfo* mInfo, char* data, int index) {
	// assert(mInfo->arrayCount == 0);

	StructInfo* sInfo = getStructInfo(mInfo->type);
	if(!sInfo->list) return {*mInfo, data};

	MemberInfo* mi = sInfo->list + index;
	return {*mi, data + mi->offset};
}

MemberInfoData getMember(StructInfo* sInfo, char* data, char* name) {
	for(int i = 0; i < sInfo->memberCount; i++) {
		MemberInfo* mi = sInfo->list + i;
		if(strCompare(mi->name, name)) {
			return {*mi, data + mi->offset};
		}
	}

	return {};
}

MemberInfoData getMember(MemberInfo* mInfo, char* data, char* name) {
	return getMember(getStructInfo(mInfo->type), data, name);
}

MemberInfo* getMember(StructInfo* sInfo, int index) {
	if(!sInfo->list) return 0;
	return sInfo->list + index;;
}

MemberInfo* getMember(StructInfo* sInfo, char* name) {
	if(!sInfo->list) return 0;

	for(int i = 0; i < sInfo->memberCount; i++) {
		MemberInfo* mInfo = sInfo->list + i;
		if(strCompare(name, mInfo->name)) return mInfo;
	}

	return 0;
}

MemberInfo* getMember(MemberInfo* mInfo, int index) {
	return getMember(getStructInfo(mInfo->type), index);
}
MemberInfo* getMember(MemberInfo* mInfo, char* name) {
	return getMember(getStructInfo(mInfo->type), name);
}

bool memberHasTag(MemberInfo* mInfo, char* tag) {
	if(!mInfo->tagCount) return false;

	for(int i = 0; i < mInfo->tagCount; i++) {
		if(strCompare(mInfo->tags[i][0], tag)) {
			return true;
		}
	}

	return false;
}

char** memberGetTag(MemberInfo* mInfo, char* tag) {
	if(!mInfo->tagCount) return 0;

	for(int i = 0; i < mInfo->tagCount; i++) {
		if(strCompare(mInfo->tags[i][0], tag)) {
			return &mInfo->tags[i][1];
		}
	}

	return 0;
}

void memberCopyTags(MemberInfo* mInfoSrc, MemberInfo* mInfoDst) {
	mInfoDst->tagCount = mInfoSrc->tagCount;

	for(int i = 0; i < mInfoSrc->tagCount; i++) {
		for(int j = 0; j < arrayCount(mInfoSrc->tags[0]); j++) {
			mInfoDst->tags[i][j] = mInfoSrc->tags[i][j];
		}
	}
}

bool compareMemberInfoData(MemberInfo* mInfo, char* data0, char* data1) {
	bool equal = memcmp(data0, data1, getStructInfo(mInfo->type)->size) == 0;
	if(mInfo->type == TYPE_string) {
		char* s0 = *((char**)data0);
		char* s1 = *((char**)data1);
		if(s0 && s1) equal = strCompare(s0, s1);
	}

	return equal;
}

void copyMemberInfoData(MemberInfo* mInfo, char* src, char* dst) {
	if(mInfo->type != TYPE_string) {
		memcpy(dst, src, getStructInfo(mInfo->type)->size);
	} else {
		char* newString = getPString(*(char**)src);
		*((char**)dst) = newString;
		*((char**)src) = newString;
	}
}

bool sDataIsUnionized(SData* sStruct, MemberInfo* mInfo) {
	if(mInfo->uInfo.type == -1) return true;

	for(int i = 0; i < sStruct->members.count; i++) {
		SData* sData = sStruct->members.atr(i);
		if(strCompare(sData->name, mInfo->uInfo.varName)) {
			int unionType = *(int*)sData->aData;
			if(unionType == mInfo->uInfo.type) return true;
			else return false;
		}
	}

	return true;
}

int getSDataTypeFromMemberInfo(MemberInfo* mInfo, int arrayLevel = 0) {
	if(!mInfo) return SData_STRUCT;

	int type = typeIsPrimitive(mInfo->type) ? SData_ATOM : SData_STRUCT;
	if(mInfo->arrayCount && arrayLevel != mInfo->arrayCount) type = SData_ARRAY;

	return type;
}

void typePrint(int type, void* data) {
	switch(type) {
		case TYPE_u64:    printf("%I64", *(u64*)data); break;
		case TYPE_i64:    printf("%I64", *(i64*)data); break;
		case TYPE_int:    printf("%i",   *(int*)data); break;
		case TYPE_uint:   printf("%u",   *(uint*)data); break;
		case TYPE_short:  printf("%i",   *(short*)data); break;
		case TYPE_ushort: printf("%u",   *(ushort*)data); break;
		case TYPE_char:   printf("'%c'", *(char*)data); break;
		case TYPE_uchar:  printf("'%c'", *(uchar*)data); break;
		case TYPE_float:  printf("%g",   *(float*)data); break;
		case TYPE_double: printf("%g",   *(double*)data); break;
		case TYPE_bool:   printf("%i",   *(bool*)data); break;
		case TYPE_string: {
			if(*(char**)data == 0) printf("%s",*(char**)data);
			else                   printf("\"%s\"",*(char**)data); break;
		}
	};
}

//

void TypeTraverse::start(int type, void* data, char* name = "") { 
	*this = {}; 

	// We start the stack by simulating a member info.
	startMemberInfo = {};
	startMemberInfo.type = type;
	startMemberInfo.name = name;
	startMemberInfo.offset = 0;
	startMemberInfo.arrayCount = 0;
	startMemberInfo.uInfo = {-1};
	startMemberInfo.vMin = -1;
	startMemberInfo.vMax = -1;

	// Hack: we hand input global vars that push() uses.
	mInfo = &startMemberInfo;
	this->type = SData_STRUCT;
	this->data = (char*)data;

	push();
};

void TypeTraverse::start(MemberInfo* mInfo, void* structBase, char* name) { 
	*this = {}; 

	// Hack: we hand input global vars that push() uses.
	startMemberInfo = *mInfo;
	this->mInfo = &startMemberInfo;
	this->type = SData_STRUCT;
	this->data = (char*)structBase;
	startMemberInfo.name = name;

	push();
};

void TypeTraverse::get() {
	if(e->memberIndex >= e->memberCount) return;

	bool notArray = e->arrayIndex == 0;
	if(stackCount == 1)
		mInfo = &startMemberInfo;
	else 
		mInfo = notArray ? getStructInfo(e->type)->list + e->memberIndex : e->mInfo;

	memberType = mInfo->type;

	type = getSDataTypeFromMemberInfo(mInfo, e->arrayIndex);

	int offset = notArray ? mInfo->offset : e->memberIndex * getArrayMemberOffset(mInfo, e->arrayIndex-1);
	data = e->data + offset;

	// Handle null pointer as zero.
	if(type == SData_ARRAY) {
		char* arrayBase = castTypeArray(data, mInfo->arrays + e->arrayIndex-1);
		if(!arrayBase) {
			type = SData_ATOM;
			memberType = TYPE_int;
			data = (char*)&temp;
		}
	}
}

bool TypeTraverse::insideArray() { return e->arrayIndex > 0; }

char* TypeTraverse::typeName() { 
	char* name = getStructInfo(mInfo->type)->name;

	if(type == SData_ARRAY) {
		ArrayInfo* aInfo = mInfo->arrays + e->arrayIndex-1;
		name = (aInfo->type == ARRAYTYPE_CONSTANT ? 
		            fString("%s [%i]", name, aInfo->size) : 
		            fString("%s %.*s", name, mInfo->arrayCount, "*****"));
	}

	return name;
}

char* TypeTraverse::memberName() {
	char* name = e->arrayIndex == 0 ? mInfo->name : fString("[%i]", e->memberIndex);
	return name;
}

bool TypeTraverse::next() {
	e->memberIndex++;
	e->firstMember = false;
	skip();

	return valid();
}

void TypeTraverse::push() {
	// Have to get() first.

	if(type == SData_STRUCT) {
		stack[stackCount++] = { mInfo->type, mInfo, data, data, 0, 0, stackCount == 0 ? 1 : getStructInfo(mInfo->type)->memberCount, true };
	} else {
		ArrayInfo* aInfo = mInfo->arrays + e->arrayIndex;
		char* arrayBase = castTypeArray(data, aInfo);
		int arraySize = getTypeArraySize((char*)e->structBase, *aInfo, (char*)arrayBase);

		stack[stackCount++] = { mInfo->type, mInfo, arrayBase, e->structBase, e->arrayIndex+1, 0, arraySize, true };
	}

	e = stack + stackCount - 1;
	skip();
}

void TypeTraverse::skip() {
	if(stackCount <= 1) return;
	if(e->memberIndex >= e->memberCount) return;

	if(e->arrayIndex == 0) {
		if(e->memberIndex < e->memberCount) {
			while(true) {
				MemberInfo* mInfo = getStructInfo(e->type)->list + e->memberIndex;

				if(!memberIsUnionized(mInfo, e->structBase) ||
					(ignoreHiddenMembers && mInfo->hide)) {

					e->memberIndex++;
					if(e->memberIndex >= e->memberCount) break;
				} else break;
			}
		}

	} else if(mInfo->arrayCount && e->arrayIndex == mInfo->arrayCount) {
		while(e->memberIndex < e->memberCount) {
			get();
			int boolInitVarOffset = getStructInfo(memberType)->initBoolOffset;
			if(boolInitVarOffset == -1) break;

			bool isInitialised = *((bool*)(((char*)data) + boolInitVarOffset));
			if(isInitialised) break;

			e->memberIndex++;
		}
	}
}

bool TypeTraverse::pop() {
	stackCount--;
	e = stack + stackCount - 1;

	return !end();
}

bool TypeTraverse::popNext() {
	bool result = pop();
	if(result) next();

	return result;
}

void TypeTraverse::getNext() {
	get();
	next();	
}

bool TypeTraverse::valid() {
	return e->memberIndex < e->memberCount;
}

bool TypeTraverse::validGet() {
	bool result = valid();
	if(result) get();

	return result;
}

bool TypeTraverse::end() {
	return stackCount == 0;
}

void TypeTraverse::save() {
	savedStackCount = stackCount;
	savedMemberIndex = stack[stackCount-1].memberIndex;
}

void TypeTraverse::load() {
	stackCount = savedStackCount;
	stack[stackCount-1].memberIndex = savedMemberIndex;
	e = stack + stackCount - 1;
}

//

char* typeToStr(int type, void* data) {
	switch(type) {
		case TYPE_u64:    return fString("%I64", *(u64*)data);
		case TYPE_i64:    return fString("%I64", *(i64*)data);
		case TYPE_int:    return fString("%i",   *(int*)data);
		case TYPE_uint:   return fString("%u",   *(uint*)data);
		case TYPE_short:  return fString("%i",   *(short*)data);
		case TYPE_ushort: return fString("%u",   *(ushort*)data);
		case TYPE_char:   return fString("'%c'", *(char*)data);
		case TYPE_uchar:  return fString("'%c'", *(uchar*)data);
		case TYPE_float:  return fString("%g",   *(float*)data);
		case TYPE_double: return fString("%g",   *(double*)data);
		case TYPE_bool:   return *(bool*)data ? "true" : "false";
		case TYPE_string: {
			if(*(char**)data == 0) return fString("0");
			return fString("\"%s\"", *(char**)data);
		}
	};

	return 0;
}

void readAtomType(SData* atom, char* buf) {
	char* data = atom->aData;
	switch(atom->typeId) {
		                                // Fix this.
		case TYPE_u64:    *(u64*)data =    strToInt(buf); break;
		case TYPE_i64:    *(i64*)data =    strToInt(buf); break;
		case TYPE_int:    *(int*)data =    strToInt(buf); break;
		case TYPE_uint:   *(uint*)data =   strToInt(buf); break;
		case TYPE_short:  *(short*)data =  strToInt(buf); break;
		case TYPE_ushort: *(ushort*)data = strToInt(buf); break;
		case TYPE_char:   *(char*)data =   (buf)[1]; break;
		case TYPE_uchar:  *(uchar*)data =  (buf)[1]; break;
		case TYPE_float:  *(float*)data =  strToFloat(buf); break;
		case TYPE_double: *(double*)data = strToFloat(buf); break;
		case TYPE_bool:   *(bool*)data =   (buf)[0] == 't' ? true : false; break;
		case TYPE_string: {
			if(buf[0] == '0') *(char**)data = 0;
			else {
				buf++;
				char* end = buf;
				while(end[0] != '\"') end++;
				*(char**)data = getPString(buf, end - buf); break;
			}
		} break;
		default: break;
	};
}

char* typeInfoToStr(TypeTraverse tt, int expandLevel = 0, int maxArrayCount = 0, int maxElementCount = 0, int maxTextWidth = 0, Font* font = 0) {
	int level = expandLevel+1;
	int indentSize = 2;

	int sizeMode = maxArrayCount ? 0 : (maxElementCount ? 1 : (maxTextWidth ? 2 : -1));
	int currentTextWidth = 0;
	int currentTextIndex = 0;

	DArray<char> str = dArray<char>(getTMemory);

	int elementCount = 0;

	while(true) {
		if(tt.valid()) {
			tt.get();

			if(tt.stackCount <= level) str.pushStr(fString("%*s", indentSize*(tt.stackCount-1), ""));
			if(tt.stackCount > level) str.pushStr(tt.e->firstMember ? " " : ", ");

			char* memberName = tt.memberName();
			if(tt.type == SData_ATOM) {
				if(!tt.insideArray()) str.pushStr(fString(tt.stackCount <= level ? "%s = " : "%s=", memberName));
				str.pushStr(typeToStr(tt.memberType, tt.data));

				tt.next();

				if(sizeMode == 0) {
					if(tt.insideArray() && tt.e->memberIndex >= maxArrayCount) {
						tt.pop();
						tt.next();
					}

				} else if(sizeMode == 1) {
					elementCount++;

					if(elementCount >= maxElementCount) {
						str.pushStr(", ... ");
						break;
					}

				} else if(sizeMode == 2) {
					str.push('\0');
					defer { str.pop(); };
					currentTextWidth += getTextDim(str.data + currentTextIndex, font).w;
					currentTextIndex = str.count-1;

					if(currentTextWidth > maxTextWidth) break;
				}

			} else {
				if(tt.stackCount <= level && strLen(memberName)) str.pushStr(fString("%s = ", memberName));
				str.pushStr(tt.type != SData_ARRAY ? "{" : "[");
				
				tt.push();
			}

		} else {
			if(!tt.pop()) break;
			tt.getNext();

			if(tt.stackCount < level) str.pushStr(fString("%*s", indentSize*(tt.stackCount-1), ""));
			else str.pushStr(" ");

			str.pushStr(tt.type != SData_ARRAY ? "}" : "]");
		}

		if(tt.stackCount > 1 && tt.stackCount <= level) str.pushStr("\n");
	}

	str.push('\0');

	return str.data;
}

char* memberToStr(MemberInfo* mInfo, void* structBase, int expandLevel = 0, char* name = "", int maxArrayCount = 0, int maxElementCount = 0, int maxTextWidth = 0, Font* font = 0) {
	TypeTraverse tt;
	tt.start(mInfo, (char*)structBase, name);

	return typeInfoToStr(tt, expandLevel, maxArrayCount, maxElementCount, maxTextWidth, font);
}

char* structToStr(int type, void* data, int expandLevel = 0, char* name = "", int maxArrayCount = 0, int maxElementCount = 0, bool ignoreHiddenMembers = false) {
	TypeTraverse tt;
	tt.start(type, (char*)data, name);
	tt.ignoreHiddenMembers = ignoreHiddenMembers;

	return typeInfoToStr(tt, expandLevel, maxArrayCount, maxElementCount);
}

void printStruct(int type, void* data, int expandLevel = 0, char* name = "", int maxArrayCount = 0, int maxElementCount = 0) {
	printf(structToStr(type, data, expandLevel, name, maxArrayCount, maxElementCount, true));
	printf("\n");
}

//

/*

getTypeInfo(void* obj, "typename").getMember("membername");
                                  .getMember(2);
                                  .setMember();
<member>.name/value




// int var = 2234;
// Mat4 mat;
// char* name1 = (char*)typeid(var).name();
// char* name2 = (char*)typeid(var).raw_name();
// char* name3 = (char*)typeid(mat).name();

// printf("%s\n %s\n %s\n", name1, name2, name3);


// Entity entity;
getTypeInfo("Entity", entity).getMember("member").value()/name()/setValue(void*)
getTypeInfo("Entity", entity).getMember("member").getMember("asdf")
getTypeInfo("Entity", entity)[123]
getTypeInfo("Entity", entity).getMember(123);

// Bonus
getTypeInfo("Entity", entity).getMember("member").next()/prev()/isLast()/isFirst();
getTypeInfo("Entity", entity).getMember("member").print()?/printValue?/valueToString?

// getTypeInfo("Entity", entity).getMember("member").value()/name()/setValue(void*)

*/

#define MakeMemberHandle(type, variable) memberHandle(getType(type), &variable)

struct MemberHandle {
	bool ignoreHiddenMembers;

	//

	MemberInfo startMemberInfo;

	StructInfo* structInfo;
	void* basePointer;
	MemberInfo* memberInfo;

	//

	int arrayIndex;
	int memberIndex;
	int memberCount;

	// 

	int type;
	int size;
	int sDataType;
	void* pointer;

	//

	int zeroInt;


	void init() { *this = {}; }
	void init(int type, void* data, char* name = "") {
		*this = {};

		this->structInfo = getStructInfo(type);
		this->basePointer = data;

		this->type = type;
		this->size = this->structInfo->size;
		this->pointer = data;

		this->memberCount = 1;

		// We start the stack by simulating a member info.
		// startMemberInfo.type = type;
		// startMemberInfo.name = name;
		// startMemberInfo.offset = 0;
		// startMemberInfo.arrayCount = 0;
		// startMemberInfo.uInfo = {-1};
		// startMemberInfo.vMin = -1;
		// startMemberInfo.vMax = -1;

		// Hack: we hand input global vars that push() uses.
		// memberInfo = &startMemberInfo;
		// this->type = SData_STRUCT;
		// this->data = (char*)data;
	}

	bool isPrimitive() { return typeIsPrimitive(type); };

	char* typeStr();
	char* memberStr();
	char* valueStr();

	MemberHandle getMember(char* memberName) {
		if(arrayIndex) return {};

		StructInfo* sInfo = !memberInfo ? structInfo : memberInfo->structInfo();

		for(int i = 0; i < sInfo->memberCount; i++) {
			if(strCompare(sInfo->list[i].name, memberName)) {
				return getMember(i);
			}
		}

		return {};
	}

	void initMember(int index) {
		bool notArray = arrayIndex == 0;
		// Have to change memberInfo if it's not an array.
		if(notArray) {
			memberInfo = structInfo->list + index;
		}

		int offset = notArray ? memberInfo->offset : index * getArrayMemberOffset(memberInfo, arrayIndex-1);
		pointer = (char*)pointer + offset;

		sDataType = getSDataTypeFromMemberInfo(memberInfo, arrayIndex);

		// Handle null pointer as zero.
		if(sDataType == SData_ARRAY) {
			char* arrayBase = castTypeArray((char*)pointer, memberInfo->arrays + arrayIndex-1);
			if(!arrayBase) {
				sDataType = SData_ATOM;
				type = TYPE_int;
				pointer = &zeroInt;
			}
		}

		//

		type = memberInfo->type;
		memberIndex = 0;

		// Have to get() first.
		if(sDataType == SData_STRUCT) {
			structInfo = getStructInfo(type);
			memberCount = structInfo->memberCount;
			basePointer = pointer;
		} else {
			ArrayInfo* arrayInfo = memberInfo->arrays + arrayIndex;
			char* arrayBase = castTypeArray((char*)pointer, arrayInfo);
			int arraySize = getTypeArraySize((char*)basePointer, *arrayInfo, (char*)arrayBase);
			pointer = arrayBase;
			arrayIndex++;
			memberCount = arraySize;
		}
	}

	MemberHandle getMember(int index) {

		// if(e->memberIndex >= e->memberCount) return;

		memberIndex = index;

		MemberHandle mh = *this;
		mh.initMember(index);

		return mh;


		/*
		if(memberInfo) {
			if(sDataType == SData_ATOM) return {};

			if(mh.arrayIndex < memberInfo->arrayCount) {
				mh.arrayIndex++;
			} else {
				mh.structInfo = memberInfo->structInfo();
			}
		}

		if(!mh.arrayIndex) {
			mh.memberInfo = mh.structInfo->list + index;
			mh.memberIndex = index;
		}
		int offset = !mh.arrayIndex ? mh.memberInfo->offset : mh.memberIndex * getArrayMemberOffset(mh.memberInfo, mh.arrayIndex-1);

		mh.type = mh.memberInfo->type;
		mh.size = structInfo->size;
		mh.sDataType = getSDataTypeFromMemberInfo(mh.memberInfo, mh.arrayIndex);
		// mh.pointer = !mh.arrayIndex ? (char*)basePointer + offset : 0;
		mh.pointer = (char*)basePointer + offset;

		// {
		// 	ArrayInfo* aInfo = mh.memberInfo->arrays + mh.arrayIndex;
		// 	char* arrayBase = castTypeArray((char*)pointer, aInfo);
		// 	mh.arraySize = getTypeArraySize((char*)mh.basePointer, *aInfo, (char*)arrayBase);
		// 	mh.pointer = arrayBase;
		// }

		mh.basePointer = mh.pointer;
		*/







		// if(type == SData_STRUCT) {
		// 	stack[stackCount++] = { mInfo->type, mInfo, data, data, 0, 0, stackCount == 0 ? 1 : getStructInfo(mInfo->type)->memberCount, true };
		// } else {
		// 	ArrayInfo* aInfo = mInfo->arrays + e->arrayIndex;
		// 	char* arrayBase = castTypeArray(data, aInfo);
		// 	int arraySize = getTypeArraySize((char*)e->structBase, *aInfo, (char*)arrayBase);

		// 	stack[stackCount++] = { mInfo->type, mInfo, arrayBase, e->structBase, e->arrayIndex+1, 0, arraySize, true };
		// }

		// e = stack + stackCount - 1;
		// skip();



		// Handle null pointer as zero.
		// if(sDataType == SData_ARRAY) {
		// 	char* arrayBase = castTypeArray(data, mInfo->arrays + e->arrayIndex-1);
		// 	if(!arrayBase) {
		// 		sDataType = SData_ATOM;
		// 		memberType = TYPE_int;
		// 		data = (char*)&temp;
		// 	}
		// }





		// if(e->memberIndex >= e->memberCount) return;

		// bool notArray = e->arrayIndex == 0;
		// if(stackCount == 1)
		// 	mInfo = &startMemberInfo;
		// else 
		// 	mInfo = notArray ? getStructInfo(e->type)->list + e->memberIndex : e->mInfo;

		// memberType = mInfo->type;

		// type = getSDataTypeFromMemberInfo(mInfo, e->arrayIndex);

		// int offset = notArray ? mInfo->offset : e->memberIndex * getArrayMemberOffset(mInfo, e->arrayIndex-1);
		// data = e->data + offset;

		// // Handle null pointer as zero.
		// if(type == SData_ARRAY) {
		// 	char* arrayBase = castTypeArray(data, mInfo->arrays + e->arrayIndex-1);
		// 	if(!arrayBase) {
		// 		type = SData_ATOM;
		// 		memberType = TYPE_int;
		// 		data = (char*)&temp;
		// 	}
		// }

		return mh;
	}

	// char* name() {
	// 	return memberInfo->name;
	// }

	void setValue(void* data) { memcpy(pointer, data, size); }
};

inline MemberHandle memberHandle(int type, void* data) {
	MemberHandle mh;
	mh.init(type, data);

	return mh;
}

char* MemberHandle::typeStr() {
	char* name = !memberInfo ? structInfo->name : getStructInfo(memberInfo->type)->name;

	// if(sDataType == SData_ARRAY) {
	// 	ArrayInfo* aInfo = memberInfo->arrays + e->arrayIndex-1;
	// 	name = (aInfo->type == ARRAYTYPE_CONSTANT ? 
	// 	            fString("%s [%i]", name, aInfo->size) : 
	// 	            fString("%s %.*s", name, memberInfo->arrayCount, "*****"));
	// }

	return name;
}

char* MemberHandle::memberStr() {
	// char* name = arrayIndex == 0 ? mInfo->name : fString("[%i]", e->memberIndex);
	char* name = !memberInfo ? "Base" : memberInfo->name;
	return name;
}

char* MemberHandle::valueStr() {
	if(!isPrimitive()) return "";
	return typeToStr(type, pointer);
}
