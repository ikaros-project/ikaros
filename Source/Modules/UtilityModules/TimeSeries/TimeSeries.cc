#include "ikaros.h"

using namespace ikaros;

class TimeSeries: public Module
{
    parameter data;
    parameter base_duration;
    parameter loop;
    parameter first_column_duration;

    matrix output;
    matrix trig;

    double series_position = 0.0;
    int current_index = 0;

    float CurrentStepDuration(const matrix & series) const
    {
        if(!first_column_duration || series.rank() < 2 || series.cols() == 0)
            return 1.0f;

        float duration = series(current_index, 0);
        return duration > 0 ? duration : 1.0f;
    }

    void EmitCurrentSample(const matrix & series)
    {
        if(first_column_duration && series.rank() >= 2)
        {
            int value_count = std::max(0, series.cols() - 1);
            if(output.is_uninitialized() || output.rank() != 1 || output.size(0) != value_count)
                output.realloc(value_count);

            for(int i = 0; i < value_count; ++i)
                output(i) = series(current_index, i + 1);

            return;
        }

        output.copy(series[current_index]);
    }

    void Init()
    {
        Bind(data, "data");
        Bind(base_duration, "base_duration");
        Bind(loop, "loop");
        Bind(first_column_duration, "first_column_duration");

        Bind(output, "OUTPUT");
        Bind(trig, "TRIG");

        trig = 0;
        series_position = 0.0;
        current_index = 0;

        matrix series = data;
        if(series.empty())
            return;

        if(series.rank() <= 1)
            output.copy(series);
        else if(series.size(0) > 0)
            EmitCurrentSample(series);
    }

    void Tick()
    {
        trig = 0;

        matrix series = data;
        if(series.empty())
            return;

        if(series.rank() <= 1)
        {
            output.copy(series);
            return;
        }

        int sample_count = series.size(0);
        if(sample_count <= 0)
            return;

        EmitCurrentSample(series);

        if(base_duration > 0)
        {
            series_position += GetTickDuration();
            while(series_position >= static_cast<double>(base_duration) * CurrentStepDuration(series))
            {
                series_position -= static_cast<double>(base_duration) * CurrentStepDuration(series);
                current_index += 1;

                if(loop)
                    current_index %= sample_count;
                else if(current_index >= sample_count)
                {
                    current_index = sample_count - 1;
                    series_position = 0.0;
                    break;
                }

                trig = 1;
            }
        }
    }
};

INSTALL_CLASS(TimeSeries)
