
struct AssetInfo {
	FILETIME lastWriteTime;
};

Meta_Parse_Struct(0);
struct Texture {
	char* name; // @String
	char* file; // @String

	uint type;
	Vec2i dim;

	D3D11_TEXTURE2D_DESC desc; // @Ignore
	ID3D11Texture2D* resource; // @Ignore
	ID3D11ShaderResourceView* view; // @Ignore

	uint format;

	bool spriteSheet;
	int spriteCount;
	Vec2i cellDim;

	AssetInfo assetInfo; // @Ignore

	bool hasAlpha;

	// int channels;
	// int levels;
	// int internalFormat;
	// int channelType;
	// int channelFormat;
};

Meta_Parse_Struct(0);
struct FrameBuffer {
	char* name; // @String
	Vec2i dim;

	DXGI_FORMAT format; // @Ignore

	bool hasRenderTargetView;
	bool hasShaderResourceView;
	bool hasDepthStencilView;

	ID3D11Texture2D* texture; // @Ignore

	bool isShadow;
	bool makeDepthView;

	union {
		struct {
			ID3D11RenderTargetView* renderTargetView; // @Ignore
			ID3D11ShaderResourceView* shaderResourceView; // @Ignore
		};

		ID3D11DepthStencilView* depthStencilView; // @Ignore
	};

	// dim, msaa, format, viewtypes, 
};

struct Shader {
	int id;
	char* name;

	char* varsData;
	int varsSize;

	ID3DBlob* vertexBlob;
	ID3D11VertexShader* vertexShader;
	ID3D11HullShader* hullShader;
	ID3D11DomainShader* domainShader;
	ID3D11PixelShader* pixelShader;
	ID3D11PixelShader* pixelShader2;
	ID3D11Buffer* constantBuffer;

	ID3D11InputLayout* inputLayout;
};

enum BlendState {
	Blend_State_Blend,
	Blend_State_BlendAlphaCoverage,
	Blend_State_NoBlend,
	Blend_State_DrawOverlay,
	Blend_State_BlitOverlay,
	Blend_State_PreMultipliedAlpha,
	Blend_State_Add,
};

struct MeshVertex {
	Vec3 pos;
	Vec2 uv;
	Vec3 normal;
	Vec3 tangent;
	Vec3 bitangent;
	Vec4 blendWeights;
	int blendIndices[4];
};

D3D11_INPUT_ELEMENT_DESC primitiveInputLayout[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

D3D11_INPUT_ELEMENT_DESC mainShaderInputLayout[] = {
	{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",       1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",       2, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 72, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

Meta_Parse_Struct(0);
struct GraphicsSettings {
	Vec2i cur3dBufferRes;
	int msaaSamples;
	int msaaQuality;
	float resolutionScale;
	float aspectRatio;
	int fieldOfView;
	float nearPlane;
	float farPlane;
};

Meta_Parse_Struct(0);
struct GraphicsMatrices {
	Mat4 view2d;
	Mat4 ortho;
	Mat4 view;
	Mat4 proj;
	Mat4 viewInv;
	Mat4 projInv;
};

struct DXTimer {
	ID3D11DeviceContext* d3ddc;

	ID3D11Query* queryStart[5];
	ID3D11Query* queryStop[5];
	ID3D11Query* queryDisjoint[5];

	int queryCount;
	int queryIndex;

	bool initialized;

	double dt;

	void init(ID3D11Device* d3dDevice, ID3D11DeviceContext* d3ddc) {
		this->d3ddc = d3ddc;

		D3D11_QUERY_DESC queryDesc;

		for(int i = 0; i < arrayCount(queryStart); i++) {
			queryDesc = {D3D11_QUERY_TIMESTAMP};
			d3dDevice->CreateQuery(&queryDesc, &queryStop[i]);
		}

		for(int i = 0; i < arrayCount(queryStart); i++) {
			queryDesc = {D3D11_QUERY_TIMESTAMP};
			d3dDevice->CreateQuery(&queryDesc, &queryStart[i]);
		}

		for(int i = 0; i < arrayCount(queryStart); i++) {
			queryDesc = {D3D11_QUERY_TIMESTAMP_DISJOINT};
			d3dDevice->CreateQuery(&queryDesc, &queryDisjoint[i]);
		}

		queryCount = arrayCount(queryStart);
		queryIndex = 0;
	}

	void start() {
		d3ddc->Begin(queryDisjoint[queryIndex]);
		d3ddc->End(queryStart[queryIndex]);
	}

	bool stop() {
		d3ddc->End(queryStop[queryIndex]);
		d3ddc->End(queryDisjoint[queryIndex]);

		// Wait for one full roundtrip.
		if(!initialized && queryIndex == queryCount-1) {
			initialized = true;
		}

		bool result = false;

		int nextIndex = (queryIndex+1)%queryCount;
		if(initialized) {

			D3D11_QUERY_DATA_TIMESTAMP_DISJOINT queryDataDisjoint;
			UINT64 queryDataStart;
			UINT64 queryDataEnd;

			while(S_OK != d3ddc->GetData(queryDisjoint[nextIndex], &queryDataDisjoint, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0)) {
				sleep(1);
			}

			bool result1 = (S_OK != d3ddc->GetData(queryStart[nextIndex], &queryDataStart, sizeof(UINT64), 0));
			bool result2 = (S_OK != d3ddc->GetData(queryStop[nextIndex], &queryDataEnd, sizeof(UINT64), 0));

			// 	if(queryDataDisjoint.Disjoint) {
			// 		waiting = false;
			// 		return false;
			// 	}

			dt = (queryDataEnd - queryDataStart) / (double)queryDataDisjoint.Frequency;
			dt *= 1000; // To ms.

			result = true;
		}

		queryIndex = nextIndex;

		return result;
	}
};