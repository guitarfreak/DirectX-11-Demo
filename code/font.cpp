
Font* fontInit(Font* fontSlot, char* file, float height, bool enableHinting = false) {
	char* fontFolder = 0;
	for(int i = 0; i < theGState->fontFolderCount; i++) {
		if(fileExists(fString("%s%s", theGState->fontFolders[i], file))) {
			fontFolder = theGState->fontFolders[i];
			break;
		}
	}
	if(!fontFolder) return 0;

	char* path = fString("%s%s", fontFolder, file);

	// Settings.

	Font font;
	
	bool stemDarkening = true;
	bool pixelAlign = false;
	bool pixelAlignStartPos = true;
	bool isSubpixel = false;

	int target;
	if(height <= 14.0f)      target = FT_LOAD_TARGET_MONO   | FT_LOAD_FORCE_AUTOHINT;
	else if(height <= 25.0f) target = FT_LOAD_TARGET_NORMAL | FT_LOAD_FORCE_AUTOHINT;
	else                     target = FT_LOAD_TARGET_NORMAL;

	int loadFlags = FT_LOAD_DEFAULT | target;	

	// FT_RENDER_MODE_NORMAL, FT_RENDER_MODE_LIGHT, FT_RENDER_MODE_MONO, FT_RENDER_MODE_LCD, FT_RENDER_MODE_LCD_V,
	FT_Render_Mode renderFlags = !isSubpixel ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_LCD;

	// isSubpixel = true;
	if(isSubpixel) {
		loadFlags   = FT_LOAD_TARGET_(FT_RENDER_MODE_LIGHT);
		renderFlags =                 FT_RENDER_MODE_LCD;
	}

	// FT_RENDER_MODE_NORMAL = 0,
	// FT_RENDER_MODE_LIGHT,
	// FT_RENDER_MODE_MONO,
	// FT_RENDER_MODE_LCD,
	// FT_RENDER_MODE_LCD_V,

	{
		font.glyphRangeCount = 0;
		auto setupRange = [](int a, int b) { return vec2i(a, b - a + 1); };
		font.glyphRanges[font.glyphRangeCount++] = setupRange(0x20, 0x7F);
		font.glyphRanges[font.glyphRangeCount++] = setupRange(0xA1, 0xFF);
		font.glyphRanges[font.glyphRangeCount++] = setupRange(0x25BA, 0x25C4);
		font.glyphRanges[font.glyphRangeCount++] = setupRange(0x258C, 0x258C); // ▌
		font.glyphRanges[font.glyphRangeCount++] = setupRange(0x03BC, 0x03BC); // μ
		// font.glyphRanges[font.glyphRangeCount++] = setupRange(0x2192, 0x2192); // →

		// font.glyphRanges[font.glyphRangeCount++] = setupRange(0x54, 0x54);
		// font.glyphRanges[font.glyphRangeCount++] = setupRange(0x6D, 0x6D);

		font.totalGlyphCount = 0;
		for(int i = 0; i < font.glyphRangeCount; i++) font.totalGlyphCount += font.glyphRanges[i].y;
	}

	font.file = getPString(file);
	font.heightIndex = height;

	int error;
	error = FT_Init_FreeType(&font.library); assert(error == 0);
	error = FT_New_Face(font.library, path, 0, &font.face); assert(error == 0);
	FT_Face face = font.face;

	FT_Parameter parameter;
	FT_Bool darkenBool = stemDarkening;
	parameter.tag = FT_PARAM_TAG_STEM_DARKENING;
	parameter.data = &darkenBool;
	error = FT_Face_Properties(face, 1, &parameter); assert(error == 0);

	int pointFraction = 64;
	font.pixelScale = (float)1/pointFraction;
	float fullHeightToAscend = (float)face->ascender / (float)(face->ascender + abs(face->descender));

	// Height < 0 means use point size instead of pixel size
	if(height > 0) {
		error = FT_Set_Pixel_Sizes(font.face, 0, roundIntf(height) * fullHeightToAscend); assert(error == 0);
	} else {
		error = FT_Set_Char_Size(font.face, 0, (roundIntf(-height) * fullHeightToAscend) * pointFraction, 0, 0); 
		assert(error == 0);
	}

	// Get true height from freetype.
	font.height = (face->size->metrics.ascender + abs(face->size->metrics.descender)) / pointFraction;
	font.baseOffset = (face->size->metrics.ascender / pointFraction);

	// We calculate the scaling ourselves because Freetype doesn't offer it??
	float scale = (float)face->size->metrics.ascender / (float)face->ascender;
	font.lineSpacing = roundIntf(((face->height * scale) / pointFraction));
	font.pixelAlign = pixelAlign;
	font.pixelAlignStartPos = pixelAlignStartPos;
	font.hasKerning = FT_HAS_KERNING(font.face);
	font.enableKerning = false;
	font.isSubpixel = isSubpixel;



	int gridSize = (sqrt((float)font.totalGlyphCount) + 1);
	Vec2i texSize = vec2i(gridSize * font.height);
	Vec2 texSizeInv = vec2(1.0f / texSize.w, 1.0f / texSize.h);

	int widthMod = isSubpixel ? 3 : 1;
	uchar* fontBitmapBuffer = mallocArray(unsigned char, texSize.x*widthMod*texSize.y);
	defer { free(fontBitmapBuffer); };

	memset(fontBitmapBuffer, 0, texSize.x*widthMod*texSize.y);

	{
		font.cData = mallocArray(PackedChar, font.totalGlyphCount);
		int glyphIndex = 0;
		for(int rangeIndex = 0; rangeIndex < font.glyphRangeCount; rangeIndex++) {
			for(int i = 0; i < font.glyphRanges[rangeIndex].y; i++) {
				int unicode = font.glyphRanges[rangeIndex].x + i;

				FT_Load_Char(face, unicode, loadFlags);
				FT_Render_Glyph(face->glyph, renderFlags);

				FT_Bitmap* bitmap = &face->glyph->bitmap;
				Vec2i coordinate = vec2i(glyphIndex%gridSize, glyphIndex/gridSize);
				Vec2i startPixel = coordinate * font.height;

				int x0 = startPixel.x;
				int x1 = startPixel.x + bitmap->width/(float)widthMod;	
				int y0 = startPixel.y;
				int y1 = startPixel.y + bitmap->rows;
				font.cData[glyphIndex].uv = rect(x0 * texSizeInv.x, y0 * texSizeInv.y, x1 * texSizeInv.x, y1 * texSizeInv.y);

				float xBearing = face->glyph->metrics.horiBearingX / pointFraction;
				float yBearing = face->glyph->metrics.horiBearingY / pointFraction;
				float width    = face->glyph->metrics.width        / pointFraction;
				// float width    = (face->glyph->metrics.width/(float)widthMod)  / pointFraction;
				float height   = face->glyph->metrics.height       / pointFraction;
				Rect r = rectBLDim(xBearing, -(height - yBearing), width, height);
				r.bottom -= font.baseOffset;
				r.top    -= font.baseOffset;
				font.cData[glyphIndex].r = r;

				font.cData[glyphIndex].xadvance = face->glyph->metrics.horiAdvance / pointFraction;
				font.cData[glyphIndex].xadvanceSubpixelOffset = (face->glyph->lsb_delta + face->glyph->rsb_delta) / pointFraction;

				for(int y = 0; y < bitmap->rows; y++) {
					for(int x = 0; x < bitmap->width; x++) {
						Vec2i coord = vec2i(startPixel.x*widthMod, startPixel.y) + vec2i(x,y);
						fontBitmapBuffer[coord.y*texSize.w*widthMod + coord.x] = bitmap->buffer[y*(bitmap->pitch) + x];
					}
				}

				// for(int i = 0; i < 10; i += 3) {
				// 	fontBitmapBuffer[0+i] = 0;
				// 	fontBitmapBuffer[1+i] = 135;
				// 	fontBitmapBuffer[2+i] = srgbToLinear(189/255.0f)*255;
				// 	// fontBitmapBuffer[2+i] = 189;
				// }

				glyphIndex++;
			}
		}
	}

	uchar* fontBitmap = mallocArray(unsigned char, texSize.x*texSize.y*4);
	defer { free(fontBitmap); };

	if(!isSubpixel) {
		memset(fontBitmap, 255, texSize.w*texSize.h*4);
		for(int i = 0; i < texSize.w*texSize.h; i++) fontBitmap[i*4+3] = fontBitmapBuffer[i];	
	} else {
		for(int i = 0; i < texSize.w*texSize.h; i++) {
			fontBitmap[i*4+0] = fontBitmapBuffer[i*3+0];
			fontBitmap[i*4+1] = fontBitmapBuffer[i*3+1];
			fontBitmap[i*4+2] = fontBitmapBuffer[i*3+2];
			fontBitmap[i*4+3] = 255;
		}		
	}

	Texture tex = {};
	tex.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tex.dim = texSize;
	dxCreateTexture(&tex, (char*)fontBitmap);
	font.tex = tex;

	*fontSlot = font;
	return fontSlot;
}

