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
    AudioFile *audiofile;

    int buffer_count;

public:
    void
    Init()
    {
        Bind(filename, "filename");
        Bind(repeat, "repeat");  
        Bind(buffersize, "buffer_size");
        Bind(channels, "channels");
        Bind(output, "OUTPUT"); 

        audiofile = new AudioFile(filename.c_str()); 

        
    }

    ~InputAudioFile()
    {
        delete audiofile;
    }


    void
    Tick()
    {
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
