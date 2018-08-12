
#include <Mmdeviceapi.h>
#include <Audioclient.h>

#define PITCH_PERCENT 0.05946309435929526456

struct AudioState;
extern AudioState* theAudioState;

// typedef struct tWAVEFORMATEX {
//   WORD  wFormatTag;
//   WORD  nChannels;
//   DWORD nSamplesPerSec;
//   DWORD nAvgBytesPerSec;
//   WORD  nBlockAlign;
//   WORD  wBitsPerSample;
//   WORD  cbSize;
// } WAVEFORMATEX;

struct Audio {
	char* name;
	char* file;

	int channels;
	int sampleRate;
	int bitsPerSample;

	short* data;
	int frameCount;
	int totalLength;
};

struct Track {
	bool used;

	bool playing;
	bool paused;
	int index;
	i64 startTime;

	Audio* audio;
	int type;
	float volume;
	float speed;

	bool isSpatial;
	Vec3 pos;

	float minDistance;
	float maxDistance;
};

enum SoundType {
	Sound_Type_Effect = 0,
	Sound_Type_Music,
	Sound_Type_Size,
};

struct Sound {
	char* name;

	int type;

	Audio** audioArray;
	int audioCount;

	float volume;
	float volumeMod;

	float pitchOffset;
	float pitchMod;

	float minDistance;
	float maxDistance;
};

struct AudioState {
	IMMDeviceEnumerator* deviceEnumerator;
	IMMDevice* immDevice;
	IAudioClient* audioClient;
	IAudioRenderClient* renderClient;
	float latencyInSeconds;

	WAVEFORMATEX* waveFormat;
	uint bufferFrameCount;
	float latency;
	int channelCount;
	int sampleRate;

	//

	Audio* files;
	int fileCount;
	int fileCountMax;

	// Mixer.

	float masterVolume;
	float effectVolume;
	float musicVolume;

	Track tracks[20];

	//

	Sound* sounds;
	int soundCount;
};

void audioDeviceInit(AudioState* as, int frameRate) {

	int hr;

	// as->latency = 1.5f;
	as->latency = 2.0f;

	const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
	hr = CoCreateInstance(
	       CLSID_MMDeviceEnumerator, NULL,
	       CLSCTX_ALL, IID_IMMDeviceEnumerator,
	       (void**)&as->deviceEnumerator);
	if(hr) { printf("Failed to initialise sound. 1"); assert(!hr); };

	hr = as->deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &as->immDevice);
	if(hr) { printf("Failed to initialise sound. 2"); assert(!hr); };

	hr = as->immDevice->Activate(__uuidof(IAudioClient),CLSCTX_ALL,NULL,(void**)&as->audioClient);
	if(hr) { printf("Failed to initialise sound. 3"); assert(!hr); };

	int referenceTimeToSeconds = 10 * 1000 * 1000;
	REFERENCE_TIME referenceTime = referenceTimeToSeconds * ((float)1/frameRate) * as->latency; // 100 nano-seconds -> 1 second.
	// REFERENCE_TIME referenceTime = referenceTimeToSeconds * 1; // 100 nano-seconds -> 1 second.
	hr = as->audioClient->GetMixFormat(&as->waveFormat);

	{
		WAVEFORMATEX* format = as->waveFormat;
		format->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
		format->nChannels = 2;
		format->wBitsPerSample = 32;
		format->cbSize = 0;

		WAVEFORMATEX what = {};
		WAVEFORMATEX* formatClosest = &what;
		hr = as->audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, format, &formatClosest);
		if(hr) { printf("Failed to initialise sound. 4"); assert(!hr); };

		as->channelCount = format->nChannels;
		as->sampleRate = format->nSamplesPerSec;
	}

	as->audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, referenceTime, 0, as->waveFormat, 0);
	if(hr) { printf("Failed to initialise sound. 5"); assert(!hr); };

	REFERENCE_TIME latency;
	as->audioClient->GetStreamLatency(&latency);
	if(hr) { printf("Failed to initialise sound. 6"); assert(!hr); };

	as->latencyInSeconds = (float)latency / referenceTimeToSeconds;

	hr = as->audioClient->GetBufferSize(&as->bufferFrameCount);
	if(hr) { printf("Failed to initialise sound. 7"); assert(!hr); };

	hr = as->audioClient->GetService(__uuidof(IAudioRenderClient), (void**)&as->renderClient);
	if(hr) { printf("Failed to initialise sound. 8"); assert(!hr); };

	// Fill with silence before starting.

	float* buffer;
	hr = as->renderClient->GetBuffer(as->bufferFrameCount, (BYTE**)&buffer);
	if(hr) { printf("Failed to initialise sound. 9"); assert(!hr); };

	// hr = as->renderClient->ReleaseBuffer(as->bufferFrameCount, AUDCLNT_BUFFERFLAGS_SILENT);
	hr = as->renderClient->ReleaseBuffer(0, AUDCLNT_BUFFERFLAGS_SILENT);
	if(hr) { printf("Failed to initialise sound. 10"); assert(!hr); };

	// // Calculate the actual duration of the allocated buffer.
	// hnsActualDuration = (double)REFTIMES_PER_SEC *
	//                     as->bufferFrameCount / pwfx->nSamplesPerSec;

	hr = as->audioClient->Start();  // Start playing.
	if(hr) { printf("Failed to initialise sound. 11"); assert(!hr); };

}

