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
/// immediately plays a mono wave data sample
/** \return track number playing this sample or UINT_MAX if no track is available */
uint32_t AudioPlay(const float* data, uint32_t numSamples, float volume, float balance, float detune);
/// immediately plays a sequence of notes, e.g., AudioMelody("{w:tri a:.025 d:.025 s:.25 r:.05 b:120} A3/12 C#4/12 E4/12 {s:.5 r:.45 g:1.5} A4/4", 0.66f, 0.0f);
/** \return track number playing this sound or UINT_MAX if no track is available */
uint32_t AudioMelody(const char* melody, float volume, float balance);
/// test if track is currently playing
int AudioPlaying(uint32_t track);
/// stops a currently playing track
void AudioStop(uint32_t track);
/// uploads mono audio MP3 sample and returns handle for later playback
size_t AudioUploadMP3(void* mp3data, uint32_t numBytes);
/// uploads mono audio WAV sample and returns handle for later playback
size_t AudioUploadWAV(void* wavdata, uint32_t numBytes);
/// buffers a mono wave data sample for later playback
/** wave data memory ownership is passed to the function */
size_t AudioSample(float* waveData, uint32_t numSamples, uint32_t offset);
/// immediately plays previously uploaded sample data
/** \return track number playing this sound or UINT_MAX if no track is available */
uint32_t AudioReplay(size_t sample, float volume, float balance, float detune);

//--- building blocks for custom sound generators ------------------

typedef float (*Oscillator_t)(float);
Oscillator_t AudioOscillator(SoundWave waveForm);
float envelope(float attack, float decay, float sustainLvl, float sustainLen, float release, float t);
float transposeFreq(float base, int n);

