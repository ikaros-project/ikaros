#include "ikaros.h"

using namespace ikaros;

class Statistics: public Module
{
    parameter bins_;

    matrix input;
    matrix sample;

    matrix output;
    matrix count;
    matrix mean;
    matrix median;
    matrix mode;
    matrix min;
    matrix max;
    matrix variance;
    matrix standard_deviation;
    matrix skewness;
    matrix kurtosis;

    matrix q1;
    matrix q3;
    matrix interquartile_range;
    matrix lower_fence;
    matrix upper_fence;
    matrix lower_whisker;
    matrix upper_whisker;
    matrix box_plot;

    matrix histogram;
    matrix histogram_min;
    matrix histogram_max;
    matrix t_test_p;

    std::vector<statistics> statistics_;

    void Init()
    {
        Bind(bins_, "bins");

        Bind(input, "INPUT");
        Bind(sample, "SAMPLE");

        Bind(output, "OUTPUT");
        Bind(count, "COUNT");
        Bind(mean, "MEAN");
        Bind(median, "MEDIAN");
        Bind(mode, "MODE");
        Bind(min, "MIN");
        Bind(max, "MAX");
        Bind(variance, "VARIANCE");
        Bind(standard_deviation, "STANDARD_DEVIATION");
        Bind(skewness, "SKEWNESS");
        Bind(kurtosis, "KURTOSIS");

        Bind(q1, "Q1");
        Bind(q3, "Q3");
        Bind(interquartile_range, "INTERQUARTILE_RANGE");
        Bind(lower_fence, "LOWER_FENCE");
        Bind(upper_fence, "UPPER_FENCE");
        Bind(lower_whisker, "LOWER_WHISKER");
        Bind(upper_whisker, "UPPER_WHISKER");
        Bind(box_plot, "BOX_PLOT");

        Bind(histogram, "HISTOGRAM");
        Bind(histogram_min, "HISTOGRAM_MIN");
        Bind(histogram_max, "HISTOGRAM_MAX");
        Bind(t_test_p, "T_TEST_P");

        statistics_.resize(input.size());
    }

    void Tick()
    {
        if (statistics_.size() != static_cast<std::size_t>(input.size()))
            statistics_.resize(input.size());

        if (sample.connected() && sample.size() != input.size())
            throw exception("Statistics: SAMPLE must have the same size as INPUT.", path_);

        for (int i = 0; i < input.size(); ++i)
            if (!sample.connected() || sample.data()[i] >= 1.0f)
                statistics_[i].push(input.data()[i]);

        WriteStatistics();
        WriteHistogram();
        WriteTTestPValues();
    }

    void WriteStatistics()
    {
        for (int i = 0; i < input.size(); ++i)
        {
            statistics & s = statistics_[i];

            const float count_value = static_cast<float>(s.count());
            const float mean_value = static_cast<float>(s.mean());
            const float median_value = static_cast<float>(s.median());
            const float mode_value = static_cast<float>(s.mode());
            const float min_value = static_cast<float>(s.min());
            const float max_value = static_cast<float>(s.max());
            const float variance_value = static_cast<float>(s.variance());
            const float stddev_value = static_cast<float>(s.standard_deviation());
            const float skewness_value = static_cast<float>(s.skewness());
            const float kurtosis_value = static_cast<float>(s.kurtosis());

            count.data()[i] = count_value;
            mean.data()[i] = mean_value;
            median.data()[i] = median_value;
            mode.data()[i] = mode_value;
            min.data()[i] = min_value;
            max.data()[i] = max_value;
            variance.data()[i] = variance_value;
            standard_deviation.data()[i] = stddev_value;
            skewness.data()[i] = skewness_value;
            kurtosis.data()[i] = kurtosis_value;

            output(0, i) = count_value;
            output(1, i) = mean_value;
            output(2, i) = median_value;
            output(3, i) = mode_value;
            output(4, i) = min_value;
            output(5, i) = max_value;
            output(6, i) = variance_value;
            output(7, i) = stddev_value;
            output(8, i) = skewness_value;
            output(9, i) = kurtosis_value;

            q1.data()[i] = static_cast<float>(s.q1());
            q3.data()[i] = static_cast<float>(s.q3());
            interquartile_range.data()[i] = static_cast<float>(s.interquartile_range());
            lower_fence.data()[i] = static_cast<float>(s.lower_fence());
            upper_fence.data()[i] = static_cast<float>(s.upper_fence());
            lower_whisker.data()[i] = static_cast<float>(s.lower_whisker());
            upper_whisker.data()[i] = static_cast<float>(s.upper_whisker());

            box_plot(0, i) = lower_whisker.data()[i];
            box_plot(1, i) = q1.data()[i];
            box_plot(2, i) = median.data()[i];
            box_plot(3, i) = q3.data()[i];
            box_plot(4, i) = upper_whisker.data()[i];
        }
    }

    void WriteTTestPValues()
    {
        const int channel_count = input.size();
        for (int i = 0; i < channel_count; ++i)
            for (int j = 0; j < channel_count; ++j)
                t_test_p(i, j) = static_cast<float>(WelchTTestPValue(statistics_[i], statistics_[j]));
    }

