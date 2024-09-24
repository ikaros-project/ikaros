
#include "ikaros.h"

#include <spawn.h>

using namespace ikaros;

class SpeechSound
{
    public:
        SpeechSound(const std::string & s) : sound_path(s) { timer = new Timer(); };

        void    Play(const std::string & command);
        bool    UpdateVolume(matrix rms, double lag=0);  // returns true if still playing

        std::string sound_path;
        std::vector<float> time;
        std::vector<float> left;
        std::vector<float> right;

        int pid;

        int frame;
        Timer * timer;
};



class EpiSpeech: public Module
{
public:
    matrix trig;
    matrix last_trig;
    matrix inhibition;

    matrix playing;
    matrix completed;
    matrix active;

    matrix attribute1;
    matrix attribute2;

    matrix rms;
    matrix volume;
    
    parameter   scale_volume;
    float   lag;

    parameter  command;
    parameter  speech_command;
    parameter text;
    
    std::vector<SpeechSound> sound;

    int queued_sound;
    int current_sound;
    int last_sound;


    virtual ~EpiSpeech() {}

    SpeechSound       CreateSound(std::string text, std::string path, int id=0);

    void 		Init();
    void 		Tick();
};



void
SpeechSound::Play(const std::string & command)
{
    timer->Restart();
    frame = 0;
    char * argv[5] = { (char *)command.c_str(), (char *)sound_path.c_str(), NULL };
    pid_t pid;

    int status = posix_spawn(&pid, command.c_str(), NULL, NULL, argv, NULL);
}

bool
SpeechSound::UpdateVolume(matrix rms, double lag)
{
    float rt = (timer->GetTime() - lag); // 0.001*
    while(frame < time.size() && time[frame] < rt)
        frame++;

    if(frame == time.size())
    {
        rms[0] = -50; // dB with no signal
        rms[1] = -50;
        return false;
    }

    rms[0] = left[frame];
    rms[1] = right[frame];
    return true;
}



SpeechSound
EpiSpeech::CreateSound(std::string text, std::string path, int id)
{
    std::cout << "EpiSpeech::CreateSound for \"" << text << "\"." << std::endl;
    std::string sound_path = path + std::to_string(id)+".aiff";
    SpeechSound sound(sound_path);

    int err = 0;

    // synthesize speech

    std::string com = std::string(speech_command) + u8" -o " + std::string(sound_path) +u8" \""+ text +u8"\"";
    system(com.c_str());

    // Calculate volume  using ffprobe

    char * command_line = create_formatted_string("ffprobe -f lavfi -i amovie=%s,astats=metadata=1:reset=1 -show_entries frame=pkt_pts_time:frame_tags=lavfi.astats.Overall.RMS_level,lavfi.astats.1.RMS_level,lavfi.astats.2.RMS_level -of csv=p=0 2>/dev/null", sound_path.c_str());
    float t, l, r;
    FILE * fp = popen(command_line, "r"); 
    if(fp != NULL)
    {
        while(fscanf(fp, "%f,%f,%f\n", &t, &l, &r) == 3)
        {
            sound.time.push_back(t);
            sound.left.push_back(l);
            sound.right.push_back(r);
        }
        err = pclose(fp);
    }
    if(err != 0) // Command failed - fake 1s output
    {
            sound.time.push_back(0);
            sound.left.push_back(-15);
            sound.right.push_back(-15);
            sound.time.push_back(1);
            sound.left.push_back(-15);
            sound.right.push_back(-15);
    }
    return sound;
}



void
EpiSpeech::Init()
{
   Bind(trig, "TRIG");
    //size = GetInputSize("TRIG");
    last_trig = matrix(trig.size());        // FIXME: Usse last() functionality later

    queued_sound = -1;
    current_sound = -1;
    last_sound = -1;

       Bind(inhibition, "INHIBITION");
       Bind(attribute1, "ATTRIBUTE1");
       Bind(attribute2, "ATTRIBUTE2");
       Bind(playing, "PLAYING");
       Bind(completed, "COMPLETED");
       Bind(active, "ACTIVE");

       Bind(rms, "RMS");
       Bind(volume, "VOLUME");
    
       Bind(command, "command");
       Bind(speech_command, "speech_command");

    Bind(scale_volume, "scale_volume");
    Bind(text, "text");

    int id = 0;
    for(auto t: split(std::string(text), ","))
        sound.push_back(CreateSound(trim(t), "", id++));    // FIXME: Use current path here *********
}


void
EpiSpeech::Tick()
{
    // Check for trig and queue sound

    for(int i=0; i<trig.size(); i++)
        if(trig(i) == 1 && last_trig(i) == 0 && i < sound.size())
        {
                queued_sound = i;
        }

    // Start play if sound in queue and no inhibition

        if(current_sound == -1 && queued_sound != -1 && (!inhibition .connected()|| inhibition == 0))   // FIXME: PROBABLY NOT WORKING
        {
            current_sound = queued_sound;
            queued_sound = -1;
            sound[current_sound].Play(std::string(command));
        }

    // Update volume

    rms[0] = -50; // dB with no signal
    rms[1] = -50;

    if(current_sound != -1)
        if(!sound[current_sound].UpdateVolume(rms, lag))
            current_sound = -1;

    volume[0] = scale_volume * pow(10, 0.1*rms[0]); // convert to linear volume scale
    volume[1] = scale_volume * pow(10, 0.1*rms[1]);

    // Set playing status outputs

    
    if(current_sound == -1)
    {
        playing.reset();
        active = 0;
    }
    else
    {
        playing.reset();
        playing(current_sound) = 1;
        active = 1;
    }

    // Set completed status

    if((current_sound != last_sound) && (last_sound != -1))
    {
        completed.reset();
        completed(last_sound) = 1;  // FIXME: ADD set_one(...) to matrix == one_hot
    }
    else
    {
        completed.reset();
    }



    // Store last trig and sound

    last_trig.copy(trig);       // FIXME: Use last()
    last_sound = current_sound;
}



INSTALL_CLASS(EpiSpeech)


