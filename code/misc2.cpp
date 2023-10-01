
char* fString(char* text, ...) {
	va_list vl;
	va_start(vl, text);

	auto stbspCallback = [](char * buf, void * user_, int len ) -> char* {
		char* buffer = getTString(len);
		memcpy(buffer, buf, len);

		return 0;
	};

	// Hack: Get base pointer.
	// This assumes single threading.
	char* buffer = getTArray(char, 0); 

	char buf[STB_SPRINTF_MIN];
	int len = stbsp_vsprintfcb(stbspCallback, 0, buf, text, vl);
	va_end(vl);

	getTArray(char, 1);
	buffer[len] = '\0';

	return buffer;
}

char* fStringCombine(char* text, char* textToAppend) {
	return fString("%s%s", text, textToAppend);
}

//

struct QuickParser {
	struct Token {
		char str[50];
		int size;
	};

	char* b;
	Token token;

	int (*parse) (char* b);

	void  init(char* buffer, int (*parse) (char* b));
	void  eatWhiteSpace();
	void  gotoNextLine();
	void  skipScope(char beginScopeChar);
	char* skipTo(char c, bool getStr = false);
	char* skipTo(char* cs, bool getStr = false);
	void  eatToken(char* tString);
	int   parseToken();
	char* peekToken(int count = 1);
	bool  peekToken(char* str, int count = 1);
	char* eatToken();
	bool  peekEatToken(char* str, int count = 1);
	bool  compareToken(char* str);
	void  error();
};

void QuickParser::init(char* buffer, int (*parse) (char* b)) {
	this->b = buffer;
	this->parse = parse;
}

int QuickParser::parseToken() {
	eatWhiteSpace();

	int size = parse(b);
	if(size == -1) error();

	strCpy(token.str, b, size);
	token.size = size;

	return size;
}

void QuickParser::eatToken(char* tString) {
	eatWhiteSpace();

	int size = strLen(tString);
	if(!strStartsWith(b, tString, size)) error();

	b += size;
}

char* QuickParser::eatToken() {
	int size = parseToken();

	b += size;
	return token.str;
}

char* QuickParser::peekToken(int count) {
	char* oldB = b;
	for(int i = 0; i < count; i++) {
		parseToken();
		b += token.size;
	}
	b = oldB;

	return token.str;
}

bool QuickParser::peekToken(char* str, int count) {
	peekToken(count);

	return strCompare(str, token.str, token.size);
}

bool QuickParser::peekEatToken(char* str, int count) {
	bool result = peekToken(str, count);
	if(result) eatToken();
	
	return result;
}

bool QuickParser::compareToken(char* str) {
	return strCompare(str, token.str, token.size);
}

void QuickParser::eatWhiteSpace() {
	while(charIsWhiteSpace(b[0])) b++;
}

void QuickParser::gotoNextLine() {
	int pos = strFind(b, '\n');
	if(!pos) error();

	b += pos;
}

void QuickParser::skipScope(char beginScopeChar) {
	char endScopeChar;
	     if(beginScopeChar == '(') endScopeChar = ')';
	else if(beginScopeChar == '[') endScopeChar = ']';
	else if(beginScopeChar == '{') endScopeChar = '}';

	eatToken();

	int scope = 1;
	while(scope) {
		     if(b[0] == endScopeChar)   scope--;
		else if(b[0] == beginScopeChar) scope++;
		b++;
	}
}

char* QuickParser::skipTo(char c, bool getStr) {
	int pos = strFind(b, c);
	if(!pos) error();

	if(getStr) {
		strCpy(token.str, b, pos-1);
		token.size = pos;
	}

	b += pos;	
	return token.str;
}

// cs is multiple chars to search for and not a single string.
char* QuickParser::skipTo(char* cs, bool getStr) {
	int pos = -1;
	{
		int count = strLen(cs);
		int index = 0;
		while(true) {
			for(int i = 0; i < count; i++) {
				if(b[index] == cs[i]) {
					pos = index + 1;
					break;
				}
			}
			if(pos != -1) break;
			index++;
		}
	}
	if(pos == -1) error();

	if(getStr) {
		strCpy(token.str, b, pos-1);
		token.size = pos;
	}

	b += pos;	
	return token.str;
}

void QuickParser::error() {
	assert(false);
}


