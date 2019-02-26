
Vec2 furthestPointInDirection(Blocker* blocker, Vec2 pos, Vec2 dir, bool sweep = false, Vec2 sweepOffset = vec2(0,0)) {
	XForm form = blocker->form;

	Vec2 result;
	switch(blocker->type) {
		case BLOCKERTYPE_RECT: {
			Rect r = rect(-form.scale.xy/2.0f, 
			               form.scale.xy/2.0f);

			dir = (inverse(form.rot) * vec3(dir,0)).xy;

			if(dir.y > 0) result = dir.x > 0 ? r.tr() : r.tl();
			else          result = dir.x > 0 ? r.br() : r.bl();

			result = (form.rot * vec3(result,0)).xy;
			result += pos;
		} break;

		case BLOCKERTYPE_CIRCLE: {
			// Assumes that dir will never be zero.
			Vec2 norm = dir/sqrt(dir.x*dir.x + dir.y*dir.y); 
			float radius = form.scale.x;
			result = pos + norm*radius;
		} break;

		case BLOCKERTYPE_LINE: {
			Vec2 dirPoint = blocker->linePoints[1].xy - blocker->linePoints[0].xy;
			result = dot(dir, dirPoint) > 0 ? blocker->linePoints[1].xy : blocker->linePoints[0].xy;
		} break;
	}

	if(sweep && (dot(sweepOffset,dir) >= 0)) result += sweepOffset;

	return result;
}

Vec2 support(Blocker* s1, Vec2 pos1, Blocker* s2, Vec2 pos2, Vec2 dir, bool sweep = false, Vec2 sweepOffset = vec2(0,0)) {
	Vec2 p1 = furthestPointInDirection(s1, pos1, dir, sweep, sweepOffset);
	Vec2 p2 = furthestPointInDirection(s2, pos2, dir*-1);
	Vec2 result = p1 - p2;

	return result; 
}

bool containsOrigin(Vec2* simplex, int* sCount, Vec2* dir) {
	Vec2 a = simplex[*sCount-1];
	Vec2 ao = a*-1;

	if(*sCount == 3) {
		Vec2 b = simplex[1];
		Vec2 c = simplex[0];

		Vec2 abPerp = perpToPoint(a, b, c)*-1;
		Vec2 acPerp = perpToPoint(a, c, b)*-1;

		if(dot(abPerp,ao) > 0) {
			// remove c
			simplex[0] = b;
			simplex[1] = a;
			*sCount = *sCount - 1;
			*dir = abPerp;

		} else if(dot(acPerp,ao) > 0) {
			// remove b
			simplex[1] = a;
			*sCount = *sCount - 1;
			*dir = acPerp;

		} else {
			return true;
		}

	} else {
		Vec2 b = simplex[0];
		Vec2 ab = {b.x-a.x, b.y-a.y};
		Vec2 perp = perpToPoint(ab, ao);

		if(perp == vec2(0,0)) perp = vec2(ab.y, -ab.x);

		*dir = perp;
	}

	return false;
}

bool gjk(Blocker* s1, Vec2 oldPos, Blocker* s2, Vec2* inputSimplex, bool sweepingEnabled) {
	Vec2 pos1 = s1->form.trans.xy;	
	Vec2 pos2 = s2->form.trans.xy;	

	Vec2 sweepingDirection = (pos1 - oldPos)*-1;
	// if(pos1 - oldPos == vec2(0,0)) sweepingEnabled = false;

	Vec2 simplex[3];
	int sCount = 0;
	Vec2 dir = pos2 - pos1;
	if(dir == vec2(0,0)) dir = vec2(1,0);
	simplex[sCount] = support(s1, pos1, s2, pos2, dir, sweepingEnabled, sweepingDirection);

	sCount++;
	dir *= -1;
	int iterations = -1;
	bool collision;
	for(;;) {
		iterations++;

		simplex[sCount] = support(s1, pos1, s2, pos2, dir, sweepingEnabled, sweepingDirection);
		sCount++;

		dir = norm(dir);

		float pastOrigin = dot(simplex[sCount-1], dir);
		if(pastOrigin <= 0) {
			collision = false;
			break;
		} else {
			if(containsOrigin(simplex, &sCount, &dir)) {
				collision = true;
				break;
			}
		}
	}

	if(inputSimplex) copyArray(inputSimplex, simplex, Vec2, 3);

	return collision;
}

