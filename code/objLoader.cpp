
struct ObjLoader {
	struct ObjectMaterial {
		char* name;

		Vec3 Ka;   // ambient
		Vec3 Kd;   // diffuse
		Vec3 Ks;   // specular
		float Ns;  // specularExponent
		Vec3 Ke;   // emission
		float Ni;  // refractionIndex
		float d;   // alpha
		int illum; // illuminationMode
		// Vec3 Tf;   // transmission filter

		char* map_Ka;
		char* map_Kd;
		char* map_Ks;
		char* bump;
		char* disp;

		// map_Ka -s 1 1 1 -o 0 0 0 -mm 0 1 chrome.mpc
		// map_Kd -s 1 1 1 -o 0 0 0 -mm 0 1 chrome.mpc
		// map_Ks -s 1 1 1 -o 0 0 0 -mm 0 1 chrome.mpc
		// map_Ns -s 1 1 1 -o 0 0 0 -mm 0 1 wisp.mps
		// map_d -s 1 1 1 -o 0 0 0 -mm 0 1 wisp.mps
		// disp -s 1 1 .5 wisp.mps
		// decal -s 1 1 1 -o 0 0 0 -mm 0 1 sand.mps
		// bump -s 1 1 1 -o 0 0 0 -bm 1 sand.mpb
	};

	struct Object {
		char* name;
		int offset;
		int size;

		int material;
		bool smoothing;
	};

	struct TangentBitangent {
		Vec3 tangent;
		Vec3 bitangent;
	};

	struct Bone {
		int depth;
		char* name;
	};

	struct SkinInfo {
		int indices[4];
		Vec4 weights;
	};

	struct XForm {
		Vec3 translation;
		Vec3 rotation;
		Vec3 scale;
	};

	struct Animation {
		char* name;

		int startTime;
		int endTime;
		float speed;
		float fps;
		int playbackMode;

		Bone* bones;
		int boneCount;

		XForm** frames;
		int frameCount;
	};

	DArray<Vec3> vertexArray;
	DArray<TangentBitangent> tangentArray;
	DArray<Vec2> uvArray;
	DArray<Vec3> normalArray;
	DArray<ObjectMaterial> materialArray;
	DArray<Object> objectArray;
	MeshVertex* tempVerts;
	int* tempIndices;

	DArray<SkinInfo> skinArray;
	DArray<Bone> boneArray;
	DArray<XForm> poseArray;
	Animation animations[10];
	int animationCount;

	DArray<int> indexBuffer;
	DArray<MeshVertex> vertexBuffer;

	int vertexIndex;
	int normalIndex;

