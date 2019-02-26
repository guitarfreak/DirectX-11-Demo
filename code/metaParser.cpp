
#include "metaParser.h"

void createMetaDataFromCodeFolder(char** folderPaths, int folderCount, char* genFile) {
	DArray<MetaStruct> structs;
	structs.init(getTMemory);

	for(int i = 0; i < arrayCount(metaTypeList); i++) {
		structs.push({metaTypeList[i]});
	}

	DArray<MetaEnum> enums;
	enums.init(getTMemory);

	for(int i = 0; i < folderCount; i++) {
		char* folder = folderPaths[i];

		FolderSearchData fd;
		bool result = folderSearchStart(&fd, folder);
		if(!result) result = folderSearchStart(&fd, fString("..\\%s", folder));
		if(!result) printf("Error.\n");

		while(folderSearchNextFile(&fd)) {
			if(fd.isFolder) continue;

			if(strCompare(fd.fileExtension, "cpp") || strCompare(fd.fileExtension, "h")) {
				char* file = readFileToBufferZeroTerminated(fd.filePath);
				defer { free(file); };

				if(file) {
					metaParseFile(file, &structs, &enums);
				}
			}
		}
	}

	// char* fileString = getTStringClr(100000); // Make systematic.
	DArray<char> fileString;
	fileString.init(getTMemory);
	buildMetaData(&structs, &enums, &fileString);
	fileString.push('\0');

	writeBufferToFile(fileString.data, genFile);
}

void metaParseFile(char* file, DArray<MetaStruct>* structs, DArray<MetaEnum>* enums) {
	char* buf = file;
	int pos = strFind(buf, MC_Parse);
	while(pos != -1) {
		if(buf[pos + strLen(MC_Parse)    ] != '\"' && // Ugh.
		   buf[pos + strLen(MC_Parse) + 1] != 'v'  && 
		   buf[pos - 2                   ] != '/') {
			
			MetaStruct ms = metaParseStruct(buf + pos);

			structs->push(ms);
		}

		buf += pos + strLen(MC_Parse);

		pos = strFind(buf, MC_Parse);
	}

	buf = file;
	pos = strFind(buf, MC_ParseEnum);
	while(pos != -1) {
		if(buf[pos + strLen(MC_ParseEnum)] != '\"' && // Ugh.
		   buf[pos - 2                   ] != 'e'  && 
		   buf[pos - 2                   ] != '/') {
			
			MetaEnum me = metaParseEnum(buf + pos);

			enums->push(me);
		}

		buf += pos + strLen(MC_ParseEnum);

		pos = strFind(buf, MC_ParseEnum);
	}
}

