#include "ikaros.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

using namespace ikaros;

class RegressionStatistics: public Module
{
    parameter max_samples_;
    parameter labels_;

    matrix x;
    matrix y;
    matrix sample;

    matrix scatter_x;
    matrix scatter_y;
    matrix sample_count;
    matrix linear_regression;
    matrix model_comparison;

    struct SamplePair
    {
        double x = 0.0;
        double y = 0.0;
    };

    std::vector<std::vector<SamplePair>> samples_;

    void Init()
    {
        Bind(max_samples_, "max_samples");
        Bind(labels_, "labels");

        Bind(x, "X");
        Bind(y, "Y");
        Bind(sample, "SAMPLE");

        Bind(scatter_x, "SCATTER_X");
        Bind(scatter_y, "SCATTER_Y");
        Bind(sample_count, "SAMPLE_COUNT");
        Bind(linear_regression, "LINEAR_REGRESSION");
        Bind(model_comparison, "MODEL_COMPARISON");

        samples_.resize(y.size());
        SetMatrixLabels();
    }

    void Tick()
    {
        ValidateInputs();

        if (samples_.size() != static_cast<std::size_t>(y.size()))
        {
            samples_.resize(y.size());
            SetMatrixLabels();
        }

        for (int channel = 0; channel < y.size(); ++channel)
        {
            if (sample.connected() && sample[channel] < 1.0f)
                continue;

            const int x_index = x.size() == 1 ? 0 : channel;
            AddSample(channel, x[x_index], y[channel]);
        }

        WriteScatterOutputs();
        WriteLinearRegression();
        WriteModelComparison();
    }

    void ValidateInputs() const
    {
        if (x.size() != 1 && x.size() != y.size())
            throw exception("RegressionStatistics: X must have size 1 or the same size as Y.", path_);

        if (sample.connected() && sample.size() != y.size())
            throw exception("RegressionStatistics: SAMPLE must have the same size as Y.", path_);

        if (max_samples_.as_int() < 1)
            throw exception("RegressionStatistics: max_samples must be at least 1.", path_);
    }

    void AddSample(int channel, double x_value, double y_value)
    {
        std::vector<SamplePair> & channel_samples = samples_[channel];
        const std::size_t capacity = static_cast<std::size_t>(std::max(1, max_samples_.as_int()));

        if (channel_samples.size() >= capacity)
            channel_samples.erase(channel_samples.begin());

        channel_samples.push_back({x_value, y_value});
    }

    void WriteScatterOutputs()
    {
        scatter_x.set(std::numeric_limits<float>::quiet_NaN());
        scatter_y.set(std::numeric_limits<float>::quiet_NaN());

        for (int channel = 0; channel < y.size(); ++channel)
        {
            const std::vector<SamplePair> & channel_samples = samples_[channel];
            sample_count[channel] = static_cast<float>(channel_samples.size());

            const int sample_count = std::min(scatter_x.rows(), static_cast<int>(channel_samples.size()));
            for (int sample_index = 0; sample_index < sample_count; ++sample_index)
            {
                scatter_x(sample_index, channel) = static_cast<float>(channel_samples[sample_index].x);
                scatter_y(sample_index, channel) = static_cast<float>(channel_samples[sample_index].y);
            }
        }
    }

    void WriteLinearRegression()
    {
        linear_regression.set(std::numeric_limits<float>::quiet_NaN());

        for (int channel = 0; channel < y.size(); ++channel)
        {
            const RegressionResult result = CalculateLinearRegression(samples_[channel]);
            linear_regression(0, channel) = static_cast<float>(result.slope);
            linear_regression(1, channel) = static_cast<float>(result.intercept);
            linear_regression(2, channel) = static_cast<float>(result.r);
            linear_regression(3, channel) = static_cast<float>(result.r_squared);
            linear_regression(4, channel) = static_cast<float>(result.p_value);
            linear_regression(5, channel) = static_cast<float>(result.slope_standard_error);
        }
    }

    struct RegressionResult
    {
        double slope = std::numeric_limits<double>::quiet_NaN();
        double intercept = std::numeric_limits<double>::quiet_NaN();
        double r = std::numeric_limits<double>::quiet_NaN();
        double r_squared = std::numeric_limits<double>::quiet_NaN();
        double p_value = std::numeric_limits<double>::quiet_NaN();
        double slope_standard_error = std::numeric_limits<double>::quiet_NaN();
    };

