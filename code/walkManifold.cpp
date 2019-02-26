
struct WalkMesh {
	XForm form; // Only box for now.
};

struct Pole {
	Vec3 pos;
	bool inside;
	int meshIndex;

	int* blockers;
	int blockerCount;
};

enum BlockerType {
	BLOCKERTYPE_RECT = 0,
	BLOCKERTYPE_CIRCLE,
	BLOCKERTYPE_LINE,
};

struct Blocker {
	int type;

	union {
		XForm form;
		Vec3 linePoints[2];
	};
};

struct WalkCell {
	struct BlockerPointList {
		int blockerIndex;
		Vec3 points[2];
		int pointCount;
	};

	Pole* poles[4]; // bl, tl, tr, br.

	int pointCount;
	Vec3 points[3];

	BlockerPointList* blockerPoints;
	int blockerPointCount;

	Line3* lines;
	int lineCount;
};

struct WalkManifold {
	WalkMesh* meshes;
	int meshCount;

	Blocker* blockers;
	int blockerCount;

	float playerRadius;
	Vec2 zRange; // Temp.
	float legHeight;
	float cellSize;
	int edgeSearchIterations;

	Recti grid;
	Vec2i gridDim;  // Temp.
	Vec2i cellsDim; // Temp.

	Pole* poles;
	int poleCount;

	WalkCell* cells;
	int cellCount;

	//

	Vec3 off; // Temp.

	//

	void init(Vec3 playerPos, Recti grid, float playerRadius, float legHeight, float cellSize, WalkMesh* meshes, int meshCount, Blocker* blockers, int blockerCount);
	void rasterize();
	Vec3 move(Vec2 playerPos, Vec2 newPos);
	bool raycast(Vec3 pos, Vec3 rayDir, Vec3* inter);
	void debugDraw();

	//

	bool lineIntersection(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, Vec2 * i);
	Vec2 gridLineCollision(Vec2 p, Vec2 np, Vec2i* cellCoord);

	WalkCell* getWalkCell(Vec2i cellCoord);
	Vec2i getWalkCell(Vec2 pos);
	float calcWalkCellZ(Vec2 pos, WalkCell* cell);
	Vec3 calcWalkCellZPos(Vec2 pos, WalkCell* cell);
	Vec3 calcWalkCellPos(Vec2 pos);
	bool poleMeshIntersection(Vec2 pos, WalkMesh* mesh, float* z);
	bool poleBlockerIntersection(Vec3 pos, Blocker* blocker);
	Vec3 calcWalkEdgePoint(Vec3 a, Vec3 b, WalkMesh* mesh, bool checkZ);
	Vec3 calcWalkEdgePoint(Pole* p1, Pole* p2);
	Vec3 calcWalkEdgePointBlocker(Vec3 a, Vec3 b, Blocker* blocker);
	void addWalkEdgePoint(Pole* p1, Pole* p2, WalkCell* cell, Vec3 edgePoint);
	void addWalkEdgePointBlocker(WalkCell* cell, Vec3 edgePoint, int blockerIndex, bool pushSecond);
	int lineCollision(Vec2 p0, Vec2 p1, Line3* lines, int lineCount, Line3 lastLine, Vec2* cp);
};

bool WalkManifold::lineIntersection(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, Vec2 * i) {

	// If line is inside other line. (This might not be sufficient.)
	if(distancePointLine(p2, p3, p0) == 0 && 
	   distancePointLine(p2, p3, p1) == 0) {
		*i = p0;
		return true;
	}

	bool result = getLineIntersection(p0, p1, p2, p3, i);
	return result;
}

Vec2 WalkManifold::gridLineCollision(Vec2 p, Vec2 np, Vec2i* cellCoord) {

	Vec2i cellDirs[] = { {-1,0}, {1,0}, {0,-1}, {0,1}, };
	int cellEdgeIndexOpposite[] = {1,0,3,2};

	Vec2i newPosCell = getWalkCell(np);

	WalkCell* cell = &cells[aIndex(cellsDim.w, cellCoord->x-grid.left, cellCoord->y-grid.bottom)];

	Rect cellRect = rect(vec2(*cellCoord)*cellSize, vec2(*cellCoord+1)*cellSize);

	Vec2 dir = np - p;

	if(dir.x == 0 && dir.y == 0) return np;

	Vec2 dirNorm = norm(dir);

	Vec2 dirFrac = 1.0f / dirNorm;
	float t[] = {
		(cellRect.left   - p.x) * dirFrac.x,
		(cellRect.right  - p.x) * dirFrac.x,
		(cellRect.bottom - p.y) * dirFrac.y,
		(cellRect.top    - p.y) * dirFrac.y,
	};

	int i = 0;
	int indices[2];

	if(dir.x < 0) indices[i++] = 0;
	if(dir.x > 0) indices[i++] = 1;
	if(dir.y < 0) indices[i++] = 2;
	if(dir.y > 0) indices[i++] = 3;

	int index = -1;

	float dist;
	if(i == 1) {
		dist = t[indices[0]];
		index = indices[0];

	} else if(i == 2) {
		if(t[indices[0]] < t[indices[1]]) {
			dist = t[indices[0]];
			index = indices[0];
		} else {
			dist = t[indices[1]];
			index = indices[1];
		}
	}

	if(dist < len(dir)) {
		p += dirNorm * dist;
		*cellCoord += cellDirs[index];

		return p;
	}

	assert(newPosCell == *cellCoord);

	return np;
}

void WalkManifold::init(Vec3 playerPos, Recti grid, float playerRadius, float legHeight, float cellSize, WalkMesh* meshes, int meshCount, Blocker* blockers, int blockerCount) {

	*this = {};

	this->grid = grid;
	this->gridDim = vec2i(grid.w() + 1, grid.h() + 1);
	this->cellsDim = vec2i(grid.w(), grid.h());

	this->playerRadius = playerRadius;
	this->zRange = vec2(playerPos.z - legHeight, playerPos.z + legHeight);
	this->legHeight = legHeight;
	this->cellSize = cellSize;

	// this->edgeSearchIterations = 16;
	this->edgeSearchIterations = 10;

	this->meshes = meshes;
	this->meshCount = meshCount;
	this->blockers = blockers;
	this->blockerCount = blockerCount;

	poleCount = gridDim.w * gridDim.h;
	poles = getTArray(Pole, poleCount);

	cellCount = cellsDim.w * cellsDim.h;
	cells = getTArray(WalkCell, cellCount);
	memset(cells, 0, sizeof(WalkCell)*cellCount);

	off = vec3(0,0,0.01f);
}

