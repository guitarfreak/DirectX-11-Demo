
enum FunctionType {
	FUNC_CONST = 0,
	FUNC_LINEAR,
	FUNC_QUADRATIC,
	FUNC_CUBIC,
	FUNC_QUARTIC,
	FUNC_TWO_DICE,
	FUNC_THREE_DICE,
	FUNC_FOUR_DICE,
	FUNC_TWO_DICE_SQUARED,
	FUNC_THREE_DICE_SQUARED,
	FUNC_FOUR_DICE_SQUARED,
	FUNC_EXTREMES,

	FUNC_SIZE,
};

char* FunctionTypeStrings[] = {
	"Const",
	"Linear",
	"Quadratic",
	"Cubic",
	"Quartic",
	"Two Dice",
	"Three Dice",
	"Four Dice",
	"Two Dice Squared",
	"Three Dice Squared",
	"Four Dice Squared",
	"Extremes",
};

template <class T>
struct ValueRange {
	T min;
	T max;
	int functionType;

	float getSample();
};

struct Particle {
	Vec3 pos;
	Vec3 vel;
	float drag;

	Vec4 color[3];

	float size;
	float velSize;

	float rot;
	float velRot;

	float lifeTime;
	float time;

	//

	float distToCam;
};

struct ParticleSettings {

	enum SpawnRegionType {
		REGION_SPHERE = 0,
		REGION_CYLINDER,
		REGION_BOX,

		REGION_SIZE,
	};

	struct SpawnRegion {
		int type;

		union {
			struct {
				float radius;
				float radiusMin;
				float height;
			};

			struct {
				Vec3 dim;
			};
		};
	};

	char* texture;
	int spriteIndex;
	SpawnRegion region;

	float spawnRate; // Particles per second.
	float lifeTime;
	float startTime;
	float restTime;
	ValueRange<float> lifeTimeP;
	float fadeInTime;
	float fadeOutTime;

	ValueRange<float> size;
	ValueRange<float> velSize;
	ValueRange<float> angleV;
	ValueRange<float> angleH;
	ValueRange<float> speed;
	Vec3 gravity;
	ValueRange<float> drag;

	bool velocityStretch;
	float velocityStretchMod;

	ValueRange<float> velRot;
	bool randomRotation;

	float colorT; // In percent.
	ValueRange<Vec4> color[3]; // Function is ignored on color 2/3.

	float alpha;
	float brightness;
};

struct ParticleGroup;
struct ParticleEmitter {
	bool initialized;

	Particle* particles;
	int particleSize;
	int particleCount;

	ParticleGroup* group;

	XForm xForm;

	//

	bool loop;
	bool localSpace; // Not working yet.

	ParticleSettings settings;

	//

	float time;
	float spawnTime;
	float startTime;
	float finishTime;
	bool starting;
	bool finished;

	//

	void init(int size);
	void particleSim(Particle* p, float dt);
	void update(float dt, Camera* cam);
	void draw(Camera* cam);
};

/*
ToDo:
- Color hsl interpolation.
- Drag based on size.
- Color bezier graph selector.
- Current lighting/shadowing isn't very good.
- Localspace.
*/

//

template <class T>
ValueRange<T> valueRange(T min, T max, int functionType) {
	return { min, max, functionType };
}

template <class T>
ValueRange<T> valueRange(T min, T max) {
	return { min, max, FUNC_LINEAR };
}

template <class T>
ValueRange<T> valueRange(T min) {
	return { min, min, FUNC_CONST };
}

