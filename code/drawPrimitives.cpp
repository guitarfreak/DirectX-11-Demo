
#define gs theGState

DArray<PrimitiveVertex>& dxBeginPrimitive(uint topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) {
	gs->d3ddc->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)topology);

	gs->currentTopology = topology;

	gs->vertexBuffer.count = 0;
	return gs->vertexBuffer;
}

DArray<PrimitiveVertex>& dxBeginPrimitive(Vec4 color, ID3D11ShaderResourceView* view, uint topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) {
	theGState->d3ddc->PSSetSamplers(0, 1, &theGState->sampler);
	theGState->d3ddc->PSSetShaderResources(0, 1, &view);
	dxGetShaderVars(Primitive)->color = vec4(1);
	dxPushShaderConstants(Shader_Primitive);

	return dxBeginPrimitive(topology);
}

DArray<PrimitiveVertex>& dxBeginPrimitive(Vec4 color, uint topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) {
	dxGetShaderVars(Primitive)->color = color;
	dxPushShaderConstants(Shader_Primitive);

	return dxBeginPrimitive(topology);
}

DArray<PrimitiveVertex>& dxBeginPrimitiveColored(Vec4 color, uint topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) {
	gs->d3ddc->PSSetShaderResources(0, 1, &gs->textureWhite->view);
	gs->d3ddc->PSSetSamplers(0, 1, &gs->samplerClamp);

	dxGetShaderVars(Primitive)->color = color;
	dxPushShaderConstants(Shader_Primitive);

	return dxBeginPrimitive(topology);
}
DArray<PrimitiveVertex>& dxBeginPrimitiveColored(uint topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) {
	return dxBeginPrimitiveColored(vec4(1), topology);
}

void dxMapDraw(PrimitiveVertex* verts, int count) {
	if(!count) return;
	
	D3D11_MAPPED_SUBRESOURCE sub;
	gs->d3ddc->Map(gs->primitiveVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub);

	memcpy(sub.pData, verts, sizeof(PrimitiveVertex) * count);

	gs->d3ddc->Unmap(gs->primitiveVertexBuffer, 0);
	gs->d3ddc->Draw(count, 0);
}

inline void dxEndPrimitive() { 
	dxMapDraw(gs->vertexBuffer.data, gs->vertexBuffer.count);
}

inline void dxEndPrimitive(int count) { 
	dxMapDraw(gs->vertexBuffer.data, count);
}

void dxDrawEndPrimitive(PrimitiveVertex* verts, int count, int vertsPerPrimitive) {
	int vertsPerBatch = roundDown(gs->primitiveVertexBufferMaxCount / vertsPerPrimitive) * vertsPerPrimitive;

	int pushedVerts = 0;
	while(pushedVerts != count) {
		int currentBatch = min(vertsPerBatch, count - pushedVerts);
		dxMapDraw(verts + pushedVerts, currentBatch);
		pushedVerts += currentBatch;
	}
}

void dxFlush() {
	dxEndPrimitive();
	gs->vertexBuffer.count = 0;
}

void dxFlushIfFull() {
	if(gs->vertexBuffer.count >= gs->primitiveVertexBufferMaxCount) {
		int toDraw = roundMod(gs->primitiveVertexBufferMaxCount, gs->primitiveVertexCount[gs->currentTopology]);

		dxEndPrimitive(toDraw);
		if(gs->vertexBuffer.count == toDraw) gs->vertexBuffer.count = 0;
		else gs->vertexBuffer.removeMove(0, toDraw);
	}
}

void dxFlushIfFull(int batchVertexCount) {
	if(gs->vertexBuffer.count + batchVertexCount > gs->primitiveVertexBufferMaxCount) {
		dxEndPrimitive();
		gs->vertexBuffer.count = 0;
	}
}

// @2d

inline PrimitiveVertex pVertex(Vec2 p,                  float z = gs->zLevel) { return { p.x, p.y, z, 1,1,1,1, 0,0 }; }
inline PrimitiveVertex pVertex(Vec2 p,         Vec2 uv, float z = gs->zLevel) { return { p.x, p.y, z, 1,1,1,1, uv.x, uv.y }; }
inline PrimitiveVertex pVertex(Vec2 p, Vec4 c,          float z = gs->zLevel) { return { p.x, p.y, z, c }; }
inline PrimitiveVertex pVertex(Vec2 p, Vec4 c, Vec2 uv, float z = gs->zLevel) { return { p.x, p.y, z, c, uv }; }
inline PrimitiveVertex pVertex(Vec3 p)                                        { return { p, 1,1,1,1 }; }
inline PrimitiveVertex pVertex(Vec3 p, Vec4 c)                                { return { p, c }; }