void WalkManifold::rasterize() {

	// Set poles for walk cells.
	for(int y = 0; y < cellsDim.h; y++) {
		for(int x = 0; x < cellsDim.w; x++) {

			WalkCell* cell = &cells[aIndex(cellsDim.w, x, y)];

			Pole* ps[] = {
				&poles[aIndex(gridDim.w, x, y)],
				&poles[aIndex(gridDim.w, x, y+1)],
				&poles[aIndex(gridDim.w, x+1, y+1)],
				&poles[aIndex(gridDim.w, x+1, y)],
			};

			for(int i = 0; i < 4; i++) cell->poles[i] = ps[i];
		}
	}

	// Sample poles.
	for(int y = 0; y < gridDim.h; y++) {
		for(int x = 0; x < gridDim.w; x++) {

			Pole* pole = &poles[aIndex(gridDim.w, x, y)];

			Vec2 pos = vec2((grid.left + x)*cellSize, (grid.bottom + y)*cellSize);
			*pole = {vec3(pos, -FLT_MAX), false, -1, 0, 0};

			for(int i = 0; i < meshCount; i++) {
				float sample;
				if(poleMeshIntersection(pos, meshes + i, &sample)) {
					if(sample >= zRange.x && sample <= zRange.y) {
						pole->inside = true;
					}
					pole->pos.z = max(sample, pole->pos.z);
					pole->meshIndex = i;
				}
			}
		}
	}

	// Calc cells.
	for(int y = 0; y < gridDim.h; y++) {
		for(int x = 0; x < gridDim.w; x++) {

			Pole* pole = &poles[aIndex(gridDim.w, x, y)];

			for(int stage = 0; stage < 2; stage++) {

				Pole* pole2 = 0;
				if(stage == 0 && x < gridDim.w-1) pole2 = &poles[aIndex(gridDim.w, x+1, y)];
				if(stage == 1 && y < gridDim.h-1) pole2 = &poles[aIndex(gridDim.w, x, y+1)];

				if(!pole2) continue;

				WalkCell* cell = 0;
				if((x < gridDim.w-1) && y < gridDim.h-1) cell = &cells[aIndex(cellsDim.w, x, y)];

				WalkCell* cell2 = 0;
				if      (stage == 0 && (y > 0 && x < gridDim.w-1))  cell2 = &cells[aIndex(cellsDim.w, x, y-1)];
				else if (stage == 1 && (x > 0 && y < gridDim.h-1)) cell2 = &cells[aIndex(cellsDim.w, x-1, y)];

				if(pole->inside != pole2->inside) {

					Pole* p1 = pole->inside ? pole : pole2; 
					Pole* p2 = pole->inside ? pole2 : pole;

					Vec3 edgePoint = calcWalkEdgePoint(p1, p2);

					if(cell) {
						addWalkEdgePoint(p1, p2, cell, edgePoint);

						// dxDrawLine(p1->pos+off, edgePoint+off, vec4(1,1));
				   }

					if(cell2) {
						addWalkEdgePoint(p1, p2, cell2, edgePoint);
					}

				} else {
					// if(pole->inside && pole2->inside)
						// dxDrawLine(pole->pos+off, pole2->pos+off, vec4(1,1));
				}
			}
		}
	}

	DArray<Blocker> allBlockers = dArray<Blocker>(getTMemory);

	for(int i = 0; i < blockerCount; i++) {
		allBlockers.push(blockers[i]);
	}

	// Collect walk edges and add them as line blockers.
	for(int i = 0; i < cellCount; i++) {
		WalkCell* cell = cells + i;

		if(cell->pointCount > 1) {
			if(cell->pointCount == 2) {
				Blocker b = {BLOCKERTYPE_LINE};
				b.linePoints[0] = cell->points[0];
				b.linePoints[1] = cell->points[1];

				allBlockers.push(b);
			} 
			// else if(cell->pointCount == 3) {
				// allBlockers.push({BLOCKERTYPE_LINE, xForm(), cell->points[0], cell->points[2]});
				// allBlockers.push({BLOCKERTYPE_LINE, xForm(), cell->points[2], cell->points[1]});
			// }
		}
		cell->pointCount = 0;
	}

	// Sample poles for allBlockers.
	for(int y = 0; y < gridDim.h; y++) {
		for(int x = 0; x < gridDim.w; x++) {

			Pole* pole = &poles[aIndex(gridDim.w, x, y)];

			for(int i = 0; i < allBlockers.count; i++) {
				bool isBlocked = poleBlockerIntersection(pole->pos, allBlockers + i);

				if(isBlocked) {
					// if(pole->inside) {
						if(!pole->blockers) pole->blockers = getTArray(int, 10);
						pole->blockers[pole->blockerCount++] = i;
						// poles[i]->inside = !isBlocked;
					// }
				}
			}
		}
	}

	// Calc blockers.
	for(int y = 0; y < gridDim.h; y++) {
		for(int x = 0; x < gridDim.w; x++) {

			Pole* pole = &poles[aIndex(gridDim.w, x, y)];

			for(int stage = 0; stage < 2; stage++) {

				Pole* pole2 = 0;
				if(stage == 0 && x < gridDim.w-1) pole2 = &poles[aIndex(gridDim.w, x+1, y)];
				if(stage == 1 && y < gridDim.h-1) pole2 = &poles[aIndex(gridDim.w, x, y+1)];

				if(!pole2) continue;

				WalkCell* cell = 0;
				if((x < gridDim.w-1) && y < gridDim.h-1) cell = &cells[aIndex(cellsDim.w, x, y)];

				WalkCell* cell2 = 0;
				if      (stage == 0 && (y > 0 && x < gridDim.w-1)) cell2 = &cells[aIndex(cellsDim.w, x, y-1)];
				else if (stage == 1 && (x > 0 && y < gridDim.h-1)) cell2 = &cells[aIndex(cellsDim.w, x-1, y)];

				for(int poleStage = 0; poleStage < 2; poleStage++) {
					Pole* p0 = poleStage == 0 ? pole : pole2;
					Pole* p1 = poleStage == 0 ? pole2 : pole;

					for(int i0 = 0; i0 < p0->blockerCount; i0++) {
						int blockerIndex = p0->blockers[i0];

						bool found = false;
						for(int i1 = 0; i1 < p1->blockerCount; i1++) {
							if(p1->blockers[i1] == blockerIndex) {
								found = true;
								break;
							}
						}

						if(!found) {
							Blocker* blocker = allBlockers + blockerIndex;
							Vec3 edgePoint = calcWalkEdgePointBlocker(p1->pos, p0->pos, blocker);

							bool pushSecond;
							if(stage == 0) pushSecond = poleStage == 1 ? false : true;
							else           pushSecond = poleStage == 1 ? true : false;

							if(cell) {
								addWalkEdgePointBlocker(cell, edgePoint, blockerIndex, pushSecond);
						   }

							if(cell2) {
								addWalkEdgePointBlocker(cell2, edgePoint, blockerIndex, !pushSecond);
							}
						}
					}
				}
			}
		}
	}
}

Vec3 WalkManifold::move(Vec2 playerPos, Vec2 newPos) {
	float offset = floatPrecisionOffset(playerPos, newPos, 10, 0.000001f);

	Vec2 p = playerPos;
	Vec2 np = newPos;

	Vec2i cellCoord = getWalkCell(p);

	Line3 lastLine = {vec3(FLT_MAX), vec3(FLT_MAX)};

	Vec2 projectDir;

	bool firstLineHit = false;
	int lineHitCount = 0;

	bool printOut = false;

	bool debugDraw = true;
	int index = -1;

	Vec2 lastSavePos = p;

	while(true) {
		index++;

		// Get line segment to cell border. (Or to end point if we don't touch a border.)
		Vec2i cellCoordOld = cellCoord;
		Vec2 cp = gridLineCollision(p, np, &cellCoord);

		bool collision = cp == np ? false : true;
	// float calcWalkCellZ(Vec2 pos, WalkCell* cell);
		
		// 			dxDrawLine(calcWalkCellPos(op), 

		Vec3 p0 = calcWalkCellZPos(p, getWalkCell(cellCoordOld))+off;
		Vec3 p1 = calcWalkCellZPos(cp, getWalkCell(cellCoord))+off;
		float r = 0.0f + (index/10.0f);
		dxDrawLine(p0, p1, vec4(r,1,r,1));

		p = cp;

		// Get gjk collision with line segment.
		// Blocker srcBlocker = {BLOCKERTYPE_CIRCLE, xForm(pos, vec3(playerRadius))};
		// bool result = gjk(&srcBlocker, vec2(0,0), blocker);

		if(index == 10) break;

		// 		dxDrawLine(line.a+off, line.b+off, vec4(1,0,0,1));

		// if(!collision) {
		// 	p = np;
		// 	break;

		// } else {

		// 	if(debugDraw) {
		// 		dxDrawLine(line.a+off, line.b+off, vec4(1,0,0,1));
		// 	}

		// 	index++;

		// 	lastLine = line;

		// 	// Project onto line.
		// 	{
		// 		if(!firstLineHit) {
		// 			projectDir = np - cp;
		// 			firstLineHit = true;
		// 		}

		// 		Vec2 a = norm(line.a.xy - line.b.xy);
		// 		Vec2 b = projectDir;
		// 		Vec2 pp = cp + a * dot(a, b);

		// 		projectDir = norm(projectDir) * len(cp - pp);

		// 		Vec2 normal;
		// 		{
		// 			normal = rotateRight(a);
		// 			// if(dot(normal, line.a.xy - p) < 0) {
		// 			// 	normal = rotateLeft(a);
		// 			// }

		// 			if(debugDraw) {
		// 				Vec3 mid = (line.a + line.b) / 2.0f;
		// 				Vec3 ln = -norm(vec3(normal, 0));

		// 				dxDrawLine(mid + off, 
		// 				           mid + ln*0.05f + off, vec4(0,1,1,1));
		// 			}
		// 		}

		// 		Vec2 op  = cp - normal * offset;
		// 		Vec2 onp = pp - normal * offset;

		// 		if(debugDraw) {
		// 			dxDrawLine(calcWalkCellPos(op), 
		// 			           calcWalkCellPos(op) + vec3(0,0,0.05f), vec4(1,0.5f,0,1));

		// 			dxDrawLine(calcWalkCellPos(onp), 
		// 			           calcWalkCellPos(onp) + vec3(0,0,0.05f), vec4(1,0,0.5f,1));
		// 		}

		// 		// Check if we can do the offset without a collision.
		// 		{
		// 			Line3 testLine;
		// 			Vec2 ncp;
		// 			bool collision = gridLineCollision(cp, op, &cellCoord, &testLine, &ncp, lastLine);

		// 			if(collision) {
		// 				// Can't do the offset so we try to compromise by taking 
		// 				// the intersection with the perpendicular offset line.
		// 				// And if that failes as well we just bail out and don't move.

		// 				Line3 ol = { vec3(line.a.xy - normal * offset,0), 
		// 					         vec3(line.b.xy - normal * offset,0) };
		// 				Vec2 ep;
		// 				if(lineIntersection(p, cp, ol.a.xy, ol.b.xy, &ep)) {
		// 					p = ep;
		// 				} else {
		// 					p = lastSavePos;
		// 				}

		// 				break;
		// 			}
		// 		}

		// 		p = op;
		// 		np = onp;
		// 	}

		// 	lastSavePos = p;

		// 	if(lineHitCount == 5) break;
		// 	lineHitCount++;

		// 	continue;
		// }
	}

	Vec3 cellPos = calcWalkCellPos(p);
	return cellPos;
}

