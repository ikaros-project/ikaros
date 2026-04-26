//
//	SoundOutput.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2013-2022 Christian Balkenius
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    See http://www.ikaros-project.org/ for more information.
//

#include "ikaros.h"

#include <fcntl.h>
#include <sstream>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace ikaros;
namespace
{
constexpr const char * kPlaybackCommand = "/usr/bin/afplay";

std::string ResolveFFProbeCommand()
{
    constexpr const char * candidates[] = {
        "/usr/bin/ffprobe",
        "/opt/homebrew/bin/ffprobe",
        "/usr/local/bin/ffprobe"
    };

    for(const char * candidate : candidates)
        if(std::filesystem::exists(candidate))
            return candidate;

    return "";
}

bool RunCommandCaptureOutput(const std::string & executable, char * const argv[], std::string & output)
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
}

class Sound
{
public:
    Sound(const std::string &s) : sound_path(s) { timer = new Timer(); };

    // void    Play(const char * command);
    // bool    UpdateVolume(float * rms, float lag=0);  // returns true if still playing

    std::string sound_path;
    std::vector<float> time;
    std::vector<float> left;
    std::vector<float> right;

    int pid;

    int frame;
    Timer *timer;
    void Play(const char *command)
    {
        timer->Restart();
        frame = 0;
        char *argv[5] = {(char *)command, (char *)sound_path.c_str(), NULL};
        pid_t pid;

        int status = posix_spawn(&pid, (char *)command, NULL, NULL, argv, NULL);
    }
    bool UpdateVolume(float *rms, float lag)
    {
        float rt = 0.001 * (timer->GetTime() - lag);
        while (frame < time.size() && time[frame] < rt)
            frame++;

        if (frame == time.size())
        {
            rms[0] = -50; // dB with no signal
            rms[1] = -50;
            return false;
        }

        rms[0] = left[frame];
        rms[1] = right[frame];
        return true;
    }
};

class SoundOutput : public Module
{

    matrix trig;
    matrix inhibition;
    matrix playing;
    matrix completed;
    matrix active;
    matrix rms;
    matrix volume;
    parameter sounds;
    matrix last_trig;

    parameter scale_volume;
    parameter lag;

    std::vector<Sound> sound;
    int queued_sound;
    int current_sound;
    int last_sound;

    void Init()
    {
        Bind(trig, "TRIG");
        queued_sound = -1;
        current_sound = -1;
        last_sound = -1;
        Bind(inhibition, "INHIBITION");
        Bind(playing, "PLAYING");
        Bind(completed, "COMPLETED");
        Bind(active, "ACTIVE");
        Bind(rms, "RMS");
        Bind(volume, "VOLUME");
        Bind(scale_volume, "scale_volume");
        Bind(lag, "lag");
        Bind(sounds, "sounds");

        for (auto sound_name : split(sounds, ","))
            sound.push_back(CreateSound(trim(sound_name)));
    }

    void Tick()
    {
        // Check for trig and queue sound
        auto last_trig = trig.last();
        for (int i = 0; i < trig.size(); i++)
            if (trig[i] == 1 && last_trig[i] == 0 && i < sound.size())
            {
                queued_sound = i;
            }

        // Start play if sound in queue and no inhibition

        if (current_sound == -1 && queued_sound != -1 && (!inhibition.connected() || inhibition(0) == 0))
        {
            current_sound = queued_sound;
            queued_sound = -1;
            sound[current_sound].Play(kPlaybackCommand);
        }

        // Update volume

        rms[0] = -50; // dB with no signal
        rms[1] = -50;

        if (current_sound != -1)
            if (!sound[current_sound].UpdateVolume(rms, lag))
                current_sound = -1;

        volume[0] = scale_volume * pow(10, 0.1 * rms[0]); // convert to linear volume scale
        volume[1] = scale_volume * pow(10, 0.1 * rms[1]);

        // Set playing status outputs

        if (current_sound == -1)
        {
            playing.set(0);
            active(0) = 0;
        }
        else
        {
            playing.set(0);
            if (0 <= current_sound && current_sound < trig.size())
                playing[current_sound] = 1;
            active(0) = 1;
        }

        // Set completed status

        if ((current_sound != last_sound) && (last_sound != -1))
        {
            completed.set(0);
            if (0 <= last_sound && last_sound < trig.size())
                completed[last_sound] = 1;
        }
        else
        {
            completed.set(0);
        }
    }

    Sound CreateSound(std::string sound_path)
    {
        Sound sound(sound_path);

        // Get amplitudes using ffprobe

        int err = 0;
        std::string ffprobe_command = ResolveFFProbeCommand();
        std::string ffprobe_input =
            "amovie=" + sound_path +
            ",astats=metadata=1:reset=1";
        std::string ffprobe_entries =
            "frame=pkt_pts_time:frame_tags=lavfi.astats.Overall.RMS_level,lavfi.astats.1.RMS_level,lavfi.astats.2.RMS_level";
        float t, l, r;
        if (!ffprobe_command.empty())
        {
            std::string ffprobe_output;
            char *argv[] = {
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
            if (RunCommandCaptureOutput(ffprobe_command, argv, ffprobe_output))
            {
                std::istringstream ffprobe_stream(ffprobe_output);
                std::string line;
                while (std::getline(ffprobe_stream, line))
                    if (std::sscanf(line.c_str(), "%f,%f,%f", &t, &l, &r) == 3)
                    {
                        sound.time.push_back(t);
                        sound.left.push_back(l);
                        sound.right.push_back(r);
                    }
                err = 0;
            }
            else
                err = 1;
        }
        if (err != 0) // Command failed - fake 1s output
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
};

INSTALL_CLASS(SoundOutput) // Install the class in the Ikaros kernel