inline void dxPushVertex(PrimitiveVertex  v)            { gs->vertexBuffer.pushNC(v); }
inline void dxPushVerts (PrimitiveVertex* v, int count) { gs->vertexBuffer.pushNC(v, count); }

inline void dxPushLine(Vec2 a, Vec2 b) {
	PrimitiveVertex verts[] = {
		pVertex(a), 
		pVertex(b), 
	};
	gs->vertexBuffer.pushNC(verts, arrayCount(verts));
}

inline void dxPushLine(Vec2 a, Vec2 b, Vec4 c) {
	PrimitiveVertex verts[] = {
		pVertex(a, c), 
		pVertex(b, c), 
	};
	gs->vertexBuffer.pushNC(verts, arrayCount(verts));
}

inline void dxPushLineH(Vec2 a, Vec2 b, Vec4 color, bool roundUp = false) {
	float off = roundUp ? 0.5f : -0.5f;
	return dxPushLine(round(a) + vec2(0, off), round(b) + vec2(0, off), color);
}

inline void dxPushLineV(Vec2 a, Vec2 b, Vec4 color, bool roundUp = false) {
	float off = roundUp ? 0.5f : -0.5f;
	return dxPushLine(round(a) + vec2(off, 0), round(b) + vec2(off, 0), color);
}

inline void dxPushTriangle(Vec2 a, Vec2 b, Vec2 c) {
	PrimitiveVertex verts[] = {
		pVertex(a), 
		pVertex(b), 
		pVertex(c), 
	};
	gs->vertexBuffer.pushNC(verts, arrayCount(verts));
}

inline void dxPushTriangle(Vec2 a, Vec2 b, Vec2 c, Vec4 color) {
	PrimitiveVertex verts[] = {
		pVertex(a, color), 
		pVertex(b, color), 
		pVertex(c, color), 
	};
	gs->vertexBuffer.pushNC(verts, arrayCount(verts));
}

void dxPushRect(Rect r, Vec4 c, Rect uv) {
	Vec2 rtl = r.tl();
	Vec2 rbr = r.br();
	Vec2 uvtl = uv.tl();
	Vec2 uvbr = uv.br();
	PrimitiveVertex verts[] = {
		{ r.min.x, r.min.y, gs->zLevel, c, uvtl   },
		{ rtl.x,   rtl.y,   gs->zLevel, c, uv.min },
		{ rbr.x,   rbr.y,   gs->zLevel, c, uv.max },
		{ rbr.x,   rbr.y,   gs->zLevel, c, uv.max },
		{ rtl.x,   rtl.y,   gs->zLevel, c, uv.min },
		{ r.max.x, r.max.y, gs->zLevel, c, uvbr   },
	};
	gs->vertexBuffer.pushNC(verts, arrayCount(verts));
}

inline void dxPushRect(Rect r, Rect uv) { dxPushRect(r, vec4(1), uv); }

inline void dxPushRect(Rect r, Vec4 c)  { 
	Vec2 rtl = r.tl();
	Vec2 rbr = r.br();
	PrimitiveVertex verts[] = {
		{ r.min.x, r.min.y, gs->zLevel, c, },
		{ rtl.x,   rtl.y,   gs->zLevel, c, },
		{ rbr.x,   rbr.y,   gs->zLevel, c, },
		{ rbr.x,   rbr.y,   gs->zLevel, c, },
		{ rtl.x,   rtl.y,   gs->zLevel, c, },
		{ r.max.x, r.max.y, gs->zLevel, c, },
	};
	gs->vertexBuffer.pushNC(verts, arrayCount(verts));
}

void dxPushRect4(Rect r, Rect uv) {
	PrimitiveVertex verts[] = {
		pVertex(r.bl(), uv.bl()), 
		pVertex(r.tl(), uv.tl()), 
		pVertex(r.br(), uv.br()), 
		pVertex(r.tr(), uv.tr()), 
	};
	gs->vertexBuffer.pushNC(verts, arrayCount(verts));
}

void dxPushQuad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec4 color, Rect uv) {
	PrimitiveVertex verts[] = {
		{ vec3(a,gs->zLevel), color, uv.bl() }, 
		{ vec3(b,gs->zLevel), color, uv.tl() }, 
		{ vec3(c,gs->zLevel), color, uv.tr() }, 
		{ vec3(c,gs->zLevel), color, uv.tr() }, 
		{ vec3(d,gs->zLevel), color, uv.br() }, 
		{ vec3(a,gs->zLevel), color, uv.bl() }, 
	};
	gs->vertexBuffer.pushNC(verts, arrayCount(verts));
}

