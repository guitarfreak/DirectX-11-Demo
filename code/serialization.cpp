
int findMember(MemberInfo* mInfo, SData* sData) {
	for(int i = 0; i < sData->members.count; i++) {
		SData* sMember = sData->members.atr(i);

		if(sMember->typeId == mInfo->type && 
		   (sMember->type == SData_ARRAY ? sMember->pointerCount == mInfo->arrayCount : true) && 
		   strCompare(sMember->name, mInfo->name)) {

			return i;
		}
	}

	return -1;
};

void serializeData(SData* sData, int type, char* data, char* memberName, MemberInfo* mInfo = 0, char* structBase = 0, int arrayLevel = 0, bool deserialize = false) {
	if(structBase == 0) structBase = (char*)data;

	int sDataType = getSDataTypeFromMemberInfo(mInfo, arrayLevel);

	if(sDataType == SData_ATOM) {
		if(!deserialize) {
			*sData = { SData_ATOM, type, memberName, type };

			if(type == TYPE_string) {
				char* str = *((char**)data);
				*((char**)sData->aData) = str == 0 ? 0 : getTString(str);
			}
			else memcpy(sData->aData, data, getStructInfo(type)->size);

		} else {
			if(type == TYPE_string) {
				char* str = *((char**)sData->aData);
				*((char**)data) = str == 0 ? 0 : getPString(str);
			}
			else memcpy(data, sData->aData, getStructInfo(type)->size);
		}

	} else if(sDataType == SData_STRUCT) {
		StructInfo* sInfo = getStructInfo(type);

		if(!deserialize) {
			*sData = { SData_STRUCT, type, getTString(memberName), sInfo->version };
			sData->members.init(getTMemory);
			sData->members.reserve(sInfo->memberCount);
		}

		for(int i = 0; i < sInfo->memberCount; i++) {
			MemberInfo* mInfo = sInfo->list + i;

			if(!memberIsUnionized(mInfo, (char*)data)) continue;

			char* newData = (char*)data + mInfo->offset;
			char* newStructBase = mInfo->arrayCount ? structBase : newData;
			if(memberIsVersioned(mInfo, sInfo->version)) {
				if(!deserialize) {
					SData sMember;
					serializeData(&sMember, mInfo->type, newData, mInfo->name, mInfo, newStructBase, 0, deserialize);
					sData->members.push(sMember);

				} else {
					SData* sMember = sData->members.atr(findMember(mInfo, sData));
					serializeData(sMember, mInfo->type, newData, mInfo->name, mInfo, newStructBase, 0, deserialize);
				}
			}
		}

	} else {
		StructInfo* sInfo = getStructInfo(mInfo->type);
		ArrayInfo* aInfo = mInfo->arrays + arrayLevel;
		char* arrayBase = castTypeArray((char*)data, aInfo);

		if(arrayBase) {
			int arraySize = !deserialize ? getTypeArraySize((char*)structBase, *aInfo, arrayBase) : 
			                               sData->members.count;
			int arrayMemberOffset = getArrayMemberOffset(mInfo, arrayLevel);

			if(!deserialize) {
				*sData = { SData_ARRAY, mInfo->type, getTString(memberName), mInfo->arrayCount - arrayLevel };
				sData->members.init(getTMemory);
				sData->members.reserve(arraySize);
			}

			for(int i = 0; i < arraySize; i++) {
				char* elementData = arrayBase + i * arrayMemberOffset;

				if(!deserialize) {
					SData sMember;
					serializeData(&sMember, mInfo->type, elementData, fString("_%i_", i), mInfo, structBase, arrayLevel + 1, deserialize);
					sData->members.push(sMember);

				} else {
					serializeData(sData->members.atr(i), mInfo->type, elementData, fString("_%i_", i), mInfo, structBase, arrayLevel + 1, deserialize);
				}
			}
		}
	}
}

