
#include "ikaros.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

using namespace ikaros;
namespace fs = std::filesystem;

static std::string
normalize_executable_name(const std::string & value)
{
    return fs::path(value).filename().string();
}


static std::string
ResolvePlaybackCommand(const std::string & requested_command)
{
    std::string normalized = normalize_executable_name(requested_command);
    if(normalized.empty() || normalized == "afplay")
        return "/usr/bin/afplay";
    if(normalized == "play")
        return "/usr/bin/play";
    throw exception("EpiSpeech only allows the playback commands \"/usr/bin/afplay\" and \"/usr/bin/play\".");
}


static std::string
ResolveSpeechCommand(const std::string & requested_command)
{
    std::string normalized = normalize_executable_name(requested_command);
    if(normalized.empty() || normalized == "say")
        return "/usr/bin/say";
    throw exception("EpiSpeech only allows the speech synthesis command \"/usr/bin/say\".");
}


static std::string
ResolveFFProbeCommand()
{
    constexpr const char * candidates[] = {
        "/usr/bin/ffprobe",
        "/opt/homebrew/bin/ffprobe",
        "/usr/local/bin/ffprobe"
    };

    for(const char * candidate : candidates)
        if(fs::exists(candidate))
            return candidate;

    return "";
}


static void
RunCommandOrThrow(const std::string & executable, char * const argv[], const std::string & error_context)
{
    pid_t pid = 0;
    int status = posix_spawn(&pid, executable.c_str(), nullptr, nullptr, argv, nullptr);
    if(status != 0)
        throw exception(error_context + ": " + std::string(std::strerror(status)));

    int wait_status = 0;
    if(waitpid(pid, &wait_status, 0) == -1)
        throw exception(error_context + ": could not wait for child process.");

    if(!WIFEXITED(wait_status) || WEXITSTATUS(wait_status) != 0)
        throw exception(error_context + ": command exited with status " + std::to_string(WEXITSTATUS(wait_status)) + ".");
}


static bool
RunCommandCaptureOutput(const std::string & executable, char * const argv[], std::string & output)
{
    int pipe_fds[2];
    if(pipe(pipe_fds) != 0)
        return false;

    int null_fd = open("/dev/null", O_WRONLY);
    if(null_fd == -1)
    {
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return false;
    }

    posix_spawn_file_actions_t file_actions;
    if(posix_spawn_file_actions_init(&file_actions) != 0)
    {
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        close(null_fd);
        return false;
    }

    bool configured =
        posix_spawn_file_actions_adddup2(&file_actions, pipe_fds[1], STDOUT_FILENO) == 0 &&
        posix_spawn_file_actions_adddup2(&file_actions, null_fd, STDERR_FILENO) == 0 &&
        posix_spawn_file_actions_addclose(&file_actions, pipe_fds[0]) == 0 &&
        posix_spawn_file_actions_addclose(&file_actions, pipe_fds[1]) == 0 &&
        posix_spawn_file_actions_addclose(&file_actions, null_fd) == 0;

    if(!configured)
    {
        posix_spawn_file_actions_destroy(&file_actions);
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        close(null_fd);
        return false;
    }

    pid_t pid = 0;
    int spawn_status = posix_spawn(&pid, executable.c_str(), &file_actions, nullptr, argv, nullptr);
    posix_spawn_file_actions_destroy(&file_actions);
    close(pipe_fds[1]);
    close(null_fd);

    if(spawn_status != 0)
    {
        close(pipe_fds[0]);
        return false;
    }

    output.clear();
    char buffer[4096];
    ssize_t bytes_read = 0;
    while((bytes_read = read(pipe_fds[0], buffer, sizeof(buffer))) > 0)
        output.append(buffer, static_cast<size_t>(bytes_read));
    close(pipe_fds[0]);

    int wait_status = 0;
    if(waitpid(pid, &wait_status, 0) == -1)
        return false;

    return WIFEXITED(wait_status) && WEXITSTATUS(wait_status) == 0;
}