void dxPushQuad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec4 color) {
	PrimitiveVertex verts[] = {
		{ vec3(a,gs->zLevel), color }, 
		{ vec3(b,gs->zLevel), color }, 
		{ vec3(c,gs->zLevel), color }, 
		{ vec3(c,gs->zLevel), color }, 
		{ vec3(d,gs->zLevel), color }, 
		{ vec3(a,gs->zLevel), color }, 
	};
	gs->vertexBuffer.pushNC(verts, arrayCount(verts));
}


inline void dxPushLine(Vec3 a, Vec3 b, Vec4 c) {
	PrimitiveVertex verts[] = {
		pVertex(a, c), 
		pVertex(b, c), 
	};
	gs->vertexBuffer.pushNC(verts, arrayCount(verts));
}

inline void dxPushTriangle(Vec3 a, Vec3 b, Vec3 c, Vec4 color, Rect uv = rect()) {
	PrimitiveVertex verts[] = {
		{ a, color, uv.bl() }, 
		{ b, color, uv.tl() }, 
		{ c, color, uv.tr() }, 
	};
	gs->vertexBuffer.pushNC(verts, arrayCount(verts));
}

void dxPushQuad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec4 color, Rect uv = rect()) {
	PrimitiveVertex verts[] = {
		{ a, color, uv.bl() }, 
		{ b, color, uv.tl() }, 
		{ c, color, uv.tr() }, 
		{ c, color, uv.tr() }, 
		{ d, color, uv.br() }, 
		{ a, color, uv.bl() }, 
	};
	gs->vertexBuffer.pushNC(verts, arrayCount(verts));
}

inline void dxGetQuad(PrimitiveVertex* dstVerts, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec4 color, Rect uv = rect()) {
	PrimitiveVertex verts[] = {
		{ a, color, uv.bl() }, 
		{ b, color, uv.tl() }, 
		{ c, color, uv.tr() }, 
		{ c, color, uv.tr() }, 
		{ d, color, uv.br() }, 
		{ a, color, uv.bl() }, 
	};
	copyStaticArray(dstVerts, verts, PrimitiveVertex);
}

//

void dxDrawLine(Vec2 a, Vec2 b, Vec4 color) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	dxPushLine(a, b);
	dxEndPrimitive();
}

void dxDrawLineTri(Vec2 a, Vec2 b, Vec4 color, float w = 1) {
	dxBeginPrimitiveColored(color);
	Vec2 dir = b - a;
	Vec2 right = norm(rotateRight(dir)) * w * 0.5f;
	dxPushQuad(a - right, b - right, b + right, a + right, color);
	dxEndPrimitive();
}

void dxDrawLineStripTri(Vec2* p, int count, Vec4 color, float w = 1) {
	dxBeginPrimitiveColored();

	Vec2 dir, left;
	Vec2 a, b, c, d;
	float w2 = w * 0.5f;

	for(int i = 0; i < count-1; i++) {
		if(p[i] == p[i+1]) continue;

		if(i == 0) {
			dir = norm(p[i] - p[i+1]);
			left = rotateRight(dir);
			Vec2 l = left * w2;
			a = p[i] - l;
			b = p[i] + l;
			continue;
		}

		Vec2 dir2 = norm(p[i] - p[i+1]);
		Vec2 dirDiag = norm(((-dir) + dir2) * 0.5f);

		float invDot = 1.0f / dot(dirDiag, left); 
		float offsetLength = invDot * w2;

		c = p[i] + dirDiag * offsetLength;
		d = p[i] + dirDiag * -offsetLength;		

		dxPushQuad(a, b, c, d, color);
		dxFlushIfFull();

		a = d;
		b = c;
		left = rotateRight(dir2);
		dir = dir2;
	}

	dir = norm(p[count-2] - p[count-1]);
	left = rotateRight(dir);
	Vec2 l = left * w2;
	c = p[count-1] + l;
	d = p[count-1] - l;

	dxPushQuad(a, b, c, d, color);
	dxEndPrimitive();
}

void dxDrawLineH(Vec2 a, Vec2 b, Vec4 color, bool roundUp = false) {
	float off = roundUp ? 0.5f : -0.5f;
	dxDrawLine(round(a) + vec2(0, off), round(b) + vec2(0, off), color);
}

void dxDrawLineV(Vec2 a, Vec2 b, Vec4 color, bool roundUp = false) {
	float off = roundUp ? 0.5f : -0.5f;
	dxDrawLine(round(a) + vec2(off, 0), round(b) + vec2(off, 0), color);
}

