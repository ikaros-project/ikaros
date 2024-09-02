#include "ikaros.h"
#include <cmath>
#include <random>

using namespace ikaros;

class AudioSynth: public Module
{
public:


    void Init();
    void Tick();

private:
    float generateWave(float phase, float waveshape);
    float sineWave(float phase);
    float triangleWave(float phase);
    float sawtoothWave(float phase);
    float squareWave(float phase);
    float noiseWave(float phase);
    float interpolateWaves(float w1, float w2, float t);
    float midiNoteToFrequency(float note);
    //void updateTimeStep();

    // Parameters
    parameter sampleRate;
    //parameter bufferSize;

    // Internal state
    float phase;
    //float timeStep;
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution;

    // Input/Output
    matrix waveshape;
    matrix frequency;
    matrix amplitude;
    matrix trigger;
    matrix output;
};

void AudioSynth::Init()
{
    Bind(sampleRate, "sample_rate");
    //Bind(bufferSize, "buffer_size");

    Bind(waveshape, "WAVESHAPE");
    Bind(frequency, "FREQUENCY");
    Bind(amplitude, "AMPLITUDE");
    Bind(trigger, "TRIGGER");
    Bind(output, "OUTPUT");

    phase = 0.0f;
    //updateTimeStep();
    distribution = std::uniform_real_distribution<float>(-1.0f, 1.0f);
}


void AudioSynth::Tick()
{
    float time = kernel().GetNominalTime();
    double sr = sampleRate;
    double sx = output.size_x();
    double time_increment = kernel().GetTickDuration()/double(output.size_x()); // Dimension of the last dimension that holds the buffer
    float timeStep = 1.0f / sampleRate;

    float currentFreq = frequency.empty() ? 440.f : midiNoteToFrequency(frequency(0) * 88 + 21); // Map 0-1 to notes 21-109 (A0 to C8)
    float currentWaveshape = waveshape.empty() ? 0.25 : waveshape(0);
    float currentAmplitude = amplitude.empty() ? 0.5 : amplitude(0);
    bool isTriggered = trigger.empty() ? true : trigger(0) > 0.5f;

    for (int i = 0; i < sx; i++)
    {
        if (isTriggered)
        {
            output(i) = currentAmplitude * generateWave(phase, currentWaveshape);
            
            //phase = 2.0f * M_PI*(time + double(i) * time_increment) * currentFreq ;
            
            phase += 2.0f * M_PI * currentFreq * timeStep;
            while (phase > 2.0f * M_PI)
                phase -= 2.0f * M_PI;
        }
        else
        {
            output(i) = 0.0f;
        }
    }
}


// void AudioSynth::updateTimeStep()
// {
//     timeStep = 1.0f / sampleRate;
// }

float AudioSynth::midiNoteToFrequency(float note)
{
    return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
}

float AudioSynth::generateWave(float phase, float waveshape)
{
    if (waveshape <= 0.25f)
        return interpolateWaves(sineWave(phase), triangleWave(phase), waveshape * 4.0f);
    else if (waveshape <= 0.5f)
        return interpolateWaves(triangleWave(phase), sawtoothWave(phase), (waveshape - 0.25f) * 4.0f);
    else if (waveshape <= 0.75f)
        return interpolateWaves(sawtoothWave(phase), squareWave(phase), (waveshape - 0.5f) * 4.0f);
    else
        return interpolateWaves(squareWave(phase), noiseWave(phase), (waveshape - 0.75f) * 4.0f);
        
}

float AudioSynth::sineWave(float phase)
{
    return std::sin(phase);
}

float AudioSynth::triangleWave(float phase)
{
    return 2.0f * std::abs(2.0f * (phase / (2.0f * M_PI)) - 1.0f) - 1.0f;
}

float AudioSynth::sawtoothWave(float phase)
{
    return 2.0f * (phase / (2.0f * M_PI)) - 1.0f;
}

float AudioSynth::squareWave(float phase)
{
    return phase < M_PI ? 1.0f : -1.0f;
}

float AudioSynth::noiseWave(float phase)
{
    return distribution(generator);
}

float AudioSynth::interpolateWaves(float w1, float w2, float t)
{
    return w1 * (1.0f - t) + w2 * t;
}


INSTALL_CLASS(AudioSynth)

