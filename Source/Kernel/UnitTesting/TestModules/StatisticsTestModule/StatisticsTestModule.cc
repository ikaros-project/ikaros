#include <cmath>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

#include "ikaros.h"

using namespace ikaros;

namespace
{
    constexpr double tolerance = 1.0e-12;

    static_assert(noexcept(std::declval<const statistics &>().mean()));
    static_assert(!noexcept(std::declval<const statistics &>().median()));
    static_assert(!noexcept(std::declval<const statistics &>().quantile(0.5)));
    static_assert(!noexcept(std::declval<const statistics &>().mode()));

    void
    require_close(double actual, double expected, const std::string & message)
    {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > tolerance)
            throw exception("StatisticsTestModule: " + message +
                            " (expected " + std::to_string(expected) +
                            ", got " + std::to_string(actual) + ")");
    }


    template<typename StatisticsType>
    StatisticsType
    make_statistics(std::initializer_list<double> values)
    {
        StatisticsType result;
        for (double value : values)
            result.push(value);
        return result;
    }


    template<typename Function>
    void
    require_invalid_argument(Function function, const std::string & message)
    {
        try
        {
            function();
        }
        catch (const std::invalid_argument &)
        {
            return;
        }
        throw exception("StatisticsTestModule: " + message);
    }


    template<typename StatisticsType>
    void
    test_non_finite_samples(const std::string & name)
    {
        StatisticsType values;
        values.push(1.0);

        require_invalid_argument(
            [&values] { values.push(std::numeric_limits<double>::quiet_NaN()); },
            name + " accepted NaN");
        require_invalid_argument(
            [&values] { values.push(std::numeric_limits<double>::infinity()); },
            name + " accepted positive infinity");
        require_invalid_argument(
            [&values] { values.push(-std::numeric_limits<double>::infinity()); },
            name + " accepted negative infinity");

        if (values.count() != 1)
            throw exception("StatisticsTestModule: " + name +
                            " changed after rejecting a non-finite sample");
    }
}


class StatisticsTestModule: public Module
{
    void Init() override
    {
        const statistics skewed = make_statistics<statistics>({1.0, 1.0, 2.0});
        const online_statistics online_skewed = make_statistics<online_statistics>({1.0, 1.0, 2.0});
        require_close(skewed.skewness(), std::sqrt(3.0), "batch corrected skewness");
        require_close(online_skewed.skewness(), std::sqrt(3.0), "online corrected skewness");

        const statistics uniform = make_statistics<statistics>({1.0, 2.0, 3.0, 4.0});
        const online_statistics online_uniform = make_statistics<online_statistics>({1.0, 2.0, 3.0, 4.0});
        require_close(uniform.skewness(), 0.0, "batch symmetric skewness");
        require_close(online_uniform.skewness(), 0.0, "online symmetric skewness");
        require_close(uniform.kurtosis(), -1.2, "batch corrected excess kurtosis");
        require_close(online_uniform.kurtosis(), -1.2, "online corrected excess kurtosis");

        const statistics varied =
            make_statistics<statistics>({2.0, 8.0, 0.0, 4.0, 1.0, 9.0, 9.0, 0.0});
        const online_statistics online_varied =
            make_statistics<online_statistics>({2.0, 8.0, 0.0, 4.0, 1.0, 9.0, 9.0, 0.0});
        require_close(online_varied.mean(), varied.mean(), "online mean matches batch");
        require_close(online_varied.variance(), varied.variance(), "online variance matches batch");
        require_close(online_varied.skewness(), varied.skewness(), "online skewness matches batch");
        require_close(online_varied.kurtosis(), varied.kurtosis(), "online kurtosis matches batch");

        online_statistics reset;
        reset.push(100.0);
        reset.push(-100.0);
        reset.reset();
        for (double value : {1.0, 2.0, 3.0, 4.0})
            reset.push(value);
        require_close(reset.skewness(), 0.0, "online reset skewness");
        require_close(reset.kurtosis(), -1.2, "online reset kurtosis");

        const statistics constant = make_statistics<statistics>({3.0, 3.0, 3.0, 3.0});
        const online_statistics online_constant =
            make_statistics<online_statistics>({3.0, 3.0, 3.0, 3.0});
        require_close(constant.skewness(), 0.0, "batch constant skewness");
        require_close(constant.kurtosis(), 0.0, "batch constant kurtosis");
        require_close(online_constant.skewness(), 0.0, "online constant skewness");
        require_close(online_constant.kurtosis(), 0.0, "online constant kurtosis");

        test_non_finite_samples<statistics>("batch statistics");
        test_non_finite_samples<online_statistics>("online statistics");

        statistics quantiles;
        quantiles.push(-std::numeric_limits<double>::max());
        quantiles.push(std::numeric_limits<double>::max());
        require_invalid_argument(
            [&quantiles] { quantiles.quantile(std::numeric_limits<double>::quiet_NaN()); },
            "quantile accepted NaN");
        require_invalid_argument(
            [&quantiles] { quantiles.quantile(std::numeric_limits<double>::infinity()); },
            "quantile accepted positive infinity");
        require_invalid_argument(
            [&quantiles] { quantiles.quantile(-std::numeric_limits<double>::infinity()); },
            "quantile accepted negative infinity");
        require_close(quantiles.mean(), 0.0, "batch extreme mean");
        require_close(quantiles.median(), 0.0, "batch extreme median");
        require_close(quantiles.quantile(0.5), 0.0, "batch extreme quantile midpoint");

        const double maximum = std::numeric_limits<double>::max();
        const statistics high = make_statistics<statistics>({maximum, maximum});
        const online_statistics online_high =
            make_statistics<online_statistics>({maximum, maximum});
        require_close(high.mean(), maximum, "batch maximum mean");
        require_close(high.median(), maximum, "batch maximum median");
        require_close(online_high.mean(), maximum, "online maximum mean");
        require_close(online_high.median(), maximum, "online maximum median");

        const online_statistics online_extremes =
            make_statistics<online_statistics>({-maximum, maximum});
        require_close(online_extremes.mean(), 0.0, "online extreme mean");
        require_close(online_extremes.median(), 0.0, "online extreme median");

        std::cout << "STATISTICS TEST OK" << std::endl;
    }
};

INSTALL_CLASS(StatisticsTestModule)
