#pragma once
#include <stddef.h>

//--- audio management API ----------------------------------------

typedef enum {
	WAVE_NONE = 0,
	WAVE_SINE,
	WAVE_TRIANGLE,
	WAVE_SQUARE,
	WAVE_SAWTOOTH,
	WAVE_NOISE,
} SoundWave;

/// opens an SDL stereo audio device
unsigned AudioOpen(unsigned freq, unsigned tracks);
/// closes audio devices
void AudioClose();
/// sets master volume
void AudioSetVolume(float volume);
/// returns master volume
float AudioGetVolume();
/// returns total number of tracks
unsigned AudioTracks();
/// immediately plays a sound, balance 0.0 means center, -1.0 left, +1.0 right
/** \return track number playing this sound or UINT_MAX if no track is available */
unsigned AudioSound(SoundWave waveForm, int freq, float duration, float volume, float balance);
/// immediately plays a user-generated mono wave data sample
/** \return track number playing this sound or UINT_MAX if no track is available */
unsigned AudioPlay(const float* data, unsigned numSamples, float volume, float balance);
/// immediately plays a sequence of notes, e.g., AudioMelody("{w:tri a:.025 d:.025 s:.25 r:.05 b:120} A3/12 C#4/12 E4/12 {s:.5 r:.45 g:1.5} A4/4", 0.66f, 0.0f);
/** \return track number playing this sound or UINT_MAX if no track is available */
unsigned AudioMelody(const char* melody, float volume, float balance);
/// test if track is currently playing
int AudioPlaying(unsigned track);
/// stops a currently playing track
void AudioStop(unsigned track);
/// uploads mono audio file data (mp3) and returns handle for later playback
size_t AudioUpload(void* mp3data, unsigned numBytes);
/// buffers a mono wave data sample for later playback
/** wave data memory ownership is passed to the function */
size_t AudioSample(float* waveData, unsigned numSamples);
/// immediately plays previously uploaded sample data
/** \return track number playing this sound or UINT_MAX if no track is available */
unsigned AudioReplay(size_t sample, float volume, float balance);

//--- building blocks for custom sound generators ------------------

typedef float (*Oscillator_t)(float);
Oscillator_t AudioOscillator(SoundWave waveForm);
float envelope(float attack, float decay, float sustainLvl, float sustainLen, float release, float t);
float transposeFreq(float base, int n);