void freeFont(Font* font) {
	freeZero(font->cData);
	dxReleaseTexture(&font->tex);

	font->heightIndex = 0;
}

Font* getFont(char* fontFile, float heightIndex, char* boldFontFile = 0, char* italicFontFile = 0) {
	GraphicsState* gs = theGState;

	int fontCount = arrayCount(gs->fonts);
	int fontSlotCount = arrayCount(gs->fonts[0]);
	Font* fontSlot = 0;
	for(int i = 0; i < fontCount; i++) {
		if(gs->fonts[i][0].heightIndex == 0) {
			fontSlot = &gs->fonts[i][0];
			break;
		} else {
			if(strCompare(fontFile, gs->fonts[i][0].file)) {
				for(int j = 0; j < fontSlotCount; j++) {
					float h = gs->fonts[i][j].heightIndex;
					if(h == 0 || h == heightIndex) {
						fontSlot = &gs->fonts[i][j];
						goto forEnd;
					}
				}
			}
		}
	}
	forEnd:

	// We are going to assume for now that a font size of 0 means it is uninitialized.
	if(fontSlot->heightIndex == 0) {
		Font* font = fontInit(fontSlot, fontFile, heightIndex);
		if(!font) {
			printf("Could not initialize font!\n");
			assert(false);
		}

		if(boldFontFile) {
			fontSlot->boldFont = getPStruct(Font);
			fontInit(fontSlot->boldFont, boldFontFile, heightIndex);
		} else font->boldFont = 0;

		if(italicFontFile) {
			fontSlot->italicFont = getPStruct(Font);
			fontInit(fontSlot->italicFont, italicFontFile, heightIndex);
		} else font->italicFont = 0;
	}

	return fontSlot;
}

