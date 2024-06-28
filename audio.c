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
#define PI2 (2.0f * M_PI)

//--- low-level functions ------------------------------------------
float randf() { return rand() / ((float)RAND_MAX+1.0f); }
float fsign(float f) { return (f > 0.0f) ? 1.0f : ((f < 0.0f) ? -1.0f : 0.0f); }

float oscSilence(float phase, float timbre, OscillatorCtx* ctx) { (void)phase; (void)timbre; (void)ctx; return 0; }
float oscSine(float phase, float timbre, OscillatorCtx* ctx) {
	(void)ctx;
	float s = sin(PI2 * phase);
	return (timbre>=1.0f) ? s : fsign(s) * powf(fabs(s), timbre);
}
float oscSquare(float phase, float timbre, OscillatorCtx* ctx) { (void)ctx; return fmod(phase, 1.0f)<timbre ? 1.0f : -1.0f; }
float oscSawtooth(float phase, float timbre, OscillatorCtx* ctx) {
	(void)ctx;
	const float timbre2 = timbre/2, x = fmod(phase, 1.0f), x2=1-timbre2;
	return (x<timbre2) ? x/timbre2 :
		(x<x2) ? 1.0f-2.0f*(x-timbre2)/(1.0f-timbre) :
		-1.0f+(x-x2)/timbre2;
}
float oscNoise(float phase, float timbre, OscillatorCtx* ctx) {
	(void)phase;
	const uint32_t waveLen = ceilf(AudioSampleRate()/20000/timbre);
	if(--ctx->counter==0) {
		ctx->counter = fmaxf(1.0f, waveLen);
		ctx->amplPrev = ctx->ampl;
		ctx->ampl = 2*randf() - 1.0f;
	}
	float fraction = (float)ctx->counter/waveLen;
	return ctx->ampl * (1.0f-fraction) + ctx->amplPrev * fraction;
}
float oscBin(float phase, float timbre, OscillatorCtx* ctx) { // binary noise
	(void)phase; 
	const uint16_t freq = timbre*20000;
	if(--ctx->counter == 0) {
		ctx->counter = fmaxf(1.0f, randf()*AudioSampleRate()/freq);
		ctx->ampl = ctx->ampl == 1.0f ? -1.0f : 1.0f;
	}
	return ctx->ampl;
}

Oscillator_t AudioOscillator(SoundWave waveForm) {
	static Oscillator_t Oscs[] = { oscSilence, oscSine, oscSawtooth, oscSquare, oscSawtooth, oscNoise, oscBin };
	return Oscs[waveForm];
}