void dxDrawPolygon(Vec2* points, int count, Vec4 color) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	for(int i = 0; i < count-1; i++) dxPushLine(points[i], points[i+1]);
	dxPushLine(points[count-1], points[0]);
	dxEndPrimitive();
}

void dxDrawRect(Rect r, Vec4 color, ID3D11ShaderResourceView* view = 0, Rect uv = rect(0,1,1,0)) {
	if(!view) view = gs->textureWhite->view;
	gs->d3ddc->PSSetShaderResources(0, 1, &view);
	gs->d3ddc->PSSetSamplers(0, 1, &gs->samplerClamp);

	dxGetShaderVars(Primitive)->color = color;
	dxPushShaderConstants(Shader_Primitive);

	dxBeginPrimitive(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	dxPushRect4(r, uv);
	dxEndPrimitive();
}

void dxDrawRectOutline(Rect r, Vec4 color) {	
	r = r.expand(-1);
	dxBeginPrimitiveColored(vec4(1), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	dxPushLine(r.bl() + vec2(0,0.5f),  r.tl() + vec2(0,0.5f),  color);
	dxPushLine(r.tl() + vec2(0.5f,0),  r.tr() + vec2(0.5f,0),  color);
	dxPushLine(r.tr() + vec2(0,-1.0f), r.br() + vec2(0,-1.0f), color);
	dxPushLine(r.br() + vec2(-0.5f,0), r.bl() + vec2(-0.5f,0), color);

	dxEndPrimitive();
}

void dxDrawRectOutlined(Rect r, Vec4 color, Vec4 colorOutline) {	
	dxDrawRect(r.expand(-2), color);
	dxDrawRectOutline(r, colorOutline);
}

void dxDrawRectGradientH(Rect r, Vec4 c0, Vec4 c1) {	
	dxBeginPrimitiveColored(vec4(1), D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	dxPushVertex(pVertex(r.bl(), c0));
	dxPushVertex(pVertex(r.tl(), c1));
	dxPushVertex(pVertex(r.br(), c0));
	dxPushVertex(pVertex(r.tr(), c1));
	
	dxEndPrimitive();
}

void dxDrawRectRounded(Rect r, Vec4 color, float size, bool outline = false) {
	if(!outline) {
		if(size == 0) return dxDrawRect(r, color);

	} else {
		if(size == 0) return dxDrawRectOutline(r, color);

		r = r.expand(-1);
	}

	float roundingMod = 1.0f/2.0f;
	int steps = roundInt(M_PI_2 * size * roundingMod);

	steps = 3;

	size = min(size, r.w()/2, r.h()/2);

	if(!outline) {
		dxDrawRect(r.expand(vec2(-size*2, 0)), color);
		dxDrawRect(r.expand(vec2(0, -size*2)), color);
	}

	//

	uint topology = outline ? D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP : 
	                          D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	dxBeginPrimitiveColored(color, topology);
	Rect rc = r.expand(-size*2);
	Vec2 corners[] = { rc.tr(), rc.br(), rc.bl(), rc.tl() };
	for(int cornerIndex = 0; cornerIndex < 4; cornerIndex++) {
		Vec2 corner = corners[cornerIndex];
		float startAngle = M_PI_2*cornerIndex;

		int st = outline ? steps : steps-1;
		for(int i = 0; i < st; i++) {
			float a = startAngle + i*(M_PI_2/(steps-1));
			Vec2 d = vec2(sin(a), cos(a));
			dxPushVertex( pVertex(corner + d*size) );

			if(!outline) {
				a = startAngle + (i+1)*(M_PI_2/(steps-1));
				d = vec2(sin(a), cos(a));
				dxPushVertex( pVertex(corner + d*size) );

				dxPushVertex( pVertex(corner) );
			}
		}
	}
	if(outline) dxPushVertex( pVertex(vec2(rc.right, rc.top + size)) );

	dxEndPrimitive();
};
void dxDrawRectRoundedOutline(Rect r, Vec4 color, float size) {
	return dxDrawRectRounded(r, color, size, true);
}

DArray<Vec2> dxGetRoundedRectVerts(Rect r, float w, float mod = 2.0f) {
	DArray<Vec2> vecs = dArray<Vec2>(4, getTMemory);

	float v = 2 * pow(1 - (mod/w), 2) - 1;
	v = clamp(v, -1.0f, 1.0f);
	float th = acos(v);
	int steps = ceil(M_2PI/th);
	steps = (ceil(steps / 4.0f)) * 4.0f;
	steps = max(steps, 3);

	w = min(w, r.w()/2, r.h()/2);

	r = r.expand(-w*2);
	Vec2 corners[] = { r.tr(), r.br(), r.bl(), r.tl() };

	for(int cornerIndex = 0; cornerIndex < 4; cornerIndex++) {
		Vec2 corner = corners[cornerIndex];
		float startAngle = M_PI_2*cornerIndex;

		int st = steps;
		for(int i = 0; i < steps; i++) {
			float a = startAngle + i*(M_PI_2/(steps-1));
			Vec2 d = vec2(sin(a), cos(a));

			vecs.push(corner + d*w);
		}
	}

	return vecs;
}

inline void dxPushTriangleFan(Vec2 p, Vec2* vecs, int count, Vec4 color) {
	for(int i = 0; i < count-1; i++) {
		dxPushTriangle(p, vecs[i], vecs[i+1], color);
	}
}

void dxDrawRectBorderRoundedTri(Rect r, Vec4 color, float borderSize, float cornerSize, float tesselationMod = 2.0f) {
	auto vecs = dxGetRoundedRectVerts(r.expand(-borderSize), cornerSize, tesselationMod);
	vecs.push(vecs[0]);
	vecs.push(vecs[1]);
	dxDrawLineStripTri(vecs.data, vecs.count, color, borderSize);
};
void dxDrawRectRoundedTri(Rect r, Vec4 color, float borderSize, float cornerSize, float tesselationMod = 2.0f) {
	auto vecs = dxGetRoundedRectVerts(r.expand(-borderSize), cornerSize, tesselationMod);
	dxBeginPrimitiveColored();
	vecs.push(vecs[0]);
	dxPushTriangleFan(r.c(), vecs.data, vecs.count, color);
	dxEndPrimitive();
};
void dxDrawRectAndBorderRoundedTri(Rect r, Vec4 color, float borderSize, float cornerSize, float tesselationMod = 2.0f) {
	auto vecs = dxGetRoundedRectVerts(r.expand(-borderSize), cornerSize, tesselationMod);
	vecs.push(vecs[0]);
	vecs.push(vecs[1]);
	dxDrawLineStripTri(vecs.data, vecs.count, color, borderSize);

	dxBeginPrimitiveColored();
	vecs.push(vecs[0]);
	dxPushTriangleFan(r.c(), vecs.data, vecs.count, color);
	dxEndPrimitive();
}



void dxDrawRectRoundedOutlined(Rect r, Vec4 color, Vec4 colorOutline, float size) {
	dxDrawRectRounded(r.expand(-2), color, size);
	dxDrawRectRounded(r, colorOutline, size, true);
};

void dxDrawRectRoundedGradient(Rect r, Vec4 color, int size, Vec4 off) {	
	gs->depthStencilState.StencilEnable = true;
	gs->depthStencilState.StencilReadMask = 0xFF;
	gs->depthStencilState.StencilWriteMask = 0xFF;
	gs->depthStencilState.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	gs->depthStencilState.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	gs->depthStencilState.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	gs->depthStencilState.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dxSetDepthStencil();

	{
		dxDrawRectRounded(r, color, size);
	}

	gs->depthStencilState.StencilWriteMask = 0x0;
	gs->depthStencilState.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dxSetDepthStencil();

	{
		dxGetShaderVars(Primitive)->gammaGradient = true;
		dxDrawRectGradientH(r, color - off, color + off);
		dxGetShaderVars(Primitive)->gammaGradient = false;
	}

	gs->depthStencilState.StencilEnable = false;
	dxSetDepthStencil();
}

void dxDrawCircle(Vec2 pos, float d, Vec4 color) {
	return dxDrawRect(rectCenDim(pos, vec2(d)), color, gs->textureCircle->view);
}

void dxDrawCircleX(Vec2 pos, float r, Vec4 color) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Vec2 right = vec2(r,0);
	Vec2 dir = right;
	int segmentCount = 20;
	for(int i = 0; i < segmentCount; i++) {
		dxPushVertex( pVertex( pos       ) );
		dxPushVertex( pVertex( pos + dir ) );

		dir = rotate(right, ((float)(i+1)/(segmentCount)) * M_2PI);
		dxPushVertex( pVertex( pos + dir ) );
	}

	dxEndPrimitive();
}

void dxDrawRing(Vec2 pos, float r, Vec4 color, float e = 0.1f) {

	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	defer { dxEndPrimitive(); };

	float th = acos(2 * pow(1 - e / r, 2) - 1);
	int segmentCount = ceil(M_2PI/th);
	segmentCount = (ceil(segmentCount / 4.0f)) * 4.0f;

	Vec2 right = vec2(r,0);
	Vec2 dir = right;
	for(int i = 0; i < segmentCount+1; i++) {
		dxPushVertex(pVertex( pos + dir ));
		dir = rotate(right, ((float)(i+1)/(segmentCount)) * M_2PI);
	}
}

void dxDrawRing(Vec2 pos, float r, Vec4 color, int thickness, float e = 0.1f) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	defer { dxEndPrimitive(); };

	float th = acos(2 * pow(1 - e / r, 2) - 1);
	int segmentCount = ceil(M_2PI/th);
	segmentCount = (ceil(segmentCount / 4.0f)) * 4.0f;
	
	for(int i = 0; i < segmentCount; i++) {
		float angle  = (i     * M_2PI/(float)segmentCount);
		Vec2 dir = rotate(vec2(0,1), angle);
		Vec2 p0 = pos + dir * (r - thickness);
		Vec2 p1 = pos + dir * (r);

		float angle2 = ((i+1) * M_2PI/(float)segmentCount);
		Vec2 dir2 = rotate(vec2(0,1), angle2);
		Vec2 p2 = pos + dir2 * (r - thickness);
		Vec2 p3 = pos + dir2 * (r);

		dxPushVertex(pVertex(p0));
		dxPushVertex(pVertex(p1));
		dxPushVertex(pVertex(p2));
		dxPushVertex(pVertex(p2));
		dxPushVertex(pVertex(p1));
		dxPushVertex(pVertex(p3));

		dxFlushIfFull();
	}
}

void dxDrawHueRing(Vec2 pos, float r, int thickness, float e = 0.1f) {
	dxBeginPrimitiveColored(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	defer { dxEndPrimitive(); };

	float th = acos(2 * pow(1 - e / r, 2) - 1);
	int segmentCount = ceil(M_2PI/th);
	segmentCount = (ceil(segmentCount / 4.0f)) * 4.0f;

	for(int i = 0; i < segmentCount; i++) {
		float angle  = (i     * M_2PI/(float)segmentCount);
		Vec4 c0 = vec4(hslToRgb(radianToDegree(angle),  1, 0.5), 1);
		Vec2 dir = rotate(vec2(0,1), angle);
		Vec2 p0 = pos + dir * (r - thickness);
		Vec2 p1 = pos + dir * (r);

		float angle2 = ((i+1) * M_2PI/(float)segmentCount);
		Vec4 c1 = vec4(hslToRgb(radianToDegree(angle2), 1, 0.5), 1);
		Vec2 dir2 = rotate(vec2(0,1), angle2);
		Vec2 p2 = pos + dir2 * (r - thickness);
		Vec2 p3 = pos + dir2 * (r);

		dxPushVertex(pVertex(p0, c0));
		dxPushVertex(pVertex(p1, c0));
		dxPushVertex(pVertex(p2, c1));
		dxPushVertex(pVertex(p2, c1));
		dxPushVertex(pVertex(p1, c0));
		dxPushVertex(pVertex(p3, c1));

		dxFlushIfFull();
	}
}

void dxDrawTriangle(Vec2 p, float r, Vec4 color, Vec2 dir) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	dir = norm(dir) * r;
	dxPushVertex( pVertex( p + dir                       ) );
	dxPushVertex( pVertex( p + rotate(dir, M_2PI*1/3.0f) ) );
	dxPushVertex( pVertex( p + rotate(dir, M_2PI*2/3.0f) ) );

	dxEndPrimitive();
}

void dxDrawTriangle(Vec2 a, Vec2 b, Vec2 c, Vec4 color) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	dxPushVertex( pVertex( a ) );
	dxPushVertex( pVertex( b ) );
	dxPushVertex( pVertex( c ) );

	dxEndPrimitive();
}