bool WalkManifold::raycast(Vec3 pos, Vec3 rayDir, Vec3* inter) {
	bool foundIntersection = false;
	Vec3 intersection = vec3(0,0,0);
	float minDist = FLT_MAX;
	for(int i = 0; i < cellCount; i++) {
		WalkCell* cell = cells + i;

		Vec3 p[] = {cell->poles[0]->pos, 
						cell->poles[1]->pos, 
						cell->poles[2]->pos, 
						cell->poles[3]->pos };

		bool skipCell = false;
		for(int j = 0; j < 4; j++) {
			if(p[j].z < 0) {
				skipCell = true;
				break;
			}
		}
		if(skipCell) continue;

		Vec3 inter;
		bool result = lineQuadIntersection(pos, rayDir, p[0], p[1], p[2], p[3], &inter, true);
		if(result) {
			float dist = len(inter - pos);
			if(dist < minDist) {
				minDist = dist;
				intersection = inter;
			}

			foundIntersection = true;
		}
	}

	if(!foundIntersection) return false;

	*inter = intersection;
	return true;
}

void WalkManifold::debugDraw() {

	// Draw blockers.
	for(int y = 0; y < gridDim.h; y++) {
		for(int x = 0; x < gridDim.w; x++) {

			Pole* pole = &poles[aIndex(gridDim.w, x, y)];

			for(int stage = 0; stage < 2; stage++) {

				Pole* pole2 = 0;
				if(stage == 0 && x < gridDim.w-1)  pole2 = &poles[aIndex(gridDim.w, x+1, y)];
				if(stage == 1 && y < gridDim.h-1) pole2 = &poles[aIndex(gridDim.w, x, y+1)];

				if(!pole2) continue;

				WalkCell* cell = 0;
				if((x < gridDim.w-1) && y < gridDim.h-1) cell = &cells[aIndex(cellsDim.w, x, y)];

				// WalkCell* cell2 = 0;
				// if      (stage == 0 && (y > 0 && x < gridDim.w-1))  cell2 = &cells[aIndex(cellsDim.w, x, y-1)];
				// else if (stage == 1 && (x > 0 && y < gridDim.h-1)) cell2 = &cells[aIndex(cellsDim.w, x-1, y)];

				// if(pole->inside && pole->blockerCount == 0) {
				// 	dxDrawLine(pole->pos, pole->pos+vec3(0,0,0.2f), vec4(0,1,1,1));
				// } else {
				// 	dxDrawLine(pole->pos, pole->pos+vec3(0,0,0.1f), vec4(0,1,1,1));
				// }

				bool in0 = pole->inside && pole->blockerCount == 0;
				bool in1 = pole2->inside && pole2->blockerCount == 0;

				if(in0 || in1) {
					Pole* p0 = pole;
					Pole* p1 = pole2;

					if(in0 && in1) {
						dxDrawLine(p0->pos+off, p1->pos+off, vec4(1,1,1,1));
					} else {
						if(in1) swap(&p0, &p1);

						if(cell) {
							Vec2 edgePos;
							float dist = FLT_MAX;
							for(int i = 0; i < cell->lineCount; i++) {
								Line3 line = cell->lines[i];
								Vec2 cp;
								bool result = getLineIntersection(p0->pos.xy, p1->pos.xy, line.a.xy, line.b.xy, &cp);	
								if(result) {
									float cDist = len(p0->pos.xy - cp);
									if(cDist < dist) {
										dist = cDist;
										edgePos = cp;
									}
								}
							}

							// dxDrawLine(p0->pos+off, (p1->pos + p0->pos)/2.0f+off, vec4(1,1,1,1));
							dxDrawLine(p0->pos+off, (vec3(edgePos, p1->pos.z))+off, vec4(1,1,1,1));
						}
					}
				}
			}

		}
	}

	// Draw Lines.
	for(int i = 0; i < cellCount; i++) {
		WalkCell* cell = cells + i;

		Vec4 c = vec4(0,1,1,1);
		if(cell->pointCount == 2) {
			dxDrawLine(cell->points[0]+off, cell->points[1]+off, c);
		}

		// if(cell->pointCount == 3) {
		// 	dxDrawLine(cell->points[0]+off, cell->points[2]+off, c);
		// 	dxDrawLine(cell->points[2]+off, cell->points[1]+off, c);

		// 	// dxDrawLine(cell->points[0]+off, cell->points[2]+off, c);
		// 	// dxDrawLine(cell->points[2]+off, cell->points[1]+off, c);
		// }

		for(int i = 0; i < cell->lineCount; i++) {
			Line3 line = cell->lines[i];
			if(line.a.z > -1000000 && line.b.z > -1000000)
				dxDrawLine(line.a+off, line.b+off, c);

			// printf("%f %f\n", line.a.z, -FLT_MAX);
		}
	}

	// Draw Blockers.
	if(false)
	{
		dxDepthTest(true);
		// dxFillWireFrame(true);
		for(int i = 0; i < blockerCount; i++) {
			XForm b = blockers[i].form;
			// dxDrawCube(b.trans, b.scale, vec4(1,0,0,0.5f));
			dxDrawCube(b.trans, b.scale, vec4(0.7,0,0,1));
		}
		// dxFillWireFrame(false);
	}
}

//

WalkCell* WalkManifold::getWalkCell(Vec2i cellCoord) {
	WalkCell* cell = &cells[aIndex(cellsDim.w, cellCoord.x-grid.left, cellCoord.y-grid.bottom)];
	return cell;
}

Vec2i WalkManifold::getWalkCell(Vec2 pos) {
	Vec2 cellF = pos / cellSize;
	Vec2i cellCoord = vec2i(floor(cellF.x), floor(cellF.y));

	return cellCoord;
}

float WalkManifold::calcWalkCellZ(Vec2 pos, WalkCell* cell) {

	Pole** ps = cell->poles;
	Vec3 coords = barycentricCoordinates(pos, ps[0]->pos.xy, ps[1]->pos.xy, ps[2]->pos.xy);

	// If not in first triangle choose the second triangle of quad.
	if(coords.x < 0 || coords.y < 0 || coords.z < 0) {
		coords = barycentricCoordinates(pos, ps[2]->pos.xy, ps[3]->pos.xy, ps[0]->pos.xy);
	}

	float z = coords.x * ps[0]->pos.z + 
	          coords.y * ps[1]->pos.z + 
	          coords.z * ps[2]->pos.z;

	return z;
}

