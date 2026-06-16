//
//	SequenceRecorder.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2015-2026 Christian Balkenius
//
#include "ikaros.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <limits>
#include <random>

namespace fs = std::filesystem;

using namespace ikaros;

static constexpr int sequence_data_version = 2;
static constexpr const char *sequence_data_time_unit = "seconds";

static std::string
make_timestamp(double t)
{
    if (t <= 0)
        return "00:00:000";

    const int total_ms = std::max(0, int(std::round(t * 1000)));
    const int t_ms = total_ms % 1000;
    const int t_s = (total_ms / 1000) % 60;
    const int t_min = std::min(99, total_ms / 60000);

    constexpr size_t BUFF_SIZE = 12; // "MM:SS:mmm\0" needs 11 chars
    char buff[BUFF_SIZE];

    const int written = snprintf(buff, BUFF_SIZE, "%02d:%02d:%03d", t_min, t_s, t_ms);

    if (written < 0 || written >= BUFF_SIZE)
        return "ERROR";

    return std::string(buff);
}

static inline double
quantize(double time, double q)
{
    if (q <= 0)
        return time;
    double quantized = q * std::round(time / q);
    return std::round(quantized * 1000.0) / 1000.0;
}

float interpolate(float t, float t1, float t2, float p1, float p2) // linear interpolation
{
    float alpha = (t - t1) / (t2 - t1);
    return p1 + alpha * (p2 - p1);
}

template <typename KeypointList>
static int
find_index_for_time(KeypointList &keypoints, float t)
{
    int n = keypoints.size();

    // binary search for nearest keypoint
    int low = 0;
    int high = n - 1;
    int i = 0;
    while (low <= high)
    {

        i = (low + high) / 2;
        if ((float)(keypoints[i]["time"]) > t)
            high = i - 1;
        else if ((float)(keypoints[i]["time"]) < t)
            low = i + 1;
        else
        {
            low = i + 1;
            break;
        }
    }

    return low;
}

static void
set_one_hot(matrix & target, int index, int size)
{
    target.set(0);
    if (0 <= index && index < size)
        target[index] = 1;
}

class SequenceRecorder : public Module
{
public:
    struct MarkedRange
    {
        float start;
        float end;
        float epsilon;

        bool
        Contains(float time) const
        {
            return start - epsilon <= time && time <= end + epsilon;
        }

        bool
        Excludes(float time) const
        {
            return time < start - epsilon || time > end + epsilon;
        }
    };

    void
    StartRecord()
    {
        last_record_position = timer.GetTime();
        start_record = true;
    }

    void
    ResetPlaybackIndex()
    {
        playback_sequence = -1;
        playback_index = 0;
        playback_keypoint_count = -1;
        playback_time = -std::numeric_limits<float>::infinity();
    }

    void
    MarkSequenceChanged()
    {
        sequence_revision++;
    }

    bool
    SequenceCanPlay(int index)
    {
        if (!sequence_data["sequences"].is_list() || index < 0 || index >= sequence_data["sequences"].size())
            return false;

        auto &sequence = sequence_data["sequences"][index];
        return sequence["keypoints"].is_list() && sequence["keypoints"].size() > 0;
    }

    int
    RandomPlayableSequence(int exclude_index)
    {
        std::vector<int> candidates;
        int sequence_count = sequence_data["sequences"].is_list() ? int(sequence_data["sequences"].size()) : 0;
        int n = std::min(max_sequences.as_int(), sequence_count);

        for (int i = 0; i < n; i++)
            if (i != exclude_index && SequenceCanPlay(i))
                candidates.push_back(i);

        if (candidates.empty() && SequenceCanPlay(exclude_index))
            candidates.push_back(exclude_index);

        if (candidates.empty())
            return -1;

        static thread_local std::mt19937 generator(std::random_device{}());
        std::uniform_int_distribution<int> distribution(0, candidates.size() - 1);
        return candidates[distribution(generator)];
    }

    value &
    CurrentSequence()
    {
        return sequence_data["sequences"][current_sequence.as_int()];
    }

    MarkedRange
    CurrentMarkedRange()
    {
        auto &sequence = CurrentSequence();
        float start = float(sequence["start_mark_time"]);
        float end = float(sequence["end_mark_time"]);
        if (end < start)
            std::swap(start, end);

        return {start, end, float(GetTickDuration() / 2)};
    }

    double
    ClampedQuantizedTime(double time, double start_time, double end_time)
    {
        double t = quantize(time, GetTickDuration());

        if (t < start_time)
            t = start_time;
        if (t > end_time)
            t = end_time;

        return t;
    }

    void
    SyncPositionFromTime(double time, double end_time)
    {
        position = end_time > 0 ? time / end_time : 0;
        last_position = position;
    }

    void
    SetPausedTime(double time)
    {
        auto &sequence = CurrentSequence();
        double start_time = sequence["start_time"];
        double end_time = sequence["end_time"];
        double t = ClampedQuantizedTime(time, start_time, end_time);

        timer.Pause();
        timer.SetPauseTime(t);
        SyncPositionFromTime(t, end_time);
        ResetPlaybackIndex();
        RequestOutputSmoothing();
    }

    double
    ContinueFromTime(double time)
    {
        SetPausedTime(time);
        timer.Continue();
        return timer.GetTime();
    }

    bool
    TimeHasReachedEnd(double time, double end_time, double tick_duration)
    {
        if (end_time <= 0)
            return true;

        double epsilon = std::max(tick_duration, 0.001) * 0.5;
        return time >= end_time - epsilon;
    }

    void
    RequestOutputSmoothing()
    {
        smoothing_pending = true;
    }

    double
    ChannelRange(int c)
    {
        double r = std::abs(double(range_max[c]) - double(range_min[c]));
        return r > 0 ? r : 0;
    }

    float
    DesiredOutputForChannel(int c)
    {
        if (channel_mode(c, 0) == 1) // locked
            return output[c];
        if (channel_mode(c, 1) == 1) // play
            return target[c];
        if (channel_mode(c, 2) == 1 || channel_mode(c, 3) == 1) // record or copy
            return input[c];

        return output[c];
    }

    double
    ComputeSmoothingDuration()
    {
        double max_delta = 0;

        for (int c = 0; c < channels.as_int(); c++)
        {
            double range = ChannelRange(c);
            double delta = std::abs(double(DesiredOutputForChannel(c)) - double(output[c]));
            double normalized_delta = range > 0 ? delta / range : (delta > 0 ? 1 : 0);
            max_delta = std::max(max_delta, normalized_delta);
        }

        return std::max(0.0, double(smoothing_time)) * std::min(1.0, max_delta);
    }