void dxDrawQuad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec4 color) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	dxPushVertex( pVertex( a ) );
	dxPushVertex( pVertex( b ) );
	dxPushVertex( pVertex( d ) );
	dxPushVertex( pVertex( c ) );

	dxEndPrimitive();
}

void dxDrawCross(Vec2 p, float size, float size2, Vec4 color) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	for(int i = 0; i < 2; i++) {
		Vec2 p0 = p + (i == 0 ? vec2(-1,-1) : rotateRight(vec2(-1,-1))) * size/2;
		Vec2 p1 = p - (i == 0 ? vec2(-1,-1) : rotateRight(vec2(-1,-1))) * size/2;
		Vec2 d0 =     (i == 0 ? vec2( 0, 1) : rotateRight(vec2( 0, 1))) * size2/2;
		Vec2 d1 =     (i == 0 ? vec2( 1, 0) : rotateRight(vec2( 1, 0))) * size2/2;

		dxPushVertex( pVertex( p0 ) );
		dxPushVertex( pVertex( p0 + d0 ) );
		dxPushVertex( pVertex( p0 + d1 ) );
		dxPushVertex( pVertex( p1 - d1 ) );
		dxPushVertex( pVertex( p1 - d0 ) );
		dxPushVertex( pVertex( p1 ) );
	}

	dxEndPrimitive();
}