Vec3 WalkManifold::calcWalkCellZPos(Vec2 pos, WalkCell* cell) {

	float z = calcWalkCellZ(pos, cell);
	return vec3(pos, z);
}

Vec3 WalkManifold::calcWalkCellPos(Vec2 pos) {
	Vec2i cellCoord = getWalkCell(pos);
	WalkCell* cell = &cells[aIndex(cellsDim.w, cellCoord.x-grid.left, cellCoord.y-grid.bottom)];

	Vec3 wp = calcWalkCellZPos(pos, cell);

	return wp;
}

bool WalkManifold::poleMeshIntersection(Vec2 pos, WalkMesh* mesh, float* z) {
	XForm b = mesh->form;

	Vec3 rayPos = vec3(pos, 100);
	Vec3 rayDir = vec3(0,0,-1);
	Quat rotInverse = inverse(b.rot);

	Vec3 lp, ld;
	lp = rayPos - b.trans;
	ld = lp + rayDir;
	lp = rotInverse * lp;
	ld = rotInverse * ld;
	ld = norm(ld - lp);

	Vec3 position, normal;
	float distance = lineBoxIntersection(lp, ld, vec3(0.0f), b.scale, &position, &normal);

	if(distance != -1) {
		position = b.rot * position;
		position = position + b.trans;

		*z = position.z;
		return true;

	} else {
		return false;
	}
}

bool gjk(Blocker* s1, Vec2 oldPos, Blocker* s2, Vec2* inputSimplex = 0, bool sweepingEnabled = false);
bool WalkManifold::poleBlockerIntersection(Vec3 pos, Blocker* blocker) {
	XForm b = blocker->form;

	Vec2 blockerZRange;
	if(blocker->type == BLOCKERTYPE_LINE) {
		blockerZRange.min = min(blocker->linePoints[0].z, blocker->linePoints[1].z);
		blockerZRange.max = max(blocker->linePoints[0].z, blocker->linePoints[1].z);
	} else {
		blockerZRange = vec2(b.trans.z - b.scale.z/2.0f, 
                    b.trans.z + b.scale.z/2.0f);
	}

	if(!(pos.z >= blockerZRange.min && pos.z <= blockerZRange.max)) return false;

	Blocker srcBlocker = {BLOCKERTYPE_CIRCLE, xForm(pos, vec3(playerRadius))};
	bool result = gjk(&srcBlocker, vec2(0,0), blocker);

	return result;
}

Vec3 WalkManifold::calcWalkEdgePoint(Vec3 a, Vec3 b, WalkMesh* mesh, bool checkZ) {
	Vec3 c;
	float z = a.z;
	for(int i = 0; i < edgeSearchIterations; i++) {
		c = (a+b)/2.0f;

		bool hit = poleMeshIntersection(c.xy, mesh, &z);
		if(checkZ) {
			if(!(z >= zRange.x && z <= zRange.y)) hit = false;
		}

		if(hit) a = c;
		else b = c;
	}

	return vec3(c.xy, z);
}

Vec3 WalkManifold::calcWalkEdgePoint(Pole* p1, Pole* p2) {
	return calcWalkEdgePoint(p1->pos, p2->pos, meshes + p1->meshIndex, p2->meshIndex != -1);
}

Vec3 WalkManifold::calcWalkEdgePointBlocker(Vec3 a, Vec3 b, Blocker* blocker) {

	Vec3 c;
	for(int i = 0; i < edgeSearchIterations; i++) {
		c = (a+b)/2.0f;

		bool hit = !poleBlockerIntersection(c, blocker);

		if(hit) a = c;
		else b = c;
	}

	return c;
}

void WalkManifold::addWalkEdgePoint(Pole* p1, Pole* p2, WalkCell* cell, Vec3 edgePoint) {

	if(cell->pointCount == 0) {
		// cell->tempPoles[0] = p1;
		// cell->tempPoles[1] = p2;
		cell->points[cell->pointCount++] = edgePoint;

		return;
	}

	if(cell->pointCount == 1) {
		cell->points[cell->pointCount++] = edgePoint;

		return;
	}

	if(cell->pointCount > 1) {
		__debugbreak();
		return;
	}

	#if 0

	Pole* poles[4] = {cell->tempPoles[0], cell->tempPoles[1], p1, p2};

	// Calc third point.
	{
		Vec3 testEp[2];

		for(int i = 0; i < 2; i++) {
			Pole pole0 = i == 0 ? *cell->tempPoles[0] : *p1;
			Pole pole1 = i == 0 ? *cell->tempPoles[1] : *p2;

			float percent = 0.1f;
			Vec3 offset = {};
			if(pole0.pos.x != pole1.pos.x)
				offset.y = percent * (cell->points[(i+1)%2].y - cell->points[i].y);
			else 
				offset.x = percent * (cell->points[(i+1)%2].x - cell->points[i].x);

			pole0.pos += offset;
			pole1.pos += offset;

			testEp[i] = calcWalkEdgePoint(&pole0, &pole1, meshes, zRange);

			// dxDrawLine(pole0->pos, pole0->pos+vec3(0,0,0.2f), vec4(1,1,0,1));
			// dxDrawLine(pole1->pos, pole1->pos+vec3(0,0,0.2f), vec4(1,0.5f,0,1));

			// dxDrawLine(testEp[0], testEp[0]+vec3(0,0,0.2f), vec4(1,0.5f,0,1));
			// dxDrawLine(testEp[1], testEp[1]+vec3(0,0,0.2f), vec4(1,0.5f,0,1));

			// dxDrawLine(pole0->pos + offset, pole0->pos + offset+vec3(0,0,0.2f), vec4(1,0.0f,0,1));
			// dxDrawLine(pole1->pos + offset, pole1->pos + offset+vec3(0,0,0.2f), vec4(1,0.5f,0,1));
		}

		Vec3 dir0 = norm(testEp[0] - cell->points[0]);
		Vec3 dir1 = norm(testEp[1] - cell->points[1]);

		Vec3 thirdPoint = lineClosestPoint(cell->points[0], dir0, cell->points[1], dir1);

		// Something went wrong.
		if(!pointInRect(thirdPoint.xy, cellRect)) {
			thirdPoint = (cell->points[0] + cell->points[1])/2.0f;
		}

		cell->points[cell->pointCount++] = thirdPoint;
	}

	#endif
}

void WalkManifold::addWalkEdgePointBlocker(WalkCell* cell, Vec3 edgePoint, int blockerIndex, bool pushSecond) {

	if(!cell->blockerPoints) {
		cell->blockerPoints = getTArray(WalkCell::BlockerPointList, 10);
		cell->blockerPointCount = 0;
	}

	WalkCell::BlockerPointList* list = 0;
	for(int i = 0; i < cell->blockerPointCount; i++) {
		if(cell->blockerPoints[i].blockerIndex == blockerIndex) {
			list = &cell->blockerPoints[i];
			break;
		}
	}

	if(!list) {
		list = &cell->blockerPoints[cell->blockerPointCount++];
		list->pointCount = 0;
		list->blockerIndex = blockerIndex;
	}

	list->points[list->pointCount++] = edgePoint;

	if(list->pointCount == 2) {
		if(!cell->lines) cell->lines = getTArray(Line3, 10);

		// Make sure to push lines in clockwise order from blocker perspective.
		Line3 line;
		if(!pushSecond) line = { list->points[0], list->points[1] };
		else            line = { list->points[1], list->points[0] };

		cell->lines[cell->lineCount++] = line;
	}

	if(list->pointCount > 2) {
		__debugbreak();
		return;
	}
}