Vec2 epa(Blocker* s1, Vec2 oldPos, Blocker* s2, Vec2* inputSimplex, Vec2 searchDirection = vec2(0,0), bool sweepingEnabled = false) {
	Vec2 pos1 = s1->form.trans.xy;	
	Vec2 pos2 = s2->form.trans.xy;

	float tolerance = floatPrecisionOffset(pos1, pos2, 10, 0.000001f);
	float offset    = floatPrecisionOffset(pos1, pos2, 10, 0.000001f);

	Vec2 simplex[16];
	memcpy(simplex, inputSimplex, sizeof(Vec2)*3);
	float sCount = 3;

	// NOTE: Make sure that polygon winding is clockwise
	float pArea = polygonArea(simplex, sCount);
	if(pArea > 0) swap(&simplex[0], &simplex[1]); 

	Vec2 sweepingDirection = (pos1 - oldPos)*-1;
	if(pos1 - oldPos == vec2(0,0)) sweepingEnabled = false;

	Vec2 originOffset = vec2(1100,400);

	Vec2 origin = vec2(0,0);
	Vec2 penetration = {};
	int iteration = -1;

	if(searchDirection == vec2(0,0)) {
		while(true) {
			iteration++;
			float smallestDistance = 10000000;
			Vec2 edgeNormal;
			int edgeIndex;
			for(int vIndex = 0; vIndex < sCount; ++vIndex) {
				int indexPlusOne = vIndex + 1;
				if(vIndex == sCount - 1) indexPlusOne = 0;
				Vec2 p1 = simplex[vIndex];
				Vec2 p2 = simplex[indexPlusOne];

				float distance = distancePointLineX(p1, p2, origin, false);
				if(distance < smallestDistance) {
					smallestDistance = distance;
					edgeNormal = lineNormal(p1, p2);
					edgeIndex = vIndex+1;
				}
			}

			Vec2 newP = support(s1, pos1, s2, pos2, edgeNormal, sweepingEnabled, sweepingDirection);

			float d = dot(newP,edgeNormal);
			if((d - smallestDistance < tolerance) || (sCount == arrayCount(simplex)-1)) {
				penetration = edgeNormal*d*-1;
				break;
			} else {
				// Insert in between points
				for(int i = sCount; i > edgeIndex; --i) simplex[i] = simplex[i-1];
				sCount++;
				simplex[edgeIndex] = newP;

				assert(sCount < arrayCount(simplex));
			}
		}
	} else {
		float searchDistance = 1000000;
		sweepingEnabled = false;

		while(true) {
			iteration++;
			// if(directionIndex > directionCount) break;
			// Vec2 searchDirection = searchDirections[directionIndex];

			float distanceToOrigin = -1;
			float bestDotProduct = 0;
			Vec2 edgeNormal;
			int edgeIndex;
			for(int vIndex = 0; vIndex < sCount; ++vIndex) {
				int indexPlusOne = vIndex + 1;
				if(vIndex == sCount - 1) indexPlusOne = 0;
				Vec2 p1 = simplex[vIndex];
				Vec2 p2 = simplex[indexPlusOne];

				Vec2 intersection = {};
				if(getLineIntersection(p1, p2, origin, origin+searchDirection*searchDistance, &intersection)) {
					edgeNormal = lineNormal(p1,p2);
					edgeIndex = vIndex+1;
					distanceToOrigin = distancePointLineX(p1,p2,origin,true);
					break;
				}
			}

			Vec2 newP = support(s1, pos1, s2, pos2, edgeNormal, sweepingEnabled, sweepingDirection);

			float d = dot(newP,edgeNormal);
			if(d - distanceToOrigin < tolerance) {
				penetration = edgeNormal*d*-1;				

				Vec2 intersection = {};
				for(int i = 0; i < sCount; i++) {
					int indexPlusOne = i + 1;
					if(i == sCount - 1) indexPlusOne = 0;
					Vec2 p1 = simplex[i];
					Vec2 p2 = simplex[indexPlusOne];

					bool intersected = getLineIntersection(p1, p2, origin, searchDirection*1000, &intersection);
					if(intersected) break;
				}

				penetration = intersection*-1;

				break;
			} else {
				// Insert in between points
				for(int i = sCount; i > edgeIndex; --i) simplex[i] = simplex[i-1];
				sCount++;
				simplex[edgeIndex] = newP;

				assert(sCount < arrayCount(simplex));
			}
		}
	}

	Vec2 correctionOffset = norm(penetration)*offset;
	penetration += correctionOffset;

	return penetration;
}