    void
    StartPendingOutputSmoothing()
    {
        if (!smoothing_pending)
            return;

        smoothing_pending = false;
        smoothing_start.copy(output);
        smoothing_duration = ComputeSmoothingDuration();

        if (smoothing_duration <= 0)
        {
            smoothing_active = false;
            smoothing_alpha = 1;
            return;
        }

        smoothing_active = true;
        smoothing_alpha = 0;
        smoothing_start_clock = GetTime();
    }

    void
    UpdateOutputSmoothing()
    {
        if (!smoothing_active)
        {
            smoothing_alpha = 1;
            return;
        }

        smoothing_alpha = (GetTime() - smoothing_start_clock) / smoothing_duration;
        if (smoothing_alpha >= 1)
        {
            smoothing_alpha = 1;
            smoothing_active = false;
        }
        else if (smoothing_alpha < 0)
            smoothing_alpha = 0;
    }

    float
    SmoothedOutputForChannel(int c)
    {
        float desired = DesiredOutputForChannel(c);
        if (!smoothing_active)
            return desired;

        float start = smoothing_start[c];
        float end = desired;
        return start + smoothing_alpha * (end - start);
    }

    bool
    ChannelModeChanged()
    {
        if (last_channel_mode.size_x() != channel_mode.size_x() ||
            last_channel_mode.size_y() != channel_mode.size_y())
            return true;

        for (int c = 0; c < channels.as_int(); c++)
            for (int m = 0; m < modes; m++)
                if (channel_mode(c, m) != last_channel_mode(c, m))
                    return true;

        return false;
    }

    void
    Stop()
    {
        bool was_recoding = state[2] > 0;
        set_one_hot(state, 0, states);
        timer.Stop();
        if (was_recoding)
            LinkKeypoints(); // at end of recording
    }

    void
    Play()
    {
        set_one_hot(state, 1, states);
        timer.Continue();
    }

    void
    Record()
    {
        StartRecord();
        set_one_hot(state, 2, states);
    }

    void
    Pause()
    {
        bool was_recoding = state[2] > 0;
        set_one_hot(state, 3, states);
        timer.Pause();
        if (was_recoding)
            LinkKeypoints(); // at end of recording
    }

    void
    SkipStart()
    {
        set_one_hot(state, 3, states); // Pause
        auto &sequence = CurrentSequence();
        MarkedRange range = CurrentMarkedRange();
        double t = quantize(timer.GetTime(), GetTickDuration());
        if (t <= range.start + range.epsilon)
            SetPausedTime(sequence["start_time"]);
        else if (t <= range.end + range.epsilon)
            SetPausedTime(range.start);
        else
            SetPausedTime(range.end);
    }

    void
    SkipEnd()
    {
        set_one_hot(state, 3, states);
        auto &sequence = CurrentSequence();
        MarkedRange range = CurrentMarkedRange();
        double t = quantize(timer.GetTime(), GetTickDuration());
        if (t >= range.end - range.epsilon)
            SetPausedTime(sequence["end_time"]);
        else if (t >= range.start - range.epsilon)
            SetPausedTime(range.end);
        else
            SetPausedTime(range.start);
    }

    void
    SetStartMark()
    {
        double t = quantize(timer.GetTime(), GetTickDuration());
        auto &sequence = CurrentSequence();
        sequence["start_mark_time"] = t;
        if (float(sequence["end_mark_time"]) < t)
            sequence["end_mark_time"] = t;
    }

    void
    SetEndMark()
    {
        double t = quantize(timer.GetTime(), GetTickDuration());
        auto &sequence = CurrentSequence();
        sequence["end_mark_time"] = t;
        if (float(sequence["start_mark_time"]) > t)
            sequence["start_mark_time"] = t;
    }

    void
    SetMarkRange(float start_time, float end_time)
    {
        auto &sequence = CurrentSequence();
        sequence["start_mark_time"] = std::min(start_time, end_time);
        sequence["end_mark_time"] = std::max(start_time, end_time);
    }

    void
    Seek(float normalized_position)
    {
        if (!std::isfinite(normalized_position))
            return;

        auto &sequence = CurrentSequence();
        bool was_playing = state[1] > 0;
        float fraction = std::max(0.0f, std::min(1.0f, normalized_position));
        SetPausedTime(double(fraction) * double(sequence["end_time"]));

        if (was_playing)
        {
            set_one_hot(state, 1, states);
            timer.Continue();
        }
        else if (state[0] == 0 && state[2] == 0)
            set_one_hot(state, 3, states);
    }

    void
    GoToPreviousKeypoint()
    {
        double tick_duration = GetTickDuration();
        float t = quantize(timer.GetTime(), tick_duration);
        float time_epsilon = tick_duration / 2;
        auto &sequence = CurrentSequence();
        auto &keypoints = sequence["keypoints"];
        int i = find_index_for_time(keypoints, t);
        if (i > 0)
        {
            float kpt = keypoints[i - 1]["time"];
            if (std::abs(kpt - t) <= time_epsilon && i > 1)
                kpt = keypoints[i - 2]["time"];
            SetPausedTime(kpt);
        }
    }

    void
    GoToNextKeypoint()
    {
        double tick_duration = GetTickDuration();
        float t = quantize(timer.GetTime(), tick_duration);
        auto &sequence = CurrentSequence();
        auto &keypoints = sequence["keypoints"];
        int n = keypoints.size();
        int i = find_index_for_time(keypoints, t);
        if (i < n)
        {
            float kpt = keypoints[i]["time"];
            SetPausedTime(kpt);
        }
    }

    void
    GoToTime(double time)
    {
        SetTargetForTime(time);
    }

    void
    ExtendTime() // add one second to the end of the sequence
    {
        auto &sequence = CurrentSequence();
        float end_time = sequence["end_time"];
        sequence["end_time"] = 1.0f + int(end_time);
    }

    void
    ReduceTime()
    {
        auto &sequence = CurrentSequence();
        float end_time = sequence["end_time"];
        end_time = -1.0f + int(end_time + 0.99999);
        sequence["end_time"] = end_time > 0 ? end_time : 0;
    }

    void
    LockChannel(int c)
    {
        if (c < 0)
            return;
        if (c >= channels)
            return;
        if (internal_control[c])
            output[c] = positions[c];
        else
            output[c] = input[c]; // Make sure output is at the present servo position
    }

