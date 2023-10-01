
Meta_Parse_Struct(0);
template <class T> struct DArray {
	T* data; // @V0 @SizeVar(count)
	int count; // @V0 @Hide 
	int reserved; // @V0 @Hide

	void* (*allocFunc) (int size);
	void  (*freeFunc) (void* ptr);

	//

	void init(void* (*alloc) (int) = 0, void (*free) (void* ptr) = 0);
	void init(int reservedCount, void* (*alloc) (int));
	void initResize(int resizedCount, void* (*alloc) (int), bool zeroMemory = false);

	int  getNewSizePow(int newCount);
	int  getNewSizeDouble(int newCount);
	void resize(int newCount);
	void reserve(int reserveCount);
	void reserveAdd(int reserveAddedCount);
	void copy(T* d, int n);
	void copy(DArray<T> array);
	void copy(DArray<T>* array);
	void free();
	void freeResize(int n);
	void push(T element);
	void push(T* element);
	void push(T* elements, int n);
	void pushNC(T element);
	void pushNC(T* element);
	void pushNC(T* elements, int n);
	void pushStr(T* string, bool includeZero = false);
	void push(DArray  array);
	void push(DArray* array);
	void insertMove(T element, int index);
	void insert(T element, int index);
	void removeMove(int index, int count);
	T*   find(T value);
	T*   find(bool (*func) (T* e));
	int  findI(T value);
	int  findStr(T value);
	T*   retrieve(int addedCount);
	void zeroMemory();
	void zeroMemoryReserved();

	DArray<T> filter(bool (*test) (void* a), void* (*alloc) (int) = 0);

	bool operator==(DArray<T> array);
	bool operator!=(DArray<T> array);

	void clear();
	T&   first();
	T&   last();
	bool empty();
	T    pop();
	void pop(int n);
	void remove(int i);
	T&   operator[](int i);
	T&   at(int i);
	T*   operator+(int i);
	T*   atr(int i);

	T*   begin() { return data; };
	T*   end() { return data+count; };
};

template <class T> inline DArray<T> dArray(void* (*alloc) (int) = 0, void (*free) (void* ptr) = 0) {
	return {0, 0, 0, alloc, free};
}

template <class T> inline DArray<T> dArray(int reservedCount, void* (*alloc) (int)) {
	DArray<T> array = {0, 0, 0, alloc};
	array.reserve(reservedCount);
	return array;
}

template <class T> inline DArray<T> dArrayResize(int count, void* (*alloc) (int), bool zeroMemory = false) {
	DArray<T> array = {0, 0, 0, alloc};
	array.reserve(count);
	array.count = count;
	if(zeroMemory) array.zeroMemory();
	return array;
}

template <class T> inline DArray<T> dArray(DArray<T>& da, void* (*alloc) (int)) {
	DArray<T> array = {0, 0, 0, alloc};
	array.reserve(da.count);
	array.copy(&da);
	return array;
}

template <class T> inline void DArray<T>::init(void* (*allocFunc) (int) = 0, void (*freeFunc) (void* ptr) = 0) {
	*this = {0, 0, 0, allocFunc, freeFunc};
}

template <class T> inline void DArray<T>::init(int reservedCount, void* (*alloc) (int)) {
	*this = {0, 0, 0, alloc};
	reserve(reservedCount);
}

template <class T> inline void DArray<T>::initResize(int resizedCount, void* (*alloc) (int), bool _zeroMemory) {
	*this = {0, 0, 0, alloc};
	reserve(resizedCount);
	this->count = resizedCount;
	if(_zeroMemory) zeroMemory();
}

template <class T> inline int DArray<T>::getNewSizePow(int newCount) {
	return pow(2.0f, ceil(logBase(max(newCount,2),2)));
}

template <class T> inline int DArray<T>::getNewSizeDouble(int newCount) {
	return max(reserved, 2)*2;
}

// template <class T> void DArray<T>::resize(int newCount) {
// 	// if(autoDouble) newCount = count ? count*2 : newCount;
// 	if(autoDouble) newCount = newCount < 2 ? 2 : pow(2.0f, ceil(logBase(newCount,2)));

