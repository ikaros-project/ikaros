#include "AudioFile.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>


namespace
{
    using Bytes = std::span<const unsigned char>;

    struct WAVFormat
    {
        std::uint16_t encoding = 0;
        std::uint16_t channels = 0;
        std::uint32_t sampleRate = 0;
        std::uint16_t blockAlign = 0;
        std::uint16_t bitsPerSample = 0;
    };


    [[noreturn]] void
    Fail(const std::filesystem::path & path, const std::string & message)
    {
        throw std::runtime_error("Could not read audio file \"" +
                                 path.string() + "\": " + message);
    }


    void
    Require(Bytes data, std::size_t offset, std::size_t size,
            const std::filesystem::path & path, std::string_view description)
    {
        if(offset > data.size() || size > data.size() - offset)
            Fail(path, "truncated " + std::string(description) + ".");
    }


    bool
    HasTag(Bytes data, std::size_t offset, std::string_view tag)
    {
        return offset <= data.size() && tag.size() <= data.size() - offset &&
               std::equal(tag.begin(), tag.end(), data.begin() + offset);
    }


    std::uint16_t
    ReadLittle16(Bytes data, std::size_t offset,
                 const std::filesystem::path & path,
                 std::string_view description)
    {
        Require(data, offset, 2, path, description);
        return static_cast<std::uint16_t>(data[offset]) |
               (static_cast<std::uint16_t>(data[offset + 1]) << 8);
    }