template <class T> float ValueRange<T>::getSample() {
	if(functionType == FUNC_CONST) return 0.0f;

	float v = randomFloat01();

	switch(functionType) {
		case FUNC_LINEAR: break;

		case FUNC_QUADRATIC: {
			v = v*v;
		} break;

		case FUNC_CUBIC: {
			v = v*v*v;
		} break;

		case FUNC_QUARTIC: {
			v = v*v*v*v;
		} break;

		case FUNC_TWO_DICE: {
			float v2 = randomFloat01();
			v = (v + v2) / 2.0f;
		} break;

		case FUNC_THREE_DICE: {
			float v2 = randomFloat01();
			float v3 = randomFloat01();
			v = (v + v2 + v3) / 3.0f;
		} break;

		case FUNC_FOUR_DICE: {
			float v2 = randomFloat01();
			float v3 = randomFloat01();
			float v4 = randomFloat01();
			v = (v + v2 + v3 + v4) / 4.0f;
		} break;

		case FUNC_TWO_DICE_SQUARED: {
			float v2 = randomFloat01();
			v = (v + v2) / 2.0f;
			v = v*v;
		} break;

		case FUNC_THREE_DICE_SQUARED: {
			float v2 = randomFloat01();
			float v3 = randomFloat01();
			v = (v + v2 + v3) / 3.0f;
			v = v*v;
		} break;

		case FUNC_FOUR_DICE_SQUARED: {
			float v2 = randomFloat01();
			float v3 = randomFloat01();
			float v4 = randomFloat01();
			v = (v + v2 + v3 + v4) / 4.0f;
			v = v*v;
		} break;

		case FUNC_EXTREMES: {
			if(randomInt01()) v = v*v*0.5f;
			else v = (1 - v*v)*0.5f + 0.5f;
		} break;
	}

	return v;
}

//

void ParticleEmitter::init(int size) {
	*this = {};

	initialized = true;

	particleSize = size;
	particles = getPArray(Particle, size);
	particleCount = 0;

	this->xForm = ::xForm();

	{
		ParticleSettings s = {};

		// s.texture = getPStringCpy("misc\\particles.dds");
		s.spriteIndex = 0;
		s.region.type = ParticleSettings::REGION_BOX;
		s.region.dim = vec3(1,1,1);

		s.spawnRate   = 400.0f;
		s.lifeTime    = 0;
		s.startTime   = 0.0f;
		s.restTime    = 0.0f;
		s.lifeTimeP   = valueRange( 0.5f, 1.0f, FUNC_LINEAR );
		s.fadeInTime  = 0.2f;
		s.fadeOutTime = 0.5f;

		s.size    = valueRange( 0.04f, 0.08f );
		s.velSize = valueRange( 0.0f );
		s.angleV  = valueRange( 90.0f, 90.0f );
		s.angleH  = valueRange( 0.0f, 360.0f );
		s.speed   = valueRange( 0.0f );
		s.gravity = vec3(0,0,0);
		s.drag    = valueRange( 0.95f, 0.98f );

		s.velocityStretch    = false;
		s.velocityStretchMod = 0.05f;

		s.velRot = valueRange( 0.0f,0.0f );
		s.randomRotation = false;

		s.colorT   = 0.0f;
		s.color[0] = valueRange( vec4(1.0f,0.5f,0, 1), vec4(0.8,0.4f, 0.0f,1), FUNC_LINEAR );

		s.alpha = 1.0f;
		s.brightness = 1.0f;

		this->settings = s;
	}

	loop = true;
	localSpace = false;

	time = 0;
	spawnTime = 0;
}

void ParticleEmitter::particleSim(Particle* p, float dt) {
	p->pos += p->vel * dt;
	p->vel += xForm.scale * settings.gravity * dt;
	p->vel *= pow(p->drag, dt);

	p->size += p->velSize * dt;
	p->size = max(p->size, 0.0f);

	p->rot += p->velRot * dt;

	p->time += dt;
}

