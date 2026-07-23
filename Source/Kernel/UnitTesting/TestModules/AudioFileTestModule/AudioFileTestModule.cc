#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "ikaros.h"
#include "Modules/IOModules/FileInput/InputAudioFile/AudioFile.h"


using namespace ikaros;


namespace
{
    using Bytes = std::vector<unsigned char>;

    constexpr std::array<std::int16_t, 12> pcmSamples
    {
        -32768, 32767,
        -16384, 16384,
        0, 0,
        16384, -16384,
        32767, -32768,
        8192, -8192,
    };

    constexpr std::array<float, 6> leftSamples
    {
        -1.0f, -0.5f, 0.0f, 0.5f, 32767.0f / 32768.0f, 0.25f,
    };

    constexpr std::array<float, 6> rightSamples
    {
        32767.0f / 32768.0f, 0.5f, 0.0f, -0.5f, -1.0f, -0.25f,
    };


    void
    require(bool condition, const std::string & message)
    {
        if(!condition)
            throw exception("AudioFileTestModule: " + message);
    }


    void
    requireNear(float actual, float expected, const std::string & message)
    {
        require(std::abs(actual - expected) <= 1e-6f,
                message + ": expected " + std::to_string(expected) +
                    ", got " + std::to_string(actual));
    }


    void
    AppendTag(Bytes & bytes, std::string_view tag)
    {
        require(tag.size() == 4, "fixture chunk tags must contain four bytes");
        bytes.insert(bytes.end(), tag.begin(), tag.end());
    }


    void
    AppendLittle16(Bytes & bytes, std::uint16_t value)
    {
        bytes.push_back(static_cast<unsigned char>(value));
        bytes.push_back(static_cast<unsigned char>(value >> 8));
    }


    void
    AppendLittle32(Bytes & bytes, std::uint32_t value)
    {
        bytes.push_back(static_cast<unsigned char>(value));
        bytes.push_back(static_cast<unsigned char>(value >> 8));
        bytes.push_back(static_cast<unsigned char>(value >> 16));
        bytes.push_back(static_cast<unsigned char>(value >> 24));
    }


    void
    AppendBig16(Bytes & bytes, std::uint16_t value)
    {
        bytes.push_back(static_cast<unsigned char>(value >> 8));
        bytes.push_back(static_cast<unsigned char>(value));
    }


    void
    AppendBig24(Bytes & bytes, std::int32_t value)
    {
        const std::uint32_t bits = static_cast<std::uint32_t>(value);
        bytes.push_back(static_cast<unsigned char>(bits >> 16));
        bytes.push_back(static_cast<unsigned char>(bits >> 8));
        bytes.push_back(static_cast<unsigned char>(bits));
    }


    void
    AppendBig32(Bytes & bytes, std::uint32_t value)
    {
        bytes.push_back(static_cast<unsigned char>(value >> 24));
        bytes.push_back(static_cast<unsigned char>(value >> 16));
        bytes.push_back(static_cast<unsigned char>(value >> 8));
        bytes.push_back(static_cast<unsigned char>(value));
    }


    void
    SetLittle32(Bytes & bytes, std::size_t offset, std::uint32_t value)
    {
        require(offset + 4 <= bytes.size(),
                "fixture little-endian patch is out of range");
        for(unsigned int byte = 0; byte < 4; ++byte)
            bytes[offset + byte] =
                static_cast<unsigned char>(value >> (8 * byte));
    }


    void
    SetBig32(Bytes & bytes, std::size_t offset, std::uint32_t value)
    {
        require(offset + 4 <= bytes.size(),
                "fixture big-endian patch is out of range");
        for(unsigned int byte = 0; byte < 4; ++byte)
            bytes[offset + byte] =
                static_cast<unsigned char>(value >> (8 * (3 - byte)));
    }


    std::size_t
    BeginLittleChunk(Bytes & bytes, std::string_view tag)
    {
        AppendTag(bytes, tag);
        AppendLittle32(bytes, 0);
        return bytes.size();
    }


    void
    EndLittleChunk(Bytes & bytes, std::size_t payloadOffset)
    {
        const std::size_t size = bytes.size() - payloadOffset;
        require(size <= std::numeric_limits<std::uint32_t>::max(),
                "fixture WAV chunk is too large");
        SetLittle32(bytes, payloadOffset - 4,
                    static_cast<std::uint32_t>(size));
        if((size & 1U) != 0)
            bytes.push_back(0);
    }


    std::size_t
    BeginBigChunk(Bytes & bytes, std::string_view tag)
    {
        AppendTag(bytes, tag);
        AppendBig32(bytes, 0);
        return bytes.size();
    }


