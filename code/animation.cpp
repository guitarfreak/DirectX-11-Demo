
void getPointsFromSkeleton(Mesh* mesh, BoneNode* node, Quat q = quat(), Vec3 p = vec3(0.0f)) {

	XForm form = mesh->basePose[node->data->index];

	Vec3 oldP = p;
	p = p + (q * form.translation);
	q = q * form.rotation;

	mesh->basePosePoints[node->data->index] = p;

	for(int i = 0; i < node->childCount; i++) {
		getPointsFromSkeleton(mesh, node->children + i, q, p);
	}
}

void buildBoneTree(Bone* bones, int count, BoneNode* node, int index = 0) {
	node->data = bones + index;
	int currentDepth = node->data->depth + 1;

	index++;

	// Count.
	int nodeCount = 0;
	for(int i = index; i < count; i++) {
		if(bones[i].depth < currentDepth) break;

		if(bones[i].depth == currentDepth) nodeCount++;
	}

	node->childCount = nodeCount;
	node->children = getPArray(BoneNode, nodeCount);

	int childIndex = 0;
	for(int i = index; i < count; i++) {
		if(bones[i].depth < currentDepth) break;
		
		if(bones[i].depth == currentDepth) {
			buildBoneTree(bones, count, node->children + childIndex, i);
			childIndex++;
		}
	}
}

void AnimationPlayer::setAnim(char* name) {
	for(int i = 0; i < mesh->animationCount; i++) {
		if(strFind(name, mesh->animations[i].name) != -1) {
			time = 0;
			animation = mesh->animations + i;
		}
	}
}

void AnimationPlayer::update(float dt) {
	this->dt = dt;
	time += dt;

	float frame = time * fps;
	if(frame > animation->frameCount-1) {
		time = 0;
	}

	frame = min(frame, (float)animation->frameCount-1);

	{
		int boneCount = animation->boneCount;

		int frame1 = floor(frame);
		int frame2 = ceil(frame);

		if(frame1 == frame || noInterp) {
			XForm* frames = animation->frames[frame1];
			for(int i = 0; i < boneCount; i++) bones[i] = frames[i];

		} else {
			XForm* frames1 = animation->frames[frame1];
			XForm* frames2 = animation->frames[frame2];

			float t = frame - frame1;

			for(int i = 0; i < boneCount; i++) {

				XForm f1 = frames1[i];
				XForm f2 = frames2[i];

				XForm f3 = {};
				f3.translation = lerp(t, f1.translation, f2.translation);
				// Should rotate the other way if over 180 degrees?
				f3.rotation = quatLerp(f1.rotation, f2.rotation, t);
				f3.scale = lerp(t, f1.scale, f2.scale);

				bones[i] = f3;
			}
		}
	}

	if(noLocomotion) {
		// Needs some work.
		Vec3 baseTranslation = animation->frames[0][0].translation;
		bones[0].translation.xy = baseTranslation.xy;
	}

	calcMats(mesh->basePose, bones, &mesh->boneTree);
}

void AnimationPlayer::calcMats(XForm* baseBones, XForm* bones, BoneNode* node, Quat q, Vec3 p, Quat totalRot) {
	XForm formPose = baseBones[node->data->index];
	XForm form = bones[node->data->index];

	Vec3 basePoseNodePos = mesh->basePosePoints[node->data->index];

	// There is probably a better way to do this.

	Quat q1 = q * formPose.rotation;
	Quat q2 = q * form.rotation;
	Quat q3 = q2 * quatInverse(q1);
	totalRot = q3 * totalRot;

	p = p + (q * form.translation);
	q = q * form.rotation;

	Mat4 model = translationMatrix(basePoseNodePos) * 
	             modelMatrix(p - basePoseNodePos, vec3(1), totalRot) * 
	             translationMatrix(-basePoseNodePos);

	mats[node->data->index] = model;

	for(int i = 0; i < node->childCount; i++) {
		calcMats(baseBones, bones, node->children + i, q, p, totalRot);
	}
}