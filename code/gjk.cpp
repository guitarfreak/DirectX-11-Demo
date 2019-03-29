
Vec2 furthestPointInDirection(Blocker* blocker, Vec2 pos, Vec2 dir, bool sweep = false, Vec2 sweepOffset = vec2(0,0)) {
	XForm form = blocker->xf;

	Vec2 result;
	switch(blocker->type) {
		case BLOCKERTYPE_RECT: {
			Vec2 dim = form.scale.xy/2.0f;
			Rect r = {-dim, dim};

			// TODO: Pre calc inverse here.
			dir = (inverse(form.rot) * vec3(dir,0)).xy;

			if(dir.y > 0) result = dir.x > 0 ? r.tr() : r.tl();
			else          result = dir.x > 0 ? r.br() : r.bl();

			result = (form.rot * vec3(result,0)).xy;
			result += pos;
		} break;

		case BLOCKERTYPE_CIRCLE: {
			// Assumes that dir will never be zero.
			Vec2 norm = dir/sqrt(dir.x*dir.x + dir.y*dir.y); 
			float radius = max(form.scale.x*0.5f, form.scale.y*0.5f);
			result = pos + norm*radius;
		} break;

		case BLOCKERTYPE_LINE: {
			// Vec2 dirPoint = blocker->linePoints[1].xy - blocker->linePoints[0].xy;
			result = dot(dir, blocker->ab.xy) > 0 ? blocker->linePoints[1].xy : blocker->linePoints[0].xy;
		} break;
	}

	if(sweep && (dot(sweepOffset,dir) >= 0)) result += sweepOffset;

	return result;
}

Vec2 support(Blocker* s1, Vec2 pos1, Blocker* s2, Vec2 pos2, Vec2 dir, bool sweep = false, Vec2 sweepOffset = vec2(0,0)) {
	Vec2 p1 = furthestPointInDirection(s1, pos1, dir, sweep, sweepOffset);
	Vec2 p2 = furthestPointInDirection(s2, pos2, dir*-1);
	return p1 - p2; 
}

bool containsOrigin(Vec2* simplex, int* sCount, Vec2* dir) {
	Vec2 a = simplex[*sCount-1];
	Vec2 ao = a*-1; // a to origin.

	if(*sCount == 3) {
		Vec2 b = simplex[1];
		Vec2 c = simplex[0];

		Vec2 ab = b-a;
		Vec2 ac = c-a;

		Vec2 abPerp = {ab.y, -ab.x};
		if(dot(abPerp, ac) > 0.0f) abPerp = {-ab.y, ab.x};

		if(dot(abPerp,ao) > 0.0f) {
			// remove c
			simplex[0] = b;
			simplex[1] = a;
			*sCount = *sCount - 1;
			*dir = abPerp;

		} else {
			Vec2 acPerp = {ac.y, -ac.x};
			if(dot(acPerp, ab) > 0.0f) acPerp = {-ac.y, ac.x};

			if(dot(acPerp, ao) > 0.0f) {
				// remove b
				simplex[1] = a;
				*sCount = *sCount - 1;
				*dir = acPerp;

			} else return true;
		}

	} else {
		Vec2 b = simplex[0];
		Vec2 ab = b-a;

		Vec2 perp = {ab.y, -ab.x};
		if(dot(perp, ao) < 0.0f) perp = {-ab.y, ab.x};

		*dir = perp;
	}

	return false;
}

bool gjk(Blocker* s1, Vec2 oldPos, Blocker* s2, Vec2* inputSimplex = 0, bool sweepingEnabled = false) {
	Vec2 pos1 = s1->xf.trans.xy;	
	Vec2 pos2 = s2->xf.trans.xy;	

	// Vec2 sweepingDirection = (pos1 - oldPos)*-1;
	// if(pos1 - oldPos == vec2(0,0)) sweepingEnabled = false;

	Vec2 simplex[3];
	int sCount = 0;
	Vec2 dir = pos2 - pos1;
	if(dir.x == 0 && dir.y == 0) dir = vec2(1,0);
	// simplex[sCount] = support(s1, pos1, s2, pos2, dir, sweepingEnabled, sweepingDirection);
	simplex[sCount++] = support(s1, pos1, s2, pos2, dir);
	dir *= -1;

	bool collision;
	while(true) {
		// simplex[sCount] = support(s1, pos1, s2, pos2, dir, sweepingEnabled, sweepingDirection);
		simplex[sCount++] = support(s1, pos1, s2, pos2, dir);

		if(dot(simplex[sCount-1], dir) <= 0.0f) {
			collision = false;
			break;

		} else if(containsOrigin(simplex, &sCount, &dir)) {
			collision = true;
			break;
		}
	}

	if(inputSimplex) copyArray(inputSimplex, simplex, Vec2, 3);

	return collision;
}

Vec2 epa(Blocker* s1, Vec2 oldPos, Blocker* s2, Vec2* inputSimplex, Vec2 searchDirection = vec2(0,0), bool sweepingEnabled = false) {
	Vec2 pos1 = s1->xf.trans.xy;	
	Vec2 pos2 = s2->xf.trans.xy;

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