void ParticleEmitter::update(float dt, Camera* cam) {
	TIMER_BLOCK();

	// Reset.
	if(finished) {
		if(!loop) return;

		finished = false;

		time = 0;
		spawnTime = 0;
		finishTime = 0;
		startTime = 0;
	}

	// Sim.
	for(int i = 0; i < particleCount; i++) {
		particleSim(particles + i, dt);
	}

	if(startTime < settings.startTime) {
		startTime += dt;
		return;
	}

	time += dt;

	// Spawn.
	if((settings.lifeTime == 0 || (time < settings.lifeTime))) {
		TIMER_BLOCK_NAMED("ParticleSpawn");

		spawnTime += dt;

		float spawnRate = settings.spawnRate > 0 ? (1.0f / settings.spawnRate) : 10000000.0f;
		float spawnCountF = spawnTime / spawnRate;
		int spawnCount = (int)spawnCountF;

		float spawnTimeRest = spawnTime - spawnRate;
		spawnTime = (spawnCountF - spawnCount) * spawnRate;

		if(particleCount + spawnCount > particleSize) {
			spawnCount = particleSize - particleCount;
		}

		for(int i = 0; i < spawnCount; i++) {
			Particle p = {};

			p.lifeTime = lerp(settings.lifeTimeP.getSample(), settings.lifeTimeP.min, settings.lifeTimeP.max);

			Vec3 offset;
			{
				ParticleSettings::SpawnRegion region = settings.region;
				switch(region.type) {
					case ParticleSettings::REGION_SPHERE: {
						Vec3 rp = randomSpherePoint();

						if(!region.radiusMin) {
							offset = rp * region.radius;
						} else {
							offset = rp * (region.radius - region.radiusMin) + norm(rp) * region.radiusMin;
						}
					} break;

					case ParticleSettings::REGION_CYLINDER: {
						Vec3 rp = randomDiskPoint();

						if(!region.radiusMin) {
							offset = rp * region.radius;
						} else {
							offset = rp * (region.radius - region.radiusMin) + norm(rp) * region.radiusMin;
						}

						offset.z += randomOffset(region.height / 2.0f);
					} break;

					case ParticleSettings::REGION_BOX: {
						offset = randomBoxPoint(region.dim / 2.0f);
					} break;
				}
			}

			p.pos = offset;

			Vec3 angle;
			{
				float angleV = lerp(settings.angleV.getSample(), settings.angleV.min, settings.angleV.max);
				float angleH = lerp(settings.angleH.getSample(), settings.angleH.min, settings.angleH.max);
				angle = quatDeg(angleH, vec3(0,0,1)) * 
				        quatDeg(angleV, vec3(1,0,0)) * vec3(0,1,0);
			}

			p.vel = angle * lerp(settings.speed.getSample(), settings.speed.min, settings.speed.max);

			p.size = lerp(settings.size.getSample(), settings.size.min, settings.size.max);
			p.velSize = lerp(settings.velSize.getSample(), settings.velSize.min, settings.velSize.max);

			p.drag = 1 - lerp(settings.drag.getSample(), settings.drag.min, settings.drag.max);

			p.rot = settings.randomRotation ? randomFloat(0,M_2PI) : 0;
			p.velRot = degreeToRadian(lerp(settings.velRot.getSample(), settings.velRot.min, settings.velRot.max));
			p.velRot *= randomInt01() ? 1.0f : -1.0f;

			{
				float t = settings.color[0].getSample();

				for(int i = 0; i < arrayCount(settings.color); i++) {
					p.color[i]  = { lerp(t, settings.color[i].min.r, settings.color[i].max.r), 
					                lerp(t, settings.color[i].min.g, settings.color[i].max.g), 
					                lerp(t, settings.color[i].min.b, settings.color[i].max.b), 
					                lerp(t, settings.color[i].min.a, settings.color[i].max.a), };
				}
			}

			// Modify based on emitter xform.
			{
				p.pos = xForm.trans + xForm.rot * (xForm.scale * p.pos);
				p.vel = xForm.rot * (xForm.scale * p.vel);

				float minScale = min(xForm.scale.x, xForm.scale.y, xForm.scale.z);
				p.size = (minScale * p.size);
			}

			particleSim(&p, spawnTimeRest);
			spawnTimeRest -= spawnRate;

			particles[particleCount++] = p;
		}
	}

	// Remove dead.
	for(int i = 0; i < particleCount; i++) {
		Particle* p = particles + i;

		if(p->time >= p->lifeTime) {
			particles[i] = particles[particleCount-1];
			particleCount--;
			i--;
		}
	}	

	// Sort.
	{
		TIMER_BLOCK_NAMED("ParticleSort");

		for(int i = 0; i < particleCount; i++) {
			Particle* p = particles + i;

			// p->distToCam = len(p->pos - cam->pos);
			p->distToCam = dot(p->pos - cam->pos, cam->look);
		}

		auto cmp = [](void* a, void* b) {
			return ((Particle*)a)->distToCam > ((Particle*)b)->distToCam;
		};

		mergeSort(particles, particleCount, cmp);
	}

	if(settings.lifeTime > 0) {
		// Wait rest time.
		if((time >= settings.lifeTime) && particleCount == 0) finishTime += dt;

		// Reset.
		if(finishTime > settings.restTime) {
			finished = true;
		}
	}
}