    void
    EndBigChunk(Bytes & bytes, std::size_t payloadOffset)
    {
        const std::size_t size = bytes.size() - payloadOffset;
        require(size <= std::numeric_limits<std::uint32_t>::max(),
                "fixture AIFF chunk is too large");
        SetBig32(bytes, payloadOffset - 4,
                 static_cast<std::uint32_t>(size));
        if((size & 1U) != 0)
            bytes.push_back(0);
    }


    Bytes
    MakePCMWAV()
    {
        Bytes bytes;
        AppendTag(bytes, "RIFF");
        AppendLittle32(bytes, 0);
        AppendTag(bytes, "WAVE");

        std::size_t payload = BeginLittleChunk(bytes, "JUNK");
        bytes.push_back(0x42);
        EndLittleChunk(bytes, payload);

        payload = BeginLittleChunk(bytes, "data");
        for(std::size_t index = 0; index < 4; ++index)
            AppendLittle16(
                bytes,
                std::bit_cast<std::uint16_t>(pcmSamples[index]));
        EndLittleChunk(bytes, payload);

        payload = BeginLittleChunk(bytes, "LIST");
        bytes.insert(bytes.end(), {'I', 'N', 'F', 'O'});
        EndLittleChunk(bytes, payload);

        payload = BeginLittleChunk(bytes, "data");
        for(std::size_t index = 4; index < pcmSamples.size(); ++index)
            AppendLittle16(
                bytes,
                std::bit_cast<std::uint16_t>(pcmSamples[index]));
        EndLittleChunk(bytes, payload);

        payload = BeginLittleChunk(bytes, "fmt ");
        AppendLittle16(bytes, 1);
        AppendLittle16(bytes, 2);
        AppendLittle32(bytes, 48000);
        AppendLittle32(bytes, 48000 * 4);
        AppendLittle16(bytes, 4);
        AppendLittle16(bytes, 16);
        EndLittleChunk(bytes, payload);

        SetLittle32(bytes, 4,
                    static_cast<std::uint32_t>(bytes.size() - 8));
        return bytes;
    }


    Bytes
    MakeFloatWAV()
    {
        Bytes bytes;
        AppendTag(bytes, "RIFF");
        AppendLittle32(bytes, 0);
        AppendTag(bytes, "WAVE");

        std::size_t payload = BeginLittleChunk(bytes, "fmt ");
        AppendLittle16(bytes, 0xFFFE);
        AppendLittle16(bytes, 1);
        AppendLittle32(bytes, 32000);
        AppendLittle32(bytes, 32000 * 4);
        AppendLittle16(bytes, 4);
        AppendLittle16(bytes, 32);
        AppendLittle16(bytes, 22);
        AppendLittle16(bytes, 32);
        AppendLittle32(bytes, 0);
        AppendLittle32(bytes, 3);
        bytes.insert(bytes.end(), {
            0x00, 0x00, 0x10, 0x00, 0x80, 0x00,
            0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71,
        });
        EndLittleChunk(bytes, payload);

        payload = BeginLittleChunk(bytes, "fact");
        AppendLittle32(bytes, 3);
        EndLittleChunk(bytes, payload);

        payload = BeginLittleChunk(bytes, "data");
        for(float value : std::array{-1.0f, 0.25f, 1.5f})
            AppendLittle32(bytes, std::bit_cast<std::uint32_t>(value));
        EndLittleChunk(bytes, payload);

        SetLittle32(bytes, 4,
                    static_cast<std::uint32_t>(bytes.size() - 8));
        return bytes;
    }


    Bytes
    MakeAIFF()
    {
        Bytes bytes;
        AppendTag(bytes, "FORM");
        AppendBig32(bytes, 0);
        AppendTag(bytes, "AIFF");

        std::size_t payload = BeginBigChunk(bytes, "ANNO");
        bytes.insert(bytes.end(), {'o', 'd', 'd'});
        EndBigChunk(bytes, payload);

        payload = BeginBigChunk(bytes, "SSND");
        AppendBig32(bytes, 2);
        AppendBig32(bytes, 0);
        bytes.push_back(0);
        bytes.push_back(0);
        for(std::int32_t value :
            std::array{-8388608, -4194304, 0, 4194304, 8388607})
            AppendBig24(bytes, value);
        EndBigChunk(bytes, payload);

        payload = BeginBigChunk(bytes, "COMM");
        AppendBig16(bytes, 1);
        AppendBig32(bytes, 5);
        AppendBig16(bytes, 24);
        AppendBig16(bytes, 0x400E);
        AppendBig32(bytes, 0xAC440000);
        AppendBig32(bytes, 0);
        EndBigChunk(bytes, payload);

        SetBig32(bytes, 4,
                 static_cast<std::uint32_t>(bytes.size() - 8));
        return bytes;
    }


