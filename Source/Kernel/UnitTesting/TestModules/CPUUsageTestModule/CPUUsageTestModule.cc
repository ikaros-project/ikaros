#include <cmath>
#include <limits>

#include "ikaros.h"

using namespace ikaros;

namespace
{
    constexpr double tolerance = 1e-12;

    void require_close(double actual, double expected, const std::string & message)
    {
        if(std::fabs(actual - expected) > tolerance)
            throw exception("CPUUsageTestModule: " + message +
                " (expected " + std::to_string(expected) +
                ", got " + std::to_string(actual) + ")");
    }
}

class CPUUsageTestModule : public Module
{
    void Init() override
    {
        require_close(CPUUsageFraction(0.01, 0.01, 8), 0.125, "normalization by duration and cores");
        require_close(CPUUsageFraction(0.005, 0.02, 4), 0.0625, "partial utilization");
        require_close(CPUUsageFraction(0.02, 0.01, 1), 1.0, "utilization upper bound");
        require_close(CPUUsageFraction(-0.01, 0.01, 1), 0.0, "negative CPU delta");
        require_close(CPUUsageFraction(0.01, 0.0, 1), 0.0, "zero duration");
        require_close(CPUUsageFraction(0.01, 0.01, 0), 0.0, "zero core count");
        require_close(CPUUsageFraction(std::numeric_limits<double>::quiet_NaN(), 0.01, 1), 0.0,
            "non-finite CPU delta");

        Kernel & k = kernel();
        if(!k.CalculateCPUUsage())
            throw exception("CPUUsageTestModule: initial CPU usage sample must run immediately");
        const double initial_cpu_usage = k.GetCPUUsage();
        if(k.CalculateCPUUsage())
            throw exception("CPUUsageTestModule: CPU usage must not be sampled twice within 100 ms");
        require_close(k.GetCPUUsage(), initial_cpu_usage,
                      "rate-limited sampling retains the previous value");

        Sleep(0.11);
        if(!k.CalculateCPUUsage())
            throw exception("CPUUsageTestModule: CPU usage must be sampled after 100 ms");
        if(!std::isfinite(k.GetCPUUsage()) || k.GetCPUUsage() < 0 || k.GetCPUUsage() > 1)
            throw exception("CPUUsageTestModule: CPU usage must be a finite fraction");
        if(k.GetCPUCoreCount() < 1)
            throw exception("CPUUsageTestModule: CPU core count must be at least one");

        std::cout << "CPU USAGE TEST OK" << std::endl;
    }
};

INSTALL_CLASS(CPUUsageTestModule)