//

inline char getRightBits(char n, int count) {
	int bitMask = 0;
	for(int i = 0; i < count; i++) bitMask += (1 << i);
	return n&bitMask;
}

int unicodeDecode(uchar* s, int* byteCount) {
	if(s[0] <= 127) {
		*byteCount = 1;
		return s[0];
	}

	int bitCount = 1;
	for(;;) {
		char bit = (1 << 8-bitCount-1);
		if(s[0]&bit) bitCount++;
		else break;
	}

	(*byteCount) = bitCount;

	int unicodeChar = 0;
	for(int i = 0; i < bitCount; i++) {
		char byte = i==0 ? getRightBits(s[i], 8-(bitCount+1)) : getRightBits(s[i], 6);

		unicodeChar += ((int)byte) << (6*((bitCount-1)-i));
	}

	return unicodeChar;
}

int unicodeGetSize(uchar* s) {
	if(s[0] <= 127) return 1;

	int bitCount = 1;
	for(;;) {
		char bit = (1 << 8-bitCount-1);
		if(s[0]&bit) bitCount++;
		else break;
	}

	return bitCount;
}

int getUnicodeRangeOffset(int c, Font* font) {
	int unicodeOffset = -1;

	bool found = false;
	for(int i = 0; i < font->glyphRangeCount; i++) {
		if(between(c, font->glyphRanges[i].x, font->glyphRanges[i].x+font->glyphRanges[i].y)) {
			unicodeOffset += c - font->glyphRanges[i].x + 1;
			found = true;
			break;
		}
		unicodeOffset += font->glyphRanges[i].y;
	}

	if(!found) {
		if(c == Font_Error_Glyph) return 0;
		unicodeOffset = getUnicodeRangeOffset(Font_Error_Glyph, font);
	}

	return unicodeOffset;
}

