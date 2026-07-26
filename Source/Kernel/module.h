// Ikaros 3.0

#pragma once

#include <functional>

#include "component.h"

namespace ikaros
{
    class Module : public Component
    {
    public:
        Module();
        ~Module() override = default;

        int SetOutputShape(dictionary d, input_map ingoing_connections) override;
        int SetOutputShapes(input_map ingoing_connections) override;
        int SetStateShapes(input_map ingoing_connections) override;
        int SetSizes(input_map ingoing_connections) override;

        tick_count GetTick() const;
        double GetTickDuration() const;
        double GetTime() const;
        double GetRealTime() const;
        double GetNominalTime() const;
        double GetRunTime() const;
        double GetTimeOfDay() const;
        double GetLag() const;
        double GetUptime() const;
        double GetActualTickDuration() const;
        double GetTickTimeUsage() const;
        double GetCPUUsage() const;
        double GetIdleTime() const;
        int GetRunMode() const;
        int GetCPUCoreCount() const;
        int GetModuleCount() const;
        int GetClassCount() const;
        tick_count GetStopAfter() const;

        bool TryProfilingBegin() override;
        void ProfilingBegin() override;
        void ProfilingEnd() override;
    };

    using ModuleCreator = std::function<Module *()>;

    class InitClass
    {
    public:
        InitClass(const char * name, ModuleCreator mc);
    };
}

#define INSTALL_CLASS(class_name) \
    static ikaros::InitClass init_##class_name(#class_name, []() { return new class_name(); });
