
void WalkManifold::init(WalkManifoldSettings* settings) {
	*this = {};
	this->settings = *settings;
}

void WalkManifold::setup(Vec3 playerPos, int gridRadius, WalkMesh* meshes, int meshCount, Blocker* blockers, int blockerCount) {
	Vec2i gridPos;
	gridPos.x = playerPos.x / settings.cellSize;
	gridPos.y = playerPos.y / settings.cellSize;
	this->grid = recti(gridPos - gridRadius, gridPos + gridRadius);

	this->gridDim = vec2i(grid.w() + 1, grid.h() + 1);
	this->cellsDim = vec2i(grid.w(), grid.h());

	this->playerZ = playerPos.z;
	this->zRange = vec2(playerPos.z - settings.legHeight, playerPos.z + settings.legHeight);
	this->zHeightIgnore = playerPos.z + settings.playerHeight;

	Vec2i edgePoles[4] = {{0,3}, {0,1}, {1,2}, {3,2}};
	copyArray(this->edgePoles, edgePoles, Vec2i, arrayCount(edgePoles));

	this->meshes = meshes;
	this->meshCount = meshCount;
	this->blockers = blockers;
	this->blockerCount = blockerCount;

	int poleCount = gridDim.w * gridDim.h;
	poles = dArray<Pole>(getTMemory);
	poles.retrieve(poleCount);
	poles.zeroMemory();

	int cellCount = cellsDim.w * cellsDim.h;
	cells = dArray<WalkCell>(getTMemory);
	cells.retrieve(cellCount);
	cells.zeroMemory();

	allBlockers = dArray<Blocker>(getTMemory);

	for(int i = 0; i < cellCount; i++) {
		cells[i].mutex = CreateMutex(0, false, 0);
	}

	WalkManifoldSettings& s = settings;
	// Make sure player radius is at least cell square diagonal.
	// Should bring the camera near plane into this calculation. 
	s.playerRadius = max(s.playerRadius, s.cellSize*1.45f);
	s.playerHeight = max(s.playerHeight, s.legHeight);
}

