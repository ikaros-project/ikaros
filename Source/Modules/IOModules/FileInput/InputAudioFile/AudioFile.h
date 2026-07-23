#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>


class AudioFile
{
public:
    explicit AudioFile(const std::filesystem::path & path);

    double sampleRate() const noexcept { return sampleRate_; }
    int bitDepth() const noexcept { return bitDepth_; }
    std::size_t channelCount() const noexcept { return channels_.size(); }
    std::size_t frameCount() const noexcept;
    std::span<const float> channel(std::size_t index) const;

private:
    void loadWAV(std::span<const unsigned char> data);
    void loadAIFF(std::span<const unsigned char> data);

    std::filesystem::path path_;
    double sampleRate_ = 0;
    int bitDepth_ = 0;
    std::vector<std::vector<float>> channels_;
};
