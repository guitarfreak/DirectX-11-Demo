
// http://codeflow.org/entries/2011/apr/13/advanced-webgl-part-2-sky-rendering/

// Ugh.
Meta_Parse_Struct(0);
struct SkySettings {
	Vec2  sunAngles; // @V0 @Tag(Range, 0, 360, -90, 90) 
	Quat  sunRot; // @Ignore
	float spotBrightness; // @V0
	float mieBrightness; // @V0
	float mieDistribution; // @V0
	float mieStrength; // @V0
	float mieCollectionPower; // @V0
	float rayleighBrightness; // @V0
	float rayleighStrength; // @V0
	float rayleighCollectionPower; // @V0
	float scatterStrength; // @V0
	float surfaceHeight; // @V0 @Tag(Range, 0,1) 
	float intensity; // @V0
	int   stepCount; // @V0
	float horizonOffset; // @V0 @Tag(Range, 0,1) 
	float sunOffset; // @V0 @Tag(Range, 0,1)
};

#define ShaderVarsStruct(name) \
struct name {\
	float4x4 viewInv;\
	float4x4 projInv;\
\
	float3 sunDir;\
\
	float spotBrightness;\
	float mieBrightness;\
	float mieDistribution;\
	float mieStrength;\
	float mieCollectionPower;\
	float rayleighBrightness;\
	float rayleighStrength;\
	float rayleighCollectionPower;\
	float scatterStrength;\
	float surfaceHeight;\
	float intensity;\
	int   stepCount;\
	float horizonOffset;\
	float sunOffset;\
\
	float3 padding;\
};\

ShaderVarsStruct(SkyShaderVars)



char* d3dSkyShader = HLSL2 (
STRINGIFY_MACRO(ShaderVarsStruct(ShaderVars)),

struct VSInput {
	uint id : SV_VertexID;
};

struct PSInput {
	float4 pos : SV_POSITION;
	float4 p : POSITION;
};

ShaderVars vars : register(b0);

PSInput vertexShader(VSInput input) {
	PSInput output;

	float2 verts[4];
	verts[0] = float2( -1, -1 );
	verts[1] = float2( -1,  1 );
	verts[2] = float2(  1, -1 );
	verts[3] = float2(  1,  1 );

	output.pos = float4(verts[input.id], 0, 1);
	output.p = output.pos;

	return output;
}

float phase(float alpha, float g){
    float a = 3.0*(1.0-g*g);
    float b = 2.0*(2.0+g*g);
    float c = 1.0+alpha*alpha;
    float d = pow(1.0+g*g-2.0*g*alpha, 1.5);
    return (a/b)*(c/d);
}

float atmospheric_depth(float3 position, float3 dir){
    float a = dot(dir, dir);
    float b = 2.0*dot(dir, position);
    float c = dot(position, position)-1.0;
    float det = b*b-4.0*a*c;
    float detSqrt = sqrt(det);
    float q = (-b - detSqrt)/2.0;
    float t1 = c/q;
    return t1;
}

float horizon_extinction(float3 position, float3 dir, float radius){
    float u = dot(dir, -position);
    if(u<0.0){
        return 1.0;
    }
    float3 near = position + u*dir;
    if(length(near) < radius){
        return 0.0;
    }
    else{
        float3 v2 = normalize(near)*radius - position;
        float diff = acos(dot(normalize(v2), dir));

        // @TODO: Some wrong pixels when using smoothstep with 0.0f as min.
        return smoothstep(0.00001f, 1.0, pow(diff*2.0, 3.0));
        // return smoothstep(0.0, 1.0, pow(diff*1.0, 3.0));
    }
}

static float3 Kr = float3(
    0.18867780436772762, 0.4978442963618773, 0.6616065586417131
);

float3 absorb(float dist, float3 color, float factor){
	float3 test = pow(Kr, float3(factor/dist, factor/dist, factor/dist));

	return color-color*test;
}

float4 pixelShader(PSInput input) : SV_Target {
	float2 p = input.p.xy;
	// float4 p = SV_Position;

	float4 deviceNormal = float4(p, 0.0, 1.0);
	float3 normal = normalize(mul(deviceNormal, vars.projInv)).xyz; 
	normal = normalize(mul(float4(normal,0), vars.viewInv));

	float3 n = normal;

	n = (n + 1) / 2;

	// float3 lightdir = normalize(float3(0.25f,0,1));
	float3 lightdir = vars.sunDir;

	float spot_brightness           = vars.spotBrightness;

	float mie_brightness            = vars.mieBrightness;
	float mie_distribution          = vars.mieDistribution;
	float mie_strength              = vars.mieStrength;
	float mie_collection_power      = vars.mieCollectionPower;

	float rayleigh_brightness       = vars.rayleighBrightness;
	float rayleigh_strength         = vars.rayleighStrength;
	float rayleigh_collection_power = vars.rayleighCollectionPower;

	float scatter_strength          = vars.scatterStrength;

	//

	float surface_height = vars.surfaceHeight;
	float intensity = vars.intensity;
	int step_count = vars.stepCount;

	float horizonOffset = vars.horizonOffset;
	float sunOffset = vars.sunOffset;

	//

	float3 eyedir = normal;
	float alpha = dot(eyedir, lightdir);

	float rayleigh_factor = phase(alpha, -0.01)*rayleigh_brightness;
	float mie_factor = phase(alpha, mie_distribution)*mie_brightness;
	float spot = smoothstep(0.0, 15.0, phase(alpha, 0.9995))*spot_brightness;

	float3 eye_position = float3(0.0, 0, surface_height);
	float eye_depth = atmospheric_depth(eye_position, eyedir);
	float step_length = eye_depth / float(step_count);

	float eye_extinction = horizon_extinction(eye_position, eyedir, surface_height-horizonOffset);

	float3 rayleigh_collected = float3(0.0, 0.0, 0.0);
	float3 mie_collected = float3(0.0, 0.0, 0.0);

	for(int i=0; i<step_count; i++){
	    float sample_distance = step_length*float(i);
	    float3 position = eye_position + eyedir*sample_distance;
	    float extinction = horizon_extinction(position, lightdir, surface_height-sunOffset);
	    float sample_depth = atmospheric_depth(position, lightdir);

	    //

		float3 influx = absorb(sample_depth, float3(intensity, intensity, intensity), scatter_strength)*extinction;

		rayleigh_collected += absorb(sample_distance, Kr*influx, rayleigh_strength);
		mie_collected += absorb(sample_distance, influx, mie_strength);
	}

	rayleigh_collected = (rayleigh_collected * eye_extinction * pow(eye_depth, rayleigh_collection_power)) / float(step_count);

	mie_collected = (mie_collected * eye_extinction * pow(eye_depth, mie_collection_power)) / float(step_count);

	float3 color = float3(spot*mie_collected + mie_factor*mie_collected + rayleigh_factor*rayleigh_collected);

	float4 fcolor = float4(pow(color,2.2f), 1);
	fcolor = saturate(fcolor);

	return fcolor;
}

);
