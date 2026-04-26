#include "ikaros.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <string>

using namespace ikaros;

class OutputAudioFile: public Module
{
    parameter   filename;
    parameter   sample_rate;
    parameter   bits_per_sample;

    matrix      input;

    std::ofstream file;
    bool        file_initialized = false;
    bool        warned_buffer_size = false;
    bool        warned_missing_input = false;
    int         channel_count = 0;
    int         sample_bits = 16;
    int         audio_format = 1; // PCM int16 by default, 3 for float32
    uint32_t    data_bytes_written = 0;
    std::filesystem::path resolved_filename;

    double EffectiveSampleRate() const
    {
        double sr = sample_rate;
        return sr > 0.0 ? sr : 1.0 / GetTickDuration();
    }

    void WarnIfBufferGeometryIsFractional(double sr)
    {
        if(warned_buffer_size || sr <= 0.0)
            return;

        const double exact_samples = sr * GetTickDuration();
        const double rounded_samples = std::round(exact_samples);
        if(std::abs(exact_samples - rounded_samples) > 1e-6)
        {
            Warning("OutputAudioFile sample_rate * tick_duration is not an integer number of samples (" + std::to_string(exact_samples) + "). Audio buffers may not match timing exactly.");
            warned_buffer_size = true;
        }
    }

    template<typename T>
    void WriteValue(T value)
    {
        file.write(reinterpret_cast<const char *>(&value), sizeof(T));
    }

    void WriteHeaderPlaceholder(int channels, int sr, int bits, int format)
    {
        const uint32_t byte_rate = static_cast<uint32_t>(sr * channels * bits / 8);
        const uint16_t block_align = static_cast<uint16_t>(channels * bits / 8);
        const uint32_t riff_chunk_size = 36;
        const uint32_t data_chunk_size = 0;
        const uint32_t fmt_chunk_size = 16;

        file.write("RIFF", 4);
        WriteValue<uint32_t>(riff_chunk_size);
        file.write("WAVE", 4);

        file.write("fmt ", 4);
        WriteValue<uint32_t>(fmt_chunk_size);
        WriteValue<uint16_t>(static_cast<uint16_t>(format));
        WriteValue<uint16_t>(static_cast<uint16_t>(channels));
        WriteValue<uint32_t>(static_cast<uint32_t>(sr));
        WriteValue<uint32_t>(byte_rate);
        WriteValue<uint16_t>(block_align);
        WriteValue<uint16_t>(static_cast<uint16_t>(bits));

        file.write("data", 4);
        WriteValue<uint32_t>(data_chunk_size);
    }

    void FinalizeHeader()
    {
        if(!file.is_open())
            return;

        const uint32_t riff_chunk_size = 36 + data_bytes_written;

        file.seekp(4, std::ios::beg);
        WriteValue<uint32_t>(riff_chunk_size);

        file.seekp(40, std::ios::beg);
        WriteValue<uint32_t>(data_bytes_written);

        file.seekp(0, std::ios::end);
        file.flush();
    }

    bool InitializeFileFromInput()
    {
        if(file_initialized)
            return true;

        if(filename.empty())
        {
            Warning("OutputAudioFile requires a non-empty filename.");
            return false;
        }

        if(input.rank() == 0 || input.size() == 0)
        {
            if(!warned_missing_input)
            {
                Warning("OutputAudioFile has no connected audio input yet; delaying file creation until audio arrives.");
                warned_missing_input = true;
            }
            return false;
        }

        channel_count = input.rank() >= 2 ? input.size_y() : 1;
        sample_bits = bits_per_sample.as_int();
        if(sample_bits != 16 && sample_bits != 32)
        {
            Warning("OutputAudioFile only supports bits_per_sample 16 or 32. Falling back to 16-bit PCM.");
            sample_bits = 16;
        }

        audio_format = sample_bits == 32 ? 3 : 1;
        data_bytes_written = 0;

        if(!kernel().SanitizeWritePath(filename.as_string(), resolved_filename))
        {
            Warning("OutputAudioFile can only write files inside UserData.");
            return false;
        }

        file.open(resolved_filename, std::ios::binary | std::ios::trunc);
        if(!file.is_open())
        {
            Warning("OutputAudioFile could not open file \"" + resolved_filename.string() + "\" for writing.");
            return false;
        }

        WriteHeaderPlaceholder(channel_count, static_cast<int>(std::round(EffectiveSampleRate())), sample_bits, audio_format);
        file_initialized = true;
        return true;
    }

    void WriteSample(float sample)
    {
        if(sample_bits == 32)
        {
            WriteValue<float>(sample);
            data_bytes_written += sizeof(float);
        }
        else
        {
            const float clipped = std::clamp(sample, -1.0f, 1.0f);
            const int16_t pcm = static_cast<int16_t>(std::lrint(clipped * 32767.0f));
            WriteValue<int16_t>(pcm);
            data_bytes_written += sizeof(int16_t);
        }
    }

public:
    void Init()
    {
        Bind(filename, "filename");
        Bind(sample_rate, "sample_rate");
        Bind(bits_per_sample, "bits_per_sample");
        Bind(input, "INPUT");
    }

    void Tick()
    {
        const double sr = EffectiveSampleRate();
        WarnIfBufferGeometryIsFractional(sr);

        if(input.rank() == 0 || input.size() == 0)
            return;

        if(input.rank() > 2)
        {
            Warning("OutputAudioFile only supports rank-1 mono buffers or rank-2 channel-by-frame buffers.");
            return;
        }

        if(!InitializeFileFromInput())
            return;

        const int frames = input.rank() >= 1 ? input.size_x() : 0;
        const int channels = input.rank() >= 2 ? input.size_y() : 1;

        if(channels != channel_count)
        {
            Warning("OutputAudioFile input channel count changed after file creation. Keeping the original file channel layout.");
        }

        for(int i = 0; i < frames; ++i)
        {
            if(input.rank() >= 2)
            {
                const int writable_channels = std::min(channels, channel_count);
                for(int ch = 0; ch < writable_channels; ++ch)
                    WriteSample(input(ch, i));
                for(int ch = writable_channels; ch < channel_count; ++ch)
                    WriteSample(0.0f);
            }
            else
            {
                WriteSample(input(i));
            }
        }
    }

    ~OutputAudioFile()
    {
        FinalizeHeader();
        if(file.is_open())
            file.close();
    }
};

INSTALL_CLASS(OutputAudioFile)
