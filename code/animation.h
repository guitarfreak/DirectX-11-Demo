
struct Bone {
	char* name;
	int index;
	int depth;
};

struct BoneNode {
	Bone* data;
	BoneNode* children;
	int childCount;
};

struct Animation {
	char* name;

	int startTime;
	int endTime;
	float speed;
	float fps;
	int playbackMode;

	Bone* bones;
	int boneCount;

	XForm** frames;
	int frameCount;
};

struct Mesh;
struct AnimationPlayer {
	Mesh* mesh;
	Animation* animation;

	bool init;

	XForm bones[100];
	int boneCount;

	Mat4 mats[100];

	float time;
	float dt;

	float fps;
	float speed;

	bool loop;
	bool noInterp;
	bool noLocomotion;

	void setAnim(char* name);
	void update(float dt);
};