void dxDrawArrow(Vec2 start, Vec2 end, float thickness, Vec4 color, float headScale = 2.0f) {
	float hw = thickness/2;

	Vec2 startToEnd = end-start;
	Vec2 down = norm(rotateRight(startToEnd));

	float headWidth = thickness*headScale;
	float headHeight = thickness*headScale;

	float length = len(startToEnd);
	bool tooSmall = false;
	if(length < headHeight) tooSmall = true;

	Vec2 dir = norm(startToEnd);
	if(!tooSmall) end -= dir * headHeight;

	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	defer { dxEndPrimitive(); };

	dxPushQuad(start + down*hw, start - down*hw, end - down*hw, end + down*hw, vec4(1));

	if(!tooSmall) {
		dxPushTriangle(end - down*headWidth/2, end + dir*headHeight, end + down*headWidth/2, vec4(1));
	}
}

// @3d

void dxDrawLine(Vec3 a, Vec3 b, Vec4 color) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	dxPushVertex( pVertex( a ) );
	dxPushVertex( pVertex( b ) );

	dxEndPrimitive();
}

void dxDrawTriangle(Vec3 a, Vec3 b, Vec3 c, Vec4 color) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxPushTriangle(a, b, c, color);
	dxEndPrimitive();
}

void dxDrawQuad(Vec3 bl, Vec3 tl, Vec3 tr, Vec3 br, Rect uv, ID3D11ShaderResourceView* view, Vec4 color) {
	dxBeginPrimitive(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	gs->d3ddc->PSSetShaderResources(0, 1, &view);
	dxGetShaderVars(Primitive)->color = vec4(1,1,1,1);
	dxPushShaderConstants(Shader_Primitive);

	dxPushVertex( { bl, color, uv.bl() } );
	dxPushVertex( { tl, color, uv.tl() } );
	dxPushVertex( { br, color, uv.br() } );
	dxPushVertex( { tr, color, uv.tr() } );

	dxEndPrimitive();
}

void dxDrawQuad(Vec3 bl, Vec3 tl, Vec3 tr, Vec3 br, Vec4 color) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	dxPushQuad(bl, tl, tr, br, color);
	dxPushQuad(bl, br, tr, tl, color);

	dxEndPrimitive();
}