int WalkManifold::lineCollision(Vec2 p0, Vec2 p1, Line3* lines, int lineCount, Line3 lastLine, Vec2* cp) {
	int lineIndex = -1;
	Vec2 collisionPoint = {};
	float collisionDist = FLT_MAX;
	for(int i = 0; i < lineCount; i++) {
		Line3 line = lines[i];

		if(lastLine.a == line.a && lastLine.b == line.b) continue;

		Vec2 cp; // Shadowed.
		if(lineIntersection(p0, p1, line.a.xy, line.b.xy, &cp)) {
			float cd = len(cp - p0);
			if(cd < collisionDist) {
				collisionDist = cd;
				collisionPoint = cp;
				lineIndex = i;
			}
		}
	}

	*cp = collisionPoint;

	return lineIndex;
}

#if 0
struct WalkMesh {
	XForm form; // Only box for now.
};

struct Pole {
	Vec3 pos;
	bool inside;
	int meshIndex;

	int* blockers;
	int blockerCount;
};

enum BlockerType {
	BLOCKERTYPE_RECT = 0,
	BLOCKERTYPE_CIRCLE,
	BLOCKERTYPE_LINE,
};

struct Blocker {
	int type;

	union {
		XForm form;
		Vec3 linePoints[2];
	};
};

struct WalkCell {
	struct BlockerPointList {
		int blockerIndex;
		Vec3 points[2];
		int pointCount;
	};

	Pole* poles[4]; // bl, tl, tr, br.

	int pointCount;
	Vec3 points[3];

	BlockerPointList* blockerPoints;
	int blockerPointCount;

	Line3* lines;
	int lineCount;
};

struct WalkManifold {
	WalkMesh* meshes;
	int meshCount;

	Blocker* blockers;
	int blockerCount;

	float playerRadius;
	Vec2 zRange; // Temp.
	float legHeight;
	float cellSize;
	int edgeSearchIterations;

	Recti grid;
	Vec2i gridDim;  // Temp.
	Vec2i cellsDim; // Temp.

	Pole* poles;
	int poleCount;

	WalkCell* cells;
	int cellCount;

	//

	Vec3 off; // Temp.

	//

	void init(Vec3 playerPos, Recti grid, float playerRadius, float legHeight, float cellSize, WalkMesh* meshes, int meshCount, Blocker* blockers, int blockerCount);
	void rasterize();
	Vec3 move(Vec2 playerPos, Vec2 newPos);
	bool raycast(Vec3 pos, Vec3 rayDir, Vec3* inter);
	void debugDraw();

	//

	bool lineIntersection(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, Vec2 * i);
	bool gridLineCollision(Vec2 p, Vec2 np, Vec2i* cellCoord, Line3* collisionLine, Vec2* collisionPoint, Line3 lastLine);

	Vec2i getWalkCell(Vec2 pos);
	float calcWalkCellZ(Vec2 pos, WalkCell* cell);
	Vec3 calcWalkCellZPos(Vec2 pos, WalkCell* cell);
	Vec3 calcWalkCellPos(Vec2 pos);
	bool poleMeshIntersection(Vec2 pos, WalkMesh* mesh, float* z);
	bool poleBlockerIntersection(Vec3 pos, Blocker* blocker);
	Vec3 calcWalkEdgePoint(Vec3 a, Vec3 b, WalkMesh* mesh, bool checkZ);
	Vec3 calcWalkEdgePoint(Pole* p1, Pole* p2);
	Vec3 calcWalkEdgePointBlocker(Vec3 a, Vec3 b, Blocker* blocker);
	void addWalkEdgePoint(Pole* p1, Pole* p2, WalkCell* cell, Vec3 edgePoint);
	void addWalkEdgePointBlocker(WalkCell* cell, Vec3 edgePoint, int blockerIndex, bool pushSecond);
	int lineCollision(Vec2 p0, Vec2 p1, Line3* lines, int lineCount, Line3 lastLine, Vec2* cp);
};

bool WalkManifold::lineIntersection(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, Vec2 * i) {

	// If line is inside other line. (This might not be sufficient.)
	if(distancePointLine(p2, p3, p0) == 0 && 
	   distancePointLine(p2, p3, p1) == 0) {
		*i = p0;
		return true;
	}

	bool result = getLineIntersection(p0, p1, p2, p3, i);
	return result;
}

bool WalkManifold::gridLineCollision(Vec2 p, Vec2 np, Vec2i* cellCoord, Line3* collisionLine, Vec2* collisionPoint, Line3 lastLine) {

	Vec2i cellDirs[] = { {-1,0}, {1,0}, {0,-1}, {0,1}, };
	int cellEdgeIndexOpposite[] = {1,0,3,2};

	Vec2i newPosCell = getWalkCell(np);

	while(true) {
		WalkCell* cell = &cells[aIndex(cellsDim.w, cellCoord->x-grid.left, cellCoord->y-grid.bottom)];
		if(cell->lineCount) {

			Vec2 cp;
			int lineIndex = lineCollision(p, np, cell->lines, cell->lineCount, lastLine, &cp);
			if(lineIndex != -1) {
				*collisionLine = cell->lines[lineIndex];
				*collisionPoint = cp;

				return true;
			}
		}

		// Get cell intersection and move to neighbour cell.
		{
			Rect cellRect = rect(vec2(*cellCoord)*cellSize, vec2(*cellCoord+1)*cellSize);

			Vec2 dir = np - p;

			if(dir.x == 0 && dir.y == 0) {
				break;
			}

			Vec2 dirNorm = norm(dir);

			Vec2 dirFrac = 1.0f / dirNorm;
			float t[] = {
				(cellRect.left   - p.x) * dirFrac.x,
				(cellRect.right  - p.x) * dirFrac.x,
				(cellRect.bottom - p.y) * dirFrac.y,
				(cellRect.top    - p.y) * dirFrac.y,
			};

			int i = 0;
			int indices[2];

			if(dir.x < 0) indices[i++] = 0;
			if(dir.x > 0) indices[i++] = 1;
			if(dir.y < 0) indices[i++] = 2;
			if(dir.y > 0) indices[i++] = 3;

			int index = -1;

			float dist;
			if(i == 1) {
				dist = t[indices[0]];
				index = indices[0];

			} else if(i == 2) {
				if(t[indices[0]] < t[indices[1]]) {
					dist = t[indices[0]];
					index = indices[0];
				} else {
					dist = t[indices[1]];
					index = indices[1];
				}
			}

			if(dist < len(dir)) {
				p += dirNorm * dist;
				*cellCoord += cellDirs[index];

				continue;
			}
		}

		break;
	}

	assert(newPosCell == *cellCoord);

	return false;
}

void WalkManifold::init(Vec3 playerPos, Recti grid, float playerRadius, float legHeight, float cellSize, WalkMesh* meshes, int meshCount, Blocker* blockers, int blockerCount) {

	*this = {};

	this->grid = grid;
	this->gridDim = vec2i(grid.w() + 1, grid.h() + 1);
	this->cellsDim = vec2i(grid.w(), grid.h());

	this->playerRadius = playerRadius;
	this->zRange = vec2(playerPos.z - legHeight, playerPos.z + legHeight);
	this->legHeight = legHeight;
	this->cellSize = cellSize;

	// this->edgeSearchIterations = 16;
	this->edgeSearchIterations = 10;

	this->meshes = meshes;
	this->meshCount = meshCount;
	this->blockers = blockers;
	this->blockerCount = blockerCount;

	poleCount = gridDim.w * gridDim.h;
	poles = getTArray(Pole, poleCount);

	cellCount = cellsDim.w * cellsDim.h;
	cells = getTArray(WalkCell, cellCount);
	memset(cells, 0, sizeof(WalkCell)*cellCount);

	off = vec3(0,0,0.001f);
}

