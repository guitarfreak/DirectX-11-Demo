
struct AssetInfo {
	FILETIME lastWriteTime;
};

struct Texture {
	char* name;
	char* file;

	uint type;
	Vec2i dim;

	D3D11_TEXTURE2D_DESC desc;
	ID3D11Texture2D* resource;
	ID3D11ShaderResourceView* view;

	uint format;

	AssetInfo assetInfo;

	// int channels;
	// int levels;
	// int internalFormat;
	// int channelType;
	// int channelFormat;
};

struct FrameBuffer {
	char* name;
	Vec2i dim;

	DXGI_FORMAT format;

	bool hasRenderTargetView;
	bool hasShaderResourceView;
	bool hasDepthStencilView;

	ID3D11Texture2D* texture;

	bool isShadow;

	union {
		struct {
			ID3D11RenderTargetView* renderTargetView;
			ID3D11ShaderResourceView* shaderResourceView;
		};

		ID3D11DepthStencilView* depthStencilView;
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

struct PrimitiveVertex {
	Vec3 pos;
	Vec4 color;
	Vec2 uv;
};

struct MeshVertex {
	Vec3 pos;
	Vec2 uv;
	Vec3 normal;
	Vec3 tangent;
	Vec3 bitangent;
};

D3D11_INPUT_ELEMENT_DESC primitiveInputLayout[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

D3D11_INPUT_ELEMENT_DESC mainShaderInputLayout[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   2, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

struct Material {
	char* name;
	char* file;

	Vec3 Ka;   // ambient
	Vec3 Kd;   // diffuse
	Vec3 Ks;   // specular
	float Ns;  // specularExponent
	Vec3 Ke;   // emission
	float Ni;  // refractionIndex
	float d;   // alpha
	int illum; // illuminationMode

	Texture map_Ka;
	Texture map_Kd;
	Texture map_Ks;
	Texture bump;
	Texture disp;

	float heightScale;
};

struct Mesh {
	struct Group {
		char* name;
		int offset;
		int size;

		Material material;
		bool smoothing;
	};

	char* name;
	char* file;
	char* mtlFile;

	Group* groups;
	int groupCount;

	ID3D11Buffer* vertexBuffer;
	int size;

	// DArray<MeshVertex> vertices;

	bool swapWinding;

	AssetInfo assetInfo[2];
};

struct Object {
	// Mesh* m;
	char* name;
	Vec3 pos;
	Vec3 dim;
	Vec4 color;
	Quat rot;

	char* material;
	bool hasAlpha;
};