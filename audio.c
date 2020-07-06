#include "audio.h"
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#include "external/dr_mp3.h"
#include "limits.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include <SDL_audio.h>

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

//--- low-level functions ------------------------------------------
float oscSilence(float phase) { (void)phase; return 0; }
float oscSine(float phase) { return sin(2.0f * M_PI * phase); }
float oscTriangle(float phase) {
	float x = fmod(phase, 1.0f);
	float y = 4.0f*x - 1.0f;
	return(x>0.5) ? 2.0f - y : y;
}
float oscSquare(float phase) { return fmod(phase, 1.0f)<0.5 ? 1.0 : -1.0; }
float oscSawtooth(float phase) { return 2.0f * fmod(phase, 1.0f) - 1.0f; }
float oscNoise(float phase) { (void)phase; return rand() / (float)RAND_MAX; }

static Oscillator_t Oscs[] = { oscSilence, oscSine, oscTriangle, oscSquare, oscSawtooth, oscNoise };

Oscillator_t AudioOscillator(SoundWave waveForm) {
	return Oscs[waveForm];
}

float envelope(float attack, float decay, float sustainLvl, float sustainLen, float release, float t) {
	if(t<0.0f)
		return 0.0f;
	if(t<attack)
		return t/attack;
	t -= attack;
	if(t<decay)
		return sustainLvl+(1.0f-sustainLvl)*(1.0f-t/decay);
	t -= decay;
	if(t<sustainLen)
		return sustainLvl;
	t -= sustainLen;
	if(t<release)
		return sustainLvl*(1.0f-t/release);
	return 0.0f;
}

float transposeFreq(float base, int n) {
	const double step = 1.0594630943592952646; // pow(2.0, 1.0/12.0);
	return base * pow(step, n);
}


//--- high-level melody functions ----------------------------------

static void skipWhitespace(char** c) {
	while(*c && (**c==' ' || **c==',' || **c=='\n' || **c=='\r' || **c=='\t'))
		++(*c);
}

static float note2freq(char note, char accidental, int octave) {
	int steps;
	switch(note) {
	case 'B':
		steps = 2; break;
	case 'C':
		steps = 3; break;
	case 'D':
		steps = 5; break;
	case 'E':
		steps = 7; break;
	case 'F':
		steps = 8; break;
	case 'G':
		steps = 10; break;
	case '-':
		return 0.0f;
	default:
		steps = 0;
	}

	if(accidental == 'b')
		--steps;
	else if(accidental == '#')
		++steps;

	if(steps>2)
		--octave;
	float base = 27.5 * pow(2.0, octave);
	return transposeFreq(base, steps);
}

typedef struct {
	SoundWave waveForm;
	float attack;
	float decay;
	float sustain;
	float release;
	float gain;
	float beatLen; /// len of 4 beats in seconds
	char* melody;
	const char* note; /// current note in melody
	float noteFreq;
	float noteLen;
	float t; // time in current note
} Melody;

static Melody* Melody_create(const char* melody) {
	Melody * melo = malloc(sizeof(Melody));
	melo->waveForm = WAVE_SQUARE;
	melo->attack = 0.0f;
	melo->decay = 0.0f;
	melo->sustain = 1.0f;
	melo->release = 0.0f;
	melo->gain = 1.0f;
	melo->beatLen = 4.0f*60.0f/72.0f; // 72 bpm
	melo->note = melo->melody = strdup(melody);
	melo->noteFreq = 0.0f;
	melo->noteLen = 0.0f;
	melo->t = 0.0f;
	return melo;
}

static int Melody_isPlaying(const Melody* melo) {
	if(!melo)
		return 0;
	return (melo->t < melo->noteLen) || (melo->note != NULL && *melo->note!=0);
}

static void readSubsequentFloat(const char** c, float* f) {
	++(*c);
	if(**c == ':')
		++(*c);
	*f = strtod(*c, (char**)c);
}