void printSData(SData* sData, int depth = 0) {
	// char* tabs = "\t\t\t\t\t\t\t\t\t\t\t\t";
	char* tabs = "            ";
	printf("%.*s", depth, tabs);

	if(sData->type == SData_ATOM) {
		printf("%s %s = ", getStructInfo(sData->typeId)->name, sData->name);
		typePrint(sData->typeId, sData->aData);
		printf(", \n");

	} else if(sData->type == SData_STRUCT) {
		printf("%s(%i) %s #%i {\n", getStructInfo(sData->typeId)->name, sData->version, sData->name, sData->members.count);
		for(int i = 0; i < sData->members.count; i++) {
			printSData(sData->members.atr(i), depth + 1);
		}
		printf("%.*s", depth, tabs);
		printf("},\n");

	} else if(sData->type == SData_ARRAY) {
		printf("%s%.*s %s #%i [\n", getStructInfo(sData->typeId)->name, sData->pointerCount, "*****", sData->name, sData->members.count);
		for(int i = 0; i < sData->members.count; i++) {
			printSData(sData->members.atr(i), depth + 1);
		}
		printf("%.*s", depth, tabs);
		printf("],\n");
	}
	if(depth == 0) printf("\n");
}

SData createSMember(MemberInfo* mInfo, int arrayLevel = 0) {

	int sDataType = getSDataTypeFromMemberInfo(mInfo, arrayLevel);

	if(sDataType == SData_ATOM) {
		return { SData_ATOM, mInfo->type, getTString(mInfo->name) };

	} else if(sDataType == SData_STRUCT) {
		StructInfo* sInfo = getStructInfo(mInfo->type);
		SData sStruct = { SData_STRUCT, mInfo->type, getTString(mInfo->name), sInfo->version };
		sStruct.members.init(getTMemory);
		sStruct.members.reserve(sInfo->memberCount);

		for(int i = 0; i < sInfo->memberCount; i++) {
			MemberInfo* mInfo = sInfo->list + i;

			if(memberIsVersioned(mInfo, sInfo->version)) {
				SData sMember = createSMember(mInfo);
				sStruct.members.push(sMember);
			}
		}

		return sStruct;

	} else {
		StructInfo* sInfo = getStructInfo(mInfo->type);
		SData sArray = { SData_ARRAY, mInfo->type, arrayLevel == 0 ? getTString(mInfo->name) : "_e_", mInfo->arrayCount - arrayLevel };
		sArray.members.init(getTMemory);
		int arraySize = mInfo->arrays[arrayLevel].size;
		sArray.members.reserve(arraySize);

		for(int i = 0; i < arraySize; i++) {
			SData sMember = createSMember(mInfo, arrayLevel + 1);
			sArray.members.push(sMember);
		}

		return sArray;
	}
};

void sDataCopy(SData* sdDst, SData* sdSrc) {
	if(sdDst->type != sdSrc->type) return;
	if(!strCompare(sdDst->name, sdSrc->name)) return;

	if(sdDst->type == SData_ATOM) {
		memcpy(sdDst->aData, sdSrc->aData, getStructInfo(sdDst->type)->size);

	} else if(sdDst->type == SData_STRUCT) {
		for(int i = 0; i < sdSrc->members.count; i++) {
			sDataCopy(sdDst->members.data + i, sdSrc->members.data + i);
		}
	}
}

void specificVersionConversion(SData* sData, int version);
void versionConversion(SData* sData, int arrayLevel = 0) {
	if(sData->type == SData_ATOM) return;

	// First we get the current struct up to date and then we 
	// go through all the members.

	if(sData->type == SData_STRUCT) 
	{
		StructInfo* sInfo = getStructInfo(sData->typeId);
		int newestVersion = sInfo->version;

		while(sData->version < newestVersion) {
			int objectType = getTypeByName(sData->name);

			int versionOld = sData->version;
			int versionNew = versionOld + 1;
			for(int i = 0; i < sInfo->memberCount; i++) {
				MemberInfo* mInfo = sInfo->list + i;

				if(!sDataIsUnionized(sData, mInfo)) continue;

				bool isVersioned     = memberIsVersioned(mInfo, versionOld);
				bool isVersionedNext = memberIsVersioned(mInfo, versionNew);

				if(!isVersioned && isVersionedNext) {
					// Member gets added.
					sData->members.push(createSMember(mInfo));

				} else if(isVersioned && !isVersionedNext) {
					// Member gets removed.
					int sMemberIndex = findMember(mInfo, sData);
					assert(sMemberIndex != -1);

					sData->members.remove(sMemberIndex);
				}
			}

			specificVersionConversion(sData, sData->version);
			
			// int objectType = getTypeByName(sData->name);
			// switch(objectType) {
			// 	case TYPE_TestStructX: {
			// 		if(version == 0) {
			// 			sObject->
			// 		}
			// 	} break;
			// }

			sData->version++;
		}
	}

	for(int i = 0; i < sData->members.count; i++) {
		versionConversion(sData->members.atr(i));
	}
}

