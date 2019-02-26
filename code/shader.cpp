
#define HLSL(src) "" #src
#define HLSL2(extra, src) extra #src
#define HLSL3(extra1, extra2, src) extra1 extra2 #src
#define HLSL4(extra1, extra2, extra3, src) extra1 extra2 extra3 #src

using float4x4 = Mat4;
using float4   = Vec4;
using float3   = Vec3;
using float2   = Vec2;

#include "shader/shaderPrimitive.cpp"
#include "shader/shaderMain.cpp"
#include "shader/shaderGradient.cpp"
#include "shader/shaderParticle.cpp"
#include "shader/shaderShadow.cpp"
#include "shader/shaderSky.cpp"
#include "shader/shaderBloom.cpp"
#include "shader/shaderCube.cpp"


#define ShaderList(func) \
	func(Primitive) \
	func(Bloom) \
	func(Particle) \
	func(Main) \
	func(Sky) \
	func(Gradient) \
	func(Cube) \
	func(Shadow) \

#define func(name) static_assert((sizeof(name##ShaderVars) % 16) == 0, "Constant Buffer size must be 16-byte aligned");
ShaderList(func)
#undef func

#define funcc(name) Shader_##name,
enum {
	Shader_Pre = -1,
	ShaderList(funcc)
};
#undef funcc

struct ShaderInfo {
	char* code;
	int varsSize;
};

#define makeShaderInfo(name) { d3d##name##Shader, sizeof(name##ShaderVars) },

ShaderInfo shaderInfos[] = {
	ShaderList(makeShaderInfo)
};

#define dxGetShaderVars(name) ((name##ShaderVars*)theGState->shaders[Shader_##name].varsData)

Shader* dxGetShader(int shaderId) {
	return theGState->shaders + shaderId;
}

//

void setupInputLayouts() {
	GraphicsState* gs = theGState;

	{
		Shader* shader = dxGetShader(Shader_Primitive);
		if ( FAILED( gs->d3dDevice->CreateInputLayout(primitiveInputLayout, arrayCount(primitiveInputLayout), shader->vertexBlob->GetBufferPointer(), shader->vertexBlob->GetBufferSize(), &gs->primitiveInputLayout) ) ) 
			printf("Could not create Input Layout!");
	}

	{
		Shader* shader = dxGetShader(Shader_Main);
		if ( FAILED( gs->d3dDevice->CreateInputLayout(mainShaderInputLayout, arrayCount(mainShaderInputLayout), shader->vertexBlob->GetBufferPointer(), shader->vertexBlob->GetBufferSize(), &gs->mainInputLayout) ) ) 
			printf("Could not create Input Layout!");
	}

	dxGetShader(Shader_Primitive)->inputLayout = gs->primitiveInputLayout;
	dxGetShader(Shader_Bloom)->inputLayout = gs->primitiveInputLayout;
	dxGetShader(Shader_Particle)->inputLayout = gs->primitiveInputLayout;
	dxGetShader(Shader_Main)->inputLayout = gs->mainInputLayout;
	dxGetShader(Shader_Sky)->inputLayout = 0;
	dxGetShader(Shader_Gradient)->inputLayout = gs->primitiveInputLayout;
	dxGetShader(Shader_Cube)->inputLayout = gs->mainInputLayout;
	dxGetShader(Shader_Shadow)->inputLayout = gs->mainInputLayout;
}

//