    void WriteHistogram()
    {
        const int bin_count = std::max(1, bins_.as_int());
        if (histogram.cols() != bin_count)
            throw exception("Statistics: HISTOGRAM output size does not match bins parameter.", path_);

        histogram.set(0.0f);

        double global_min = std::numeric_limits<double>::quiet_NaN();
        double global_max = std::numeric_limits<double>::quiet_NaN();

        for (statistics & s : statistics_)
        {
            if (s.no_data())
                continue;

            const double channel_min = s.min();
            const double channel_max = s.max();

            if (std::isnan(global_min) || channel_min < global_min)
                global_min = channel_min;
            if (std::isnan(global_max) || channel_max > global_max)
                global_max = channel_max;
        }

        histogram_min[0] = static_cast<float>(global_min);
        histogram_max[0] = static_cast<float>(global_max);

        if (std::isnan(global_min) || std::isnan(global_max))
            return;

        const double range = global_max - global_min;
        for (std::size_t channel = 0; channel < statistics_.size(); ++channel)
        {
            for (double value : statistics_[channel].data())
            {
                int bin = 0;
                if (range > 0.0)
                    bin = static_cast<int>(std::floor(((value - global_min) / range) * bin_count));
                if (bin < 0)
                    bin = 0;
                if (bin >= bin_count)
                    bin = bin_count - 1;
                histogram(static_cast<int>(channel), bin) += 1.0f;
            }
        }
    }

    double WelchTTestPValue(const statistics & a, const statistics & b) const
    {
        if (&a == &b)
            return 1.0;

        const double n1 = static_cast<double>(a.count());
        const double n2 = static_cast<double>(b.count());
        if (n1 < 2.0 || n2 < 2.0)
            return std::numeric_limits<double>::quiet_NaN();

        const double mean1 = a.mean();
        const double mean2 = b.mean();
        const double var1 = a.variance();
        const double var2 = b.variance();
        if (!std::isfinite(mean1) || !std::isfinite(mean2) || !std::isfinite(var1) || !std::isfinite(var2))
            return std::numeric_limits<double>::quiet_NaN();

        const double sem1 = var1 / n1;
        const double sem2 = var2 / n2;
        const double denominator = std::sqrt(sem1 + sem2);
        if (denominator == 0.0)
            return mean1 == mean2 ? 1.0 : 0.0;

        const double t = std::fabs((mean1 - mean2) / denominator);
        const double df_numerator = (sem1 + sem2) * (sem1 + sem2);
        const double df_denominator = (sem1 * sem1) / (n1 - 1.0) + (sem2 * sem2) / (n2 - 1.0);
        if (df_denominator <= 0.0)
            return std::numeric_limits<double>::quiet_NaN();

        const double degrees_of_freedom = df_numerator / df_denominator;
        if (!std::isfinite(degrees_of_freedom) || degrees_of_freedom <= 0.0)
            return std::numeric_limits<double>::quiet_NaN();

        const double x = degrees_of_freedom / (degrees_of_freedom + t * t);
        const double p = RegularizedIncompleteBeta(x, degrees_of_freedom * 0.5, 0.5);
        return std::clamp(p, 0.0, 1.0);
    }

    double RegularizedIncompleteBeta(double x, double a, double b) const
    {
        if (x < 0.0 || x > 1.0 || a <= 0.0 || b <= 0.0)
            return std::numeric_limits<double>::quiet_NaN();
        if (x == 0.0)
            return 0.0;
        if (x == 1.0)
            return 1.0;

        const double front = std::exp(
            std::lgamma(a + b) - std::lgamma(a) - std::lgamma(b) +
            a * std::log(x) + b * std::log1p(-x)
        );

        if (x < (a + 1.0) / (a + b + 2.0))
            return front * IncompleteBetaContinuedFraction(a, b, x) / a;

        return 1.0 - front * IncompleteBetaContinuedFraction(b, a, 1.0 - x) / b;
    }

    double IncompleteBetaContinuedFraction(double a, double b, double x) const
    {
        constexpr int max_iterations = 200;
        constexpr double epsilon = 3.0e-14;
        constexpr double fpmin = std::numeric_limits<double>::min() / epsilon;

        double qab = a + b;
        double qap = a + 1.0;
        double qam = a - 1.0;
        double c = 1.0;
        double d = 1.0 - qab * x / qap;
        if (std::fabs(d) < fpmin)
            d = fpmin;
        d = 1.0 / d;
        double h = d;

        for (int m = 1; m <= max_iterations; ++m)
        {
            const int m2 = 2 * m;
            double aa = m * (b - m) * x / ((qam + m2) * (a + m2));
            d = 1.0 + aa * d;
            if (std::fabs(d) < fpmin)
                d = fpmin;
            c = 1.0 + aa / c;
            if (std::fabs(c) < fpmin)
                c = fpmin;
            d = 1.0 / d;
            h *= d * c;

            aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2));
            d = 1.0 + aa * d;
            if (std::fabs(d) < fpmin)
                d = fpmin;
            c = 1.0 + aa / c;
            if (std::fabs(c) < fpmin)
                c = fpmin;
            d = 1.0 / d;
            const double delta = d * c;
            h *= delta;

            if (std::fabs(delta - 1.0) < epsilon)
                break;
        }

        return h;
    }
};

INSTALL_CLASS(Statistics)