MetaStruct metaParseStruct(char* str, bool skipVersionHeader) {

	MetaStruct ms = {};
	ms.members.init(getTMemory);

	//

	auto parse = [](char* b) -> int {
		char* tokenStrings[] = { 
			"//", "{", "}", "[", "]", "(", ")", "<", ">", "*", "&", ";", ",", "=", 
			MC_Version, MC_SizeString, MC_SizeVar, MC_Union, MC_Ignore, 
			MC_Enum, MC_Hide, MC_Init, MC_Tag };

		for(int i = 0; i < arrayCount(tokenStrings); i++) {
			if(strStartsWith(b, tokenStrings[i])) {
				return strLen(tokenStrings[i]);
			}
		}

		if(charIsDigit(b[0]) || b[0] == '-' ) return parseNumber(b);
		else return parseIdentifier(b);
	};

	QuickParser parser;
	parser.init(str, parse);

	//

	if(!skipVersionHeader) {
		parser.eatToken(MC_Parse);

		parser.eatToken("(");
		if(!parser.peekToken(")")) ms.version = strToInt(parser.eatToken());
		else ms.version = -1;
		parser.eatToken(")");

		parser.eatToken(";");
	}

	if(parser.peekEatToken("template")) {
		parser.eatToken("<");
		parser.eatToken("class");
		char* templateType = getTString(parser.eatToken());
		parser.eatToken(">");

		ms.isTemplate = true;
		ms.templateType = templateType;
	};

	if(!parser.peekToken("struct") && !parser.peekToken("union")) parser.error();
	parser.eatToken();

	ms.name = getTString(parser.eatToken());

	parser.eatToken("{");

	int nestedScopes = 0;

	// Parse members.

	while(true) {

		MetaMember mm[5] = {};
		mm[0].minVersion = -1;
		mm[0].maxVersion = -1;

		char* names[5];
		int mc = 1;

		//

		bool memberIsCommented = false;
		if(parser.peekEatToken("//")) {
			int pos = strFind(parser.b, "\n");
			if(pos == -1) parser.error();

			if(strFind(parser.b, MC_Version, pos) == -1) {
				parser.gotoNextLine();
				continue;
			}

			memberIsCommented = true;
		}

		if(parser.peekEatToken("}")) {
			if(nestedScopes == 0) {
				break;
			} else {
				parser.eatToken(";");
				nestedScopes--;
				continue;
			}
		}

		// Skip enums.
		if(parser.peekEatToken("enum")) {
			parser.eatToken();
			parser.skipScope('{');
			parser.eatToken(";");
			continue;
		}

		if(parser.peekToken("struct") || 
		   parser.peekToken("union")) {

			// if(!parser.peekEatToken("{")) parser.error();
			if(!parser.peekToken("{", 2)) {
				parser.error();
				// MetaStruct nestedStruct = metaParseStruct(parser.b, true);

			} else {
				// Anonymous struct.
				parser.eatToken();
				parser.eatToken("{");

				nestedScopes++;
				continue;
			}
		}

		mm[0].type = getTString(parser.eatToken());

		if(parser.peekToken("<")) {
			mm[0].usesTemplate = true;

			int pos = strFind(parser.b, ">");
			if(!pos) parser.error();

			// Add templated argument to type
			pos++;
			mm[0].templateType = getTString(parser.b + 1, pos - 2);
			mm[0].typeWithUnderscores = fString("%s_%s", mm[0].type, mm[0].templateType);
			mm[0].typeWithoutTemplate = getTString(mm[0].type);
			mm[0].type = fString("%s%.*s", mm[0].type, pos, parser.b);

			parser.b += pos;
		}

		mm[0].pointerCount = 0;
		while(parser.peekEatToken("*")) mm[0].pointerCount++;
		while(parser.peekEatToken("&"));

		// Skip function pointers.
		if(parser.peekToken("(")) {
			parser.gotoNextLine();
			continue;
		}

		// Skip operators.
		if(parser.peekToken("operator")) {
			parser.gotoNextLine();
			continue;
		}

		names[0] = getTString(parser.eatToken());

		// Skip function declarations and definitions.
		if(parser.peekToken("(")) {
			parser.skipScope('(');

			if(parser.peekEatToken(";")) continue;

			parser.skipScope('{');

			while(parser.peekEatToken(";"));
			continue;
		}

		while(parser.peekToken("[")) {
			mm[0].constArrayCount++;
			parser.skipTo(']');
		}

		while(parser.peekEatToken(",")) {
			assert(mc <= 5);
			names[mc] = getTString(parser.eatToken());
			mc++;
		}

		if(parser.peekToken("=")) {
			while(!parser.peekToken(";")) parser.eatToken();
		}

		parser.eatToken(";");

		bool ignore = false;

		if(parser.peekToken("//") && parser.peekToken(2)[0] == '@') {
			parser.eatToken("//");

			while(true) {
				parser.peekToken();

				if(parser.compareToken(MC_Version)) {
					parser.eatToken();
					// parser.eatToken("(");
					mm[0].minVersion = strToInt(parser.eatToken());
					if(parser.peekEatToken("-")) {
						mm[0].maxVersion = strToInt(parser.eatToken());
					}

					if(memberIsCommented && mm[0].maxVersion == -1) {
						ignore = true;
						break;
					}

					// parser.eatToken(")");

				} else if(parser.compareToken(MC_SizeString)) {
					parser.eatToken();
					mm[0].arraySizeString = true;

				} else if(parser.compareToken(MC_SizeVar)) {
					parser.eatToken();
					parser.eatToken("(");
					mm[0].arraySizeVar = getTString(parser.eatToken());
					parser.eatToken(")");

				} else if(parser.compareToken(MC_Union)) {
					parser.eatToken();
					parser.eatToken("(");
					mm[0].usesUnion = true;
					mm[0].unionVar = getTString(parser.eatToken());
					parser.eatToken(",");
					mm[0].unionType = getTString(parser.eatToken());
					parser.eatToken(")");

				} else if(parser.compareToken(MC_Enum)) {
					parser.eatToken();
					mm[0].isEnum = true;

					parser.eatToken("(");
					mm[0].enumName = getTString(parser.eatToken());
					parser.eatToken(")");

				} else if(parser.compareToken(MC_Hide)) {
					parser.eatToken();
					mm[0].hide = true;

				} else if(parser.compareToken(MC_Init)) {
					parser.eatToken();
					mm[0].structIsInit = true;

				} else if(parser.compareToken(MC_Tag)) {
					parser.eatToken();
					parser.eatToken("(");

					int tagIndex = mm[0].tagCount;
					mm[0].tagCount++;

					int subIndex = 0;
					while(true) {
						parser.eatWhiteSpace();
						mm[0].tags[tagIndex][subIndex++] = getTString(parser.skipTo(",)", true));
						if(parser.b[-1] == ')') break;
					}

				} else if(parser.compareToken(MC_Ignore)) {
					ignore = true;
					break;

				} else {
					parser.gotoNextLine();
					break;
				}
			}
		}

		if(ignore) {
			parser.gotoNextLine();
			continue;
		}

		for(int i = 1; i < mc; i++) mm[i] = mm[0];
		for(int i = 0; i < mc; i++) mm[i].name = names[i];

		ms.members.push(mm, mc);
	}

	parser.eatToken(";");

	return ms;
}


