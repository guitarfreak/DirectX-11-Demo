
#define ShaderVarsStruct(name) \
struct name {\
	float4x4 viewProj;\
};\

ShaderVarsStruct(CubeShaderVars)



char* d3dCubeShader = HLSL2 (
STRINGIFY_MACRO(ShaderVarsStruct(ShaderVars)),

struct VSInput {
	float3 pos : POSITION;
};

struct PSInput {
	float4 pos      : SV_POSITION;
	float3 position : POSITION;
};

TextureCube  tex  : register(t0);
SamplerState samp : register(s0);
ShaderVars   vars : register(b0);

PSInput vertexShader(VSInput input) {
	PSInput output;

	output.pos = mul(float4(input.pos, 1), vars.viewProj);

	float3 p = input.pos;
	p.x *= -1;
	output.position = p;

	return output;
}

float4 pixelShader(PSInput input) : SV_Target {
	float4 output;

	float3 direction = normalize(input.position);
	output = tex.Sample(samp, direction);

	return output;
}

);