float getKerning(int c, int c2, Font* font) {
	if(!font->hasKerning) return 0;

	FT_Vector kerning;

	uint charIndex1 = FT_Get_Char_Index(font->face, c);
	uint charIndex2 = FT_Get_Char_Index(font->face, c2);

	FT_Get_Kerning(font->face, charIndex1, charIndex2, FT_KERNING_DEFAULT, &kerning);
	float kernAdvance = kerning.x * font->pixelScale;
	
	return kernAdvance;
}

struct TextMarkerInfo {
	int index;
	u8 type;
	bool enable;
	Vec3 color;
};

bool parseTextMarkerAndAdvance(char** _text, TextMarkerInfo* info, bool colorState) {
	char* text = *_text;
	if(text[0] != '<') return false;
	if(!(text[1] != '\0' && text[2] == '>')) return false;

	if(text[1] == 'b') {
		info->type = TEXT_MARKER_BOLD;
		*_text += 3;
		return true;
	} else if(text[1] == 'i') {
		info->type = TEXT_MARKER_ITALIC;
		*_text += 3;
		return true;
	} else if(text[1] == 'c') {
		info->type = TEXT_MARKER_COLOR;
		if(!colorState) {
			// Assuming rest of string if formatted correctly.
			info->color.r = colorIntToFloat(strHexToInt(getTString(text + 3, 2)));
			info->color.g = colorIntToFloat(strHexToInt(getTString(text + 5, 2)));
			info->color.b = colorIntToFloat(strHexToInt(getTString(text + 7, 2)));
			*_text += 9;
		} else {
			*_text += 3;
		}
		return true;
	}

	return false;
}

struct TextParseInfo {
	int c;
	PackedChar *charData;
	float kerning;
};

struct ParsedTextData {
	DArray<TextParseInfo> chars;
	DArray<TextMarkerInfo> markers;
	bool noLineBreak;
};

ParsedTextData parseText(char* text, Font* font) {
	auto chars = dArray<TextParseInfo>(4, getTMemory);
	auto markers = dArray<TextMarkerInfo>(20, getTMemory);
	bool noLineBreak = true;
	{
		char* t = text;
		int charIndex = 0;
		bool markerSwitches[TEXT_MARKER_Size] = {};
		Font* currentFont = font;

		while(true) {
			TextMarkerInfo markerInfo;
			if(t[0] == '<') {
				if(parseTextMarkerAndAdvance(&t, &markerInfo, markerSwitches[TEXT_MARKER_COLOR])) {
					markerSwitches[markerInfo.type] = !markerSwitches[markerInfo.type];

					markerInfo.enable = markerSwitches[markerInfo.type];
					markerInfo.index = charIndex;
					markers.push(markerInfo);

					if(markerInfo.type == TEXT_MARKER_BOLD && font->boldFont) {
						if(markerInfo.enable) currentFont = font->boldFont;
						else currentFont = font;

					} else if(markerInfo.type == TEXT_MARKER_ITALIC && font->italicFont) {
						if(markerInfo.enable) currentFont = font->italicFont;
						else currentFont = font;
					}

					continue;
				}
			}

			int cSize;
			int c = unicodeDecode((uchar*)t, &cSize);
			if(c == '\0') break;

			if(c == '\n') noLineBreak = false;

			t += cSize;

			TextParseInfo info;
			info.c = c;
			info.charData = currentFont->cData + getUnicodeRangeOffset(c, currentFont);
			info.kerning = 0;

			chars.push(info);

			if(currentFont->enableKerning && charIndex > 0) {
				TextParseInfo* prevInfo = &chars[chars.count-2];
				prevInfo->kerning = getKerning(prevInfo->c, c, currentFont);
			}

			charIndex++;
		}
	}

	ParsedTextData td = {chars, markers, noLineBreak};
	return td;
}