void addAudio(AudioState* as, char* filePath, char* name) {

	if(as->fileCount == as->fileCountMax) return;

	char* extension = getFileExtension(filePath);
	if(!extension) return;

	// Only load ogg files.

	int result = strFind(extension, "ogg");
	if(result == -1) return;

	int channels;
	int sampleRate;
	short* data;
	int frameCount = stb_vorbis_decode_filename(filePath, &channels, &sampleRate, &data);

	Audio audio = {};

	audio.name = getPStringCpy(name);
	audio.file = 0;
	audio.data = data;
	audio.sampleRate = sampleRate;
	audio.channels = channels;
	audio.bitsPerSample = 16; // Always correct?

	int bytesPerSample = audio.bitsPerSample/8;
	audio.totalLength = frameCount/bytesPerSample;
	audio.frameCount = frameCount;

	as->files[as->fileCount++] = audio;
}

Audio* getAudio(char* name) {
	AudioState* as = theAudioState;
	for(int i = 0; i < as->fileCount; i++) {
		if(strCompare(as->files[i].name, name)) {
			return (as->files + i);
		}
	}

	return 0;
}

// Get mono or stereo samples from buffer and linearly interpolate if necessary.

inline void audioGetSamples(short* data, float fIndex, int channelCount, float speed, float samples[2]) {
	int index = fIndex;

	for(int channelIndex = 0; channelIndex < channelCount; channelIndex++) {
		if(speed == 1.0f) {
			samples[channelIndex]  = (float)data[index * channelCount + channelIndex] / SHRT_MAX;

		} else {
			int nextIndex = index + 1;

			short value = data[index * channelCount + channelIndex];
			short nextValue = data[nextIndex * channelCount + channelIndex];

			float valueFloat = (float)value/SHRT_MAX;
			float nextValueFloat = (float)nextValue/SHRT_MAX;

			float percent = fIndex - index;
			samples[channelIndex] = valueFloat + (nextValueFloat-valueFloat) * percent;
		}
	}

	if(channelCount == 1) samples[1] = samples[0];
}

