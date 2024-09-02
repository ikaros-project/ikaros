#include "AudioFile.h"
#include <cstring>  // For std::strncmp
#include <algorithm> // For std::transform

AudioFile::AudioFile(const std::string &filename) {
    if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".wav") == 0) {
        loadWAV(filename);
    } else if (filename.size() >= 5 && filename.compare(filename.size() - 5, 5, ".aiff") == 0) {
        loadAIFF(filename);
    } else {
        std::cerr << "Unsupported file format: " << filename << std::endl;
    }
}

void AudioFile::loadWAV(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open WAV file: " << filename << std::endl;
        return;
    }

    file.seekg(22);
    file.read(reinterpret_cast<char*>(&numChannels), sizeof(numChannels));
    numChannels = static_cast<int>(numChannels);

    file.seekg(24);
    file.read(reinterpret_cast<char*>(&sampleRate), sizeof(sampleRate));

    file.seekg(34);
    file.read(reinterpret_cast<char*>(&bitDepth), sizeof(bitDepth));
    bitDepth = static_cast<int>(bitDepth);

    file.seekg(44);

    int16_t sample;
    while (file.read(reinterpret_cast<char *>(&sample), sizeof(sample))) {
        float normalizedSample = sample / 32768.0f;
        leftChannelData.push_back(normalizedSample);
        if (numChannels == 2) {
            file.read(reinterpret_cast<char *>(&sample), sizeof(sample));
            normalizedSample = sample / 32768.0f;
            rightChannelData.push_back(normalizedSample);
        }
    }

    file.close();

    numSamples = leftChannelData.size();
}

void AudioFile::loadAIFF(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open AIFF file: " << filename << std::endl;
        return;
    }

    char chunkID[4];
    uint32_t chunkSize;

    // Read the FORM chunk
    file.read(chunkID, 4);
    if (std::strncmp(chunkID, "FORM", 4) != 0) {
        std::cerr << "Invalid AIFF file: " << filename << std::endl;
        return;
    }

    // Skip the next 4 bytes (file size) and read the AIFF type
    file.seekg(4, std::ios::cur);
    file.read(chunkID, 4);
    if (std::strncmp(chunkID, "AIFF", 4) != 0) {
        std::cerr << "Not a valid AIFF file: " << filename << std::endl;
        return;
    }

    bool foundCOMM = false;
    bool foundSSND = false;

    // Iterate through chunks until SSND is found
    std::cout << "reading ";
    while (file.read(chunkID, 4)) {
        std::cout << ".";
        file.read(reinterpret_cast<char *>(&chunkSize), 4);
        chunkSize = __builtin_bswap32(chunkSize); // Convert from big-endian

        if (std::strncmp(chunkID, "COMM", 4) == 0) {
            foundCOMM = true;

            uint16_t numChannels;
            file.read(reinterpret_cast<char *>(&numChannels), 2);
            this->numChannels = __builtin_bswap16(numChannels);

            uint32_t numFrames;
            file.read(reinterpret_cast<char *>(&numFrames), 4);
            numFrames = __builtin_bswap32(numFrames);

            uint16_t sampleSize;
            file.read(reinterpret_cast<char *>(&sampleSize), 2);
            this->bitDepth = __builtin_bswap16(sampleSize);

            char sampleRateBytes[10];
            file.read(sampleRateBytes, 10);

            // Convert 80-bit IEEE 754 floating-point to standard integer sample rate
            int exponent = ((sampleRateBytes[0] & 0x7F) << 8) | sampleRateBytes[1];
            int mantissa = ((sampleRateBytes[2] << 24) | (sampleRateBytes[3] << 16) |
                            (sampleRateBytes[4] << 8) | sampleRateBytes[5]);

            if (exponent == 0 && mantissa == 0) {
                this->sampleRate = 0;
            } else {
                exponent -= 16383;
                this->sampleRate = mantissa >> (29 - exponent);
            }

            numSamples = numFrames;

            // Skip any remaining bytes in the COMM chunk
            file.seekg(chunkSize - 18, std::ios::cur);
        } else if (std::strncmp(chunkID, "SSND", 4) == 0) {
            foundSSND = true;

            uint32_t offset, blockSize;
            file.read(reinterpret_cast<char *>(&offset), 4);
            file.read(reinterpret_cast<char *>(&blockSize), 4);

            // Skip offset
            file.seekg(offset, std::ios::cur);

            // Read the sound data
            int numSamplesToRead = chunkSize - 8;
            std::vector<int16_t> samples(numSamplesToRead / 2);
            file.read(reinterpret_cast<char *>(samples.data()), numSamplesToRead);

            for (size_t i = 0; i < samples.size(); i += numChannels) {
                samples[i] = __builtin_bswap16(samples[i]); // Convert from big-endian
                leftChannelData.push_back(samples[i] / 32768.0f);
                if (numChannels == 2 && i + 1 < samples.size()) {
                    samples[i + 1] = __builtin_bswap16(samples[i + 1]);
                    rightChannelData.push_back(samples[i + 1] / 32768.0f);
                }
            }

            break; // SSND chunk should be the last chunk we care about
        } else {
            // Skip unknown or unhandled chunks
            file.seekg(chunkSize, std::ios::cur);
        }
    }
    std::cout << "\n";
    if (!foundCOMM) {
        std::cerr << "COMM chunk not found in AIFF file: " << filename << std::endl;
    }

    if (!foundSSND) {
        std::cerr << "SSND chunk not found or empty in AIFF file: " << filename << std::endl;
    }

    if (leftChannelData.empty()) {
        std::cerr << "No audio data found in AIFF file: " << filename << std::endl;
    }

    file.close();
}

float AudioFile::getLeftSample(int index) const {
    if (index < 0 || index >= numSamples) return 0.0f;
    return leftChannelData[index];
}

float AudioFile::getRightSample(int index) const {
    if (numChannels == 1 || index < 0 || index >= numSamples) return 0.0f;
    return rightChannelData[index];
}

float AudioFile::getSample(int channel, int index) const {
    if(channel==0) {
        if (index < 0 || index >= numSamples) return 0.0f;
        return leftChannelData[index];
    } else if(channel==1) {
        if (numChannels == 1 || index < 0 || index >= numSamples) return 0.0f;
        return rightChannelData[index];
    }
    return 0.f;
}

int 
AudioFile::getSize() const
{
    return leftChannelData.size() + rightChannelData.size();
}