bool textSim(DArray<TextParseInfo> chars, TextSimInfo* tsi, TextInfo* ti) {
	if(ti) ti->lineBreak = false;

	if(tsi->lineBreak) {
		if(ti) {
			ti->lineBreak = true;
			ti->breakPos = tsi->breakPos;
		}
		tsi->lineBreak = false;
	}

	if(tsi->index == chars.count) {
		if(ti) {
			ti->pos = tsi->pos;
			ti->index = tsi->index;
		}
		return false;		
	}

	bool wrapped = false;
	if(tsi->wrapWidth && tsi->index == tsi->wrapIndex) {
		int i = tsi->index;
		float wordWidth = 0;
		while(!(chars[i].c == ' ' || i == chars.count || chars[i].c == '\n')) {
			wordWidth += chars[i].charData->xadvance + chars[i].kerning;
			i++;
		}

		if(tsi->pos.x + wordWidth >= tsi->startPos.x + tsi->wrapWidth) wrapped = true;

		if(i != tsi->index) tsi->wrapIndex = i;
		else tsi->wrapIndex++;
	}

	Vec2 oldPos = tsi->pos;
	TextParseInfo* info = chars + tsi->index;

	if(info->c == '\n' || wrapped) {
		tsi->lineBreak = true;
		tsi->breakPos = tsi->pos;
		if(info->c == '\n') tsi->breakPos.x += info->charData->xadvance;

		tsi->pos.x = tsi->startPos.x;
		tsi->pos.y -= tsi->font->lineSpacing;

		if(wrapped) return textSim(chars, tsi, ti);
	} else {
		tsi->pos.x += info->charData->xadvance + info->kerning;
	}

	if(ti) {
		ti->pos = oldPos;
		ti->index = tsi->index;
		ti->r = info->charData->r;
		ti->r.min += ti->pos;
		ti->r.max += ti->pos;
		ti->uv = info->charData->uv;
	}

	tsi->index++;

	return true;
}

TextSettings textSettings(Font* font, Vec4 color, int shadowMode, Vec2 shadowDir, float shadowSize, Vec4 shadowColor) {
	return {font, color, shadowMode, shadowDir, shadowSize, shadowColor};
}
TextSettings textSettings(Font* font, Vec4 color, int shadowMode, float shadowSize, Vec4 shadowColor) {
	return {font, color, shadowMode, vec2(-1,-1), shadowSize, shadowColor};
}
TextSettings textSettings(Font* font, Vec4 color) {
	return {font, color};
}

Vec2 getTextDim(DArray<TextParseInfo> chars, Font* font, Vec2 startPos = vec2(0,0), int wrapWidth = 0, bool hExact = false, bool getWidth = true, bool getHeight = true, bool noLineBreak = false) {
	Vec2 dim;
	if(getHeight && noLineBreak) {
		getHeight = false;
		dim.h = font->height;
	}

	if(!getWidth && !getHeight) return dim;

	float maxX = startPos.x;

	TextSimInfo tsi = textSimInfo(startPos, font, wrapWidth);
	while(true) {
		if(!textSim(chars, &tsi, 0)) break;
		if(getWidth) maxX = max(maxX, tsi.pos.x);
	}

	if(getWidth)  dim.x = maxX - startPos.x;
	if(getHeight) dim.y = startPos.y - (tsi.pos.y - font->height);
	return dim;
}

inline Vec2 getTextDim(char* text, Font* font, Vec2 startPos = vec2(0,0), int wrapWidth = 0, bool hExact = false) {
	ParsedTextData td = parseText(text, font);
	return getTextDim(td.chars, font, startPos, wrapWidth, hExact, td.noLineBreak);
}

float getTextMaxWidth(char* textArray[], int count, Font* font) {
	float maxWidth = 0;
	for(int i = 0; i < count; i++) {
		maxWidth = max(maxWidth, getTextDim(textArray[i], font).w);
	}
	return maxWidth;
}

template <class T> inline float getTextMaxWidth(T* array, int count, Font* font, char* (*getText) (T* a)) {
	float maxWidth = 0;
	for(int i = 0; i < count; i++) {
		char* text = getText(array + i); 
		maxWidth = max(maxWidth, getTextDim(text, font).w);
	}
	return maxWidth;
}