static void Melody_readParams(Melody* melo) {
	while(*melo->note) {
		skipWhitespace((char**)&melo->note);
		switch(*melo->note) {
		case 'a':
			readSubsequentFloat(&melo->note, &melo->attack);
			break;
		case 'b': {
			float bpm;
			readSubsequentFloat(&melo->note, &bpm);
			melo->beatLen = 4.0f*60.0f/bpm;
			break;
		}
		case 'd':
			readSubsequentFloat(&melo->note, &melo->decay);
			break;
		case 'g':
			readSubsequentFloat(&melo->note, &melo->gain);
			break;
		case 'r':
			readSubsequentFloat(&melo->note, &melo->release);
			break;
		case 's':
			readSubsequentFloat(&melo->note, &melo->sustain);
			break;
		case 'w': // waveForm
			++melo->note;
			if(*melo->note == ':')
				++melo->note;
			if(strncmp(melo->note, "sin", 3)==0)
				melo->waveForm = WAVE_SINE;
			else if(strncmp(melo->note, "tri", 3)==0)
				melo->waveForm = WAVE_TRIANGLE;
			else if(strncmp(melo->note, "sqr", 3)==0)
				melo->waveForm = WAVE_SQUARE;
			else if(strncmp(melo->note, "saw", 3)==0)
				melo->waveForm = WAVE_SAWTOOTH;
			else if(strncmp(melo->note, "noi", 3)==0)
				melo->waveForm = WAVE_NOISE;
			melo->note += 3;
			break;
		case '}':
			++melo->note;
			return;
		case 0:
			return;
		default:
			++melo->note;
		}
	}
}

static size_t Melody_nextNote(Melody* melo, float* freq, float* duration) {
	unsigned numNotes = 0;
	const char* c = melo->note;
	while(*c) {
		skipWhitespace((char**)&c);
		if(*c!='-' && (*c<'A' || *c>'G')) {
			fprintf(stderr, "invalid note %c at pos %u\n", *c, numNotes);
			break;
		}
		char note = *c;
		++c;
		char accidental = (*c && (*c == '#' || *c=='b')) ? *c : ' ';
		if(accidental!=' ')
			++c;
		if(note!='-' && *c && (*c < '0' || *c>'9')) {
			fprintf(stderr, "invalid octave %c at pos %u\n", *c, numNotes);
			break;
		}
		int octave = *c - '0';
		if(note!='-')
			++c;
		if(*c!='/' && *c!='*') {
			fprintf(stderr, "invalid duration signifier at pos %u\n", numNotes);
			break;
		}
		int isDivision = *c=='/';
		++c;
		float d = strtod(c, (char**)&c);
		if(d>0.0f) {
			*freq = note2freq(note, accidental, octave);
			*duration = isDivision ? 1.0f/d : d;
			break;
		}
		if(*c)
			++c;
	}
	size_t numCharsRead = c - melo->note;
	melo->note += numCharsRead;
	return numCharsRead;
}


static void Melody_chunk(Melody* melo, unsigned sampleRate, unsigned chunkSz, float* chunk) {
	while(chunkSz>0) {
		if(!Melody_isPlaying(melo)) {
			memset(chunk, 0, chunkSz*sizeof(float));
			return;
		}
		if(melo->t >= melo->noteLen) { // read next note:
			skipWhitespace((char**)&melo->note);
			if(*melo->note == '{') {
				Melody_readParams(melo);
				//printf("{w:%i a:%f d:%f s:%f r:%f g:%f b:%f}\n", melo->waveForm, melo->attack, melo->decay, melo->sustain, melo->release, melo->gain, melo->beatLen);
			}

			float freq=0.0f, duration=0.0f;
			if(!Melody_nextNote(melo, &freq, &duration))
				return;

			melo->noteFreq = freq;
			melo->noteLen = duration*melo->beatLen;
			melo->t = 0.0f;
			//printf("freq:%f len:%f\n", freq, melo->noteLen);
		}
		unsigned numSamples = 0.5f + (melo->noteLen - melo->t) * sampleRate;
		if(numSamples) {
			if(numSamples > chunkSz)
				numSamples = chunkSz;
			Oscillator_t osc = AudioOscillator(melo->waveForm);
			float sustainLen = melo->noteLen - melo->attack - melo->decay - melo->release;

			for(unsigned i=0; i<numSamples; ++i, ++chunk) {
				float t = melo->t + (float)i/sampleRate;
				if(!melo->noteFreq)
					*chunk = 0.0f;
				else
					*chunk = (*osc)(melo->noteFreq*t) * envelope(melo->attack, melo->decay,
						melo->sustain, sustainLen, melo->release, t) * melo->gain;
			}
			melo->t += (float)numSamples/sampleRate;
			chunkSz -= numSamples;
		}
		else if(chunkSz) //  safeguard against rounding errors
			melo->t += 1.0f/sampleRate;
	}
}

//--- SDL interface ------------------------------------------------
typedef struct {
	SoundWave waveForm;
	const float* waveData;
	Melody* melody;
	int freq;
	unsigned numSamples;
	unsigned sample;
	float volume[2];
} SoundSpec;

