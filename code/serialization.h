
enum SDataType {
	SData_ATOM = 0,
	SData_STRUCT,
	SData_ARRAY,
};

struct SData {
	char type;

	char typeId;
	char* name;

	union {
		char aData[8]; // Atom.

		struct {
			union {
				int version; // Object.
				int pointerCount; // Array.
			};

			DArray<SData> members;
		};
	};
};