void updateAudio(AudioState* as, float dt, Camera cam) {
	{
		TIMER_BLOCK_NAMED("Audio");

		int framesPerFrame = dt * as->waveFormat->nSamplesPerSec * as->latency;

		uint numFramesPadding;
		as->audioClient->GetCurrentPadding(&numFramesPadding);
		uint numFramesAvailable = as->bufferFrameCount - numFramesPadding;
		framesPerFrame = min(framesPerFrame, (int)numFramesAvailable);

		// int framesToPush = framesPerFrame - numFramesPadding;
		// printf("%i %i %i %i\n", numFramesAvailable, numFramesPadding, as->bufferFrameCount, framesToPush);

		// numFramesPadding = framesPerFrame;
		// framesPerFrame = framesToPush;

		// if(framesPerFrame && framesToPush > 0) 
		if(framesPerFrame) {

			float* buffer;
			as->renderClient->GetBuffer(numFramesAvailable, (BYTE**)&buffer);

			for(int i = 0; i < numFramesAvailable*2; i++) buffer[i] = 0.0f;

			for(int trackIndex = 0; trackIndex < arrayCount(as->tracks); trackIndex++) {
				Track* track = as->tracks + trackIndex;
				Audio* audio = track->audio;

				if(!track->used) continue;


				int index = track->index;
				int channels = audio->channels;

				float speed = track->speed * ((float)audio->sampleRate / as->sampleRate);
				int frameCount = audio->frameCount;
				if(speed != 1.0f) {
					frameCount = roundf((frameCount-1) * (float)(1/speed)) + 1;
				}

				int availableFrames = min(frameCount - index, (int)numFramesAvailable);

				{
					float distance = track->isSpatial ? len(track->pos - cam.pos) : 0;
					if(!track->isSpatial || (track->isSpatial && distance <= track->maxDistance)) {

						float volume = as->masterVolume * track->volume;
						if(track->type == Sound_Type_Effect) volume *= as->effectVolume;
						else if(track->type == Sound_Type_Music) volume *= as->musicVolume;

						float panning = 0; // -1 to 1
						if(track->isSpatial) {

							Vec3 planePoint = projectPointOnPlane(track->pos, cam.pos, cam.up);
							Vec3 dir = norm(planePoint - cam.pos);
							float rightPercent = acos(dot(dir, -cam.right)) / M_PI;
							panning = (rightPercent * 2) - 1;

							float distanceVolumeMod = track->minDistance / distance;
							distanceVolumeMod = min(distanceVolumeMod, 1.0f);

							volume *= distanceVolumeMod;
							volume *= 0.5f; // We don't want to distort when playing only in one channel.
						}

						for(int frameIndex = 0; frameIndex < availableFrames; frameIndex++) {
							float sampleIndex = (index + frameIndex) * speed;
							if(sampleIndex > (audio->frameCount-1)) break;

							float samples[2];
							audioGetSamples(audio->data, sampleIndex, channels, speed, samples);

							float* bufferLeftChannel  = buffer + ((frameIndex*2) + 0);
							float* bufferRightChannel = buffer + ((frameIndex*2) + 1);

							float pan = abs(panning);
							if(panning < 0) {
								(*bufferLeftChannel)  += samples[0] * volume + (samples[1] * volume * pan);
								(*bufferRightChannel) += samples[1] * volume * (1 - pan);

							} else {
								(*bufferRightChannel) += samples[1] * volume + (samples[0] * volume * pan);
								(*bufferLeftChannel)  += samples[0] * volume * (1 - pan);
							}
						}
					}
				}

				track->index += framesPerFrame;

				if(track->index >= audio->frameCount) {
					track->used = false;
				}
			}

			as->renderClient->ReleaseBuffer(framesPerFrame, 0);
		}
	}
}

//

Sound* getSound(char* name) {
	AudioState* as = theAudioState;
	for(int i = 0; i < as->soundCount; i++) {
		Sound* s = as->sounds + i;
		if(strCompare(s->name, name)) return s;
	}

	return 0;
}

