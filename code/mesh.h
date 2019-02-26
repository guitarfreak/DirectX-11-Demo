
struct Material {
	char* name;
	char* file;

	Vec3 Ka;   // ambient
	Vec3 Kd;   // diffuse
	Vec3 Ks;   // specular
	float Ns;  // specularExponent
	Vec3 Ke;   // emission
	float Ni;  // refractionIndex
	float d;   // alpha
	int illum; // illuminationMode

	Texture map_Ka;
	Texture map_Kd;
	Texture map_Ks;
	Texture bump;
	Texture disp;

	bool hasAlpha;

	float heightScale;
};

Meta_Parse_Struct(0);
struct Object {
	// Mesh* m;
	char* name;
	Vec3 pos;
	Vec3 dim;
	Vec4 color;
	Quat rot;

	char* material;
	bool hasAlpha;
};

struct Mesh {
	struct Group {
		char* name;
		int offset;
		int size;

		Material material;
		bool smoothing;
	};

	char* name;
	char* file;
	char* mtlFile;

	Group* groups;
	int groupCount;

	ID3D11Buffer* vertexBuffer;
	int size;

	DArray<MeshVertex> vertices;
	Rect3 boundingBox;

	bool swapWinding;

	bool hasAlpha;
	bool doubleSided;

	//

	AnimationPlayer animPlayer;

	Bone* bones;
	int boneCount;
	BoneNode boneTree;

	XForm* basePose;
	Rect3* boneBoundingBoxes;

	Animation animations[10];
	int animationCount;

	//

	AssetInfo assetInfo[2];
};