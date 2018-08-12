
inline int colorFloatToInt(float color) { return round(color * 255); };
inline float colorIntToFloat(int color) { return (1.0f / 255) * color; };

Vec3 rgbToHsl(double r, double g, double b) {
	Vec3 color;

	double M = 0.0, m = 0.0, c = 0.0;
	M = max(r, g, b);
	m = min(r, g, b);
	c = M - m;
	color.z = 0.5 * (M + m);
	if (c != 0.0) {
	 	      if (M == r) color.x = modf(((g - b) / c), 6.0);
		else  if (M == g) color.x = ((b - r) / c) + 2.0;
		else /*if(M==b)*/ color.x = ((r - g) / c) + 4.0;

		color.x *= 60.0;
		color.y = c / (1.0 - fabs(2.0 * color.z - 1.0));

	} else {
		color.x = 0;
		color.y = 0;
		color.z = r;
	}

	return color;
}

Vec3 hslToRgb(double h, double s, double l) {
	Vec3 color;
	double c = 0.0, m = 0.0, x = 0.0;
	c = (1.0 - fabs(2 * l - 1.0)) * s;
	m = 1.0 * (l - 0.5 * c);
	x = c * (1.0 - fabs(modf(h / 60.0, 2) - 1.0));
	if (h == 360) h = 0;
	     if (h >= 0.0 && h < 60)  color = vec3(c + m, x + m, m);
	else if (h >=  60 && h < 120) color = vec3(x + m, c + m, m);
	else if (h >= 120 && h < 180) color = vec3(m, c + m, x + m);
	else if (h >= 180 && h < 240) color = vec3(m, x + m, c + m);
	else if (h >= 240 && h < 300) color = vec3(x + m, m, c + m);
	else if (h >= 300 && h < 360) color = vec3(c + m, m, x + m);
	else color = vec3(m, m, m);

	return color;
}

inline Vec3 rgbToHsl(Vec3 c) { return rgbToHsl(c.r, c.g, c.b); }
inline Vec3 hslToRgb(Vec3 c) { return hslToRgb(c.x, c.y, c.z); }

inline Vec3 hslToRgbf(Vec3 hsl) {
	hsl.x = modf(hsl.x, 1.0f);
	hsl.y = clamp01(hsl.y);
	hsl.z = clamp01(hsl.z);

	Vec3 c = hslToRgb(360 * hsl.x, hsl.y, hsl.z);
	return c;
}
inline Vec3 hslToRgbf(float h, float s, float l) { return hslToRgbf(vec3(h,s,l)); }
inline Vec4 hslToRgbf(Vec3 hsl, float a) { return vec4(hslToRgbf(hsl), a); }
inline Vec4 hslToRgbf(float h, float s, float l, float a) { return vec4(hslToRgbf(vec3(h,s,l)), a); }

inline Vec3 rgbToHslf(Vec3 rgb) {
	Vec3 hsl = rgbToHsl(rgb);
	Vec3 hslFloat = vec3(hsl.x / 360.0f, hsl.y, hsl.z);
	return hslFloat;
}

Vec3 linearToGamma(Vec3 color) {
	color.x = powf(color.x, 2.2f);
	color.y = powf(color.y, 2.2f);
	color.z = powf(color.z, 2.2f);
	return color;
}

Vec3 gammaToLinear(Vec3 color) {
	color.x = powf(color.x, 1/2.2f);
	color.y = powf(color.y, 1/2.2f);
	color.z = powf(color.z, 1/2.2f);
	return color;
}

Vec4 linearToGamma(Vec4 color) { return vec4(linearToGamma(color.rgb), color.a); }
Vec4 gammaToLinear(Vec4 color) { return vec4(gammaToLinear(color.rgb), color.a); }

// Taken from d3dx_dxgiformatconvert.inl.
inline float srgbToLinear(float val) {
    if( val < 0.04045f )
        val /= 12.92f;
    else
        val = pow((val + 0.055f)/1.055f,2.4f);
    return val;
}

inline float linearToSrgb(float val) { 
    if( val < 0.0031308f )
        val *= 12.92f;
    else
        val = 1.055f * pow(val,1.0f/2.4f) - 0.055f;
    return val;
}