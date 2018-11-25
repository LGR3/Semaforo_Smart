/**
 * @file audio.h
 * @brief Contains AudioData structure, and relevent audio functions.
 */

#include <stdint.h>
#include <string.h>

/**
 * Holds all of the necessary information for building an
 * audio file.
 * @var duration Contains how long the audio file will be in seconds.
 * @var formatType Contains the audio formatting type.
 * @var numberOfChannels Contains the number of audio channels.
 * @var sampleRate Contains the sample rate in Hertz.
 * @var frameIndex Contains the current frame to be processed.
 * @var maxFrameIndex Contains the number of frames that the audio file will store.
 * @var recordedSamples Contains the raw PCM audio data.
 */

typedef struct
{
    uint32_t duration;
    uint16_t formatType;
    uint16_t numberOfChannels;
    uint32_t sampleRate;
    uint32_t frameIndex;
    uint32_t maxFrameIndex;
    float* recordedSamples;
} AudioData;

int recordFLAC(AudioData data, const char *fileName);
AudioData initAudioData(uint32_t sample_rate, uint16_t channels, uint32_t duration);