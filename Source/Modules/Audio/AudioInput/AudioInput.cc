#include "ikaros.h"
#include <AudioToolbox/AudioToolbox.h>
#include <vector>

using namespace ikaros;

class AudioInput: public Module
{
public:
    AudioInput() : isRecording(true) {}
    virtual ~AudioInput();

    void Init();
    void Tick();

private:
    static void HandleInputBuffer(void* inUserData, 
                                  AudioQueueRef inAQ, 
                                  AudioQueueBufferRef inBuffer, 
                                  const AudioTimeStamp* inStartTime, 
                                  UInt32 inNumberPacketDescriptions, 
                                  const AudioStreamPacketDescription* inPacketDescs);

    parameter(samplingrate);
    matrix output;
    int outputSize;

    AudioQueueRef queue;
    AudioQueueBufferRef buffer;
    std::vector<float> recordedSamples;
    bool isRecording;
};

AudioInput::~AudioInput()
{
    if (queue) {
        AudioQueueStop(queue, true);
        AudioQueueDispose(queue, true);
    }
}

void AudioInput::Init()
{
    Bind(samplingrate, "samplingrate");
    Bind(output, "OUTPUT");
    std::cout << "outputsize: " << (int)output.size() << "\n";
    outputSize = output.size();
    recordedSamples.reserve(outputSize);

    AudioStreamBasicDescription format = {0};
    format.mSampleRate = samplingrate;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kLinearPCMFormatFlagIsFloat | kLinearPCMFormatFlagIsPacked;
    format.mBitsPerChannel = 32;
    format.mChannelsPerFrame = 1;
    format.mBytesPerPacket = format.mBytesPerFrame = format.mChannelsPerFrame * sizeof(float);
    format.mFramesPerPacket = 1;

    OSStatus status = AudioQueueNewInput(&format, HandleInputBuffer, this, nullptr, nullptr, 0, &queue);
    if (status != noErr) {
        Notify(msg_fatal_error, "Failed to create audio queue");
        return;
    }

    int bufferSize = samplingrate * sizeof(float);  // 1 second buffer
    status = AudioQueueAllocateBuffer(queue, bufferSize, &buffer);
    if (status != noErr) {
        Notify(msg_fatal_error, "Failed to allocate audio buffer");
        return;
    }

    status = AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
    if (status != noErr) {
        Notify(msg_fatal_error, "Failed to enqueue audio buffer");
        return;
    }

    status = AudioQueueStart(queue, nullptr);
    if (status != noErr) {
        Notify(msg_fatal_error, "Failed to start audio queue");
        return;
    }
}

void AudioInput::Tick()
{
    int samplesToCopy = std::min(outputSize, (int)recordedSamples.size());
    std::cout << "recorded samples: " << (int)recordedSamples.size() << "\n";
    std::cout << "outputsize: " << (int)output.size() << "\n";
    for (int i = 0; i < samplesToCopy; i++) {
        output[i] = recordedSamples[recordedSamples.size() - samplesToCopy + i];
    }

    recordedSamples.clear();
}

void AudioInput::HandleInputBuffer(void* inUserData, 
                                   AudioQueueRef inAQ, 
                                   AudioQueueBufferRef inBuffer, 
                                   const AudioTimeStamp* inStartTime, 
                                   UInt32 inNumberPacketDescriptions, 
                                   const AudioStreamPacketDescription* inPacketDescs)
{
    AudioInput* recorder = static_cast<AudioInput*>(inUserData);
    if (!recorder->isRecording) return;

    float* bufferData = static_cast<float*>(inBuffer->mAudioData);
    int numSamples = inBuffer->mAudioDataByteSize / sizeof(float);

    recorder->recordedSamples.insert(recorder->recordedSamples.end(), bufferData, bufferData + numSamples);

    // Re-enqueue the buffer for continuous recording
    AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, nullptr);
}

// Add the installation of the module
INSTALL_CLASS(AudioInput);