int playSound(char* name) {
	AudioState* as = theAudioState;

	Sound* s = getSound(name);
	if(!s) return -1;

	// Get unused track.
	Track* track = 0;
	int trackIndex = -1;
	for(int i = 0; i < arrayCount(as->tracks); i++) {
		if(!as->tracks[i].used) {
			track = as->tracks + i;
			trackIndex = i;
		}
	}

	// Get oldest track and overwrite with new one.
	if(!track) {
		i64 oldestStartTime = LLONG_MAX;
		for(int i = 0; i < arrayCount(as->tracks); i++) {
			if(as->tracks[i].startTime < oldestStartTime) {
				oldestStartTime = as->tracks[i].startTime;
				track = as->tracks + i;
				trackIndex = i;
			}
		}
	}

	*track = {};
	track->used = true;
	track->playing = true;
	track->paused = false;
	track->index = 0;

	LARGE_INTEGER timeStamp;
	QueryPerformanceCounter(&timeStamp);
	track->startTime = timeStamp.QuadPart;
	
	track->audio = s->audioArray[0];

	track->type = s->type;
	track->volume = s->volume;
	float mod = 1.0f + s->volumeMod;
	track->volume *= randomFloat(1.0/mod, mod); // Probably not what we want.

	// Pitch offset.
	track->speed = 1;
	float percent = pow(2, s->pitchOffset/12.0);
	if(s->pitchOffset < 0) percent = 1.0 / percent;
	track->speed *= percent;

	// Pitch mod.
	// if(s->pitchMod != 0.0f) {
		percent = pow(2, s->pitchMod/12.0);
		float randomPercent = randomFloat(1/percent, percent);
		track->speed *= randomPercent;
	// }

	track->minDistance = s->minDistance;
	track->maxDistance = s->maxDistance;

	return trackIndex;
}

//

void soundInit(Sound* s, char* name, int type, char* file) {
	s->name = getPStringCpy(name);
	s->type = type;
	s->audioArray = getPArray(Audio*, 1);
	s->audioArray[s->audioCount++] = getAudio(file);
}

Sound sound(char* name, int type, char* file, float volume, float volumeMod, float pitchOffset, float pitchMod, float minDistance, float maxDistance) {
	Sound s = {};
	soundInit(&s, name, type, file);

	s.volume = volume;
	s.volumeMod = volumeMod;
	s.pitchOffset = pitchOffset;
	s.pitchMod = pitchMod;
	s.minDistance = minDistance;
	s.maxDistance = maxDistance;

	return s;
}

Sound sound(char* name, int type, char* file, float volume, float volumeMod, float pitchOffset, float pitchMod) {
	Sound s = {};
	soundInit(&s, name, type, file);

	s.volume = volume;
	s.volumeMod = volumeMod;
	s.pitchOffset = pitchOffset;
	s.pitchMod = pitchMod;

	return s;
}

Sound sound(char* name, int type, char* file, float volume, float minDistance, float maxDistance) {
	Sound s = {};
	soundInit(&s, name, type, file);

	s.volume = volume;
	s.minDistance = minDistance;
	s.maxDistance = maxDistance;

	return s;
}

void initSoundTable(AudioState* as) {
	as->sounds = getPArray(Sound, 100);
	as->soundCount = 0;

	Sound* sa = as->sounds;
	int sc = 0;

	int type = Sound_Type_Effect;
	sa[sc++] = sound("gameStart",  type, "ui\\start.ogg",        0.5f, 0.3f, 0, 0.5f);
	sa[sc++] = sound("menuSelect", type, "ui\\select.ogg",       0.5f, 0.3f, 0, 0.5f);
	sa[sc++] = sound("menuOption", type, "ui\\changeOption.ogg", 0.5f, 0.3f, 0, 0.5f);
	sa[sc++] = sound("menuPush",   type, "ui\\menuPush.ogg",     0.5f, 0.3f, 0, 0.5f);
	sa[sc++] = sound("menuPop",    type, "ui\\menuPop.ogg",      0.5f, 0.3f, 0, 0.5f);
	sa[sc++] = sound("ping",       type, "misc\\pianoKey.ogg",   0.8f, 0, 0, 0, 2, 100);

	as->soundCount = sc;
}