    struct GroupSample
    {
        double x = 0.0;
        double y = 0.0;
        int group = 0;
    };

    struct ModelFit
    {
        double sse = std::numeric_limits<double>::quiet_NaN();
        int samples = 0;
        int parameters = 0;
        bool valid = false;
    };

    void WriteModelComparison()
    {
        model_comparison.set(std::numeric_limits<float>::quiet_NaN());

        const std::vector<GroupSample> data = CompactGroupSamples(FiniteGroupSamples());
        const int group_count = ActiveGroupCount(data);
        const int total_samples = static_cast<int>(data.size());
        if (group_count < 2 || total_samples == 0)
            return;

        const ModelFit common_line = FitModel(data, group_count, false, false);
        const ModelFit different_intercepts = FitModel(data, group_count, true, false);
        const ModelFit different_slopes = FitModel(data, group_count, true, true);

        WriteModelComparisonColumn(0, common_line, different_intercepts, group_count, total_samples);
        WriteModelComparisonColumn(1, different_intercepts, different_slopes, group_count, total_samples);
    }

    std::vector<GroupSample> FiniteGroupSamples() const
    {
        std::vector<GroupSample> data;
        for (int group = 0; group < static_cast<int>(samples_.size()); ++group)
        {
            for (const SamplePair & sample_pair : samples_[group])
                if (std::isfinite(sample_pair.x) && std::isfinite(sample_pair.y))
                    data.push_back({sample_pair.x, sample_pair.y, group});
        }
        return data;
    }

    int ActiveGroupCount(const std::vector<GroupSample> & data) const
    {
        std::vector<bool> active(samples_.size(), false);
        int count = 0;
        for (const GroupSample & sample : data)
        {
            if (sample.group < 0 || sample.group >= static_cast<int>(active.size()) || active[sample.group])
                continue;
            active[sample.group] = true;
            ++count;
        }
        return count;
    }

    std::vector<GroupSample> CompactGroupSamples(const std::vector<GroupSample> & data) const
    {
        std::vector<int> group_map(samples_.size(), -1);
        int next_group = 0;
        for (const GroupSample & sample : data)
            if (sample.group >= 0 && sample.group < static_cast<int>(group_map.size()) && group_map[sample.group] < 0)
                group_map[sample.group] = next_group++;

        std::vector<GroupSample> compact;
        compact.reserve(data.size());
        for (const GroupSample & sample : data)
        {
            if (sample.group < 0 || sample.group >= static_cast<int>(group_map.size()) || group_map[sample.group] < 0)
                continue;
            compact.push_back({sample.x, sample.y, group_map[sample.group]});
        }
        return compact;
    }

    ModelFit FitModel(const std::vector<GroupSample> & data, int group_count, bool separate_intercepts, bool separate_slopes) const
    {
        ModelFit fit;
        fit.samples = static_cast<int>(data.size());

        if (group_count < 1 || fit.samples == 0)
            return fit;

        fit.parameters = ModelParameterCount(group_count, separate_intercepts, separate_slopes);
        if (fit.samples <= fit.parameters)
            return fit;

        std::vector<std::vector<double>> xtx(fit.parameters, std::vector<double>(fit.parameters, 0.0));
        std::vector<double> xty(fit.parameters, 0.0);

        for (const GroupSample & sample : data)
        {
            const std::vector<double> row = DesignRow(sample, group_count, separate_intercepts, separate_slopes);
            for (int i = 0; i < fit.parameters; ++i)
            {
                xty[i] += row[i] * sample.y;
                for (int j = 0; j < fit.parameters; ++j)
                    xtx[i][j] += row[i] * row[j];
            }
        }

        std::vector<double> beta;
        if (!SolveLinearSystem(xtx, xty, beta))
            return fit;

        fit.sse = 0.0;
        for (const GroupSample & sample : data)
        {
            const std::vector<double> row = DesignRow(sample, group_count, separate_intercepts, separate_slopes);
            double predicted = 0.0;
            for (int i = 0; i < fit.parameters; ++i)
                predicted += row[i] * beta[i];

            const double residual = sample.y - predicted;
            fit.sse += residual * residual;
        }

        fit.valid = std::isfinite(fit.sse);
        return fit;
    }

    int ModelParameterCount(int group_count, bool separate_intercepts, bool separate_slopes) const
    {
        if (separate_slopes)
            return 2 * group_count;
        if (separate_intercepts)
            return group_count + 1;
        return 2;
    }

