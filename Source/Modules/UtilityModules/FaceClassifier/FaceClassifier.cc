#include "ikaros.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

using namespace ikaros;

class FaceClassifier : public Module
{
    matrix input_;
    matrix learn_;
    matrix output_;
    matrix novelty_;
    matrix distance_;
    matrix classes_count_;
    matrix count_;
    matrix vectors_;
    parameter capacity_;
    parameter threshold_;
    parameter confirmation_count_;
    parameter k_;
    parameter vectors_per_class_;

    struct Neighbor
    {
        float distance;
        int class_index;
    };

    std::vector<std::vector<std::vector<float>>> classes_;
    std::vector<float> pending_;
    int pending_count_ = 0;

    std::vector<float>
    descriptor() const
    {
        std::vector<float> d;

        if(input_.empty())
            return d;

        if(input_.rank() == 2 && input_.rows() > 0)
        {
            d.reserve(input_.cols());
            for(int i = 0; i < input_.cols(); ++i)
                d.push_back(input_(0, i));
            return d;
        }

        d.reserve(input_.size());
        for(int i = 0; i < input_.size(); ++i)
            d.push_back(input_.data()[i]);
        return d;
    }

    float
    distance(const std::vector<float> & a, const std::vector<float> & b) const
    {
        if(a.size() != b.size())
            return std::numeric_limits<float>::max();

        float sum = 0.0f;
        for(std::size_t i = 0; i < a.size(); ++i)
        {
            float d = a[i] - b[i];
            sum += d * d;
        }
        return std::sqrt(sum);
    }

    int
    nearest(const std::vector<float> & d, float & best_distance) const
    {
        std::vector<Neighbor> neighbors;
        best_distance = std::numeric_limits<float>::max();

        for(std::size_t i = 0; i < classes_.size(); ++i)
            for(const auto & sample : classes_[i])
            {
                float candidate = distance(d, sample);
                neighbors.push_back({candidate, static_cast<int>(i)});
                if(candidate < best_distance)
                    best_distance = candidate;
            }

        if(neighbors.empty())
            return -1;

        std::sort(neighbors.begin(), neighbors.end(), [](const Neighbor & a, const Neighbor & b) {
            return a.distance < b.distance;
        });

        int vote_count = std::min(std::max(1, k_.as_int()), static_cast<int>(neighbors.size()));
        int capacity = std::max(0, capacity_.as_int());
        std::vector<int> votes(capacity, 0);
        std::vector<float> distance_sums(capacity, 0.0f);

        for(int i = 0; i < vote_count; ++i)
        {
            int c = neighbors[i].class_index;
            if(c >= 0 && c < capacity)
            {
                votes[c] += 1;
                distance_sums[c] += neighbors[i].distance;
            }
        }

        int best = -1;
        int best_votes = -1;
        float best_average_distance = std::numeric_limits<float>::max();
        for(int c = 0; c < static_cast<int>(classes_.size()) && c < capacity; ++c)
        {
            if(votes[c] == 0)
                continue;

            float average_distance = distance_sums[c] / static_cast<float>(votes[c]);
            if(votes[c] > best_votes || (votes[c] == best_votes && average_distance < best_average_distance))
            {
                best_votes = votes[c];
                best_average_distance = average_distance;
                best = c;
            }
        }

        return best;
    }

    bool
    is_zero_descriptor(const std::vector<float> & d) const
    {
        for(float v : d)
            if(v != 0.0f)
                return false;
        return true;
    }

    int
    add_class(const std::vector<float> & d)
    {
        int capacity = std::max(0, capacity_.as_int());
        if(static_cast<int>(classes_.size()) >= capacity)
            return -1;

        classes_.push_back({d});
        return static_cast<int>(classes_.size()) - 1;
    }

    void
    add_sample(int class_index, const std::vector<float> & d)
    {
        if(class_index < 0 || class_index >= static_cast<int>(classes_.size()))
            return;

        int limit = std::max(1, vectors_per_class_.as_int());
        auto & samples = classes_[class_index];
        if(static_cast<int>(samples.size()) >= limit)
            return;

        for(const auto & sample : samples)
            if(distance(d, sample) < 0.001f)
                return;

        samples.push_back(d);
    }

    bool
    learning_enabled() const
    {
        return !learn_.connected() || (!learn_.empty() && learn_(0) != 0.0f);
    }

    void
    clear_pending()
    {
        pending_.clear();
        pending_count_ = 0;
    }

    bool
    update_pending(const std::vector<float> & d)
    {
        if(pending_.empty() || distance(d, pending_) > static_cast<float>(threshold_))
        {
            pending_ = d;
            pending_count_ = 1;
        }
        else
        {
            ++pending_count_;
            for(std::size_t i = 0; i < pending_.size(); ++i)
                pending_[i] += (d[i] - pending_[i]) / static_cast<float>(pending_count_);
        }

        return pending_count_ >= std::max(1, confirmation_count_.as_int());
    }

    void
    update_vectors()
    {
        vectors_.reset();
        int limit = std::min(static_cast<int>(classes_.size()), vectors_.size());
        for(int i = 0; i < limit; ++i)
            vectors_(i) = static_cast<float>(classes_[i].size());
    }

    void
    set_output(int index)
    {
        output_.reset();
        if(index >= 0 && index < output_.size())
        {
            output_(index) = 1.0f;
            if(index < count_.size())
                count_(index) += 1.0f;
        }
    }

public:
    void Init() override
    {
        Bind(input_, "INPUT");
        Bind(learn_, "LEARN");
        Bind(output_, "OUTPUT");
        Bind(novelty_, "NOVELTY");
        Bind(distance_, "DISTANCE");
        Bind(classes_count_, "CLASSES");
        Bind(count_, "COUNT");
        Bind(vectors_, "VECTORS");
        Bind(capacity_, "capacity");
        Bind(threshold_, "threshold");
        Bind(confirmation_count_, "confirmation_count");
        Bind(k_, "k");
        Bind(vectors_per_class_, "vectors_per_class");
    }

    void Tick() override
    {
        novelty_.reset();
        distance_.reset();
        classes_count_(0) = static_cast<float>(classes_.size());
        update_vectors();

        std::vector<float> d = descriptor();
        if(d.empty() || is_zero_descriptor(d))
        {
            output_.reset();
            clear_pending();
            return;
        }

        float best_distance = std::numeric_limits<float>::max();
        int best = nearest(d, best_distance);

        if(best < 0 || best_distance > static_cast<float>(threshold_))
        {
            if(!learning_enabled() || !update_pending(d))
            {
                output_.reset();
                distance_(0) = best >= 0 ? best_distance : 0.0f;
                return;
            }

            int new_class = add_class(pending_);
            if(new_class >= 0)
            {
                novelty_(0) = 1.0f;
                distance_(0) = 0.0f;
                classes_count_(0) = static_cast<float>(classes_.size());
                clear_pending();
                set_output(new_class);
                update_vectors();
                return;
            }
        }

        clear_pending();
        distance_(0) = best >= 0 ? best_distance : 0.0f;
        classes_count_(0) = static_cast<float>(classes_.size());
        if(learning_enabled() && best_distance <= static_cast<float>(threshold_))
            add_sample(best, d);
        update_vectors();
        set_output(best);
    }
};

INSTALL_CLASS(FaceClassifier)