void ParticleEmitter::draw(Camera* cam) {
	TIMER_BLOCK();

	dxSetShader(Shader_Particle);

	dxSetBlendState(Blend_State_PreMultipliedAlpha);
	// dxSetBlendState(Blend_State_Add);
	dxDepthTest(false); 
	dxBindFrameBuffer("3dMsaa", 0);
	dxSetTexture(dxGetFrameBuffer("ds3d")->shaderResourceView, 7);

	dxPushShaderConstants(Shader_Particle);

	defer { 
		dxSetBlendState(Blend_State_Blend); 
		dxDepthTest(true); 
		dxBindFrameBuffer("3dMsaa", "ds3d"); 
	};

	{
		// Set texture.
		Rect uv;
		{
			// Texture* tex = dxGetTexture(settings.texture);
			Texture* tex = dxGetTexture("misc\\particles.dds");
			if(tex->spriteSheet) {
				Vec2 tDim = vec2(tex->dim);
				float cellSize = tDim.w / tex->cellDim.w;
				Vec2 p = vec2(1.0f/tex->cellDim.w);
				
				float div = (float)settings.spriteIndex / tex->cellDim.w;
				Vec2 textureCell = vec2((div - (int)div) * tex->cellDim.w, (int)div);

				uv = rectBLDim(textureCell * p, p);
				swap(&uv.bottom, &uv.top);

			} else {
				uv = rect(1,0,0,1);
			}

			dxSetTexture(tex->view, 0);
		}

		PrimitiveVertex* v = dxBeginPrimitive(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		for(int i = 0; i < particleCount; i++) {
			Particle* p = particles + i;

			// Color.

			Vec4 c;
			{
				float timePercent = p->time / p->lifeTime;

				if(settings.colorT != 0) {
					float t;
					Vec4 color[2];
					if(timePercent < settings.colorT) {
						color[0] = p->color[0];
						color[1] = p->color[1];
						t = timePercent / settings.colorT;
					} else {
						color[0] = p->color[1];
						color[1] = p->color[2];
						t = (timePercent - settings.colorT) / (1 - settings.colorT);
					}

					c = { lerp(t, color[0].r, color[1].r), 
			            lerp(t, color[0].g, color[1].g), 
			            lerp(t, color[0].b, color[1].b), 
			            lerp(t, color[0].a, color[1].a), };

				} else {
					c = p->color[0];
				}

				if(p->time < settings.fadeInTime)
					c.a *= p->time / settings.fadeInTime;
				else if(p->lifeTime - p->time < settings.fadeOutTime) 
					c.a *= (p->lifeTime - p->time) / settings.fadeOutTime;

				c.a *= settings.alpha;
				c.rgb *= settings.brightness;
			}

			Vec3 right, up;
			{
				if(settings.velocityStretch) {
					// Vec3 vel = p->vel * settings.velocityStretchMod;
					// right = vel - cam->look * dot(cam->look, vel);

					// // Clamp right to pSize.
					// // if(len(right) < p->size) right = norm(right) * p->size;
					// // up    = quat(-M_PI_2, cam->look) * (norm(right)*p->size);

					// // Clamp up to right.
					// // if(len(right) < p->size) {
					// // 	up    = quat(-M_PI_2, cam->look) * (right);
					// // } else {
					// // 	up    = quat(-M_PI_2, cam->look) * (norm(right)*p->size);
					// // }

					// // Dont clamp.
					// up = quat(-M_PI_2, cam->look) * (norm(right)*p->size);

					// Use camPpos instead of look when stretching by velocity.
					Vec3 vel = p->vel * settings.velocityStretchMod;

					Vec3 look = norm(p->pos - cam->pos);
					right = vel - look * dot(look, vel);
					if(len(right) < p->size) right = norm(right) * p->size;
					up = quat(-M_PI_2, look) * (norm(right)*p->size);

				} else {
					Quat q = quat(p->rot, cam->look);
					right  = q * (p->size*cam->right);
					up     = q * (p->size*cam->up);
				}
			}

			dxPushQuad( p->pos - right - up, 
			            p->pos - right + up,
			            p->pos + right + up,
			            p->pos + right - up,
			            c, uv );

			if(theGState->pVertexCount >= theGState->primitiveVertexBufferMaxCount-12) {
				dxEndPrimitive();
				dxBeginPrimitive(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			}
		}

		dxEndPrimitive();
	}

	#if 0
	// Debug draw emitter.
	{
		dxDrawCube(pos, vec3(0.05f), vec4(1,1,1,1));

		float lineLength = 0.2f;
		Vec3 dirs[] = { vec3(1,0,0), vec3(0,1,0), vec3(0,0,1) };
		for(int i = 0; i < arrayCount(dirs); i++) {
			Vec3 dir = rot * dirs[i];
			dxDrawLine(pos, pos + dir*lineLength, vec4(dirs[i], 1));
		}
	}
	#endif
}

//

// This could be replaced by some sort of entity group system.
// But we're not using entities for that right now.
struct ParticleGroup {
	ParticleEmitter emitters[5];
	int emitterCount;

	XForm xForm;

	// Shrug.
	float distToCam;

	void init(XForm xForm);
	void add(ParticleEmitter* emitter);
	void add(ParticleEmitter* emitter, XForm xForm);
	void update(float dt, Camera* cam);
	void draw(Camera* cam);
};

void ParticleGroup::init(XForm xForm) {
	*this = {};
	this->xForm = xForm;
}

void ParticleGroup::add(ParticleEmitter* emitter) {
	emitters[emitterCount++] = *emitter;
}

void ParticleGroup::add(ParticleEmitter* emitter, XForm xForm) {
	emitter->xForm = xForm;
	return add(emitter);
}

void ParticleGroup::update(float dt, Camera* cam) {

	// Temporarily transform from local space to world space.
	XForm oldForms[10];
	for(int i = 0; i < emitterCount; i++) {
		oldForms[i] = emitters[i].xForm;

		emitters[i].xForm = xFormCombine(xForm, emitters[i].xForm);
	}
	defer {
		for(int i = 0; i < emitterCount; i++) emitters[i].xForm = oldForms[i];
	};

	int finishedCount = 0;
	for(int i = 0; i < emitterCount; i++) {
		ParticleEmitter* e = emitters + i;
		if(e->finished) finishedCount++; 
	}
	bool everyoneFinished = finishedCount == emitterCount;

	for(int i = 0; i < emitterCount; i++) {
		ParticleEmitter* e = emitters + i;

		if(!e->finished || everyoneFinished) e->update(dt, cam);
	}
}

void ParticleGroup::draw(Camera* cam) {
	for(int i = emitterCount-1; i >= 0; i--) {
		ParticleEmitter* e = emitters + i;
		e->draw(cam);
	}
}