void WalkManifold::rasterize() {
	TIMER_BLOCK();

	// Set poles on cells.
	{
		TIMER_BLOCK_NAMED("SetCellPoles");

		for(int y = 0; y < cellsDim.h; y++) {
			for(int x = 0; x < cellsDim.w; x++) {
				WalkCell* cell = getCell(x, y);
				cell->coord = vec2i(x,y) + grid.min;
				Pole* ps[] = { getPole(x,y), getPole(x,y+1), getPole(x+1,y+1), getPole(x+1,y) };
				for(int i = 0; i < 4; i++) cell->poles[i] = ps[i];
			}
		}
	}

	// Get potential walk meshes for pole intersections.
	{
		TIMER_BLOCK_NAMED("GatherPoleMeshes");

		for(int i = 0; i < meshCount; i++) {
			WalkMesh* mesh = meshes + i;
			Vec2 mi = mesh->aabb.min.xy / settings.cellSize;
			Vec2 ma = mesh->aabb.max.xy / settings.cellSize;

			Recti pr = recti(ceil(mi.x), ceil(mi.y), floor(ma.x), floor(ma.y));
			if(!rectGetIntersection(&pr, grid, pr)) continue;

			for(int y = pr.bottom; y <= pr.top; y++) {
				for(int x = pr.left; x <= pr.right; x++) {
					Pole* pole = getPole(x - grid.left,y - grid.bottom);
					if(!pole->potentialMeshes.count) pole->potentialMeshes = dArray<int>(5, getTMemory);
					pole->potentialMeshes.push(i);
				}
			}
		}
	}

	// Sample walk meshes for poles.
	{
		TIMER_BLOCK_NAMED("WalkPoles");

		auto threadFunc = [](void* data) {
			TIMER_BLOCK();
			ThreadHeader* h = (ThreadHeader*)data;
			WalkManifold* wm = (WalkManifold*)h->data;

			for(int i = 0; i < h->count; i++) {
				int x, y;
				aCoord(wm->gridDim.w, h->index + i, &x, &y);
				Pole* pole = wm->getPole(x,y);				

				pole->pos = vec2((wm->grid.left + x)*wm->settings.cellSize, (wm->grid.bottom + y)*wm->settings.cellSize);;
				pole->samples = dArray<MeshSample>(5, getTMemoryInterlocked);
				DArray<MeshSample>& samples = pole->samples;

				for(auto meshIndex : pole->potentialMeshes) {
					float sample;
					if(wm->poleMeshIntersection(pole->pos, wm->meshes + meshIndex, &sample)) 
						samples.push({sample, meshIndex});
				}

				// Sort samples.
				auto cmp = [](const void* a, const void* b) -> int { 
					return ((MeshSample*)a)->z < ((MeshSample*)b)->z ? -1 : 1;
				};
				qsort(samples.data, samples.count, sizeof(MeshSample), cmp);

				// Reject samples that don't fit the player.
				for(int i = 0; i < samples.count-1; i++) {
					MeshSample* s0 = samples.data + i;
					MeshSample* s1 = samples.data + i+1;

					if(s1->z - s0->z < wm->settings.playerHeight) s0->z = -FLT_MAX;
				}

				for(int i = 0; i < samples.count; i++) {
					if(samples[i].z == -FLT_MAX) {
						samples.remove(i);
						i--;
					}
				}
			}
		};

		splitThreadTask(poles.count, this, threadFunc);
	}

	{
		TIMER_BLOCK_NAMED("MarkWalkPoles");

		for(auto& cell : cells) {
			for(auto& layer : cell.layers) {
				if(!layer.poles[0].valid && !layer.poles[1].valid &&
				   !layer.poles[2].valid && !layer.poles[3].valid) {
					layer.outside = true;
				}
			}
		}
	}

	{
		TIMER_BLOCK_NAMED("CreateLayers");

		// I have no idea if this threaded version actually works. But it seems to.
		auto threadFunc = [](void* data) {
			TIMER_BLOCK();
			ThreadHeader* h = (ThreadHeader*)data;
			WalkManifold* wm = (WalkManifold*)h->data;

			for(int i = 0; i < h->count; i++) {
				int x, y;
				aCoord(wm->cellsDim.w, h->index + i, &x, &y);
				WalkCell& cell = *wm->getCell(x,y);

				cell.layers = dArray<WalkLayer>(3, getTMemoryInterlocked);

				int maxSampleCount = 0;
				int startPoleIndex = 0;
				for(int i = 0; i < arrayCount(cell.poles); i++) {
					Pole* pole = cell.poles[i];
					if(pole->samples.count > maxSampleCount) {
						maxSampleCount = pole->samples.count;
						startPoleIndex = i;
					}
				}

				for(int i = 0; i < 4; i++) {
					int poleIndex = (i + startPoleIndex) % 4;
					Pole* pole = cell.poles[poleIndex];

					for(int sampleIndex = 0; sampleIndex < pole->samples.count; sampleIndex++) {
						MeshSample& sample = pole->samples[sampleIndex];

						WalkLayer* layer = 0;
						float minDist = FLT_MAX;
						for(auto& la : cell.layers) {
							if(la.poles[poleIndex].valid) continue;

							float dist = la.zRange.max < sample.z ? sample.z - la.zRange.max : 
							            (la.zRange.min > sample.z ? la.zRange.min - sample.z : 0);

							// If next sample fits better, skip.
							if(sampleIndex+1 < pole->samples.count) {
								MeshSample& sample2 = pole->samples[sampleIndex+1];
								float dist2 = la.zRange.max < sample2.z ? sample2.z - la.zRange.max : 
								             (la.zRange.min > sample2.z ? la.zRange.min - sample2.z : 0);
								if(dist2 < dist) continue;
							}

							if(dist < minDist && dist < wm->settings.playerHeight) {
								minDist = dist;
								layer = &la;
								continue;
							}
						}

						if(!layer) {
							WalkLayer la = {};
							for(int i = 0; i < 4; i++) la.poles[i].pos.xy = cell.poles[i]->pos;
							la.poles[poleIndex].valid = true;
							la.poles[poleIndex].pos.z = sample.z;
							la.poles[poleIndex].sample = &sample;

							la.zRange = vec2(sample.z);
							la.cell = &cell;
							cell.layers.push(la);

						} else {
							layer->poles[poleIndex] = {true, vec3(pole->pos, sample.z), &sample};
							layer->zRange.min = min(layer->zRange.min, sample.z);
							layer->zRange.max = max(layer->zRange.max, sample.z);
						}
					}
				}

				auto cmp = [](const void* a, const void* b) -> int { 
					Vec2 zr0 = ((WalkLayer*)a)->zRange;
					Vec2 zr1 = ((WalkLayer*)b)->zRange;
					return ((zr0.min + zr0.max)/2.0f) < ((zr1.min + zr1.max)/2.0f) ? -1 : 1;
				};
				qsort(cell.layers.data, cell.layers.count, sizeof(WalkLayer), cmp);
			}
		};

		splitThreadTask(cells.count, this, threadFunc);
	}

	// Calc walk edges.
	{
		TIMER_BLOCK_NAMED("WalkEdges");

		auto threadFunc = [](void* data) {
			TIMER_BLOCK();
			ThreadHeader* h = (ThreadHeader*)data;
			WalkManifold* wm = (WalkManifold*)h->data;

			// Maybe these are not necessary anymore.
			DArray<WalkLayer*> pushedLayers = dArray<WalkLayer*>(3, getTMemoryInterlocked);

			for(int i = 0; i < h->count; i++) {
				int x, y;
				aCoord(wm->cellsDim.w, h->index + i, &x, &y);
				WalkCell* cell = wm->getCell(x,y);

				pushedLayers.clear();

				for(int layerIndex = cell->layers.count-1; layerIndex >= 0; layerIndex--) {
					WalkLayer* layer = cell->layers + layerIndex;

					for(int edgeIndex = 0; edgeIndex < 4; edgeIndex++) {
						Vec2i is = wm->edgePoles[edgeIndex];
						LayerPole* p[2] = {layer->poles + is.e[0], layer->poles + is.e[1]};

						if(!p[0]->valid && !p[1]->valid) continue;
						if( p[0]->valid &&  p[1]->valid && fabs(p[0]->pos.z - p[1]->pos.z) <= wm->settings.legHeight) continue;

						// Check if we have adjacent layers on top or right that will take care of those
						// walk edge points or if we have to calculate them ourselves.
						if(edgeIndex > 1 && wm->adjacentLayerSharesEdge(layer, vec2i(x,y), edgeIndex)) continue;

						// Probably a bad way to handle this:
						// mode  1||2  -> selects direction to go after player z height test
						// mode <0||>0 -> selected what the pole direction is.
						int playerHeightMode = 0;
						for(int i = 0; i < 2; i++) {
							if(p[i]->valid) continue;
							if(!(layerIndex < cell->layers.count-1)) continue;

							WalkLayer* aboveLayer = cell->layers + layerIndex+1;
							LayerPole* abovePole = aboveLayer->poles + is.e[i];

							if(abovePole->valid) {
								p[i] = abovePole;
								playerHeightMode = (fabs(p[(i+1)%2]->pos.z - p[i]->pos.z) < wm->settings.playerHeight) ? 1 : 2;
								if(i == 0) playerHeightMode *= -1;
							}
						}

						Vec3 edgePoint = wm->calcWalkEdgePoint(p[0], p[1], playerHeightMode);
						PointInfo pi = {p[0], p[1], edgeIndex, playerHeightMode};

						wm->addWalkEdgePoint(layer, edgePoint, pi);

						if(edgeIndex < 2) {
							WalkLayer* layer2;
							if(wm->adjacentLayerSharesEdge(layer, vec2i(x,y), edgeIndex, &layer2)) {
								if(!pushedLayers.find(layer2)) {
									pi.edgeIndex = (pi.edgeIndex+2)%4;
									wm->addWalkEdgePoint(layer2, edgePoint, pi);
									pushedLayers.push(layer2);
								}
							}
						}
					}
				}
			}
		};

		// Threading bug here. So we disable threading for now.
		splitThreadTask(cells.count, this, threadFunc, -1);
	}

	{
		for(int i = 0; i < blockerCount; i++) allBlockers.push(blockers[i]);

		// Add walk edges as line blockers.
		for(auto& cell : cells) {
			for(auto& layer : cell.layers) {
				if(layer.pointCount) {
					if(layer.pointCount == 1) {
						Blocker b = {BLOCKERTYPE_LINE};
						b.linePoints[0] = layer.points[0];
						b.linePoints[1] = layer.points[0];
						allBlockers.push(b);

					} else if(layer.pointCount == 2) {
						Blocker b = {BLOCKERTYPE_LINE};
						b.linePoints[0] = layer.points[0];
						b.linePoints[1] = layer.points[1];
						allBlockers.push(b);

					} else if(layer.pointCount == 3) {
						{
							Blocker b = {BLOCKERTYPE_LINE};
							b.linePoints[0] = layer.points[0];
							b.linePoints[1] = layer.points[2];
							allBlockers.push(b);
						}
						{
							Blocker b = {BLOCKERTYPE_LINE};
							b.linePoints[0] = layer.points[2];
							b.linePoints[1] = layer.points[1];
							allBlockers.push(b);
						}
					} 
				}
			}
		}
	}

	{
		TIMER_BLOCK_NAMED("GatherPoleBlockers");

		for(auto& it : allBlockers) {
			Rect3 aabb;
			if(it.type == BLOCKERTYPE_RECT) {
				aabb = transformBoundingBox(rect3(vec3(-0.5f), vec3(0.5f)), modelMatrix(it.xf));

			} else if(it.type == BLOCKERTYPE_CIRCLE) {
				float xy = max(it.xf.scale.x, it.xf.scale.y) * 0.5f;
				aabb = rect3(it.xf.trans - vec3(xy, xy, it.xf.scale.z*0.5f), it.xf.trans + vec3(xy, xy, it.xf.scale.z*0.5f));

			} else if(it.type == BLOCKERTYPE_LINE) {
				Vec3 mi = min(it.linePoints[0], it.linePoints[1]);
				Vec3 ma = max(it.linePoints[0], it.linePoints[1]);
				aabb = rect3(mi, ma);

				it.ab = it.linePoints[1] - it.linePoints[0];
			}

			it.aabb = aabb;

			aabb.min -= settings.playerRadius;
			aabb.max += settings.playerRadius;

			Vec2 mi = aabb.min.xy / settings.cellSize;
			Vec2 ma = aabb.max.xy / settings.cellSize;

			Recti pr = recti(ceil(mi.x), ceil(mi.y), floor(ma.x), floor(ma.y));
			if(!rectGetIntersection(&pr, grid, pr)) continue;

			for(int y = pr.bottom; y <= pr.top; y++) {
				for(int x = pr.left; x <= pr.right; x++) {
					Pole* pole = getPole(x - grid.left,y - grid.bottom);
					if(!pole->potentialBlockers.count) pole->potentialBlockers = dArray<int>(5, getTMemory);

					int blockerIndex = &it - allBlockers.data;
					pole->potentialBlockers.push(blockerIndex);
				}
			}
		}
	}

	// Sample poles for allBlockers.
	{
		TIMER_BLOCK_NAMED("BlockerPoles");

		auto threadFunc = [](void* data) {
			TIMER_BLOCK();
			ThreadHeader* h = (ThreadHeader*)data;
			WalkManifold* wm = (WalkManifold*)h->data;

			int index;
			while(h->addStackIndex(&index, 5)) {
				Pole* pole = wm->poles + index;

				for(auto& sample : pole->samples) {
					for(auto blockerIndex : pole->potentialBlockers) {
						bool isBlocked = wm->poleBlockerIntersection(pole->pos, sample.z, wm->allBlockers.data + blockerIndex); // pole->inside
						if(isBlocked) {
							if(!sample.blockers.count) sample.blockers = dArray<int>(10, getTMemoryInterlocked);
							sample.blockers.push(blockerIndex);
						}
					}
				}
			}
		};

		splitThreadTask(poles.count, this, threadFunc);
	}

	// Mark layers that lie inside a blocker and don't need edges.
	{
		TIMER_BLOCK_NAMED("MarkOutsideLayers");

		for(auto& cell : cells) {
			for(auto& layer : cell.layers) {
				if(layer.outside) continue;

				int startIndex = 0;
				for(int i = 0; i < 4; i++) {
					if(layer.poles[i].valid) {
						startIndex = i;
						break;
					}
				}

				bool outside = false;
				MeshSample* startSample = layer.poles[startIndex].sample;
				for(int i = 0; i < startSample->blockers.count; i++) {
					int blockerIndex = startSample->blockers[i];

					bool foundAll = true;
					for(int pi = 1; pi < 4; pi++) {

						int poleIndex = (pi + startIndex) % 4;
						LayerPole* pole = &layer.poles[poleIndex];

						bool found = false;
						if(pole->valid) {
							MeshSample* sample = pole->sample;
							for(auto it : sample->blockers) {
								if(it == blockerIndex) { found = true; break; }
							}
						} else found = true;
						if(!found) { foundAll = false; break; }
					}
					if(foundAll) { outside = true; break; }
				}
				
				layer.outside = outside;
			}
		}
	}

	// Calc blockers.
	{
		TIMER_BLOCK_NAMED("BlockerEdges");

		struct ThreadData {
			WalkManifold* wm;

			bool collectCells;
			DArray<int> collectedCells;
		};

		auto threadFunc = [](void* data) {
			TIMER_BLOCK();
			ThreadHeader* h = (ThreadHeader*)data;
			ThreadData* td = (ThreadData*)h->data;
			WalkManifold* wm = td->wm;

			int index;
			while(h->incStackIndex(&index)) {
				int x, y;
				aCoord(wm->cellsDim.w, index, &x, &y);
				WalkCell* cell = wm->getCell(x,y);

				if (cell->coord == vec2i(0, 1)) {
					int stop = 234;
				}

				for(auto& layer : cell->layers) {
					for(int edgeIndex = 0; edgeIndex < 4; edgeIndex++) {
						Vec2i is = wm->edgePoles[edgeIndex];
						LayerPole* p[2] = {layer.poles + is.e[0], layer.poles + is.e[1]};

						if(!p[0]->valid && !p[1]->valid) continue;
						if(edgeIndex > 1 && wm->adjacentLayerSharesEdge(&layer, vec2i(x,y), edgeIndex)) continue;

						for(int poleStage = 0; poleStage < 2; poleStage++) {
							if(poleStage) swap(&p[0], &p[1]);
							if(!p[0]->valid) continue;

							for(auto blockerIndex0 : p[0]->sample->blockers) {
								bool found = false;

								if(p[1]->valid) {
									for(auto blockerIndex1 : p[1]->sample->blockers) {
										if(blockerIndex1 == blockerIndex0) {
											found = true;
											break;
										}
									}
								}

								if(!found) {
									WalkLayer* layer2 = 0;
									if(edgeIndex < 2) wm->adjacentLayerSharesEdge(&layer, vec2i(x,y), edgeIndex, &layer2);

									if(!layer.outside || (layer2 && !layer2->outside)) {
										Vec3 edgePoint = wm->calcWalkEdgePointBlocker(p[1]->pos, p[0]->pos, wm->allBlockers + blockerIndex0);

										bool pushSecond = edgeIndex < 2 ? edgeIndex == poleStage : (edgeIndex-2) != poleStage;
										if(          !layer.outside  ) wm->addWalkEdgePointBlocker(&layer,  edgePoint, blockerIndex0,  pushSecond);
										if(layer2 && !layer2->outside) wm->addWalkEdgePointBlocker( layer2, edgePoint, blockerIndex0, !pushSecond);
									}
								}
							}
						}
					}
				}
			}
		};

		ThreadData td = {this, true, dArray<int>(cells.count, getTMemoryInterlocked)};
		splitThreadTask(cells.count, &td, threadFunc);
	}

	{
		TIMER_BLOCK_NAMED("CleanupLines");

		auto threadFunc = [](void* data) {
			TIMER_BLOCK();
			ThreadHeader* h = (ThreadHeader*)data;
			WalkManifold* wm = (WalkManifold*)h->data;

			struct Point {
				Vec2 p;
				float dist;
			};

			struct LineMarked {
				Vec2 a;
				Vec2 b;
				bool remove;
			};

			float flattenPercent = wm->settings.lineFlattenPercent;

			DArray<Line2> collectedLines = dArray<Line2>(5, getTMemoryInterlocked);
			DArray<Point> points = dArray<Point>(5, getTMemoryInterlocked);
			DArray<Line2> finalList = dArray<Line2>(5, getTMemoryInterlocked);
			DArray<LineMarked> markedLines = dArray<LineMarked>(5, getTMemoryInterlocked);

			int index;
			while(h->addStackIndex(&index, 10)) {
				WalkCell* cell = wm->cells + index;

				for(auto& layer : cell->layers) {
					if(!layer.lines.count) continue;

					layer.linesNoCleanup = dArray<Line3>(layer.lines, getTMemoryInterlocked);

					float floatOffset;
					{
						float maxVal = max(max((float)fabs(cell->poles[0]->pos.x), 
						                       (float)fabs(cell->poles[0]->pos.y)),
						                   max((float)fabs(cell->poles[2]->pos.x),
						                       (float)fabs(cell->poles[2]->pos.y)));
						floatOffset = floatPrecisionOffset(maxVal, 5, 0.0f);
					}

					for(auto it : layer.lines) {
						Line2 lineToAdd = {it.a.xy, it.b.xy};

						for(int i = 0; i < collectedLines.count; i++) {
							Line2 l0 = collectedLines[i];
							Line2 l1 = lineToAdd;
							
							Vec2 p;
							bool result = getLineIntersection(l0.a, l0.b, l1.a, l1.b, &p);
							if(result) {
								finalList.push({l0.a, p});
								finalList.push({p, l0.b});

								float dist = len(lineToAdd.a - p);
								points.push({p, dist});
							} else {
								finalList.push(l0);
							}
						}

						// Sort intersection points by distance to line start and then
						// create a line strip by those points.
						{
							auto cmp = [](const void* a, const void* b) -> int { 
								return ((Point*)a)->dist > ((Point*)b)->dist ? 1 : -1;
							};
							qsort(points.data, points.count, sizeof(Point), cmp);

							if(points.count) {
								for(int i = 0; i < points.count + 1; i++) {
									if(i == 0) finalList.push({lineToAdd.a, points[i].p});
									else if(i == points.count) finalList.push({points[i-1].p, lineToAdd.b});
									else finalList.push({points[i-1].p, points[i].p});
								}
							} else finalList.push(lineToAdd);
						}

						collectedLines.clear();
						collectedLines.push(finalList);
						finalList.clear();
						points.clear();
					}

					// Remove unneeded lines.
					// @Note(2023): Why do we keep checking lines that have been already removed?
					// Why do we check a/b and then b/a later? J should start at i, no?
					{
						DArray<int> removeList = dArray<int>(5, getTMemoryInterlocked);

						for(int i = 0; i < collectedLines.count; i++) {
							Line2 l0 = collectedLines[i];

							for(int j = 0; j < collectedLines.count; j++) {
								if(i == j) continue;

								Line2 l1 = collectedLines[j];
								for(int pi = 0; pi < 2; pi++) {
									Vec2 p = l0.e[pi];
									if(p == l1.a || p == l1.b) continue;

									float d = dot(rotateRight(l1.b-l1.a), p-l1.a);
									if(d < -floatOffset) goto remove;
								}
							}

							if(false) {
								remove:
								removeList.push(i);
							}
						}

						// Remove 0 length lines.
						for(int i = 0; i < collectedLines.count; i++) {
							Line2* line = collectedLines + i;
							if(line->a == line->b) {
								removeList.push(i);
							}
						}

						// Remove duplicates.
						for(int i = 0; i < collectedLines.count; i++) {
							Line2* l0 = collectedLines + i;
							for(int j = i+1; j < collectedLines.count; j++) {
								Line2* l1 = collectedLines + j;

								if((l0->a == l1->a) && (l0->b == l1->b)) {
									removeList.push(j);
								}
							}
						}

						// Ugh.
						for(auto it : removeList) { collectedLines[it] = {FLT_MAX}; }
						for(int i = 0; i < collectedLines.count; i++) {
							if(collectedLines[i].a.x == FLT_MAX) {
								collectedLines.remove(i);
								i--;
							}
						}
					}

					// Sort in some way to make the smoothing results consistent with threading.
					{
						auto cmp = [](const void* a, const void* b) -> int { 
							return ((Line2*)a)->a.x > ((Line2*)b)->b.x ? 1 : -1;
						};
						qsort(collectedLines.data, collectedLines.count, sizeof(Line2), cmp);
					}

					// Smooth out slight edges. Iterative, not great but fine for now.
					{
						finalList.clear();

						if(collectedLines.count == 1) {
							finalList.push(collectedLines[0]);

						} else if(collectedLines.count > 1) {
							markedLines.clear();
							for(auto& it : collectedLines) 
								markedLines.push({it.a, it.b, false});

							for(int i = 0; i < markedLines.count; i++) {
								LineMarked* l0 = markedLines + i;
								if(l0->remove) continue;
								
								for(int j = i+1; j < markedLines.count; j++) {
									LineMarked* l1 = markedLines + j;
									if(l1->remove) continue;

									if((l0->b == l1->a) ||
										(l1->b == l0->a)) { 

										bool flipped = l1->b == l0->a;
										Vec2 a = flipped ? l1->a : l0->a;
										Vec2 b = flipped ? l1->b : l0->b;
										Vec2 c = flipped ? l0->b : l1->b;

										bool result = dot(c-a, b-a) >= 0 && dot(a-c, b-c) >= 0;
										if(result) {
											float dist = distancePointLine(a,c,b);
											float lineLength = len(a-c);

											if(dist <= lineLength * flattenPercent) {
												l1->remove = true;
												*l0 = {a,c};

												// After we modified the line we have to check everyone again.
												j = i;
												continue;
											}
										}
									}
								}
							}

							for(auto it : markedLines) {
								if(!it.remove) finalList.push({it.a, it.b});
							}
						}
					}

					collectedLines.clear();

					float z = layer.lines[0].a.z;
					layer.lines.clear();
					for(auto it : finalList) {
						float z0 = wm->calcWalkCellZ(it.a, z, cell);
						float z1 = wm->calcWalkCellZ(it.b, z, cell);
						Line3 l = {vec3(it.a, z0), vec3(it.b, z1)};

						layer.lines.push(l);
					}

					finalList.clear();
				}
			}
		};

		splitThreadTask(cells.count, this, threadFunc);
	}
}

