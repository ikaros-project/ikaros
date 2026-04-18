#include "ikaros.h"
#include <AudioToolbox/AudioToolbox.h>
#include <atomic>
#include <thread>


using namespace ikaros;

class AudioOutput : public Module
{
public:
    matrix input;
    matrix buffer;

    parameter sampleRate;
    parameter bufferSize;

    AudioQueueRef queue = nullptr;
    AudioStreamBasicDescription asbd;
    std::atomic<bool> isPlaying{false};
    
    std::thread playbackThread;
    std::mutex inputMutex; // Mutex to protect the input matrix
    bool warned_buffer_size = false;

    void WarnIfBufferGeometryIsFractional(double sr)
    {
        if(warned_buffer_size || sr <= 0.0)
            return;

        const double exact_samples = sr * GetTickDuration();
        const double rounded_samples = std::round(exact_samples);
        if (std::abs(exact_samples - rounded_samples) > 1e-6)
        {
            Warning("AudioOutput sample_rate * tick_duration is not an integer number of samples (" + std::to_string(exact_samples) + "). Audio buffers may not match timing exactly.");
            warned_buffer_size = true;
        }
    }

    void Init()
    {
        Bind(input, "INPUT");
        Bind(sampleRate, "sample_rate");
        Bind(bufferSize, "buffer_size");

        WarnIfBufferGeometryIsFractional(sampleRate.as_double());

        buffer.copy(input);


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

        isPlaying = true;

        // Start playback thread during initialization
        playbackThread = std::thread(&AudioOutput::playAudio, this);
    }


    int c = 0;

    void Tick()
    {
        std::lock_guard<std::mutex> lock(inputMutex);
        //if(c++< 10)
        buffer.copy(input);
        // input.print();
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
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

        AudioQueueStop(queue, true);
    }

    static void audioQueueOutputCallback(void* inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer)
    {
        AudioOutput* audioModule = static_cast<AudioOutput*>(inUserData);
        int framesToCopy = std::min(audioModule->bufferSize.as_int(), static_cast<int>(inBuffer->mAudioDataBytesCapacity / sizeof(float)));

        if (framesToCopy > 0)
        {
            // Lock the mutex before accessing the input matrix
            std::lock_guard<std::mutex> lock(audioModule->inputMutex);
            memcpy(inBuffer->mAudioData, audioModule->buffer.data(), framesToCopy * sizeof(float));
            inBuffer->mAudioDataByteSize = framesToCopy * sizeof(float);
        }
        else
        {
            inBuffer->mAudioDataByteSize = 0;
            audioModule->isPlaying = false;
            std::cout << "PLAY STOPPED" << std::endl;
        }

        AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
    }

    ~AudioOutput()
    {
        isPlaying = false;
        if (queue != nullptr)
            AudioQueueStop(queue, true);
        if (playbackThread.joinable())
        {
            playbackThread.join();
        }
        if (queue != nullptr)
            AudioQueueDispose(queue, true);
    }
};

INSTALL_CLASS(AudioOutput)
