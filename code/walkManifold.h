
struct WalkMesh {
	XForm xf; // Only box for now.
	XForm xfInv;
	Rect3 aabb;
};

Meta_Parse_Enum();
enum BlockerType {
	BLOCKERTYPE_NONE = 0,
	BLOCKERTYPE_RECT,
	BLOCKERTYPE_CIRCLE,
	BLOCKERTYPE_LINE,
};

struct Blocker {
	int type;

	union {
		XForm xf;
		struct {
			Vec3 linePoints[2];
			Vec3 ab;
		};
	};

	Rect3 aabb;
};

struct MeshSample {
	float z;
	int mesh;
	DArray<int> blockers;
};

struct Pole {
	Vec2 pos;
	DArray<MeshSample> samples;

	DArray<int> potentialMeshes;
	DArray<int> potentialBlockers;
};

struct BlockerPointList {
	int blockerIndex;
	Vec3 points[2];
	int pointCount;
};

struct LayerPole {
	bool valid;
	Vec3 pos;
	MeshSample* sample;
};

struct PointInfo {
	LayerPole* p[2];
	int edgeIndex;
	int playerHeightMode;
};

struct WalkCell;
struct WalkLayer {
	LayerPole poles[4];
	Vec2 zRange;

	WalkCell* cell;

	Vec3 points[3];
	PointInfo pointInfos[2];
	int pointCount;

	bool outside;

	DArray<BlockerPointList> blockerPoints;
	DArray<Line3> linesNoCleanup;
	DArray<Line3> lines;
};

struct WalkCell {
	Vec2i coord;
	Pole* poles[4]; // bl, tl, tr, br.
	DArray<WalkLayer> layers;
	HANDLE mutex;
};

Meta_Parse_Struct(0);
struct WalkManifoldSettings {
	float playerRadius; // @Tag(Range, 0.1, 1.5);
	float legHeight; // @Tag(Range, 0.1, 1);
	float playerHeight;
	float cellSize; // @Tag(Range, 0.05, 1);
	int edgeSearchIterations; // @Tag(Range, 1, 20);
	float lineFlattenPercent; // @Tag(Range, 0, 0.5f);
	int maxLineCollisionCount;

	bool depthTest;
	bool drawWalkEdges;
	bool drawOutsideGrid;
	bool drawLinesNoCleanup;

	Vec4 cGrid;
	Vec4 cGridOutside;
	Vec4 cBlockerLine;
	Vec4 cBlockerLineNoCleanup;
	Vec4 cWalkEdge;

	void init(float cellSize, float playerRadius, float playerHeight, float legHeight) { 
		*this = {}; 

		this->cellSize = cellSize;
		this->playerRadius = playerRadius;
		this->playerHeight = playerHeight;
		this->legHeight = legHeight;

		defaults();
	}

	void defaults() {
		edgeSearchIterations = 14;
		lineFlattenPercent = 0.07f;
		maxLineCollisionCount = 5;

		depthTest = true;
		drawOutsideGrid = false;
		drawLinesNoCleanup = false;
		cGrid = vec4(1,0.8f);
		cGridOutside = vec4(1,0.05f);
		cBlockerLine = vec4(0,1,1,1);
		cBlockerLineNoCleanup = vec4(1,0,0,1);
		cWalkEdge = vec4(1,0,1,1);
	};

	// WalkManifoldSettings(float playerRadius, float legheight, float playerheight, float cellSize) {
	// 	this->playerRadius = playerRadius;
	// 	this->legHeight = legHeight;
	// 	this->playerHeight = playerHeight;
	// 	this->cellSize = cellSize;
	// }
};

struct WalkManifold {
	WalkMesh* meshes;
	int meshCount;
	Blocker* blockers;
	int blockerCount;

	WalkManifoldSettings settings;

	// Temp.

	Recti grid;
	Vec2i gridDim;
	Vec2i cellsDim;
	float playerZ;
	Vec2 zRange;
	float zHeightIgnore;
	Vec2i edgePoles[4];

	//

	DArray<Pole> poles;
	DArray<WalkCell> cells;
	DArray<Blocker> allBlockers;

	//

	void init(WalkManifoldSettings* settings);
	void setup(Vec3 playerPos, int gridRadius, WalkMesh* meshes, int meshCount, Blocker* blockers, int blockerCount);
	void rasterize();
	bool outside(Vec2 pos, float z);
	Vec3 pushInside(Vec2 playerPos, float z);
	Vec3 move(Vec2 playerPos, float z, Vec2 newPos);
	bool raycast(Vec3 pos, Vec3 rayDir, Vec3* inter);
	void debugDraw();
	void free();

	//

	inline Pole*     getPole(int x, int y);
	inline Pole*     getPoleBoundsCheck(int x, int y);
	inline WalkCell* getCell(int x, int y);
	inline WalkCell* getCellBoundsCheck(int x, int y);
	inline WalkCell* getCellGlobal(int x, int y);
	inline WalkCell* getCellGlobalBoundsCheck(int x, int y);

	Vec2i     getCellCoord(Vec2 pos);
	WalkCell* getCell(Vec2 pos);
	float     calcWalkLayerZ(Vec2 pos, WalkLayer* layer);
	float     calcWalkCellZ(Vec2 pos, float z, WalkCell* cell);
	Vec3      calcWalkCellZPos(Vec2 pos, float pz, WalkCell* cell);
	Vec3      calcWalkCellPos(Vec2 pos, float pz);

	WalkLayer* getWalkLayer(WalkCell* cell, float z);
	WalkLayer* getWalkLayer(Vec2i coord, float z);
	WalkLayer* getWalkLayer(WalkCell* cell, WalkLayer* layer);
	WalkLayer* getWalkLayer(Vec2i coord, WalkLayer* layer);

	bool adjacentLayerSharesEdge(WalkLayer* layer, Vec2i coord, int edgeIndex, WalkLayer** adjLayer = 0);

	bool poleMeshIntersection(Vec2 pos, WalkMesh* mesh, float* z);
	bool poleBlockerIntersection(Vec2 pos, float z, Blocker* blocker1);
	bool poleBlockerIntersection(Blocker* blocker0, Blocker* blocker1);

	Vec3 calcWalkEdgePoint(LayerPole* p0, LayerPole* p1 = 0, int zTestMode = 0, bool* hitSomething = 0);
	Vec3 calcWalkEdgePointBlocker(Vec3 a, Vec3 b, Blocker* blocker);

	void addWalkEdgePoint(WalkLayer* layer, Vec3 edgePoint, PointInfo pi);
	void addWalkEdgePointBlocker(WalkLayer* layer, Vec3 edgePoint, int blockerIndex, bool pushSecond);

	bool lineIntersection(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, Vec2 * i);
	bool gridLineCollision(Vec2 p, float z, Vec2 np, Vec2i* cellCoord, Line3* collisionLine, Vec2* collisionPoint, Line3 lastLine);
	int lineCollision(Vec2 p0, Vec2 p1, Line3* lines, int lineCount, Line3 lastLine, Vec2* cp);
};