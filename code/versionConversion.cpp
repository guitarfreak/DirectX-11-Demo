
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
	}
}