	void parseMtl(char* folder, char* fileName) {
		char* objFolder = getTStringCpy(fileName, strFindBackwards(fileName, '\\'));

		char* finalFolder = fillString("%s%s", folder, fileName);
		
		char* file = readFileToBufferZeroTerminated(finalFolder);
		defer{::free(file);};

		int lineCountMtl = 0;

		char* buf = file;

		while(true) {

			char c = buf[0];
			char c2 = buf[1];

			switch(c) {
				case 'n': {
					// "newmtl"
					buf += 7;

					ObjectMaterial m = {};
					m.d = 1;

					materialArray.push(m);

					char* fileName = getTStringCpy(buf, endOfLine(buf));
					materialArray[materialArray.count-1].name = fileName;
				} break;

				case 'N': {
					buf += 3;
					if(c2 == 's') {
						materialArray[materialArray.count-1].Ns = strToFloat(buf);
					} else if(c2 == 'i') {
						materialArray[materialArray.count-1].Ni = strToFloat(buf);
					}
				} break;

				case 'd': {
					if(c2 == ' ') {
						buf += 2;
						materialArray[materialArray.count-1].d = strToFloat(buf);

					} else if(c2 == 'i') {
						// disp
						buf += 5;

						char* fileName = getTStringCpy(buf, endOfLine(buf));
						materialArray[materialArray.count-1].disp = fillString("%s%s", objFolder, fileName);
					}

				} break;

				case 'i': {
					// "illum"
					buf += 6;
					materialArray[materialArray.count-1].illum = strToInt(buf);
				} break;

				case 'K': {
					if(c2 == ' ') buf += 2;
					else buf += 3;

					Vec3 v;
					v.x = strToFloat(buf); buf += strFind(buf, ' ');
					v.y = strToFloat(buf); buf += strFind(buf, ' ');
					v.z = strToFloat(buf);

					     if(c2 == 'a') materialArray[materialArray.count-1].Ka = v;
					else if(c2 == 'd') materialArray[materialArray.count-1].Kd = v;
					else if(c2 == 's') materialArray[materialArray.count-1].Ks = v;
					else if(c2 == 'e') materialArray[materialArray.count-1].Ke = v;
				} break;

				case 'm': {
					if(buf[4] == 'K' && buf[5] == 'a') {
						// map_Ka
						buf += 7;

						char* fileName = getTStringCpy(buf, endOfLine(buf));
						materialArray[materialArray.count-1].map_Ka = fillString("%s%s", objFolder, fileName);

					} else if(buf[4] == 'K' && buf[5] == 'd') {
						// map_Kd
						buf += 7;

						char* fileName = getTStringCpy(buf, endOfLine(buf));
						materialArray[materialArray.count-1].map_Kd = fillString("%s%s", objFolder, fileName);

					} else if(buf[4] == 'N' && buf[5] == 's') {
						// map_Ks
						buf += 7;

						char* fileName = getTStringCpy(buf, endOfLine(buf));
						materialArray[materialArray.count-1].map_Ks = fillString("%s%s", objFolder, fileName);
					}
				} break;

				case 'b': {
					// bump
					buf += 5;

					char* fileName = getTStringCpy(buf, endOfLine(buf));
					materialArray[materialArray.count-1].bump = fillString("%s%s", objFolder, fileName);
				} break;
			}

			int pos = strFind(buf, '\n');
			if(pos != -1) buf += pos;
			else break;

			lineCountMtl++;
		}

	}

