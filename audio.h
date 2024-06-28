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
extern uint32_t AudioOpen(uint32_t freq, uint32_t tracks);
/// closes audio devices
extern void AudioClose();
/// (temporarily) suspends all audio output
extern void AudioSuspend();
/// resumes previously suspended audio output
extern void AudioResume();
/// checks if audio output is not suspended
extern int AudioIsRunning();
/// sets master volume
extern void AudioSetVolume(float volume);
/// returns master volume
extern float AudioGetVolume();
/// returns total number of tracks
extern uint32_t AudioTracks();
/// returns the device's sample rate
extern uint32_t AudioSampleRate();
/// immediately plays a sound, balance 0.0 means center, -1.0 left, +1.0 right
/** \return track number playing this sound or UINT_MAX if no track is available */
extern uint32_t AudioSound(SoundWave waveForm, float freq, float duration, float volume, float balance);
/// immediately plays a mono or stereo PCM wave data sample
/** \return track number playing this sample or UINT_MAX if no track is available */
extern uint32_t AudioPlay(const float* data, uint32_t numSamples, uint8_t numChannels, float volume, float balance, float detune);
/// creates a queue for sequentially playing chunks of samples
/** \return track number playing this queue or UINT_MAX if no track is available */
extern uint32_t AudioQueue(uint8_t numChannels, float volume, float balance, float detune);
/// pushes a chunk of samples onto an existing queue
/** \param data is copied on push, so the caller retains the ownership of the source data*/
/** \return overall number of queued samples */
extern uint32_t AudioPush(uint32_t track, float* data, uint32_t numSamples);
/// immediately plays a sequence of notes, e.g., AudioMelody("{w:tri a:.025 d:.025 s:.25 r:.05 b:120} A3/12 C#4/12 E4/12 {s:.5 r:.45 g:1.5} A4/4", 0.66f, 0.0f);
/** \return track number playing this sound or UINT_MAX if no track is available */
extern uint32_t AudioMelody(const char* melody, float volume, float balance);
/// test if track is currently playing
extern int AudioPlaying(uint32_t track);
/// stops a currently playing track
extern void AudioStop(uint32_t track);
/// fades out a currently playing track
extern void AudioFadeOut(uint32_t track, float deltaT);
/// adjusts volume of a currently playing track
extern void AudioAdjustVolume(uint32_t track, float volume);
/// tries to convert a MP3 or WAV sample file to an audio buffer
extern float* AudioRead(void* data, uint32_t numBytes, uint32_t* samples, uint8_t* channels, uint32_t* offset);
/// converts and uploads an audio sample in MP3 or WAV format, returns a handle for later playback
extern uint32_t AudioUpload(void* data, uint32_t numBytes);
/// uploads mono or stereo PCM wave data and returns a handle for later playback
/** wave data memory ownership is passed to the function */
extern uint32_t AudioUploadPCM(float* waveData, uint32_t numSamples, uint8_t numChannels, uint32_t offset);
/// releases an uploaded audio sample from memory
extern void AudioRelease(uint32_t sample);
/// immediately plays previously uploaded sample data
/** \note For stereo samples, detune and balance must be 0.0f
 * \return track number playing this sound or UINT_MAX if the input is invalid or no track is available */
extern uint32_t AudioReplay(uint32_t sample, float volume, float balance, float detune);
/// repeatedly plays previously uploaded sample data
/** \note For stereo samples, detune and balance must be 0.0f
 * \return track number playing this sound or UINT_MAX if the input is invalid or no track is available */
extern uint32_t AudioLoop(uint32_t sample, float volume, float balance, float detune);

//--- experimental extensions custom sound generators --------------

/// creates a mono sound buffer based on an oscillator and frequency/volume/shape curves
extern float* AudioCreateSoundBuffer(SoundWave waveForm, uint8_t numControlPoints, const float shape[], uint32_t* numSamples);
/// creates a mono sound sample based on an oscillator and frequency/volume/shape curves, returns a handle for later playback
extern uint32_t AudioCreateSound(SoundWave waveForm, uint8_t numControlPoints, const float shape[]);
/// access to a sample buffer
extern float* AudioSampleBuffer(uint32_t sample, uint32_t* numSamples);
/// clamps values within a buffer by a given minValue/maxValue interval
extern void AudioClampBuffer(uint32_t bufSz, float* buffer, float minValue, float maxValue);
/// mixes a sample to a stereo buffer
extern void AudioMixToBuffer(uint32_t stereoBufLen, float* stereoBuffer,
	uint32_t sampleBufLen, const float* sampleBuffer,
	double startTime, float volume, float balance);

typedef struct { float ampl; float amplPrev; uint32_t counter; } OscillatorCtx;
typedef float (*Oscillator_t)(float, float, OscillatorCtx*);
Oscillator_t AudioOscillator(SoundWave waveForm);
extern float envelope(float attack, float decay, float sustainLvl, float sustainLen, float release, float t);
extern float transposeFreq(float base, int n);
extern float note2freq(char note, char accidental, int octave);