    void
    LinkKeypoints()
    {
        auto &keypoints = CurrentSequence()["keypoints"];
        int n = keypoints.size();

        left_output.copy(default_output);
        right_output.copy(default_output);

        std::vector<int> left_link(channels.as_int(), -1);
        std::vector<int> right_link(channels.as_int(), -1);

        // left to right sweep

        for (int i = 0; i < n; i++)
        {
            for (int c = 0; c < channels.as_int(); c++)
                if (!keypoints[i]["point"][c].is_null()) // channel has data from this keypoint
                {
                    left_link[c] = i;
                    right_output[c] = keypoints[i]["point"][c].as_float(); // candidate rightmost output
                }
            for (int c = 0; c < channels.as_int(); c++)
                keypoints[i]["link_left"][c] = left_link[c];
        }

        // right to left sweep

        for (int i = n - 1; i >= 0; i--)
        {
            for (int c = 0; c < channels.as_int(); c++)
                if (!keypoints[i]["point"][c].is_null()) // channel has data from this keypoint
                {
                    right_link[c] = i;
                    left_output[c] = keypoints[i]["point"][c].as_float(); // candidate leftmost output
                }
            for (int c = 0; c < channels.as_int(); c++)
                keypoints[i]["link_right"][c] = right_link[c];
        }
    }

