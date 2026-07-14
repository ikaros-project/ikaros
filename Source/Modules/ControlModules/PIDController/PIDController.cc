#include "ikaros.h"

#include <cmath>

using namespace ikaros;

class PIDController: public Module
{
    parameter Kb;
    parameter Kp;
    parameter Ki;
    parameter Kd;
    parameter derivativeMode;

    parameter Fs;
    parameter Fm;
    parameter Fp;
    parameter Fi;
    parameter Fd;
    parameter Fc;

    parameter Cmin;
    parameter Cmax;
    parameter outputRateLimit;

    matrix input;
    matrix setPoint;
    matrix reset;

    matrix output;
    matrix delta;
    matrix filteredSetPoint;
    matrix filteredInput;
    matrix filteredErrorP;
    matrix filteredErrorI;
    matrix filteredErrorD;
    matrix integral;

    matrix previousDelta;
    matrix previousFilteredInput;

    void Init()
    {
        Bind(Kb, "Kb");
        Bind(Kp, "Kp");
        Bind(Ki, "Ki");
        Bind(Kd, "Kd");
        Bind(derivativeMode, "derivative_mode");

        Bind(Fs, "Fs");
        Bind(Fm, "Fm");
        Bind(Fp, "Fp");
        Bind(Fi, "Fi");
        Bind(Fd, "Fd");
        Bind(Fc, "Fc");

        Bind(Cmin, "Cmin");
        Bind(Cmax, "Cmax");
        Bind(outputRateLimit, "output_rate_limit");

        Bind(input, "INPUT");
        Bind(setPoint, "SETPOINT");
        Bind(reset, "RESET");

        Bind(output, "OUTPUT");
        Bind(delta, "DELTA");
        Bind(filteredSetPoint, "FILTERED_SETPOINT");
        Bind(filteredInput, "FILTERED_INPUT");
        Bind(filteredErrorP, "FILTERED_ERROR_P");
        Bind(filteredErrorI, "FILTERED_ERROR_I");
        Bind(filteredErrorD, "FILTERED_ERROR_D");
        Bind(integral, "INTEGRAL");

        ValidateShapes();
        ValidateParameters();
        previousDelta.realloc(input.shape());
        previousDelta.set(0.0f);
        previousFilteredInput.realloc(input.shape());
        previousFilteredInput.set(0.0f);
    }

    void Tick()
    {
        const float kb = Kb.as_float();
        const float kp = Kp.as_float();
        const float ki = Ki.as_float();
        const float kd = Kd.as_float();

        const float cmin = Cmin.as_float();
        const float cmax = Cmax.as_float();
        const float dt = static_cast<float>(GetTickDuration());
        if(dt <= 0.0f)
            throw exception("PIDController: tick duration must be positive.", path_);

        for(int i = 0; i < input.size(); ++i)
        {
            if(ShouldReset(i))
            {
                ResetElement(i, kb);
                continue;
            }

            filteredSetPoint.data()[i] = Filter(filteredSetPoint.data()[i], setPoint.data()[i], Fs);
            filteredInput.data()[i] = Filter(filteredInput.data()[i], input.data()[i], Fm);
            delta.data()[i] = filteredSetPoint.data()[i] - filteredInput.data()[i];
            filteredErrorP.data()[i] = Filter(filteredErrorP.data()[i], delta.data()[i], Fp);
            filteredErrorI.data()[i] = Filter(filteredErrorI.data()[i], delta.data()[i], Fi);
            const float derivative = Derivative(i, dt);
            filteredErrorD.data()[i] = Filter(filteredErrorD.data()[i], derivative, Fd);

            integral.data()[i] += filteredErrorI.data()[i] * dt;

            float control = kb + kp * filteredErrorP.data()[i] + ki * integral.data()[i] +
                kd * filteredErrorD.data()[i];

            if(control > cmax)
            {
                control = cmax;
                integral.data()[i] -= filteredErrorI.data()[i] * dt;
            }
            else if(control < cmin)
            {
                control = cmin;
                integral.data()[i] -= filteredErrorI.data()[i] * dt;
            }

            const float filteredControl = Filter(output.data()[i], control, Fc);
            output.data()[i] = LimitOutputChange(output.data()[i], filteredControl);
            previousDelta.data()[i] = delta.data()[i];
            previousFilteredInput.data()[i] = filteredInput.data()[i];
        }
    }

    void ValidateShapes()
    {
        if(input.shape() != setPoint.shape())
            throw exception("PIDController: SETPOINT shape must match INPUT shape.", path_);

        if(reset.connected() && reset.shape() != input.shape())
            throw exception("PIDController: RESET shape must match INPUT shape.", path_);

        ValidateOutput(output, "OUTPUT");
        ValidateOutput(delta, "DELTA");
        ValidateOutput(filteredSetPoint, "FILTERED_SETPOINT");
        ValidateOutput(filteredInput, "FILTERED_INPUT");
        ValidateOutput(filteredErrorP, "FILTERED_ERROR_P");
        ValidateOutput(filteredErrorI, "FILTERED_ERROR_I");
        ValidateOutput(filteredErrorD, "FILTERED_ERROR_D");
        ValidateOutput(integral, "INTEGRAL");
    }

    void ValidateParameters()
    {
        if(Cmin.as_float() > Cmax.as_float())
            throw exception("PIDController: Cmin must be less than or equal to Cmax.", path_);

        if(derivativeMode.as_int() < 0 || derivativeMode.as_int() > 1)
            throw exception("PIDController: derivative_mode must be error or measurement.", path_);

        if(outputRateLimit.as_float() < 0.0f)
            throw exception("PIDController: output_rate_limit must be non-negative.", path_);
    }

    void ValidateOutput(const matrix & m, const std::string & name)
    {
        if(m.shape() != input.shape())
            throw exception("PIDController: " + name + " shape must match INPUT shape.", path_);
    }

    bool ShouldReset(int i) const
    {
        return reset.connected() && reset.data()[i] != 0.0f;
    }

    void ResetElement(int i, float kb)
    {
        filteredSetPoint.data()[i] = setPoint.data()[i];
        filteredInput.data()[i] = input.data()[i];
        delta.data()[i] = filteredSetPoint.data()[i] - filteredInput.data()[i];
        filteredErrorP.data()[i] = 0.0f;
        filteredErrorI.data()[i] = 0.0f;
        filteredErrorD.data()[i] = 0.0f;
        integral.data()[i] = 0.0f;
        previousDelta.data()[i] = delta.data()[i];
        previousFilteredInput.data()[i] = filteredInput.data()[i];
        output.data()[i] = kb;
    }

    float Derivative(int i, float dt)
    {
        switch(derivativeMode.as_int())
        {
            case 0:
                return (delta.data()[i] - previousDelta.data()[i]) / dt;

            case 1:
                return -(filteredInput.data()[i] - previousFilteredInput.data()[i]) / dt;

            default:
                throw exception("PIDController: derivative_mode must be error or measurement.", path_);
        }
    }

    float Filter(float current, float target, const parameter & rate)
    {
        const float ratePerTick = rate.as_float();
        if(ratePerTick <= 0.0f)
            return target;

        const float blend = 1.0f - std::exp(-ratePerTick);
        return current + blend * (target - current);
    }

    float LimitOutputChange(float current, float target)
    {
        const float maxChange = outputRateLimit.as_float();
        if(maxChange <= 0.0f)
            return target;

        const float change = target - current;
        if(change > maxChange)
            return current + maxChange;
        if(change < -maxChange)
            return current - maxChange;
        return target;
    }
};

INSTALL_CLASS(PIDController)