void dxLoadShader(int type, char* shaderCode, char* name, ID3D11VertexShader** vertexShader, ID3D11HullShader** hullShader, ID3D11DomainShader** domainShader, ID3D11PixelShader** pixelShader, ID3DBlob** shaderBlob = 0) {
	enum {
		SHADER_TYPE_VERTEX = 0,
		SHADER_TYPE_HULL,
		SHADER_TYPE_DOMAIN,
		SHADER_TYPE_PIXEL,
	};

	     if(type == SHADER_TYPE_VERTEX && *vertexShader) (*vertexShader)->Release();
	else if(type == SHADER_TYPE_HULL   && *hullShader)   (*hullShader)->Release();
	else if(type == SHADER_TYPE_DOMAIN && *domainShader) (*domainShader)->Release();
	else if(type == SHADER_TYPE_PIXEL  && *pixelShader)  (*pixelShader)->Release();

	#if SHADER_DEBUG
	uint flags1 = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	#else
	uint flags1 = 0;
	#endif

	ID3DBlob* blobError;

	char* version[] = {"vs_5_0", "hs_5_0", "ds_5_0", "ps_5_0"};

	ID3DBlob* blob;
	D3DCompile(shaderCode, strlen(shaderCode), NULL, NULL, NULL, name, version[type], flags1, 0, &blob, &blobError);
	{
		if (blobError != nullptr) {
			char* errorMessage = (char*)blobError->GetBufferPointer();

			// If we get an "entrypoint not found" message on hull or domain shaders we assume that that's intended.
			if(strFind(errorMessage, "entrypoint not found") != -1) {
				     if(type == SHADER_TYPE_HULL)   hullShader = 0;
				else if(type == SHADER_TYPE_DOMAIN) domainShader = 0;

				return;
			} else {
				printf(errorMessage);
			}

			blobError->Release();
		}
	}

	     if(type == SHADER_TYPE_VERTEX) theGState->d3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), 0, vertexShader);
	else if(type == SHADER_TYPE_HULL)   theGState->d3dDevice->CreateHullShader(blob->GetBufferPointer(), blob->GetBufferSize(), 0, hullShader);
	else if(type == SHADER_TYPE_DOMAIN) theGState->d3dDevice->CreateDomainShader(blob->GetBufferPointer(), blob->GetBufferSize(), 0, domainShader);
	else if(type == SHADER_TYPE_PIXEL)  theGState->d3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), 0, pixelShader);

	if(shaderBlob) *shaderBlob = blob;
}

void dxLoadShaders() {
	for(int i = 0; i < theGState->shaderCount; i++) {
		Shader* shader = theGState->shaders + i;
		ShaderInfo* info = shaderInfos + shader->id;

		dxLoadShader(0, info->code, "vertexShader", &shader->vertexShader, 0, 0, 0, &shader->vertexBlob);
		dxLoadShader(1, info->code, "hullShader",   0, &shader->hullShader,   0, 0, 0);
		dxLoadShader(2, info->code, "domainShader", 0, 0, &shader->domainShader, 0, 0);
		dxLoadShader(3, info->code, "pixelShader",  0, 0, 0, &shader->pixelShader,  0);
	}
}

void dxSetShader(int shaderId) {
	GraphicsState* gs = theGState;
	Shader* shader = gs->shaders + shaderId;

	if(shader->id == gs->currentShader) return;

	gs->currentShader = shader->id;

	{
		gs->d3ddc->VSSetShader(shader->vertexShader, 0, 0);
		gs->d3ddc->HSSetShader(shader->hullShader, 0, 0);   // If the shaders are zero they will be unbound intentionally.
		gs->d3ddc->DSSetShader(shader->domainShader, 0, 0); // If the shaders are zero they will be unbound intentionally.
		gs->d3ddc->PSSetShader(shader->pixelShader, 0, 0);
		
		gs->d3ddc->VSSetConstantBuffers(0, 1, &shader->constantBuffer);
		if(shader->hullShader) gs->d3ddc->HSSetConstantBuffers(0, 1, &shader->constantBuffer);
		if(shader->domainShader) gs->d3ddc->DSSetConstantBuffers(0, 1, &shader->constantBuffer);
		gs->d3ddc->PSSetConstantBuffers(0, 1, &shader->constantBuffer);

		if(shader->inputLayout) {
			gs->d3ddc->IASetInputLayout(shader->inputLayout);
		}
	}

	if((shader->id == Shader_Primitive) || 
	   (shader->id == Shader_Particle) || 
	   (shader->id == Shader_Bloom)) {
		UINT stride = sizeof(PrimitiveVertex);
		UINT offset = 0;
		gs->d3ddc->IASetVertexBuffers( 0, 1, &gs->primitiveVertexBuffer, &stride, &offset );
	}
}

void dxPushShaderConstants(int shaderId) {
	GraphicsState* gs = theGState;
	Shader* shader = gs->shaders + shaderId;

	D3D11_MAPPED_SUBRESOURCE sub;
	gs->d3ddc->Map(shader->constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub);

	memcpy(sub.pData, shader->varsData, shader->varsSize);

	gs->d3ddc->Unmap(shader->constantBuffer, 0);
}

inline void dxSetShaderAndPushConstants(int shaderId) {
	dxSetShader(shaderId);
	dxPushShaderConstants(shaderId);
}
