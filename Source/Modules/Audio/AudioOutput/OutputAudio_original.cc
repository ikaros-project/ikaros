#include "ikaros.h"
#include <AudioToolbox/AudioToolbox.h>
#include <atomic>
#include <thread>

using namespace ikaros;

class OutputAudio : public Module
{
public:
    matrix input;
    parameter sampleRate;
    parameter bufferSize;

    AudioQueueRef queue;
    AudioStreamBasicDescription asbd;
    std::atomic<bool> isPlaying;
    std::thread playbackThread;

    void Init()
    {
        Bind(input, "INPUT");
        Bind(sampleRate, "sample_rate");
        Bind(bufferSize, "buffer_size");

        // Initialize audio queue
        memset(&asbd, 0, sizeof(asbd));
        asbd.mSampleRate = sampleRate.as_int();
        asbd.mFormatID = kAudioFormatLinearPCM;
        asbd.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved;
        asbd.mBitsPerChannel = 32;
        asbd.mChannelsPerFrame = 1;
        asbd.mFramesPerPacket = 1;
        asbd.mBytesPerFrame = 4;
        asbd.mBytesPerPacket = 4;

        AudioQueueNewOutput(&asbd, audioQueueOutputCallback, this, NULL, NULL, 0, &queue);

        isPlaying = false;
    }

    void Tick()
    {
        if (!isPlaying)
        {
            // Start a new playback thread
            if (playbackThread.joinable())
            {
                playbackThread.join();
            }
            playbackThread = std::thread(&OutputAudio::playAudio, this);
        }
    }

    void playAudio()
    {
        isPlaying = true;

        // Allocate and enqueue buffers
        for (int i = 0; i < 3; ++i)
        {
            AudioQueueBufferRef buffer;
            AudioQueueAllocateBuffer(queue, bufferSize.as_int() * sizeof(float), &buffer);
            audioQueueOutputCallback(this, queue, buffer);
        }

        AudioQueueStart(queue, NULL);

        // Wait for playback to complete
        while (isPlaying)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        AudioQueueStop(queue, true);
    }

    static void audioQueueOutputCallback(void* inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer)
    {
        OutputAudio* audioModule = static_cast<OutputAudio*>(inUserData);
        int framesToCopy = std::min(audioModule->bufferSize.as_int(), static_cast<int>(inBuffer->mAudioDataBytesCapacity / sizeof(float)));

        if (framesToCopy > 0)
        {
            memcpy(inBuffer->mAudioData, audioModule->input.data(), framesToCopy * sizeof(float));
            inBuffer->mAudioDataByteSize = framesToCopy * sizeof(float);
        }
        else
        {
            inBuffer->mAudioDataByteSize = 0;
            audioModule->isPlaying = false;
        }

        AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
    }

    ~OutputAudio()
    {
        if (playbackThread.joinable())
        {
            playbackThread.join();
        }
        AudioQueueDispose(queue, true);
    }
};

INSTALL_CLASS(OutputAudio)