bool WalkManifold::outside(Vec2 pos, float z) {
	WalkCell* cell = getCell(pos);
	WalkLayer* layer = getWalkLayer(cell, z);
	if(!layer) return true;
	if(layer->outside) return true;

	if(!layer->lines.count) {
		bool noBlockers = true;
		for(auto& it : layer->poles) {
			if(it.sample->blockers.count) {
				noBlockers = false;
				break;
			}
		}
		if(noBlockers) return false;
		else return true;
	}

	bool behindLine = false;
	for(auto line : layer->lines) {
		Line2 l = {line.a.xy, line.b.xy};

		Vec2 dir  = rotateLeft(l.b - l.a);
		Vec2 dir2 = (l.a - pos);

		if(dot(dir, dir2) <= 0) {
			behindLine = true;
			break;
		}
	}
	if(behindLine) return true;

	return false;
}

Vec3 WalkManifold::pushInside(Vec2 playerPos, float z) {
	float offset = floatPrecisionOffset(playerPos, playerPos, 10, 0.000001f);
	Vec2 p = playerPos;

	int radius = 2;
	int hitCountMax = 10;

	int hitCount = -1;
	while(true) {
		hitCount++;

		float shortestDistance = FLT_MAX;
		Vec2 cp;

		Vec2i cellCoord = getCellCoord(p);
		Vec2i cMin = cellCoord - vec2i(radius, radius);
		Vec2i cMax = cellCoord + vec2i(radius, radius);

		for(int x = cMin.x; x <= cMax.x; x++) {
			for(int y = cMin.y; y <= cMax.y; y++) {
				WalkCell* cell = getCellGlobalBoundsCheck(x, y);
				if(!cell) continue;

				WalkLayer* layer = getWalkLayer(cell, z);
				if(!layer) continue;

				for(auto line : layer->lines) {
					Line2 l = {line.a.xy, line.b.xy};

					Vec2 dir  = rotateLeft(l.b - l.a);
					Vec2 dir2 = (l.a - p);

					bool behind = dot(dir, dir2) <= 0;
					if(behind) {
						Vec2 linePoint = projectPointOnLine(p, l.a, l.b, true);
						float dist = len(linePoint - p);
						if(dist < shortestDistance) {
							shortestDistance = dist;

							Vec2 off = norm(rotateRight(l.b - l.a)) * offset;
							cp = linePoint + off;
						}
					}
				}

			}
		}

		if(shortestDistance != FLT_MAX) {
			p = cp;

			if(!outside(p, z)) break;
			if(hitCount >= hitCountMax) break;

		} else break;
	}

	return calcWalkCellPos(p, z);
}