// 	T* newData = allocFunc ? (T*)allocFunc(sizeof(T)*newCount) :
// 	                             mallocArray(T, newCount);
// 	copyArray(newData, data, T, count);

// 	if(data) {
// 		if(freeFunc) freeFunc(data);
// 		else ::free(data);
// 	}

// 	data = newData;
// 	reserved = newCount;
// }

template <class T> void DArray<T>::resize(int newCount) {
	T* newData = allocFunc ? (T*)allocFunc(sizeof(T)*newCount) :
	                             mallocArray(T, newCount);
	copyArray(newData, data, T, min(count, newCount));

	if(data) {
		if(allocFunc) {
			if(freeFunc) freeFunc(data);
		} else ::free(data);
	}

	data = newData;
	reserved = newCount;
}

template <class T> void DArray<T>::reserve(int reserveCount) {
	if(reserveCount > reserved) resize(reserveCount);
}

template <class T> void DArray<T>::reserveAdd(int reserveAddedCount) {
	resize(reserved + reserveAddedCount);
}

template <class T> void DArray<T>::copy(T* d, int n) {
	count = 0;
	push(d, n);
}

template <class T> void DArray<T>::copy(DArray<T> array) {
	return copy(array.data, array.count);
}
template <class T> void DArray<T>::copy(DArray<T>* array) {
	return copy(*array);
}

template <class T> void DArray<T>::free() {
	if(data) {
		freeFunc ? freeFunc(data) : ::free(data);
		count = 0;
		data = 0;
		reserved = 0;
	}
}

template <class T> void DArray<T>::freeResize(int n) {
	free();
	resize(n);
}

template <class T> inline void DArray<T>::push(T element) {
	if(count == reserved) resize(getNewSizeDouble(count+1));

	data[count++] = element;
}

template <class T> inline void DArray<T>::push(T* element) {
	if(count == reserved) resize(getNewSizeDouble(count+1));

	data[count++] = *element;
}

template <class T> inline void DArray<T>::push(T* elements, int n) {
	if(count+n-1 >= reserved) resize(getNewSizePow(count+n));

	copyArray(data+count, elements, T, n);
	count += n;
}

template <class T> inline void DArray<T>::pushNC(T element) {
	data[count++] = element;
}

template <class T> inline void DArray<T>::pushNC(T* element) {
	data[count++] = *element;
}

template <class T> inline void DArray<T>::pushNC(T* elements, int n) {
	copyArray(data+count, elements, T, n);
	count += n;
}

template <class T> inline void DArray<T>::pushStr(T* string, bool includeZero) {
	int size = strLen(string);
	if(includeZero) size++;
	push(string, size);
}

template <class T> inline void DArray<T>::push(DArray array) {
	push(array.data, array.count);
}

template <class T> inline void DArray<T>::push(DArray* array) {
	push(array->data, array->count);
}

template <class T> void DArray<T>::insertMove(T element, int index) {
	if(index > count-1) return push(element);

	if(count == reserved) resize(getNewSizeDouble(count+1));

	moveArray(data+index+1, data+index, T, count-(index+1));
	data[index] = element;
	count++;
}

template <class T> void DArray<T>::insert(T element, int index) {
	myAssert(index <= count);
	
	if(index == count) return push(element);
	push(data[index]);
	data[index] = element;
}

template <class T> inline void DArray<T>::removeMove(int index, int n) {
	int end = index + n;
	moveArray(data + index, data + end, T, count - end);
	count -= n;
}

template <class T> T* DArray<T>::find(T value) {
	for(int i = 0; i < count; i++) {
		if(value == data[i]) return data + i;
	}
	return 0;
}

template <class T> T* DArray<T>::find(bool (*func) (T* e)) {
	for(int i = 0; i < count; i++) {
		bool result = func(data + i);
		return data + i;
	}
	return 0;
}

template <class T> int DArray<T>::findI(T value) {
	for(int i = 0; i < count; i++) {
		if(value == data[i]) return i;
	}
	return -1;
}