static int devId = 0;
static unsigned numTracks = 0;
static float masterVolume;
static SoundSpec* tracks = NULL;
static SDL_AudioSpec audioSpec;

typedef struct {
	float* waveData;
	unsigned waveLen;
} SampleSpec;

static SampleSpec* samples=NULL;
static unsigned numSamples=0;
static unsigned numSamplesMax=0;

static void Melody_cleanup(SoundSpec* t) {
	free(t->melody->melody);
	free(t->melody);
	t->melody = NULL;
	free((void*)t->waveData);
	t->waveData = NULL;
}

static void audioCallback(void *user_data, Uint8 *raw_buffer, int bytes) {
	unsigned chunkSz = audioSpec.samples;
	Sint16 *buffer = (Sint16*)raw_buffer;
	SoundSpec* tracks = (SoundSpec*)user_data;

	for(unsigned k=0; k<numTracks; ++k) {
		SoundSpec* t = &tracks[k];
		if(!t->melody)
			continue;
		if(!Melody_isPlaying(t->melody))
			Melody_cleanup(t);
		else {
			Melody_chunk(t->melody, audioSpec.freq, chunkSz, (float*)t->waveData);
			t->sample = 0;
			t->numSamples = chunkSz;
		}
		continue;
	}

	for(unsigned i = 0; i < chunkSz; ++i) {
		float v[] = { 0.0f, 0.0f };
		for(unsigned k=0; k<numTracks; ++k) {
			SoundSpec* t = &tracks[k];
			if(t->sample >= t->numSamples)
				continue;
			if(t->waveForm) {
				double time = (double)(t->sample) / audioSpec.freq;
				for(int j=0; j<audioSpec.channels; ++j)
					v[j] += 32767.0f * t->volume[j] * masterVolume * (Oscs[t->waveForm])(t->freq * time);
			}
			else if(t->waveData) {
				for(int j=0; j<audioSpec.channels; ++j)
					v[j] += 32767.0f * t->volume[j] * masterVolume * t->waveData[t->sample];
			}
			++t->sample;
		}
		for(int j=0; j<audioSpec.channels; ++j)
			buffer[i*audioSpec.channels+j] = v[j]>32767.0f ? 32767 : v[j] < -32768.0f ? -32768 : (Sint16)v[j];
	}
}

unsigned AudioOpen(unsigned freq, unsigned nTracks) {
	if(SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
		SDL_Log("Failed to initialize SDL audio: %s", SDL_GetError());
		return 0;
	}

	numTracks = nTracks;
	tracks = (SoundSpec*)malloc(sizeof(SoundSpec)*numTracks);
	for(unsigned i=0; i<numTracks; ++i) {
		tracks[i].numSamples = 0;
		tracks[i].sample = 0;
		tracks[i].waveData = NULL;
		tracks[i].melody = NULL;
	}

	SDL_AudioSpec want;
	want.freq = freq; // number of samples per second
	want.format = AUDIO_S16; // sample type (here: signed short i.e. 16 bit)
	want.channels = 2;
	want.samples = 1024;
	want.callback = audioCallback; // function SDL calls periodically to refill the buffer
	want.userdata = tracks;
	masterVolume = 1.0;

	devId = SDL_OpenAudioDevice(0, 0, &want, &audioSpec, 0);
	if(devId == 0) {
		SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to open audio: %s", SDL_GetError());
		free(tracks);
	}
	else if(want.format != audioSpec.format) {
		SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to get the desired AudioSpec");
		free(tracks);
		devId = 0;
	}
	else
		SDL_PauseAudioDevice(devId, 0); // start playing tracks
	SDL_ClearError();
	return devId;
}

void AudioClose() {
	if(!devId)
		return;
	SDL_CloseAudioDevice(devId);
	devId = 0;
	for(unsigned i=0; i<numTracks; ++i) {
		if(tracks[i].melody) {
			free((void*)tracks[i].waveData);
			free(tracks[i].melody->melody);
			free(tracks[i].melody);
		}
	}
	free(tracks);

	if(numSamples) {
		for(unsigned i=0; i<numSamples; ++i)
			free(samples[i].waveData);
		numSamples = numSamplesMax = 0;
		free(samples);
		samples = NULL;
	}
}

void AudioSetVolume(float volume) {
	masterVolume = volume;
}

float AudioGetVolume() {
	return masterVolume;
}