	void parse(char* folder, char* fileName, bool swapTriangleWinding) {
		*this = {};

		if(vertexArray.data != 0) clear();

		char* fName = fillString("%s%s", folder, fileName);
		char* file = readFileToBufferZeroTerminated(fName);
		defer{::free(file);};

		int lineCount = 0;
		tempVerts = getTArray(MeshVertex, 100); // Oughta be enough.
		tempIndices = getTArray(int, 100); 

		addObjectIfChange();

		char* buf = file;

		while(true) {

			char c = buf[0];
			char c2 = buf[1];

			switch(c) {
				case '#': {
				} break;

				case 'm': {
					// "mtllib"
					buf += 7;

					int size = endOfLine(buf);
					char* mtlFileName = getTStringCpy(buf, size);
					char* objFolder = getTStringCpy(fileName, strFindBackwards(fileName, '\\'));

					char* folderAndFile = fillString("%s%s", objFolder, mtlFileName);
					parseMtl(folder, folderAndFile);

					buf += size;

				} break;

				case 'u': {
					// "usemtl"
					buf += 7;

					int size = endOfLine(buf);
					char* materialName = getTStringCpy(buf, size);
					buf += size;

					addObjectIfChange();

					int materialId = -1;
					for(int i = 0; i < materialArray.count; i++) {
						ObjectMaterial* m = materialArray.atr(i);
						if(strCompare(m->name, materialName)) {
							materialId = i;
							break;
						}
					}

					objectArray[objectArray.count-1].material = materialId;

				} break;

				case 's': {
					buf += 2;

					addObjectIfChange();

					// We assume that 's' comes after "usemtl".
					bool smoothing = !(buf[0] == '0' || buf[0] == 'o');
					objectArray[objectArray.count-1].smoothing = smoothing;

				} break;

				case 'o': {
					buf += 2;

				} break;

				case 'v': {
					if(c2 == ' ') buf += 2;
					else buf += 3;

					if(c2 == 't') {
						Vec2 v;
						v.x = strToFloat(buf);
						buf += strFind(buf, ' ');
						v.y = strToFloat(buf);

						uvArray.push(v);
						break;
					} 

					Vec3 v;
					v.x = strToFloat(buf);
					buf += strFind(buf, ' ');
					v.y = strToFloat(buf);
					buf += strFind(buf, ' ');
					v.z = strToFloat(buf);

					if(c2 == 'n') normalArray.push(v);
					else vertexArray.push(v);

				} break;

				case 'f': {
					buf++;

					MeshVertex verts[6] = {};

					int count = 0;
					while(true) {
						buf = eatWhitespace(buf);
						if(!charIsDigitOrMinus(buf[0])) break;

						Vec3i index = {0,0,0};

						index.x = strToInt(buf);

						while(charIsDigitOrMinus(buf[0])) buf++;

						if(buf[0] == '/') {
							buf++;

							if(buf[0] != '/') {
								index.y = strToInt(buf);
								while(charIsDigitOrMinus(buf[0])) buf++;
							}

							if(buf[0] == '/') {
								buf++;
								index.z = strToInt(buf);
								while(charIsDigitOrMinus(buf[0])) buf++;
							}
						}

						if(index.y == 0) index.y = index.x;
						if(index.z == 0) index.z = index.x;

						{
							if(index.x < 0) index.x = vertexArray.count + index.x + 1;
							tempVerts[count].pos = vertexArray[index.x-1];

							tempIndices[count] = index.x-1;

							if(index.y < 0) index.y = uvArray.count + index.y + 1;
							if(uvArray.count >= index.y)
								tempVerts[count].uv = uvArray[index.y-1];

							if(index.z < 0) index.z = normalArray.count + index.z + 1;
							if(normalArray.count >= index.z)
								tempVerts[count].normal = normalArray[index.z-1];
						}

						count++;
					}

					if(!swapTriangleWinding) {
						for(int i = 1; i < count-1; i++) {
							vertexBuffer.push(tempVerts[0]);
							vertexBuffer.push(tempVerts[i]);
							vertexBuffer.push(tempVerts[i+1]);

							indexBuffer.push(tempIndices[0]);
							indexBuffer.push(tempIndices[i]);
							indexBuffer.push(tempIndices[i+1]);
						}

					} else {
						for(int i = 1; i < count-1; i++) {
							vertexBuffer.push(tempVerts[0]);
							vertexBuffer.push(tempVerts[i+1]);
							vertexBuffer.push(tempVerts[i]);

							indexBuffer.push(tempIndices[0]);
							indexBuffer.push(tempIndices[i+1]);
							indexBuffer.push(tempIndices[i]);
						}
					}
					
				} break;

				default: {
					// assert(false);
				}
			}

			bool done = false;

			int pos = strFind(buf, '\n');
			if(pos != -1) buf += pos;
			else done = true;

			if(done) {
				// Set object sizes.
				for(int i = 0; i < objectArray.count; i++) {
					Object* o = &objectArray[i];
					if(i == objectArray.count-1) {
						o->size = vertexBuffer.count - o->offset;
					} else {
						o->size = objectArray[i+1].offset - o->offset;
					}
				}

				// Calculate tangents/bitangents.
				// If smoothShading is enabled we average them.

				tangentArray.reserve(vertexBuffer.count);
				tangentArray.clear();

				for(int i = 0; i < objectArray.count; i++) {
					Object* o = &objectArray[i];
					int start = o->offset;
					int count = o->size;

					bool smoothing = o->smoothing;

					tangentArray.clear();
					memset(tangentArray.data, 0, sizeof(TangentBitangent) * vertexBuffer.count);

					int end = start + count;
					for(int i = start; i < end; i += 3) {
						Vec3 v0 = vertexBuffer[i  ].pos;
						Vec3 v1 = vertexBuffer[i+1].pos;
						Vec3 v2 = vertexBuffer[i+2].pos;

						Vec2 uv0 = vertexBuffer[i  ].uv;
						Vec2 uv1 = vertexBuffer[i+1].uv;
						Vec2 uv2 = vertexBuffer[i+2].uv;

						Vec3 deltaPos1 = v1 - v0;
						Vec3 deltaPos2 = v2 - v0;

						Vec2 deltaUV1 = uv1 - uv0;
						Vec2 deltaUV2 = uv2 - uv0;

						float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
						Vec3 tangent   = (deltaPos1  * deltaUV2.y - deltaPos2  * deltaUV1.y) * r;
						Vec3 bitangent = (deltaPos2  * deltaUV1.x - deltaPos1  * deltaUV2.x) * r;


						Vec3 n0 = vertexBuffer[i  ].normal;
						Vec3 n1 = vertexBuffer[i+1].normal;
						Vec3 n2 = vertexBuffer[i+2].normal;

						Vec3 t0 = norm(tangent - n0 * dot(n0, tangent));
						Vec3 t1 = norm(tangent - n1 * dot(n1, tangent));
						Vec3 t2 = norm(tangent - n2 * dot(n2, tangent));

						if(dot(cross(n0, t0), bitangent) < 0.0f) t0 *= -1.0f;
						if(dot(cross(n1, t1), bitangent) < 0.0f) t1 *= -1.0f;
						if(dot(cross(n2, t2), bitangent) < 0.0f) t2 *= -1.0f;

						if(smoothing) {
							tangentArray[indexBuffer[i  ]].tangent += t0;
							tangentArray[indexBuffer[i+1]].tangent += t1;
							tangentArray[indexBuffer[i+2]].tangent += t2;

							tangentArray[indexBuffer[i  ]].bitangent += bitangent;
							tangentArray[indexBuffer[i+1]].bitangent += bitangent;
							tangentArray[indexBuffer[i+2]].bitangent += bitangent;

						} else {
							vertexBuffer[i  ].tangent = t0;
							vertexBuffer[i+1].tangent = t1;
							vertexBuffer[i+2].tangent = t2;

							vertexBuffer[i  ].bitangent = bitangent;
							vertexBuffer[i+1].bitangent = bitangent;
							vertexBuffer[i+2].bitangent = bitangent;
						}
					}

					if(smoothing) {
						for(int i = start; i < end; i++) {
							int index = indexBuffer[i];
							TangentBitangent tb = tangentArray[index];
							vertexBuffer[i].tangent = tb.tangent;
							vertexBuffer[i].bitangent = tb.bitangent;
						}
					}
				}

				break;
			}

			lineCount++;
		}
	}

