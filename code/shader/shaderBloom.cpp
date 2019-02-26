
// Modes: 0-Filter, 1-HBlur, 2-VBlur.

#define ShaderVarsStruct(name) \
struct name {\
	float4x4 viewProj;\
\
	int mode;\
	float3 _pad;\
};\

ShaderVarsStruct(BloomShaderVars)

char* d3dBloomShader = HLSL3 (
STRINGIFY_MACRO(ShaderVarsStruct(ShaderVars)),

"static float weights[5] = {0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216};",

struct VSInput {
	float3 pos : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct PSInput {
	float4 svPos : SV_POSITION;
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
	output.color = float4(pow(input.color.rgb, 2.2f), input.color.a);
	output.uv = input.uv;

	return output;
}

float4 pixelShader(PSInput input) : SV_Target {
	float4 color = tex.Sample(samp, input.uv) * input.color;

	if(vars.mode == 0) {
		// float brightness = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
		float brightness = dot(color.rgb, float3(0.299, 0.587, 0.114));

		float filterValue = 1.1f;
		if(brightness <= filterValue) {
			color = float4(0.0, 0.0, 0.0, 1.0);
		} else {
			color.rgb *= brightness - filterValue;				
		}

	} else if(vars.mode == 1 || vars.mode == 2) {
		float2 texDim;
		tex.GetDimensions(texDim.x, texDim.y);

		float2 tex_offset = 1.0 / texDim; // gets size of single texel
		float3 result = tex.Sample(samp, input.uv).rgb * weights[0]; // current fragment's contribution

		bool horizontalBlur = vars.mode == 1;

		if(horizontalBlur) {
			for(int i = 1; i < 5; ++i) {
				result += tex.Sample(samp, input.uv + float2(tex_offset.x * i, 0.0)).rgb * weights[i];
				result += tex.Sample(samp, input.uv - float2(tex_offset.x * i, 0.0)).rgb * weights[i];
			}

		} else {
			for(int i = 1; i < 5; ++i) {
				result += tex.Sample(samp, input.uv + float2(0.0, tex_offset.y * i)).rgb * weights[i];
				result += tex.Sample(samp, input.uv - float2(0.0, tex_offset.y * i)).rgb * weights[i];
			}
		}

		color = float4(result, 1);
	}

	return color;
}

);