#include "Kernel/statistics.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string>

#include "StatisticsStandaloneTests.h"

namespace
{
constexpr double tolerance = 1.0e-12;


void
require(bool condition, const std::string & message)
{
    if (!condition)
        throw std::runtime_error(message);
}


void
require_close(double actual, double expected, const std::string & message)
{
    const double scale = std::max(1.0, std::fabs(expected));
    if (!std::isfinite(actual) || std::fabs(actual - expected) > tolerance * scale)
        throw std::runtime_error(message + ": expected " + std::to_string(expected) +
                                 ", got " + std::to_string(actual));
}


void
require_nan(double value, const std::string & message)
{
    require(std::isnan(value), message + ": expected NaN");
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
    throw std::runtime_error(message + ": expected std::invalid_argument");
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
    require(values.count() == 1, name + " changed after rejecting samples");
}


void
test_empty_sets()
{
    const statistics batch;
    require(batch.no_data(), "empty batch no_data");
    require(batch.count() == 0, "empty batch count");
    require(batch.data().empty(), "empty batch data");
    require_nan(batch.mean(), "empty batch mean");
    require_nan(batch.median(), "empty batch median");
    require_nan(batch.mode(), "empty batch mode");
    require_nan(batch.min(), "empty batch minimum");
    require_nan(batch.max(), "empty batch maximum");
    require_nan(batch.variance(), "empty batch variance");
    require_nan(batch.standard_deviation(), "empty batch standard deviation");
    require_nan(batch.skewness(), "empty batch skewness");
    require_nan(batch.kurtosis(), "empty batch kurtosis");
    require_nan(batch.q1(), "empty batch first quartile");
    require_nan(batch.q3(), "empty batch third quartile");

    const online_statistics online;
    require(online.count() == 0, "empty online count");
    require_nan(online.mean(), "empty online mean");
    require_nan(online.median(), "empty online median");
    require_nan(online.mode(), "empty online mode");
    require_nan(online.min(), "empty online minimum");
    require_nan(online.max(), "empty online maximum");
    require_nan(online.variance(), "empty online variance");
    require_nan(online.standard_deviation(), "empty online standard deviation");
    require_nan(online.skewness(), "empty online skewness");
    require_nan(online.kurtosis(), "empty online kurtosis");
}


void
test_reference_values()
{
    const statistics uniform = make_statistics<statistics>({1.0, 2.0, 3.0, 4.0});
    require(uniform.count() == 4, "uniform count");
    require_close(uniform.mean(), 2.5, "uniform mean");
    require_close(uniform.median(), 2.5, "uniform median");
    require_close(uniform.mode(), 1.0, "uniform mode tie break");
    require_close(uniform.min(), 1.0, "uniform minimum");
    require_close(uniform.max(), 4.0, "uniform maximum");
    require_close(uniform.variance(), 5.0 / 3.0, "uniform sample variance");
    require_close(uniform.standard_deviation(), std::sqrt(5.0 / 3.0),
                  "uniform standard deviation");
    require_close(uniform.skewness(), 0.0, "uniform skewness");
    require_close(uniform.kurtosis(), -1.2, "uniform excess kurtosis");

    const statistics::order_summary order = uniform.summarize_order();
    require_close(order.q1, 1.75, "uniform first quartile");
    require_close(order.q3, 3.25, "uniform third quartile");
    require_close(order.interquartile_range, 1.5, "uniform interquartile range");
    require_close(order.lower_fence, -0.5, "uniform lower fence");
    require_close(order.lower_whisker, 1.0, "uniform lower whisker");
    require_close(order.upper_whisker, 4.0, "uniform upper whisker");
    require_close(order.upper_fence, 5.5, "uniform upper fence");

    const statistics skewed = make_statistics<statistics>({1.0, 1.0, 2.0});
    require_close(skewed.skewness(), std::sqrt(3.0), "skewed reference skewness");
}


void
test_batch_online_agreement()
{
    const statistics batch =
        make_statistics<statistics>({2.0, 8.0, 0.0, 4.0, 1.0, 9.0, 9.0, 0.0});
    const online_statistics online =
        make_statistics<online_statistics>({2.0, 8.0, 0.0, 4.0, 1.0, 9.0, 9.0, 0.0});

    require(batch.count() == online.count(), "batch and online counts");
    require_close(online.mean(), batch.mean(), "batch and online means");
    require_close(online.median(), batch.median(), "batch and online medians");
    require_close(online.mode(), batch.mode(), "batch and online modes");
    require_close(online.min(), batch.min(), "batch and online minima");
    require_close(online.max(), batch.max(), "batch and online maxima");
    require_close(online.variance(), batch.variance(), "batch and online variances");
    require_close(online.standard_deviation(), batch.standard_deviation(),
                  "batch and online standard deviations");
    require_close(online.skewness(), batch.skewness(), "batch and online skewness");
    require_close(online.kurtosis(), batch.kurtosis(), "batch and online kurtosis");
}


void
test_constant_and_reset()
{
    statistics batch = make_statistics<statistics>({3.0, 3.0, 3.0, 3.0});
    online_statistics online = make_statistics<online_statistics>({3.0, 3.0, 3.0, 3.0});

    require_close(batch.mean(), 3.0, "constant batch mean");
    require_close(batch.variance(), 0.0, "constant batch variance");
    require_close(batch.standard_deviation(), 0.0, "constant batch standard deviation");
    require_close(batch.skewness(), 0.0, "constant batch skewness");
    require_close(batch.kurtosis(), 0.0, "constant batch kurtosis");
    require_close(online.mean(), 3.0, "constant online mean");
    require_close(online.variance(), 0.0, "constant online variance");
    require_close(online.standard_deviation(), 0.0, "constant online standard deviation");
    require_close(online.skewness(), 0.0, "constant online skewness");
    require_close(online.kurtosis(), 0.0, "constant online kurtosis");

    batch.reset();
    online.reset();
    require(batch.count() == 0, "batch reset count");
    require(online.count() == 0, "online reset count");
    for (double value : {1.0, 2.0, 3.0, 4.0})
    {
        batch.push(value);
        online.push(value);
    }
    require_close(online.mean(), batch.mean(), "batch and online means after reset");
    require_close(online.kurtosis(), batch.kurtosis(), "batch and online kurtosis after reset");
}


void
test_extreme_finite_values()
{
    const double maximum = std::numeric_limits<double>::max();
    const statistics high = make_statistics<statistics>({maximum, maximum});
    const online_statistics online_high =
        make_statistics<online_statistics>({maximum, maximum});
    require_close(high.mean(), maximum, "extreme batch mean");
    require_close(high.median(), maximum, "extreme batch median");
    require_close(online_high.mean(), maximum, "extreme online mean");
    require_close(online_high.median(), maximum, "extreme online median");

    const statistics extremes = make_statistics<statistics>({-maximum, maximum});
    const online_statistics online_extremes =
        make_statistics<online_statistics>({-maximum, maximum});
    require_close(extremes.mean(), 0.0, "opposite extreme batch mean");
    require_close(extremes.median(), 0.0, "opposite extreme batch median");
    require_close(extremes.quantile(0.5), 0.0, "opposite extreme quantile");
    require_close(online_extremes.mean(), 0.0, "opposite extreme online mean");
    require_close(online_extremes.median(), 0.0, "opposite extreme online median");
}


void
test_non_finite_policy()
{
    test_non_finite_samples<statistics>("batch statistics");
    test_non_finite_samples<online_statistics>("online statistics");

    statistics values;
    values.push(1.0);
    require_invalid_argument(
        [&values] { values.quantile(std::numeric_limits<double>::quiet_NaN()); },
        "quantile accepted NaN");
    require_invalid_argument(
        [&values] { values.quantile(std::numeric_limits<double>::infinity()); },
        "quantile accepted positive infinity");
    require_invalid_argument(
        [&values] { values.quantile(-std::numeric_limits<double>::infinity()); },
        "quantile accepted negative infinity");
}


void
test_bounded_history()
{
    statistics values(3);
    for (double value : {1.0, 2.0, 3.0, 4.0, 5.0})
        values.push(value);

    require(values.sample_limit() == 3, "bounded history limit");
    require(values.count() == 3, "bounded history count");
    require_close(values.mean(), 4.0, "bounded history mean");
    require_close(values.min(), 3.0, "bounded history minimum");
    require_close(values.max(), 5.0, "bounded history maximum");
}
}


namespace statistics_test
{
void
run_standalone_tests()
{
    test_empty_sets();
    test_reference_values();
    test_batch_online_agreement();
    test_constant_and_reset();
    test_extreme_finite_values();
    test_non_finite_policy();
    test_bounded_history();
}
}
