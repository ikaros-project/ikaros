#include "ikaros.h"

using namespace ikaros;

class Statistics: public Module
{
    parameter bins_;
    parameter max_outliers_;
    parameter group_test_;
    parameter labels_;
    parameter normality_test_;
    parameter pairwise_test_;
    parameter p_adjustment_;

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
    matrix box_plot;
    matrix box_plot_outliers;

    matrix histogram;
    matrix histogram_min;
    matrix histogram_max;
    matrix group_test;
    matrix normality_test;
    matrix pairwise_test_p;
    matrix pairwise_test_p_adjusted;

    std::vector<statistics> statistics_;

    struct PairwisePValue
    {
        int i = 0;
        int j = 0;
        double p = std::numeric_limits<double>::quiet_NaN();
        double adjusted = std::numeric_limits<double>::quiet_NaN();
    };

    void Init()
    {
        Bind(bins_, "bins");
        Bind(max_outliers_, "max_outliers");
        Bind(group_test_, "group_test");
        Bind(labels_, "labels");
        Bind(normality_test_, "normality_test");
        Bind(pairwise_test_, "pairwise_test");
        Bind(p_adjustment_, "p_adjustment");

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
        Bind(box_plot, "BOX_PLOT");
        Bind(box_plot_outliers, "BOX_PLOT_OUTLIERS");

        Bind(histogram, "HISTOGRAM");
        Bind(histogram_min, "HISTOGRAM_MIN");
        Bind(histogram_max, "HISTOGRAM_MAX");
        Bind(group_test, "GROUP_TEST");
        Bind(normality_test, "NORMALITY_TEST");
        Bind(pairwise_test_p, "PAIRWISE_TEST_P");
        Bind(pairwise_test_p_adjusted, "PAIRWISE_TEST_P_ADJUSTED");

        statistics_.resize(input.size());
        SetMatrixLabels();
    }

    void SetMatrixLabels()
    {
        SetLabels(output, 0, {
            "count", "mean", "median", "mode", "min", "max", "variance", "standard deviation", "skewness", "kurtosis"
        });
        SetChannelLabels(output, 1);

        SetLabels(box_plot, 0, {
            "lower fence", "lower whisker", "Q1", "median", "Q3", "upper whisker", "upper fence"
        });
        SetChannelLabels(box_plot, 1);

        SetIndexLabels(box_plot_outliers, 0, "outlier");
        SetChannelLabels(box_plot_outliers, 1);

        SetChannelLabels(histogram, 0);
        SetIndexLabels(histogram, 1, "bin");

        SetLabels(group_test, 0, {
            "p-value", "statistic", "df1", "df2", "effect size", "groups"
        });
        group_test.clear_labels(1);
        group_test.push_label(1, "value");

        SetLabels(normality_test, 0, {
            "statistic", "p-value", "mean", "standard deviation"
        });
        SetChannelLabels(normality_test, 1);

        SetChannelLabels(pairwise_test_p, 0);
        SetChannelLabels(pairwise_test_p, 1);
        SetChannelLabels(pairwise_test_p_adjusted, 0);
        SetChannelLabels(pairwise_test_p_adjusted, 1);

        for (matrix * vector_output : {
            &count, &mean, &median, &mode, &min, &max, &variance, &standard_deviation,
            &skewness, &kurtosis, &q1, &q3, &interquartile_range
        })
            SetChannelLabels(*vector_output, 0);
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
        for (int i = 0; i < input.size(); ++i)
            target.push_label(dimension, labels[i]);
    }

    std::vector<std::string> ChannelLabels() const
    {
        std::vector<std::string> labels(input.size());
        for (int i = 0; i < input.size(); ++i)
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
        WriteGroupTest();
        WriteNormalityTest();
        WritePairwiseTestPValues();
    }