    std::vector<double> DesignRow(const GroupSample & sample, int group_count, bool separate_intercepts, bool separate_slopes) const
    {
        std::vector<double> row(ModelParameterCount(group_count, separate_intercepts, separate_slopes), 0.0);

        if (separate_slopes)
        {
            row[sample.group] = 1.0;
            row[group_count + sample.group] = sample.x;
            return row;
        }

        if (separate_intercepts)
        {
            row[sample.group] = 1.0;
            row[group_count] = sample.x;
            return row;
        }

        row[0] = 1.0;
        row[1] = sample.x;
        return row;
    }

    bool SolveLinearSystem(std::vector<std::vector<double>> a, std::vector<double> b, std::vector<double> & x) const
    {
        const int n = static_cast<int>(b.size());
        if (n == 0 || static_cast<int>(a.size()) != n)
            return false;

        for (int col = 0; col < n; ++col)
        {
            int pivot = col;
            for (int row = col + 1; row < n; ++row)
                if (std::fabs(a[row][col]) > std::fabs(a[pivot][col]))
                    pivot = row;

            if (std::fabs(a[pivot][col]) < 1.0e-12)
                return false;

            if (pivot != col)
            {
                std::swap(a[pivot], a[col]);
                std::swap(b[pivot], b[col]);
            }

            const double divisor = a[col][col];
            for (int j = col; j < n; ++j)
                a[col][j] /= divisor;
            b[col] /= divisor;

            for (int row = 0; row < n; ++row)
            {
                if (row == col)
                    continue;

                const double factor = a[row][col];
                if (factor == 0.0)
                    continue;

                for (int j = col; j < n; ++j)
                    a[row][j] -= factor * a[col][j];
                b[row] -= factor * b[col];
            }
        }

        x = std::move(b);
        return true;
    }

    void WriteModelComparisonColumn(int column, const ModelFit & reduced, const ModelFit & full, int group_count, int total_samples)
    {
        if (!reduced.valid || !full.valid)
            return;

        const double df1 = static_cast<double>(full.parameters - reduced.parameters);
        const double df2 = static_cast<double>(full.samples - full.parameters);
        if (df1 <= 0.0 || df2 <= 0.0)
            return;

        const double ss_effect = std::max(0.0, reduced.sse - full.sse);
        const double denominator = full.sse / df2;
        const double f = denominator > 0.0 ? (ss_effect / df1) / denominator : (ss_effect == 0.0 ? 0.0 : std::numeric_limits<double>::infinity());
        const double p = FisherFUpperTail(f, df1, df2);
        const double effect_size = (ss_effect + full.sse) > 0.0 ? ss_effect / (ss_effect + full.sse) : 0.0;

        model_comparison(0, column) = static_cast<float>(p);
        model_comparison(1, column) = static_cast<float>(f);
        model_comparison(2, column) = static_cast<float>(df1);
        model_comparison(3, column) = static_cast<float>(df2);
        model_comparison(4, column) = static_cast<float>(effect_size);
        model_comparison(5, column) = static_cast<float>(group_count);
        model_comparison(6, column) = static_cast<float>(total_samples);
    }