    std::uint32_t
    ReadLittle32(Bytes data, std::size_t offset,
                 const std::filesystem::path & path,
                 std::string_view description)
    {
        Require(data, offset, 4, path, description);
        return static_cast<std::uint32_t>(data[offset]) |
               (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
               (static_cast<std::uint32_t>(data[offset + 2]) << 16) |
               (static_cast<std::uint32_t>(data[offset + 3]) << 24);
    }


    std::uint64_t
    ReadLittle64(Bytes data, std::size_t offset,
                 const std::filesystem::path & path,
                 std::string_view description)
    {
        Require(data, offset, 8, path, description);
        std::uint64_t value = 0;
        for(unsigned int byte = 0; byte < 8; ++byte)
            value |= static_cast<std::uint64_t>(data[offset + byte])
                     << (8 * byte);
        return value;
    }


    std::uint16_t
    ReadBig16(Bytes data, std::size_t offset,
              const std::filesystem::path & path,
              std::string_view description)
    {
        Require(data, offset, 2, path, description);
        return (static_cast<std::uint16_t>(data[offset]) << 8) |
               static_cast<std::uint16_t>(data[offset + 1]);
    }


    std::uint32_t
    ReadBig32(Bytes data, std::size_t offset,
              const std::filesystem::path & path,
              std::string_view description)
    {
        Require(data, offset, 4, path, description);
        return (static_cast<std::uint32_t>(data[offset]) << 24) |
               (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
               (static_cast<std::uint32_t>(data[offset + 2]) << 8) |
               static_cast<std::uint32_t>(data[offset + 3]);
    }


    std::uint64_t
    ReadBig64(Bytes data, std::size_t offset,
              const std::filesystem::path & path,
              std::string_view description)
    {
        Require(data, offset, 8, path, description);
        std::uint64_t value = 0;
        for(unsigned int byte = 0; byte < 8; ++byte)
            value = (value << 8) | data[offset + byte];
        return value;
    }


    std::size_t
    CheckedContainerEnd(std::uint32_t declaredSize, std::size_t fileSize,
                        const std::filesystem::path & path,
                        std::string_view format)
    {
        const std::uint64_t end = 8ULL + declaredSize;
        if(end > fileSize)
            Fail(path, "the " + std::string(format) +
                           " container extends beyond the end of the file.");
        if(end < 12)
            Fail(path, "invalid " + std::string(format) + " container size.");
        return static_cast<std::size_t>(end);
    }


    std::size_t
    NextChunk(std::size_t dataOffset, std::uint32_t chunkSize,
              std::size_t containerEnd, const std::filesystem::path & path,
              std::string_view format)
    {
        const std::uint64_t unpaddedEnd =
            static_cast<std::uint64_t>(dataOffset) + chunkSize;
        const std::uint64_t paddedEnd = unpaddedEnd + (chunkSize & 1U);
        if(unpaddedEnd > containerEnd || paddedEnd > containerEnd)
            Fail(path, "a " + std::string(format) +
                           " chunk extends beyond its container.");
        return static_cast<std::size_t>(paddedEnd);
    }


    bool
    IsWaveSubtype(Bytes format, std::uint32_t subtype)
    {
        static constexpr unsigned char guidTail[] = {
            0x00, 0x00, 0x10, 0x00, 0x80, 0x00,
            0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71,
        };
        if(format.size() < 40)
            return false;
        const std::uint32_t formatSubtype =
            static_cast<std::uint32_t>(format[24]) |
            (static_cast<std::uint32_t>(format[25]) << 8) |
            (static_cast<std::uint32_t>(format[26]) << 16) |
            (static_cast<std::uint32_t>(format[27]) << 24);
        if(formatSubtype != subtype)
            return false;
        return std::equal(guidTail, guidTail + sizeof(guidTail),
                          format.begin() + 28);
    }


    WAVFormat
    ParseWAVFormat(Bytes format, const std::filesystem::path & path)
    {
        if(format.size() < 16)
            Fail(path, "the WAV fmt chunk is shorter than 16 bytes.");

        WAVFormat result;
        result.encoding = ReadLittle16(format, 0, path, "WAV encoding");
        result.channels = ReadLittle16(format, 2, path, "WAV channel count");
        result.sampleRate = ReadLittle32(format, 4, path, "WAV sample rate");
        const std::uint32_t byteRate =
            ReadLittle32(format, 8, path, "WAV byte rate");
        result.blockAlign =
            ReadLittle16(format, 12, path, "WAV block alignment");
        result.bitsPerSample =
            ReadLittle16(format, 14, path, "WAV bit depth");

        if(result.encoding == 0xFFFE)
        {
            const std::uint16_t extensionSize =
                ReadLittle16(format, 16, path, "WAV extension size");
            if(format.size() < 40 || extensionSize < 22 ||
               extensionSize > format.size() - 18)
                Fail(path, "invalid WAVE_FORMAT_EXTENSIBLE fmt chunk.");
            const std::uint16_t validBits =
                ReadLittle16(format, 18, path, "WAV valid bit depth");
            if(validBits == 0 || validBits > result.bitsPerSample)
                Fail(path, "invalid WAVE_FORMAT_EXTENSIBLE valid bit depth.");
            if(IsWaveSubtype(format, 1))
                result.encoding = 1;
            else if(IsWaveSubtype(format, 3))
                result.encoding = 3;
            else
                Fail(path, "unsupported WAVE_FORMAT_EXTENSIBLE subtype.");
        }

        if(result.encoding != 1 && result.encoding != 3)
            Fail(path, "unsupported WAV encoding " +
                           std::to_string(result.encoding) + ".");
        if(result.channels == 0 || result.channels > 64)
            Fail(path, "invalid WAV channel count " +
                           std::to_string(result.channels) + ".");
        if(result.sampleRate == 0)
            Fail(path, "the WAV sample rate is zero.");

        const bool validPCMDepth =
            result.encoding == 1 &&
            (result.bitsPerSample == 8 || result.bitsPerSample == 16 ||
             result.bitsPerSample == 24 || result.bitsPerSample == 32);
        const bool validFloatDepth =
            result.encoding == 3 &&
            (result.bitsPerSample == 32 || result.bitsPerSample == 64);
        if(!validPCMDepth && !validFloatDepth)
            Fail(path, "unsupported WAV bit depth " +
                           std::to_string(result.bitsPerSample) +
                           " for its encoding.");

        const std::uint64_t bytesPerSample = result.bitsPerSample / 8;
        const std::uint64_t expectedBlockAlign =
            static_cast<std::uint64_t>(result.channels) * bytesPerSample;
        if(result.blockAlign != expectedBlockAlign)
            Fail(path, "WAV block alignment does not match its channel count "
                       "and bit depth.");
        const std::uint64_t expectedByteRate =
            static_cast<std::uint64_t>(result.sampleRate) * result.blockAlign;
        if(byteRate != expectedByteRate)
            Fail(path, "WAV byte rate does not match its sample rate and block "
                       "alignment.");

        return result;
    }


    float
    DecodeWAVSample(Bytes data, std::size_t offset, const WAVFormat & format,
                    const std::filesystem::path & path)
    {
        if(format.encoding == 3)
        {
            if(format.bitsPerSample == 32)
            {
                const float value = std::bit_cast<float>(
                    ReadLittle32(data, offset, path, "WAV float sample"));
                if(!std::isfinite(value))
                    Fail(path, "WAV data contains a non-finite sample.");
                return value;
            }

            const double value = std::bit_cast<double>(
                ReadLittle64(data, offset, path, "WAV double sample"));
            if(!std::isfinite(value) ||
               value < -std::numeric_limits<float>::max() ||
               value > std::numeric_limits<float>::max())
                Fail(path, "WAV data contains an invalid floating-point "
                           "sample.");
            return static_cast<float>(value);
        }

        switch(format.bitsPerSample)
        {
            case 8:
                Require(data, offset, 1, path, "WAV 8-bit sample");
                return (static_cast<int>(data[offset]) - 128) / 128.0f;

            case 16:
            {
                const std::int16_t sample = std::bit_cast<std::int16_t>(
                    ReadLittle16(data, offset, path, "WAV 16-bit sample"));
                return sample / 32768.0f;
            }

            case 24:
            {
                Require(data, offset, 3, path, "WAV 24-bit sample");
                const std::uint32_t raw =
                    static_cast<std::uint32_t>(data[offset]) |
                    (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
                    (static_cast<std::uint32_t>(data[offset + 2]) << 16);
                const std::int32_t sample = (raw & 0x00800000U) != 0
                    ? static_cast<std::int32_t>(
                          static_cast<std::int64_t>(raw) - 0x01000000LL)
                    : static_cast<std::int32_t>(raw);
                return sample / 8388608.0f;
            }

            case 32:
            {
                const std::int32_t sample = std::bit_cast<std::int32_t>(
                    ReadLittle32(data, offset, path, "WAV 32-bit sample"));
                return static_cast<float>(
                    static_cast<double>(sample) / 2147483648.0);
            }
        }

        Fail(path, "internal error decoding a WAV sample.");
    }


    double
    ReadExtended80(Bytes data, std::size_t offset,
                   const std::filesystem::path & path)
    {
        const std::uint16_t signAndExponent =
            ReadBig16(data, offset, path, "AIFF sample rate");
        const std::uint64_t mantissa =
            ReadBig64(data, offset + 2, path, "AIFF sample rate");
        const bool negative = (signAndExponent & 0x8000U) != 0;
        const unsigned int exponent = signAndExponent & 0x7FFFU;

        if(negative || exponent == 0x7FFFU)
            Fail(path, "invalid AIFF sample rate.");
        if(exponent == 0 && mantissa == 0)
            return 0;

        const int unbiasedExponent =
            exponent == 0 ? 1 - 16383 : static_cast<int>(exponent) - 16383;
        const long double value =
            std::ldexp(static_cast<long double>(mantissa),
                       unbiasedExponent - 63);
        if(!std::isfinite(value) ||
           value > std::numeric_limits<double>::max())
            Fail(path, "AIFF sample rate is outside the supported range.");
        return static_cast<double>(value);
    }


    float
    DecodeAIFFSample(Bytes data, std::size_t offset, int bits,
                     const std::filesystem::path & path)
    {
        switch(bits)
        {
            case 8:
            {
                Require(data, offset, 1, path, "AIFF 8-bit sample");
                const std::int8_t sample =
                    std::bit_cast<std::int8_t>(data[offset]);
                return sample / 128.0f;
            }

            case 16:
            {
                const std::int16_t sample = std::bit_cast<std::int16_t>(
                    ReadBig16(data, offset, path, "AIFF 16-bit sample"));
                return sample / 32768.0f;
            }

            case 24:
            {
                Require(data, offset, 3, path, "AIFF 24-bit sample");
                const std::uint32_t raw =
                    (static_cast<std::uint32_t>(data[offset]) << 16) |
                    (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
                    static_cast<std::uint32_t>(data[offset + 2]);
                const std::int32_t sample = (raw & 0x00800000U) != 0
                    ? static_cast<std::int32_t>(
                          static_cast<std::int64_t>(raw) - 0x01000000LL)
                    : static_cast<std::int32_t>(raw);
                return sample / 8388608.0f;
            }

            case 32:
            {
                const std::int32_t sample = std::bit_cast<std::int32_t>(
                    ReadBig32(data, offset, path, "AIFF 32-bit sample"));
                return static_cast<float>(
                    static_cast<double>(sample) / 2147483648.0);
            }
        }

        Fail(path, "internal error decoding an AIFF sample.");
    }
}


AudioFile::AudioFile(const std::filesystem::path & path)
    : path_(path)
{
    std::ifstream file(path_, std::ios::binary | std::ios::ate);
    if(!file)
        Fail(path_, "the file could not be opened.");

    const std::streampos end = file.tellg();
    if(end < 0)
        Fail(path_, "the file size could not be determined.");
    const auto fileSize = static_cast<std::uintmax_t>(end);
    if(fileSize > std::numeric_limits<std::size_t>::max())
        Fail(path_, "the file is too large for this platform.");
    if(fileSize >
       static_cast<std::uintmax_t>(
           std::numeric_limits<std::streamsize>::max()))
        Fail(path_, "the file is too large to read.");

    std::vector<unsigned char> data(static_cast<std::size_t>(fileSize));
    file.seekg(0, std::ios::beg);
    if(!data.empty() &&
       !file.read(reinterpret_cast<char *>(data.data()),
                  static_cast<std::streamsize>(data.size())))
        Fail(path_, "the file could not be read completely.");

    const Bytes bytes(data);
    if(HasTag(bytes, 0, "RIFF") && HasTag(bytes, 8, "WAVE"))
        loadWAV(bytes);
    else if(HasTag(bytes, 0, "FORM") && HasTag(bytes, 8, "AIFF"))
        loadAIFF(bytes);
    else if(HasTag(bytes, 0, "FORM") && HasTag(bytes, 8, "AIFC"))
        Fail(path_, "AIFF-C containers are not supported.");
    else
        Fail(path_, "expected a RIFF/WAVE or FORM/AIFF container.");
}


std::size_t
AudioFile::frameCount() const noexcept
{
    return channels_.empty() ? 0 : channels_.front().size();
}


std::span<const float>
AudioFile::channel(std::size_t index) const
{
    if(index >= channels_.size())
        throw std::out_of_range("AudioFile channel index is out of range.");
    return channels_[index];
}


void
AudioFile::loadWAV(std::span<const unsigned char> data)
{
    Require(data, 0, 12, path_, "WAV header");
    const std::size_t containerEnd =
        CheckedContainerEnd(ReadLittle32(data, 4, path_, "RIFF size"),
                            data.size(), path_, "RIFF");

    Bytes formatChunk;
    std::vector<Bytes> dataChunks;
    std::size_t offset = 12;
    while(offset < containerEnd)
    {
        if(containerEnd - offset < 8)
            Fail(path_, "truncated WAV chunk header.");
        const std::uint32_t chunkSize =
            ReadLittle32(data, offset + 4, path_, "WAV chunk size");
        const std::size_t dataOffset = offset + 8;
        const std::size_t next =
            NextChunk(dataOffset, chunkSize, containerEnd, path_, "WAV");

        if(HasTag(data, offset, "fmt "))
        {
            if(!formatChunk.empty())
                Fail(path_, "multiple WAV fmt chunks are not supported.");
            formatChunk = data.subspan(dataOffset, chunkSize);
        }
        else if(HasTag(data, offset, "data"))
            dataChunks.push_back(data.subspan(dataOffset, chunkSize));

        offset = next;
    }

    if(formatChunk.empty())
        Fail(path_, "the WAV fmt chunk is missing.");
    if(dataChunks.empty())
        Fail(path_, "the WAV data chunk is missing.");

    const WAVFormat format = ParseWAVFormat(formatChunk, path_);
    std::size_t totalFrames = 0;
    for(Bytes chunk : dataChunks)
    {
        if(chunk.size() % format.blockAlign != 0)
            Fail(path_, "WAV data size is not a whole number of frames.");
        const std::size_t frames = chunk.size() / format.blockAlign;
        if(frames > std::numeric_limits<std::size_t>::max() - totalFrames)
            Fail(path_, "WAV frame count is too large.");
        totalFrames += frames;
    }
    if(totalFrames == 0)
        Fail(path_, "the WAV file contains no audio frames.");
    if(totalFrames >
       std::numeric_limits<std::size_t>::max() / format.channels)
        Fail(path_, "WAV sample count is too large.");

    channels_.assign(format.channels, std::vector<float>(totalFrames));
    const std::size_t bytesPerSample = format.bitsPerSample / 8;
    std::size_t destinationFrame = 0;
    for(Bytes chunk : dataChunks)
    {
        const std::size_t frames = chunk.size() / format.blockAlign;
        for(std::size_t frame = 0; frame < frames; ++frame)
            for(std::size_t channelIndex = 0;
                channelIndex < format.channels; ++channelIndex)
            {
                const std::size_t sampleOffset =
                    frame * format.blockAlign +
                    channelIndex * bytesPerSample;
                channels_[channelIndex][destinationFrame + frame] =
                    DecodeWAVSample(chunk, sampleOffset, format, path_);
            }
        destinationFrame += frames;
    }

    sampleRate_ = format.sampleRate;
    bitDepth_ = format.bitsPerSample;
}


void
AudioFile::loadAIFF(std::span<const unsigned char> data)
{
    Require(data, 0, 12, path_, "AIFF header");
    const std::size_t containerEnd =
        CheckedContainerEnd(ReadBig32(data, 4, path_, "FORM size"),
                            data.size(), path_, "FORM");

    Bytes commonChunk;
    Bytes soundChunk;
    std::size_t offset = 12;
    while(offset < containerEnd)
    {
        if(containerEnd - offset < 8)
            Fail(path_, "truncated AIFF chunk header.");
        const std::uint32_t chunkSize =
            ReadBig32(data, offset + 4, path_, "AIFF chunk size");
        const std::size_t dataOffset = offset + 8;
        const std::size_t next =
            NextChunk(dataOffset, chunkSize, containerEnd, path_, "AIFF");

        if(HasTag(data, offset, "COMM"))
        {
            if(!commonChunk.empty())
                Fail(path_, "multiple AIFF COMM chunks are not supported.");
            commonChunk = data.subspan(dataOffset, chunkSize);
        }
        else if(HasTag(data, offset, "SSND"))
        {
            if(!soundChunk.empty())
                Fail(path_, "multiple AIFF SSND chunks are not supported.");
            soundChunk = data.subspan(dataOffset, chunkSize);
        }

        offset = next;
    }

    if(commonChunk.size() < 18)
        Fail(path_, "the AIFF COMM chunk is missing or truncated.");
    if(soundChunk.size() < 8)
        Fail(path_, "the AIFF SSND chunk is missing or truncated.");

    const std::uint16_t channelCount =
        ReadBig16(commonChunk, 0, path_, "AIFF channel count");
    const std::uint32_t frameCount =
        ReadBig32(commonChunk, 2, path_, "AIFF frame count");
    const std::uint16_t bitsPerSample =
        ReadBig16(commonChunk, 6, path_, "AIFF bit depth");
    const double sampleRate = ReadExtended80(commonChunk, 8, path_);

    if(channelCount == 0 || channelCount > 64)
        Fail(path_, "invalid AIFF channel count " +
                       std::to_string(channelCount) + ".");
    if(frameCount == 0)
        Fail(path_, "the AIFF file contains no audio frames.");
    if(bitsPerSample != 8 && bitsPerSample != 16 &&
       bitsPerSample != 24 && bitsPerSample != 32)
        Fail(path_, "unsupported AIFF bit depth " +
                       std::to_string(bitsPerSample) + ".");
    if(!std::isfinite(sampleRate) || sampleRate <= 0)
        Fail(path_, "invalid AIFF sample rate.");

    const std::uint32_t soundOffset =
        ReadBig32(soundChunk, 0, path_, "AIFF sound offset");
    const std::size_t bytesPerSample = bitsPerSample / 8;
    const std::uint64_t bytesPerFrame =
        static_cast<std::uint64_t>(channelCount) * bytesPerSample;
    const std::uint64_t requiredBytes =
        static_cast<std::uint64_t>(frameCount) * bytesPerFrame;
    if(soundOffset > soundChunk.size() - 8 ||
       requiredBytes > soundChunk.size() - 8 - soundOffset)
        Fail(path_, "the AIFF SSND chunk is shorter than the declared frame "
                    "count.");
    if(static_cast<std::uint64_t>(frameCount) * channelCount >
       std::numeric_limits<std::size_t>::max())
        Fail(path_, "AIFF sample count is too large.");

    const Bytes samples =
        soundChunk.subspan(8 + soundOffset,
                           static_cast<std::size_t>(requiredBytes));
    channels_.assign(channelCount, std::vector<float>(frameCount));
    for(std::size_t frame = 0; frame < frameCount; ++frame)
        for(std::size_t channelIndex = 0;
            channelIndex < channelCount; ++channelIndex)
        {
            const std::size_t sampleOffset =
                (frame * channelCount + channelIndex) * bytesPerSample;
            channels_[channelIndex][frame] =
                DecodeAIFFSample(samples, sampleOffset, bitsPerSample, path_);
        }

    sampleRate_ = sampleRate;
    bitDepth_ = bitsPerSample;
}