void WalkManifold::rasterize() {

	// Set poles for walk cells.
	for(int y = 0; y < cellsDim.h; y++) {
		for(int x = 0; x < cellsDim.w; x++) {

			WalkCell* cell = &cells[aIndex(cellsDim.w, x, y)];

			Pole* ps[] = {
				&poles[aIndex(gridDim.w, x, y)],
				&poles[aIndex(gridDim.w, x, y+1)],
				&poles[aIndex(gridDim.w, x+1, y+1)],
				&poles[aIndex(gridDim.w, x+1, y)],
			};

			for(int i = 0; i < 4; i++) cell->poles[i] = ps[i];
		}
	}

	// Sample poles.
	for(int y = 0; y < gridDim.h; y++) {
		for(int x = 0; x < gridDim.w; x++) {

			Pole* pole = &poles[aIndex(gridDim.w, x, y)];

			Vec2 pos = vec2((grid.left + x)*cellSize, (grid.bottom + y)*cellSize);
			*pole = {vec3(pos, -FLT_MAX), false, -1, 0, 0};

			for(int i = 0; i < meshCount; i++) {
				float sample;
				if(poleMeshIntersection(pos, meshes + i, &sample)) {
					if(sample >= zRange.x && sample <= zRange.y) {
						pole->inside = true;
					}
					pole->pos.z = max(sample, pole->pos.z);
					pole->meshIndex = i;
				}
			}
		}
	}

	// Calc cells.
	for(int y = 0; y < gridDim.h; y++) {
		for(int x = 0; x < gridDim.w; x++) {

			Pole* pole = &poles[aIndex(gridDim.w, x, y)];

			for(int stage = 0; stage < 2; stage++) {

				Pole* pole2 = 0;
				if(stage == 0 && x < gridDim.w-1) pole2 = &poles[aIndex(gridDim.w, x+1, y)];
				if(stage == 1 && y < gridDim.h-1) pole2 = &poles[aIndex(gridDim.w, x, y+1)];

				if(!pole2) continue;

				WalkCell* cell = 0;
				if((x < gridDim.w-1) && y < gridDim.h-1) cell = &cells[aIndex(cellsDim.w, x, y)];

				WalkCell* cell2 = 0;
				if      (stage == 0 && (y > 0 && x < gridDim.w-1))  cell2 = &cells[aIndex(cellsDim.w, x, y-1)];
				else if (stage == 1 && (x > 0 && y < gridDim.h-1)) cell2 = &cells[aIndex(cellsDim.w, x-1, y)];

				if(pole->inside != pole2->inside) {

					Pole* p1 = pole->inside ? pole : pole2; 
					Pole* p2 = pole->inside ? pole2 : pole;

					Vec3 edgePoint = calcWalkEdgePoint(p1, p2);

					if(cell) {
						addWalkEdgePoint(p1, p2, cell, edgePoint);

						// dxDrawLine(p1->pos+off, edgePoint+off, vec4(1,1));
				   }

					if(cell2) {
						addWalkEdgePoint(p1, p2, cell2, edgePoint);
					}

				} else {
					// if(pole->inside && pole2->inside)
						// dxDrawLine(pole->pos+off, pole2->pos+off, vec4(1,1));
				}
			}
		}
	}

	DArray<Blocker> allBlockers = dArray<Blocker>(getTMemory);

	for(int i = 0; i < blockerCount; i++) {
		allBlockers.push(blockers[i]);
	}

	// Collect walk edges and add them as line blockers.
	for(int i = 0; i < cellCount; i++) {
		WalkCell* cell = cells + i;

		if(cell->pointCount > 1) {
			if(cell->pointCount == 2) {
				Blocker b = {BLOCKERTYPE_LINE};
				b.linePoints[0] = cell->points[0];
				b.linePoints[1] = cell->points[1];

				allBlockers.push(b);
			} 
			// else if(cell->pointCount == 3) {
				// allBlockers.push({BLOCKERTYPE_LINE, xForm(), cell->points[0], cell->points[2]});
				// allBlockers.push({BLOCKERTYPE_LINE, xForm(), cell->points[2], cell->points[1]});
			// }
		}
		cell->pointCount = 0;
	}

	// Sample poles for allBlockers.
	for(int y = 0; y < gridDim.h; y++) {
		for(int x = 0; x < gridDim.w; x++) {

			Pole* pole = &poles[aIndex(gridDim.w, x, y)];

			for(int i = 0; i < allBlockers.count; i++) {
				bool isBlocked = poleBlockerIntersection(pole->pos, allBlockers + i);

				if(isBlocked) {
					// if(pole->inside) {
						if(!pole->blockers) pole->blockers = getTArray(int, 10);
						pole->blockers[pole->blockerCount++] = i;
						// poles[i]->inside = !isBlocked;
					// }
				}
			}
		}
	}

	// Calc blockers.
	for(int y = 0; y < gridDim.h; y++) {
		for(int x = 0; x < gridDim.w; x++) {

			Pole* pole = &poles[aIndex(gridDim.w, x, y)];

			for(int stage = 0; stage < 2; stage++) {

				Pole* pole2 = 0;
				if(stage == 0 && x < gridDim.w-1) pole2 = &poles[aIndex(gridDim.w, x+1, y)];
				if(stage == 1 && y < gridDim.h-1) pole2 = &poles[aIndex(gridDim.w, x, y+1)];

				if(!pole2) continue;

				WalkCell* cell = 0;
				if((x < gridDim.w-1) && y < gridDim.h-1) cell = &cells[aIndex(cellsDim.w, x, y)];

				WalkCell* cell2 = 0;
				if      (stage == 0 && (y > 0 && x < gridDim.w-1)) cell2 = &cells[aIndex(cellsDim.w, x, y-1)];
				else if (stage == 1 && (x > 0 && y < gridDim.h-1)) cell2 = &cells[aIndex(cellsDim.w, x-1, y)];

				for(int poleStage = 0; poleStage < 2; poleStage++) {
					Pole* p0 = poleStage == 0 ? pole : pole2;
					Pole* p1 = poleStage == 0 ? pole2 : pole;

					for(int i0 = 0; i0 < p0->blockerCount; i0++) {
						int blockerIndex = p0->blockers[i0];

						bool found = false;
						for(int i1 = 0; i1 < p1->blockerCount; i1++) {
							if(p1->blockers[i1] == blockerIndex) {
								found = true;
								break;
							}
						}

						if(!found) {
							Blocker* blocker = allBlockers + blockerIndex;
							Vec3 edgePoint = calcWalkEdgePointBlocker(p1->pos, p0->pos, blocker);

							bool pushSecond;
							if(stage == 0) pushSecond = poleStage == 1 ? false : true;
							else           pushSecond = poleStage == 1 ? true : false;

							if(cell) {
								addWalkEdgePointBlocker(cell, edgePoint, blockerIndex, pushSecond);
						   }

							if(cell2) {
								addWalkEdgePointBlocker(cell2, edgePoint, blockerIndex, !pushSecond);
							}
						}
					}
				}
			}
		}
	}
}