    void
    DeleteEmptyKeypoints()
    {
        list keypoints = CurrentSequence()["keypoints"];
        auto it = keypoints.begin();
        while (it != keypoints.end())
        {
            int e = 0;
            for (int c = 0; c < channels.as_int(); c++)
                if (!(*it)["point"][c].is_null())
                    e++;

            if (e == 0)
            {
                it = keypoints.erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    template <typename KeypointList>
    int
    FindKeypointNearTime(KeypointList &keypoints, float time, float epsilon)
    {
        int n = keypoints.size();
        if (n < 1)
            return -1;

        int i = find_index_for_time(keypoints, time);
        int best_index = -1;
        float best_distance = epsilon;

        for (int candidate : {i - 1, i})
        {
            if (candidate < 0 || candidate >= n)
                continue;

            float distance = std::abs(float(keypoints[candidate]["time"]) - time);
            if (distance <= best_distance)
            {
                best_distance = distance;
                best_index = candidate;
            }
        }

        return best_index;
    }

    list
    CurrentChannelMode()
    {
        list cm = list();
        for (int c = 0; c < channels.as_int(); c++)
        {
            list modes = list();
            for (int m = 0; m < 4; m++)
                modes.push_back(channel_mode(c, m));
            cm.push_back(modes);
        }
        return cm;
    }

    void
    StoreChannelMode()
    {
        list cm = CurrentChannelMode();
        sequence_data["channel_mode"] = cm;
    }

    void
    LoadChannelMode()
    {
        try
        {
            for (int c = 0; c < channels.as_int(); c++)
                for (int m = 0; m < 4; m++)
                    channel_mode(c, m) = sequence_data["channel_mode"][c][m];
        }
        catch (const std::exception &e)
        {
            return; // ignore error
        }
    }

    void
    AddKeypoint(double time)
    {
        ResetPlaybackIndex();
        MarkSequenceChanged();
        list keypoints = CurrentSequence()["keypoints"];
        int n = keypoints.size();

        double qtime = quantize(time, GetTickDuration());
        float time_epsilon = GetTickDuration() / 2;

        // Create the point data array

        list points = list();
        for (int c = 0; c < channels.as_int(); c++)
            if (channel_mode[c](0) == 1)      // locked
                points.push_back(null());     // Do not record locked channel???
            else if (channel_mode(c, 1) == 1) // play - use null to indicate nodata
                points.push_back(null());
            else if (channel_mode(c, 2) == 1) // record - store current input (or sliders)
                points.push_back(input(c));
            else if (channel_mode(c, 3) == 1) // copy - do not record this channel but use null
                points.push_back(null());
            else // default
                points.push_back(null());

        dictionary keypoint;
        keypoint["time"] = qtime;
        keypoint["point"] = points;

        if (n == 0)
        {
            keypoints.push_back(keypoint);
            return;
        }

        double last_time = keypoints[n - 1]["time"];
        if (qtime >= last_time)
        {
            if (qtime - last_time <= time_epsilon)
            {
                for (int c = 0; c < channels.as_int(); c++)
                    if (!points[c].is_null())
                        keypoints[n - 1]["point"][c] = points[c];
            }
            else
                keypoints.push_back(keypoint);
            return;
        }

        int nearby_index = FindKeypointNearTime(keypoints, qtime, time_epsilon);
        if (nearby_index != -1)
        {
            for (int c = 0; c < channels.as_int(); c++)
                if (!points[c].is_null())
                    keypoints[nearby_index]["point"][c] = points[c];
            return;
        }

        int i = find_index_for_time(keypoints, qtime);

        // Insert in time order.

        if (i < n)
            keypoints.insert(keypoints.begin() + i, keypoint);
        else
            keypoints.push_back(keypoint);
    }

    void
    ClearSequence()
    {
        ResetPlaybackIndex();
        MarkSequenceChanged();
        auto &sequence = CurrentSequence();
        sequence["keypoints"] = list();
        sequence["start_time"] = 0;
        sequence["start_mark_time"] = 0;
        sequence["end_mark_time"] = 1;
        sequence["end_time"] = 1;
    }

    void
    Crop()
    {
        ResetPlaybackIndex();
        MarkSequenceChanged();
        auto &sequence = CurrentSequence();
        auto &keypoints = sequence["keypoints"];
        int n = keypoints.size();

        if (n < 1)
            return;
        MarkedRange range = CurrentMarkedRange();

        for (int i = 0; i < n; i++)
            if (range.Excludes(float(keypoints[i]["time"])))
                ClearKeypointAtIndex(i, true);

        DeleteEmptyKeypoints();

        // Retime keypoints

        n = keypoints.size();
        if (n < 1)
        {
            sequence["start_mark_time"] = 0;
            sequence["end_mark_time"] = 0;
            LinkKeypoints();
            return;
        }

        float start_time = keypoints[0]["time"];
        for (int i = 0; i < n; i++)
            keypoints[i]["time"] = float(keypoints[i]["time"]) - start_time;

        sequence["start_mark_time"] = 0;
        sequence["end_mark_time"] = float(sequence["end_mark_time"]) - start_time;

        LinkKeypoints();
    }

    void
    DeleteKeypoint(double time)
    {
        ResetPlaybackIndex();
        MarkSequenceChanged();
        auto &keypoints = CurrentSequence()["keypoints"];
        int n = keypoints.size();
        if (n < 1)
            return;

        int i = FindKeypointNearTime(keypoints, time, GetTickDuration() / 2);
        if (i != -1)
            ClearKeypointAtIndex(i);
    }

    void
    DeleteKeypoints()
    {
        ResetPlaybackIndex();
        MarkSequenceChanged();
        auto &keypoints = CurrentSequence()["keypoints"];
        MarkedRange range = CurrentMarkedRange();

        int n = keypoints.size();
        for (int i = 0; i < n; i++)
        {
            float t = keypoints[i]["time"];
            if (range.Contains(t))
            {
                for (int c = 0; c < channels.as_int(); c++)
                {
                    if (channel_mode(c, 2) == 1) // record mode
                    {
                        keypoints[i]["point"][c] = null();
                    }
                }
            }
        }

        // Clean up

        DeleteEmptyKeypoints();
        LinkKeypoints();
    }

    bool
    KeypointIsInMarkedRange(value &keypoints, int index, const MarkedRange &range)
    {
        float t = keypoints[index]["time"];
        return range.Contains(t);
    }

    void
    KeepDouglasPeuckerPoint(value &keypoints, const std::vector<int> &indices, std::vector<bool> &remove, int left, int right, int channel, float epsilon)
    {
        if (right - left < 2)
            return;

        int left_index = indices[left];
        int right_index = indices[right];
        float t_left = keypoints[left_index]["time"];
        float t_right = keypoints[right_index]["time"];
        float v_left = keypoints[left_index]["point"][channel];
        float v_right = keypoints[right_index]["point"][channel];

        float max_error = -1;
        int max_position = -1;

        for (int p = left + 1; p < right; p++)
        {
            if (!remove[p])
                continue;

            int index = indices[p];
            float t = keypoints[index]["time"];
            float actual = keypoints[index]["point"][channel];
            float expected = v_left;
            if (interpolation[channel] != 0 && t_right != t_left)
                expected = interpolate(t, t_left, t_right, v_left, v_right);
            float error = std::abs(actual - expected);

            if (error > max_error)
            {
                max_error = error;
                max_position = p;
            }
        }

        if (max_position == -1)
            return;

        if (max_error > epsilon)
        {
            remove[max_position] = false;
            KeepDouglasPeuckerPoint(keypoints, indices, remove, left, max_position, channel, epsilon);
            KeepDouglasPeuckerPoint(keypoints, indices, remove, max_position, right, channel, epsilon);
        }
    }

    void
    SimplifyChannel(int channel, const MarkedRange &range, float epsilon)
    {
        auto &keypoints = CurrentSequence()["keypoints"];
        int n = keypoints.size();

        std::vector<int> indices;
        indices.reserve(n);
        for (int i = 0; i < n; i++)
            if (!keypoints[i]["point"][channel].is_null())
                indices.push_back(i);

        int m = indices.size();
        if (m < 3)
            return;

        std::vector<bool> remove(m, false);

        int p = 0;
        while (p < m)
        {
            while (p < m && !KeypointIsInMarkedRange(keypoints, indices[p], range))
                p++;

            if (p >= m)
                break;

            int run_start = p;
            while (p < m && KeypointIsInMarkedRange(keypoints, indices[p], range))
                p++;
            int run_end = p - 1;

            int left = run_start > 0 ? run_start - 1 : run_start;
            int right = run_end + 1 < m ? run_end + 1 : run_end;

            if (right - left < 2)
                continue;

            for (int q = run_start; q <= run_end; q++)
                if (q != left && q != right)
                    remove[q] = true;

            KeepDouglasPeuckerPoint(keypoints, indices, remove, left, right, channel, epsilon);
        }

        for (int i = 0; i < m; i++)
            if (remove[i])
                keypoints[indices[i]]["point"][channel] = null();
    }

    void
    Simplify()
    {
        ResetPlaybackIndex();
        MarkSequenceChanged();
        MarkedRange range = CurrentMarkedRange();

        float epsilon = std::max(0.0f, simplify_epsilon.as_float());

        for (int c = 0; c < channels.as_int(); c++)
            if (channel_mode(c, 2) == 1)
                SimplifyChannel(c, range, epsilon);

        DeleteEmptyKeypoints();
        LinkKeypoints();
    }

    void
    SelectAll()
    {
        auto &sequence = CurrentSequence();
        sequence["start_mark_time"] = sequence["start_time"];
        sequence["end_mark_time"] = sequence["end_time"];
    }

    void
    ClearKeypointAtIndex(int i, bool all = false)
    {
        auto &keypoints = CurrentSequence()["keypoints"];
        int n = keypoints.size();
        if (i < 0 || i >= n)
            return;

        bool changed = false;
        for (int c = 0; c < channels.as_int(); c++)
        {
            if (channel_mode(c, 2) == 1 || all) // record mode or all-flag set
            {
                if (!keypoints[i]["point"][c].is_null())
                    changed = true;
                keypoints[i]["point"][c] = null();
            }
        }
        if (changed)
            MarkSequenceChanged();
    }

    void
    DeleteKeypointsInRange(float t0, float t1)
    {
        auto &keypoints = CurrentSequence()["keypoints"];
        int n = keypoints.size();

        if (n == 0 || t1 <= t0)
            return;

        if (t0 >= float(keypoints[n - 1]["time"]) || t1 <= float(keypoints[0]["time"]))
            return;

        int i0 = find_index_for_time(keypoints, t0);
        int i1 = find_index_for_time(keypoints, t1);

        if (i0 >= i1)
            return;

        for (int i = i0; i < i1; i++)
            ClearKeypointAtIndex(i);
    }

    void
    Trig(int id)
    {
        int m = max_sequences.as_int();
        if (id < 0 || id >= m)
        {
            Notify(msg_warning, "Sequence index is outside max_sequences: " + std::to_string(id));
            return;
        }

        if (!sequence_data["sequences"].is_list() || id >= sequence_data["sequences"].size())
        {
            Notify(msg_warning, "Sequence data is missing sequence " + std::to_string(id));
            return;
        }
        Stop();
        current_sequence = id;
        ResetPlaybackIndex();
        RequestOutputSmoothing();
        Play();
    }

    void
    Command(std::string command_name, dictionary &parameters)
    {
        std::string s = command_name;
        std::string value = parameters.contains("value") ? std::string(parameters["value"]) : "";

        float x = 0;
        float y = 0;
        bool has_y = false;

        if (parameters.contains("x"))
            x = parameters["x"];

        if (parameters.contains("y"))
        {
            y = parameters["y"];
            has_y = true;
        }

        std::cout << "COMMAND: " << s << std::endl;

        bool uses_current_sequence =
            s == "skip_start" || s == "skip_end" ||
            s == "set_start_mark" || s == "set_end_mark" ||
            s == "step_forward" || s == "step_backward" ||
            s == "extend_time" || s == "reduce_time" ||
            s == "add_keypoint" || s == "delete_keypoint" ||
            s == "crop" || s == "clear" || s == "delete" || s == "simplify" ||
            s == "lock" || s == "seek" || s == "set_mark_range" || s == "select_all" ||
            s == "rename" || s == "save" || s == "saveas";

        if (uses_current_sequence && !EnsureCurrentSequence())
            return;

        if (s == "stop")
            Stop();
        else if (s == "play")
            Play();
        else if (s == "record")
            Record();
        else if (s == "pause")
            Pause();
        else if (s == "skip_start")
            SkipStart();
        else if (s == "skip_end")
            SkipEnd();
        else if (s == "set_start_mark")
            SetStartMark();
        else if (s == "set_end_mark")
            SetEndMark();
        else if (s == "set_mark_range")
            SetMarkRange(x, y);
        else if (s == "seek")
            Seek(x);
        else if (s == "select_all")
            SelectAll();
        else if (s == "step_forward")
            GoToNextKeypoint();
        else if (s == "step_backward")
            GoToPreviousKeypoint();
        else if (s == "extend_time")
            ExtendTime();
        else if (s == "reduce_time")
            ReduceTime();
        else if (s == "add_keypoint")
        {
            AddKeypoint(timer.GetTime());
            LinkKeypoints();
        }
        else if (s == "delete_keypoint")
        {
            DeleteKeypoint(timer.GetTime());
            DeleteEmptyKeypoints();
            LinkKeypoints();
        }
        else if (s == "crop")
            Crop();
        else if (s == "clear")
            ClearSequence();
        else if (s == "delete")
            DeleteKeypoints();
        else if (s == "simplify")
            Simplify();
        else if (s == "lock")
            LockChannel(y);
        else if (s == "trig")
        {
            if (x < 0 || (has_y && (y < 0 || x >= layout_width.as_int())))
            {
                Notify(msg_warning, "Sequence grid coordinate is out of range.");
                return;
            }

            int id = int(x);
            if (has_y)
                id += layout_width.as_int() * int(y);
            Trig(id);
        }
        else if (s == "rename")
            Rename(value);

        else if (s == "new")
            New();
        else if (s == "open")
            Open(value);
        else if (s == "save")
            Save(filename);
        else if (s == "saveas")
            Save(value);
    }

    std::string json(const std::string &name)
    {
        if (name == "RANGES")
            return sequence_data["ranges"].json();

        else if (name == "SEQUENCE")
        {
            if (!EnsureCurrentSequence())
                return "{}";

            return CreateWebUISequence().json();
        }

        else if (name == "SEQUENCE_STATE")
        {
            if (!EnsureCurrentSequence())
                return "{}";

            return CreateWebUISequenceState().json();
        }

        else
            return "";
    }

    void
    SetTargetForTime(float t)
    {
        auto &keypoints = CurrentSequence()["keypoints"];
        int n = keypoints.size();
        int sequence_index = current_sequence.as_int();
        int i = 0;

        if (playback_sequence == sequence_index &&
            playback_keypoint_count == n &&
            playback_index >= 0 &&
            playback_index <= n &&
            t >= playback_time)
        {
            i = playback_index;
            while (i < n && float(keypoints[i]["time"]) <= t)
                i++;
        }
        else
            i = find_index_for_time(keypoints, t);

        playback_sequence = sequence_index;
        playback_index = i;
        playback_keypoint_count = n;
        playback_time = t;

        // Check if no keypoints: use default output as target

        if (n == 0)
        {
            for (int c = 0; c < channels.as_int(); c++)
                target[c] = default_output[c];
            return;
        }

        // Check if index is zero OR there is only one keypoint: use first keypoint

        if (i == 0 || n == 1)
        {
            for (int c = 0; c < channels.as_int(); c++)
                if (keypoints[0]["point"][c].is_null())
                    target[c] = left_output[c];
                else
                    target[c] = keypoints[0]["point"][c].as_float();

            return;
        }

        // Check if we are at or after the last keypoint: use last keypoint

        if (i > n - 1)
        {
            for (int c = 0; c < channels.as_int(); c++)
                if (keypoints[n - 1]["point"][c].is_null())
                    target[c] = right_output[c];
                else
                    target[c] = keypoints[n - 1]["point"][c].as_float();

            return;
        }

        // Do normal interpolation

        for (int c = 0; c < channels.as_int(); c++)
        {
            // Process left point

            auto &kp_left = keypoints[i - 1];
            double time_left = keypoints[i - 1]["time"];
            double point_left = left_output[c];

            if (!kp_left["point"][c].is_null()) // keypoint has data
            {
                point_left = kp_left["point"][c];
            }
            else if (!kp_left["link_left"][c].is_null()) // use linked keypoint if it exists
            {
                int l = kp_left["link_left"][c];
                if (l != -1 && !keypoints[l]["point"][c].is_null()) // check that linked keypoint has data - as should be the case
                {
                    point_left = keypoints[l]["point"][c];
                    time_left = keypoints[l]["time"];
                }
            }

            auto &kp_right = keypoints[i];
            double time_right = keypoints[i]["time"];
            double point_right = right_output[c];

            if (!kp_right["point"][c].is_null()) // keypoint has data
            {
                point_right = kp_right["point"][c];
            }
            else if (!kp_right["link_right"][c].is_null()) // use linked keypoint if it exists
            {
                int l = kp_right["link_right"][c];
                if (l != -1 && !keypoints[l]["point"][c].is_null()) // check that linked keypoint has data - as should be the case
                {
                    point_right = keypoints[l]["point"][c];
                    time_right = keypoints[l]["time"];
                }
            }

            if (interpolation[c] == 0)
                target[c] = point_left;
            else // 1 = linear interpolation
                target[c] = interpolate(t, time_left, time_right, point_left, point_right);
        }
    }

    void
    SetOutputForChannel(int c)
    {
        if (channel_mode(c, 0) == 1) // locked
        {
            // Do not change output
            active(c) = 1;
        }

        else if (channel_mode(c, 1) == 1) // play
        {
            output(c) = SmoothedOutputForChannel(c);
            positions(c) = output(c);
            active(c) = 1;
        }

        else if (channel_mode(c, 2) == 1) // record
        {
            output(c) = SmoothedOutputForChannel(c);
            active(c) = 0;
            if (internal_control(c) == 1)
                active(c) = 1;
        }

        else if (channel_mode(c, 3) == 1) // copy
        {
            output(c) = SmoothedOutputForChannel(c);
            active(c) = 0;
            if (internal_control(c) == 1)
                active(c) = 1;
        }
    }

    void
    Rename(const std::string &new_name)
    {
        CurrentSequence()["name"] = new_name;
        MarkSequenceChanged();
        UpdateSequenceNames();
    }

    void
    UpdateSequenceNames()
    {
        sequence_names = "";
        std::string sep = "";

        for (auto s : sequence_data["sequences"])
        {
            std::string name = s["name"];
            sequence_names = std::string(sequence_names) + sep + name;
            sep = ",";
        }
    }

    static dictionary create_sequence(int index, int layout_width)
    {
        dictionary sq;

        sq["name"] = "Sequence " + std::string(1, 65 + index / layout_width) + std::to_string(1 + index % layout_width);
        sq["start_time"] = 0;
        sq["start_mark_time"] = 0;
        sq["end_mark_time"] = 1;
        sq["end_time"] = 1;
        sq["keypoints"] = list();

        return sq;
    }

    list
    CreateDefaultRanges()
    {
        list ranges;
        for (int c = 0; c < channels.as_int(); c++)
        {
            list range;
            range.push_back(range_min(c));
            range.push_back(range_max(c));
            ranges.push_back(range);
        }
        return ranges;
    }

    void
    FillMissingSequenceFields(value &sequence, int index)
    {
        dictionary defaults = create_sequence(index, layout_width.as_int());

        if (!sequence.is_dictionary())
        {
            sequence = defaults;
            return;
        }

        if (sequence["name"].is_null())
            sequence["name"] = defaults["name"];
        if (sequence["start_time"].is_null())
            sequence["start_time"] = defaults["start_time"];
        if (sequence["start_mark_time"].is_null())
            sequence["start_mark_time"] = defaults["start_mark_time"];
        if (sequence["end_mark_time"].is_null())
            sequence["end_mark_time"] = defaults["end_mark_time"];
        if (sequence["end_time"].is_null())
            sequence["end_time"] = defaults["end_time"];
        if (sequence["keypoints"].is_null())
            sequence["keypoints"] = defaults["keypoints"];
    }

    bool
    ValidateLoadedKeypoints(value &sequence)
    {
        if (!sequence["keypoints"].is_list())
        {
            Notify(msg_warning, "Sequence file has invalid keypoint data. Cannot be opened.");
            return false;
        }

        double previous_time = -std::numeric_limits<double>::infinity();
        for (int i = 0; i < sequence["keypoints"].size(); i++)
        {
            value &keypoint = sequence["keypoints"][i];
            if (!keypoint.is_dictionary() || !keypoint["time"].is_number() ||
                !keypoint["point"].is_list() || keypoint["point"].size() < channels.as_int())
            {
                Notify(msg_warning, "Sequence file has invalid keypoint data. Cannot be opened.");
                return false;
            }

            for (int c = 0; c < channels.as_int(); c++)
            {
                if (!keypoint["point"][c].is_null() && !keypoint["point"][c].is_number())
                {
                    Notify(msg_warning, "Sequence file has invalid keypoint data. Cannot be opened.");
                    return false;
                }
            }

            double time = quantize(double(keypoint["time"]), GetTickDuration());
            if (time <= previous_time)
            {
                Notify(msg_warning, "Sequence file has unordered keypoints. Cannot be opened.");
                return false;
            }
            previous_time = time;
            keypoint["time"] = time;
        }

        return true;
    }

    bool
    ConvertLoadedSequenceTimes(dictionary &data, double scale)
    {
        if (!data["sequences"].is_list())
            return true;

        for (int i = 0; i < data["sequences"].size(); i++)
        {
            value &sequence = data["sequences"][i];
            if (!sequence.is_dictionary())
                continue;

            for (const char *field : {"start_time", "start_mark_time", "end_mark_time", "end_time"})
                if (!sequence[field].is_null())
                {
                    if (!sequence[field].is_number())
                    {
                        Notify(msg_warning, "Sequence file has invalid time data. Cannot be opened.");
                        return false;
                    }
                    sequence[field] = sequence[field].as_float() * scale;
                }

            if (sequence["keypoints"].is_null())
                continue;

            if (!sequence["keypoints"].is_list())
            {
                Notify(msg_warning, "Sequence file has invalid keypoint data. Cannot be opened.");
                return false;
            }

            for (int k = 0; k < sequence["keypoints"].size(); k++)
            {
                value &keypoint = sequence["keypoints"][k];
                if (!keypoint.is_dictionary() || !keypoint["time"].is_number())
                {
                    Notify(msg_warning, "Sequence file has invalid keypoint data. Cannot be opened.");
                    return false;
                }
                keypoint["time"] = keypoint["time"].as_float() * scale;
            }
        }

        return true;
    }

    bool
    NormalizeLoadedSequenceHeader(dictionary &data)
    {
        int version = 1;
        std::string time_unit = "milliseconds";

        if (!data["version"].is_null())
        {
            if (!data["version"].is_number())
            {
                Notify(msg_warning, "Sequence file has unsupported version. Cannot be opened.");
                return false;
            }
            version = data["version"].as_int();
        }

        if (!data["time_unit"].is_null())
        {
            if (!data["time_unit"].is_string())
            {
                Notify(msg_warning, "Sequence file has unsupported time unit. Cannot be opened.");
                return false;
            }
            time_unit = data["time_unit"].as_string();
        }

        if (version == 1 && time_unit == "milliseconds")
        {
            if (!ConvertLoadedSequenceTimes(data, 0.001))
                return false;
        }
        else if (!(version == sequence_data_version && time_unit == sequence_data_time_unit))
        {
            Notify(msg_warning, "Sequence file has unsupported version. Cannot be opened.");
            return false;
        }

        data["version"] = sequence_data_version;
        data["time_unit"] = sequence_data_time_unit;

        return true;
    }

    bool
    EnsureCurrentSequence()
    {
        if (!sequence_data["sequences"].is_list() || sequence_data["sequences"].size() == 0)
        {
            Notify(msg_warning, "Sequence data has no sequences.");
            return false;
        }

        int index = current_sequence.as_int();
        if (0 <= index && index < sequence_data["sequences"].size() && index < max_sequences.as_int())
            return true;

        Notify(msg_warning, "Current sequence is out of range. Resetting to sequence 0.");
        current_sequence = 0;
        Stop();
        return true;
    }

    bool
    NormalizeLoadedSequenceData(dictionary &data)
    {
        if (data["sequences"].is_null() || !data["sequences"].is_list())
        {
            Notify(msg_warning, "Sequence file has no sequence list. Cannot be opened.");
            return false;
        }

        int expected_sequences = max_sequences.as_int();
        int loaded_sequences = data["sequences"].size();

        if (loaded_sequences > expected_sequences)
        {
            Notify(msg_warning, "Sequence file has more sequences than max_sequences. Cannot be opened.");
            return false;
        }

        for (int i = 0; i < loaded_sequences; i++)
        {
            FillMissingSequenceFields(data["sequences"][i], i);
            if (!ValidateLoadedKeypoints(data["sequences"][i]))
                return false;
        }

        while (data["sequences"].size() < expected_sequences)
        {
            int index = data["sequences"].size();
            data["sequences"].push_back(create_sequence(index, layout_width.as_int()));
        }

        if (data["ranges"].is_null())
            data["ranges"] = CreateDefaultRanges();
        else if (!data["ranges"].is_list() || data["ranges"].size() != channels.as_int())
        {
            Notify(msg_warning, "Sequence file has wrong number of ranges. Cannot be opened.");
            return false;
        }
        else
        {
            for (int c = 0; c < channels.as_int(); c++)
            {
                if (!data["ranges"][c].is_list() || data["ranges"][c].size() != 2 ||
                    !data["ranges"][c][0].is_number() || !data["ranges"][c][1].is_number())
                {
                    Notify(msg_warning, "Sequence file has invalid range data. Cannot be opened.");
                    return false;
                }
            }
        }

        return true;
    }

    dictionary
    CreateWebUISequence()
    {
        dictionary sq;
        int index = current_sequence.as_int();
        value &source = sequence_data["sequences"][index];

        sq["revision"] = sequence_revision;
        sq["current_sequence"] = index;
        sq["name"] = source["name"];

        list keypoints;
        for (int i = 0; i < source["keypoints"].size(); i++)
        {
            list keypoint;
            keypoint.push_back(source["keypoints"][i]["time"]);
            for (int c = 0; c < channels.as_int(); c++)
                keypoint.push_back(source["keypoints"][i]["point"][c]);
            keypoints.push_back(keypoint);
        }
        sq["keypoints"] = keypoints;

        return sq;
    }

    dictionary
    CreateWebUISequenceState()
    {
        dictionary state;
        int index = current_sequence.as_int();
        value &sequence = sequence_data["sequences"][index];

        state["revision"] = sequence_revision;
        state["current_sequence"] = index;
        state["start_time"] = sequence["start_time"];
        state["start_mark_time"] = sequence["start_mark_time"];
        state["end_mark_time"] = sequence["end_mark_time"];
        state["end_time"] = sequence["end_time"];
        state["channel_mode"] = CurrentChannelMode();

        return state;
    }

    void
    New()
    {
        ResetPlaybackIndex();
        filename = "untitled" + std::to_string(untitled_count++) + ".json";
        sequence_data = dictionary(); // in case it is not empty
        sequence_data["type"] = "Ikaros Sequence Data";
        sequence_data["version"] = sequence_data_version;
        sequence_data["time_unit"] = sequence_data_time_unit;
        sequence_data["channels"] = channels.as_int();
        sequence_data["ranges"] = CreateDefaultRanges();
        sequence_data["sequences"] = list();
        for (int i = 0; i < max_sequences; i++)
        {
            sequence_data["sequences"].push_back(create_sequence(i, layout_width.as_int()));
        }

        UpdateSequenceNames();
        MarkSequenceChanged();
    }

    bool
    Open(const std::string &name)
    {
        ResetPlaybackIndex();
        if (name.empty())
            return false;

        auto path = resolved_directory / name;

        if (!fs::exists(path))
        {
            Notify(msg_warning, "File does not exist.");
            return false;
        }

        try
        {
            dictionary data;
            data.load_json(path.string());

            // Validate

            if (data["type"].is_null() || data["type"].as_string() != u8"Ikaros Sequence Data")
            {
                Notify(msg_warning, "File has wrong format. Cannot be opened.");
                return false;
            }

            if (!NormalizeLoadedSequenceHeader(data))
                return false;

            if (data["channels"].is_null() || data["channels"] < channels) // File might include more channels then used.
            {
                Notify(msg_warning, "Sequence file has wrong number of channels. Cannot be opened.");
                return false;
            }

            if (!NormalizeLoadedSequenceData(data))
                return false;

            // Data is ok

            sequence_data = data;
            UpdateSequenceNames();
            LoadChannelMode();
            LinkKeypoints(); // Just in case...
            current_sequence = 0;
            filename = name;
            ResetPlaybackIndex();
            MarkSequenceChanged();
        }

        catch (const std::exception &e)
        {
            Notify(msg_warning, "Sequence file could not be loaded.");
            return false;
        }
        return true;
    }

    bool
    FileNameIsListed(const std::string &name)
    {
        for (auto file_name : split(std::string(file_names), ","))
            if (file_name == name)
                return true;

        return false;
    }

    void
    Save(const std::string &name)
    {
        if (ends_with(name, ".json"))
            filename = name;
        else
            filename = name + ".json";
        auto path = resolved_directory / std::string(filename);

        LinkKeypoints();
        StoreChannelMode();

        std::ofstream file(path);

        if (!file.is_open())
        {
            Notify(msg_warning, "Could not open file for writing: " + path.string());
            return;
        }

        file << sequence_data.json() << std::endl;

        if (!FileNameIsListed(std::string(filename)))
        {
            if (std::string(file_names).empty())
                file_names = filename;
            else
                file_names = std::string(file_names) + "," + std::string(filename);
        }
    }

    void
    Init()
    {
        timer.Stop();
        sequence_revision = 0;
        ResetPlaybackIndex();
        smoothing_active = false;
        smoothing_pending = false;
        smoothing_start_clock = 0;
        smoothing_duration = 0;
        smoothing_alpha = 1;
        last_current_sequence = 0;

        Bind(channels, "channels");
        Bind(positions, "positions"); // parameter size will be set by the value channels
        Bind(range_min, "range_min");
        Bind(range_max, "range_max");
        Bind(interpolation, "interpolation");
        Bind(smoothing_time, "smoothing_time");
        Bind(simplify_epsilon, "simplify_epsilon");
        Bind(state, "state");
        Bind(loop, "loop");
        Bind(shuffle, "shuffle");
        Bind(channel_mode, "channel_mode");
        Bind(time_string, "time");
        Bind(end_time_string, "end_time");
        Bind(position, "position");
        Bind(mark_start, "mark_start");
        Bind(mark_end, "mark_end");
        Bind(max_sequences, "max_sequences");
        Bind(layout_width, "layout_width");
        Bind(sequence_names, "sequence_names");
        Bind(file_names, "file_names");
        Bind(filename, "filename");
        Bind(current_sequence, "current_sequence");
        Bind(internal_control, "internal_control");
        Bind(default_output, "default_output");
        Bind(directory, "directory");
        Bind(filename, "filename");
        Bind(trig, "TRIG");
        trig_last = matrix(trig.size());
        trig_last.reset();
        Bind(playing, "PLAYING");
        Bind(completed, "COMPLETED");
        Bind(target, "TARGET");
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
        Bind(active, "ACTIVE");
        Bind(smoothing_start, "SMOOTHING_START");

        if (range_max.size_x() != channels)
            Notify(msg_fatal_error, "Max range not set for correct number of channels.");

        if (interpolation.size_x() != channels)
            Notify(msg_fatal_error, "Interpolation not set for correct number of channels.");

        if (range_min.size_x() != channels)
            Notify(msg_fatal_error, "Min range not set for correct number of channels.");

        if (max_sequences.as_int() <= 0)
            Notify(msg_fatal_error, "max_sequences must be greater than zero.");

        if (layout_width.as_int() <= 0)
            Notify(msg_fatal_error, "layout_width must be greater than zero.");

        if (default_output.size_x() != channels)
            Notify(msg_fatal_error, "Incorrect size for default_output; does not match number of channels.");

        file_names = "";

        if(!kernel().SanitizeWritePath(std::string(directory), resolved_directory))
        {
            Notify(msg_fatal_error, "SequenceRecorder can only use directories inside UserData.");
            return;
        }
        directory = resolved_directory.string();

        left_output.copy(default_output);
        right_output.copy(default_output);

        for (int c = 0; c < channels.as_int(); c++)
            if (internal_control(c))
                positions(c) = default_output(c);

        untitled_count = 1;

        fs::create_directories(resolved_directory);

        // Trick to make module run with too few inputs connected

        input.realloc(channels.as_int());

        left_index = matrix(channels.as_int());
        right_index = matrix(channels.as_int());

        for (int c = 0; c < channels.as_int(); c++)
        {
            left_index[c] = 0;
            right_index[c] = INT_MAX;
        }

        if (!std::string(filename).empty() && fs::exists(resolved_directory / std::string(filename)))
        {
            if (!Open(std::string(filename)))
                New();
        }
        else
            New();

        last_current_sequence = current_sequence.as_int();
        last_channel_mode.copy(channel_mode);

        // Get files in directory

        std::string fsep = "";
        for (auto &p : fs::directory_iterator(resolved_directory))
        {
            auto pp = p.path();
            if (pp.extension() == ".json")
            {
                file_names = std::string(file_names) + fsep + std::string(pp.filename());
                fsep = ",";
            }
        }

        Stop();
    }

    void
    Tick()
    {
        double tl = GetTickDuration();
        playing.reset();
        completed.reset();

        if (!EnsureCurrentSequence())
            return;

        if (current_sequence.as_int() != last_current_sequence)
        {
            ResetPlaybackIndex();
            RequestOutputSmoothing();
            last_current_sequence = current_sequence.as_int();
        }

        if (ChannelModeChanged())
        {
            RequestOutputSmoothing();
            last_channel_mode.copy(channel_mode);
        }

        // Check trig input

        for (int s = 0; s < trig.size(); s++)
            if (trig[s] > 0 && trig_last[s] == 0) // Trig on rising edge
                Trig(s);

        trig_last.copy(trig);
        last_current_sequence = current_sequence.as_int();
        auto &sequence = CurrentSequence();
        float t = timer.GetTime();

        if (start_record) // timer start at tick to increase probability of overlapping keypoint when starting at a keypoint
        {
            timer.Continue();
            start_record = false;
            t = timer.GetTime();
        }

        float end_time = sequence["end_time"];
        SyncPositionFromTime(t, end_time);

        for (int c = 0; c < channels.as_int(); c++)
            if (internal_control[c])
                input[c] = positions[c];

        if (state[1]) // handle play mode
        {
            set_one_hot(playing, current_sequence, max_sequences);
            if (loop && t >= float(sequence["end_mark_time"])) // loop
            {
                t = ContinueFromTime(sequence["start_mark_time"]);
                SyncPositionFromTime(t, sequence["end_time"]);
            }
            else if (TimeHasReachedEnd(t, end_time, tl))
            {
                int completed_sequence = current_sequence.as_int();
                set_one_hot(completed, completed_sequence, max_sequences);

                if (shuffle)
                {
                    int next_sequence = RandomPlayableSequence(completed_sequence);
                    if (next_sequence != -1)
                    {
                        Trig(next_sequence);
                        set_one_hot(playing, current_sequence, max_sequences);
                        return;
                    }
                    else
                    {
                        Pause();
                        SetPausedTime(sequence["end_time"]);
                    }
                }
                else
                {
                    Pause();
                    SetPausedTime(sequence["end_time"]);
                }
            }
        }

        else if (state[2]) // handle record mode
        {
            if (TimeHasReachedEnd(t, end_time, tl)) // extend recoding if at end
                sequence["end_time"] = quantize(t, tl);
        }

        // Set outputs

        GoToTime(t);
        StartPendingOutputSmoothing();
        UpdateOutputSmoothing();

        for (int c = 0; c < channels.as_int(); c++)
            SetOutputForChannel(c);

        // AddPoints if in recording mode

        if (state[2] == 1) // record mode
        {
            DeleteKeypointsInRange(quantize(last_record_position, tl), quantize(t, tl));
            last_record_position = t;
            AddKeypoint(t);
        }

        // Set position again

        end_time = sequence["end_time"];
        SyncPositionFromTime(t, end_time);

        time_string = make_timestamp(quantize(t, tl));
        end_time_string = make_timestamp(quantize(double(sequence["end_time"]), tl));

        if (float(end_time = sequence["end_time"]) != 0)
        {
            mark_start = float(sequence["start_mark_time"]) / end_time;
            mark_end = float(sequence["end_mark_time"]) / end_time;
        }

        for (int c = 0; c < channels.as_int(); c++)
            if (internal_control[c] == 0)
                positions[c] = output[c];
    }

    // Current state

    parameter channels;
    parameter max_sequences;
    parameter layout_width;
    parameter simplify_epsilon;

    matrix range_min;
    matrix range_max;
    matrix interpolation;

    matrix trig;
    matrix trig_last;

    matrix playing;
    matrix completed;

    matrix positions;

    parameter smoothing_time;
    matrix smoothing_start;
    bool smoothing_active;
    bool smoothing_pending;
    double smoothing_start_clock;
    double smoothing_duration;
    double smoothing_alpha;

    matrix target;
    matrix input;
    matrix default_output; // value for initial from ikg file if set
    matrix left_output;    // Value to use to the left of first keypoint
    matrix right_output;   // value to use to the right of the last keypoint
    matrix internal_control;

    matrix output;
    matrix active;

    bool start_record;
    parameter current_sequence;
    parameter sequence_names;
    parameter file_names;
    dictionary sequence_data;

    // Control  variables

    int states = 8;
    int modes = 4;
    int last_current_sequence;

    matrix state; // state of the head controller buttons
    matrix channel_mode;
    matrix last_channel_mode;
    parameter loop;
    parameter shuffle;

    matrix left_index;
    matrix right_index;

    float last_time;
    float last_index;
    int sequence_revision;
    int playback_sequence;
    int playback_index;
    int playback_keypoint_count;
    float playback_time;

    Timer timer;
    parameter position;
    float last_record_position;
    float last_position; // to see if the value has been changed by WebUI
    parameter mark_start;
    parameter mark_end;

    parameter directory;
    parameter filename;
    fs::path resolved_directory;

    int untitled_count;
    parameter time_string;
    parameter end_time_string;

    void SetOutputForTime(float t);
};

INSTALL_CLASS(SequenceRecorder)
