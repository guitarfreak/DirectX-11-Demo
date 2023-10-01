
struct SettingsParser {
	struct Var {
		char* name;
		char* value;
	};

	struct Section {
		char* name;
		DArray<Var> vars;
	};

	DArray<Section> sections;

	void parseFile(char* filePath);
};

char* eatLine(char* s) {
	while(s[0] != '\r' && s[0] != '\n' && s[0] != '\0') (s)++;
	if(s[0] == '\0') return s;
	if(s[0] == '\r') {
		if(s[1] == '\n') (s) += 1;
	}
	s[0] = '\0';
	s++;

	return s;
}
void eatLine(char** s) { *s = eatLine(*s); }

void SettingsParser::parseFile(char* filePath) {
	char* b = readFileToBufferZeroTerminated(filePath, getTMemory);
	if(!b) {
		logPrint("Settings", Log_Error, fString("Could not open variable file: \"%s\"", filePath));
		return;
	}

	while(b[0] != '\0') {
		eatWhiteSpace(&b);
		char* s = b;
		b = eatLine(b);

		if(s[0] == '#') {
			continue;

		} else if(s[0] == ':') {
			if(s[1] != '/') {
				logPrint("Settings", Log_Error, "Expected '/' after ':'.");
				break;
			}
			s += 2;

			int sectionNameSize = parseIdentifier(s);
			if(sectionNameSize == -1) {
				logPrint("Settings", Log_Error, "Expected a section name.");
				break;
			}

			sections.push({getPString(s, sectionNameSize)});

		} else if(charIsAlphaNumeric(s[0])) {
			int nameLength = parseIdentifier(s);
			char* name = getPString(s, nameLength);
			s += nameLength;
			eatWhiteSpace(&s);
			char* value = getPString(s, strLen(s));

			sections.last().vars.push({name, value});
		}
	}
}