template <class T> float getTextMaxWidth(DArray<T> array, Font* font, char* (*getText) (T* a)) {
	return getTextMaxWidth(array.data, array.count, font, getText);
}

Vec2 getTextStartPos(DArray<TextParseInfo> chars, Font* font, Vec2 startPos, Vec2i align = vec2i(-1,1), int wrapWidth = 0, bool noLineBreak = false) {
	bool hExact = align.y == 0;

	bool getWidth  = align.x != -1;
	bool getHeight = align.y !=  1;
	Vec2 dim;
	if(getWidth || getHeight) {
		dim = getTextDim(chars, font, startPos, wrapWidth, hExact, getWidth, getHeight, noLineBreak);
	}
	if(getWidth)  startPos.x -= (align.x+1)*0.5f*dim.w;
	if(getHeight) startPos.y -= (align.y-1)*0.5f*dim.h;

	if(font->pixelAlignStartPos) startPos = round(startPos);

	return startPos;
}

void drawText(char* text, Vec2 startPos, Vec2i align, int wrapWidth, TextSettings settings) {
	Font* font = settings.font;

	tMemoryPushMarkerScoped();
	ParsedTextData td = parseText(text, font);

	int cullWidth = wrapWidth;
	if(settings.cull) wrapWidth = 0;
	// int cullOffset = font->height * 2;

	startPos = getTextStartPos(td.chars, font, startPos, align, wrapWidth, td.noLineBreak);

	dxBeginPrimitive(vec4(1), font->tex.view);
	defer { dxEndPrimitive(); };

	Vec4 currentColor = settings.color;
	// Vec4 shadowColor = settings.shadowColor;

	TextSimInfo tsi = textSimInfo(startPos, font, wrapWidth);
	int markerIndex = 0;
	while(true) {
		if(td.markers.count) {
			while(td.markers[markerIndex].index == tsi.index) {
				auto& marker = td.markers[markerIndex];
				if(marker.type == TEXT_MARKER_COLOR) {
					if(marker.enable) currentColor.rgb = marker.color;
					else currentColor.rgb = settings.color.rgb;

				} else if(marker.type == TEXT_MARKER_BOLD && font->boldFont) {
					Font* f = marker.enable ? font->boldFont : font;
					dxFlush();
					dxSetTexture(f->tex.view);

				} else if(marker.type == TEXT_MARKER_ITALIC && font->italicFont) {
					Font* f = marker.enable ? font->italicFont : font;
					dxFlush();
					dxSetTexture(f->tex.view);
				}
				markerIndex++;
			}
		}

		TextInfo ti;
		if(!textSim(td.chars, &tsi, &ti)) break;

		if(settings.cull) {
			if(ti.pos.x > startPos.x + cullWidth) break;
			// if(ti.pos.x > startPos.x + cullWidth-cullOffset) {
			// 	float cullEnd = startPos.x + cullWidth;
			// 	float p = 1 - mapRangeClamp(ti.pos.x, cullEnd - cullOffset, cullEnd - font->height*0.7, 0, 1);
			// 	currentColor.a = settings.color.a * p;
			// 	shadowColor.a = settings.shadowColor.a * p;
			// }
		}
		if(settings.shadowMode != TEXTSHADOW_MODE_NOSHADOW) {
			if(settings.shadowMode == TEXTSHADOW_MODE_SHADOW) {
				Vec2 p = ti.r.min + norm(settings.shadowDir) * settings.shadowSize;
				Rect sr = rectBLDim(vec2(roundf(p.x), roundf(p.y)), ti.r.dim());

				dxPushRect(sr, settings.shadowColor, ti.uv);
			} else if(settings.shadowMode == TEXTSHADOW_MODE_OUTLINE) {
				for(int i = 0; i < 8; i++) {
					// Not sure if we should align to pixels on an outline.

					Vec2 dir = rotate(vec2(1,0), (M_2PI/8)*i);
					Rect r = ti.r.trans(dir*settings.shadowSize);

					dxPushRect(r, settings.shadowColor, ti.uv);
				}
			}
		}

		dxPushRect(ti.r, currentColor, ti.uv);
		dxFlushIfFull();
	}
}
void drawText(char* text, Vec2 startPos, TextSettings settings) {
	return drawText(text, startPos, vec2i(-1,1), 0, settings);
}
void drawText(char* text, Vec2 startPos, Vec2i align, TextSettings settings) {
	return drawText(text, startPos, align, 0, settings);
}

