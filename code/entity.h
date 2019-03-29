
Meta_Parse_Enum();
enum Entity_Type {
	ET_Player = 0,
	ET_Camera,
	ET_Sky,
	ET_Object,
	ET_ParticleEffect,
	ET_Group,
	ET_Sound,

	ET_Size, // @Size
};

Meta_Parse_Struct(3);
struct Entity {
	int type; // @V0 @Enum(Entity_Type) @Hide
	int id; // @V0 @Hide

	XForm xf; // @V0

	char* name; // @V0 @String
	int groupId; // @V0 @Tag(Id, Group)
	int groupZ; // @V0
	int mountParentId; // @V0 @Tag(Id, Entity)
	XForm xfMount; // @V0

	char* mesh; // @V0 @String
	Vec3 meshoffset; // @V0
	char* material; // @V0 @String
	Vec4 color; // @V0 @Tag(Color)
	bool noRender; // @V3

	Vec3 vel; // @V0
	Vec3 acc; // @V0

	int blockerType; // @V2 @Enum(BlockerType)
	XForm xfBlocker; // @V2

	Rect3 aabb; // @Hide
	Mat4 modelInv; // @Hide

	bool deleted; // @V0

	union {
		// Vec3 camOff; // @V0-0 @Union(type, ET_Player)
		Vec2 camRot; // @V0 @Union(type, ET_Camera) @Tag(Range, 0, 6.2831, -6.2831, 6.2831)

		SkySettings skySettings; // @V0 @Union(type, ET_Sky)

		int trackIndex; // @V0 @Union(type, ET_Sound)

		ParticleEmitter particleEmitter; // @V0 @Union(type, ET_ParticleEffect)

		struct {
			bool handlesParticles; // @V0 @Union(type, ET_Group)
			// int restTime;
		};
	};
};

Meta_Parse_Struct(0);
struct EntityManager {
	DArray<int> indices; // @V0
	DArray<Entity> entities; // @V0

	DArray<Entity*> byType[ET_Size]; // @Ignore
	DArray<Entity*>* groupMembers; // @Ignore

	char* currentMap;
};