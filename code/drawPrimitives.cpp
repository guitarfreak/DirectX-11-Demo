
PrimitiveVertex* dxBeginPrimitive(uint topology = -1) {
	GraphicsState* gs = theGState;

	if(topology != -1) 
		gs->d3ddc->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)topology);

	gs->currentTopology = topology;

	D3D11_MAPPED_SUBRESOURCE sub;
	gs->d3ddc->Map(gs->primitiveVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub);

	gs->pVertexArray = (PrimitiveVertex*)sub.pData;
	gs->pVertexCount = 0;

	return gs->pVertexArray;
}

PrimitiveVertex* dxBeginPrimitive(Vec4 color, uint topology = -1) {
	GraphicsState* gs = theGState;

	dxGetShaderVars(Primitive)->color = color;
	dxPushShaderConstants(Shader_Primitive);

	return dxBeginPrimitive(topology);
}

PrimitiveVertex* dxBeginPrimitiveColored(Vec4 color, uint topology = -1) {
	GraphicsState* gs = theGState;

	gs->d3ddc->PSSetShaderResources(0, 1, &gs->textureWhite->view);
	gs->d3ddc->PSSetSamplers(0, 1, &gs->samplerClamp);

	dxGetShaderVars(Primitive)->color = color;
	dxPushShaderConstants(Shader_Primitive);

	return dxBeginPrimitive(topology);
}
PrimitiveVertex* dxBeginPrimitiveColored(uint topology) {
	return dxBeginPrimitiveColored(vec4(1), topology);
}

void dxEndPrimitive(int vertexCount = theGState->pVertexCount) {
	GraphicsState* gs = theGState;

	gs->d3ddc->Unmap(gs->primitiveVertexBuffer, 0);
	gs->d3ddc->Draw(vertexCount, 0);
}

void dxFlush() {
	GraphicsState* gs = theGState;

	dxEndPrimitive();
	dxBeginPrimitive(gs->currentTopology);
}

void dxFlushIfFull() {
	GraphicsState* gs = theGState;

	if(gs->pVertexCount >= gs->primitiveVertexBufferMaxCount-12) {
		dxEndPrimitive();
		dxBeginPrimitive(gs->currentTopology);
	}
}

// @2d

PrimitiveVertex pVertex(Vec2 p, float z = theGState->zLevel) {
	return { p.x, p.y, z, 1,1,1,1, 0,0 };
}

PrimitiveVertex pVertex(Vec2 p, Vec2 uv, float z = theGState->zLevel) {
	return { p.x, p.y, z, 1,1,1,1, uv.x, uv.y };
}

PrimitiveVertex pVertex(Vec2 p, Vec4 c, float z = theGState->zLevel) {
	return { p.x, p.y, z, c };
}

PrimitiveVertex pVertex(Vec2 p, Vec4 c, Vec2 uv, float z = theGState->zLevel) {
	return { p.x, p.y, z, c, uv };
}

PrimitiveVertex pVertex(Vec3 p) {
	return { p, 1,1,1,1 };
}

PrimitiveVertex pVertex(Vec3 p, Vec4 c) {
	return { p, c };
}

void dxPushVertex(PrimitiveVertex v) {
	theGState->pVertexArray[theGState->pVertexCount++] = v;
}

void dxPushVerts(PrimitiveVertex* v, int count) {
	for(int i = 0; i < count; i++) {
		theGState->pVertexArray[theGState->pVertexCount++] = v[i];
	}
}

void dxPushLine(Vec2 a, Vec2 b, Vec4 c) {
	theGState->pVertexArray[theGState->pVertexCount++] = pVertex(a, c);
	theGState->pVertexArray[theGState->pVertexCount++] = pVertex(b, c);
}

void dxPushTriangle(Vec2 a, Vec2 b, Vec2 c) {
	theGState->pVertexArray[theGState->pVertexCount++] = pVertex(a);
	theGState->pVertexArray[theGState->pVertexCount++] = pVertex(b);
	theGState->pVertexArray[theGState->pVertexCount++] = pVertex(c);
}

void dxPushTriangle(Vec2 a, Vec2 b, Vec2 c, Vec4 color) {
	theGState->pVertexArray[theGState->pVertexCount++] = pVertex(a, color);
	theGState->pVertexArray[theGState->pVertexCount++] = pVertex(b, color);
	theGState->pVertexArray[theGState->pVertexCount++] = pVertex(c, color);
}

