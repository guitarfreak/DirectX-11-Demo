
#define ShaderVarsStruct(name) \
struct name {\
	float4x4 viewProj;\
	float4 color;\
	int gammaGradient;\
	\
	float3 _pad;\
};

ShaderVarsStruct(PrimitiveShaderVars)



char* d3dPrimitiveShader = HLSL2 (
STRINGIFY_MACRO(ShaderVarsStruct(ShaderVars)),

struct VSInput {
	float3 pos : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct PSInput {
	float4 svPos : SV_POSITION;
	float  zPos : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

Texture2D    tex  : register(t0);
Texture2D    tex2 : register(t1);
SamplerState samp : register(s0);
ShaderVars   vars : register(b0);

PSInput vertexShader(VSInput input) {
	PSInput output;

	output.svPos = mul(float4(input.pos, 1), vars.viewProj);
	output.zPos = output.svPos.z;

	if(!vars.gammaGradient) {
		output.color = float4(pow(input.color.rgb, 2.2f), input.color.a) * 
		               float4(pow(vars.color.rgb, 2.2f), vars.color.a);
	} else {
		output.color = input.color * vars.color;
	}

	output.uv = input.uv;

	return output;
}

float4 pixelShader(PSInput input) : SV_Target {
	float4 color = tex.Sample(samp, input.uv) * input.color;

	if(vars.gammaGradient) {
		color.rgb = pow(color.rgb, 2.2f);
	}

	return color;
}

);