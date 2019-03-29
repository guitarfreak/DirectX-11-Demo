
void specificVersionConversion(SData* sData, int version) {
	switch(sData->typeId) {
		case TYPE_ParticleEmitter: {
			if(version == 0) {
				for(auto& it : sData->members) {
					     if(strCompare(it.name, "fadeDistance")) *((float*)it.aData) = 0.1f;
					else if(strCompare(it.name, "fadeContrast")) *((float*)it.aData) = 2.0f;
				}
			}
		} break;

		case TYPE_Entity: {
			if(version == 1) {
				for(auto& it : sData->members) {
					if(strCompare(it.name, "xfBlocker")) {
						XForm xf = xForm();
						SData sData2 = {};
						serializeData(&sData2, getType(XForm), (char*)&xf, it.name);
						sDataCopy(&it, &sData2);
					}
				}
			}
		} break;
	}
}