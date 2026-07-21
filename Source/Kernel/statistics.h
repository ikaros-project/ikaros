#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <vector>



class statistics 
{
public:

    struct order_summary
    {
        double lower_fence = std::numeric_limits<double>::quiet_NaN();
        double lower_whisker = std::numeric_limits<double>::quiet_NaN();
        double q1 = std::numeric_limits<double>::quiet_NaN();
        double median = std::numeric_limits<double>::quiet_NaN();
        double q3 = std::numeric_limits<double>::quiet_NaN();
        double upper_whisker = std::numeric_limits<double>::quiet_NaN();
        double upper_fence = std::numeric_limits<double>::quiet_NaN();
        double interquartile_range = std::numeric_limits<double>::quiet_NaN();
    };

    explicit statistics(std::size_t sample_limit = 0)
        : sample_limit_(sample_limit)
    {
    }

    void reset() noexcept 
    {
        data_.clear();
        next_replacement_ = 0;
    }

    void push(double x) 
    {
        if (!std::isfinite(x))
            throw std::invalid_argument("statistics requires finite samples");

        if (sample_limit_ == 0 || data_.size() < sample_limit_)
        {
            data_.push_back(x);
            return;
        }

        data_[next_replacement_] = x;
        if (++next_replacement_ == sample_limit_)
            next_replacement_ = 0;
    }

    std::size_t count() const noexcept 
    { 
        return data_.size(); 
    }

    bool no_data() const noexcept 
    { 
        return data_.empty(); 
    }

    std::size_t sample_limit() const noexcept
    {
        return sample_limit_;
    }


    double mean() const noexcept 
    {
        return static_cast<double>(mean_ld_());
    }

    double median() const
    {
        return summarize_order().median;
    }

    double quantile(double q) const
    {
        if (!std::isfinite(q))
            throw std::invalid_argument("statistics quantile requires a finite probability");

        const std::size_t n = data_.size();
        if (n == 0)
            return nan_();

        if (q <= 0.0)
            return min();
        if (q >= 1.0)
            return max();

        std::vector<double> v = data_;
        std::sort(v.begin(), v.end());
        return quantile_sorted_(v, q);
    }

    double q1() const
    {
        return summarize_order().q1;
    }

    double q3() const
    {
        return summarize_order().q3;
    }

    double interquartile_range() const
    {
        return summarize_order().interquartile_range;
    }

    double lower_fence() const
    {
        return summarize_order().lower_fence;
    }

    double upper_fence() const
    {
        return summarize_order().upper_fence;
    }

    double lower_whisker() const
    {
        return summarize_order().lower_whisker;
    }

    double upper_whisker() const
    {
        return summarize_order().upper_whisker;
    }

    order_summary summarize_order() const
    {
        if (data_.empty())
            return {};

        std::vector<double> v = data_;
        std::sort(v.begin(), v.end());

        order_summary result;
        result.q1 = quantile_sorted_(v, 0.25);
        result.median = quantile_sorted_(v, 0.5);
        result.q3 = quantile_sorted_(v, 0.75);
        result.interquartile_range = result.q3 - result.q1;
        result.lower_fence = result.q1 - 1.5 * result.interquartile_range;
        result.upper_fence = result.q3 + 1.5 * result.interquartile_range;

        const auto lower = std::lower_bound(v.begin(), v.end(), result.lower_fence);
        result.lower_whisker = lower == v.end() ? v.front() : *lower;

        const auto upper = std::upper_bound(v.begin(), v.end(), result.upper_fence);
        result.upper_whisker = upper == v.begin() ? v.front() : *(upper - 1);
        return result;
    }

    const std::vector<double> & data() const noexcept
    {
        // Bounded histories retain the newest samples but are not stored chronologically after wrapping.
        return data_;
    }

    double mode() const
    {
        if (data_.empty()) 
            return nan_();
            
        std::unordered_map<double, std::size_t> freq;
        freq.reserve(data_.size());
        std::size_t best_c = 0;
        double best_v = std::numeric_limits<double>::quiet_NaN();

        for (double x : data_) 
        {
            std::size_t c = ++freq[x];
            if (c > best_c || (c == best_c && (std::isnan(best_v) || x < best_v)))
            {
                best_c = c;
                best_v = x;
            }
        }
        return best_v;
    }