void dxPushRect(Rect r, Rect uv) {
	GraphicsState* gs = theGState;
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.bl(), uv.tl());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.tl(), uv.bl());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.br(), uv.tr());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.br(), uv.tr());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.tl(), uv.bl());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.tr(), uv.br());
}

void dxPushRect(Rect r, Vec4 c, Rect uv) {
	GraphicsState* gs = theGState;
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.bl(), c, uv.tl());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.tl(), c, uv.bl());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.br(), c, uv.tr());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.br(), c, uv.tr());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.tl(), c, uv.bl());
	gs->pVertexArray[gs->pVertexCount++] = pVertex(r.tr(), c, uv.br());
}

void dxPushQuad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec4 color, Rect uv = rect()) {
	theGState->pVertexArray[theGState->pVertexCount++] = { vec3(a,theGState->zLevel), color, uv.bl() };
	theGState->pVertexArray[theGState->pVertexCount++] = { vec3(b,theGState->zLevel), color, uv.tl() };
	theGState->pVertexArray[theGState->pVertexCount++] = { vec3(c,theGState->zLevel), color, uv.tr() };
	theGState->pVertexArray[theGState->pVertexCount++] = { vec3(c,theGState->zLevel), color, uv.tr() };
	theGState->pVertexArray[theGState->pVertexCount++] = { vec3(d,theGState->zLevel), color, uv.br() };
	theGState->pVertexArray[theGState->pVertexCount++] = { vec3(a,theGState->zLevel), color, uv.bl() };
}

void dxPushLine(Vec3 a, Vec3 b, Vec4 c) {
	theGState->pVertexArray[theGState->pVertexCount++] = pVertex(a, c);
	theGState->pVertexArray[theGState->pVertexCount++] = pVertex(b, c);
}

void dxPushTriangle(Vec3 a, Vec3 b, Vec3 c, Vec4 color, Rect uv = rect()) {
	theGState->pVertexArray[theGState->pVertexCount++] = { a, color, uv.bl() };
	theGState->pVertexArray[theGState->pVertexCount++] = { b, color, uv.tl() };
	theGState->pVertexArray[theGState->pVertexCount++] = { c, color, uv.tr() };
}

void dxPushQuad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec4 color, Rect uv = rect()) {
	theGState->pVertexArray[theGState->pVertexCount++] = { a, color, uv.bl() };
	theGState->pVertexArray[theGState->pVertexCount++] = { b, color, uv.tl() };
	theGState->pVertexArray[theGState->pVertexCount++] = { c, color, uv.tr() };
	theGState->pVertexArray[theGState->pVertexCount++] = { c, color, uv.tr() };
	theGState->pVertexArray[theGState->pVertexCount++] = { d, color, uv.br() };
	theGState->pVertexArray[theGState->pVertexCount++] = { a, color, uv.bl() };
}

void dxGetQuad(PrimitiveVertex* verts, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec4 color, Rect uv = rect()) {
	verts[0] = { a, color, uv.bl() };
	verts[1] = { b, color, uv.tl() };
	verts[2] = { c, color, uv.tr() };
	verts[3] = { c, color, uv.tr() };
	verts[4] = { d, color, uv.br() };
	verts[5] = { a, color, uv.bl() };
}

//

void dxDrawLine(Vec2 a, Vec2 b, Vec4 color) {
	PrimitiveVertex* v = dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	v[0] = pVertex(a);
	v[1] = pVertex(b);

	dxEndPrimitive(2);
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
	PrimitiveVertex* v = dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	for(int i = 0; i < count-1; i++) {
		dxPushLine(points[i], points[i+1], vec4(1,1));
	}
	dxPushLine(points[count-1], points[0], color);

	dxEndPrimitive();
}

