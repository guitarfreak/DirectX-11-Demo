
enum Entity_Type {
	ET_Player = 0,
	ET_Camera,

	ET_Object,
	ET_Sound,

	ET_Size,
};

struct Camera {
	Vec3 pos;
	Vec3 look;
	Vec3 up;
	Vec3 right;
};

struct Entity {
	bool init;

	int type;
	int id;
	char name[16];

	Vec3 pos;
	Vec3 dim;
	Vec3 dir;
	Vec3 rot;
	float rotAngle;

	Vec3 vel;
	Vec3 acc;

	int movementType;
	int spatial;

	bool onGround;

	bool deleted;
	bool isMoving;
	bool isColliding;

	union {
		// Player.
		struct {
			Vec3 camOff;
		};

		// Sound.
		struct {
			int trackIndex;
		};
	};
};

struct EntityList {
	Entity* e;
	int size;
};

//

float camDistanceFromFOVandWidth(float fovInDegrees, float w) {
	float angle = degreeToRadian(fovInDegrees);
	float sideAngle = ((M_PI-angle)/2.0f);
	float side = w/sin(angle) * sin(sideAngle);
	float h = side*sin(sideAngle);
	
	return h;
}

Camera getCamData(Vec3 pos, Vec3 rot, Vec3 offset = vec3(0,0,0), Vec3 gUp = vec3(0,0,1), Vec3 startDir = vec3(0,1,0)) {
	Camera c;
	c.pos = pos + offset;
	c.look = startDir;
	rotate(&c.look, rot.x, gUp);
	rotate(&c.look, rot.y, norm(cross(gUp, c.look)));
	c.up = norm(cross(c.look, norm(cross(gUp, c.look))));
	c.right = norm(cross(gUp, c.look));
	c.look = -c.look;

	return c;
}

Camera getCamDataLook(Vec3 pos, Vec3 look) {
	Camera c;
	c.pos = pos;
	c.look = norm(look);
	c.right = norm(cross(vec3(0,0,1), look));
	c.up = norm(cross(look, c.right));

	return c;
}

Camera getCamDataLook(Vec3 pos, Vec3 look, Vec3 right) {
	Camera c;
	c.pos = pos;
	c.look = norm(look);
	c.right = norm(right);
	c.up = norm(cross(look, c.right));

	return c;
}

Mat4 viewMatrix(Camera cam) {
	return viewMatrix(cam.pos, -cam.look, cam.up, cam.right);
}

//

Vec3 getRotationToVector(Vec3 start, Vec3 dest, float* angle) {
	Vec3 side = norm(cross(start, norm(dest)));
	*angle = dot(start, norm(dest));
	*angle = acos(*angle)*2;

	return side;
}	

void initEntity(Entity* e, int type, Vec3 pos, Vec3 dim, Vec2i chunk) {
	*e = {};
	e->init = true;
	e->type = type;
	e->pos = pos;
	e->dim = dim;
}

Entity* addEntity(EntityList* list, Entity* e) {
	bool foundSlot = false;
	Entity* freeEntity = 0;
	int id = 0;
	for(int i = 0; i < list->size; i++) {
		if(list->e[i].init == false) {
			freeEntity = &list->e[i];
			id = i;
			break;
		}
	}

	assert(freeEntity);

	*freeEntity = *e;
	freeEntity->id = id;

	return freeEntity;
}

//

void entityMouseLook(Entity* e, Input* input, float mouseSensitivity) {
	float turnRate = mouseSensitivity * 0.01f;
	e->rot.y -= turnRate * input->mouseDelta.y;
	e->rot.x -= turnRate * input->mouseDelta.x;

	float margin = 0.05f;
	clamp(&e->rot.y, (float)-M_PI_2+margin, (float)M_PI_2-margin);
	e->rot.x = modf(e->rot.x, (float)M_PI*2);
}

void entityKeyboardAcceleration(Entity* e, Input* input, float speed, float boost, bool freeForm) {

	Vec3 up = vec3(0,0,1);

	Camera cam = getCamData(e->pos, e->rot);

	bool rightLock = !freeForm || input->keysDown[KEYCODE_CTRL];
	if(rightLock) cam.look = cross(up, cam.right);

	Vec3 acceleration = vec3(0,0,0);
	if(input->keysDown[KEYCODE_SHIFT]) speed *= boost;
	if(input->keysDown[KEYCODE_W]) acceleration +=  norm(cam.look);
	if(input->keysDown[KEYCODE_S]) acceleration += -norm(cam.look);
	if(input->keysDown[KEYCODE_D]) acceleration +=  norm(cam.right);
	if(input->keysDown[KEYCODE_A]) acceleration += -norm(cam.right);

	if(freeForm) {
		if(input->keysDown[KEYCODE_E]) acceleration +=  norm(up);
		if(input->keysDown[KEYCODE_Q]) acceleration += -norm(up);
	}

	if(acceleration != vec3(0,0,0)) {
		e->acc += norm(acceleration)*speed;
	}
}