Vec3 WalkManifold::move(Vec2 playerPos, Vec2 newPos) {

	float offset;
	{
		float maxVal = max(max(playerPos.x, playerPos.y), 
		                   max(newPos.x, newPos.y));
		float next = nextafter(maxVal, FLT_MAX);
		offset = (next - maxVal)*10;

		offset = max(offset, 0.000001f);
	}

	Vec2 p = playerPos;
	Vec2 np = newPos;

	Vec2i cellCoord = getWalkCell(p);

	Line3 lastLine = {vec3(FLT_MAX), vec3(FLT_MAX)};

	Vec2 projectDir;

	bool firstLineHit = false;
	int lineHitCount = 0;

	bool printOut = false;

	bool debugDraw = true;
	int index = 0;

	Vec2 lastSavePos = p;

	while(true) {

		Line3 line;
		Vec2 cp;
		bool collision = gridLineCollision(p, np, &cellCoord, &line, &cp, lastLine);

		if(collision) {

			if(debugDraw) {
				dxDrawLine(line.a+off, line.b+off, vec4(1,0,0,1));
			}

			index++;

			lastLine = line;

			// Project onto line.
			{
				if(!firstLineHit) {
					projectDir = np - cp;
					firstLineHit = true;
				}

				Vec2 a = norm(line.a.xy - line.b.xy);
				Vec2 b = projectDir;
				Vec2 pp = cp + a * dot(a, b);

				projectDir = norm(projectDir) * len(cp - pp);

				Vec2 normal;
				{
					normal = rotateRight(a);
					// if(dot(normal, line.a.xy - p) < 0) {
					// 	normal = rotateLeft(a);
					// }

					if(debugDraw) {
						Vec3 mid = (line.a + line.b) / 2.0f;
						Vec3 ln = -norm(vec3(normal, 0));

						dxDrawLine(mid + off, 
						           mid + ln*0.05f + off, vec4(0,1,1,1));
					}
				}

				Vec2 op  = cp - normal * offset;
				Vec2 onp = pp - normal * offset;

				if(debugDraw) {
					dxDrawLine(calcWalkCellPos(op), 
					           calcWalkCellPos(op) + vec3(0,0,0.05f), vec4(1,0.5f,0,1));

					dxDrawLine(calcWalkCellPos(onp), 
					           calcWalkCellPos(onp) + vec3(0,0,0.05f), vec4(1,0,0.5f,1));
				}

				// Check if we can do the offset without a collision.
				{
					Line3 testLine;
					Vec2 ncp;
					bool collision = gridLineCollision(cp, op, &cellCoord, &testLine, &ncp, lastLine);

					if(collision) {
						// Can't do the offset so we try to compromise by taking 
						// the intersection with the perpendicular offset line.
						// And if that failes as well we just bail out and don't move.

						Line3 ol = { vec3(line.a.xy - normal * offset,0), 
							         vec3(line.b.xy - normal * offset,0) };
						Vec2 ep;
						if(lineIntersection(p, cp, ol.a.xy, ol.b.xy, &ep)) {
							p = ep;
						} else {
							p = lastSavePos;
						}

						break;
					}
				}

				p = op;
				np = onp;
			}

			lastSavePos = p;

			if(lineHitCount == 5) break;
			lineHitCount++;

			continue;

		} else {
			p = np;
			break;
		}
	}

	Vec3 cellPos = calcWalkCellPos(p);
	return cellPos;
}

bool WalkManifold::raycast(Vec3 pos, Vec3 rayDir, Vec3* inter) {
	bool foundIntersection = false;
	Vec3 intersection = vec3(0,0,0);
	float minDist = FLT_MAX;
	for(int i = 0; i < cellCount; i++) {
		WalkCell* cell = cells + i;

		Vec3 p[] = {cell->poles[0]->pos, 
						cell->poles[1]->pos, 
						cell->poles[2]->pos, 
						cell->poles[3]->pos };

		bool skipCell = false;
		for(int j = 0; j < 4; j++) {
			if(p[j].z < 0) {
				skipCell = true;
				break;
			}
		}
		if(skipCell) continue;

		Vec3 inter;
		bool result = lineQuadIntersection(pos, rayDir, p[0], p[1], p[2], p[3], &inter, true);
		if(result) {
			float dist = len(inter - pos);
			if(dist < minDist) {
				minDist = dist;
				intersection = inter;
			}

			foundIntersection = true;
		}
	}

	if(!foundIntersection) return false;

	*inter = intersection;
	return true;
}

void WalkManifold::debugDraw() {

	// Draw blockers.
	for(int y = 0; y < gridDim.h; y++) {
		for(int x = 0; x < gridDim.w; x++) {

			Pole* pole = &poles[aIndex(gridDim.w, x, y)];

			for(int stage = 0; stage < 2; stage++) {

				Pole* pole2 = 0;
				if(stage == 0 && x < gridDim.w-1)  pole2 = &poles[aIndex(gridDim.w, x+1, y)];
				if(stage == 1 && y < gridDim.h-1) pole2 = &poles[aIndex(gridDim.w, x, y+1)];

				if(!pole2) continue;

				WalkCell* cell = 0;
				if((x < gridDim.w-1) && y < gridDim.h-1) cell = &cells[aIndex(cellsDim.w, x, y)];

				// WalkCell* cell2 = 0;
				// if      (stage == 0 && (y > 0 && x < gridDim.w-1))  cell2 = &cells[aIndex(cellsDim.w, x, y-1)];
				// else if (stage == 1 && (x > 0 && y < gridDim.h-1)) cell2 = &cells[aIndex(cellsDim.w, x-1, y)];

				// if(pole->inside && pole->blockerCount == 0) {
				// 	dxDrawLine(pole->pos, pole->pos+vec3(0,0,0.2f), vec4(0,1,1,1));
				// } else {
				// 	dxDrawLine(pole->pos, pole->pos+vec3(0,0,0.1f), vec4(0,1,1,1));
				// }

				bool in0 = pole->inside && pole->blockerCount == 0;
				bool in1 = pole2->inside && pole2->blockerCount == 0;

				if(in0 || in1) {
					Pole* p0 = pole;
					Pole* p1 = pole2;

					if(in0 && in1) {
						dxDrawLine(p0->pos+off, p1->pos+off, vec4(1,1,1,1));
					} else {
						if(in1) swap(&p0, &p1);

						if(cell) {
							Vec2 edgePos;
							float dist = FLT_MAX;
							for(int i = 0; i < cell->lineCount; i++) {
								Line3 line = cell->lines[i];
								Vec2 cp;
								bool result = getLineIntersection(p0->pos.xy, p1->pos.xy, line.a.xy, line.b.xy, &cp);	
								if(result) {
									float cDist = len(p0->pos.xy - cp);
									if(cDist < dist) {
										dist = cDist;
										edgePos = cp;
									}
								}
							}

							// dxDrawLine(p0->pos+off, (p1->pos + p0->pos)/2.0f+off, vec4(1,1,1,1));
							dxDrawLine(p0->pos+off, (vec3(edgePos, p1->pos.z))+off, vec4(1,1,1,1));
						}
					}
				}
			}

		}
	}

	// Draw Lines.
	for(int i = 0; i < cellCount; i++) {
		WalkCell* cell = cells + i;

		Vec4 c = vec4(0,1,1,1);
		// if(cell->pointCount == 2) {
		// 	dxDrawLine(cell->points[0]+off, cell->points[1]+off, c);
		// }

		// if(cell->pointCount == 3) {
		// 	dxDrawLine(cell->points[0]+off, cell->points[2]+off, c);
		// 	dxDrawLine(cell->points[2]+off, cell->points[1]+off, c);

		// 	// dxDrawLine(cell->points[0]+off, cell->points[2]+off, c);
		// 	// dxDrawLine(cell->points[2]+off, cell->points[1]+off, c);
		// }

		for(int i = 0; i < cell->lineCount; i++) {
			Line3 line = cell->lines[i];
			if(line.a.z > -1000000 && line.b.z > -1000000)
				dxDrawLine(line.a+off, line.b+off, c);

			// printf("%f %f\n", line.a.z, -FLT_MAX);
		}
	}

	// Draw Blockers.
	if(false)
	{
		dxDepthTest(true);
		// dxFillWireFrame(true);
		for(int i = 0; i < blockerCount; i++) {
			XForm b = blockers[i].form;
			// dxDrawCube(b.trans, b.scale, vec4(1,0,0,0.5f));
			dxDrawCube(b.trans, b.scale, vec4(0.7,0,0,1));
		}
		// dxFillWireFrame(false);
	}
}

//

Vec2i WalkManifold::getWalkCell(Vec2 pos) {
	Vec2 cellF = pos / cellSize;
	Vec2i cellCoord = vec2i(floor(cellF.x), floor(cellF.y));

	return cellCoord;
}

float WalkManifold::calcWalkCellZ(Vec2 pos, WalkCell* cell) {

	Pole** ps = cell->poles;
	Vec3 coords = barycentricCoordinates(pos, ps[0]->pos.xy, ps[1]->pos.xy, ps[2]->pos.xy);

	// If not in first triangle choose the second triangle of quad.
	if(coords.x < 0 || coords.y < 0 || coords.z < 0) {
		coords = barycentricCoordinates(pos, ps[2]->pos.xy, ps[3]->pos.xy, ps[0]->pos.xy);
	}

	float z = coords.x * ps[0]->pos.z + 
	          coords.y * ps[1]->pos.z + 
	          coords.z * ps[2]->pos.z;

	return z;
}

Vec3 WalkManifold::calcWalkCellZPos(Vec2 pos, WalkCell* cell) {

	float z = calcWalkCellZ(pos, cell);
	return vec3(pos, z);
}

