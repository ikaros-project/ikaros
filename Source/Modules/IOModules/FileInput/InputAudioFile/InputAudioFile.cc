#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string>

#include "ikaros.h"
#include "AudioFile.h"


using namespace ikaros;


class InputAudioFile: public Module
{
public:
    void
    Init() override
    {
        Bind(filename_, "filename");
        Bind(repeat_, "repeat");
        Bind(bufferSize_, "buffer_size");
        Bind(channels_, "channels");
        Bind(output_, "OUTPUT");

        bufferSizeValue_ = PositiveInteger(bufferSize_, "buffer_size");
        channelCountValue_ = PositiveInteger(channels_, "channels");
        ValidateOutput();

        const std::string configuredFilename = filename_.as_string();
        if(configuredFilename.empty())
            throw exception("InputAudioFile requires a non-empty filename.",
                            path_);
        if(!kernel().SanitizeReadPath(configuredFilename, resolvedFilename_))
            throw exception(
                "InputAudioFile could not resolve \"" + configuredFilename +
                    "\" inside the project directory or UserData.",
                path_);

        audioFile_ = std::make_unique<AudioFile>(resolvedFilename_);
        if(audioFile_->channelCount() !=
           static_cast<std::size_t>(channelCountValue_))
            throw exception(
                "InputAudioFile configured channels=" +
                    std::to_string(channelCountValue_) + ", but \"" +
                    configuredFilename + "\" contains " +
                    std::to_string(audioFile_->channelCount()) +
                    " channel(s).",
                path_);
    }


    void
    Tick() override
    {
        const std::size_t blockFrames =
            static_cast<std::size_t>(bufferSizeValue_);
        const std::size_t totalFrames = audioFile_->frameCount();
        float * output = output_.contiguous_data();
        std::size_t destinationFrame = 0;

        while(destinationFrame < blockFrames)
        {
            if(currentFrame_ == totalFrames)
            {
                if(!static_cast<bool>(repeat_))
                    break;
                currentFrame_ = 0;
            }

            const std::size_t copyCount =
                std::min(blockFrames - destinationFrame,
                         totalFrames - currentFrame_);
            for(std::size_t channelIndex = 0;
                channelIndex < audioFile_->channelCount(); ++channelIndex)
            {
                const std::span<const float> source =
                    audioFile_->channel(channelIndex);
                std::copy_n(
                    source.data() + currentFrame_, copyCount,
                    output + channelIndex * blockFrames + destinationFrame);
            }
            currentFrame_ += copyCount;
            destinationFrame += copyCount;
        }

        if(destinationFrame < blockFrames)
            for(std::size_t channelIndex = 0;
                channelIndex < audioFile_->channelCount(); ++channelIndex)
                std::fill(
                    output + channelIndex * blockFrames + destinationFrame,
                    output + (channelIndex + 1) * blockFrames, 0.0f);
    }

private:
    static int
    PositiveInteger(const parameter & value, const std::string & name)
    {
        const double number = value.as_double();
        if(!std::isfinite(number) || number < 1 ||
           number > std::numeric_limits<int>::max() ||
           number != std::trunc(number))
            throw exception("InputAudioFile parameter \"" + name +
                            "\" must be a positive integer.");
        return static_cast<int>(number);
    }


    void
    ValidateOutput() const
    {
        if(output_.rank() != 2 ||
           output_.size_y() != channelCountValue_ ||
           output_.size_x() != bufferSizeValue_)
            throw exception(
                "InputAudioFile OUTPUT must have shape [" +
                std::to_string(channelCountValue_) + ", " +
                std::to_string(bufferSizeValue_) + "].");
        if(!output_.is_contiguous())
            throw exception("InputAudioFile OUTPUT must be contiguous.");
    }


    parameter filename_;
    parameter repeat_;
    parameter bufferSize_;
    parameter channels_;
    matrix output_;

    std::unique_ptr<AudioFile> audioFile_;
    std::filesystem::path resolvedFilename_;
    std::size_t currentFrame_ = 0;
    int bufferSizeValue_ = 0;
    int channelCountValue_ = 0;
};


INSTALL_CLASS(InputAudioFile)