	void parseCustom(char* folder, char* fileName, bool swapTriangleWinding) {
		*this = {};

		if(vertexArray.data != 0) clear();

		char* fName = fillString("%s%s", folder, fileName);
		char* file = readFileToBufferZeroTerminated(fName);
		defer{::free(file);};

		int lineCount = 0;
		tempVerts = getTArray(MeshVertex, 100); // Oughta be enough.
		tempIndices = getTArray(int, 100);

		addObjectIfChange();

		char* buf = file;

		//

		{
			int boneCount;

			// Bones.
			{
				buf += strLen("Bones: ");
				boneCount = strToInt(buf); buf += strFind(buf, '\n');

				for(int i = 0; i < boneCount; i++) {
					int depth = strToInt(buf); buf += strFind(buf, ' ');
					char* name = getTStringCpy(buf, endOfLine(buf)); buf += strFind(buf, '\n');

					Bone bone = {depth, name};
					boneArray.push(bone);
				}
			}

			// Pose.
			{
				buf += strLen("Pose: "); buf += strFind(buf, '\n');

				for(int i = 0; i < boneCount; i++) {
					Vec3 translation;
					translation.x = strToFloat(buf); buf += strFind(buf, ' ');
					translation.y = strToFloat(buf); buf += strFind(buf, ' ');
					translation.z = strToFloat(buf); buf += strFind(buf, '\n');

					Vec3 rotation;
					rotation.x = strToFloat(buf); buf += strFind(buf, ' ');
					rotation.y = strToFloat(buf); buf += strFind(buf, ' ');
					rotation.z = strToFloat(buf); buf += strFind(buf, '\n');

					Vec3 scale;
					scale.x = strToFloat(buf); buf += strFind(buf, ' ');
					scale.y = strToFloat(buf); buf += strFind(buf, ' ');
					scale.z = strToFloat(buf); buf += strFind(buf, '\n');

					XForm form = {translation, rotation, scale};
					poseArray.push(form);

					buf += strFind(buf, '\n');
				}
			}
		}

		buf += strLen("Count: ");
		int objectCount = strToInt(buf);
		while(charIsDigitOrMinus(buf[0])) buf++;

		vertexIndex = 0;
		normalIndex = 0;

		for(int objectIndex = 0; objectIndex < objectCount; objectIndex++) {

			int vertCount;
			int normalCount;

			// Verts.
			{
				buf += strLen("Verts: ");
				vertCount = strToInt(buf);
				buf += strFind(buf, '\n');

				for(int i = 0; i < vertCount; i++) {
					Vec3 v;
					v.x = strToFloat(buf); buf += strFind(buf, ' ');
					v.y = strToFloat(buf); buf += strFind(buf, ' ');
					v.z = strToFloat(buf);

					// TEMP!!!!
					v /= 100.0f;

					vertexArray.push(v);

					buf += strFind(buf, '\n');
				}
			}

			// Normals.
			{
				buf += strLen("Normals: ");
				normalCount = strToInt(buf);
				buf += strFind(buf, '\n');

				for(int i = 0; i < normalCount; i++) {
					Vec3 v;
					v.x = strToFloat(buf); buf += strFind(buf, ' ');
					v.y = strToFloat(buf); buf += strFind(buf, ' ');
					v.z = strToFloat(buf);

					normalArray.push(v);

					buf += strFind(buf, '\n');
				}
			}

			// Skin.
			{
				buf += strLen("Skin: ");
				int skinCount = strToInt(buf);
				buf += strFind(buf, '\n');

				for(int i = 0; i < skinCount; i++) {
					SkinInfo skinInfo = {};
					int index = 0;

					while(true) {
						int nameSize = strFind(buf, ' ');

						// Slow, we check every bone name for every vertex.
						// Should make a hashtable but it's probably fine.

						int boneIndex = -1;
						for(int i = 0; i < boneArray.count; i++) {
							if(strCompare(buf, nameSize-1, boneArray[i].name, strLen(boneArray[i].name))) {
								boneIndex = i;
								break;
							}
						}

						skinInfo.indices[index++] = boneIndex;

						buf += nameSize;

						if(buf[0] == '\n') {
							buf++;
							break;
						}
					}

					for(int i = 0; i < index; i++) {
						skinInfo.weights.e[i] = strToFloat(buf); buf += strFind(buf, ' ');
					}

					skinArray.push(skinInfo);

					buf += strFind(buf, '\n');
				}
			}

			// Name.
			{
				buf += strLen("Node: ");
				int end = strFind(buf, '\n');

				buf += end;

				addObjectIfChange();
			}

			// Faces.
			{
				buf += strLen("Faces: ");
				int faceCount = strToInt(buf);
				buf += strFind(buf, '\n');

				for(int i = 0; i < faceCount; i++) {
					int count = 0;

					while(true) {
						buf = eatWhitespace(buf);
						if(!charIsDigitOrMinus(buf[0])) break;

						Vec3i index = {0,0,0};

						index.x = strToInt(buf);

						while(charIsDigitOrMinus(buf[0])) buf++;

						if(buf[0] == '/') {
							buf++;

							if(buf[0] != '/') {
								index.y = strToInt(buf);
								while(charIsDigitOrMinus(buf[0])) buf++;
							}

							if(buf[0] == '/') {
								buf++;
								index.z = strToInt(buf);
								while(charIsDigitOrMinus(buf[0])) buf++;
							}
						}

						if(index.y == 0) index.y = index.x;
						if(index.z == 0) index.z = index.x;

						{
							if(index.x < 0) index.x = vertexArray.count + index.x + 1;
							tempVerts[count].pos = vertexArray[vertexIndex + index.x-1];
							tempVerts[count].blendWeights = skinArray[vertexIndex + index.x-1].weights;
							for(int i = 0; i < 4; i++)
								tempVerts[count].blendIndices[i] = skinArray[vertexIndex + index.x-1].indices[i];

							tempIndices[count] = index.x-1;

							if(index.y < 0) index.y = uvArray.count + index.y + 1;
							if(uvArray.count >= index.y)
								tempVerts[count].uv = uvArray[index.y-1];

							if(index.z < 0) index.z = normalArray.count + index.z + 1;
							if(normalArray.count >= index.z)
								tempVerts[count].normal = normalArray[normalIndex + index.z-1];
						}

						count++;
					}

					if(!swapTriangleWinding) {
						for(int i = 1; i < count-1; i++) {
							vertexBuffer.push(tempVerts[0]);
							vertexBuffer.push(tempVerts[i]);
							vertexBuffer.push(tempVerts[i+1]);

							indexBuffer.push(tempIndices[0]);
							indexBuffer.push(tempIndices[i]);
							indexBuffer.push(tempIndices[i+1]);
						}

					} else {
						for(int i = 1; i < count-1; i++) {
							vertexBuffer.push(tempVerts[0]);
							vertexBuffer.push(tempVerts[i+1]);
							vertexBuffer.push(tempVerts[i]);

							indexBuffer.push(tempIndices[0]);
							indexBuffer.push(tempIndices[i+1]);
							indexBuffer.push(tempIndices[i]);
						}
					}

					buf += strFind(buf, '\n');
				}
			}

			vertexIndex += vertCount;
			normalIndex += normalCount;
		}

		{
			// Set object sizes.
			for(int i = 0; i < objectArray.count; i++) {
				Object* o = &objectArray[i];
				if(i == objectArray.count-1) {
					o->size = vertexBuffer.count - o->offset;
				} else {
					o->size = objectArray[i+1].offset - o->offset;
				}
			}

			// Calculate tangents/bitangents.
			// If smoothShading is enabled we average them.

			tangentArray.reserve(vertexBuffer.count);
			tangentArray.clear();

			for(int i = 0; i < objectArray.count; i++) {
				Object* o = &objectArray[i];
				int start = o->offset;
				int count = o->size;

				bool smoothing = o->smoothing;

				tangentArray.clear();
				memset(tangentArray.data, 0, sizeof(TangentBitangent) * vertexBuffer.count);

				int end = start + count;
				for(int i = start; i < end; i += 3) {
					Vec3 v0 = vertexBuffer[i  ].pos;
					Vec3 v1 = vertexBuffer[i+1].pos;
					Vec3 v2 = vertexBuffer[i+2].pos;

					Vec2 uv0 = vertexBuffer[i  ].uv;
					Vec2 uv1 = vertexBuffer[i+1].uv;
					Vec2 uv2 = vertexBuffer[i+2].uv;

					Vec3 deltaPos1 = v1 - v0;
					Vec3 deltaPos2 = v2 - v0;

					Vec2 deltaUV1 = uv1 - uv0;
					Vec2 deltaUV2 = uv2 - uv0;

					float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
					Vec3 tangent   = (deltaPos1  * deltaUV2.y - deltaPos2  * deltaUV1.y) * r;
					Vec3 bitangent = (deltaPos2  * deltaUV1.x - deltaPos1  * deltaUV2.x) * r;


					Vec3 n0 = vertexBuffer[i  ].normal;
					Vec3 n1 = vertexBuffer[i+1].normal;
					Vec3 n2 = vertexBuffer[i+2].normal;

					Vec3 t0 = norm(tangent - n0 * dot(n0, tangent));
					Vec3 t1 = norm(tangent - n1 * dot(n1, tangent));
					Vec3 t2 = norm(tangent - n2 * dot(n2, tangent));

					if(dot(cross(n0, t0), bitangent) < 0.0f) t0 *= -1.0f;
					if(dot(cross(n1, t1), bitangent) < 0.0f) t1 *= -1.0f;
					if(dot(cross(n2, t2), bitangent) < 0.0f) t2 *= -1.0f;

					if(smoothing) {
						tangentArray[indexBuffer[i  ]].tangent += t0;
						tangentArray[indexBuffer[i+1]].tangent += t1;
						tangentArray[indexBuffer[i+2]].tangent += t2;

						tangentArray[indexBuffer[i  ]].bitangent += bitangent;
						tangentArray[indexBuffer[i+1]].bitangent += bitangent;
						tangentArray[indexBuffer[i+2]].bitangent += bitangent;

					} else {
						vertexBuffer[i  ].tangent = t0;
						vertexBuffer[i+1].tangent = t1;
						vertexBuffer[i+2].tangent = t2;

						vertexBuffer[i  ].bitangent = bitangent;
						vertexBuffer[i+1].bitangent = bitangent;
						vertexBuffer[i+2].bitangent = bitangent;
					}
				}

				if(smoothing) {
					for(int i = start; i < end; i++) {
						int index = indexBuffer[i];
						TangentBitangent tb = tangentArray[index];
						vertexBuffer[i].tangent = tb.tangent;
						vertexBuffer[i].bitangent = tb.bitangent;
					}
				}
			}
		}

		// Search the folder for animations.
		// "..\data\Meshes\figure\custom.mesh"

		// Get folder from file path.
		int pos = strFindBackwards(fName, '\\');
		char* folderPath = getTStringCpy(fName, pos);

		FolderSearchData fd;
		folderSearchStart(&fd, fillString("%s*", folderPath));
		while(folderSearchNextFile(&fd)) {

			if(strFind(fd.fileName, ".anim") == -1) continue;

			parseCustomAnim(folderPath, fd.fileName);
		}
	}