bool gjkEpa(Blocker s1, Vec2 oldPos, Blocker s2, Vec2* vec, bool sweepingEnabled = false) {
	Vec2 simplex[3];
	bool result = gjk(&s1, oldPos, &s2, simplex, sweepingEnabled);
	if(result) {
		// Vec2 searchDirection = sweepingEnabled ? -norm(oldPos - s1.form.trans.xy) : vec2(0);
		Vec2 searchDirection = vec2(0);
		*vec = epa(&s1, oldPos, &s2, simplex, searchDirection, sweepingEnabled);
	}

	return result;
}

// Vec2 epa(Blocker* s1, Vec2 oldPos, Blocker* s2, Vec2* inputSimplex, Vec2 searchDirection = vec2(0,0)) {

// 	Vec2 pos1 = s1->form.trans.xy;	
// 	Vec2 pos2 = s2->form.trans.xy;

// 	bool visualisation = EPA_VISUALS;
// 	float tolerance = 0.00001f;
// 	float offset    = 0.00001f;

// 	Vec2 simplex[16];
// 	memcpy(simplex, inputSimplex, sizeof(Vec2)*3);
// 	float sCount = 3;

// 	// NOTE: Make sure that polygon winding is clockwise
// 	float pArea = polygonArea(simplex, sCount);
// 	if(pArea > 0) {
// 		swap(&simplex[0], &simplex[1]); 
// 	}

// 	Vec2 sweepingDirection = (pos1 - oldPos)*-1;
// 	bool sweepingEnabled = true;
// 	if(pos1 - oldPos == vec2(0,0)) sweepingEnabled = false;

// 	Vec2 originOffset = vec2(1100,400);
// 	// if(visualisation) {
// 	// 	// drawLine(drawCallBuffer, originOffset+simplex[0], originOffset+simplex[1], mapRGBA(0,1,1,1));
// 	// 	// drawLine(drawCallBuffer, originOffset+simplex[1], originOffset+simplex[2], mapRGBA(0,1,1,1));
// 	// 	// drawLine(drawCallBuffer, originOffset+simplex[2], originOffset+simplex[0], mapRGBA(0,1,1,1));
// 	// 	for(int i = 0; i < 3; i++) {
// 	// 		drawCircle(drawCallBuffer, originOffset+simplex[i], 3, mapRGBA(1,1,0,1), 3, true);
// 	// 	}
// 	// }

// 	Vec2 origin = vec2(0,0);
// 	Vec2 penetration = {};
// 	int iteration = -1;

// 	if(searchDirection == vec2(0,0)) {
// 		for(;;) {
// 			iteration++;
// 			float smallestDistance = 10000000;
// 			Vec2 edgeNormal;
// 			int edgeIndex;
// 			for(int vIndex = 0; vIndex < sCount; ++vIndex) {
// 				int indexPlusOne = vIndex + 1;
// 				if(vIndex == sCount - 1) indexPlusOne = 0;
// 				Vec2 p1 = simplex[vIndex];
// 				Vec2 p2 = simplex[indexPlusOne];

// 				float distance = distancePointLineX(p1, p2, origin, false);
// 				if(distance < smallestDistance) {
// 					smallestDistance = distance;
// 					edgeNormal = lineNormal(p1, p2);
// 					edgeIndex = vIndex+1;
// 				}
// 			}

// 			Vec2 newP = support(s1, pos1, s2, pos2, edgeNormal, sweepingEnabled, sweepingDirection);
// 			// if(visualisation) {
// 			// 	// drawPoint(drawCallBuffer, originOffset + newP, mapRGBA(0,1,1,1));
// 			// 	drawCircle(drawCallBuffer, originOffset + newP, 2, mapRGBA(0,1,1,1), 2, true);
// 			// }