template <class T> int DArray<T>::findStr(T value) {
	for(int i = 0; i < count; i++) {
		if(strCompare(value, data[i])) return i;
	}
	return -1;
}

template <class T> T* DArray<T>::retrieve(int addedCount) {
	if(count+addedCount-1 >= reserved) resize(getNewSizePow(count+addedCount));

	T* p = data + count;
	count += addedCount;

	return p;
}

template <class T> inline void DArray<T>::zeroMemory() {
	memset(data, 0, sizeof(T)*count);
}

template <class T> inline void DArray<T>::zeroMemoryReserved() {
	memset(data, 0, sizeof(T)*reserved);
}

template <class T> DArray<T> DArray<T>::filter(bool (*test) (void* a), void* (*alloc) (int)) {
	DArray<T> results = dArray<T>(alloc);
	for(auto& it : *this) {
		if(test(&it)) results.push(it);
	}

	return results;
}

template <class T> inline bool DArray<T>::operator==(DArray<T> array) {
	if(count != array.count) return false;
	for(int i = 0; i < count; i++) {
		if(data[i] != array.data[i]) return false;
	}
	return true;
}
template <class T> inline bool DArray<T>::operator!=(DArray<T> array) { return !(*this == array); }

template <class T> inline void DArray<T>::clear()           { count = 0; }
template <class T> inline T&   DArray<T>::first()           { return data[0]; }
template <class T> inline T&   DArray<T>::last()            { return data[count-1]; }
template <class T> inline bool DArray<T>::empty()           { return count == 0; };
template <class T> inline T    DArray<T>::pop()             { return data[--count]; }
template <class T> inline void DArray<T>::pop(int n)        { count -= n; }
template <class T> inline void DArray<T>::remove(int i)     { data[i] = data[--count]; }
template <class T> inline T&   DArray<T>::operator[](int i) { return data[i]; }
template <class T> inline T&   DArray<T>::at(int i)         { return data[i]; }
template <class T> inline T*   DArray<T>::operator+(int i)  { return data + i; }
template <class T> inline T*   DArray<T>::atr(int i)        { return data + i; }

//

template <class T> struct HashTable {

	uint* hashArray; // 0 means unused.
	T* data;
	int size;
	int count;
	int maxCount;
	float maxCountPercent;

	uint (*hashFunction) (T* element);

	//

	void init(int size);
	void init(int size, uint (*hashFunction) (T*), float maxCountPercent = 0.8f);
	void add(T* element);
	void resize();
	T* find(uint hash);
	void clear();
	void free();
};

template <class T> void HashTable<T>::init(int size) {
	hashArray = mallocArray(uint, size);
	data = mallocArray(T, size);

	this->size = size;
	count = 0;
	maxCount = size * maxCountPercent;

	clear();
}

template <class T> void HashTable<T>::init(int size, uint (*hashFunction) (T*), float maxCountPercent = 0.8f) {
	this->hashFunction = hashFunction;
	this->maxCountPercent = maxCountPercent;

	init(size);
}

template <class T> void HashTable<T>::add(T* element) {
	if(count == maxCount-1) resize();

	uint hash = hashFunction(element);
	int index = hash % size;
	while(hashArray[index] != 0) index = (index + 1) % size;
	hashArray[index] = hash+1;
	data[index] = *element;

	count++;
}

template <class T> void HashTable<T>::resize() {
	uint* oldHashArray = hashArray;
	T* oldData = data;

	int oldSize = size;
	init(size * 2);

	for(int i = 0; i < oldSize; i++) {
		if(oldHashArray[i] != 0) add(oldData + i);
	}

	::free(oldHashArray);
	::free(oldData);
}

template <class T> T* HashTable<T>::find(uint hash) {
	int index = hash % size;
	hash += 1;
	while(hashArray[index] != hash) index = (index + 1) % size;

	return data + index;
}

template <class T> void HashTable<T>::clear() {
	memset(hashArray, 0, sizeof(uint)*size);
}