	void parseCustomAnim(char* folder, char* fileName) {
		char* fName = fillString("%s%s", folder, fileName);
		char* file = readFileToBufferZeroTerminated(fName);
		defer{::free(file);};

		int lineCount = 0;

		char* buf = file;

		//

		Animation* anim = &animations[animationCount++];

		anim->name = getTStringCpy(fileName);

		buf += strLen("StartTime: ");
		anim->startTime = strToInt(buf); buf += strFind(buf, '\n');
		buf += strLen("EndTime: ");
		anim->endTime = strToInt(buf); buf += strFind(buf, '\n');
		buf += strLen("Speed: ");
		anim->speed = strToFloat(buf); buf += strFind(buf, '\n');
		buf += strLen("Fps: ");
		anim->fps = strToFloat(buf); buf += strFind(buf, '\n');
		buf += strLen("Loop: ");
		// anim->playbackMode = strToInt(buf); 
		buf += strFind(buf, '\n');

		buf += strFind(buf, '\n');

		int boneCount = 0;

		// Bones.
		{
			buf += strLen("Bones: ");
			boneCount = strToInt(buf); buf += strFind(buf, '\n');

			anim->bones = getTArray(Bone, boneCount);
			anim->boneCount = boneCount;

			for(int i = 0; i < boneCount; i++) {
				int depth = strToInt(buf); buf += strFind(buf, ' ');
				char* name = getTStringCpy(buf, endOfLine(buf)); buf += strFind(buf, '\n');

				Bone bone = {depth, name};
				anim->bones[i] = bone;
			}
		}

		buf += strLen("Frames: ");
		anim->frameCount = strToInt(buf); buf += strFind(buf, '\n');

		anim->frames = getTArray(XForm*, anim->frameCount);

		for(int frameIndex = 0; frameIndex < anim->frameCount; frameIndex++) {
			anim->frames[frameIndex] = getTArray(XForm, boneCount);

			// Frame pose.
			buf += strLen("Frame: "); buf += strFind(buf, '\n');

			for(int i = 0; i < boneCount; i++) {
				Vec3 translation;
				translation.x = strToFloat(buf); buf += strFind(buf, ' ');
				translation.y = strToFloat(buf); buf += strFind(buf, ' ');
				translation.z = strToFloat(buf); buf += strFind(buf, '\n');

				Vec3 rotation;
				rotation.x = strToFloat(buf); buf += strFind(buf, ' ');
				rotation.y = strToFloat(buf); buf += strFind(buf, ' ');
				rotation.z = strToFloat(buf); buf += strFind(buf, '\n');

				Vec3 scale;
				scale.x = strToFloat(buf); buf += strFind(buf, ' ');
				scale.y = strToFloat(buf); buf += strFind(buf, ' ');
				scale.z = strToFloat(buf); buf += strFind(buf, '\n');

				XForm form = {translation, rotation, scale};
				anim->frames[frameIndex][i] = form;

				buf += strFind(buf, '\n');
			}
		}
	}

