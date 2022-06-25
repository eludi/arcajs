#pragma once
#include <stddef.h>
#include <stdint.h>

//--- audio management API ----------------------------------------

typedef enum {
	WAVE_NONE = 0,
	WAVE_SINE,
	WAVE_TRIANGLE,
	WAVE_SQUARE,
	WAVE_SAWTOOTH,
	WAVE_NOISE,
	WAVE_BINNOISE,
} SoundWave;

/// opens an SDL stereo audio device
uint32_t AudioOpen(uint32_t freq, uint32_t tracks);
/// closes audio devices
void AudioClose();
/// sets master volume
void AudioSetVolume(float volume);
/// returns master volume
float AudioGetVolume();
/// returns total number of tracks
uint32_t AudioTracks();
/// returns the device's sample rate
uint32_t AudioSampleRate();
/// immediately plays a sound, balance 0.0 means center, -1.0 left, +1.0 right
/** \return track number playing this sound or UINT_MAX if no track is available */
uint32_t AudioSound(SoundWave waveForm, int freq, float duration, float volume, float balance);
/// immediately plays a mono or stereo wave data sample
/** \return track number playing this sample or UINT_MAX if no track is available */
uint32_t AudioPlay(const float* data, uint32_t numSamples, uint8_t numChannels, float volume, float balance, float detune);
/// immediately plays a sequence of notes, e.g., AudioMelody("{w:tri a:.025 d:.025 s:.25 r:.05 b:120} A3/12 C#4/12 E4/12 {s:.5 r:.45 g:1.5} A4/4", 0.66f, 0.0f);
/** \return track number playing this sound or UINT_MAX if no track is available */
uint32_t AudioMelody(const char* melody, float volume, float balance);
/// test if track is currently playing
int AudioPlaying(uint32_t track);
/// stops a currently playing track
void AudioStop(uint32_t track);
/// fades out a currently playing track
void AudioFadeOut(uint32_t track, float deltaT);
/// adjusts volume of a currently playing track
void AudioAdjustVolume(uint32_t track, float volume);
/// uploads an audio MP3 sample and returns a handle for later playback
uint32_t AudioUploadMP3(void* mp3data, uint32_t numBytes);
/// uploads an audio WAV sample and returns a handle for later playback
uint32_t AudioUploadWAV(void* wavdata, uint32_t numBytes);
/// uploads mono or stereo PCM wave data and returns a handle for later playback
/** wave data memory ownership is passed to the function */
uint32_t AudioUploadPCM(float* waveData, uint32_t numSamples, uint8_t numChannels, uint32_t offset);
/// immediately plays previously uploaded sample data
/** \note For stereo samples, detune and balance must be 0.0f
 * \return track number playing this sound or UINT_MAX if the input is invalid or no track is available */
uint32_t AudioReplay(uint32_t sample, float volume, float balance, float detune);

//--- experimental extensions custom sound generators --------------

/// creates a mono sound buffer based on an oscillator and frequency/volume/shape curves
float* AudioCreateSoundBuffer(SoundWave waveForm, uint8_t numControlPoints, const float shape[], uint32_t* numSamples);
/// creates a mono sound sample based on an oscillator and frequency/volume/shape curves, returns a handle for later playback
uint32_t AudioCreateSound(SoundWave waveForm, uint8_t numControlPoints, const float shape[]);
/// access to a sample buffer
float* AudioSampleBuffer(uint32_t sample, uint32_t* numSamples);
/// clamps values within a buffer by a given minValue/maxValue interval
void AudioClampBuffer(uint32_t bufSz, float* buffer, float minValue, float maxValue);
/// mixes a sample to a stereo buffer
void AudioMixToBuffer(uint32_t stereoBufLen, float* stereoBuffer,
	uint32_t sampleBufLen, const float* sampleBuffer,
	double startTime, float volume, float balance);

typedef struct { float ampl; float amplPrev; uint32_t counter; } OscillatorCtx;
typedef float (*Oscillator_t)(float, float, OscillatorCtx*);
Oscillator_t AudioOscillator(SoundWave waveForm);
float envelope(float attack, float decay, float sustainLvl, float sustainLen, float release, float t);
float transposeFreq(float base, int n);
float note2freq(char note, char accidental, int octave);