    Bytes
    MakeUnsupportedWAV()
    {
        Bytes bytes;
        AppendTag(bytes, "RIFF");
        AppendLittle32(bytes, 0);
        AppendTag(bytes, "WAVE");

        std::size_t payload = BeginLittleChunk(bytes, "fmt ");
        AppendLittle16(bytes, 6);
        AppendLittle16(bytes, 1);
        AppendLittle32(bytes, 8000);
        AppendLittle32(bytes, 8000);
        AppendLittle16(bytes, 1);
        AppendLittle16(bytes, 8);
        EndLittleChunk(bytes, payload);

        payload = BeginLittleChunk(bytes, "data");
        bytes.push_back(0);
        EndLittleChunk(bytes, payload);

        SetLittle32(bytes, 4,
                    static_cast<std::uint32_t>(bytes.size() - 8));
        return bytes;
    }


    Bytes
    MakeTruncatedWAV()
    {
        Bytes bytes;
        AppendTag(bytes, "RIFF");
        AppendLittle32(bytes, 100);
        AppendTag(bytes, "WAVE");
        return bytes;
    }


    std::filesystem::path
    ResolveFixturePath(const std::string & prefix, std::string_view suffix,
                       bool write)
    {
        const std::string filename = prefix + std::string(suffix);
        std::filesystem::path path;
        const bool resolved = write
            ? kernel().SanitizeWritePath(filename, path)
            : kernel().SanitizeReadPath(filename, path);
        require(resolved, "could not resolve fixture path " + filename);
        return path;
    }


    void
    WriteFile(const std::filesystem::path & path, const Bytes & bytes)
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        require(static_cast<bool>(file),
                "could not create fixture " + path.string());
        file.write(reinterpret_cast<const char *>(bytes.data()),
                   static_cast<std::streamsize>(bytes.size()));
        require(static_cast<bool>(file),
                "could not write fixture " + path.string());
    }


    void
    requireLoadFailure(const std::filesystem::path & path,
                       std::string_view expected)
    {
        try
        {
            const AudioFile unused(path);
            (void)unused;
        }
        catch(const std::runtime_error & error)
        {
            require(std::string(error.what()).find(expected) !=
                        std::string::npos,
                    "unexpected parser error: " + std::string(error.what()));
            return;
        }
        throw exception("AudioFileTestModule: invalid fixture was accepted");
    }
}


class AudioFileFixtureWriter: public Module
{
public:
    void
    Init() override
    {
        Bind(prefix_, "prefix");
        const std::string prefix = prefix_.as_string();
        fixtures_ = {
            {ResolveFixturePath(prefix, "_pcm.wav", true),
             MakePCMWAV()},
            {ResolveFixturePath(prefix, "_float.wav", true),
             MakeFloatWAV()},
            {ResolveFixturePath(prefix, "_audio.aiff", true),
             MakeAIFF()},
            {ResolveFixturePath(prefix, "_unsupported.wav", true),
             MakeUnsupportedWAV()},
            {ResolveFixturePath(prefix, "_truncated.wav", true),
             MakeTruncatedWAV()},
        };
        for(const auto & [path, bytes] : fixtures_)
            WriteFile(path, bytes);
    }


    void
    Stop() override
    {
        for(const auto & [path, bytes] : fixtures_)
        {
            (void)bytes;
            std::error_code error;
            std::filesystem::remove(path, error);
        }
        fixtures_.clear();
    }

private:
    parameter prefix_;
    std::vector<std::pair<std::filesystem::path, Bytes>> fixtures_;
};


