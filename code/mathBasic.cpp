#pragma once

#include <math.h>

#define M_E         2.7182818284590452354               // e
#define M_LOG2E     1.4426950408889634074               // log_2 e
#define M_LOG10E    0.43429448190325182765              // log_10 e
#define M_LN2       0.69314718055994530942              // log_e 2
#define M_LN10      2.30258509299404568402              // log_e 10
#define M_2PI       6.2831853071795864769252867665590   // 2*pi
#define M_PI        3.1415926535897932384626433832795   // pi
#define M_3PI_2		4.7123889803846898576939650749193	// 3/2*pi
#define M_PI_2      1.5707963267948966192313216916398   // pi/2
#define M_PI_4      0.78539816339744830962              // pi/4
#define M_1_PI      0.31830988618379067154              // 1/pi
#define M_2_PI      0.63661977236758134308              // 2/pi
#define M_2_SQRTPI  1.12837916709551257390              // 2/sqrt(pi)
#define M_SQRT2     1.41421356237309504880              // sqrt(2)
#define M_SQRT1_2   0.70710678118654752440              // 1/sqrt(2)
#define M_PI_180    0.0174532925199432957692369076848   // pi/180
#define M_180_PI    57.295779513082320876798154814105   // 180/pi

inline int mod(int a, int b) {
	int result;
	result = a % b;
	if(result < 0) result += b;

	return result;
}

inline float modf(float val, float d) {
	float result = fmod(val, d);
	if(result < 0 && abs(result) < d) {
		result = d + result;
	}

	return result;
}

template <class T> inline void swap(T* a, T* b) {
	T temp = *a;
	*a = *b;
	*b = temp;
}

inline float diff(float a, float b) { return abs(a - b); }
inline float sameSign(float a, float b) { return (a < 0 && b < 0) || (a > 0 && b > 0); }

template <class T> inline T min(T a, T b) { return a <= b ? a : b; }
template <class T> inline T min(T a, T b, T c) { return min(min(a, b), min(b, c)); }
template <class T> inline T max(T a, T b) { return a >= b ? a : b; }
template <class T> inline T max(T a, T b, T c) { return max(max(a, b), max(b, c)); }

template <class T> inline T clampMin(T a, T mi) { return max(a, mi); };
template <class T> inline T clampMax(T a, T ma) { return min(a, ma); };
template <class T> inline T clamp(T a, T mi, T ma) { return min(max(a, mi), ma); };
template <class T> inline T clamp01(T a) { return clamp(a, (T)0, (T)1); };

template <class T> inline void clampMin(T* a, T mi) { *a = clampMin(*a, mi); };
template <class T> inline void clampMax(T* a, T ma) { *a = clampMax(*a, ma); };
template <class T> inline void clamp(T* a, T mi, T ma) { *a = clamp(*a, mi, ma); };
template <class T> inline void clamp01(T* a) { *a = clamp01(*a); };

float lerp(float percent, float min, float max) { return min + percent * (max-min); }

template <class T> inline T mapRange(T value, T min, T max, T rangeMin, T rangeMax) {
	T result = ((double)(value-min)/((max-min)-(min-min))) * (rangeMax-rangeMin) + rangeMin;
	return result;
};

template <class T> inline T mapRange01(T value, T min, T max) {
	T result = ((double)(value-min)/((max-min)-(min-min)));
	return result;
};

inline float mapRangeClamp(float value, float min, float max, float rangeMin, float rangeMax) {
	float result = mapRange(value, min, max, rangeMin, rangeMax);
	result = clamp(result, rangeMin, rangeMax);

	return result;
};

template <class T> inline bool between(T v, T min, T max) { return v >= min && v <= max; }

inline float radianToDegree(float angle) { return angle*(180.0f / M_PI); }
inline float degreeToRadian(float angle) { return angle*(M_PI / 180.0f); }

inline int sign(float x) { return x < 0 ? -1 : (x > 0 ? 1 : 0); }	

inline int roundInt(double i) { return floor(i + 0.5); }
inline int roundIntf(float i) { return floor(i + 0.5f); }

inline double roundUp(double i) { return ceil(i); }
inline float roundUpf(float i) { return ceil(i); }
inline double round(double i) { return floor(i + 0.5); }
inline float roundf(float i) { return floor(i + 0.5f); }
inline double roundDown(double i) { return floor(i); }
inline float roundDownf(float i) { return floor(i); }

inline double roundDigits(double f, int d) { return floor(f*(10*d) + 0.5) / (10*d); };
inline double roundMod(double i, double s) { return (round(i/s))*s; }

inline int roundMod(int i, int s) { return (i/s)*s; }
inline int roundModUp(int i, int s) { return ceil(i/(double)s)*s; }

inline double divSafe(double a, double b) { return b==0 ? 0 : a/b; }
inline int triangularNumber(int n) { return n*(n+1) / 2; }
inline float root(float a, float root) { return powf(a, 1/root); }
inline float logBase(float v, float base) { return log(v) / log(base); }