void writeAtomType(char** dest, SData* atom) {
	strAppend(dest, typeToStr(atom->typeId, atom->aData));
}

void writeAtomType(DArray<char>* dest, SData* atom) {
	dest->pushStr(typeToStr(atom->typeId, atom->aData));
}

void writeSData(SData* sData, DArray<char>* buf, int depth = 0) {
	char* tabs = "\t\t\t\t\t\t\t\t\t\t";
	buf->pushStr(fString("%.*s", depth, tabs));

	if(sData->type == SData_ATOM) {
		buf->pushStr(fString("%s %s = ", getStructInfo(sData->typeId)->name, sData->name));
		writeAtomType(buf, sData);
		buf->pushStr(", \n");

	} else if(sData->type == SData_STRUCT) {
		buf->pushStr(fString("{ %s(%i) %s %i\n", getStructInfo(sData->typeId)->name, sData->version, sData->name, sData->members.count));
		for(int i = 0; i < sData->members.count; i++) {
			writeSData(sData->members.atr(i), buf, depth + 1);
		}
		buf->pushStr(fString("%.*s", depth, tabs));
		buf->pushStr("},\n");

	} else if(sData->type == SData_ARRAY) {
		buf->pushStr(fString("[ %s %i %s %i\n", getStructInfo(sData->typeId)->name, sData->pointerCount, sData->name, sData->members.count));
		for(int i = 0; i < sData->members.count; i++) {
			writeSData(sData->members.atr(i), buf, depth + 1);
		}
		buf->pushStr(fString("%.*s", depth, tabs));
		buf->pushStr("],\n");
	}
}

SData readSData(QuickParser* parser) {

	char fc = parser->peekToken()[0];
	int type = fc == '{' ? SData_STRUCT : fc == '[' ? SData_ARRAY : SData_ATOM;

	SData sData = { type };

	if(type == SData_ATOM) {
		sData.typeId = getTypeByName(parser->eatToken());
		sData.name = getTString(parser->eatToken());

		parser->eatToken("=");

		parser->eatWhiteSpace();
		readAtomType(&sData, parser->b);

		parser->skipTo(',');

	} else if(type == SData_STRUCT) {
		parser->eatToken("{");

		sData.typeId = getTypeByName(parser->eatToken());

		parser->eatToken("(");
		sData.version = strToInt(parser->eatToken());
		parser->eatToken(")");

		sData.name = getTString(parser->eatToken());

		int memberCount = strToInt(parser->eatToken());
		sData.members.init();
		sData.members.resize(memberCount);

		for(int i = 0; i < memberCount; i++) {
			sData.members.push(readSData(parser));
		}

		parser->eatToken("}");
		parser->eatToken(",");

	} else {
		parser->eatToken("[");
		sData.typeId = getTypeByName(parser->eatToken());
		sData.pointerCount = strToInt(parser->eatToken());
		sData.name = getTString(parser->eatToken());
		
		int memberCount = strToInt(parser->eatToken());
		sData.members.init();
		sData.members.resize(memberCount);

		for(int i = 0; i < memberCount; i++) {
			sData.members.push(readSData(parser));
		}

		parser->eatToken("]");
		parser->eatToken(",");
	}

	return sData;
}

void readSDataFromFile(SData* sData, char* filePath) {
	char* fileBuffer = readFileToBufferZeroTerminated(filePath);
	defer { free(fileBuffer); };

	auto parse = [](char* b) -> int {
		char* tokenStrings[] = { "{", "}", "[", "]", "(", ")", ",", "=" };

		for(int i = 0; i < arrayCount(tokenStrings); i++) {
			if(strStartsWith(b, tokenStrings[i])) {
				return strLen(tokenStrings[i]);
			}
		}

		if(charIsDigit(b[0])) return parseNumber(b);
		else return parseIdentifier(b);
	};

	QuickParser parser = {};
	parser.init(fileBuffer, parse);

	*sData = readSData(&parser);
}

//