    double min() const noexcept {
        if (data_.empty()) return nan_();
        return *std::min_element(data_.begin(), data_.end());
    }

    double max() const noexcept {
        if (data_.empty()) return nan_();
        return *std::max_element(data_.begin(), data_.end());
    }

    // Sample variance (n-1). NaN if n < 2.
    double variance() const noexcept {
        const std::size_t n = data_.size();
        if (n < 2) return nan_();
        const long double mu = mean_ld_();
        long double M2 = 0.0L;
        for (double xd : data_) {
            const long double d = static_cast<long double>(xd) - mu;
            M2 += d * d;
        }
        return static_cast<double>(M2 / static_cast<long double>(n - 1));
        // For population variance: return static_cast<double>(M2 / n);
    }

    double standard_deviation() const noexcept {
        double v = variance();
        return std::isnan(v) ? v : std::sqrt(v);
    }

    // Bias-corrected sample skewness (Fisher-Pearson). NaN if n < 3.
    double skewness() const noexcept {
        const std::size_t n = data_.size();
        if (n < 3) return nan_();

        const long double mu = mean_ld_();
        long double M2 = 0.0L, M3 = 0.0L;

        for (double xd : data_) {
            const long double d  = static_cast<long double>(xd) - mu;
            const long double d2 = d * d;
            M2 += d2;
            M3 += d2 * d;
        }

        const long double s2 = M2 / static_cast<long double>(n - 1);
        if (s2 == 0.0L) return 0.0; // all equal
        const long double s = std::sqrt(s2);
        const long double nld = static_cast<long double>(n);
        const long double g1 = nld * M3 /
                               ((nld - 1.0L) * (nld - 2.0L) * s * s * s);
        return static_cast<double>(g1);
    }

    // Bias-corrected sample excess kurtosis (Joanes & Gill). NaN if n < 4.
    // Returns "excess" (kurtosis - 3).
    double kurtosis() const noexcept {
        const std::size_t n = data_.size();
        if (n < 4) return nan_();

        const long double mu = mean_ld_();
        long double M2 = 0.0L, M4 = 0.0L;

        for (double xd : data_) {
            const long double d  = static_cast<long double>(xd) - mu;
            const long double d2 = d * d;
            M2 += d2;
            M4 += d2 * d2;
        }

        const long double s2 = M2 / static_cast<long double>(n - 1);
        if (s2 == 0.0L) return 0.0;
        const long double m4_over_s4 = M4 / (s2 * s2);

        const long double nld = static_cast<long double>(n);
        const long double num = nld * (nld + 1.0L) / (nld - 1.0L) * m4_over_s4 -
                                3.0L * (nld - 1.0L) * (nld - 1.0L);
        const long double den = (nld - 2.0L) * (nld - 3.0L);
        return static_cast<double>(num / den);
    }

private:
    static double nan_() noexcept { return std::numeric_limits<double>::quiet_NaN(); }

    static long double next_mean_(long double mean, long double value,
                                  long double count) noexcept
    {
        if (std::signbit(mean) != std::signbit(value))
            return mean * ((count - 1.0L) / count) + value / count;
        return mean + (value - mean) / count;
    }

    static double midpoint_(double a, double b) noexcept
    {
        if (std::signbit(a) != std::signbit(b))
            return a * 0.5 + b * 0.5;
        return a + (b - a) * 0.5;
    }

    static double interpolate_(double a, double b, double fraction) noexcept
    {
        if (std::signbit(a) != std::signbit(b))
            return a * (1.0 - fraction) + b * fraction;
        return a + (b - a) * fraction;
    }

    static double quantile_sorted_(const std::vector<double> & values, double q) noexcept
    {
        if (q <= 0.0)
            return values.front();
        if (q >= 1.0)
            return values.back();

        const double position = q * static_cast<double>(values.size() - 1);
        const std::size_t lower = static_cast<std::size_t>(std::floor(position));
        const std::size_t upper = static_cast<std::size_t>(std::ceil(position));
        if (lower == upper)
            return values[lower];

        return interpolate_(values[lower], values[upper],
                            position - static_cast<double>(lower));
    }

