
// __declspec(align(16)) 
// #pragma pack(push,16)

#define ExtraVarsStruct() \
struct Light {            \
	int type;              \
	float3 dir;            \
	float3 color;          \
	float _pad0;           \
};                        \

#define ShaderVarsStruct(name)  \
struct MVPMatrix {              \
	float4x4 model;              \
	float4x4 viewProj;           \
};                              \
                                \
struct ShaderMaterial {         \
	int smoothing;               \
	float3 Ka;                   \
	float3 Kd;                   \
	float _pad0;                 \
	float3 Ks;                   \
	float Ns;                    \
                                \
	float3 Ke;                   \
	float Ni;                    \
	float d;                     \
	int illum;                   \
	int hasBumpMap;              \
                                \
	int   hasDispMap;            \
	float heightScale;           \
                                \
	float3 _pad1;                \
};                              \
                                \
struct ShaderSamples {          \
	float4 data[16];             \
	int count;                   \
                                \
	float3 padding;              \
};                              \
                                \
struct name {                   \
	MVPMatrix mvp;               \
	float4 color;                \
	float3 camPos;               \
	float _pad0;                 \
                                \
	float3 ambient;              \
	float _pad1;                 \
	Light light;                 \
	bool disableLighting;        \
	int lightCount;              \
	float2 shadowMapSize;        \
                                \
	ShaderMaterial material;     \
                                \
	MVPMatrix mvpShadow;         \
	int sharpenAlpha;            \
                                \
	float farTessDistance;       \
	float closeTessDistance;     \
	float farTessFactor;         \
	float closeTessFactor;       \
                                \
	int boneCount;               \
	float2 _pad2;                \
                                \
	float4x4 boneMatrices[100];  \
};                              \

ExtraVarsStruct()
ShaderVarsStruct(MainShaderVars)



