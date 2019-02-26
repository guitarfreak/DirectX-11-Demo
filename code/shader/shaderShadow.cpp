#define ShaderVarsStruct(name) \
struct name {\
	float4x4 model;\
	float4x4 viewProj;\
\
	float3 camPos;\
	int sharpenAlpha;\
\
	float farTessDistance;\
	float closeTessDistance;\
	float farTessFactor;\
	float closeTessFactor;\
\
	int hasDispMap;\
	float heightScale;\
\
	int boneCount;\
	float _pad;\
	float4x4 boneMatrices[100];\
};\

ShaderVarsStruct(ShadowShaderVars)



char* d3dShadowShader = HLSL2 (

STRINGIFY_MACRO(ShaderVarsStruct(ShaderVars)),



struct VSInput {
	float3 pos    : POSITION;
	float2 uv     : TEXCOORD;
	float3 normal : NORMAL0;

	float3 tangent   : NORMAL1;
	float3 bitangent : NORMAL2;
	float4 blendWeights : BLENDWEIGHT;
	int4   blendIndices : BLENDINDICES;
};

struct VSOutput {
	float4 svPos : SV_POSITION;
	float3 pos   : POSITION;
	float2 uv    : TEXCOORD0;

	float3 normal    : NORMAL0;

	float tessFactor : TESS;
};

ShaderVars   vars : register(b0);
Texture2D    tex  : register(t0);
Texture2D    height : register(t4);
SamplerState samp : register(s0);

VSOutput vertexShader(VSInput input) {
	VSOutput output;

	float4 pos;
	float3 normal;

	if(vars.boneCount) {
		for(int i = 0; i < vars.boneCount; i++) {
			float weight = input.blendWeights[i];
			int index = input.blendIndices[i];

			pos    += weight * mul(float4(input.pos, 1.0f), vars.boneMatrices[index]);
			normal += weight * mul(input.normal, vars.boneMatrices[index]);
		}

	} else {
		pos = float4(input.pos, 1);
		normal = input.normal;
	}

	output.pos = mul(float4(pos.xyz, 1), vars.model);
	output.uv = input.uv;
	output.normal = mul(normal, vars.model);

   // Copied from main shader.
	if(vars.hasDispMap) {
		float d = distance(output.pos, vars.camPos);
		float tess = saturate((vars.farTessDistance - d) / (vars.farTessDistance - vars.closeTessDistance));
		output.tessFactor = vars.farTessFactor + tess*(vars.closeTessFactor - vars.farTessFactor);

	} else {
		output.svPos = mul(float4(output.pos, 1.0f), vars.viewProj);
	}

	return output;
}

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

	output.pos    = p[i].pos;
	output.uv     = p[i].uv;
	output.normal = p[i].normal;

	return output;
}

[domain("tri")]
VSOutput domainShader(PatchTess patchTess, float3 bary : SV_DomainLocation, const OutputPatch<VSOutput,3> tri) {
	VSOutput output;

	output.pos    = bary.x*tri[0].pos    + bary.y*tri[1].pos    + bary.z*tri[2].pos;
	output.uv     = bary.x*tri[0].uv     + bary.y*tri[1].uv     + bary.z*tri[2].uv;
	output.normal = bary.x*tri[0].normal + bary.y*tri[1].normal + bary.z*tri[2].normal;

	output.normal = normalize(output.normal);

	{
		const float MipInterval = 10.0f;
		float mipLevel = clamp((distance(output.pos, vars.camPos) - MipInterval) / MipInterval, 0.0f, 8.0f);
		float h = height.SampleLevel(samp, output.uv, mipLevel).r - 0.5f;

		output.pos += output.normal * h * vars.heightScale;
	}

	output.svPos = mul(float4(output.pos, 1.0f), vars.viewProj);

	return output;
}

void pixelShader(VSOutput input) {
   float4 output = tex.Sample(samp, input.uv);

   // Copied from main shader.
   if(vars.sharpenAlpha) {
   	output.a = (output.a - 0.25f) / max(fwidth(output.a), 0.0001) + 0.5;
   }

	clip(output.a <= 0.5f ? -1:1);
}

);
