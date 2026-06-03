#include "ikaros.h"

#include <algorithm>
#include <array>
#include <cmath>

using namespace ikaros;

class BalkeniusReactiveControl : public Module
{
    matrix sensors_;
    matrix odour_;
    matrix custom_weights_;
    matrix motor_;
    matrix sensory_;
    matrix nodes_;
    matrix weight_output_;
    matrix raw_motor_;

    parameter behavior_;
    parameter a_;
    parameter b_;
    parameter c_;
    parameter theta_;
    parameter motor_threshold_;
    parameter gain_;
    parameter max_motor_;
    parameter odour_channel_;

    void Init()
    {
        Bind(sensors_, "SENSORS");
        Bind(odour_, "ODOUR");
        Bind(custom_weights_, "WEIGHTS");
        Bind(motor_, "MOTOR");
        Bind(sensory_, "SENSORY");
        Bind(nodes_, "NODES");
        Bind(weight_output_, "WEIGHT_OUTPUT");
        Bind(raw_motor_, "RAW_MOTOR");

        Bind(behavior_, "behavior");
        Bind(a_, "a");
        Bind(b_, "b");
        Bind(c_, "c");
        Bind(theta_, "theta");
        Bind(motor_threshold_, "motor_threshold");
        Bind(gain_, "gain");
        Bind(max_motor_, "max_motor");
        Bind(odour_channel_, "odour_channel");
    }

    static float positive_part(float x)
    {
        return std::max(0.0f, x);
    }

    std::array<float, 6> PresetWeights()
    {
        const float a = a_;
        const float b = b_;
        const float c = c_;
        matrix behavior_matrix = behavior_;
        const int behavior = behavior_matrix.empty() ? 0 : static_cast<int>(std::round(behavior_matrix(0)));

        // Weight order follows Table 4.2.11: m, n, o, p, q, r.
        std::array<float, 6> w = {0, 0, 0, 0, 0, 0};

        if (behavior == 12)
        {
            if (custom_weights_.connected())
                for (int i = 0; i < 6 && i < custom_weights_.size(); ++i)
                    w[i] = custom_weights_(i);
            return w;
        }

        if (behavior == 0)
            w = {a, 0, 0, 0, 0, 0};
        else if (behavior == 1)
            w = {0, a, 0, 0, 0, c};
        else if (behavior == 2)
            w = {a, 0, b, 0, 0, 0};
        else if (behavior == 3)
            w = {0, a, 0, 0, b, c};
        else if (behavior == 4)
            w = {a, 0, 0, b, 0, 0};
        else if (behavior == 5)
            w = {0, 0, a, 0, 0, 0};
        else if (behavior == 6)
            w = {0, 0, 0, 0, a, c};
        else if (behavior == 7)
            w = {b, 0, a, 0, 0, 0};
        else if (behavior == 8)
            w = {0, b, 0, 0, a, c};
        else if (behavior == 9)
            w = {0, 0, 0, a, 0, c};
        else if (behavior == 10)
            w = {a, a, 0, 0, 0, 0};
        else if (behavior == 11)
            w = {0, 0, a, 0, a, 0};
        else
            Notify(msg_warning, "Unknown behavior preset; using zero weights.");

        return w;
    }

    std::array<float, 2> ReadSensors() const
    {
        if (odour_.connected())
        {
            if (odour_.size_y() < 2)
                throw exception("BalkeniusReactiveControl: ODOUR must have at least two rows.", path_);

            const int channel = static_cast<int>(clip(std::round(static_cast<float>(odour_channel_)), 0.0f, static_cast<float>(odour_.size_x() - 1)));
            return {odour_(0, channel), odour_(1, channel)};
        }

        if (!sensors_.connected())
            return {0, 0};

        if (sensors_.size() < 2)
            throw exception("BalkeniusReactiveControl: SENSORS must contain left and right values.", path_);

        return {sensors_(0), sensors_(1)};
    }

    void Tick()
    {
        const std::array<float, 2> s = ReadSensors();
        const std::array<float, 6> w = PresetWeights();

        const float s_left = s[0];
        const float s_right = s[1];
        const float theta = theta_;

        const float a_left = positive_part(s_left - theta);
        const float a_right = positive_part(s_right - theta);
        const float q_passive = positive_part(0.5f * (s_left + s_right) - theta);

        const float m = w[0];
        const float n = w[1];
        const float o = w[2];
        const float p = w[3];
        const float q = w[4];
        const float r = w[5];

        float left = r + m * s_right + o * s_left - n * a_left - q * a_right - p * q_passive;
        float right = r + m * s_left + o * s_right - n * a_right - q * a_left - p * q_passive;

        left = positive_part(left - static_cast<float>(motor_threshold_));
        right = positive_part(right - static_cast<float>(motor_threshold_));

        raw_motor_(0) = left;
        raw_motor_(1) = right;

        const float limit = std::max(0.0f, static_cast<float>(max_motor_));
        left = clip(left * static_cast<float>(gain_), -limit, limit);
        right = clip(right * static_cast<float>(gain_), -limit, limit);

        motor_(0) = left;
        motor_(1) = right;

        sensory_(0) = s_left;
        sensory_(1) = s_right;

        nodes_(0) = a_left;
        nodes_(1) = q_passive;
        nodes_(2) = a_right;

        for (int i = 0; i < 6; ++i)
            weight_output_(i) = w[i];
    }
};

INSTALL_CLASS(BalkeniusReactiveControl)
