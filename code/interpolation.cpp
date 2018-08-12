
enum {
	EASE_Linear,
	EASE_InQuad,
	EASE_OutQuad,
	EASE_InOutQuad,
	EASE_InCubic,
	EASE_OutCubic,
	EASE_InOutCubic,
	EASE_InQuart,
	EASE_OutQuart,
	EASE_InOutQuart,
	EASE_InQuint,
	EASE_OutQuint,
	EASE_InOutQuint,
	EASE_InSine,
	EASE_OutSine,
	EASE_InOutSine,
	EASE_InExpo,
	EASE_OutExpo,
	EASE_InOutExpo,
	EASE_InCirc,
	EASE_OutCirc,
	EASE_InOutCirc,
};

float easeFunction(int type, float t, float b, float c, float d) {
	switch(type) {
		case EASE_Linear: 
			return c*t / d + b;

		case EASE_InQuad: 
			t /= d;
			return c*t*t + b;

		case EASE_OutQuad: 
			t /= d;
			return -c * t*(t - 2) + b;

		case EASE_InOutQuad: 
			t /= d / 2;
			if (t < 1) return c / 2 * t*t + b;
			t--;
			return -c / 2 * (t*(t - 2) - 1) + b;

		case EASE_InCubic: 
			t /= d;
			return c*t*t*t + b;

		case EASE_OutCubic: 
			t /= d;
			t--;
			return c*(t*t*t + 1) + b;

		case EASE_InOutCubic: 
			t /= d / 2;
			if (t < 1) return c / 2 * t*t*t + b;
			t -= 2;
			return c / 2 * (t*t*t + 2) + b;

		case EASE_InQuart: 
			t /= d;
			return c*t*t*t*t + b;

		case EASE_OutQuart: 
			t /= d;
			t--;
			return -c * (t*t*t*t - 1) + b;

		case EASE_InOutQuart: 
			t /= d / 2;
			if (t < 1) return c / 2 * t*t*t*t + b;
			t -= 2;
			return -c / 2 * (t*t*t*t - 2) + b;

		case EASE_InQuint: 
			t /= d;
			return c*t*t*t*t*t + b;

		case EASE_OutQuint: 
			t /= d;
			t--;
			return c*(t*t*t*t*t + 1) + b;

		case EASE_InOutQuint: 
			t /= d / 2;
			if (t < 1) return c / 2 * t*t*t*t*t + b;
			t -= 2;
			return c / 2 * (t*t*t*t*t + 2) + b;

		case EASE_InSine: 
			return -c * cos(t / d * M_PI_2) + c + b;

		case EASE_OutSine: 
			return c * sin(t / d * M_PI_2) + b;

		case EASE_InOutSine: 
			return -c / 2 * (cos(M_PI*t / d) - 1) + b;

		case EASE_InExpo: 
			return c * pow(2, 10 * (t / d - 1)) + b;

		case EASE_OutExpo: 
			return c * (-pow(2, -10 * t / d) + 1) + b;

		case EASE_InOutExpo: 
			t /= d / 2;
			if (t < 1) return c / 2 * pow(2, 10 * (t - 1)) + b;
			t--;
			return c / 2 * (-pow(2, -10 * t) + 2) + b;

		case EASE_InCirc: 
			t /= d;
			return -c * (sqrt(1 - t*t) - 1) + b;

		case EASE_OutCirc: 
			t /= d;
			t--;
			return c * sqrt(1 - t*t) + b;

		case EASE_InOutCirc: 
			t /= d / 2;
			if (t < 1) return -c / 2 * (sqrt(1 - t*t) - 1) + b;
			t -= 2;
			return c / 2 * (sqrt(1 - t*t) + 1) + b;
	}

	return 0;
}

//

float cubicBezierInterpolationSeemless(float A, float B, float CC, float D, float t) {
	float a = -A/2.0f + (3.0f*B)/2.0f - (3.0f*CC)/2.0f + D/2.0f;
	float b = A - (5.0f*B)/2.0f + 2.0f*CC - D / 2.0f;
	float c = -A/2.0f + CC/2.0f;
	float d = B;
	
	return a*t*t*t + b*t*t + c*t + d;
}

inline Vec2 quadraticBezierInterpolation(Vec2 p0, Vec2 p1, Vec2 p2, float t) {
	Vec2 pa = lerp(t, p0, p1);
	Vec2 pb = lerp(t, p1, p2);

	Vec2 v = lerp(t, pa, pb);
	return v;
}

inline Vec2 cubicBezierInterpolation(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t) {
	Vec2 pa = quadraticBezierInterpolation(p0, p1, p2, t);
	Vec2 pb = quadraticBezierInterpolation(p1, p2, p3, t);

	Vec2 v = lerp(t, pa, pb);
	return v;
}

inline Vec2 cubicBezierInterpolationSeemless(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t) {
	Vec2 v;
	v.x = cubicBezierInterpolationSeemless(p0.x, p1.x, p2.x, p3.x, t);
	v.y = cubicBezierInterpolationSeemless(p0.y, p1.y, p2.y, p3.y, t);
	return v;
}

inline float cubicBezierGuessLength(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3) {
	float length = lenLine(p0,p1) + lenLine(p1,p2) + lenLine(p2,p3) + lenLine(p3,p0);
	length = length / 2;
	return length;
}

void cubicBezierTesselate(Vec2* points, int* pointsCount, Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float tolerance, int step = 0) {

	if(step == 0) *pointsCount = 0;

	if(step > 10) return;

	float d = distancePointLine(p0, p3, p1) + distancePointLine(p0, p3, p2);
	bool lineFlat = d < tolerance * lenLine(p0, p3);
	if(!lineFlat) {
		Vec2 p01 = lerp(0.5f, p0, p1);
		Vec2 p12 = lerp(0.5f, p1, p2);
		Vec2 p23 = lerp(0.5f, p2, p3);
		Vec2 p012 = lerp(0.5f, p01, p12);
		Vec2 p123 = lerp(0.5f, p12, p23);
		Vec2 p0123 = lerp(0.5f, p012, p123);

		if(step == 0) {
			if(points) points[(*pointsCount)++] = p0;
			else (*pointsCount)++;
		}

		cubicBezierTesselate(points, pointsCount, p0, p01, p012, p0123, tolerance, step+1);

		if(points) points[(*pointsCount)++] = p0123;
		else (*pointsCount)++;

		cubicBezierTesselate(points, pointsCount, p0123, p123, p23, p3, tolerance, step+1);

		if(step == 0) {
			if(points) points[(*pointsCount)++] = p3;
			else (*pointsCount)++;
		}

	} else {
		if(step == 0) {
			if(points) points[(*pointsCount)++] = p0;
			else (*pointsCount)++;
		}

		if(step == 0) {
			if(points) points[(*pointsCount)++] = p3;
			else (*pointsCount)++;
		}
	}
}