void dxDrawArrow(Vec3 start, Vec3 end, Vec3 up, float thickness, Vec4 color) {

	float hw = thickness/2;
	Vec3 down = norm(cross(norm(end-start), norm(up)));

	float headWidth = thickness*2;
	float headHeight = thickness*2;

	float length = len(start - end);
	bool tooSmall = false;
	if(length < headHeight) tooSmall = true;

	Vec3 dir = norm(end - start);
	if(!tooSmall) end -= dir * headHeight;

	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	defer { dxEndPrimitive(); };

	dxPushQuad(start + down*hw, start - down*hw, end - down*hw, end + down*hw, vec4(1));

	if(!tooSmall) {
		dxPushTriangle(end - down*headWidth/2, end + dir*headHeight, end + down*headWidth/2, vec4(1));
	}
}

void dxDrawPlane(Vec3 pos, Vec3 normal, Vec3 up, Vec2 dim, Vec4 color, ID3D11ShaderResourceView* view = 0, Rect uv = rect(0,1,1,0)) {
	if(!view) view = gs->textureWhite->view;
	gs->d3ddc->PSSetShaderResources(0, 1, &view);

	dim = dim/2.0f;
	Vec3 right = cross(up, normal);

	dxBeginPrimitive(vec4(1), D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	defer { dxEndPrimitive(); };

	dxPushQuad(pos + right*dim.x + up*dim.y, 
	           pos + right*dim.x - up*dim.y, 
	           pos - right*dim.x - up*dim.y, 
	           pos - right*dim.x + up*dim.y, 
	           color, uv);
}

void dxDrawPlaneOutline(Vec3 pos, Vec3 normal, Vec3 up, Vec2 dim, Vec4 color) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	defer { dxEndPrimitive(); };

	dim = dim/2.0f;
	Vec3 right = cross(up, normal);

	dxPushVertex(pVertex(pos + right*dim.x + up*dim.y));
	dxPushVertex(pVertex(pos + right*dim.x - up*dim.y));
	dxPushVertex(pVertex(pos - right*dim.x - up*dim.y));
	dxPushVertex(pVertex(pos - right*dim.x + up*dim.y));
	dxPushVertex(pVertex(pos + right*dim.x + up*dim.y));
}