static std::string
cache_key_for_text(const std::string & text, const std::string & voice, const std::string & speech_command)
{
    // FNV-1a provides a stable cache key across process restarts.
    uint64_t hash = 1469598103934665603ULL;
    std::string key_source = speech_command + "\n" + voice + "\n" + text;
    for(unsigned char c : key_source)
    {
        hash ^= c;
        hash *= 1099511628211ULL;
    }

    std::ostringstream s;
    s << std::hex << hash;
    return s.str();
}

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
    parameter  voice;
    parameter text;
    std::string playback_command;
    std::string synthesis_command;
    
    std::vector<SpeechSound> sound;

    int queued_sound;
    int current_sound;
    int last_sound;


    virtual ~EpiSpeech() {}

    SpeechSound       CreateSound(std::string text);

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
EpiSpeech::CreateSound(std::string text)
{
    std::cout << "EpiSpeech::CreateSound for \"" << text << "\"." << std::endl;
    fs::path cache_dir = fs::current_path() / "EpiSpeechCache";
    fs::create_directories(cache_dir);

    std::string key = cache_key_for_text(text, std::string(voice), synthesis_command);
    fs::path sound_path = cache_dir / (key + ".aiff");
    fs::path analysis_path = cache_dir / (key + ".rms");
    SpeechSound sound(sound_path.string());

    int err = 1;

    if(fs::exists(sound_path) && fs::exists(analysis_path))
    {
        std::ifstream analysis(analysis_path);
        float t, l, r;
        while(analysis >> t >> l >> r)
        {
            sound.time.push_back(t);
            sound.left.push_back(l);
            sound.right.push_back(r);
        }

        if(!sound.time.empty())
            return sound;
    }

    // Synthesize speech only when the cached audio file is missing.
    if(!fs::exists(sound_path))
    {
        std::string voice_name = std::string(voice);
        std::string output_path = sound_path.string();
        char * argv[] = {
            const_cast<char *>(synthesis_command.c_str()),
            const_cast<char *>("-v"),
            const_cast<char *>(voice_name.c_str()),
            const_cast<char *>("-o"),
            const_cast<char *>(output_path.c_str()),
            const_cast<char *>("--"),
            const_cast<char *>(text.c_str()),
            nullptr
        };
        RunCommandOrThrow(synthesis_command, argv, "EpiSpeech could not synthesize speech");
    }

    // Calculate volume  using ffprobe

    std::string ffprobe_command = ResolveFFProbeCommand();
    std::string ffprobe_input =
        "amovie=" + sound_path.string() +
        ",astats=metadata=1:reset=1";
    std::string ffprobe_entries =
        "frame=pkt_pts_time:frame_tags=lavfi.astats.Overall.RMS_level,lavfi.astats.1.RMS_level,lavfi.astats.2.RMS_level";
    float t, l, r;
    std::string ffprobe_output;
    if(!ffprobe_command.empty())
    {
        char * argv[] = {
            const_cast<char *>(ffprobe_command.c_str()),
            const_cast<char *>("-f"),
            const_cast<char *>("lavfi"),
            const_cast<char *>("-i"),
            const_cast<char *>(ffprobe_input.c_str()),
            const_cast<char *>("-show_entries"),
            const_cast<char *>(ffprobe_entries.c_str()),
            const_cast<char *>("-of"),
            const_cast<char *>("csv=p=0"),
            nullptr
        };
        if(RunCommandCaptureOutput(ffprobe_command, argv, ffprobe_output))
        {
            std::istringstream ffprobe_stream(ffprobe_output);
            std::string line;
            while(std::getline(ffprobe_stream, line))
            {
                if(std::sscanf(line.c_str(), "%f,%f,%f", &t, &l, &r) == 3)
                {
                    sound.time.push_back(t);
                    sound.left.push_back(l);
                    sound.right.push_back(r);
                }
            }
            err = 0;
        }
        else
        {
            err = 1;
        }
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

    if(!sound.time.empty())
    {
        std::ofstream analysis(analysis_path);
        for(size_t i = 0; i < sound.time.size(); ++i)
            analysis << sound.time[i] << ' ' << sound.left[i] << ' ' << sound.right[i] << '\n';
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
       Bind(voice, "voice");

    Bind(scale_volume, "scale_volume");
    Bind(text, "text");

    playback_command = ResolvePlaybackCommand(std::string(command));
    synthesis_command = ResolveSpeechCommand(info_.contains_non_null("speech_command") ? std::string(info_["speech_command"]) : "");

    for(auto t: split(std::string(text), ","))
        sound.push_back(CreateSound(trim(t)));
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
            sound[current_sound].Play(playback_command);
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