	void addObjectIfChange() {
		bool push = false;
		if(objectArray.count == 0) push = true;
		else {
			if(objectArray[objectArray.count-1].offset != vertexBuffer.count)
				push = true;
		}

		if(push) {
			Object o = {};
			o.offset = vertexBuffer.count;
			o.material = -1;
			objectArray.push(o);
		}
	};

	// bool nextLine() {
	// 	int pos = strFind(buf, '\n');
	// 	if(pos != -1) {
	// 		buf += pos;
	// 		return true
	// 	} else {
	// 		return false;
	// 	}
	// }

	int parseName(char* buf) {
		char* bufStart = buf;
		while(charIsDigitOrLetter(buf[0]) || 
		      buf[0] == '.' || buf[0] == '_') buf++;

		return buf - bufStart;
	}

	int endOfLine(char* buf) {
		int newLine = strFind(buf, '\n')-1;
		if(newLine < 0) newLine = strLen(buf);

		if(buf[newLine-1] == '\r') newLine--;
		return newLine;
	}

	char* eatWhitespace(char* buf) {
		// while(buf[0] == ' ' || buf[0] == '\\') buf++;

		while(true) {
			     if(buf[0] == ' ' ) buf++;
			else if(buf[0] == '\\') buf += 3;
			else break;
		}

		return buf;
	}

	inline bool charIsDigit(char c) {
		return (c >= (int)'0') && (c <= (int)'9');
	}

	inline bool charIsDigitOrMinus(char c) {
		return charIsDigit(c) || c == '-';
	}

	bool charIsLetter(char c) {
		return ((c >= (int)'a') && (c <= (int)'z') || 
		        (c >= (int)'A') && (c <= (int)'Z'));
	}

	bool charIsDigitOrLetter(char c) {
		return charIsDigit(c) || charIsLetter(c);
	}

	void clear() {
		vertexArray.clear();
		uvArray.clear();
		normalArray.clear();
		materialArray.clear();
		vertexBuffer.clear();
		objectArray.clear();
		tangentArray.clear();
		indexBuffer.clear();
	}

	void free() {
		vertexArray.dealloc();
		uvArray.dealloc();
		normalArray.dealloc();
		materialArray.dealloc();
		vertexBuffer.dealloc();
		objectArray.dealloc();
		tangentArray.dealloc();
		indexBuffer.dealloc();
	}
};