void dxDrawCube(Vec3 point, Vec3 scale, Vec4 color) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Mesh* m = dxGetMesh("cube\\obj.obj");
	for(int i = 0; i < m->vertices.count; i++) {
		dxPushVertex( pVertex( m->vertices[i].pos * scale + point ) );
	}

	dxEndPrimitive();
}

void dxDrawCube(XForm form, Vec4 color) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Mesh* m = dxGetMesh("cube\\obj.obj");
	for(int i = 0; i < m->vertices.count; i++) {
		dxPushVertex( pVertex( (form.rot*(m->vertices[i].pos * form.scale)) + form.trans ) );
	}

	dxEndPrimitive();	
}

void dxDrawCube(Mat4 mat, Vec4 color) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Mesh* m = dxGetMesh("cube\\obj.obj");
	for(int i = 0; i < m->vertices.count; i++) {
		dxPushVertex( pVertex( (mat * vec4(m->vertices[i].pos,1)).xyz ) );
	}

	dxEndPrimitive();	
}

// Expensive, should be better.
void dxDrawSphere(Vec3 point, float scale, Vec4 color) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	defer { dxEndPrimitive(); };

	Mesh* m = dxGetMesh("sphere\\obj.obj");
	for(int i = 0; i < m->vertices.count/3; i++) {
		dxPushVertex( pVertex(m->vertices[(i*3)+0].pos * scale + point) );
		dxPushVertex( pVertex(m->vertices[(i*3)+1].pos * scale + point) );
		dxPushVertex( pVertex(m->vertices[(i*3)+2].pos * scale + point) );

		dxFlushIfFull();
	}
}

void dxDrawRing(Vec3 pos, Vec3 normal, float r, float thickness, Vec4 color, float error) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	defer { dxEndPrimitive(); };

	int segmentCount;
	if(error < 0) {
		segmentCount = fabs(error);
	} else {
		float th = acos(2 * pow(1 - error / r, 2) - 1);
		segmentCount = ceil(M_2PI/th);
		segmentCount = (ceil(segmentCount / 4.0f)) * 4.0f;
	}

	Vec3 otherVector = normal == vec3(1,0,0) || normal == vec3(-1,0,0) ? vec3(0,1,0) : vec3(1,0,0);
	Vec3 up = norm(cross(normal, otherVector));
	float ri = r - thickness;
	
	for(int i = 0; i < segmentCount; i++) {
		Vec3 dir = rotate(up, (i * M_2PI/(float)segmentCount), normal);
		Vec3 p0 = pos + dir * ri;
		Vec3 p1 = pos + dir * r;

		Vec3 dir2 = rotate(up, ((i+1) * M_2PI/(float)segmentCount), normal);
		Vec3 p2 = pos + dir2 * ri;
		Vec3 p3 = pos + dir2 * r;

		dxPushVertex(pVertex(p0));
		dxPushVertex(pVertex(p1));
		dxPushVertex(pVertex(p2));
		dxPushVertex(pVertex(p2));
		dxPushVertex(pVertex(p1));
		dxPushVertex(pVertex(p3));

		dxFlushIfFull();
	}
}

void dxDrawTriangleFan(Vec3 pos, Vec3 start, float angle, Vec3 up, Vec4 color, float error) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	defer { dxEndPrimitive(); };

	float r = len(start-pos);

	int segmentCount;
	if(error < 0) {
		segmentCount = fabs(error);
	} else {
		float th = acos(2 * pow(1 - error / r, 2) - 1);
		segmentCount = ceil(angle/th);
		segmentCount = (ceil(segmentCount / 4.0f)) * 4.0f;
	}
	
	for(int i = 0; i < segmentCount; i++) {
		Vec3 p0 = rotateAround(start, (i * angle/(float)segmentCount), up, pos);
		Vec3 p1 = rotateAround(start, ((i+1) * angle/(float)segmentCount), up, pos);

		dxPushVertex(pVertex(pos));
		dxPushVertex(pVertex(p0));
		dxPushVertex(pVertex(p1));

		dxFlushIfFull();
	}
}

#undef gs