    RegressionResult CalculateLinearRegression(const std::vector<SamplePair> & channel_samples) const
    {
        RegressionResult result;

        std::vector<SamplePair> finite_samples;
        finite_samples.reserve(channel_samples.size());
        for (const SamplePair & sample_pair : channel_samples)
            if (std::isfinite(sample_pair.x) && std::isfinite(sample_pair.y))
                finite_samples.push_back(sample_pair);

        const int n = static_cast<int>(finite_samples.size());
        if (n < 2)
            return result;

        double sum_x = 0.0;
        double sum_y = 0.0;
        for (const SamplePair & sample_pair : finite_samples)
        {
            sum_x += sample_pair.x;
            sum_y += sample_pair.y;
        }

        const double mean_x = sum_x / static_cast<double>(n);
        const double mean_y = sum_y / static_cast<double>(n);

        double sxx = 0.0;
        double syy = 0.0;
        double sxy = 0.0;
        for (const SamplePair & sample_pair : finite_samples)
        {
            const double dx = sample_pair.x - mean_x;
            const double dy = sample_pair.y - mean_y;
            sxx += dx * dx;
            syy += dy * dy;
            sxy += dx * dy;
        }

        if (sxx <= 0.0)
            return result;

        result.slope = sxy / sxx;
        result.intercept = mean_y - result.slope * mean_x;
        result.r = syy > 0.0 ? sxy / std::sqrt(sxx * syy) : std::numeric_limits<double>::quiet_NaN();
        result.r_squared = std::isfinite(result.r) ? result.r * result.r : std::numeric_limits<double>::quiet_NaN();

        double sse = 0.0;
        for (const SamplePair & sample_pair : finite_samples)
        {
            const double residual = sample_pair.y - (result.intercept + result.slope * sample_pair.x);
            sse += residual * residual;
        }

        if (n > 2)
        {
            const double df = static_cast<double>(n - 2);
            const double mean_squared_error = sse / df;
            result.slope_standard_error = std::sqrt(mean_squared_error / sxx);
            if (syy > 0.0 && std::isfinite(result.r_squared) && result.r_squared < 1.0)
            {
                const double t = std::fabs(result.r) * std::sqrt(df / (1.0 - result.r_squared));
                result.p_value = StudentTTwoSidedPValue(t, df);
            }
            else if (syy == 0.0)
                result.p_value = 1.0;
            else
                result.p_value = 0.0;
        }

        return result;
    }

    double StudentTTwoSidedPValue(double t, double degrees_of_freedom) const
    {
        if (!std::isfinite(t) || degrees_of_freedom <= 0.0)
            return std::numeric_limits<double>::quiet_NaN();

        const double x = degrees_of_freedom / (degrees_of_freedom + t * t);
        const double p = RegularizedIncompleteBeta(x, degrees_of_freedom * 0.5, 0.5);
        return std::clamp(p, 0.0, 1.0);
    }

    double FisherFUpperTail(double f, double df1, double df2) const
    {
        if (!std::isfinite(f) || df1 <= 0.0 || df2 <= 0.0)
            return f == std::numeric_limits<double>::infinity() ? 0.0 : std::numeric_limits<double>::quiet_NaN();
        if (f <= 0.0)
            return 1.0;

        const double x = df2 / (df2 + df1 * f);
        const double p = RegularizedIncompleteBeta(x, df2 * 0.5, df1 * 0.5);
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

    void SetMatrixLabels()
    {
        SetIndexLabels(scatter_x, 0, "sample");
        SetChannelLabels(scatter_x, 1);

        SetIndexLabels(scatter_y, 0, "sample");
        SetChannelLabels(scatter_y, 1);

        SetChannelLabels(sample_count, 0);

        SetLabels(linear_regression, 0, {
            "slope", "intercept", "r", "r-squared", "p-value", "slope standard error"
        });
        SetChannelLabels(linear_regression, 1);

        SetLabels(model_comparison, 0, {
            "p-value", "F", "df1", "df2", "effect size", "groups", "samples"
        });
        SetLabels(model_comparison, 1, {
            "intercept difference", "slope difference"
        });
    }

    void SetLabels(matrix & target, int dimension, std::initializer_list<std::string> labels)
    {
        target.clear_labels(dimension);
        for (const std::string & label : labels)
            target.push_label(dimension, label);
    }

    void SetChannelLabels(matrix & target, int dimension)
    {
        const std::vector<std::string> labels = ChannelLabels();

        target.clear_labels(dimension);
        for (int i = 0; i < y.size(); ++i)
            target.push_label(dimension, labels[i]);
    }

    std::vector<std::string> ChannelLabels() const
    {
        std::vector<std::string> labels(y.size());
        for (int i = 0; i < y.size(); ++i)
            labels[i] = "channel " + std::to_string(i);

        const std::string label_string = labels_.as_string();
        if (trim(label_string).empty())
            return labels;

        const std::vector<std::string> configured_labels = split(label_string, ",");
        for (std::size_t i = 0; i < configured_labels.size() && i < labels.size(); ++i)
        {
            const std::string label = trim(configured_labels[i]);
            if (!label.empty())
                labels[i] = label;
        }

        return labels;
    }

    void SetIndexLabels(matrix & target, int dimension, const std::string & prefix)
    {
        target.clear_labels(dimension);
        for (int i = 0; i < target.shape(dimension); ++i)
            target.push_label(dimension, prefix + " " + std::to_string(i));
    }
};

INSTALL_CLASS(RegressionStatistics)
