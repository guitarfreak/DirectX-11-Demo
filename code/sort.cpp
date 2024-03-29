
template <class T> struct SortPair {
	T key;
	int index;
};

template <class S, class T> struct Pair {
	S a;
	T b;
};

bool sortFuncString(void* a, void* b) {
	char* s1 = (char*)a;
	char* s2 = (char*)b;
	int i = 0;
	while(s1[i] == s2[i] && (s1[i] != '\0' && s2[i] != '\0')) i++;

	return s1[i] < s2[i];
}

//

template <class T> void bubbleSort(T* list, int size, bool (*compare) (void* a, void* b)) {
	for(int off = 0; off < size-1; off++) {
		bool sw = false;

		for(int i = 0; i < size-1 - off; i++) {
			if(compare(list + i+1, list + i)) {
				swap(list + i, list + (i+1));
				sw = true;
			}
		}

		if(!sw) break;
	}
}

template <class T> void bubbleSort(T* list, int* indices, int size, bool (*compare) (void* a, void* b)) {
	SortPair<T>* pairs = (SortPair<T>*)mallocX(sizeof(SortPair<T>) * size);
	for(int i = 0; i < size; i++) pairs[i] = {list[i], i};

	bubbleSort(pairs, size, compare);

	for(int i = 0; i < size; i++) indices[i] = pairs[i].index;

	free(pairs);
}

//

template <class T> void mergeSort(T* list, int size, bool (*compare) (void* a, void* b)) {
	T* buffer = (T*)mallocX(sizeof(T)*size);

	int stageCount = ceil(logBase(size, 2));
	for(int stage = 0; stage < stageCount; stage++) {
		int splitSize = 1 << stage;
		int stageSize = 1 << stage+1;

		T* src = stage%2 == 0 ? list : buffer;
		T* dst = stage%2 == 0 ? buffer : list;

		for(int index = 0; index < size; index += stageSize) {
			int i = index;

			int s0, s1;
			int remainder = size - i;
			if(remainder >= stageSize) s0 = s1 = splitSize;
			else {
				s0 = min(splitSize, remainder);
				s1 = min(splitSize, max(remainder-splitSize,0));
			}
			
			int i0 = i;
			int e0 = i + s0;
			int i1 = e0;
			int e1 = e0 + s1;

			while(i0 < e0 && i1 < e1) {
				if(compare(src+i0, src+i1)) dst[i++] = src[i0++];
				else                        dst[i++] = src[i1++];
			}

			if(i0 < e0) memcpy(dst + i, src + i0, sizeof(T)*(e0 - i0));
			else        memcpy(dst + i, src + i1, sizeof(T)*(e1 - i1));
		}
	}

	if(stageCount%2 == 1) memcpy(list, buffer, sizeof(T)*size);

	free(buffer);
}

template <class T> void mergeSort(T* list, int* indices, int size, bool (*compare) (void* a, void* b)) {
	SortPair<T>* pairs = (SortPair<T>*)mallocX(sizeof(SortPair<T>) * size);
	for(int i = 0; i < size; i++) pairs[i] = {list[i], i};

	mergeSort(pairs, size, compare);

	for(int i = 0; i < size; i++) indices[i] = pairs[i].index;

	free(pairs);
}

//

template <class T> void radixSort(T* list, int size, int* (*getKey) (void* a), bool invert = false) {
	T* buffer = (T*)mallocX(sizeof(T)*size);

	int byteCount = 4; // Hardcoded 32 bit values for now. (int/float.)
	for(int byteIndex = 0; byteIndex < byteCount; byteIndex++) {
		T* src = byteIndex%2 == 0 ? list : buffer;
		T* dst = byteIndex%2 == 0 ? buffer : list;
		int bucket[257] = {};
		int bits = 8*byteIndex;

		// Counting.
		for(int i = 0; i < size; i++) {
			uchar byte = *getKey(src + i) >> bits;
			bucket[byte+1]++;
		}

		// Turn counts into offsets.
		for(int i = 0; i < 256-1; i++) {
			bucket[i+1] += bucket[i];
		}

		// Place values into slots.
		for(int i = 0; i < size; i++) {
			uchar byte = *getKey(src + i) >> bits;
			dst[bucket[byte]] = src[i];
			bucket[byte]++;
		}
	}

	free(buffer);

	if(invert) {
		for(int i = 0; i < size/2; i++) {
			swap(list + i, list + (size-1 - i));
		}
	}
}

template <class T> void radixSort(T* list, int* indices, int size, int* (*getKey) (void* a)) {
	SortPair<T>* pairs = (SortPair<T>*)mallocX(sizeof(SortPair<T>) * size);
	for(int i = 0; i < size; i++) pairs[i] = {list[i], i};

	radixSort(pairs, size, getKey);

	for(int i = 0; i < size; i++) indices[i] = pairs[i].index;

	free(pairs);
}

//

template<class T, class Compare> void interleaveArrays(T* d0, int n0, T* d1, int n1, T* buf, Compare cmp) {
	int i0 = 0;
	int i1 = 0;
	int index = 0;

	int total = n0 + n1;
	while(index < total) {
		T cd0 = d0[i0];
		T cd1 = d1[i1];

		if(i0 == n0) {
			buf[index++] = cd1;
			i1++;
			continue;

		} else if(i1 == n1) {
			buf[index++] = cd0;
			i0++;
			continue;
		}

		bool result = cmp(cd0, cd1);
		if(result) {
			buf[index++] = cd0;
			i0++;
		} else {
			buf[index++] = cd1;
			i1++;
		}
	}
}

template<class T, class Compare> void introsortThreaded(T* a, int n, Compare cmp) {
	// static bool start = true;
	// static Statistic stat = {};
	// if(start) {
	// 	stat.begin();
	// 	start = false;
	// }
	// Timer timer;
	// timer.init();
	// timer.start();

	// defer {
	// 	double time = timer.stop();
	// 	stat.update(time);

	// 	printf("%f %f\n", time, stat.getAvg());
	// };

	int threadCount = theThreadQueue->threadCount + 1;

	if(n < threadCount * 100) 
		return ksIntrosort(a, n, cmp);

	struct ThreadData {
		T* array;
		Compare cmp;
	};

	auto func = [](void* data) {
		ThreadHeader* h = (ThreadHeader*)data;
		ThreadData* td = (ThreadData*)h->data;

		ksIntrosort(td->array + h->index, h->count, td->cmp);
	};

	Vec2i* ranges;
	{
		ThreadData td = {a, cmp};
		splitThreadTask(n, &td, func, threadCount, &ranges);
	}

	T* buf = mallocArray(T, n);
	defer { free(buf); };

	int totalCount = 0;
	for(int i = 0; i < threadCount-1; i++) {
		Vec2i range = ranges[i+1];
		totalCount += ranges[i].y;

		T* src = i % 2 == 0 ? a : buf;
		T* dst = i % 2 == 0 ? buf : a;
		interleaveArrays(src, totalCount, a + range.x, range.y, dst, cmp);
	}

	if(threadCount % 2 == 0) copyArray(a, buf, T, n);
}