// 			float d = dot(newP,edgeNormal);
// 			if((d - smallestDistance < tolerance) || (sCount == arrayCount(simplex)-1)) {
// 				penetration = edgeNormal*d*-1;
// 				break;
// 			} else {
// 				// Insert in between points
// 				for(int i = sCount; i > edgeIndex; --i) simplex[i] = simplex[i-1];
// 				sCount++;
// 				simplex[edgeIndex] = newP;

// 				assert(sCount < arrayCount(simplex));
// 			}
// 		}
// 	} else {
// 		float searchDistance = 1000000;
// 		sweepingEnabled = false;

// 		for(;;) {
// 			iteration++;
// 			// if(directionIndex > directionCount) break;
// 			// Vec2 searchDirection = searchDirections[directionIndex];

// 			float distanceToOrigin = -1;
// 			float bestDotProduct = 0;
// 			Vec2 edgeNormal;
// 			int edgeIndex;
// 			for(int vIndex = 0; vIndex < sCount; ++vIndex) {
// 				int indexPlusOne = vIndex + 1;
// 				if(vIndex == sCount - 1) indexPlusOne = 0;
// 				Vec2 p1 = simplex[vIndex];
// 				Vec2 p2 = simplex[indexPlusOne];

// 				// Vec2 normal = lineNormal(p1, p2);
// 				// float dot = searchDirection*normal;
// 				// if(dot > bestDotProduct) {
// 				// 	bestDotProduct = dot;
// 				// 	edgeNormal = normal;
// 				// 	edgeIndex = vIndex+1;
// 				// 	distanceToOrigin = distancePointLineX(p1,p2,origin,true);
// 				// }

// 				Vec2 intersection = {};
// 				if(getLineIntersection(p1, p2, origin, origin+searchDirection*searchDistance, &intersection)) {
// 					edgeNormal = lineNormal(p1,p2);
// 					edgeIndex = vIndex+1;
// 					distanceToOrigin = distancePointLineX(p1,p2,origin,true);
// 					break;
// 				}
// 			}

// 			Vec2 newP = support(s1, pos1, s2, pos2, edgeNormal, sweepingEnabled, sweepingDirection);

// 			float d = dot(newP,edgeNormal);
// 			if(d - distanceToOrigin < tolerance) {
// 				penetration = edgeNormal*d*-1;				

// 				Vec2 intersection = {};
// 				for(int i = 0; i < sCount; i++) {
// 					int indexPlusOne = i + 1;
// 					if(i == sCount - 1) indexPlusOne = 0;
// 					Vec2 p1 = simplex[i];
// 					Vec2 p2 = simplex[indexPlusOne];

// 					bool intersected = getLineIntersection(p1, p2, origin, searchDirection*1000, &intersection);
// 					if(intersected) break;
// 				}

// 				penetration = intersection*-1;

// 				break;
// 			} else {
// 				// Insert in between points
// 				for(int i = sCount; i > edgeIndex; --i) simplex[i] = simplex[i-1];
// 				sCount++;
// 				simplex[edgeIndex] = newP;

// 				assert(sCount < arrayCount(simplex));
// 			}
// 		}
// 	}

// 	Vec2 correctionOffset = norm(penetration)*offset;
// 	penetration += correctionOffset;

// 	// if(visualisation) {
// 	// 	for(int i = 0; i < sCount; i++) {
// 	// 		int indexPlusOne = i + 1;
// 	// 		if(i == sCount - 1) indexPlusOne = 0;
// 	// 		Vec2 p1 = simplex[i];
// 	// 		Vec2 p2 = simplex[indexPlusOne];

// 	// 			drawLine(drawCallBuffer, originOffset+p1, originOffset+p2, mapRGBA(1,0,0,1));
// 	// 	}

// 	// 	Vec2 finalOffset = penetration*-1;
// 	// 	drawLine(drawCallBuffer, originOffset, originOffset+finalOffset, COLOR_MAGENTA);
// 	// }

// 	return penetration;
// }