Vec2 textIndexToPos(char* text, Font* font, Vec2 startPos, int index, Vec2i align = vec2i(-1,1), int wrapWidth = 0) {
	ParsedTextData td = parseText(text, font);
	startPos = getTextStartPos(td.chars, font, startPos, align, wrapWidth);

	TextSimInfo tsi = textSimInfo(startPos, font, wrapWidth);
	while(true) {
		TextInfo ti;
		bool done = !textSim(td.chars, &tsi, &ti);

		if(ti.index == index) {
			Vec2 pos = ti.pos - vec2(0, font->height/2);
			return pos;
		}

		if(done) break;
	}

	return vec2(0,0);
}

void drawTextSelection(char* text, Font* font, Vec2 startPos, int index1, int index2, Vec4 color, Vec2i align = vec2i(-1,1), int wrapWidth = 0) {
	if(index1 == index2) return;
	if(index1 > index2) swap(&index1, &index2);

	ParsedTextData td = parseText(text, font);

	startPos = getTextStartPos(td.chars, font, startPos, align, wrapWidth);

	Vec2 lineStart;
	bool drawCurrentTextLine = false;

	TextSimInfo tsi = textSimInfo(startPos, font, wrapWidth);
	while(true) {
		TextInfo ti;
		bool done = !textSim(td.chars, &tsi, &ti);

		if(drawCurrentTextLine) {
			bool endReached = ti.index == index2;
			if(ti.lineBreak || endReached) {
				Vec2 lineEnd;
				if(ti.lineBreak) lineEnd = ti.breakPos;
				else if(done) lineEnd = tsi.pos;
				else lineEnd = ti.pos;

				Rect r = rect(lineStart - vec2(0, font->height), lineEnd);
				dxDrawRect(r, color);

				lineStart = ti.pos;

				if(endReached) break;
			}
		}

		if(!drawCurrentTextLine && (ti.index >= index1)) {
			drawCurrentTextLine = true;
			lineStart = ti.pos;
		}

		if(done) break;
	}
}

int textMouseToIndex(char* text, Font* font, Vec2 startPos, Vec2 mousePos, Vec2i align = vec2i(-1,1), int wrapWidth = 0) {
	ParsedTextData td = parseText(text, font);
	startPos = getTextStartPos(td.chars, font, startPos, align, wrapWidth);

	if(mousePos.y > startPos.y) return 0;
	
	bool foundLine = false;
	TextSimInfo tsi = textSimInfo(startPos, font, wrapWidth);
	while(true) {
		TextInfo ti;
		bool done = !textSim(td.chars, &tsi, &ti);
		
		bool fLine = between(mousePos.y, ti.pos.y - font->height, ti.pos.y);
		if(fLine) foundLine = true;
		else if(foundLine) return ti.index-1;

		if(foundLine) {
			float posAdvanceX = tsi.pos.x - ti.pos.x;
			float charMid = ti.pos.x + posAdvanceX*0.5f;
			if(mousePos.x < charMid) return ti.index;
		}

		if(done) break;
	}

	return tsi.index;
}

// char* textSelectionToString(char* text, int index1, int index2) {
// 	myAssert(index1 >= 0 && index2 >= 0);

// 	int range = abs(index1 - index2);
// 	char* str = getTStringDebug(range + 1); // We assume text selection will only be used for debug things.
// 	strCpy(str, text + minInt(index1, index2), range);
// 	return str;
// }

// char* textSelectionToString(char* text, int index1, int index2) {
// 	assert(index1 >= 0 && index2 >= 0);

// 	int range = abs(index1 - index2);
// 	char* str = getTStringDebug(range + 1); // We assume text selection will only be used for debug things.
// 	strCpy(str, text + min(index1, index2), range);
// 	return str;
// }
