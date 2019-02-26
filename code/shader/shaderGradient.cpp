
#define ShaderVarsStruct(name) \
struct name {\
	float4x4 view;\
	float4x4 proj;\
\
	float4 color0;\
	float4 color1;\
\
	float value;\
\
	float3 padding;\
};\

ShaderVarsStruct(GradientShaderVars)



char* d3dGradientShader = HLSL2 (
STRINGIFY_MACRO(ShaderVarsStruct(ShaderVars)),

struct VSInput {
	float3 pos   : POSITION;
	float4 color : COLOR;
	float2 uv    : TEXCOORD;
};

struct PSInput {
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

ShaderVars vars : register(b0);

PSInput vertexShader(VSInput input) {
	PSInput output;

	output.pos = mul(mul(float4(input.pos, 1), vars.view), vars.proj);
	output.uv = input.uv;

	return output;
}

float4 pixelShader(PSInput input) : SV_Target {
	float4 color;

	color = vars.color0 *    input.uv.y + 
	        vars.color1 * (1-input.uv.y);
	color.rgb = pow(color.rgb, vars.value);

	return color;
}

);