    void WriteStatistics()
    {
        box_plot_outliers.set(std::numeric_limits<float>::quiet_NaN());

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
            const float lower_fence = static_cast<float>(s.lower_fence());
            const float upper_fence = static_cast<float>(s.upper_fence());
            const float lower_whisker = static_cast<float>(s.lower_whisker());
            const float upper_whisker = static_cast<float>(s.upper_whisker());

            box_plot(0, i) = lower_fence;
            box_plot(1, i) = lower_whisker;
            box_plot(2, i) = q1.data()[i];
            box_plot(3, i) = median.data()[i];
            box_plot(4, i) = q3.data()[i];
            box_plot(5, i) = upper_whisker;
            box_plot(6, i) = upper_fence;

            WriteBoxPlotOutliers(i, s, lower_fence, upper_fence);
        }
    }

    void WriteBoxPlotOutliers(int channel, const statistics & s, float lower, float upper)
    {
        if (!std::isfinite(lower) || !std::isfinite(upper))
            return;

        int outlier_index = 0;
        const int max_outliers = box_plot_outliers.rows();
        for (double value : s.data())
        {
            if (value >= lower && value <= upper)
                continue;

            if (outlier_index >= max_outliers)
                break;

            box_plot_outliers(outlier_index, channel) = static_cast<float>(value);
            ++outlier_index;
        }
    }

    void WritePairwiseTestPValues()
    {
        const int channel_count = input.size();
        const std::string test = pairwise_test_.as_string();

        for (int i = 0; i < channel_count; ++i)
            for (int j = 0; j < channel_count; ++j)
                pairwise_test_p(i, j) = static_cast<float>(PairwiseTestPValue(statistics_[i], statistics_[j], test));

        WriteAdjustedPairwiseTestPValues();
    }

    double PairwiseTestPValue(const statistics & a, const statistics & b, const std::string & test) const
    {
        if (test == "parametric")
            return WelchTTestPValue(a, b);
        if (test == "non_parametric")
            return MannWhitneyUTestPValue(a, b);

        throw exception("Statistics: unknown pairwise_test method \"" + test + "\".", path_);
    }

    void WriteAdjustedPairwiseTestPValues()
    {
        const int channel_count = input.size();
        for (int i = 0; i < channel_count; ++i)
            for (int j = 0; j < channel_count; ++j)
                pairwise_test_p_adjusted(i, j) = pairwise_test_p(i, j);

        std::vector<PairwisePValue> pairs;
        for (int i = 0; i < channel_count; ++i)
        {
            pairwise_test_p_adjusted(i, i) = 1.0f;
            for (int j = i + 1; j < channel_count; ++j)
            {
                const double p = pairwise_test_p(i, j);
                if (std::isfinite(p))
                    pairs.push_back({i, j, p, p});
            }
        }

        const std::string method = p_adjustment_.as_string();
        if (method == "none")
        {
            // Raw values were copied above.
        }
        else if (method == "bonferroni")
            ApplyBonferroniAdjustment(pairs);
        else if (method == "holm")
            ApplyHolmAdjustment(pairs);
        else if (method == "bh")
            ApplyBenjaminiHochbergAdjustment(pairs);
        else
            throw exception("Statistics: unknown p_adjustment method \"" + method + "\".", path_);

        for (const PairwisePValue & pair : pairs)
        {
            const float adjusted = static_cast<float>(pair.adjusted);
            pairwise_test_p_adjusted(pair.i, pair.j) = adjusted;
            pairwise_test_p_adjusted(pair.j, pair.i) = adjusted;
        }
    }

    void ApplyBonferroniAdjustment(std::vector<PairwisePValue> & pairs) const
    {
        const double comparisons = static_cast<double>(pairs.size());
        for (PairwisePValue & pair : pairs)
            pair.adjusted = std::clamp(pair.p * comparisons, 0.0, 1.0);
    }

    void ApplyHolmAdjustment(std::vector<PairwisePValue> & pairs) const
    {
        std::vector<int> order = SortedPairOrder(pairs);
        double previous_adjusted = 0.0;

        for (std::size_t rank = 0; rank < order.size(); ++rank)
        {
            PairwisePValue & pair = pairs[order[rank]];
            const double remaining = static_cast<double>(order.size() - rank);
            pair.adjusted = std::max(previous_adjusted, std::clamp(pair.p * remaining, 0.0, 1.0));
            previous_adjusted = pair.adjusted;
        }
    }

    void ApplyBenjaminiHochbergAdjustment(std::vector<PairwisePValue> & pairs) const
    {
        std::vector<int> order = SortedPairOrder(pairs);
        double next_adjusted = 1.0;

        for (int rank = static_cast<int>(order.size()) - 1; rank >= 0; --rank)
        {
            PairwisePValue & pair = pairs[order[rank]];
            const double one_based_rank = static_cast<double>(rank + 1);
            const double adjusted = std::clamp(pair.p * static_cast<double>(order.size()) / one_based_rank, 0.0, 1.0);
            pair.adjusted = std::min(adjusted, next_adjusted);
            next_adjusted = pair.adjusted;
        }
    }

    std::vector<int> SortedPairOrder(const std::vector<PairwisePValue> & pairs) const
    {
        std::vector<int> order(pairs.size());
        for (std::size_t i = 0; i < order.size(); ++i)
            order[i] = static_cast<int>(i);

        std::sort(order.begin(), order.end(), [&pairs](int a, int b) {
            return pairs[a].p < pairs[b].p;
        });

        return order;
    }

    void WriteGroupTest()
    {
        const std::string test = group_test_.as_string();
        group_test.set(std::numeric_limits<float>::quiet_NaN());

        if (test == "parametric")
            WriteAnovaGroupTest();
        else if (test == "non_parametric")
            WriteKruskalWallisGroupTest();
        else
            throw exception("Statistics: unknown group_test method \"" + test + "\".", path_);
    }

    std::vector<std::vector<double>> FiniteSampleGroups() const
    {
        std::vector<std::vector<double>> groups;
        for (const statistics & s : statistics_)
        {
            std::vector<double> group;
            for (double value : s.data())
                if (std::isfinite(value))
                    group.push_back(value);

            if (!group.empty())
                groups.push_back(std::move(group));
        }

        return groups;
    }

    void WriteAnovaGroupTest()
    {
        const std::vector<std::vector<double>> groups = FiniteSampleGroups();
        const int group_count = static_cast<int>(groups.size());
        if (group_count < 2)
            return;

        int total_count = 0;
        double total_sum = 0.0;
        for (const auto & group : groups)
        {
            total_count += static_cast<int>(group.size());
            for (double value : group)
                total_sum += value;
        }

        if (total_count <= group_count)
            return;

        const double grand_mean = total_sum / static_cast<double>(total_count);
        double ss_between = 0.0;
        double ss_within = 0.0;

        for (const auto & group : groups)
        {
            double group_sum = 0.0;
            for (double value : group)
                group_sum += value;

            const double group_mean = group_sum / static_cast<double>(group.size());
            ss_between += static_cast<double>(group.size()) * (group_mean - grand_mean) * (group_mean - grand_mean);

            for (double value : group)
                ss_within += (value - group_mean) * (value - group_mean);
        }

        const double df_between = static_cast<double>(group_count - 1);
        const double df_within = static_cast<double>(total_count - group_count);
        if (df_between <= 0.0 || df_within <= 0.0)
            return;

        const double ms_between = ss_between / df_between;
        const double ms_within = ss_within / df_within;
        const double statistic = ms_within == 0.0 ? (ms_between == 0.0 ? 0.0 : std::numeric_limits<double>::infinity()) : ms_between / ms_within;
        const double p = FisherFUpperTail(statistic, df_between, df_within);
        const double ss_total = ss_between + ss_within;
        const double effect_size = ss_total > 0.0 ? ss_between / ss_total : 0.0;

        WriteGroupTestResult(p, statistic, df_between, df_within, effect_size, group_count);
    }

    void WriteKruskalWallisGroupTest()
    {
        const std::vector<std::vector<double>> groups = FiniteSampleGroups();
        const int group_count = static_cast<int>(groups.size());
        if (group_count < 2)
            return;

        struct RankedGroupValue
        {
            double value = 0.0;
            int group = 0;
        };

        std::vector<RankedGroupValue> values;
        std::vector<int> counts(group_count, 0);
        for (int group = 0; group < group_count; ++group)
        {
            counts[group] = static_cast<int>(groups[group].size());
            for (double value : groups[group])
                values.push_back({value, group});
        }

        const int total_count = static_cast<int>(values.size());
        if (total_count <= group_count)
            return;

        std::sort(values.begin(), values.end(), [](const RankedGroupValue & a, const RankedGroupValue & b) {
            return a.value < b.value;
        });

        std::vector<double> rank_sums(group_count, 0.0);
        double tie_sum = 0.0;
        for (int i = 0; i < total_count;)
        {
            int j = i + 1;
            while (j < total_count && values[j].value == values[i].value)
                ++j;

            const double average_rank = (static_cast<double>(i + 1) + static_cast<double>(j)) * 0.5;
            for (int k = i; k < j; ++k)
                rank_sums[values[k].group] += average_rank;

            const double tie_count = static_cast<double>(j - i);
            if (tie_count > 1.0)
                tie_sum += tie_count * tie_count * tie_count - tie_count;

            i = j;
        }

        const double n = static_cast<double>(total_count);
        double h = 0.0;
        for (int group = 0; group < group_count; ++group)
            h += rank_sums[group] * rank_sums[group] / static_cast<double>(counts[group]);
        h = 12.0 / (n * (n + 1.0)) * h - 3.0 * (n + 1.0);

        const double tie_correction = 1.0 - tie_sum / (n * n * n - n);
        if (tie_correction > 0.0)
            h /= tie_correction;

        const double df = static_cast<double>(group_count - 1);
        const double p = ChiSquareUpperTail(h, df);
        const double effect_size = total_count > group_count ? std::clamp((h - static_cast<double>(group_count) + 1.0) / (n - static_cast<double>(group_count)), 0.0, 1.0) : 0.0;

        WriteGroupTestResult(p, h, df, std::numeric_limits<double>::quiet_NaN(), effect_size, group_count);
    }

    void WriteGroupTestResult(double p, double statistic, double df1, double df2, double effect_size, int group_count)
    {
        group_test(0, 0) = static_cast<float>(p);
        group_test(1, 0) = static_cast<float>(statistic);
        group_test(2, 0) = static_cast<float>(df1);
        group_test(3, 0) = static_cast<float>(df2);
        group_test(4, 0) = static_cast<float>(effect_size);
        group_test(5, 0) = static_cast<float>(group_count);
    }

    void WriteNormalityTest()
    {
        const std::string test = normality_test_.as_string();
        normality_test.set(std::numeric_limits<float>::quiet_NaN());

        if (test != "anderson_darling")
            throw exception("Statistics: unknown normality_test method \"" + test + "\".", path_);

        for (int channel = 0; channel < input.size(); ++channel)
            WriteAndersonDarlingNormalityTest(channel, statistics_[channel]);
    }

    void WriteAndersonDarlingNormalityTest(int channel, const statistics & s)
    {
        std::vector<double> values;
        for (double value : s.data())
            if (std::isfinite(value))
                values.push_back(value);

        const int n = static_cast<int>(values.size());
        if (n < 3)
            return;

        const double mean_value = s.mean();
        const double stddev_value = s.standard_deviation();
        if (!std::isfinite(mean_value) || !std::isfinite(stddev_value) || stddev_value <= 0.0)
            return;

        std::sort(values.begin(), values.end());

        double sum = 0.0;
        for (int i = 0; i < n; ++i)
        {
            const double lower_cdf = ClampedStandardNormalCDF((values[i] - mean_value) / stddev_value);
            const double upper_cdf = ClampedStandardNormalCDF((values[n - 1 - i] - mean_value) / stddev_value);
            sum += static_cast<double>(2 * i + 1) * (std::log(lower_cdf) + std::log(1.0 - upper_cdf));
        }

        const double a2 = -static_cast<double>(n) - sum / static_cast<double>(n);
        const double adjusted_a2 = a2 * (1.0 + 0.75 / static_cast<double>(n) + 2.25 / (static_cast<double>(n) * static_cast<double>(n)));
        const double p = AndersonDarlingNormalityPValue(adjusted_a2);

        normality_test(0, channel) = static_cast<float>(adjusted_a2);
        normality_test(1, channel) = static_cast<float>(p);
        normality_test(2, channel) = static_cast<float>(mean_value);
        normality_test(3, channel) = static_cast<float>(stddev_value);
    }

    double ClampedStandardNormalCDF(double z) const
    {
        const double cdf = 0.5 * std::erfc(-z / std::sqrt(2.0));
        constexpr double epsilon = 1.0e-15;
        return std::clamp(cdf, epsilon, 1.0 - epsilon);
    }

    double AndersonDarlingNormalityPValue(double adjusted_a2) const
    {
        if (!std::isfinite(adjusted_a2))
            return std::numeric_limits<double>::quiet_NaN();
        if (adjusted_a2 < 0.0)
            return 1.0;

        double p;
        if (adjusted_a2 < 0.2)
            p = 1.0 - std::exp(-13.436 + 101.14 * adjusted_a2 - 223.73 * adjusted_a2 * adjusted_a2);
        else if (adjusted_a2 < 0.34)
            p = 1.0 - std::exp(-8.318 + 42.796 * adjusted_a2 - 59.938 * adjusted_a2 * adjusted_a2);
        else if (adjusted_a2 < 0.6)
            p = std::exp(0.9177 - 4.279 * adjusted_a2 - 1.38 * adjusted_a2 * adjusted_a2);
        else
            p = std::exp(1.2937 - 5.709 * adjusted_a2 + 0.0186 * adjusted_a2 * adjusted_a2);

        return std::clamp(p, 0.0, 1.0);
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

    double MannWhitneyUTestPValue(const statistics & a, const statistics & b) const
    {
        if (&a == &b)
            return 1.0;

        const int n1 = a.count();
        const int n2 = b.count();
        if (n1 < 1 || n2 < 1)
            return std::numeric_limits<double>::quiet_NaN();

        struct RankedValue
        {
            double value = 0.0;
            int group = 0;
        };

        std::vector<RankedValue> values;
        values.reserve(static_cast<std::size_t>(n1 + n2));

        for (double value : a.data())
            if (std::isfinite(value))
                values.push_back({value, 0});
        for (double value : b.data())
            if (std::isfinite(value))
                values.push_back({value, 1});

        if (static_cast<int>(values.size()) != n1 + n2)
            return std::numeric_limits<double>::quiet_NaN();

        std::sort(values.begin(), values.end(), [](const RankedValue & x, const RankedValue & y) {
            return x.value < y.value;
        });

        const int n = static_cast<int>(values.size());
        double rank_sum_a = 0.0;
        double tie_sum = 0.0;

        for (int i = 0; i < n;)
        {
            int j = i + 1;
            while (j < n && values[j].value == values[i].value)
                ++j;

            const double average_rank = (static_cast<double>(i + 1) + static_cast<double>(j)) * 0.5;
            for (int k = i; k < j; ++k)
                if (values[k].group == 0)
                    rank_sum_a += average_rank;

            const double tie_count = static_cast<double>(j - i);
            if (tie_count > 1.0)
                tie_sum += tie_count * tie_count * tie_count - tie_count;

            i = j;
        }

        const double dn1 = static_cast<double>(n1);
        const double dn2 = static_cast<double>(n2);
        const double total = dn1 + dn2;
        const double u = rank_sum_a - dn1 * (dn1 + 1.0) * 0.5;
        const double mean_u = dn1 * dn2 * 0.5;

        if (total <= 1.0)
            return std::numeric_limits<double>::quiet_NaN();

        const double variance_u = dn1 * dn2 / 12.0 * ((total + 1.0) - tie_sum / (total * (total - 1.0)));
        if (!std::isfinite(variance_u) || variance_u <= 0.0)
            return u == mean_u ? 1.0 : 0.0;

        const double z = std::max(0.0, std::fabs(u - mean_u) - 0.5) / std::sqrt(variance_u);
        const double p = std::erfc(z / std::sqrt(2.0));
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

    double ChiSquareUpperTail(double statistic, double degrees_of_freedom) const
    {
        if (!std::isfinite(statistic) || degrees_of_freedom <= 0.0)
            return std::numeric_limits<double>::quiet_NaN();
        if (statistic <= 0.0)
            return 1.0;

        return RegularizedUpperIncompleteGamma(degrees_of_freedom * 0.5, statistic * 0.5);
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

    double RegularizedUpperIncompleteGamma(double a, double x) const
    {
        if (a <= 0.0 || x < 0.0)
            return std::numeric_limits<double>::quiet_NaN();
        if (x == 0.0)
            return 1.0;

        if (x < a + 1.0)
            return std::clamp(1.0 - RegularizedLowerIncompleteGammaSeries(a, x), 0.0, 1.0);

        return std::clamp(RegularizedUpperIncompleteGammaFraction(a, x), 0.0, 1.0);
    }

    double RegularizedLowerIncompleteGammaSeries(double a, double x) const
    {
        constexpr int max_iterations = 200;
        constexpr double epsilon = 3.0e-14;

        double sum = 1.0 / a;
        double delta = sum;
        double ap = a;

        for (int n = 1; n <= max_iterations; ++n)
        {
            ap += 1.0;
            delta *= x / ap;
            sum += delta;
            if (std::fabs(delta) < std::fabs(sum) * epsilon)
                break;
        }

        return sum * std::exp(-x + a * std::log(x) - std::lgamma(a));
    }

    double RegularizedUpperIncompleteGammaFraction(double a, double x) const
    {
        constexpr int max_iterations = 200;
        constexpr double epsilon = 3.0e-14;
        constexpr double fpmin = std::numeric_limits<double>::min() / epsilon;

        double b = x + 1.0 - a;
        double c = 1.0 / fpmin;
        double d = 1.0 / std::max(std::fabs(b), fpmin);
        if (std::fabs(b) < fpmin)
            b = fpmin;
        double h = d;

        for (int i = 1; i <= max_iterations; ++i)
        {
            const double an = -static_cast<double>(i) * (static_cast<double>(i) - a);
            b += 2.0;
            d = an * d + b;
            if (std::fabs(d) < fpmin)
                d = fpmin;
            c = b + an / c;
            if (std::fabs(c) < fpmin)
                c = fpmin;
            d = 1.0 / d;
            const double delta = d * c;
            h *= delta;
            if (std::fabs(delta - 1.0) < epsilon)
                break;
        }

        return std::exp(-x + a * std::log(x) - std::lgamma(a)) * h;
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