Vec3 WalkManifold::move(Vec2 playerPos, float z, Vec2 newPos) {
	float offset = floatPrecisionOffset(playerPos, newPos, 10, 0.000001f);

	Vec2 p = playerPos;
	Vec2 np = newPos;

	Vec2i cellCoord = getCellCoord(p);
	Line3 lastLine = {vec3(FLT_MAX), vec3(FLT_MAX)};

	bool firstLineHit = false;
	int lineHitCount = 0;
	int index = 0;
	Vec2 lastSavePos = p;

	Vec2 projectDir;

	while(true) {
		Line3 line;
		Vec2 cp;
		bool collision = gridLineCollision(p, z, np, &cellCoord, &line, &cp, lastLine);

		if(collision) {
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

				Vec2 normal = rotateRight(a);
				Vec2 op  = cp - normal * offset;
				Vec2 onp = pp - normal * offset;

				// Check if we can do the offset without a collision.
				{
					Line3 testLine;
					Vec2 ncp;
					bool collision = gridLineCollision(cp, z, op, &cellCoord, &testLine, &ncp, lastLine);
					if(collision) {
						// Can't do the offset so we try to compromise by taking 
						// the intersection with the perpendicular offset line.
						// And if that failes as well we just bail and don't move.

						Line3 ol = { vec3(line.a.xy - normal * offset,0), 
							         vec3(line.b.xy - normal * offset,0) };
						Vec2 ep;
						if(lineIntersection(p, cp, ol.a.xy, ol.b.xy, &ep)) p = ep;
						else p = lastSavePos;

						break;
					}
				}

				p = op;
				np = onp;

				// z = calcWalkCellPos(p, z).z;
			}

			lastSavePos = p;

			if(lineHitCount >= settings.maxLineCollisionCount) break;
			lineHitCount++;

			continue;

		} else {
			p = np;
			break;
		}
	}

	Vec3 cellPos = calcWalkCellPos(p, z);
	return cellPos;
}