    long double mean_ld_() const noexcept {
        if (data_.empty()) return std::numeric_limits<long double>::quiet_NaN();
        long double mean = 0.0L;
        long double count = 0.0L;
        for (double x : data_)
        {
            count += 1.0L;
            mean = next_mean_(mean, static_cast<long double>(x), count);
        }
        return mean;
    }

    std::vector<double> data_;
    std::size_t sample_limit_ = 0;
    std::size_t next_replacement_ = 0;
};




class online_statistics 
{
public:

  void reset() noexcept
    {
        // Clear heaps
        while (!lower_.empty()) 
            lower_.pop();
        while (!upper_.empty()) 
            upper_.pop();

        // Clear frequency map
        freq_.clear();

        // Reset running counters and moments
        count_ = 0;
        mean_ = 0.0L;
        M2_ = 0.0L;
        M3_ = 0.0L;
        M4_ = 0.0L;

        // Reset min/max
        min_ = 0.0;
        max_ = 0.0;

        // Reset mode tracking
        mode_count_ = 0;
        mode_value_ = std::numeric_limits<double>::quiet_NaN();
    }

    // Add a new observation
    void push(double x) 
    {
        if (!std::isfinite(x))
            throw std::invalid_argument("online_statistics requires finite samples");

        const long double previous_count = static_cast<long double>(count_);

        // --- Update online moments (Pébay/Welford) ---
        const long double new_count = previous_count + 1.0L;
        if (count_ == 0)
        {
            mean_ = static_cast<long double>(x);
        }
        else
        {
            const long double value = static_cast<long double>(x);
            const long double delta = value - mean_;
            const long double delta_n = delta / new_count;
            const long double delta_n2 = delta_n * delta_n;
            const long double term1 = delta * delta_n * previous_count;

            M4_ += term1 * delta_n2 * (new_count * new_count - 3.0L * new_count + 3.0L)
                 + 6.0L * delta_n2 * M2_
                 - 4.0L * delta_n * M3_;
            M3_ += term1 * delta_n * (new_count - 2.0L) - 3.0L * delta_n * M2_;
            M2_ += term1;
            mean_ = next_mean_(mean_, value, new_count);
        }
        ++count_;

        // --- Track min/max ---
        if (count_ == 1)
            min_ = max_ = x;
        else if (x < min_)
            min_ = x;
        else if (x > max_)
            max_ = x;

        // --- Maintain heaps for median ---
        if (lower_.empty() || x <= lower_.top())
            lower_.push(x); // max-heap
        else
            upper_.push(x); // min-heap
        rebalance_heaps_();

        // --- Mode (exact-equality for doubles) ---
        auto c = ++freq_[x];
        if (c > mode_count_ || (c == mode_count_ && x < mode_value_))
        {
            mode_value_ = x;
            mode_count_ = c;
        }
    }

    // Number of observations
    std::size_t count() const noexcept 
    {
        return count_;
    }

    // Mean of the data (NaN if no data)
    double mean() const noexcept 
    {
        return count_ ? static_cast<double>(mean_) : nan_();
    }

    // Median (NaN if no data)
    double median() const noexcept 
    {
        if (count_ == 0)
            return nan_();
            
        if (lower_.size() == upper_.size())
            return midpoint_(lower_.top(), upper_.top());
        
        return lower_.size() > upper_.size() ? lower_.top() : upper_.top();
    }

    // Mode (exact match; NaN if no data)
    double mode() const noexcept 
    {
        return count_ ? mode_value_ : nan_();
    }

    // Min / Max (NaN if no data)
    double min() const noexcept 
    {
        return count_ ? min_ : nan_();
    }
    double max() const noexcept 
    {
        return count_ ? max_ : nan_();
    }

    // Sample variance (dividing by n-1). NaN if n < 2
    double variance() const noexcept 
    {
        if (count_ < 2)
            return nan_();
        return static_cast<double>(M2_ / static_cast<long double>(count_ - 1));
        // If you want population variance, use: M2_ / count_
    }