void dxDrawRect(Rect r, Vec4 color, ID3D11ShaderResourceView* view = 0, Rect uv = rect(0,1,1,0)) {
	GraphicsState* gs = theGState;

	if(!view) view = gs->textureWhite->view;
	gs->d3ddc->PSSetShaderResources(0, 1, &view);
	gs->d3ddc->PSSetSamplers(0, 1, &gs->samplerClamp);

	dxGetShaderVars(Primitive)->color = color;
	dxPushShaderConstants(Shader_Primitive);

	PrimitiveVertex* v = dxBeginPrimitive(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	v[0] = pVertex(r.bl(), uv.bl());
	v[1] = pVertex(r.tl(), uv.tl());
	v[2] = pVertex(r.br(), uv.br());
	v[3] = pVertex(r.tr(), uv.tr());

	dxEndPrimitive(4);
}

void dxDrawRectOutline(Rect r, Vec4 color) {	
	r = r.expand(-1);
	dxBeginPrimitiveColored(vec4(1), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	dxPushLine(r.bl() + vec2(0,0.5f),  r.tl() + vec2(0,0.5f),  color);
	dxPushLine(r.tl() + vec2(0.5f,0),  r.tr() + vec2(0.5f,0),  color);
	dxPushLine(r.tr() + vec2(0,-0.5f), r.br() + vec2(0,-0.5f), color);
	dxPushLine(r.br() + vec2(-0.5f,0), r.bl() + vec2(-0.5f,0), color);

	dxEndPrimitive();
}

void dxDrawRectOutlined(Rect r, Vec4 color, Vec4 colorOutline) {	
	dxDrawRect(r.expand(-2), color);
	dxDrawRectOutline(r, colorOutline);
}

void dxDrawRectGradientH(Rect r, Vec4 c0, Vec4 c1) {	
	PrimitiveVertex* v = dxBeginPrimitiveColored(vec4(1), D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	v[0] = pVertex(r.bl(), c0);
	v[1] = pVertex(r.tl(), c1);
	v[2] = pVertex(r.br(), c0);
	v[3] = pVertex(r.tr(), c1);

	dxEndPrimitive(4);
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
	PrimitiveVertex* v = dxBeginPrimitiveColored(color, topology);
	int vc = 0;

	Rect rc = r.expand(-size*2);
	Vec2 corners[] = { rc.tr(), rc.br(), rc.bl(), rc.tl() };
	for(int cornerIndex = 0; cornerIndex < 4; cornerIndex++) {
		Vec2 corner = corners[cornerIndex];
		float startAngle = M_PI_2*cornerIndex;

		// v[vc++] = pVertex(corner);
		// v[vc++] = pVertex(corner + vec2(-1,0) * size);

		int st = outline ? steps : steps-1;
		for(int i = 0; i < st; i++) {
			float a = startAngle + i*(M_PI_2/(steps-1));
			Vec2 d = vec2(sin(a), cos(a));
			v[vc++] = pVertex(corner + d*size);

			if(!outline) {
				a = startAngle + (i+1)*(M_PI_2/(steps-1));
				d = vec2(sin(a), cos(a));
				v[vc++] = pVertex(corner + d*size);

				v[vc++] = pVertex(corner);
			}
		}

		// v[vc++] = pVertex(corner + vec2(0,1) * size);
		// v[vc++] = pVertex(corner);

	}
	if(outline) v[vc++] = pVertex(vec2(rc.right, rc.top + size));

	dxEndPrimitive(vc);
};
void dxDrawRectRoundedOutline(Rect r, Vec4 color, float size) {
	return dxDrawRectRounded(r, color, size, true);
}

void dxDrawRectRoundedOutlined(Rect r, Vec4 color, Vec4 colorOutline, float size) {
	dxDrawRectRounded(r.expand(-2), color, size);
	dxDrawRectRounded(r, colorOutline, size, true);
};

void dxDrawRectRoundedGradient(Rect r, Vec4 color, int size, Vec4 off) {	
	GraphicsState* gs = theGState;

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
	return dxDrawRect(rectCenDim(pos, vec2(d)), color, theGState->textureCircle->view);
}

void dxDrawCircleX(Vec2 pos, float r, Vec4 color) {

	PrimitiveVertex* v = dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	int pi = 0;

	Vec2 right = vec2(r,0);
	Vec2 dir = right;
	int segmentCount = 20;
	for(int i = 0; i < segmentCount; i++) {
		v[pi++] = pVertex( pos       );
		v[pi++] = pVertex( pos + dir );

		dir = rotate(right, ((float)(i+1)/(segmentCount)) * M_2PI);
		v[pi++] = pVertex( pos + dir );
	}

	dxEndPrimitive(pi);
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
	PrimitiveVertex* v = dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	dir = norm(dir) * r;
	v[0] = pVertex( p + dir                       );
	v[1] = pVertex( p + rotate(dir, M_2PI*1/3.0f) );
	v[2] = pVertex( p + rotate(dir, M_2PI*2/3.0f) );

	dxEndPrimitive(3);
}

void dxDrawTriangle(Vec2 a, Vec2 b, Vec2 c, Vec4 color) {
	PrimitiveVertex* v = dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	v[0] = pVertex( a );
	v[1] = pVertex( b );
	v[2] = pVertex( c );

	dxEndPrimitive(3);
}

void dxDrawQuad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec4 color) {
	PrimitiveVertex* v = dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	v[0] = pVertex( a );
	v[1] = pVertex( b );
	v[2] = pVertex( d );
	v[3] = pVertex( c );

	dxEndPrimitive(4);
}

void dxDrawCross(Vec2 p, float size, float size2, Vec4 color) {

	PrimitiveVertex* v = dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	int vc = 0;

	for(int i = 0; i < 2; i++) {
		Vec2 p0 = p + (i == 0 ? vec2(-1,-1) : rotateRight(vec2(-1,-1))) * size/2;
		Vec2 p1 = p - (i == 0 ? vec2(-1,-1) : rotateRight(vec2(-1,-1))) * size/2;
		Vec2 d0 =     (i == 0 ? vec2( 0, 1) : rotateRight(vec2( 0, 1))) * size2/2;
		Vec2 d1 =     (i == 0 ? vec2( 1, 0) : rotateRight(vec2( 1, 0))) * size2/2;

		v[vc++] = pVertex( p0 );
		v[vc++] = pVertex( p0 + d0 );
		v[vc++] = pVertex( p0 + d1 );
		v[vc++] = pVertex( p1 - d1 );
		v[vc++] = pVertex( p1 - d0 );
		v[vc++] = pVertex( p1 );
	}

	dxEndPrimitive(vc);
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
	PrimitiveVertex* v = dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	v[0] = pVertex( a );
	v[1] = pVertex( b );

	dxEndPrimitive(2);
}

void dxDrawTriangle(Vec3 a, Vec3 b, Vec3 c, Vec4 color) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxPushTriangle(a, b, c, color);
	dxEndPrimitive();
}

void dxDrawQuad(Vec3 bl, Vec3 tl, Vec3 tr, Vec3 br, Rect uv, ID3D11ShaderResourceView* view, Vec4 color) {
	PrimitiveVertex* v = dxBeginPrimitive(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	theGState->d3ddc->PSSetShaderResources(0, 1, &view);
	dxGetShaderVars(Primitive)->color = vec4(1,1,1,1);
	dxPushShaderConstants(Shader_Primitive);

	v[0] = { bl, color, uv.bl() };
	v[1] = { tl, color, uv.tl() };
	v[2] = { br, color, uv.br() };
	v[3] = { tr, color, uv.tr() };

	dxEndPrimitive(4);
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
	if(!view) view = theGState->textureWhite->view;
	theGState->d3ddc->PSSetShaderResources(0, 1, &view);

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
	PrimitiveVertex* v = dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Mesh* m = dxGetMesh("cube\\obj.obj");

	int c = 0;
	for(int i = 0; i < m->vertices.count; i++) {
		v[c++] = pVertex( m->vertices[i].pos * scale + point );
	}

	dxEndPrimitive(c);
}

void dxDrawCube(XForm form, Vec4 color) {
	PrimitiveVertex* v = dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Mesh* m = dxGetMesh("cube\\obj.obj");

	int c = 0;
	for(int i = 0; i < m->vertices.count; i++) {
		v[c++] = pVertex( (form.rot*(m->vertices[i].pos * form.scale)) + form.trans );
	}

	dxEndPrimitive(c);	
}

void dxDrawCube(Mat4 mat, Vec4 color) {
	PrimitiveVertex* v = dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Mesh* m = dxGetMesh("cube\\obj.obj");

	int c = 0;
	for(int i = 0; i < m->vertices.count; i++) {
		v[c++] = pVertex( (mat * vec4(m->vertices[i].pos,1)).xyz );
	}

	dxEndPrimitive(c);	
}

// Expensive, should be better.
void dxDrawSphere(Vec3 point, float scale, Vec4 color) {
	dxBeginPrimitiveColored(color, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	defer { dxEndPrimitive(); };

	Mesh* m = dxGetMesh("sphere\\obj.obj");

	int c = 0;
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
		segmentCount = abs(error);
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
		segmentCount = abs(error);
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
