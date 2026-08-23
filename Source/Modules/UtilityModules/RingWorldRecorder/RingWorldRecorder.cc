#include "ikaros.h"

using namespace ikaros;

class RingWorldRecorder: public Module
{
    parameter signalCount_;
    parameter capacity_;

    matrix signals_;
    matrix protocolTime_;
    matrix trialIndex_;
    matrix trialTime_;
    matrix trialActive_;
    matrix sampleWindows_;
    matrix signalHistory_;
    matrix timeHistory_;
    matrix trialIndexHistory_;
    matrix trialTimeHistory_;
    matrix trialActiveHistory_;
    matrix windowHistory_;
    matrix count_;
    matrix overflow_;

    int sampleCount_ = 0;


    void Init() override
    {
        Bind(signalCount_, "signal_count");
        Bind(capacity_, "capacity");
        Bind(signals_, "SIGNALS");
        Bind(protocolTime_, "PROTOCOL_TIME");
        Bind(trialIndex_, "TRIAL_INDEX");
        Bind(trialTime_, "TRIAL_TIME");
        Bind(trialActive_, "TRIAL_ACTIVE");
        Bind(sampleWindows_, "SAMPLE_WINDOWS");
        Bind(signalHistory_, "SIGNAL_HISTORY");
        Bind(timeHistory_, "TIME_HISTORY");
        Bind(trialIndexHistory_, "TRIAL_INDEX_HISTORY");
        Bind(trialTimeHistory_, "TRIAL_TIME_HISTORY");
        Bind(trialActiveHistory_, "TRIAL_ACTIVE_HISTORY");
        Bind(windowHistory_, "WINDOW_HISTORY");
        Bind(count_, "COUNT");
        Bind(overflow_, "OVERFLOW");

        if(signalHistory_.rank() != 2 || signalHistory_.rows() != capacity_.as_int() ||
           signalHistory_.cols() != signalCount_.as_int())
            throw exception("RingWorldRecorder: SIGNAL_HISTORY shape was not resolved.", path_);
        if(windowHistory_.rank() != 2 || windowHistory_.rows() != capacity_.as_int() ||
           windowHistory_.cols() != sampleWindows_.size())
            throw exception("RingWorldRecorder: WINDOW_HISTORY shape was not resolved.", path_);
    }


    void Tick() override
    {
        if(sampleCount_ >= capacity_.as_int())
        {
            overflow_(0) = 1.0f;
            return;
        }

        for(int signal = 0; signal < signalCount_.as_int(); ++signal)
            signalHistory_(sampleCount_, signal) = signals_(signal);
        for(int window = 0; window < sampleWindows_.size(); ++window)
            windowHistory_(sampleCount_, window) = sampleWindows_(window);
        timeHistory_(sampleCount_) = protocolTime_(0);
        trialIndexHistory_(sampleCount_) = trialIndex_(0);
        trialTimeHistory_(sampleCount_) = trialTime_(0);
        trialActiveHistory_(sampleCount_) = trialActive_(0);
        ++sampleCount_;
        count_(0) = float(sampleCount_);
    }
};

INSTALL_CLASS(RingWorldRecorder)