MetaEnum metaParseEnum(char* str) {

	MetaEnum ms = {};
	ms.members.init(getTMemory);

	//

	auto parse = [](char* b) -> int {
		char* tokenStrings[] = { 
			"//", "{", "}", ";", ",", "=", MC_Size };

		for(int i = 0; i < arrayCount(tokenStrings); i++) {
			if(strStartsWith(b, tokenStrings[i])) {
				return strLen(tokenStrings[i]);
			}
		}

		if(charIsDigit(b[0])) return parseNumber(b);
		else return parseIdentifier(b);
	};

	QuickParser parser;
	parser.init(str, parse);

	//

	parser.eatToken(MC_ParseEnum);
	parser.eatToken("(");
	parser.eatToken(")");
	parser.eatToken(";");

	parser.eatToken("enum");
	ms.name = getTString(parser.eatToken());
	parser.eatToken("{");

	while(true) {
		if(parser.peekToken("}")) break;

		char* name = getTString(parser.eatToken());
		ms.members.push(name);

		if(parser.peekEatToken("=")) {
			parser.eatToken();
		}
	
		parser.eatToken(",");


		if(parser.peekToken("//") && parser.peekToken(2)[0] == '@') {
			parser.eatToken("//");

			while(true) {
				parser.peekToken();

				if(parser.compareToken(MC_Size)) {
					parser.eatToken();
					ms.sizeName = ms.members.last();

				} else {
					parser.gotoNextLine();
					break;
				}
			}
		}
	}

	parser.eatToken("}");
	parser.eatToken(";");

	return ms;
}