    // Sample standard deviation. NaN if n < 2
    double standard_deviation() const noexcept 
    {
        double v = variance();
        return std::isnan(v) ? v : std::sqrt(v);
    }

    // Bias-corrected sample skewness (Fisher-Pearson). NaN if n < 3
    double skewness() const noexcept {
        const std::size_t n = count_;
        if (n < 3) return nan_();
        const long double s2 = M2_ / static_cast<long double>(n - 1);
        if (s2 == 0.0L) return 0.0; // all equal
        const long double s = std::sqrt(s2);
        const long double n_ld = static_cast<long double>(n);
        const long double g1 = n_ld * M3_ /
                               ((n_ld - 1.0L) * (n_ld - 2.0L) * s * s * s);
        return static_cast<double>(g1);
    }

    // Bias-corrected sample excess kurtosis. NaN if n < 4
    // Returns "excess" (kurtosis - 3), so 0 for a normal distribution.
    double kurtosis() const noexcept {
        const std::size_t n = count_;
        if (n < 4) return nan_();
        const long double n_ld = static_cast<long double>(n);
        const long double s2 = M2_ / static_cast<long double>(n - 1);
        if (s2 == 0.0L) return 0.0; // all equal
        const long double m4_over_s4 = M4_ / (s2 * s2);

        // Joanes & Gill (1998) unbiased estimator of excess kurtosis
        const long double num = n_ld * (n_ld + 1.0L) / (n_ld - 1.0L) * m4_over_s4 -
                                3.0L * (n_ld - 1.0L) * (n_ld - 1.0L);
        const long double den = (n_ld - 2.0L) * (n_ld - 3.0L);
        return static_cast<double>((num / den));
    }

private:
    // Heaps: lower_ is max-heap (lower half), upper_ is min-heap (upper half)
    std::priority_queue<double> lower_;
    std::priority_queue<double, std::vector<double>, std::greater<double>> upper_;

    void rebalance_heaps_() 
    {
        if (lower_.size() > upper_.size() + 1)
        {
            upper_.push(lower_.top());
            lower_.pop();
        }
        else if (upper_.size() > lower_.size() + 1)
        {
            lower_.push(upper_.top());
            upper_.pop();
        }
    }

    static double nan_() noexcept 
    {
        return std::numeric_limits<double>::quiet_NaN();
    }

    static long double next_mean_(long double mean, long double value,
                                  long double count) noexcept
    {
        if (std::signbit(mean) != std::signbit(value))
            return mean * ((count - 1.0L) / count) + value / count;
        return mean + (value - mean) / count;
    }

    static double midpoint_(double a, double b) noexcept
    {
        if (std::signbit(a) != std::signbit(b))
            return a * 0.5 + b * 0.5;
        return a + (b - a) * 0.5;
    }

    // Running counters / moments
    std::size_t count_ = 0;
    long double mean_  = 0.0L;
    long double M2_    = 0.0L;
    long double M3_    = 0.0L;
    long double M4_    = 0.0L;

    // Min/Max
    double min_ = 0.0, max_ = 0.0;

    // Mode tracking
    std::unordered_map<double, std::size_t> freq_;
    std::size_t mode_count_ = 0;
    double mode_value_ = std::numeric_limits<double>::quiet_NaN();
};


// Example usage:

/*

    statistics S;
    for (double x : {1.0, 2.0, 2.0, 3.0, 4.0}) S.push(x);

    std::cout << "n = " << S.count() << "\n";
    std::cout << "mean = " << S.mean() << "\n";
    std::cout << "median = " << S.median() << "\n";
    std::cout << "mode = " << S.mode() << "\n";
    std::cout << "min = " << S.min() << "\n";
    std::cout << "max = " << S.max() << "\n";
    std::cout << "variance = " << S.variance() << "\n";
    std::cout << "stddev = " << S.standard_deviation() << "\n";
    std::cout << "skewness = " << S.skewness() << "\n";
    std::cout << "kurtosis(excess) = " << S.kurtosis() << "\n" ;

    */