unsigned AudioTracks() {
	return numTracks;
}

unsigned AudioSound(SoundWave waveForm, int freq, float duration, float volume, float balance) {
	unsigned ret = UINT_MAX;
	if(!devId)
		return ret;
	SDL_LockAudioDevice(devId);
	for(unsigned i=0; i<numTracks; ++i) 
		if(!AudioPlaying(i)) {
			SoundSpec* track = &tracks[i]; 
			track->waveForm = waveForm;
			track->waveData = NULL;
			track->melody = NULL;
			track->freq = freq;
			track->sample = 0;
			track->numSamples = 0.5f + audioSpec.freq * duration;
			track->volume[0] = volume*(-0.4f*balance+0.6f);
			track->volume[1] = volume*(+0.4f*balance+0.6f);
			ret = i;
			break;
		}
	SDL_UnlockAudioDevice(devId);
	return ret;
}

unsigned AudioPlay(const float* data, unsigned waveLen, float volume, float balance) {
	unsigned ret = UINT_MAX;
	if(!devId)
		return ret;
	SDL_LockAudioDevice(devId);
	for(unsigned i=0; i<numTracks; ++i) 
		if(!AudioPlaying(i)) {
			SoundSpec* track = &tracks[i]; 
			track->waveForm = WAVE_NONE;
			track->waveData = data;
			track->melody = NULL;
			track->freq = 0;
			track->sample = 0;
			track->numSamples = waveLen;
			track->volume[0] = volume*(-0.4f*balance+0.6f);
			track->volume[1] = volume*(+0.4f*balance+0.6f);
			ret = i;
			break;
		}
	SDL_UnlockAudioDevice(devId);
	return ret;
}

unsigned AudioMelody(const char* melody, float volume, float balance) {
	unsigned ret = UINT_MAX;
	if(!devId)
		return ret;
	SDL_LockAudioDevice(devId);
	for(unsigned i=0; i<numTracks; ++i) 
		if(!AudioPlaying(i)) {
			Melody* melo = Melody_create(melody);
			SoundSpec* track = &tracks[i]; 
			track->waveForm = WAVE_NONE;
			track->waveData = malloc(sizeof(float)*audioSpec.samples);
			track->melody = melo;
			track->freq = 0;
			track->sample = 0;
			track->numSamples = audioSpec.samples;
			track->volume[0] = volume*(-0.4f*balance+0.6f);
			track->volume[1] = volume*(+0.4f*balance+0.6f);
			ret = i;
			break;
		}
	SDL_UnlockAudioDevice(devId);
	return ret;
}

int AudioPlaying(unsigned track) {
	if(!devId || track>=numTracks)
		return 0;
	SoundSpec* t = &tracks[track];
	return Melody_isPlaying(t->melody) || (t->sample < t->numSamples);
}

void AudioStop(unsigned track) {
	if(!devId || track>=numTracks)
		return;
	SDL_LockAudioDevice(devId);
	SoundSpec* t = &tracks[track];
	if(Melody_isPlaying(t->melody))
		Melody_cleanup(t);
	else
		t->sample = t->numSamples;
	SDL_UnlockAudioDevice(devId);
}

size_t AudioUpload(void* mp3data, unsigned numBytes) {
	drmp3_config cfg = { 1, audioSpec.freq };
	uint64_t pcmSamples;

	float* pcmData = drmp3_open_memory_and_read_f32(mp3data, numBytes, &cfg, &pcmSamples);
	if(!pcmSamples || cfg.outputChannels!=1 || cfg.outputSampleRate!=audioSpec.freq) {
		drmp3_free(pcmData);
		return 0;
	}
	return AudioSample(pcmData, pcmSamples);
}

size_t AudioSample(float* waveData, unsigned waveLen) {
	if(numSamplesMax==0) {
		numSamplesMax=4;
		samples = (SampleSpec*)malloc(numSamplesMax*sizeof(SampleSpec));
	}
	else if(numSamples == numSamplesMax) {
		numSamplesMax *= 2;
		samples = (SampleSpec*)realloc(samples, numSamplesMax*sizeof(SampleSpec));
	}
	samples[numSamples].waveData = waveData;
	samples[numSamples].waveLen = waveLen;
	return ++numSamples;
}

unsigned AudioReplay(size_t sample, float volume, float balance) {
	if(!sample || sample>numSamples)
		return UINT_MAX;
	--sample;
	return AudioPlay(samples[sample].waveData, samples[sample].waveLen, volume, balance);
}
