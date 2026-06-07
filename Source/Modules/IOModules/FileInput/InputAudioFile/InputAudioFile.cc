#include "ikaros.h"
#include "AudioFile.h"
// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

class InputAudioFile : public Module 
{
    parameter filename;
    parameter repeat;
    parameter buffersize;
    parameter channels;
    matrix output;
    AudioFile *audiofile = nullptr;
    std::filesystem::path resolved_filename;

    int buffer_count = 0;

    bool
    OpenAudioFile()
    {
        if(audiofile != nullptr)
            return true;

        if(!kernel().SanitizeReadPath(filename.as_string(), resolved_filename))
            throw exception("InputAudioFile can only read files from the project directory or UserData.", path_);

        filename = resolved_filename.string();
        audiofile = new AudioFile(resolved_filename.string());
        return true;
    }

public:
    void
    Init() override
    {
        Bind(filename, "filename");
        Bind(repeat, "repeat");  
        Bind(buffersize, "buffer_size");
        Bind(channels, "channels");
        Bind(output, "OUTPUT"); 

        OpenAudioFile();
    }

    ~InputAudioFile()
    {
        Stop();
    }

    void
    Stop() override
    {
        delete audiofile;
        audiofile = nullptr;
    }


    void
    Tick() override
    {
        if(!OpenAudioFile())
            return;

        // channels as rows
        for(int j=0; j< channels.as_int(); ++j)
            for(int i=0; i < buffersize.as_int(); ++i)
                output.data()[j*buffersize.as_int() + i] = audiofile->getSample(j, i);
        buffer_count++; 
        if(repeat && buffer_count == audiofile->getSize())
            buffer_count %= audiofile->getSize();
    }
};


// Add the installation of the module
INSTALL_CLASS(InputAudioFile);