template <class T> void HashTable<T>::free() {
	::free(oldHashArray);
	::free(oldData);
	count = 0;
	size = 0;
}

//

template <class T> struct LinkedList {

	struct Node {
		T data;
		Node* prev;
		Node* next;
	};

	Node* head; // list->prev points to last node.
	int count;

	Node* currentNode;

	bool singly;
	void* (*alloc) (int size);

	//

	void init(bool singly = false, void* (*alloc) (int) = 0);
	void insert(T element, int index);
	void append(T* elements, int elementCount);
	void append(T element);
	void remove(int index);
	void remove();
	void clear();

	T* next();
};

template <class T> void LinkedList<T>::init(bool singly = false, void* (*alloc) (int) = 0) {
	head = 0;
	count = 0;
	this->singly = singly;

	currentNode = 0;

	// We assume custom allocated memory is not beeing freed.
	this->alloc = alloc;
}

template <class T> void LinkedList<T>::insert(T element, int index) {
	Node* newNode = alloc ? (Node*) alloc(sizeof(Node)) : 
							      (Node*)mallocX(sizeof(Node));
	newNode->data = element;

	if(!singly) {
		if(!head) {
			head = newNode;
			head->next = 0;
			head->prev = newNode;

		} else if(index == 0) {
			newNode->prev = head->prev;
			newNode->next = head;
			head->prev = newNode;

			head = newNode;

		} else if(index == count) {
			Node* node = head->prev;

			node->next = newNode;
			newNode->prev = node;
			newNode->next = 0;
			head->prev = newNode; 

		} else {
			Node* prev = head;
			for(int i = 0; i < index-1; i++) prev = prev->next;

			Node* next = prev->next;
			prev->next = newNode;
			next->prev = newNode;
			newNode->prev = prev;
			newNode->next = next;
		}

	} else {
		if(!head) {
			head = newNode;
			head->next = 0;

		} else if(index == 0) {
			newNode->next = head;
			head = newNode;

		} else {
			Node* node = head;
			for(int i = 0; i < index-1; i++) node = node->next;

			Node* next = node->next;
			node->next = newNode;
			newNode->next = next;
		}
	}

	count++;
}

template <class T> void LinkedList<T>::append(T* elements, int elementCount) { 
	for(int i = 0; i < elementCount; i++) insert(elements[i], count); 
}

template <class T> void LinkedList<T>::append(T element) { return insert(element, count); }

template <class T> void LinkedList<T>::remove(int index) {
	if(!singly) {
		if(count == 1) {
			if(!alloc) free(head);
			head = 0;

		} else if(index == 0) {
			Node* next = head->next;
			next->prev = head->prev;
			if(!alloc) free(head);
			head = next;

		} else if(index == count-1) {
			Node* node = head->prev->prev;
			if(!alloc) free(node->next);
			node->next = 0;
			head->prev = node;

		} else {
			Node* node = head;
			for(int i = 0; i < index; i++) node = node->next;

			node->prev->next = node->next;
			node->next->prev = node->prev;

			if(!alloc) free(node);
		}

	} else {
		if(count == 1) {
			if(!alloc) free(head);
			head = 0;

		} else if(index == 0) {
			Node* next = head->next;
			if(!alloc) free(head);
			head = next;

		} else {
			Node* node = head;
			for(int i = 0; i < index-1; i++) node = node->next;

			Node* newNext = node->next->next;
			if(!alloc) free(node->next);
			node->next = newNext;
		}
	}

	count--;
}

template <class T> void LinkedList<T>::remove() { return remove(count-1); }

template <class T> void LinkedList<T>::clear() {
	if(!alloc) {
		if(count == 1) {
			free(head);

		} else {
			Node* node = head->next;
			Node* next = node->next;

			while(next != 0) {
				free(node);
				node = next;
				noxt = node->next;
			}

			free(node);
		}
	}

	head = 0;
	count = 0;
}

template <class T> T* LinkedList<T>::next() {
	if(currentNode == 0) currentNode = head;
	else currentNode = currentNode->next;

	return currentNode;
}