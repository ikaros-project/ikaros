#ifndef AUDIOFILE_H
#define AUDIOFILE_H

#include <vector>
#include <iostream>
#include <fstream>
#include <string>

class AudioFile {
public:
    AudioFile(const std::string &filename);

    float getLeftSample(int index) const;
    float getRightSample(int index) const;
    float getSample(int channel, int index) const;
    int getSize() const;

    int getSampleRate() const { return sampleRate; }
    int getBitDepth() const { return bitDepth; }
    int getNumChannels() const { return numChannels; }
    bool isStereo() const { return numChannels == 2; }

private:
    std::vector<float> leftChannelData;
    std::vector<float> rightChannelData;
    int sampleRate;
    int bitDepth;
    int numSamples;
    int numChannels;

    void loadWAV(const std::string &filename);
    void loadAIFF(const std::string &filename);
};

#endif // AUDIOFILE_H
