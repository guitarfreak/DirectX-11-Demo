
Meta_Parse_Enum();
enum Particle_Textures{
	PARTICLE_TEXTURE_Circle = 0,
	PARTICLE_TEXTURE_Square,
	PARTICLE_TEXTURE_Triangle,
	PARTICLE_TEXTURE_Cloud,
	PARTICLE_TEXTURE_Ring,
	PARTICLE_TEXTURE_Bubble1,
	PARTICLE_TEXTURE_Bubble2,
	PARTICLE_TEXTURE_Point,
	PARTICLE_TEXTURE_Star,
	PARTICLE_TEXTURE_Smoke1,
	PARTICLE_TEXTURE_Smoke2,
	PARTICLE_TEXTURE_Fire,

	PARTICLE_TEXTURE_Size, // @Size
};

struct Particle {
	Vec3 pos;
	Vec3 vel;
	float drag;

	float size;
	float velSize;

	float rot;
	float velRot;

	float time;
	float lifeTime;

	Vec4 color[3];

	//

	float distToCam;
	Vec3 globalPos;
};

Meta_Parse_Enum();
enum SpawnRegionType {
	SPAWN_REGION_SPHERE = 0,
	SPAWN_REGION_CYLINDER,
	SPAWN_REGION_BOX,

	SPAWN_REGION_SIZE, // @Size
};

Meta_Parse_Struct(0);
struct SpawnRegion {
	int type; // @V0 @Enum(SpawnRegionType)

	union {
		struct {
			float radiusMin;
			float radius;
			float height;
		};

		struct {
			Vec3 dim; // @V0
		};
	};
};

Meta_Parse_Enum();
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

	FUNC_SIZE, // @Size
};

Meta_Parse_Struct(0);
template <class T>
struct ValueRange {
	T min; // @V0
	T max; // @V0
	int functionType; // @V0 @Enum(FunctionType)

	float getSample();
};

Meta_Parse_Struct(0);
struct ParticleSettings {
	int spriteIndex; // @V0 @Enum(Particle_Textures)
	SpawnRegion region; // @V0

	float spawnRate; // @V0 // Particles per second.
	float lifeTime; // @V0
	float startTime; // @V0
	float restTime; // @V0
	ValueRange<float> lifeTimeP; // @V0
	float fadeInTime; // @V0
	float fadeOutTime; // @V0 @Tag(Section)

	ValueRange<float> size; // @V0
	ValueRange<float> velSize; // @V0
	ValueRange<float> speed; // @V0
	ValueRange<float> angleV; // @V0 @Tag(Range, -90, 90)
	ValueRange<float> angleH; // @V0 @Tag(Range, 0, 360)
	Vec3 gravity; // @V0
	ValueRange<float> drag; // @V0

	bool stretch; // @V0
	ValueRange<float> stretchMod; // @V0

	ValueRange<float> rotation; // @V0 @Tag(Range, -90, 90)
	ValueRange<float> velRot; // @V0 @Tag(Section)

	float colorT; // @V0 @Tag(Range, 0,1) // In percent.
	ValueRange<Vec4> color[3]; // @V0 // Function is ignored on color 2/3.

	float alpha; // @V0 @Tag(Range, 0,1)
	float brightness; // @V0 @Tag(Range, 0,5)
};

struct ParticleGroup;
Meta_Parse_Struct(1);
struct ParticleEmitter {
	DArray<Particle> particles; // @Ignore

	float time; // @Hide
	float spawnTime; // @Hide
	float startTime; // @Hide
	float finishTime; // @Hide
	bool finished; // @Hide

	//

	bool loop; // @V0
	bool localSpace; // @V0
	float fadeDistance; // @V1
	float fadeContrast; // @V1

	ParticleSettings settings; // @V0

	bool markedForUpdate; // @Ignore

	//