bool WalkManifold::raycast(Vec3 pos, Vec3 rayDir, Vec3* inter) {
	bool foundIntersection = false;
	Vec3 intersection = vec3(0,0,0);
	float minDist = FLT_MAX;
	for(auto& cell : cells) {
		for(auto& l : cell.layers) {
			if(!l.poles[0].valid || !l.poles[1].valid ||
			   !l.poles[2].valid || !l.poles[3].valid) continue;

			Vec3 p[] = {l.poles[0].pos, 
							l.poles[1].pos, 
							l.poles[2].pos, 
							l.poles[3].pos };

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
	}

	if(!foundIntersection) return false;

	*inter = intersection;
	return true;
}

void WalkManifold::debugDraw() {
	TIMER_BLOCK();

	int cellOffset = ceil(settings.playerRadius / settings.cellSize);
	dxDepthTest(settings.depthTest); defer { dxDepthTest(true); }; 
	Vec3 off = vec3(0, 0, settings.depthTest ? 0.001f : 0);

	dxBeginPrimitiveColored(vec4(1,1), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	defer { dxEndPrimitive(); };

	// Draw grid.
	for(int y = cellOffset; y < cellsDim.h-cellOffset; y++) {
		for(int x = cellOffset; x < cellsDim.w-cellOffset; x++) {
			WalkCell* cell = getCell(x,y);

			for(auto& layer : cell->layers) {
				for(int edgeIndex = 0; edgeIndex < 4; edgeIndex++) {
					Vec2i is = edgePoles[edgeIndex];
					LayerPole* p[2] = {layer.poles + is.e[0], layer.poles + is.e[1]};

					if((edgeIndex == 2 && y != cellsDim.h-cellOffset-1) || 
					   (edgeIndex == 3 && x != cellsDim.w-cellOffset-1)) {
						if(edgeIndex > 1 && adjacentLayerSharesEdge(&layer, vec2i(x,y), edgeIndex)) continue;
					}

					bool in[2] = {p[0]->valid && !p[0]->sample->blockers.count, 
					              p[1]->valid && !p[1]->sample->blockers.count};

					if(in[0] || in[1]) {
						if(in[0] && in[1]) {
							dxPushLine(p[0]->pos+off, p[1]->pos+off, settings.cGrid);

						} else {
							if(!in[0]) swap(&p[0], &p[1]);

							Vec3 edgePos;
							float dist = FLT_MAX;
							for(int i = 0; i < layer.lines.count; i++) {
								Line3 line = layer.lines[i];
								Vec2 cp;
								bool result = getLineIntersection(p[0]->pos.xy, p[1]->pos.xy, line.a.xy, line.b.xy, &cp);	
								if(result) {
									float cDist = len(p[0]->pos.xy - cp);
									if(cDist < dist) {
										dist = cDist;
										edgePos.xy = cp;
									}
								}
							}

							edgePos.z = calcWalkCellZ(edgePos.xy, layer.zRange.max, cell);
							dxPushLine(p[0]->pos+off, edgePos+off, settings.cGrid);

							if(settings.drawOutsideGrid)
								dxPushLine(p[1]->pos+off, edgePos+off, settings.cGridOutside);
						}

					} else {
						if(settings.drawOutsideGrid) {
							if(p[0]->valid && p[1]->valid) {
								dxPushLine(p[0]->pos+off, p[1]->pos+off, settings.cGridOutside);

							} else if(p[0]->valid || p[1]->valid) {
								if(!p[0]->valid) swap(&p[0], &p[1]);
							
								if(layer.pointCount) {
									bool sw = dot(norm(layer.points[0].xy-p[0]->pos.xy), norm(p[1]->pos.xy-p[0]->pos.xy)) >
												 dot(norm(layer.points[1].xy-p[0]->pos.xy), norm(p[1]->pos.xy-p[0]->pos.xy));
									Vec3 edgePos = sw ? layer.points[0] : layer.points[1];

									dxPushLine(p[0]->pos+off, edgePos+off, settings.cGridOutside);
								}
							}
						}
					}

					dxFlushIfFull();
				}

			}
		}
	}

	off.z *= 1.05f;

	// Draw walk edges.
	if(settings.drawWalkEdges) {
		for(int y = 0; y < cellsDim.h; y++) {
			for(int x = 0; x < cellsDim.w; x++) {
				WalkCell* cell = getCell(x,y);

				for(auto& it : cell->layers) {
					if(it.pointCount == 2) {
						dxPushLine(it.points[0]+off, it.points[1]+off, settings.cWalkEdge);
						dxFlushIfFull();
					}

					if(it.pointCount == 3) {
						dxPushLine(it.points[0]+off, it.points[2]+off, settings.cWalkEdge);
						dxPushLine(it.points[2]+off, it.points[1]+off, settings.cWalkEdge);
						dxFlushIfFull();
					}
				}
			}
		}
	}

	// Draw lines.
	{
		for(int y = cellOffset; y < cellsDim.h-cellOffset; y++) {
			for(int x = cellOffset; x < cellsDim.w-cellOffset; x++) {
				WalkCell* cell = getCell(x,y);

				for(auto& layer : cell->layers) {
					if(settings.drawLinesNoCleanup) {
						for(auto& line : layer.linesNoCleanup) {
							dxPushLine(line.a+off, line.b+off, settings.cBlockerLineNoCleanup);
							dxFlushIfFull();
						}
					}

					for(auto& line : layer.lines) {
						dxPushLine(line.a+off, line.b+off, settings.cBlockerLine);
						dxFlushIfFull();
					}
				}
			}
		}
	}
}

void WalkManifold::free() {
	for(auto& cell : cells) {
		CloseHandle(cell.mutex);
	}
}

//

inline Pole* WalkManifold::getPole(int x, int y) {
	return poles + aIndex(gridDim.w, x, y);
}

inline Pole* WalkManifold::getPoleBoundsCheck(int x, int y) {
	if(x < 0 || x > gridDim.w-1 || y < 0 || y > gridDim.h-1) return 0;
	return getPole(x, y);
}

inline WalkCell* WalkManifold::getCell(int x, int y) {
	return cells + aIndex(cellsDim.w, x, y);
}

inline WalkCell* WalkManifold::getCellBoundsCheck(int x, int y) {
	if(x < 0 || x > cellsDim.w-1 || y < 0 || y > cellsDim.h-1) return 0;
	return getCell(x, y);
}

inline WalkCell* WalkManifold::getCellGlobal(int x, int y) {
	x = x - grid.left;
	y = y - grid.bottom;
	return cells + aIndex(cellsDim.w, x, y);
}

inline WalkCell* WalkManifold::getCellGlobalBoundsCheck(int x, int y) {
	x = x - grid.left;
	y = y - grid.bottom;
	if(x < 0 || x > cellsDim.w-1 || y < 0 || y > cellsDim.h-1) return 0;
	return getCell(x, y);
}

Vec2i WalkManifold::getCellCoord(Vec2 pos) {
	Vec2 cellF = pos / settings.cellSize;
	Vec2i cellCoord = vec2i(floor(cellF.x), floor(cellF.y));

	return cellCoord;
}

WalkCell* WalkManifold::getCell(Vec2 pos) {
	Vec2i coord = getCellCoord(pos);
	return getCellGlobal(coord.x, coord.y);
}

float WalkManifold::calcWalkLayerZ(Vec2 pos, WalkLayer* layer) {
	if(!layer) return 0;

	// Get highest pole. Then triangulate clockwise.
	float zMax = -FLT_MAX;
	int index = 0;
	for(auto& pole : layer->poles) {
		if(!pole.valid) return 0;

		if(pole.pos.z > zMax) {
			zMax = pole.pos.z;
			index = &pole - layer->poles;
		}
	}

	Vec3 v[4];
	for(int i = 0; i < 4; i++) v[i] = layer->poles[(index+i)%4].pos;

	Vec3 coords = barycentricCoordinates(pos, v[0].xy, v[1].xy, v[2].xy);
	float z;

	// If not in first triangle choose the second triangle of quad.
	// floatOffset = floatPrecisionOffset(maxVal, 5, 0.0f);
	float fp = 0.0f - floatPrecisionOffset(max(pos.x, pos.y), 5, 0.0f);
	if(coords.x < fp || coords.y < fp || coords.z < fp) {
		coords = barycentricCoordinates(pos, v[2].xy, v[3].xy, v[0].xy);
		z = coords.x * v[2].z + coords.y * v[3].z + coords.z * v[0].z;

	} else {
		z = coords.x * v[0].z + coords.y * v[1].z + coords.z * v[2].z;
	}

	return z;
}

float WalkManifold::calcWalkCellZ(Vec2 pos, float pz, WalkCell* cell) {
	return calcWalkLayerZ(pos, getWalkLayer(cell, pz));
}

Vec3 WalkManifold::calcWalkCellZPos(Vec2 pos, float pz, WalkCell* cell) {
	float z = calcWalkCellZ(pos, pz, cell);
	return vec3(pos, z);
}

Vec3 WalkManifold::calcWalkCellPos(Vec2 pos, float pz) {
	Vec2i cellCoord = getCellCoord(pos);
	WalkCell* cell = &cells[aIndex(cellsDim.w, cellCoord.x-grid.left, cellCoord.y-grid.bottom)];

	Vec3 wp = calcWalkCellZPos(pos, pz, cell);

	return wp;
}

WalkLayer* WalkManifold::getWalkLayer(WalkCell* cell, float z) {
	if(!cell->layers.count) return 0;
	if(cell->layers.count == 1) return &cell->layers[0];

	float minDist = FLT_MAX;
	WalkLayer* bestLayer = 0;
	for(auto& layer : cell->layers) {
		float dist = layer.zRange.max < z ? z - layer.zRange.max : 
		            (layer.zRange.min > z ? layer.zRange.min - z : 0);

		if(dist < minDist) {
			minDist = dist;
			bestLayer = &layer;
		}
	}

	return bestLayer;
}

WalkLayer* WalkManifold::getWalkLayer(Vec2i coord, float z) {
	WalkCell* cell = getCellBoundsCheck(coord.x, coord.y);
	if(!cell) return 0;

	return getWalkLayer(cell, z);
}

WalkLayer* WalkManifold::getWalkLayer(WalkCell* cell, WalkLayer* layer) {
	return getWalkLayer(cell, (layer->zRange.min + layer->zRange.max)/2.0f);
}

WalkLayer* WalkManifold::getWalkLayer(Vec2i coord, WalkLayer* layer) {
	WalkCell* cell = getCellBoundsCheck(coord.x, coord.y);
	if(!cell) return 0;

	return getWalkLayer(cell, (layer->zRange.min + layer->zRange.max)/2.0f);
}

bool WalkManifold::adjacentLayerSharesEdge(WalkLayer* layer, Vec2i coord, int edgeIndex, WalkLayer** adjLayer) {
	// EdgeIndex: 0->3 = bottom, left, top, right.

	Vec2i coordDirs[] = {{0,-1}, {-1,0}, {0,1}, {1,0}};
	coord += coordDirs[edgeIndex];

	if(adjLayer) *adjLayer = 0;

	WalkCell* adjacentCell = getCellBoundsCheck(coord.x, coord.y);
	if(!adjacentCell) return false;

	WalkLayer* adjacentLayer = getWalkLayer(adjacentCell, layer);
	if(!adjacentLayer) return false;

	Vec2i poles[] = {edgePoles[edgeIndex], edgePoles[(edgeIndex+2)%4]};
	if(layer->poles[poles[0].e[0]].sample == adjacentLayer->poles[poles[1].e[0]].sample && 
	   layer->poles[poles[0].e[1]].sample == adjacentLayer->poles[poles[1].e[1]].sample) {
		if(adjLayer) *adjLayer = adjacentLayer;
	   return true;
	}

	return false;
}

//

bool WalkManifold::poleMeshIntersection(Vec2 pos, WalkMesh* mesh, float* z) {
	Vec3 rayPos = vec3(pos, 1000);
	Vec3 rayDir = vec3(0,0,-1);
	Vec3 rayPosT, rayDirT;
	transformInverse(rayPos, rayDir, mesh->xfInv, rayPosT, rayDirT);

	Vec3 intersection;
	float dist = lineBoxIntersection(rayPosT, rayDirT, vec3(0.0f), vec3(1.0f), &intersection);
	if(dist != -1) {
		intersection = mesh->xf * intersection;
		*z = intersection.z;
		return true;
	}

	return false;
}

bool WalkManifold::poleBlockerIntersection(Blocker* blocker0, Blocker* blocker1) {
	{
		float z = blocker0->xf.trans.z;
		Vec2 zr = vec2(blocker1->aabb.min.z, blocker1->aabb.max.z);

		if(blocker1->type == BLOCKERTYPE_LINE) {
			if(z > zr.max + settings.playerRadius) return false;
			if(z < zr.min - settings.playerHeight) return false;

			Vec2 p = blocker0->xf.trans.xy;
			Vec3 a = blocker1->linePoints[0];
			Vec3 b = blocker1->linePoints[1];

			bool test2d = false;
			if(zr.min == zr.max) test2d = true;
			else if(z > zr.max) test2d = true;
			else if(z < zr.min && z > (zr.min - (settings.playerHeight - (zr.max - zr.min)))) test2d = true;

			if(test2d) {
				return distancePointLine(a.xy, b.xy, p) <= settings.playerRadius;

			} else {
				if(z < zr.min) z += settings.playerHeight;
				float percent = mapRange(z, a.z, b.z, 0.0f, 1.0f);
				Vec3 _p = a + (percent * (b - a));

				return len(p - _p.xy) <= settings.playerRadius;
			}

		} else {
			zr.min -= settings.playerHeight;
			if(!(z >= zr.min && z <= zr.max)) return false;
		}
	}

	bool result = gjk(blocker0, vec2(0,0), blocker1);
	return result;
}

bool WalkManifold::poleBlockerIntersection(Vec2 pos, float z, Blocker* blocker1) {
	Blocker srcBlocker = {BLOCKERTYPE_CIRCLE, xForm(vec3(pos, z), vec3(settings.playerRadius*2))};
	return poleBlockerIntersection(&srcBlocker, blocker1);
}

Vec3 WalkManifold::calcWalkEdgePoint(LayerPole* p0, LayerPole* p1, int zTestMode, bool* hitSomething) {
	bool swp = false;
	if(!p0->valid || !p1->valid) swp = !p0->valid;
	else if(zTestMode < 0 || (!zTestMode && p0->pos.z <= p1->pos.z)) swp = true;
	if(swp) swap(&p0, &p1);

	Vec2 c;
	float startZ = p0->pos.z, z = p0->pos.z;
	WalkMesh* mesh0 = meshes + p0->sample->mesh;

	Vec2 p[] = {p0->pos.xy, p1->pos.xy};

	bool hits[2] = {};
	if(!p0->valid || !p1->valid) {
		for(int i = 0; i < settings.edgeSearchIterations; i++) {
			c = (p[0]+p[1])/2.0f;

			bool hit = poleMeshIntersection(c, mesh0, &z);
			hits[hit] = true;
			if(hit && fabs(startZ - z) > settings.legHeight) hit = false;

			p[hit ? 0 : 1] = c;
		}

	} else {
		WalkMesh* mesh1 = meshes + p1->sample->mesh;
		zTestMode = abs(zTestMode);

		for(int i = 0; i < settings.edgeSearchIterations; i++) {
			c = (p[0]+p[1])/2.0f;

			float z0, z1;
			bool hit0 = poleMeshIntersection(c, mesh0, &z0);
			bool hit1 = poleMeshIntersection(c, mesh1, &z1);
			if(hit0) z = z0;

			if(hit0) hits[0] = true;
			if(hit1) hits[1] = true;

			if(zTestMode) {
				if(hit1 && fabs(startZ - z1) <= settings.playerHeight) {
					if(zTestMode == 1) hit0 = false; 
					else if(zTestMode == 2) hit1 = false; 
				}
			}

			bool leg0 = fabs(startZ - z0) <= settings.legHeight;
			if     (!hit0 && !hit1) p[1] = c;
			else if( hit0 && !hit1) p[leg0 ? 0 : 1] = c;
			else if(!hit0 &&  hit1) p[1] = c;
			else                    p[leg0 ? 0 : 1] = c;
		}
	}

	// HitSomething is for calculating the interpolated walk edge point,
	// if we exclusively get one hit event or another we assume the 
	// calculated point is not valid.
	if(hitSomething) *hitSomething = hits[0] && hits[1];

	return vec3(c, z);
}

Vec3 WalkManifold::calcWalkEdgePointBlocker(Vec3 a, Vec3 b, Blocker* blocker1) {
	Blocker blocker0 = {BLOCKERTYPE_CIRCLE, xForm(a, vec3(settings.playerRadius*2))};

	Vec3 c;
	for(int i = 0; i < settings.edgeSearchIterations; i++) {
		c = (a+b)/2.0f;

		blocker0.xf.trans = c;
		bool hit = !poleBlockerIntersection(&blocker0, blocker1);

		if(hit) a = c;
		else b = c;
	}

	return c;
}

void WalkManifold::addWalkEdgePoint(WalkLayer* layer, Vec3 edgePoint, PointInfo pointInfo) {
	// assert(cell->pointCount <= 1);
	if(layer->pointCount > 1) return;

	{
		volatile uint currentPointCount;
		while(true) {
			currentPointCount = layer->pointCount;
			volatile uint newPointCount = currentPointCount + 1;

			LONG oldValue = InterlockedCompareExchange((LONG volatile*)&layer->pointCount, newPointCount, currentPointCount);
			if(oldValue == currentPointCount) break;
		}

		layer->points[currentPointCount] = edgePoint;
		layer->pointInfos[currentPointCount] = pointInfo;
	}

	// Calc interpolated middle point.
	if(layer->pointCount == 2) {

		float testOffset = settings.cellSize * 0.01f;
		float ignoreDist = settings.cellSize * 0.001f;
		float poleExpansion = settings.cellSize * 0.5f;

		Vec3 testPoints[2];
		Vec2 edgeDirs[] = {{0,1}, {1,0}, {0,-1}, {-1,0}};

		bool bothHit = true;
		for(int i = 0; i < 2; i++) {
			PointInfo* pi = layer->pointInfos + i;
			Vec2i is = edgePoles[pi->edgeIndex];

			LayerPole p[2] = {*pi->p[0], *pi->p[1]};
			Vec2 offset = edgeDirs[pi->edgeIndex] * testOffset;
			p[0].pos.xy += offset;
			p[1].pos.xy += offset;

			Vec2 dirs[2] = {};
			if(p[0].pos.x != p[1].pos.x) dirs[0].x = p[0].pos.x < p[1].pos.x ? -1 : 1;
			else                         dirs[0].y = p[0].pos.y < p[1].pos.y ? -1 : 1;
			dirs[1] = -dirs[0];

			// Hit says if there is actually a valid edge.
			bool hit;
			testPoints[i] = calcWalkEdgePoint(&p[0], &p[1], pi->playerHeightMode, &hit);
			if(!hit) {
				bothHit = false;
				break;
			}

			// If the distance of the middle point isn't far enough we ignore it.
			float dist = distancePointLine(layer->points[0].xy, layer->points[1].xy, testPoints[i].xy);
			if(dist < ignoreDist) {
				bothHit = false;
				break;
			}
		}
		if(!bothHit) return;

		Vec2 dirs[2] = {testPoints[0].xy - layer->points[0].xy, testPoints[1].xy - layer->points[1].xy};
		Vec2 thirdPoint;
		bool result = getLineIntersectionInf(layer->points[0].xy, dirs[0], layer->points[1].xy, dirs[1], &thirdPoint);
		if(!result) return;

		// If the distance from the main cell/layer is too far we assume we messed up and bail.
		if(min(len(thirdPoint - layer->points[0].xy), len(thirdPoint - layer->points[1].xy)) > settings.cellSize*2) return;

		// Interpolate z.
		float zs[2];
		for(int i = 0; i < 2; i++) {
			float zp = len(layer->points[i].xy - thirdPoint) / len(dirs[i]);
			zs[i] = testPoints[i].z + (testPoints[i].z - layer->points[i].z)*zp;
		}
		float z = (zs[0] + zs[1]) / 2.0f;

		layer->points[layer->pointCount++] = vec3(thirdPoint, z);
	}
}

void WalkManifold::addWalkEdgePointBlocker(WalkLayer* layer, Vec3 edgePoint, int blockerIndex, bool pushSecond) {
	WalkCell* cell = layer->cell;

	int state = WaitForSingleObject(cell->mutex, INFINITE);
	defer { ReleaseMutex(cell->mutex); };

	if(!layer->blockerPoints.count) {
		layer->blockerPoints = dArray<BlockerPointList>(10, getTMemoryInterlocked);
	}

	BlockerPointList* list = 0;
	for(int i = 0; i < layer->blockerPoints.count; i++) {
		if(layer->blockerPoints[i].blockerIndex == blockerIndex) {
			list = layer->blockerPoints + i;
			break;
		}
	}

	if(!list) {
		BlockerPointList nl;
		nl.pointCount = 0;
		nl.blockerIndex = blockerIndex;
		layer->blockerPoints.push(nl);

		list = layer->blockerPoints.data + layer->blockerPoints.count-1;
	}

	if(list->pointCount > 1) return;

	list->points[list->pointCount++] = edgePoint;

	if(list->pointCount == 2) {
		if(!layer->lines.count) layer->lines = dArray<Line3>(10, getTMemoryInterlocked);

		// Make sure to push lines in clockwise order from blocker perspective.
		Line3 line;
		if(!pushSecond) line = { list->points[0], list->points[1] };
		else            line = { list->points[1], list->points[0] };

		layer->lines.push(line);
	}
}

bool WalkManifold::gridLineCollision(Vec2 p, float z, Vec2 np, Vec2i* cellCoord, Line3* collisionLine, Vec2* collisionPoint, Line3 lastLine) {
	Vec2i cellDirs[] = { {-1,0}, {1,0}, {0,-1}, {0,1}, };
	int cellEdgeIndexOpposite[] = {1,0,3,2};

	Vec2i newPosCell = getCellCoord(np);

	while(true) {
		WalkCell* cell = &cells[aIndex(cellsDim.w, cellCoord->x-grid.left, cellCoord->y-grid.bottom)];
		WalkLayer* layer = getWalkLayer(cell, z);
		if(layer->lines.count) {
			Vec2 cp;
			int lineIndex = lineCollision(p, np, layer->lines.data, layer->lines.count, lastLine, &cp);
			if(lineIndex != -1) {
				*collisionLine = layer->lines[lineIndex];
				*collisionPoint = cp;

				return true;
			}
		}

		// Get cell intersection and move to neighbour cell.
		{
			Rect cellRect = rect(vec2(*cellCoord)*settings.cellSize, vec2(*cellCoord+1)*settings.cellSize);

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

	// assert(newPosCell == *cellCoord);

	return false;
}

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

int WalkManifold::lineCollision(Vec2 p0, Vec2 p1, Line3* lines, int lineCount, Line3 lastLine, Vec2* cp) {
	int lineIndex = -1;
	Vec2 collisionPoint = {};
	float collisionDist = FLT_MAX;
	for(int i = 0; i < lineCount; i++) {
		Line3 line = lines[i];

		if(lastLine.a == line.a && lastLine.b == line.b) continue;
		if(dot(rotateRight(line.b.xy - line.a.xy), p1-p0) > 0) continue;

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