char* d3dMainShader = HLSL4 (
STRINGIFY_MACRO(ExtraVarsStruct()),
STRINGIFY_MACRO(ShaderVarsStruct(ShaderVars)),

"static float2 samples[] = {"
"	{0.410875, -0.504685},"
"	{0.153835, -0.060955},"
"	{0.172805, -0.847635},"
"	{0.778885, -0.081865},"
"	{-0.063715, -0.477125},"
"	{-0.591845, -0.496635},"
"	{-0.443255, 0.005445},"
"	{-0.786705, -0.211425},"
"	{-0.448325, 0.364535},"
"	{0.027865, 0.299825},"
"	{-0.173285, 0.719705},"
"	{-0.747335, 0.200035},"
"	{0.560205, 0.704155},"
"	{0.170125, 0.730885},"
"	{0.433185, 0.316815},"
"	{0.913445, 0.267335},"
"};",

Texture2D              ambi   : register(t0);
Texture2D              tex    : register(t1);
Texture2D              bump   : register(t2);
Texture2D              rough  : register(t3);
Texture2D              height : register(t4);
Texture2D              pTex   : register(t5);
ShaderVars             vars   : register(b0);
SamplerState           samp   : register(s0);
SamplerComparisonState sampP  : register(s1);

struct VSInput {
	float3 pos       : POSITION;
	float2 uv        : TEXCOORD;
	float3 normal    : NORMAL0;
	float3 tangent   : NORMAL1;
	float3 bitangent : NORMAL2;
	float4 blendWeights : BLENDWEIGHT;
	int4   blendIndices : BLENDINDICES;
};

// We use VSOutput for every stage to make things easier for us.
// The only things that would change between shader stage output 
// structs is SV_POSITION and TESS.

struct VSOutput {
	float4 svPos : SV_POSITION;

	float3 pos : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;

	float3 normal : NORMAL0;
	float3 tangent : NORMAL1;
	float3 bitangent : NORMAL2;

	float3 look : NORMAL3;
	float4 shadowPos : POSITION1;

	float tessFactor : TESS;
};

VSOutput vertexShader(VSInput input) {
	VSOutput output;

	float4 pos;
	float3 normal;
	float3 tangent;
	float3 bitangent;

	if(vars.boneCount) {
		for(int i = 0; i < 4; i++) {
			float weight = input.blendWeights[i];
			if(weight == 0.0f) break;
			
			int index = input.blendIndices[i];

			pos       += weight * mul(float4(input.pos, 1.0f), vars.boneMatrices[index]);
			normal    += weight * mul(input.normal, vars.boneMatrices[index]);
			tangent   += weight * mul(input.tangent, vars.boneMatrices[index]);
			bitangent += weight * mul(input.bitangent, vars.boneMatrices[index]);
		}

	} else {
		pos = float4(input.pos, 1);
		normal = input.normal;
		tangent = input.tangent;
		bitangent = input.bitangent;
	}

	output.pos = mul(float4(pos.xyz, 1), vars.mvp.model);

	float4 color = float4(pow(vars.color.rgb, 2.2f), vars.color.a);
	output.color = color;
	output.color.a *= vars.material.d;
	output.uv = input.uv;

	output.normal = mul(normal, vars.mvp.model);
	output.look = vars.camPos - output.pos.xyz;

	if(vars.material.hasBumpMap) {
		output.tangent   = mul(tangent, vars.mvp.model);
		output.bitangent = mul(bitangent, vars.mvp.model);
	}

	if(vars.material.hasDispMap) {
		float d = distance(output.pos, vars.camPos);
		float tess = saturate((vars.farTessDistance - d) / (vars.farTessDistance - vars.closeTessDistance));
		output.tessFactor = vars.farTessFactor + tess*(vars.closeTessFactor - vars.farTessFactor);

	} else {
		output.svPos     = mul(float4(output.pos, 1.0f), vars.mvp.viewProj);
		output.shadowPos = mul(float4(output.pos, 1.0f), vars.mvpShadow.viewProj);
	}

	return output;
}

// http://www.richardssoftware.net/2013/09/bump-and-displacement-mapping-with.html

struct PatchTess {
	float EdgeTess[3] : SV_TessFactor;
	float InsideTess  : SV_InsideTessFactor;
};

PatchTess PatchHS(InputPatch<VSOutput,3> patch, uint patchID : SV_PrimitiveID) {
	PatchTess pt;

	pt.EdgeTess[0] = 0.5f*(patch[1].tessFactor + patch[2].tessFactor);
	pt.EdgeTess[1] = 0.5f*(patch[2].tessFactor + patch[0].tessFactor);
	pt.EdgeTess[2] = 0.5f*(patch[0].tessFactor + patch[1].tessFactor);
	pt.InsideTess  = pt.EdgeTess[0];

	return pt;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchHS")]
VSOutput hullShader(InputPatch<VSOutput,3> p, uint i : SV_OutputControlPointID, uint patchId : SV_PrimitiveID) {
	VSOutput output;

	output.pos       = p[i].pos;
	output.color     = p[i].color;
	output.uv        = p[i].uv;
	output.normal    = p[i].normal;
	output.tangent   = p[i].tangent;
	output.bitangent = p[i].bitangent;
	output.look      = p[i].look;
	output.shadowPos = p[i].shadowPos;

	return output;
}

[domain("tri")]
VSOutput domainShader(PatchTess patchTess, float3 bary : SV_DomainLocation, const OutputPatch<VSOutput,3> tri) {
	VSOutput output;

	output.pos       = bary.x*tri[0].pos       + bary.y*tri[1].pos       + bary.z*tri[2].pos;
	output.color     = bary.x*tri[0].color     + bary.y*tri[1].color     + bary.z*tri[2].color;
	output.uv        = bary.x*tri[0].uv        + bary.y*tri[1].uv        + bary.z*tri[2].uv;
	output.normal    = bary.x*tri[0].normal    + bary.y*tri[1].normal    + bary.z*tri[2].normal;
	output.tangent   = bary.x*tri[0].tangent   + bary.y*tri[1].tangent   + bary.z*tri[2].tangent;
	output.bitangent = bary.x*tri[0].bitangent + bary.y*tri[1].bitangent + bary.z*tri[2].bitangent;
	output.look      = bary.x*tri[0].look      + bary.y*tri[1].look      + bary.z*tri[2].look;
	output.shadowPos = bary.x*tri[0].shadowPos + bary.y*tri[1].shadowPos + bary.z*tri[2].shadowPos;

	output.normal    = normalize(output.normal);
	output.tangent   = normalize(output.tangent);
	output.bitangent = normalize(output.bitangent);

	{
		// uint mipLevels;
		// {
		// 	uint w;
		// 	uint h;
		// 	height.GetDimensions(0, w, h, mipLevels);
		// }

		const float MipInterval = 10.0f;
		float mipLevel = clamp((distance(output.pos, vars.camPos) - MipInterval) / MipInterval, 0.0f, 8.0f);
		float h = height.SampleLevel(samp, output.uv, mipLevel).r - 0.5f;

		output.pos += output.normal * h * vars.material.heightScale;
	}

	output.svPos     = mul(float4(output.pos, 1.0f), vars.mvp.viewProj);
	output.shadowPos = mul(float4(output.pos, 1.0f), vars.mvpShadow.viewProj);

	return output;
}

float SampleShadowMap(in float2 base_uv, in float u, in float v, in float2 shadowMapSizeInv, in uint cascadeIdx, in float depth, in float2 receiverPlaneDepthBias) {

    float2 uv = base_uv + float2(u, v) * shadowMapSizeInv;

     // float z = depth + dot(float2(u, v) * shadowMapSizeInv, receiverPlaneDepthBias);
     float z = depth;

    return pTex.SampleCmpLevelZero(sampP, uv, z);
}

float SampleShadowMapOptimizedPCF(float3 shadowPos, float3 shadowPosDX, float3 shadowPosDY, uint cascadeIdx, float2 receiverPlaneDepthBias) {
	float2 shadowMapSize = vars.shadowMapSize;

	float lightDepth = shadowPos.z;

	float2 uv = shadowPos.xy * shadowMapSize; // 1 unit - 1 texel

	float2 shadowMapSizeInv = 1.0 / shadowMapSize;

	float2 base_uv;
	base_uv.x = floor(uv.x + 0.5);
	base_uv.y = floor(uv.y + 0.5);

	float s = (uv.x + 0.5 - base_uv.x);
	float t = (uv.y + 0.5 - base_uv.y);

	base_uv -= float2(0.5, 0.5);
	base_uv *= shadowMapSizeInv;

	
	float uw0 = (4 - 3 * s);
	float uw1 = 7;
	float uw2 = (1 + 3 * s);

	float u0 = (3 - 2 * s) / uw0 - 2;
	float u1 = (3 + s) / uw1;
	float u2 = s / uw2 + 2;

	float vw0 = (4 - 3 * t);
	float vw1 = 7;
	float vw2 = (1 + 3 * t);

	float v0 = (3 - 2 * t) / vw0 - 2;
	float v1 = (3 + t) / vw1;
	float v2 = t / vw2 + 2;


	float sum = 0;
	sum += uw0 * vw0 * SampleShadowMap(base_uv, u0, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
	sum += uw1 * vw0 * SampleShadowMap(base_uv, u1, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
	sum += uw2 * vw0 * SampleShadowMap(base_uv, u2, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

	sum += uw0 * vw1 * SampleShadowMap(base_uv, u0, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
	sum += uw1 * vw1 * SampleShadowMap(base_uv, u1, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
	sum += uw2 * vw1 * SampleShadowMap(base_uv, u2, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

	sum += uw0 * vw2 * SampleShadowMap(base_uv, u0, v2, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
	sum += uw1 * vw2 * SampleShadowMap(base_uv, u1, v2, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
	sum += uw2 * vw2 * SampleShadowMap(base_uv, u2, v2, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

	return sum * 1.0f / 144;
}

float3 calculateLight(VSOutput input) {

	// Sample shadow.
	float shadowFactor;
	{
		float3 projCoord = input.shadowPos.xyz / input.shadowPos.w;

		if(projCoord.x < -1.0f || projCoord.x > 1.0f || 
		   projCoord.y < -1.0f || projCoord.y > 1.0f || 
		   projCoord.z <  0.0f || projCoord.z > 1.0f) {
			return vars.material.Ka * vars.ambient;
			// return float3(0,1,0);
		}

		projCoord.y *= -1;
		projCoord.xy = (projCoord.xy + 1) / 2.0f;

		// Calculate bias.

		// float bias = 0.00002f;
		float bias = 0.00005f;
		float receiverPlaneDepthBias;
		{
			// Constant, slope-scale, normal and receiver plane depth biases.

			float normalBiasScale;
			float slopeBiasScale;
			{
				{
					float3 N = normalize(input.normal);
					float3 L = -vars.light.dir;

					float cos_alpha = saturate(dot(N, L));
					normalBiasScale = sqrt(1 - cos_alpha*cos_alpha); // sin(acos(L·N))
					slopeBiasScale = normalBiasScale / cos_alpha;    // tan(acos(L·N))
					slopeBiasScale = min(2, slopeBiasScale);
				}

				{
					float3 texCoordDX = ddx(projCoord);
					float3 texCoordDY = ddy(projCoord);

					float2 biasUV;
					biasUV.x = texCoordDY.y * texCoordDX.z - texCoordDX.y * texCoordDY.z;
					biasUV.y = texCoordDX.x * texCoordDY.z - texCoordDY.x * texCoordDX.z;
					biasUV *= 1.0f / ((texCoordDX.x * texCoordDY.y) - (texCoordDX.y * texCoordDY.x));

					receiverPlaneDepthBias = biasUV;
				}
			}

			bias += 0.0005f * normalBiasScale;
			bias += 0.0005f * slopeBiasScale;

			// // Static depth biasing to make up for incorrect fractional sampling on the shadow map grid
			// float2 texelSize = 1.0f / vars.shadowMapSize;
			// float fractionalSamplingError = 2 * dot(float2(1.0f, 1.0f) * texelSize, abs(receiverPlaneDepthBias));

			// bias += min(fractionalSamplingError, 0.01f);
		}

		projCoord.z -= bias;

		shadowFactor = 0.0f;
		{
			// Single PCF.
			{
				// shadowFactor = pTex.SampleCmpLevelZero(sampP, projCoord.xy, projCoord.z);
			}

			// 4x4 PCF grid.
			{
				// for(float y = -1.5f; y <= 1.5f; y += 1.0f) {
				// 	for(float x = -1.5f; x <= 1.5f; x += 1.0f) {
				// 		float2 uvOffset = float2(x * 1/vars.shadowMapSize.x, y * 1/vars.shadowMapSize.y);
				// 		shadowFactor += pTex.SampleCmpLevelZero(sampP, projCoord.xy + uvOffset, projCoord.z);
				// 	}
				// }
				// shadowFactor /= 16.0f;
			}

			// Poisson disk 16 samples.
			{
				// for(int i = 0; i < 16; i++) {
				// 	// float2 sample = samples[i] * 1.5f;
				// 	float2 sample = samples[i] * 1.5f;
				// 	float2 uvOffset = sample * (1 / vars.shadowMapSize);

				// 	shadowFactor += pTex.SampleCmpLevelZero(sampP, projCoord.xy + uvOffset, projCoord.z);

				// }
				// shadowFactor /= 16.0f;
			}

			// The Witness sampling.
			{
				shadowFactor = SampleShadowMapOptimizedPCF(projCoord, 0, 0, 0, receiverPlaneDepthBias);
			}
		}

		if(shadowFactor == 0.0f) return vars.material.Ka * vars.ambient;
	}

	float3 lightDir = -vars.light.dir;
	float3 normal;

	if(vars.material.hasBumpMap) {
		// float3x3 tbn = float3x3(normalize(input.bitangent), 
		//                         normalize(input.tangent), 
		//                         normalize(input.normal));

		float3x3 tbn = float3x3(-normalize(input.tangent), 
		                        -normalize(input.bitangent), 
		                         normalize(input.normal));

		float3 bumpColor = bump.Sample(samp, input.uv).rgb;
		float3 bumpNormal = normalize((bumpColor*2)-1);

		normal = mul(bumpNormal, tbn);
		normal = normalize(normal);

	} else {
		normal = normalize(input.normal);
	}

	if(vars.sharpenAlpha) {
		if(dot(normal, input.look) < 0) normal *= -1;
	}

	float3 ambient = ambi.Sample(samp, input.uv).rgb;
	float3 roughness = rough.Sample(samp, input.uv).rgb;

	float3 half = normalize(lightDir + normalize(input.look));

	// Blinn-Phong.
	float3 lightIntensity;
	{
		ShaderMaterial m = vars.material;

		float3 Ia = m.Ka * ambient.rgb * vars.ambient;
		float3 Id = m.Kd * saturate(dot(normal, lightDir));
		float3 Is = m.Ks * roughness.rgb * pow(saturate(dot(normal, half)), m.Ns);

		lightIntensity = Ia + shadowFactor * ((Id + Is) * vars.light.color);
	}

	return lightIntensity;
}

float4 pixelShader(VSOutput input) : SV_Target {
	float4 output;

	float4 lighting;
	if(!vars.disableLighting) lighting = float4(calculateLight(input), 1);
	else lighting = float4(1,1,1,1);

	output = input.color * tex.Sample(samp, input.uv) * lighting;

	if(vars.sharpenAlpha) {
		output.a = (output.a - 0.25f) / max(fwidth(output.a), 0.0001) + 0.5;
		output.a = saturate(output.a);
	}

	output = saturate(output);

	return output;
}
);