	void init();
	void reset();
	void update(float dt, XForm xForm, Camera* cam);
	void draw(XForm xForm, Camera* cam);
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

ParticleSettings getDefaultParticleSettings() {
	ParticleSettings s = {};

	// s.texture = getPString("misc\\particles.dds");
	s.spriteIndex = 0;
	s.region.type = SPAWN_REGION_BOX;
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
	s.drag    = valueRange( 0.8f, 0.90f );

	s.stretch    = false;
	s.stretchMod = valueRange( 0.1f,0.2f );

	s.rotation = valueRange( 0.0f,0.0f );
	s.velRot = valueRange( 0.0f,0.0f );

	s.colorT   = 0.0f;
	s.color[0] = valueRange( vec4(1.0f,0.5f,0, 1), vec4(0.8,0.4f, 0.0f,1), FUNC_LINEAR );

	s.alpha = 1.0f;
	s.brightness = 1.0f;

	return s;
}

void ParticleEmitter::init() {
	*this = {};

	this->settings = getDefaultParticleSettings();

	loop = false;
	localSpace = true;
	fadeDistance = 0.1f;
	fadeContrast = 2;	

	reset();
}

void ParticleEmitter::reset() {
	particles = {};

	time = 0;
	spawnTime = 0;
	startTime = 0;
	finishTime = 0;
	finished = false;
}

inline void particleSim(Particle* p, Vec3 gravityScaled, float dt) {
	p->pos += p->vel * dt;
	p->vel *= pow(p->drag, dt);
	p->vel += gravityScaled;

	p->size += p->velSize * dt;
	p->size = max(p->size, 0.0f);

	p->rot += p->velRot * dt;

	p->time += dt;
}

void particleGetQuad(Particle* p, ParticleSettings* settings, Rect uv, bool localSpace, Camera* cam, Mat4* mat, PrimitiveVertex* verts) {
	Vec4 c;
	{
		float timePercent = p->time / p->lifeTime;

		if(settings->colorT != 0) {
			float t;
			Vec4 color[2];
			if(timePercent < settings->colorT) {
				color[0] = p->color[0];
				color[1] = p->color[1];
				t = timePercent / settings->colorT;
			} else {
				color[0] = p->color[1];
				color[1] = p->color[2];
				t = (timePercent - settings->colorT) / (1 - settings->colorT);
			}

			c = { lerp(t, color[0].r, color[1].r), 
	            lerp(t, color[0].g, color[1].g), 
	            lerp(t, color[0].b, color[1].b), 
	            lerp(t, color[0].a, color[1].a), };

		} else c = p->color[0];

		if(p->time < settings->fadeInTime)
			c.a *= p->time / settings->fadeInTime;
		else if(p->lifeTime - p->time < settings->fadeOutTime) 
			c.a *= (p->lifeTime - p->time) / settings->fadeOutTime;

		c.a *= settings->alpha;
		c.rgb *= settings->brightness;
	}

	Vec3 pos = p->globalPos;

	Vec3 right, up;
	if(settings->stretch) {
		// Use camPpos instead of look when stretching by velocity.
		Vec3 vel = p->vel * p->rot;

		Vec3 look = norm(pos - cam->pos);
		right = vel - look * dot(look, vel);
		if(len(right) < p->size) right = norm(right) * p->size;
		up = quat(-M_PI_2, look) * (norm(right)*p->size);

	} else {
		Quat q = quat(p->rot, cam->look);
		right  = q * (p->size*cam->right);
		up     = norm(cross(right, cam->look)) * p->size;
	}

	dxGetQuad( verts, 
	           pos - right - up, 
	           pos - right + up,
	           pos + right + up,
	           pos + right - up,
	           c, uv );
}

void ParticleEmitter::update(float dt, XForm xForm, Camera* cam) {
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
	Vec3 gravityScaled = xForm.scale * settings.gravity;
	{
		TIMER_BLOCK_NAMED("ParticleSim");

		Vec3 gravityScaledDt = gravityScaled * dt;

		for(auto& it : particles) {
			particleSim(&it, gravityScaledDt, dt);
		}
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

		// if(particles.count + spawnCount > particleSize) {
		// 	spawnCount = particleSize - particleCount;
		// }

		for(int i = 0; i < spawnCount; i++) {
			Particle p = {};

			p.lifeTime = lerp(settings.lifeTimeP.getSample(), settings.lifeTimeP.min, settings.lifeTimeP.max);

			Vec3 offset;
			{
				SpawnRegion region = settings.region;
				switch(region.type) {
					case SPAWN_REGION_SPHERE: {
						Vec3 rp = randomSpherePoint();

						if(!region.radiusMin) {
							offset = rp * region.radius;
						} else {
							offset = rp * (region.radius - region.radiusMin) + norm(rp) * region.radiusMin;
						}
					} break;

					case SPAWN_REGION_CYLINDER: {
						Vec3 rp = randomDiskPoint();

						if(!region.radiusMin) {
							offset = rp * region.radius;
						} else {
							offset = rp * (region.radius - region.radiusMin) + norm(rp) * region.radiusMin;
						}

						offset.z += randomOffset(region.height / 2.0f);
					} break;

					case SPAWN_REGION_BOX: {
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

			p.vel = norm(angle) * lerp(settings.speed.getSample(), settings.speed.min, settings.speed.max);

			p.size = lerp(settings.size.getSample(), settings.size.min, settings.size.max);
			p.velSize = lerp(settings.velSize.getSample(), settings.velSize.min, settings.velSize.max);

			p.drag = 1 - lerp(settings.drag.getSample(), settings.drag.min, settings.drag.max);

			p.rot = degreeToRadian(lerp(settings.rotation.getSample(), settings.rotation.min, settings.rotation.max));

			if(!settings.stretch) {
				p.velRot = degreeToRadian(lerp(settings.velRot.getSample(), settings.velRot.min, settings.velRot.max));
			} else {
				p.velRot = lerp(settings.stretchMod.getSample(), settings.stretchMod.min, settings.stretchMod.max);
			}

			{
				float t = settings.color[0].getSample();

				for(int i = 0; i < arrayCount(settings.color); i++) {
					p.color[i]  = { lerp(t, settings.color[i].min.r, settings.color[i].max.r), 
					                lerp(t, settings.color[i].min.g, settings.color[i].max.g), 
					                lerp(t, settings.color[i].min.b, settings.color[i].max.b), 
					                lerp(t, settings.color[i].min.a, settings.color[i].max.a), };
				}
			}

			if(!localSpace)
				p.pos = xForm.trans + xForm.rot * (xForm.scale * p.pos);

			{
				float minScale = min(xForm.scale.x, xForm.scale.y, xForm.scale.z);
				p.vel = xForm.rot * (xForm.scale * p.vel);
				p.velSize = minScale * p.velSize;
				p.size = (minScale * p.size);
			}

			particleSim(&p, gravityScaled * spawnTimeRest, spawnTimeRest);
			spawnTimeRest -= spawnRate;

			particles.push(p);
		}
	}

	{
		TIMER_BLOCK_NAMED("ParticleUpdate");

		Mat4 mat = modelMatrix(xForm);

		for(int i = 0; i < particles.count; i++) {
			Particle* p = particles + i;

			// Remove dead.
			if(p->time >= p->lifeTime) {
				particles.remove(i);
				i--;
				continue;
			}

			// Calc global pos.
			if(localSpace) p->globalPos = (mat * vec4(p->pos,1)).xyz;
			else           p->globalPos = p->pos;

			p->distToCam = lenSquared(p->globalPos - cam->pos);
		}	
	}

	{
		TIMER_BLOCK_NAMED("ParticleSort");

		// Sort.
		auto cmp = [](Particle& a, Particle& b) -> bool { 
			return a.distToCam > b.distToCam; 
		};
		ksIntrosort(particles.data, particles.count, cmp);
	}

	if(settings.lifeTime > 0) {
		// Wait rest time.
		if((time >= settings.lifeTime) && particles.count == 0) finishTime += dt;

		// Reset.
		if(finishTime > settings.restTime)
			finished = true;
	}
}

void ParticleEmitter::draw(XForm xForm, Camera* cam) {
	TIMER_BLOCK();

	Mat4 mat = modelMatrix(xForm);

	ParticleShaderVars* vars = dxGetShaderVars(Particle);
	vars->fadeDistance = (fadeDistance / vars->farPlane) * 2.0f;
	vars->fadeContrast = fadeContrast;
	dxPushShaderConstants(Shader_Particle);

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

		{
			tMemoryPushMarkerScoped();
			DArray<PrimitiveVertex> verts; verts.initResize(particles.count*6, getTMemory);

			{
				struct ThreadData {
					Particle* particles;
					PrimitiveVertex* verts;
					
					ParticleSettings* settings;
					Rect uv;
					bool localSpace;
					Camera* cam;
					Mat4* mat;
				};

				auto threadFunc = [](void* data) {
					TIMER_BLOCK_NAMED("ParticleGetQuads");
					ThreadHeader* h = (ThreadHeader*)data;
					ThreadData* d = (ThreadData*)h->data;

					for(int i = 0; i < h->count; i++) {
						Particle* p = d->particles + h->index + i;
						particleGetQuad(p, d->settings, d->uv, d->localSpace, d->cam, d->mat, d->verts + (h->index+i) * 6);
					}
				};

				ThreadData td = {particles.data, verts.data, &settings, uv, localSpace, cam, &mat};
				splitThreadTask(particles.count, &td, threadFunc, -1);
			}

			if(particles.count) {
				dxBeginPrimitive(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				dxDrawEndPrimitive(verts.data, verts.count, 6);
			}
		}
	}
}