void buildMetaData(DArray<MetaStruct>* structs, DArray<MetaEnum>* enums, DArray<char>* fileString) {

	fileString->pushStr("\n");

	for(int i = 0; i < enums->count; i++) {
		MetaEnum me = (*enums)[i];
		if(me.members.count == 0) continue;

		char* name = me.name;
		fileString->pushStr(fString("EnumInfo %s_EnumInfos[] = {\n", name));

		int tagLength = -1;
		if(me.members.count > 1) {
			int i = 0;
			while(me.members.data[0][i] == me.members.data[1][i]) i++;
			tagLength = i;
		}

		for(int i = 0; i < me.members.count; i++) {
			char* name = me.members[i];

			if(name == me.sizeName) continue; // Intentional ==

			char* camelCaseName = getTString(name + tagLength);
			camelCaseName = stringToCamelCase(camelCaseName);
			char* s = fString("\t{ %s, \"%s\", \"%s\" },\n", name, name, tagLength == -1 ? name : camelCaseName);

			fileString->pushStr(s);
		}

		fileString->pushStr("};\n\n");
	}

	//

	// Find used templates.
	for(int structIndex = 0; structIndex < structs->count; structIndex++) {
		MetaStruct* ms = structs->atr(structIndex);

		if(ms->isTemplate) {
			ms->usedTemplateTypes.init(getTMemory);
			ms->usedTemplateMembers.init(getTMemory);

			for(int i = 0; i < structs->count; i++) {
				if(i == structIndex) continue;

				MetaStruct* mStruct = structs->atr(i);

				for(int i = 0; i < mStruct->members.count; i++) {
					MetaMember* mMember = mStruct->members.atr(i);

					if(mMember->usesTemplate && strCompare(mMember->typeWithoutTemplate, ms->name)) {
						bool alreadyAdded = false;
						for(int i = 0; i < ms->usedTemplateTypes.count; i++) {
							if(strCompare(ms->usedTemplateTypes[i], mMember->typeWithUnderscores)) {
								alreadyAdded = true;
								break;
							}
						}

						if(!alreadyAdded) {
							ms->usedTemplateTypes.push(mMember->typeWithUnderscores);
							ms->usedTemplateMembers.push(mMember);
						}
					}
				}
			}
		}
	}

	// Type Enum.
	{
		fileString->pushStr("enum StructType {\n");

		for(int structIndex = 0; structIndex < structs->count; structIndex++) {
			MetaStruct ms = (*structs)[structIndex];

			char** list = ms.isTemplate ? ms.usedTemplateTypes.data : &ms.name;
			int size = ms.isTemplate ? ms.usedTemplateTypes.count : 1;

			for(int i = 0; i < size; i++) {
				char* name = list[i];
				fileString->pushStr(fString("\tTYPE_%s%s,\n", name, structIndex == 0 ? " = 0" : ""));
			}
		}

		fileString->pushStr("\n\tTYPE_SIZE, \n");
		fileString->pushStr("};\n\n");
	}

	// Member arrays.
	for(int structIndex = 0; structIndex < structs->count; structIndex++) {
		// MetaStruct ms = (*structs)[structIndex];
		MetaStruct* ms = (*structs) + structIndex;
		if(ms->members.count == 0) continue;

		char** list = ms->isTemplate ? ms->usedTemplateTypes.data : &ms->name;
		int size = ms->isTemplate ? ms->usedTemplateTypes.count : 1;

		for(int subStructIndex = 0; subStructIndex < size; subStructIndex++) {
			char* msName = list[subStructIndex];

			fileString->pushStr(fString("MemberInfo %s_MemberInfos[] = {\n", msName));

			for(int i = 0; i < ms->members.count; i++) {
				MetaMember mm = ms->members[i];

				char* typeString = mm.usesTemplate ? mm.typeWithUnderscores : mm.type;
				if(ms->isTemplate && strCompare(typeString, ms->templateType)) typeString = ms->usedTemplateMembers[subStructIndex]->templateType;

				char* type = fString("TYPE_%s", typeString);
				char* name = fString("\"%s\"", mm.name);
				char* vMin = fString("%i", mm.minVersion);
				char* vMax = fString("%i", mm.maxVersion);
				char* offset = fString("offsetof(%s, %s)", ms->isTemplate ? ms->usedTemplateMembers[subStructIndex]->type : msName, mm.name);

				if(mm.structIsInit) {
					ms->initBoolOffsetString = fString("%s, ", offset);
				}

				if(mm.minVersion != -1 && mm.maxVersion != -1) offset = "0";

				char* typeStringX = ms->isTemplate ? ms->usedTemplateMembers[subStructIndex]->type : msName;

				// Arrays.
				char* arrayCountString = "0";
				char* array = "{}";
				int arrayCount = mm.constArrayCount + mm.pointerCount;

				if(arrayCount) {
					assert(arrayCount <= 2);

					ArrayInfo infos[2] = {};
					{
						// First const then dynamic arrays.
						int arrayIndex = 0;
						int count = mm.constArrayCount;
						while(count--) infos[arrayIndex++] = { ARRAYTYPE_CONSTANT, ARRAYSIZE_CONST };
						count = mm.pointerCount;
						while(count--) infos[arrayIndex++] = { ARRAYTYPE_DYNAMIC, ARRAYSIZE_CONST };

						// Last array gets string type.
						if(mm.arraySizeString) infos[arrayCount-1].sizeMode = ARRAYSIZE_STRING;

						if(infos[arrayCount-1].type == ARRAYTYPE_DYNAMIC && 
						   infos[arrayCount-1].sizeMode == ARRAYSIZE_STRING && 
						   strCompare(typeString, "char")) {
							type = "TYPE_string";
							arrayCount--;
							mm.pointerCount--;
						}

						// First dArray gets var then first const.
						if(mm.arraySizeVar) {
							if(mm.pointerCount > 0) {
								for(int i = 0; i < arrayCount; i++) {
									if(infos[i].type == ARRAYTYPE_DYNAMIC) infos[i].sizeMode = ARRAYSIZE_VAR;
								}
							} else {
								infos[arrayCount-1].sizeMode = ARRAYSIZE_VAR;
							}
						}
					}

					arrayCountString = fString("%i", arrayCount);

					int constArrayCount = -1;
					char* arrays[2] = {"{-1}", "{-1}"};
					for(int i = 0; i < arrayCount; i++) {
						ArrayInfo* ai = infos + i;

						if(ai->type == ARRAYTYPE_CONSTANT) constArrayCount++;

						char* type = arrayTypeStrings[ai->type];
						char* sizeMode = arraySizeStrings[ai->sizeMode];

						char* arrayExtra = "";
						for(int j = 0; j < constArrayCount; j++)
							arrayExtra = fString("%s%s", arrayExtra, "[0]");
						char* arrayExtra2 = fString("%s[0]", arrayExtra);

						char* sizeOrOffset;
						if(ai->sizeMode == ARRAYSIZE_CONST)
							sizeOrOffset = ai->type == ARRAYTYPE_CONSTANT ? 
							               fString("memberArrayCount(%s, %s%s)", msName, mm.name, arrayExtra) : 
							               "1";
						else if(ai->sizeMode == ARRAYSIZE_VAR)
							sizeOrOffset = fString("offsetof(%s, %s)", typeStringX, mm.arraySizeVar);
						else 
							sizeOrOffset = "0";

						char* constSize = ai->type == ARRAYTYPE_CONSTANT ? 
						                  fString("memberSize(%s, %s%s)", msName, mm.name, arrayExtra2) : 
						                  i < arrayCount-1 ? "sizeof(char*)" : fString("sizeof(%s)", typeStringX);

						arrays[i] = fString("{%s, %s, %s, %s}", type, sizeMode, sizeOrOffset, constSize);
					}

					array = arrayCount == 0 ? "{}" : fString("{ %s, %s}", arrays[0], arrays[1]);
				}

				char* unionInfo = "{-1}";
				if(mm.usesUnion) {
					unionInfo = fString("{%s, offsetof(%s, %s), \"%s\"}", mm.unionType, typeStringX, mm.unionVar, mm.unionVar);
				}

				char* enumInfo = "0, 0";
				if(mm.isEnum) {
					enumInfo = fString("arrayCount(%s_EnumInfos), %s_EnumInfos", mm.enumName, mm.enumName);
				}

				char* tags = "0, {}";
				if(mm.tagCount) {
					char* ts = "";
					for(int i = 0; i < mm.tagCount; i++) {
						char* t = "";
						for(int j = 0; j < arrayCount(mm.tags[0]); j++) {
							char* tag = mm.tags[i][j];
							if(!tag) break;
							tag = fString("\"%s\"", tag);

							t = fString("%s%s%s", t, j ? ", " : "", tag);
						}
						ts = fString("%s%s{%s}", ts, i ? ", " : "", t);
					}
					tags = fString("%i, {%s}", mm.tagCount, ts);
				}

				fileString->pushStr(fString("\t{ %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s },\n", type, name, offset, arrayCountString, array, unionInfo, enumInfo, mm.hide ? "1" : "0", tags, vMin, vMax));
			}

			fileString->pushStr("};\n\n");
		}
	}

	// Struct infos.
	{
		fileString->pushStr("StructInfo structInfos[] = {\n");

		for(int structIndex = 0; structIndex < structs->count; structIndex++) {
			MetaStruct ms = (*structs)[structIndex];

			char** list = ms.isTemplate ? ms.usedTemplateTypes.data : &ms.name;
			int size = ms.isTemplate ? ms.usedTemplateTypes.count : 1;

			for(int i = 0; i < size; i++) {
				char* msName = list[i];

				char* name   = fString("\"%s\", ", msName);
				char* size   = fString("sizeof(%s), ", ms.isTemplate ? ms.usedTemplateMembers[i]->type : msName);
				if(strCompare(msName, "string")) size = "sizeof(char*), ";

				char* ver    = fString("%i, ", ms.version);
				char* mCount = fString("%i, ", ms.members.count);
				char* mList  = ms.members.count ? fString("%s_MemberInfos, ", msName) : "0, ";

				char* init   = ms.initBoolOffsetString ? ms.initBoolOffsetString : "-1, ";

				fileString->pushStr(fString("\t{ %s%s%s%s%s%s},\n", name, size, mCount, mList, init, ver));
			}
		}

		fileString->pushStr("};\n\n");
	}
}