Vec3 WalkManifold::calcWalkCellPos(Vec2 pos) {
	Vec2i cellCoord = getWalkCell(pos);
	WalkCell* cell = &cells[aIndex(cellsDim.w, cellCoord.x-grid.left, cellCoord.y-grid.bottom)];

	Vec3 wp = calcWalkCellZPos(pos, cell);

	return wp;
}

bool WalkManifold::poleMeshIntersection(Vec2 pos, WalkMesh* mesh, float* z) {
	XForm b = mesh->form;

	Vec3 rayPos = vec3(pos, 100);
	Vec3 rayDir = vec3(0,0,-1);
	Quat rotInverse = inverse(b.rot);

	Vec3 lp, ld;
	lp = rayPos - b.trans;
	ld = lp + rayDir;
	lp = rotInverse * lp;
	ld = rotInverse * ld;
	ld = norm(ld - lp);

	Vec3 position, normal;
	float distance = lineBoxIntersection(lp, ld, vec3(0.0f), b.scale, &position, &normal);

	if(distance != -1) {
		position = b.rot * position;
		position = position + b.trans;

		*z = position.z;
		return true;

	} else {
		return false;
	}
}

bool gjk(Blocker* s1, Vec2 oldPos, Blocker* s2);
bool WalkManifold::poleBlockerIntersection(Vec3 pos, Blocker* blocker) {
	XForm b = blocker->form;

	Vec2 blockerZRange;
	if(blocker->type == BLOCKERTYPE_LINE) {
		blockerZRange.min = min(blocker->linePoints[0].z, blocker->linePoints[1].z);
		blockerZRange.max = max(blocker->linePoints[0].z, blocker->linePoints[1].z);
	} else {
		blockerZRange = vec2(b.trans.z - b.scale.z/2.0f, 
                    b.trans.z + b.scale.z/2.0f);
	}

	if(!(pos.z >= blockerZRange.min && pos.z <= blockerZRange.max)) return false;

	Blocker srcBlocker = {BLOCKERTYPE_CIRCLE, xForm(pos, vec3(playerRadius))};
	bool result = gjk(&srcBlocker, vec2(0,0), blocker);

	return result;
}

Vec3 WalkManifold::calcWalkEdgePoint(Vec3 a, Vec3 b, WalkMesh* mesh, bool checkZ) {
	Vec3 c;
	float z = a.z;
	for(int i = 0; i < edgeSearchIterations; i++) {
		c = (a+b)/2.0f;

		bool hit = poleMeshIntersection(c.xy, mesh, &z);
		if(checkZ) {
			if(!(z >= zRange.x && z <= zRange.y)) hit = false;
		}

		if(hit) a = c;
		else b = c;
	}

	return vec3(c.xy, z);
}

Vec3 WalkManifold::calcWalkEdgePoint(Pole* p1, Pole* p2) {
	return calcWalkEdgePoint(p1->pos, p2->pos, meshes + p1->meshIndex, p2->meshIndex != -1);
}

Vec3 WalkManifold::calcWalkEdgePointBlocker(Vec3 a, Vec3 b, Blocker* blocker) {

	Vec3 c;
	for(int i = 0; i < edgeSearchIterations; i++) {
		c = (a+b)/2.0f;

		bool hit = !poleBlockerIntersection(c, blocker);

		if(hit) a = c;
		else b = c;
	}

	return c;
}

void WalkManifold::addWalkEdgePoint(Pole* p1, Pole* p2, WalkCell* cell, Vec3 edgePoint) {

	if(cell->pointCount == 0) {
		// cell->tempPoles[0] = p1;
		// cell->tempPoles[1] = p2;
		cell->points[cell->pointCount++] = edgePoint;

		return;
	}

	if(cell->pointCount == 1) {
		cell->points[cell->pointCount++] = edgePoint;

		return;
	}

	if(cell->pointCount > 1) {
		__debugbreak();
		return;
	}

	#if 0

	Pole* poles[4] = {cell->tempPoles[0], cell->tempPoles[1], p1, p2};

	// Calc third point.
	{
		Vec3 testEp[2];

		for(int i = 0; i < 2; i++) {
			Pole pole0 = i == 0 ? *cell->tempPoles[0] : *p1;
			Pole pole1 = i == 0 ? *cell->tempPoles[1] : *p2;

			float percent = 0.1f;
			Vec3 offset = {};
			if(pole0.pos.x != pole1.pos.x)
				offset.y = percent * (cell->points[(i+1)%2].y - cell->points[i].y);
			else 
				offset.x = percent * (cell->points[(i+1)%2].x - cell->points[i].x);

			pole0.pos += offset;
			pole1.pos += offset;

			testEp[i] = calcWalkEdgePoint(&pole0, &pole1, meshes, zRange);

			// dxDrawLine(pole0->pos, pole0->pos+vec3(0,0,0.2f), vec4(1,1,0,1));
			// dxDrawLine(pole1->pos, pole1->pos+vec3(0,0,0.2f), vec4(1,0.5f,0,1));

			// dxDrawLine(testEp[0], testEp[0]+vec3(0,0,0.2f), vec4(1,0.5f,0,1));
			// dxDrawLine(testEp[1], testEp[1]+vec3(0,0,0.2f), vec4(1,0.5f,0,1));

			// dxDrawLine(pole0->pos + offset, pole0->pos + offset+vec3(0,0,0.2f), vec4(1,0.0f,0,1));
			// dxDrawLine(pole1->pos + offset, pole1->pos + offset+vec3(0,0,0.2f), vec4(1,0.5f,0,1));
		}

		Vec3 dir0 = norm(testEp[0] - cell->points[0]);
		Vec3 dir1 = norm(testEp[1] - cell->points[1]);

		Vec3 thirdPoint = lineClosestPoint(cell->points[0], dir0, cell->points[1], dir1);

		// Something went wrong.
		if(!pointInRect(thirdPoint.xy, cellRect)) {
			thirdPoint = (cell->points[0] + cell->points[1])/2.0f;
		}

		cell->points[cell->pointCount++] = thirdPoint;
	}

	#endif
}

void WalkManifold::addWalkEdgePointBlocker(WalkCell* cell, Vec3 edgePoint, int blockerIndex, bool pushSecond) {

	if(!cell->blockerPoints) {
		cell->blockerPoints = getTArray(WalkCell::BlockerPointList, 10);
		cell->blockerPointCount = 0;
	}

	WalkCell::BlockerPointList* list = 0;
	for(int i = 0; i < cell->blockerPointCount; i++) {
		if(cell->blockerPoints[i].blockerIndex == blockerIndex) {
			list = &cell->blockerPoints[i];
			break;
		}
	}

	if(!list) {
		list = &cell->blockerPoints[cell->blockerPointCount++];
		list->pointCount = 0;
		list->blockerIndex = blockerIndex;
	}

	list->points[list->pointCount++] = edgePoint;

	if(list->pointCount == 2) {
		if(!cell->lines) cell->lines = getTArray(Line3, 10);

		// Make sure to push lines in clockwise order from blocker perspective.
		Line3 line;
		if(!pushSecond) line = { list->points[0], list->points[1] };
		else            line = { list->points[1], list->points[0] };

		cell->lines[cell->lineCount++] = line;
	}

	if(list->pointCount > 2) {
		__debugbreak();
		return;
	}
}

int WalkManifold::lineCollision(Vec2 p0, Vec2 p1, Line3* lines, int lineCount, Line3 lastLine, Vec2* cp) {
	int lineIndex = -1;
	Vec2 collisionPoint = {};
	float collisionDist = FLT_MAX;
	for(int i = 0; i < lineCount; i++) {
		Line3 line = lines[i];

		if(lastLine.a == line.a && lastLine.b == line.b) continue;

		Vec2 cp; // Shadowed.
		if(lineIntersection(p0, p1, line.a.xy, line.b.xy, &cp)) {
			float cd = len(cp - p0);
			if(cd < collisionDist) {
				collisionDist = cd;
				collisionPoint = cp;
				lineIndex = i;
			}
		}
	}

	*cp = collisionPoint;

	return lineIndex;
}


#endif