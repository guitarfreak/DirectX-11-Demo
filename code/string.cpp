
inline void strClear(char* string) {
	string[0] = '\0';
}

int intDigits(int n) {
	int count = 0;
	if(n == 0) return 1;

	while(n != 0) {
		n /= 10;
		count++;
	}

	return count;
}

inline char * intToStr(char * buffer, int var) {
	int digits = intDigits(var);
	if(var < 0) digits++;
	buffer[digits--] = '\0';

	if(var < 0) {
		buffer[0] = '-';
		var *= -1;
	}

	if(var == 0) {
		buffer[0] = '0';
		return buffer;
	}


	while(var != 0) {
		buffer[digits--] = (char)((var % 10) + '0');
		var /= 10;
	}

	return buffer;
};

int intDigits(i64 n) {
	int count = 0;
	if(n == 0) return 1;

	while(n != 0) {
		n /= 10;
		count++;
	}

	return count;
}

inline char * intToStr(char* buffer, i64 var) {
	int digits = intDigits(var);
	if(var < 0) digits++;
	buffer[digits--] = '\0';

	if(var < 0) {
		buffer[0] = '-';
		var *= -1;
	}

	if(var == 0) {
		buffer[0] = '0';
		return buffer;
	}

	while(var != 0) {
		buffer[digits--] = (char)((var % 10) + '0');
		var /= 10;
	}

	return buffer;
};

char* floatToStr(char * buffer, float f, int precision = 0) {
	int stringSize = sprintf(buffer, "%1.*f", precision, f);
	if(precision == 0) {
		int stringIndex = stringSize-1;
		while(buffer[stringIndex] == '0') {
			stringIndex--;
		}
		if(buffer[stringIndex] == '.') {
			stringIndex++;
			buffer[stringIndex] = '0';
		}
		buffer[stringIndex + 1] = '\0'; 
	}

	return buffer;
}

void strCpyBackwards(char* dest, char* str, int size) {
	for(int i = size-1; i >= 0; i--) dest[i] = str[i];
}

inline void strInsert(char* string, int index, char ch) {
	int stringLength = strLen(string);
	strCpyBackwards(string+index+1, string+index, stringLength-index+1);
	string[index] = ch;
}

void strInsert(char* destination, int index, char* str, int size = -1) {
	if(size == -1) size = strLen(str);
	int sizeDest = strLen(destination);

	strCpyBackwards(destination + size + index, destination + index, sizeDest - index + 1);
	strCpyBackwards(destination + index, str, size);
}

void strAppend(char* destination, char* str, int size = -1) {
	int destSize = strLen(destination);
	int strSize = size == -1? strLen(str) : size;

	destination[destSize+strSize] = '\0';

	for(int i = 0; i < strSize; i++) {
		destination[destSize+i] = str[i];
	}
}

inline int charToInt(char c) {
	return (int)c - '0';
}

inline int strToInt(char* string) {
	int result = atoi(string);
	return result;
}

inline int strHexToInt(char* string) {
	int result = (int)strtol(string, 0, 16);
	return result;
}

inline float strToFloat(char* string) {
	float result = atof(string);		
	return result;
}

inline void strErase(char* string, int index, int size) {
	int amount = strLen(string) - index - size;
	memcpy(string+index, string+index+size, amount+1);
}

inline bool strEmpty(char* string) {
	bool result = string[0] == '\0';
	return result;
}

void copySubstring(char * destination, char * source, int start, int end) {
	int size = end - start;
	memcpy(destination, source + start, size + 1);
	destination[size + 1] = '\0';
}

void strCpy(char * destination, char * source, int size = -1) {
	if(size != 0) {
		if(size == -1) size = strLen(source);
		memcpy(destination, source, size);
	}
	destination[size] = '\0';
}

bool strCompare(char* str, char* str2, int size = -1) {
	int length = size == -1 ? strLen(str2) : size;

	if(length != strLen(str)) return false;

	bool result = true;
	for(int i = 0; i < length; i++) {
		if(str[i] != str2[i]) {
			result = false;
			break;
		}
	}

	return result;
}

bool strCompare(char* str, int size1, char* str2, int size2) {
	if(size1 != size2) return false;

	bool result = true;
	for(int i = 0; i < size1; i++) {
		if(str[i] != str2[i]) {
			result = false;
			break;
		}
	}

	return result;
}