class AudioFileTestModule: public Module
{
public:
    void
    Init() override
    {
        Bind(prefix_, "prefix");
        const std::string prefix = prefix_.as_string();

        const AudioFile pcm(
            ResolveFixturePath(prefix, "_pcm.wav", false));
        require(pcm.channelCount() == 2 && pcm.frameCount() == 6,
                "PCM WAV dimensions were decoded incorrectly");
        require(pcm.sampleRate() == 48000 && pcm.bitDepth() == 16,
                "PCM WAV metadata was decoded incorrectly");
        for(std::size_t frame = 0; frame < pcm.frameCount(); ++frame)
        {
            requireNear(pcm.channel(0)[frame], leftSamples[frame],
                        "PCM WAV left sample");
            requireNear(pcm.channel(1)[frame], rightSamples[frame],
                        "PCM WAV right sample");
        }

        bool channelRangeChecked = false;
        try
        {
            (void)pcm.channel(2);
        }
        catch(const std::out_of_range &)
        {
            channelRangeChecked = true;
        }
        require(channelRangeChecked,
                "AudioFile accepted an out-of-range channel");

        const AudioFile floating(
            ResolveFixturePath(prefix, "_float.wav", false));
        require(floating.channelCount() == 1 &&
                    floating.frameCount() == 3 &&
                    floating.sampleRate() == 32000 &&
                    floating.bitDepth() == 32,
                "floating-point WAV metadata was decoded incorrectly");
        requireNear(floating.channel(0)[0], -1.0f,
                    "floating WAV negative sample");
        requireNear(floating.channel(0)[1], 0.25f,
                    "floating WAV fractional sample");
        requireNear(floating.channel(0)[2], 1.5f,
                    "floating WAV sample above unity");

        const AudioFile aiff(
            ResolveFixturePath(prefix, "_audio.aiff", false));
        require(aiff.channelCount() == 1 && aiff.frameCount() == 5 &&
                    aiff.sampleRate() == 44100 && aiff.bitDepth() == 24,
                "AIFF metadata was decoded incorrectly");
        const std::array<float, 5> expectedAIFF
        {
            -1.0f, -0.5f, 0.0f, 0.5f, 8388607.0f / 8388608.0f,
        };
        for(std::size_t frame = 0; frame < expectedAIFF.size(); ++frame)
            requireNear(aiff.channel(0)[frame], expectedAIFF[frame],
                        "AIFF sample");

        requireLoadFailure(
            ResolveFixturePath(prefix, "_unsupported.wav", false),
            "unsupported WAV encoding");
        requireLoadFailure(
            ResolveFixturePath(prefix, "_truncated.wav", false),
            "container extends beyond");

        std::cout << "AUDIO PARSER TEST OK" << std::endl;
    }

private:
    parameter prefix_;
};


class AudioFileOutputVerifier: public Module
{
public:
    void
    Init() override
    {
        Bind(once_, "ONCE");
        Bind(repeat_, "REPEAT");
        require(once_.rank() == 2 && once_.size_y() == 2 &&
                    once_.size_x() == 4,
                "non-repeating InputAudioFile output has the wrong shape");
        require(repeat_.rank() == 2 && repeat_.size_y() == 2 &&
                    repeat_.size_x() == 4,
                "repeating InputAudioFile output has the wrong shape");
    }


    void
    Tick() override
    {
        static constexpr float maximum = 32767.0f / 32768.0f;
        static constexpr std::array<std::array<float, 8>, 3> expectedOnce
        {{
            {-1.0f, -0.5f, 0.0f, 0.5f,
             maximum, 0.5f, 0.0f, -0.5f},
            {maximum, 0.25f, 0.0f, 0.0f,
             -1.0f, -0.25f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 0.0f,
             0.0f, 0.0f, 0.0f, 0.0f},
        }};
        static constexpr std::array<std::array<float, 8>, 3> expectedRepeat
        {{
            {-1.0f, -0.5f, 0.0f, 0.5f,
             maximum, 0.5f, 0.0f, -0.5f},
            {maximum, 0.25f, -1.0f, -0.5f,
             -1.0f, -0.25f, maximum, 0.5f},
            {0.0f, 0.5f, maximum, 0.25f,
             0.0f, -0.5f, -1.0f, -0.25f},
        }};

        require(tick_ < expectedOnce.size(),
                "verifier received more ticks than expected");
        for(int channel = 0; channel < 2; ++channel)
            for(int frame = 0; frame < 4; ++frame)
            {
                const std::size_t expectedIndex =
                    static_cast<std::size_t>(channel * 4 + frame);
                requireNear(once_(channel, frame),
                            expectedOnce[tick_][expectedIndex],
                            "non-repeating audio block");
                requireNear(repeat_(channel, frame),
                            expectedRepeat[tick_][expectedIndex],
                            "repeating audio block");
            }

        ++tick_;
        if(tick_ == expectedOnce.size())
            std::cout << "INPUT AUDIO FILE TEST OK" << std::endl;
    }

private:
    matrix once_;
    matrix repeat_;
    std::size_t tick_ = 0;
};


INSTALL_CLASS(AudioFileFixtureWriter)
INSTALL_CLASS(AudioFileTestModule)
INSTALL_CLASS(AudioFileOutputVerifier)
