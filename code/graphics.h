
Meta_Parse_Struct(0);
struct GraphicsState {

	IDXGISwapChain* swapChain; // @Ignore
	ID3D11Device* d3dDevice; // @Ignore
	ID3D11DeviceContext* d3ddc; // @Ignore

	//

	Texture* textures; // @SizeVar(textureCount)
	int textureCount;
	int textureCountMax;
	Texture* textureWhite;
	Texture* textureCircle;

	ID3D11Texture2D* cubeMapTexture; // @Ignore
	ID3D11ShaderResourceView* cubeMapView; // @Ignore
	int cubeMapSize;

	int shadowMapSize;

	ID3D11RenderTargetView* backBufferView; // @Ignore
	FrameBuffer* frameBuffers; // @SizeVar(frameBufferCount)
	int frameBufferCount;
	int frameBufferCountMax;

	Font fonts[10][20]; // @SizeVar(fontsCount)
	int fontsCount;
	char* fontFolders[10]; // @String @SizeVar(fontFolderCount)
	int fontFolderCount;

	Mesh* meshes; // @Ignore
	int meshCount; // @Ignore

	Material* materials; // @Ignore
	int materialCount; // @Ignore

	//

	Shader* shaders; // @Ignore
	int shaderCount; // @Ignore

	ID3D11InputLayout* primitiveInputLayout; // @Ignore
	ID3D11InputLayout* mainInputLayout; // @Ignore

	ID3D11Buffer* primitiveVertexBuffer; // @Ignore
	int primitiveVertexBufferMaxCount; // @Ignore

	ID3D11Buffer* shaderSamplesBuffer; // @Ignore

	//

	D3D11_RASTERIZER_DESC rasterizerState; // @Ignore
	D3D11_DEPTH_STENCIL_DESC depthStencilState; // @Ignore
	
	ID3D11SamplerState* sampler; // @Ignore
	ID3D11SamplerState* samplerClamp; // @Ignore
	ID3D11SamplerState* samplerCmp; // @Ignore

	ID3D11BlendState* blendStates[10]; // @Ignore
	int blendStateCount; // @Ignore

	GraphicsMatrices gMats;
	GraphicsSettings* gSettings;
	Camera activeCam;

	//

	DXTimer timer; // @Ignore

	// D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP = 5,
	int primitiveVertexCount[D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP+1];

	//

	int currentShader;

	// PrimitiveVertex* pVertexArray; // @Ignore
	DArray<PrimitiveVertex> vertexBuffer; // @Ignore

	uint currentTopology;

	Vec2i screenRes;
	Rect screenRect;

	float zLevel;

	//

	void init() { *this = {}; };
};

extern GraphicsState* theGState;