float defaultTimbre(SoundWave waveForm) {
	switch(waveForm) {
	case WAVE_TRIANGLE:
	case WAVE_SQUARE: return 0.5f;
	default: return 1.0;
	}
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

float note2freq(char note, char accidental, int octave) {
	int steps;
	switch(note) {
	case 'B':
	case 'H':
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
	uint32_t numNotes = 0;
	const char* c = melo->note;
	while(*c) {
		skipWhitespace((char**)&c);
/*
		if(*c>='0' && *c<='9') {
			*freq = *c - '0';
			while(*++c >='0' && *c<='9')
				*freq = 10*(*freq) + (*c - '0');
		}
*/
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


static void Melody_chunk(Melody* melo, uint32_t sampleRate, uint32_t chunkSz, float* chunk) {
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
		uint32_t numSamples = 0.5f + (melo->noteLen - melo->t) * sampleRate;
		if(numSamples) {
			if(numSamples > chunkSz)
				numSamples = chunkSz;
			Oscillator_t osc = AudioOscillator(melo->waveForm);
			OscillatorCtx ctx = { 0.0f, 0.0f, 1 };
			float timbre = defaultTimbre(melo->waveForm);
			float sustainLen = melo->noteLen - melo->attack - melo->decay - melo->release;

			for(uint32_t i=0; i<numSamples; ++i, ++chunk) {
				float t = melo->t + (float)i/sampleRate;
				if(!melo->noteFreq)
					*chunk = 0.0f;
				else
					*chunk = (*osc)(melo->noteFreq*t, timbre, &ctx) * envelope(melo->attack, melo->decay,
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
typedef struct AudioChunk {
	float* waveData;
	uint32_t numSamples;
	struct AudioChunk* next;
} AudioChunk;

typedef struct {
	SoundWave waveForm;
	OscillatorCtx octx;
	const float* waveData;
	Melody* melody;
	float freq;
	uint32_t numSamples;
	uint8_t numChannels;
	uint8_t loops;
	double sample;
	float volume[2];
	float volumeDelta[2];
	float playbackRate;
	AudioChunk* queue;
} AudioTrack;

static int devId = 0;
static uint32_t numTracks = 0;
static float masterVolume;
static AudioTrack* tracks = NULL;
static SDL_AudioSpec audioSpec;

typedef struct {
	float* waveData;
	uint32_t waveLen;
	uint32_t offset;
	uint8_t numChannels;
} SampleSpec;

static SampleSpec* samples=NULL;
static uint32_t numSamples=0;
static uint32_t numSamplesMax=0;

static void Melody_cleanup(AudioTrack* t) {
	free(t->melody->melody);
	free(t->melody);
	t->melody = NULL;
	free((void*)t->waveData);
	t->waveData = NULL;
}

static void audioCallback(void *user_data, Uint8 *raw_buffer, int bytes) {
	uint32_t chunkSz = audioSpec.samples;
	Sint16 *buffer = (Sint16*)raw_buffer;
	AudioTrack* tracks = (AudioTrack*)user_data;

	for(uint32_t k=0; k<numTracks; ++k) {
		AudioTrack* t = &tracks[k];
		if(!t->melody)
			continue;
		if(!Melody_isPlaying(t->melody))
			Melody_cleanup(t);
		else {
			Melody_chunk(t->melody, audioSpec.freq, chunkSz, (float*)t->waveData);
			t->sample = 0.0;
			t->numSamples = chunkSz;
			t->numChannels = 1;
		}
		continue;
	}

	for(uint32_t i = 0; i < chunkSz; ++i) {
		float v[] = { 0.0f, 0.0f }, vol = 32767.0f * masterVolume;
		for(uint32_t k=0; k<numTracks; ++k) {
			AudioTrack* t = &tracks[k];
			if(t->sample >= t->numSamples) {
				if(!t->loops)
					continue;
				if(t->loops != UINT8_MAX)
					--t->loops;
				t->sample = 0.0;
			}
			float amplitude[] = { 0.0f, 0.0f };
			if(t->waveForm) {
				float timbre = defaultTimbre(t->waveForm);
				double time = t->sample / audioSpec.freq;
				amplitude[0] = amplitude[1] = vol * AudioOscillator(t->waveForm)(t->freq * time, timbre, &t->octx);
			}
			else {
				const float* waveData = t->waveData;
				if(t->queue) {
					AudioChunk* ch = t->queue;
					while(t->sample >= ch->numSamples && ch->next) {
						t->sample -= ch->numSamples;
						t->numSamples -= ch->numSamples;
						t->queue = ch->next;
						free(ch->waveData);
						free(ch);
						ch = t->queue;
					}
					waveData = (t->sample < ch->numSamples) ? ch->waveData : NULL;
				}
				if(waveData) {
					if(t->playbackRate==1.0f)
						for(uint8_t j=0; j<audioSpec.channels; ++j)
							amplitude[j] = vol * waveData[(uint32_t)t->sample * t->numChannels + (j%t->numChannels)];
					else { // linearly interpolate mono samples
						double fract = fmod(t->sample, 1.0f);
						uint32_t pos0 = t->sample, pos1 = pos0+1;
						float amp0 = waveData[pos0];
						float amp1 = (pos1>=t->numSamples) ? 0.0f : waveData[pos1];
						amplitude[0] = amplitude[1] = vol * ((1.0f-fract)*amp0 + fract*amp1);
					}
				}
			}

			for(uint8_t j=0; j<audioSpec.channels; ++j) {
				t->volume[j] += t->volumeDelta[j];
				v[j] += t->volume[j] * amplitude[j];
			}
			t->sample += t->playbackRate;
		}
		for(int j=0; j<audioSpec.channels; ++j)
			buffer[i*audioSpec.channels+j] = v[j]>32767.0f ? 32767 : v[j] < -32768.0f ? -32768 : (Sint16)v[j];
	}
}

uint32_t AudioOpen(uint32_t freq, uint32_t nTracks) {
	if(SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
		SDL_Log("Failed to initialize SDL audio: %s", SDL_GetError());
		return 0;
	}

	numTracks = nTracks;
	tracks = (AudioTrack*)malloc(sizeof(AudioTrack)*numTracks);
	for(uint32_t i=0; i<numTracks; ++i) {
		tracks[i].numSamples = 0;
		tracks[i].loops = 0;
		tracks[i].sample = 0.0;
		tracks[i].waveData = NULL;
		tracks[i].melody = NULL;
		tracks[i].playbackRate = 1.0f;
		tracks[i].queue = NULL;
	}

	SDL_AudioSpec want;
	want.freq = freq; // number of samples per second
	want.format = AUDIO_S16; // sample type (here: signed short i.e. 16 bit)
	want.channels = 2;
	want.samples = freq/60;
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

	// hack, playing any sample seems necessary for correct sound/melody volume: 
	float controlPoints[] = {0.0f,0.0f,0.001f,0.0f};
	uint32_t silence = AudioCreateSound(WAVE_NONE,1,controlPoints);
	AudioReplay(silence,1,0,0);
	return devId;
}

static void clearQueue(AudioChunk* ch) {
	while(ch) {
		AudioChunk* n = ch->next;
		free(ch->waveData);
		free(ch);
		ch = n;
	}
}

void AudioClose() {
	if(!devId)
		return;
	SDL_CloseAudioDevice(devId);
	devId = 0;
	for(uint32_t i=0; i<numTracks; ++i) {
		if(tracks[i].melody) {
			free((void*)tracks[i].waveData);
			free(tracks[i].melody->melody);
			free(tracks[i].melody);
		}
		clearQueue(tracks[i].queue);
	}
	free(tracks);

	if(numSamples) {
		for(uint32_t i=0; i<numSamples; ++i)
			free(samples[i].waveData);
		numSamples = numSamplesMax = 0;
		free(samples);
		samples = NULL;
	}
}

void AudioSuspend() {
	SDL_PauseAudioDevice(devId, 1);
}

void AudioResume() {
	SDL_PauseAudioDevice(devId, 0);
}

int AudioIsRunning() {
	return devId!=0 && SDL_GetAudioDeviceStatus(devId)==SDL_AUDIO_PLAYING;
}

void AudioSetVolume(float volume) {
	masterVolume = volume;
}

float AudioGetVolume() {
	return masterVolume;
}

uint32_t AudioTracks() {
	return numTracks;
}

uint32_t AudioSampleRate() {
	return (uint32_t)audioSpec.freq;
}

uint32_t AudioSound(SoundWave waveForm, float freq, float duration, float volume, float balance) {
	uint32_t ret = UINT_MAX;
	if(!devId)
		return ret;
	SDL_LockAudioDevice(devId);
	for(uint32_t i=0; i<numTracks; ++i)
		if(!AudioPlaying(i)) {
			AudioTrack* track = &tracks[i];
			track->waveForm = waveForm;
			track->octx.ampl = track->octx.amplPrev = 0.0f;
			track->octx.counter = 1;
			track->waveData = NULL;
			track->melody = NULL;
			track->freq = freq;
			track->loops = 0;
			track->sample = 0.0;
			track->numSamples = 0.5f + audioSpec.freq * duration;
			track->numChannels = 1;
			track->playbackRate = 1.0f;
			track->volume[0] = volume*(-0.4f*balance+0.6f);
			track->volume[1] = volume*(+0.4f*balance+0.6f);
			track->volumeDelta[0] = track->volumeDelta[1] = 0.0f;
			track->queue = NULL;
			ret = i;
			break;
		}
	SDL_UnlockAudioDevice(devId);
	return ret;
}

uint32_t AudioPlay(const float* data, uint32_t waveLen, uint8_t numChannels, float volume, float balance, float detune) {
	uint32_t ret = UINT_MAX;
	if(!devId || numChannels<1 || (detune && numChannels!=1))
		return ret;
	SDL_LockAudioDevice(devId);
	for(uint32_t i=0; i<numTracks; ++i)
		if(!AudioPlaying(i)) {
			AudioTrack* track = &tracks[i];
			track->waveForm = WAVE_NONE;
			track->waveData = data;
			track->melody = NULL;
			track->freq = 0.0f;
			track->loops = 0;
			track->sample = 0.0;
			track->numSamples = waveLen;
			track->numChannels = numChannels;
			track->playbackRate = pow(2.0f, detune/12.0f);
			track->volume[0] = volume*(-0.4f*balance+0.6f);
			track->volume[1] = volume*(+0.4f*balance+0.6f);
			track->volumeDelta[0] = track->volumeDelta[1] = 0.0f;
			track->queue = NULL;
			ret = i;
			break;
		}
	SDL_UnlockAudioDevice(devId);
	return ret;
}

uint32_t AudioQueue(uint8_t numChannels, float volume, float balance, float detune) {
	uint32_t ret = UINT_MAX;
	if(!devId || numChannels<1 || (detune && numChannels!=1))
		return ret;
	SDL_LockAudioDevice(devId);
	for(uint32_t i=0; i<numTracks; ++i)
		if(!AudioPlaying(i)) {
			AudioTrack* track = &tracks[i];
			track->waveForm = WAVE_NONE;
			track->waveData = NULL;
			track->melody = NULL;
			track->freq = 0.0f;
			track->loops = 0;
			track->sample = 0.0;
			track->numSamples = 0;
			track->numChannels = numChannels;
			track->playbackRate = pow(2.0f, detune/12.0f);
			track->volume[0] = volume*(-0.4f*balance+0.6f);
			track->volume[1] = volume*(+0.4f*balance+0.6f);
			track->volumeDelta[0] = track->volumeDelta[1] = 0.0f;
			track->queue = malloc(sizeof(AudioChunk));
			memset(track->queue, 0, sizeof(AudioChunk));
			ret = i;
			break;
		}
	SDL_UnlockAudioDevice(devId);
	return ret;
}

uint32_t AudioPush(uint32_t track, float* data, uint32_t numSamples) {
	if(!devId || track>=numTracks || !tracks[track].queue)
		return 0;
	SDL_LockAudioDevice(devId);
	AudioTrack* tr = &tracks[track];
	AudioChunk* ch = tr->queue;
	while(ch->next)
		ch = ch->next;
	if(data && numSamples) {
		if(ch->waveData) {
			ch->next = malloc(sizeof(AudioChunk));
			ch = ch->next;
		}
		const size_t numBytes = sizeof(float)*numSamples*tr->numChannels;
		ch->waveData = malloc(numBytes);
		memcpy(ch->waveData, data, numBytes);
		ch->numSamples = numSamples;
		ch->next = NULL;
		tr->numSamples += numSamples;
	}
	SDL_UnlockAudioDevice(devId);
	return tr->numSamples - (uint32_t)(tr->sample);
}

uint32_t AudioMelody(const char* melody, float volume, float balance) {
	uint32_t ret = UINT_MAX;
	if(!devId)
		return ret;
	SDL_LockAudioDevice(devId);
	for(uint32_t i=0; i<numTracks; ++i)
		if(!AudioPlaying(i)) {
			Melody* melo = Melody_create(melody);
			AudioTrack* track = &tracks[i];
			track->waveForm = WAVE_NONE;
			track->waveData = malloc(sizeof(float)*audioSpec.samples);
			track->melody = melo;
			track->freq = 0.0f;
			track->loops = 0;
			track->sample = 0.0;
			track->numSamples = audioSpec.samples;
			track->playbackRate = 1.0f;
			track->volume[0] = volume*(-0.4f*balance+0.6f);
			track->volume[1] = volume*(+0.4f*balance+0.6f);
			track->volumeDelta[0] = track->volumeDelta[1] = 0.0f;
			track->queue = NULL;
			ret = i;
			break;
		}
	SDL_UnlockAudioDevice(devId);
	return ret;
}

int AudioPlaying(uint32_t track) {
	if(!devId || track>=numTracks)
		return 0;
	AudioTrack* t = &tracks[track];
	return Melody_isPlaying(t->melody) || (t->sample < t->numSamples || t->loops || t->queue);
}

void AudioStop(uint32_t track) {
	if(!devId || track>=numTracks)
		return;
	SDL_LockAudioDevice(devId);
	AudioTrack* t = &tracks[track];
	if(Melody_isPlaying(t->melody))
		Melody_cleanup(t);
	else {
		t->sample = t->numSamples;
		t->loops = 0;
		clearQueue(t->queue);
		t->queue = NULL;
	}
	SDL_UnlockAudioDevice(devId);
}

void AudioFadeOut(uint32_t track, float deltaT) {
	if(!devId || track>=numTracks)
		return;
	SDL_LockAudioDevice(devId);
	AudioTrack* t = &tracks[track];
	uint32_t numSamples = AudioSampleRate()*deltaT, numSamplesMax = t->sample + numSamples;
	if(numSamplesMax < t->numSamples)
		t->numSamples = numSamplesMax;
	t->loops = 0; // FIXME the number of remaining loops could be calculated

	t->volumeDelta[0] = -t->volume[0] / numSamples;
	t->volumeDelta[1] = -t->volume[1] / numSamples;
	SDL_UnlockAudioDevice(devId);
}

void AudioAdjustVolume(uint32_t track, float volume) {
	if(!devId || track>=numTracks)
		return;
	SDL_LockAudioDevice(devId);
	AudioTrack* t = &tracks[track];
	float currVolume = (t->volume[0] + t->volume[1])/2.0f;
	if(!volume || !currVolume)
		t->volume[0] = t->volume[1] = volume * 0.6f;
	else {
		float factor = volume/currVolume;
		t->volume[0] *= factor;
		t->volume[1] *= factor;
	}
	SDL_UnlockAudioDevice(devId);
}

static float* AudioReadMP3(void* mp3data, uint32_t numBytes, uint32_t* samples, uint8_t* channels, uint32_t* offset) {
	if(!samples || !channels || !offset || !numBytes)
		return 0;
	*samples = *offset = *channels = 0;

	drmp3_config cfg = { 0/*numChannels unknown*/, audioSpec.freq };
	uint64_t pcmSamples;

	float* pcmData = drmp3_open_memory_and_read_pcm_frames_f32(mp3data, numBytes, &cfg, &pcmSamples, NULL);
	if(!pcmSamples || !cfg.outputChannels || cfg.outputChannels>2 || cfg.outputSampleRate!=audioSpec.freq || pcmSamples>SDL_MAX_UINT32) {
		drmp3_free(pcmData, NULL);
		return 0;
	}
	*samples = pcmSamples;
	*channels = cfg.outputChannels;
	// skip initial silence of MP3:
	for(; *offset<pcmSamples; ++(*offset))
		if(pcmData[*offset]>0.002 || pcmData[*offset]<-0.002)
			break;
	return pcmData;
}

static float* AudioReadWAV(void* wavdata, uint32_t numBytes, uint32_t* samples, uint8_t* channels, uint32_t* offset) {
	if(!samples || !channels || !offset || !numBytes)
		return 0;
	*samples = *offset = *channels = 0;
	SDL_RWops* rwo = SDL_RWFromConstMem(wavdata, (int)numBytes);
	SDL_AudioSpec wavSpec;
	uint8_t* buf;
	uint32_t buflen;
	if(!SDL_LoadWAV_RW(rwo, 1, &wavSpec, &buf, &buflen))
		return 0;
	*channels = wavSpec.channels>2 ? 2 : wavSpec.channels;

	SDL_AudioCVT cvt;
	SDL_BuildAudioCVT(&cvt, wavSpec.format, wavSpec.channels, wavSpec.freq, AUDIO_F32, *channels, audioSpec.freq);
	cvt.len = buflen;
	cvt.buf = malloc(cvt.len * cvt.len_mult);
	memcpy(cvt.buf, buf, buflen);
	SDL_FreeWAV(buf);
	SDL_ConvertAudio(&cvt);
	*samples = cvt.len_cvt/sizeof(float)/(*channels);
	return (float*)cvt.buf;
}

float* AudioRead(void* data, uint32_t numBytes, uint32_t* samples, uint8_t* channels, uint32_t* offset) {
	if(!samples || !channels || !offset || numBytes<4)
		return 0;
	*samples = *offset = *channels = 0;
	const char* cdata = data;
	if(cdata[0]=='R' && cdata[1]=='I' && cdata[2]=='F' && cdata[3]=='F')
		return AudioReadWAV(data, numBytes, samples, channels, offset);
	return AudioReadMP3(data, numBytes, samples, channels, offset);
}

uint32_t AudioUpload(void* data, uint32_t numBytes) {
	uint32_t samples, offset;
	uint8_t channels;
	float* pcmdata = AudioRead(data, numBytes, &samples, &channels, &offset);
	return AudioUploadPCM(pcmdata, samples, channels, offset);
}

uint32_t AudioUploadPCM(float* waveData, uint32_t waveLen, uint8_t numChannels, uint32_t offset) {
	if(!waveData || !waveLen || offset>waveLen || numChannels<1 || numChannels>2)
		return 0;
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
	samples[numSamples].offset = offset;
	samples[numSamples].numChannels = numChannels;
	return ++numSamples;
}

uint32_t AudioReplay(uint32_t sample, float volume, float balance, float detune) {
	if(!sample || sample>numSamples)
		return UINT_MAX;
	uint8_t numChannels = samples[--sample].numChannels;
	if(numChannels!=1 && (detune || balance))
		return UINT_MAX;

	uint32_t offset = samples[sample].offset;
	return AudioPlay(samples[sample].waveData+offset*numChannels,
		samples[sample].waveLen-offset, numChannels, volume, balance, detune);
}

uint32_t AudioLoop(uint32_t sample, float volume, float balance, float detune) {
	uint32_t track = AudioReplay(sample, volume, balance, detune);
	if(track!= UINT_MAX)
		tracks[track].loops = UINT8_MAX;
	return track;
}

void AudioRelease(uint32_t sample) {
	if(!sample || sample>numSamples)
		return;
	SampleSpec* s = &samples[sample-1];
	if(!s->waveData)
		return;

	SDL_LockAudioDevice(devId);
	for(uint32_t i=0; i<numTracks; ++i) {
		AudioTrack* t = &tracks[i];
		if(t->melody || (t->sample >= t->numSamples))
			continue;
		if(t->waveData >= s->waveData && t->waveData < s->waveData+s->waveLen*s->numChannels*sizeof(float))
			t->sample = t->numSamples;
	}
	SDL_UnlockAudioDevice(devId);

	free(s->waveData);
	s->waveData = NULL;
	s->waveLen = s->offset = s->numChannels = 0;
}

//--- experimental extensions custom sound generators --------------

float* AudioSampleBuffer(uint32_t sample, uint32_t* numSamples) {
	if(numSamples)
		*numSamples = 0;

	if(!sample)
		return NULL;
	SampleSpec* s = &samples[--sample];
	if(s->numChannels!=1)
		return NULL;
	if(numSamples)
		*numSamples = s->waveLen - s->offset;
	return s->waveData + s->offset;
}

void AudioMixToBuffer(uint32_t stereoBufLen, float* stereoBuffer, uint32_t sampleLen, const float* sampleBuf,
	double startTime, float volume, float balance)
{
	if(!stereoBufLen || !stereoBuffer || !sampleBuf || !sampleLen)
		return;
	const float volL = volume*(-0.4f*balance+0.6f), volR = volume*(+0.4f*balance+0.6f);
	for(uint32_t frame = round(startTime*AudioSampleRate()), pos = 0; frame<stereoBufLen && pos<sampleLen; ++frame, ++pos) {
		stereoBuffer[frame*2] += sampleBuf[pos] * volL;
		stereoBuffer[frame*2+1] += sampleBuf[pos] * volR;
	}
}

void AudioClampBuffer(uint32_t bufSz, float* buffer, float minValue, float maxValue) {
	for(uint32_t i=0; i<bufSz; ++i)
		if(buffer[i] < minValue)
			buffer[i] = minValue;
		else if(buffer[i] > maxValue)
			buffer[i] = maxValue;
}

static float chirp(Oscillator_t osc, OscillatorCtx* ctx, float* samples, uint32_t numSamples,
	float phase, float freq1, float freq2, float vol1, float vol2, float timbre1, float timbre2)
{
	const float dvol = vol2-vol1, dfreq = freq2-freq1, dtimbre = timbre2-timbre1, dt=1.0f/AudioSampleRate();
	float t=0;
	for(uint32_t i=0; i<numSamples; ++i, t+=dt) {
		const float rel = (float)i/(float)numSamples;
		const float vol = vol1 + dvol*rel, freq=freq1 + dfreq*rel, timbre=timbre1 + dtimbre*rel;
		samples[i] = fmaxf(-1.0f, fminf(1.0f, osc(phase, timbre, ctx)*vol));
		phase += freq*dt;
	}
	return phase;
}

float* AudioCreateSoundBuffer(SoundWave waveForm, uint8_t numControlPoints, const float shape[], uint32_t* numSamples) {
	Oscillator_t osc = AudioOscillator(waveForm);
	OscillatorCtx ctx = { 0.0f, 0.0f, 1 };

	uint32_t pos=0, nSamples=0;
	for(uint8_t i=0; i<numControlPoints; ++i)
		nSamples += fmaxf(0.0f, shape[i*4+2]) * AudioSampleRate();
	if(numSamples)
		*numSamples = nSamples;
	if(!nSamples)
		return NULL;
	float* buffer = malloc(sizeof(float)*nSamples);

	float phase=0, freq1, vol1, timbre1;
	for(uint8_t i=0; i<numControlPoints; ++i) {

		const float freq2 = shape[i*4], vol2 = shape[i*4+1];
		const float duration = shape[i*4+2], timbre2 = shape[i*4+3];
		if(duration>0.0f) {
			if(i==0) {
				freq1 = freq2;
				vol1 = vol2;
				timbre1 = timbre2;
			}
			uint32_t chirpSz = duration*AudioSampleRate();
			phase = chirp(osc, &ctx, &buffer[pos], chirpSz, phase, freq1, freq2, vol1, vol2, timbre1, timbre2);
			pos += chirpSz;
		}
		freq1 = freq2;
		vol1 = vol2;
		timbre1 = timbre2;
	}
	return buffer;
}

uint32_t AudioCreateSound(SoundWave waveForm, uint8_t numControlPoints, const float shape[]) {
	if(!numControlPoints)
		return 0;
	uint32_t numSamples;
	float* samples = AudioCreateSoundBuffer(waveForm, numControlPoints, shape, &numSamples);
	if(!numSamples)
		return 0;
	return AudioUploadPCM(samples, numSamples, 1, 0);
}
