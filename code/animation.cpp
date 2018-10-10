
void xFormsLocalToGlobal(XForm* forms, BoneNode* node, XForm cForm = xForm()) {
	XForm form = forms[node->data->index];

	cForm = xFormCombine(cForm, form);
	forms[node->data->index] = cForm;

	for(int i = 0; i < node->childCount; i++) {
		xFormsLocalToGlobal(forms, node->children + i, cForm);
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
		boneCount = animation->boneCount;

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
				f3.trans = lerp(t, f1.trans, f2.trans);
				// Should rotate the other way if over 180 degrees?
				f3.rot = quatLerp(f1.rot, f2.rot, t);
				f3.scale = lerp(t, f1.scale, f2.scale);

				bones[i] = f3;
			}
		}
	}

	if(noLocomotion) {
		// Needs some work.
		Vec3 baseTranslation = animation->frames[0][0].trans;
		bones[0].trans.xy = baseTranslation.xy;
	}

	xFormsLocalToGlobal(bones, &mesh->boneTree);

	// Calc mats.
	{
		for(int i = 0; i < boneCount; i++) {
			XForm a = mesh->basePose[i];
			XForm b = bones[i];

			Quat totalRot = b.rot * quatInverse(a.rot);

			Mat4 model = translationMatrix(a.trans) * 
			             modelMatrix(b.trans - a.trans, vec3(1), totalRot) * 
			             translationMatrix(-a.trans);

			mats[i] = model;
		}
	}
}