bool strStartsWith(char* str, char* str2, int size = -1) {
	int length = size == -1 ? strLen(str2) : size;

	if(strLen(str) < length) return false;

	bool result = true;
	for(int i = 0; i < length; i++) {
		if(str[i] != str2[i]) {
			result = false;
			break;
		}
	}

	return result;
}

int strFind(const char* str, char chr, int startIndex = 0) {
	int index = startIndex;
	int pos = -1;
	while(str[index] != '\0') {
		if(str[index] == chr) {
			pos = index+1;
			break;
		}
		index++;
	}

	return pos;
}

int strFindX(const char* str, char chr, int startIndex = 0) {
	int pos = strFind(str, chr, startIndex);
	if(pos == 0) return -1;
	else return pos;
}

int strFindOrEnd(const char* str, char chr, int startIndex = 0) {
	int pos = strFind(str, chr, startIndex);
	if(pos == 0) return strLen((char*)str);
	else return pos;
}

int strFindBackwards(char* str, char chr, int startIndex = -1) {
	int length = startIndex == -1 ? strLen(str) : startIndex;

	int pos = -1;
	for(int i = length - 1; i >= 0; i--) {
		if(str[i] == chr) {
			pos = i;
			break;
		}
	}

	return pos+1;
}

int strFind(char* source, char* str, int to = 0, int from = 0) {
	int sourceLength = to > 0 ? to : strLen(source);
	int length = strLen(str);

	if(length > sourceLength) return -1;

	bool found = false;
	int pos = -1;
	for(int i = from; i < (sourceLength-length) + 1; i++) {
		if(source[i] == str[0]) {
			bool check = true;
			pos = i;
			for(int subIndex = 1; subIndex < length; subIndex++) {
				if(source[i+subIndex] != str[subIndex]) {
					check = false;
					break;
				}
			}
			if(check) {
				found = true;
				break;
			}
		}
	}

	int result = -1;
	if(found) result = pos;

	return result;
}

int strFindRight(char* source, char* str, int searchDistance = 0) {
	int result = strFind(source, str, searchDistance);
	if(result != -1) result += strLen(str);

	return result;
}

bool strSearchEnd(char* str, int searchLength) {
	bool foundEnd = false;
	for(int i = 0; i < searchLength; i++) {
		if(str[i] == '\0') {
			foundEnd = true;
			break;
		}
	}

	return foundEnd;
}

void strRemove(char* str, int index, int size = 0) {
	int length = size == 0 ? strLen(str) : size;

	memcpy(str + index - 1, str + (index), length-index+1);
}

void strRemoveX(char* str, int index, int amount, int size = 0) {
	int length = size == 0 ? strLen(str) : size;

	memcpy(str + index, str + index+amount, length-index+amount);
}

void strRemoveWhitespace(char* str, int size = 0) {
	int length = size == 0 ? strLen(str) : size;

	int i = 0;
	while(i < length) {
		if(str[i] == '\t' || str[i] == ' ') {
			strRemove(str, i+1, length); 
			length--;
		} else {
			i++;
		}
	}
}

int eatEndOfLineOrFile(char** buffer) {
	char* buf = *buffer;
	int i = 0;
	while(buf[i] != '\r' && buf[i] != '\n' && buf[i] != '\0') i++;

	int lineWidth = i-1;
	if(buf[i] != '\0') {
		while(buf[i] == '\r' || buf[i] == '\n' && buf[i]) {
			i++;
		}
	}
	*buffer = buf + i;

	return lineWidth;
}

//

// Old and not in use right now.

#define initString(function, size) init(function(size), size)
struct String {
	char* data;
	int size;
	int maxSize;

	void init(void* data, int maxSize) {
		this->data = (char*)data;
		this->maxSize = maxSize;
		this->size = 0;
		setEndZero();
	}

	void append(char c) {
		data[size++] = c;
		setEndZero();
	}

	void append(char* str, int length = 0) {
		if(!length) length = strLen(str);
		memcpy(data + size, str, length);
		size += length;
		setEndZero();
	}

	int find(char* str, int start) {
		int len = strLen(str);
		bool found = false;
		int position = 0;
		for(int index = start; index < size-len; index++) {
			if(data[index] == str[0]) {
				position = index+1;
				found = true;
				for(int i = 0; i < len; i++) {
					if(data[index + i] != str[i]) {
						position = 0;
						found = false;
					}
				}
				if(found) break;
			}
		} 

		return position;
	}

	void setEndZero() { data[size] = '\0'; }
};