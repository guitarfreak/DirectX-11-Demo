
#define ShaderVarsStruct(name) \
struct name {\
	float4x4 viewProj;\
	float4x4 shadowViewProj;\
\
	float nearPlane;\
	float farPlane;\
	int msaaSamples;\
\
	float fadeDistance;\
	float fadeContrast;\
\
	float3 _pad0;\
\
	float3 ambient;\
	int lightCount;\
	Light light;\
\
	float2 shadowMapSize;\
\
	float2 _pad1;\
};\

ShaderVarsStruct(ParticleShaderVars)



char* d3dParticleShader = HLSL3 (
STRINGIFY_MACRO(ExtraVarsStruct()),
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

	float4 shadowPos : POSITION1;
};

Texture2D    tex  : register(t0);
SamplerState samp : register(s0);
ShaderVars   vars : register(b0);

Texture2DMS<float> depthTex : register(t7);

Texture2D              pTex   : register(t5);
SamplerComparisonState sampP  : register(s1);

PSInput vertexShader(VSInput input) {
	PSInput output;

	output.svPos = mul(float4(input.pos, 1), vars.viewProj);
	output.zPos = output.svPos.z;

	output.color = float4(pow(input.color.rgb, 2.2f), input.color.a);
	output.uv = input.uv;

	output.shadowPos = mul(float4(input.pos, 1.0f), vars.shadowViewProj);

	return output;
}

float contrast(float input, float contrastPower) {
	float output = 0.5*pow(saturate(2*((input > 0.5) ? 1-input : input)),
	contrastPower);
	output = (input > 0.5) ? 1-output : output;
	return output;
}

float linearizeDepth(float z) {
	/*
		Internet says this:
		(2.0 * near) / (far + near - z * (far - near));	

		NVIDIA paper says this:
		return (far * near) / (far + z * (near - far));

		But our projectionMatrix is different so its this:
		Should fix projectionMatrix because it's likely to be wrong.

		(far * near) / (-(far*z) + far + z*near);
		-> (above - near) / (far - near))
	*/

	return -(vars.nearPlane * z) / (vars.farPlane*z - vars.farPlane - vars.nearPlane*z);
}

float4 pixelShader(PSInput input) : SV_Target {
	float4 color = tex.Sample(samp, input.uv) * input.color;

	float shadowFactor;
	{
		float3 projCoord = input.shadowPos.xyz / input.shadowPos.w;

		if(projCoord.x < -1.0f || projCoord.x > 1.0f || 
		   projCoord.y < -1.0f || projCoord.y > 1.0f || 
		   projCoord.z <  0.0f || projCoord.z > 1.0f) {
			// return float4(vars.ambient, 1);
			// return float4(0,1,0,1);
			shadowFactor = 0;
		
		} else {
			projCoord.y *= -1;
			projCoord.xy = (projCoord.xy + 1) / 2.0f;

			// 4x4 PCF grid.
			{
				float dist = 1.5f;
				float diff = 1.0f;

				for(float y = -dist; y <= dist; y += diff) {
					for(float x = -dist; x <= dist; x += diff) {
						float2 uvOffset = float2(x * 1/vars.shadowMapSize.x, y * 1/vars.shadowMapSize.y);
						shadowFactor += pTex.SampleCmpLevelZero(sampP, projCoord.xy + uvOffset, projCoord.z);
					}
				}
				shadowFactor /= 16.0f;
			}

			// shadowFactor = pTex.SampleCmpLevelZero(sampP, projCoord.xy, projCoord.z);
		}
	}

	float3 lightIntensity = vars.ambient + shadowFactor * vars.light.color;
	color.rgb *= lightIntensity;

	float alphaScale;
	{
		float zScene = 0;
		
		// for(int i = 0; i < vars.msaaSamples; i++) {
		// 	zScene += depthTex.Load(input.svPos.xy, i);
		// }
		// zScene /= vars.msaaSamples;

		zScene = depthTex.Load(input.svPos.xy, 0);
		zScene = linearizeDepth(zScene);

		float zParticle = input.zPos / vars.farPlane;
		float diff = zScene - zParticle;

		// Geometry fade.
		float c = saturate(diff / vars.fadeDistance);
		c = contrast(c, vars.fadeContrast);
		// float c = smoothstep(0,0.02f,diff);

		// Near plane fade.
		float c2 = saturate(zParticle / vars.fadeDistance);
		c2 = contrast(c2, vars.fadeContrast);

		color.a *= c * c2;
		alphaScale = c * c2;

		// Not sure about this.
		// Makes alpha fade visibly linear, but do we want that?
		alphaScale = pow(alphaScale, 1/2.2f);
	}

	color.a *= alphaScale;
	color.rgb *= color.a;

	return color;
}

);
