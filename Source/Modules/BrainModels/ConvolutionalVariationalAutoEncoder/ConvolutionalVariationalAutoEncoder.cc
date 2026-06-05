#include "ikaros.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

using namespace ikaros;

namespace
{
    using clock_type = std::chrono::steady_clock;

    double
    elapsed_ms(clock_type::time_point start)
    {
        return std::chrono::duration<double, std::milli>(clock_type::now() - start).count();
    }

    float
    uniform_weight(std::mt19937 & rng, int fan_in)
    {
        const float limit = std::sqrt(6.0f / std::max(1, fan_in));
        std::uniform_real_distribution<float> distribution(-limit, limit);
        return distribution(rng);
    }

    void
    fill_uniform(matrix & values, std::mt19937 & rng, int fan_in)
    {
        values.apply([&rng, fan_in](float) { return uniform_weight(rng, fan_in); });
    }

    void
    write_matrix(std::ofstream & stream, const matrix & values)
    {
        const int size = values.size();
        stream.write(reinterpret_cast<const char *>(&size), sizeof(size));
        stream.write(reinterpret_cast<const char *>(values.data()), sizeof(float) * size);
    }

    void
    read_matrix(std::ifstream & stream, matrix & values)
    {
        int size = 0;
        stream.read(reinterpret_cast<char *>(&size), sizeof(size));
        if(size < 0)
            throw std::runtime_error("ConvolutionalVariationalAutoEncoder: invalid weight file.");

        if(size != values.size())
            throw std::runtime_error("ConvolutionalVariationalAutoEncoder: weight file does not match network shape.");

        stream.read(reinterpret_cast<char *>(values.data()), sizeof(float) * values.size());
        if(!stream)
            throw std::runtime_error("ConvolutionalVariationalAutoEncoder: could not read weight data.");
    }

    void
    add_channel_bias(matrix & values, const matrix & bias)
    {
        if(values.rank() != 3 || bias.rank() != 1 || values.shape(2) != bias.size())
            throw std::runtime_error("ConvolutionalVariationalAutoEncoder: channel bias shape mismatch.");

        float * output = values.data();
        const float * bias_values = bias.data();
        const int channels = values.shape(2);
        const int positions = values.shape(0) * values.shape(1);

        for(int position = 0; position < positions; ++position)
        {
            float * output_position = output + position * channels;
            for(int channel = 0; channel < channels; ++channel)
                output_position[channel] += bias_values[channel];
        }
    }
}

class ConvolutionalVariationalAutoEncoder: public Module
{
    static constexpr int reconstruction_source_sample = 0;
    static constexpr int reconstruction_source_mean = 1;
    static constexpr int reconstruction_source_top_down = 2;

    enum class LatentMode
    {
        Dense,
        Spatial
    };

    parameter latent_mode_;
    parameter latent_size_;
    parameter latent_maps_;
    parameter latent_kernel_size_;
    parameter feature_maps_;
    parameter kernel_size_;
    parameter learning_rate_;
    parameter optimizer_;
    parameter adam_beta1_;
    parameter adam_beta2_;
    parameter adam_epsilon_;
    parameter beta_;
    parameter train_;
    parameter train_interval_;
    parameter dense_train_interval_;
    parameter profile_;
    parameter profile_interval_;
    parameter sample_;
    parameter reconstruction_source_;
    parameter weights_filename_;
    parameter load_weights_;
    parameter save_weights_;

    matrix input_;
    matrix top_down_;
    matrix effort_;
    matrix output_;
    matrix latent_mean_;
    matrix latent_log_variance_;
    matrix latent_sample_;
    matrix loss_;
    matrix reconstruction_loss_;
    matrix kl_loss_;

    int input_height_ = 0;
    int input_width_ = 0;
    int input_channels_ = 1;
    int encoded_height_ = 0;
    int encoded_width_ = 0;
    int latent_height_ = 0;
    int latent_width_ = 0;
    int encoded_size_ = 0;
    LatentMode latent_mode_value_ = LatentMode::Dense;
    int latent_size_value_ = 0;
    int latent_maps_value_ = 1;
    int latent_kernel_size_value_ = 1;
    int feature_maps_value_ = 0;
    int kernel_size_value_ = 0;
    int train_interval_value_ = 1;
    int dense_train_interval_value_ = 1;
    int train_tick_ = 0;
    int dense_train_tick_ = 0;
    bool initialized_ = false;
    bool training_reconstruction_ = false;

    struct ProfileCounters
    {
        double encode = 0.0;
        double decode = 0.0;
        double publish = 0.0;
        double loss = 0.0;
        double train = 0.0;
        double save = 0.0;
        double output_gradient = 0.0;
        double decoder_filter_gradient = 0.0;
        double decoder_activation_gradient = 0.0;
        double decoder_relu_dense = 0.0;
        double latent_gradients = 0.0;
        double encoder_dense_gradients = 0.0;
        double encoder_activation_gradient = 0.0;
        double encoder_filter_gradient = 0.0;
        double optimizer = 0.0;
        int ticks = 0;
        int train_ticks = 0;
        int dense_updates = 0;

        void reset()
        {
            *this = ProfileCounters();
        }
    } profile_counters_;

    matrix encoder_filters_;
    matrix encoder_bias_;
    matrix encoder_pre_activation_;
    matrix encoder_activation_;

    matrix mean_weights_;
    matrix mean_bias_;
    matrix log_variance_weights_;
    matrix log_variance_bias_;
    matrix latent_mean_values_;
    matrix latent_log_variance_values_;
    matrix latent_stddev_;
    matrix latent_epsilon_;
    matrix latent_values_;

    matrix decoder_weights_;
    matrix decoder_bias_;
    matrix decoder_pre_activation_;
    matrix decoder_activation_;
    matrix decoder_filters_;
    matrix latent_mean_projection_;
    matrix latent_log_variance_projection_;
    matrix decoder_projection_;
    matrix reconstruction_error_;
    matrix kl_terms_;
    matrix exp_log_variance_;
    matrix displayed_output_;

    matrix spatial_mean_filters_;
    matrix spatial_mean_bias_;
    matrix spatial_log_variance_filters_;
    matrix spatial_log_variance_bias_;
    matrix spatial_decoder_filters_;
    matrix spatial_decoder_bias_;

    matrix d_output_;
    matrix d_decoder_filters_;
    matrix d_decoder_activation_;
    matrix d_decoder_pre_activation_;
    matrix d_decoder_weights_;
    matrix d_latent_;
    matrix d_mean_;
    matrix d_log_variance_;
    matrix d_log_variance_kl_;
    matrix d_mean_weights_;
    matrix d_mean_bias_;
    matrix d_log_variance_weights_;
    matrix d_log_variance_bias_;
    matrix d_encoder_activation_;
    matrix d_encoder_filters_;
    matrix d_encoder_bias_;
    matrix d_spatial_mean_filters_;
    matrix d_spatial_mean_bias_;
    matrix d_spatial_log_variance_filters_;
    matrix d_spatial_log_variance_bias_;
    matrix d_spatial_decoder_filters_;
    matrix d_spatial_decoder_bias_;

    matrix encoder_filters_m_;
    matrix encoder_filters_v_;
    matrix encoder_bias_m_;
    matrix encoder_bias_v_;
    matrix mean_weights_m_;
    matrix mean_weights_v_;
    matrix mean_bias_m_;
    matrix mean_bias_v_;
    matrix log_variance_weights_m_;
    matrix log_variance_weights_v_;
    matrix log_variance_bias_m_;
    matrix log_variance_bias_v_;
    matrix decoder_weights_m_;
    matrix decoder_weights_v_;
    matrix decoder_bias_m_;
    matrix decoder_bias_v_;
    matrix decoder_filters_m_;
    matrix decoder_filters_v_;
    matrix spatial_mean_filters_m_;
    matrix spatial_mean_filters_v_;
    matrix spatial_mean_bias_m_;
    matrix spatial_mean_bias_v_;
    matrix spatial_log_variance_filters_m_;
    matrix spatial_log_variance_filters_v_;
    matrix spatial_log_variance_bias_m_;
    matrix spatial_log_variance_bias_v_;
    matrix spatial_decoder_filters_m_;
    matrix spatial_decoder_filters_v_;
    matrix spatial_decoder_bias_m_;
    matrix spatial_decoder_bias_v_;
    int adam_step_ = 0;
    int dense_adam_step_ = 0;
    float output_bias_m_ = 0.0f;
    float output_bias_v_ = 0.0f;

    float output_bias_ = 0.0f;

    std::mt19937 rng_{std::random_device{}()};
    std::normal_distribution<float> normal_{0.0f, 1.0f};
    std::filesystem::path resolved_weights_filename_;

    void
    Init()
    {
        Bind(latent_mode_, "latent_mode");
        Bind(latent_size_, "latent_size");
        Bind(latent_maps_, "latent_maps");
        Bind(latent_kernel_size_, "latent_kernel_size");
        Bind(feature_maps_, "feature_maps");
        Bind(kernel_size_, "kernel_size");
        Bind(learning_rate_, "learning_rate");
        Bind(optimizer_, "optimizer");
        Bind(adam_beta1_, "adam_beta1");
        Bind(adam_beta2_, "adam_beta2");
        Bind(adam_epsilon_, "adam_epsilon");
        Bind(beta_, "beta");
        Bind(train_, "train");
        Bind(train_interval_, "train_interval");
        Bind(dense_train_interval_, "dense_train_interval");
        Bind(profile_, "profile");
        Bind(profile_interval_, "profile_interval");
        Bind(sample_, "sample");
        Bind(reconstruction_source_, "reconstruction_source");
        Bind(weights_filename_, "weights_filename");
        Bind(load_weights_, "load_weights");
        Bind(save_weights_, "save_weights");

        Bind(input_, "INPUT");
        Bind(top_down_, "TOP_DOWN");
        Bind(effort_, "EFFORT");
        Bind(output_, "OUTPUT");
        Bind(latent_mean_, "LATENT_MEAN");
        Bind(latent_log_variance_, "LATENT_LOG_VARIANCE");
        Bind(latent_sample_, "LATENT_SAMPLE");
        Bind(loss_, "LOSS");
        Bind(reconstruction_loss_, "RECONSTRUCTION_LOSS");
        Bind(kl_loss_, "KL_LOSS");

        latent_mode_value_ = parse_latent_mode(latent_mode_.as_string());
        latent_size_value_ = std::max(1, latent_size_.as_int());
        latent_maps_value_ = std::max(1, latent_maps_.as_int());
        latent_kernel_size_value_ = std::max(1, latent_kernel_size_.as_int());
        feature_maps_value_ = std::max(1, feature_maps_.as_int());
        kernel_size_value_ = std::max(1, kernel_size_.as_int());
        train_interval_value_ = std::max(1, train_interval_.as_int());
        dense_train_interval_value_ = std::max(1, dense_train_interval_.as_int());

        if(load_weights_.as_bool())
        {
            if(!kernel().SanitizeReadPath(weights_filename_.as_string(), resolved_weights_filename_))
                throw std::runtime_error("ConvolutionalVariationalAutoEncoder can only read weight files from the project directory or UserData.");
            weights_filename_ = resolved_weights_filename_.string();
        }
        else if(save_weights_.as_bool())
        {
            if(!kernel().SanitizeWritePath(weights_filename_.as_string(), resolved_weights_filename_))
                throw std::runtime_error("ConvolutionalVariationalAutoEncoder can only write weight files inside UserData.");
            weights_filename_ = resolved_weights_filename_.string();
        }
    }

    bool
    reconstruction_source_is(int source) const
    {
        return reconstruction_source_.as_int() == source;
    }

    bool
    effective_reconstruction_source_is(int source) const
    {
        if(training_reconstruction_ && reconstruction_source_is(reconstruction_source_top_down))
            return source == reconstruction_source_mean;
        return reconstruction_source_is(source);
    }

    void
    validate_reconstruction_source() const
    {
        const int source = reconstruction_source_.as_int();
        if(source >= reconstruction_source_sample && source <= reconstruction_source_top_down)
            return;
        throw exception("ConvolutionalVariationalAutoEncoder: reconstruction_source must be sample, mean, or top_down.", path_);
    }

    LatentMode
    parse_latent_mode(const std::string & mode) const
    {
        if(mode == "dense")
            return LatentMode::Dense;
        if(mode == "spatial")
            return LatentMode::Spatial;

        throw exception("ConvolutionalVariationalAutoEncoder: latent_mode must be dense or spatial.", path_);
    }

    void
    initialize_for_input()
    {
        if(input_.rank() != 2 && input_.rank() != 3)
            throw exception("ConvolutionalVariationalAutoEncoder: INPUT must be a two- or three-dimensional matrix.", path_);

        input_height_ = input_.shape(0);
        input_width_ = input_.shape(1);
        input_channels_ = input_.rank() == 3 ? input_.shape(2) : 1;

        if(input_height_ < kernel_size_value_ || input_width_ < kernel_size_value_)
            throw exception("ConvolutionalVariationalAutoEncoder: kernel_size must fit inside INPUT.", path_);

        encoded_height_ = input_height_ - kernel_size_value_ + 1;
        encoded_width_ = input_width_ - kernel_size_value_ + 1;
        encoded_size_ = encoded_height_ * encoded_width_ * feature_maps_value_;

        if(latent_mode_value_ == LatentMode::Spatial)
        {
            if(encoded_height_ < latent_kernel_size_value_ || encoded_width_ < latent_kernel_size_value_)
                throw exception("ConvolutionalVariationalAutoEncoder: latent_kernel_size must fit inside the encoded feature map.", path_);

            latent_height_ = encoded_height_ - latent_kernel_size_value_ + 1;
            latent_width_ = encoded_width_ - latent_kernel_size_value_ + 1;
        }

        if(input_channels_ == 1)
            encoder_filters_.realloc(feature_maps_value_, kernel_size_value_, kernel_size_value_);
        else
            encoder_filters_.realloc(feature_maps_value_, kernel_size_value_, kernel_size_value_, input_channels_);
        encoder_bias_.realloc(feature_maps_value_);
        encoder_pre_activation_.realloc(encoded_height_, encoded_width_, feature_maps_value_);
        encoder_activation_.realloc(encoded_height_, encoded_width_, feature_maps_value_);

        if(input_channels_ == 1)
            decoder_filters_.realloc(feature_maps_value_, kernel_size_value_, kernel_size_value_);
        else
            decoder_filters_.realloc(feature_maps_value_, kernel_size_value_, kernel_size_value_, input_channels_);
        decoder_activation_.realloc(encoded_size_);
        decoder_pre_activation_.realloc(encoded_size_);
        reconstruction_error_.realloc(input_.shape());

        if(latent_mode_value_ == LatentMode::Spatial)
        {
            spatial_mean_filters_.realloc(latent_maps_value_, latent_kernel_size_value_, latent_kernel_size_value_, feature_maps_value_);
            spatial_mean_bias_.realloc(latent_maps_value_);
            spatial_log_variance_filters_.realloc(latent_maps_value_, latent_kernel_size_value_, latent_kernel_size_value_, feature_maps_value_);
            spatial_log_variance_bias_.realloc(latent_maps_value_);
            spatial_decoder_filters_.realloc(latent_maps_value_, latent_kernel_size_value_, latent_kernel_size_value_, feature_maps_value_);
            spatial_decoder_bias_.realloc(feature_maps_value_);

            latent_mean_values_.realloc(latent_height_, latent_width_, latent_maps_value_);
            latent_log_variance_values_.realloc(latent_height_, latent_width_, latent_maps_value_);
            latent_stddev_.realloc(latent_height_, latent_width_, latent_maps_value_);
            latent_epsilon_.realloc(latent_height_, latent_width_, latent_maps_value_);
            latent_values_.realloc(latent_height_, latent_width_, latent_maps_value_);
            kl_terms_.realloc(latent_height_, latent_width_, latent_maps_value_);
            exp_log_variance_.realloc(latent_height_, latent_width_, latent_maps_value_);

            d_output_.realloc(input_.shape());
            d_decoder_filters_.realloc(decoder_filters_.shape());
            d_decoder_activation_.realloc(encoded_height_, encoded_width_, feature_maps_value_);
            d_decoder_pre_activation_.realloc(encoded_height_, encoded_width_, feature_maps_value_);
            d_latent_.realloc(latent_height_, latent_width_, latent_maps_value_);
            d_mean_.realloc(latent_height_, latent_width_, latent_maps_value_);
            d_log_variance_.realloc(latent_height_, latent_width_, latent_maps_value_);
            d_log_variance_kl_.realloc(latent_height_, latent_width_, latent_maps_value_);
            d_encoder_activation_.realloc(encoded_height_, encoded_width_, feature_maps_value_);
            d_encoder_filters_.realloc(encoder_filters_.shape());
            d_encoder_bias_.realloc(feature_maps_value_);
            d_spatial_mean_filters_.realloc(spatial_mean_filters_.shape());
            d_spatial_mean_bias_.realloc(spatial_mean_bias_.shape());
            d_spatial_log_variance_filters_.realloc(spatial_log_variance_filters_.shape());
            d_spatial_log_variance_bias_.realloc(spatial_log_variance_bias_.shape());
            d_spatial_decoder_filters_.realloc(spatial_decoder_filters_.shape());
            d_spatial_decoder_bias_.realloc(spatial_decoder_bias_.shape());

            encoder_filters_m_.realloc(encoder_filters_.shape());
            encoder_filters_v_.realloc(encoder_filters_.shape());
            encoder_bias_m_.realloc(encoder_bias_.shape());
            encoder_bias_v_.realloc(encoder_bias_.shape());
            decoder_filters_m_.realloc(decoder_filters_.shape());
            decoder_filters_v_.realloc(decoder_filters_.shape());
            spatial_mean_filters_m_.realloc(spatial_mean_filters_.shape());
            spatial_mean_filters_v_.realloc(spatial_mean_filters_.shape());
            spatial_mean_bias_m_.realloc(spatial_mean_bias_.shape());
            spatial_mean_bias_v_.realloc(spatial_mean_bias_.shape());
            spatial_log_variance_filters_m_.realloc(spatial_log_variance_filters_.shape());
            spatial_log_variance_filters_v_.realloc(spatial_log_variance_filters_.shape());
            spatial_log_variance_bias_m_.realloc(spatial_log_variance_bias_.shape());
            spatial_log_variance_bias_v_.realloc(spatial_log_variance_bias_.shape());
            spatial_decoder_filters_m_.realloc(spatial_decoder_filters_.shape());
            spatial_decoder_filters_v_.realloc(spatial_decoder_filters_.shape());
            spatial_decoder_bias_m_.realloc(spatial_decoder_bias_.shape());
            spatial_decoder_bias_v_.realloc(spatial_decoder_bias_.shape());

            spatial_mean_bias_.reset();
            spatial_log_variance_bias_.reset();
            spatial_decoder_bias_.reset();
            encoder_bias_.reset();
            latent_stddev_.set(1.0f);
            latent_epsilon_.reset();
            latent_values_.reset();
            reset_adam_state();

            fill_uniform(encoder_filters_, rng_, kernel_size_value_ * kernel_size_value_ * input_channels_);
            fill_uniform(spatial_mean_filters_, rng_, latent_kernel_size_value_ * latent_kernel_size_value_ * feature_maps_value_);
            fill_uniform(spatial_log_variance_filters_, rng_, latent_kernel_size_value_ * latent_kernel_size_value_ * feature_maps_value_);
            fill_uniform(spatial_decoder_filters_, rng_, latent_kernel_size_value_ * latent_kernel_size_value_ * latent_maps_value_);
            fill_uniform(decoder_filters_, rng_, kernel_size_value_ * kernel_size_value_ * feature_maps_value_);

            output_.realloc(input_.shape());
            latent_mean_.realloc(latent_mean_values_.shape());
            latent_log_variance_.realloc(latent_log_variance_values_.shape());
            latent_sample_.realloc(latent_values_.shape());

            initialized_ = true;
            train_tick_ = 0;
            dense_train_tick_ = 0;

            if(load_weights_.as_bool())
                load_weights();
            return;
        }

        mean_weights_.realloc(encoded_size_, latent_size_value_);
        mean_bias_.realloc(latent_size_value_);
        log_variance_weights_.realloc(encoded_size_, latent_size_value_);
        log_variance_bias_.realloc(latent_size_value_);
        latent_mean_values_.realloc(latent_size_value_);
        latent_log_variance_values_.realloc(latent_size_value_);
        latent_stddev_.realloc(latent_size_value_);
        latent_epsilon_.realloc(latent_size_value_);
        latent_values_.realloc(latent_size_value_);

        decoder_weights_.realloc(latent_size_value_, encoded_size_);
        decoder_bias_.realloc(encoded_size_);
        latent_mean_projection_.realloc(latent_size_value_);
        latent_log_variance_projection_.realloc(latent_size_value_);
        decoder_projection_.realloc(encoded_size_);
        kl_terms_.realloc(latent_size_value_);
        exp_log_variance_.realloc(latent_size_value_);

        d_output_.realloc(input_.shape());
        d_decoder_filters_.realloc(decoder_filters_.shape());
        d_decoder_activation_.realloc(encoded_height_, encoded_width_, feature_maps_value_);
        d_decoder_pre_activation_.realloc(encoded_size_);
        d_decoder_weights_.realloc(decoder_weights_.shape());
        d_latent_.realloc(latent_size_value_);
        d_mean_.realloc(latent_size_value_);
        d_log_variance_.realloc(latent_size_value_);
        d_log_variance_kl_.realloc(latent_size_value_);
        d_mean_weights_.realloc(mean_weights_.shape());
        d_mean_bias_.realloc(latent_size_value_);
        d_log_variance_weights_.realloc(log_variance_weights_.shape());
        d_log_variance_bias_.realloc(latent_size_value_);
        d_encoder_activation_.realloc(encoded_size_);
        d_encoder_filters_.realloc(encoder_filters_.shape());
        d_encoder_bias_.realloc(feature_maps_value_);

        encoder_filters_m_.realloc(encoder_filters_.shape());
        encoder_filters_v_.realloc(encoder_filters_.shape());
        encoder_bias_m_.realloc(encoder_bias_.shape());
        encoder_bias_v_.realloc(encoder_bias_.shape());
        mean_weights_m_.realloc(mean_weights_.shape());
        mean_weights_v_.realloc(mean_weights_.shape());
        mean_bias_m_.realloc(mean_bias_.shape());
        mean_bias_v_.realloc(mean_bias_.shape());
        log_variance_weights_m_.realloc(log_variance_weights_.shape());
        log_variance_weights_v_.realloc(log_variance_weights_.shape());
        log_variance_bias_m_.realloc(log_variance_bias_.shape());
        log_variance_bias_v_.realloc(log_variance_bias_.shape());
        decoder_weights_m_.realloc(decoder_weights_.shape());
        decoder_weights_v_.realloc(decoder_weights_.shape());
        decoder_bias_m_.realloc(decoder_bias_.shape());
        decoder_bias_v_.realloc(decoder_bias_.shape());
        decoder_filters_m_.realloc(decoder_filters_.shape());
        decoder_filters_v_.realloc(decoder_filters_.shape());

        encoder_bias_.reset();
        mean_bias_.reset();
        log_variance_bias_.reset();
        decoder_bias_.reset();
        latent_stddev_.set(1.0f);
        latent_epsilon_.reset();
        latent_values_.reset();
        reset_adam_state();

        fill_uniform(encoder_filters_, rng_, kernel_size_value_ * kernel_size_value_ * input_channels_);
        fill_uniform(mean_weights_, rng_, encoded_size_);
        fill_uniform(log_variance_weights_, rng_, encoded_size_);
        fill_uniform(decoder_weights_, rng_, latent_size_value_);
        fill_uniform(decoder_filters_, rng_, kernel_size_value_ * kernel_size_value_ * feature_maps_value_);

        output_.realloc(input_.shape());
        latent_mean_.realloc(latent_size_value_);
        latent_log_variance_.realloc(latent_size_value_);
        latent_sample_.realloc(latent_size_value_);

        initialized_ = true;
        train_tick_ = 0;
        dense_train_tick_ = 0;

        if(load_weights_.as_bool())
            load_weights();
    }

    void
    encode()
    {
        encode_input_to_features();
        encoder_activation_.copy(encoder_pre_activation_).apply([](float value) { return std::max(0.0f, value); });

        if(latent_mode_value_ == LatentMode::Spatial)
        {
            encode_spatial_latent();
            return;
        }

        encoder_activation_.reshape(encoded_size_);
        latent_mean_projection_.dense_forward(encoder_activation_, mean_weights_);
        latent_log_variance_projection_.dense_forward(encoder_activation_, log_variance_weights_);
        encoder_activation_.reshape(encoded_height_, encoded_width_, feature_maps_value_);

        latent_mean_values_.copy(latent_mean_projection_).add(mean_bias_);
        latent_log_variance_values_.copy(latent_log_variance_projection_).add(log_variance_bias_);
        latent_log_variance_values_.apply([](float value) { return std::clamp(value, -20.0f, 10.0f); });
        latent_stddev_.copy(latent_log_variance_values_).scale(0.5f).apply([](float value) { return std::exp(value); });

        if(sample_.as_bool())
            latent_epsilon_.apply([this](float) { return normal_(rng_); });
        else
            latent_epsilon_.reset();

        latent_values_.copy(latent_epsilon_).multiply(latent_stddev_).add(latent_mean_values_);
    }

    void
    decode()
    {
        if(latent_mode_value_ == LatentMode::Spatial)
        {
            decode_spatial_latent();
            return;
        }

        decoder_projection_.dense_forward(decoder_latent_input(), decoder_weights_);
        decoder_pre_activation_.copy(decoder_projection_).add(decoder_bias_);
        decoder_activation_.copy(decoder_pre_activation_).apply([](float value) { return std::max(0.0f, value); });

        decode_features_to_output();
    }

    void
    encode_input_to_features()
    {
        if(input_channels_ == 1)
            encoder_pre_activation_.conv2_valid_filterbank(input_, encoder_filters_, encoder_bias_);
        else
            encoder_pre_activation_.conv2_valid_channel_filterbank(input_, encoder_filters_, encoder_bias_);
    }

    void
    decode_features_to_output()
    {
        decoder_activation_.reshape(encoded_height_, encoded_width_, feature_maps_value_);
        if(input_channels_ == 1)
            output_.conv2_valid_filterbank_backward_input(decoder_activation_, decoder_filters_, input_height_, input_width_).add(output_bias_);
        else
        {
            output_.conv2_valid_channel_filterbank_backward_input(decoder_activation_, decoder_filters_, input_height_, input_width_);
            output_.add(output_bias_);
        }
        decoder_activation_.reshape(encoded_size_);
    }

    void
    encode_spatial_latent()
    {
        latent_mean_values_.conv2_valid_channel_filterbank(encoder_activation_, spatial_mean_filters_, spatial_mean_bias_);
        latent_log_variance_values_.conv2_valid_channel_filterbank(encoder_activation_, spatial_log_variance_filters_, spatial_log_variance_bias_);
        latent_log_variance_values_.apply([](float value) { return std::clamp(value, -20.0f, 10.0f); });
        latent_stddev_.copy(latent_log_variance_values_).scale(0.5f).apply([](float value) { return std::exp(value); });

        if(sample_.as_bool())
            latent_epsilon_.apply([this](float) { return normal_(rng_); });
        else
            latent_epsilon_.reset();

        latent_values_.copy(latent_epsilon_).multiply(latent_stddev_).add(latent_mean_values_);
    }

    void
    decode_spatial_latent()
    {
        decoder_pre_activation_.reshape(encoded_height_, encoded_width_, feature_maps_value_);
        decoder_activation_.reshape(encoded_height_, encoded_width_, feature_maps_value_);

        decoder_pre_activation_.conv2_valid_channel_filterbank_backward_input(decoder_latent_input(), spatial_decoder_filters_, encoded_height_, encoded_width_);
        add_channel_bias(decoder_pre_activation_, spatial_decoder_bias_);
        decoder_activation_.copy(decoder_pre_activation_).apply([](float value) { return std::max(0.0f, value); });
        decode_features_to_output();
        decoder_pre_activation_.reshape(encoded_size_);
    }

    const matrix &
    decoder_latent_input() const
    {
        if(effective_reconstruction_source_is(reconstruction_source_mean))
            return latent_mean_values_;
        if(effective_reconstruction_source_is(reconstruction_source_top_down))
        {
            if(!top_down_.connected() || top_down_.is_uninitialized() || top_down_.empty())
                return latent_mean_values_;
            if(top_down_.shape() != latent_mean_values_.shape())
                return latent_mean_values_;
            return top_down_;
        }
        return latent_values_;
    }

    void
    publish_latent()
    {
        latent_mean_.copy(latent_mean_values_);
        latent_log_variance_.copy(latent_log_variance_values_);
        latent_sample_.copy(latent_values_);
    }

    void
    compute_losses()
    {
        reconstruction_error_.subtract(output_, input_);
        reconstruction_error_.multiply(reconstruction_error_);
        const float reconstruction = 0.5f * reconstruction_error_.sum() / std::max(1, input_.size());

        kl_terms_.copy(latent_mean_values_).multiply(latent_mean_values_);
        exp_log_variance_.copy(latent_log_variance_values_).apply([](float value) { return std::exp(value); });
        kl_terms_.add(exp_log_variance_).subtract(latent_log_variance_values_).subtract(1.0f);
        const float kl = 0.5f * kl_terms_.sum() / std::max(1, latent_values_.size());

        reconstruction_loss_(0) = reconstruction;
        kl_loss_(0) = kl;
        loss_(0) = reconstruction + beta_.as_float() * kl;
    }

    void
    train_step()
    {
        if(latent_mode_value_ == LatentMode::Spatial)
        {
            train_step_spatial();
            return;
        }

        const float learning_rate = learning_rate_.as_float();
        if(learning_rate <= 0.0f)
            return;

        const bool profiling = profile_.as_bool();
        auto step_start = clock_type::now();
        float d_output_bias = 0.0f;

        const float output_scale = 1.0f / std::max(1, input_.size());
        d_output_.subtract(output_, input_).scale(output_scale);
        d_output_bias = d_output_.sum();
        if(profiling)
        {
            profile_counters_.output_gradient += elapsed_ms(step_start);
            step_start = clock_type::now();
        }

        decoder_activation_.reshape(encoded_height_, encoded_width_, feature_maps_value_);
        if(input_channels_ == 1)
            d_decoder_filters_.conv2_valid_filterbank_backward_filters(d_output_, decoder_activation_, kernel_size_value_, kernel_size_value_);
        else
            d_decoder_filters_.conv2_valid_channel_filterbank_backward_filters(d_output_, decoder_activation_, kernel_size_value_, kernel_size_value_);
        if(profiling)
        {
            profile_counters_.decoder_filter_gradient += elapsed_ms(step_start);
            step_start = clock_type::now();
        }
        d_decoder_activation_.reshape(encoded_height_, encoded_width_, feature_maps_value_);
        if(input_channels_ == 1)
            d_decoder_activation_.conv2_valid_filterbank(d_output_, decoder_filters_);
        else
            d_decoder_activation_.conv2_valid_channel_filterbank(d_output_, decoder_filters_);
        decoder_activation_.reshape(encoded_size_);
        d_decoder_activation_.reshape(encoded_size_);
        if(profiling)
        {
            profile_counters_.decoder_activation_gradient += elapsed_ms(step_start);
            step_start = clock_type::now();
        }

        d_decoder_pre_activation_.relu_backward(d_decoder_activation_, decoder_pre_activation_);

        d_decoder_weights_.outer_product(decoder_latent_input(), d_decoder_pre_activation_);
        d_latent_.dense_backward_input(decoder_weights_, d_decoder_pre_activation_);
        if(profiling)
        {
            profile_counters_.decoder_relu_dense += elapsed_ms(step_start);
            step_start = clock_type::now();
        }

        const float beta = beta_.as_float();
        const float kl_scale = beta / std::max(1, latent_size_value_);
        const bool source_sample = effective_reconstruction_source_is(reconstruction_source_sample);
        const bool source_mean = effective_reconstruction_source_is(reconstruction_source_mean);

        if(source_sample)
        {
            d_mean_.copy(d_latent_).multiply_and_accumulate(latent_mean_values_, kl_scale);
            d_log_variance_.copy(d_latent_).multiply(latent_epsilon_).multiply(latent_stddev_).scale(0.5f);
        }
        else if(source_mean)
        {
            d_mean_.copy(d_latent_).multiply_and_accumulate(latent_mean_values_, kl_scale);
            d_log_variance_.reset();
        }
        else
        {
            d_mean_.copy(latent_mean_values_).scale(kl_scale);
            d_log_variance_.reset();
        }
        d_log_variance_kl_.copy(latent_log_variance_values_)
            .apply([](float value) { return std::exp(value) - 1.0f; })
            .scale(0.5f * kl_scale);
        d_log_variance_.add(d_log_variance_kl_);

        d_mean_bias_.copy(d_mean_);
        d_log_variance_bias_.copy(d_log_variance_);
        if(profiling)
        {
            profile_counters_.latent_gradients += elapsed_ms(step_start);
            step_start = clock_type::now();
        }

        encoder_activation_.reshape(encoded_size_);
        d_mean_weights_.outer_product(encoder_activation_, d_mean_);
        d_log_variance_weights_.outer_product(encoder_activation_, d_log_variance_);
        if(profiling)
        {
            profile_counters_.encoder_dense_gradients += elapsed_ms(step_start);
            step_start = clock_type::now();
        }

        compute_encoder_activation_gradient();
        if(profiling)
        {
            profile_counters_.encoder_activation_gradient += elapsed_ms(step_start);
            step_start = clock_type::now();
        }

        encoder_activation_.reshape(encoded_height_, encoded_width_, feature_maps_value_);
        d_mean_.reshape(latent_size_value_);
        d_log_variance_.reshape(latent_size_value_);

        d_encoder_activation_.reshape(encoded_height_, encoded_width_, feature_maps_value_);
        if(input_channels_ == 1)
        {
            d_encoder_filters_.conv2_valid_filterbank_backward_filters_relu(input_, d_encoder_activation_, encoder_pre_activation_, kernel_size_value_, kernel_size_value_);
            d_encoder_bias_.sum_first_two_dimensions_relu(d_encoder_activation_, encoder_pre_activation_);
        }
        else
        {
            d_decoder_activation_.relu_backward(d_encoder_activation_, encoder_pre_activation_);
            d_encoder_filters_.conv2_valid_channel_filterbank_backward_filters(input_, d_decoder_activation_, kernel_size_value_, kernel_size_value_);
            d_encoder_bias_.sum_first_two_dimensions(d_decoder_activation_);
        }

        d_encoder_activation_.reshape(encoded_size_);
        if(profiling)
        {
            profile_counters_.encoder_filter_gradient += elapsed_ms(step_start);
            step_start = clock_type::now();
        }

        apply_optimizer_update(learning_rate, d_output_bias, d_decoder_pre_activation_);
        if(profiling)
            profile_counters_.optimizer += elapsed_ms(step_start);
    }

    void
    train_step_spatial()
    {
        const float learning_rate = learning_rate_.as_float();
        if(learning_rate <= 0.0f)
            return;

        const bool profiling = profile_.as_bool();
        auto step_start = clock_type::now();

        const float output_scale = 1.0f / std::max(1, input_.size());
        d_output_.subtract(output_, input_).scale(output_scale);
        const float d_output_bias = d_output_.sum();
        if(profiling)
        {
            profile_counters_.output_gradient += elapsed_ms(step_start);
            step_start = clock_type::now();
        }

        decoder_activation_.reshape(encoded_height_, encoded_width_, feature_maps_value_);
        if(input_channels_ == 1)
            d_decoder_filters_.conv2_valid_filterbank_backward_filters(d_output_, decoder_activation_, kernel_size_value_, kernel_size_value_);
        else
            d_decoder_filters_.conv2_valid_channel_filterbank_backward_filters(d_output_, decoder_activation_, kernel_size_value_, kernel_size_value_);
        if(profiling)
        {
            profile_counters_.decoder_filter_gradient += elapsed_ms(step_start);
            step_start = clock_type::now();
        }

        d_decoder_activation_.reshape(encoded_height_, encoded_width_, feature_maps_value_);
        if(input_channels_ == 1)
            d_decoder_activation_.conv2_valid_filterbank(d_output_, decoder_filters_);
        else
            d_decoder_activation_.conv2_valid_channel_filterbank(d_output_, decoder_filters_);
        if(profiling)
        {
            profile_counters_.decoder_activation_gradient += elapsed_ms(step_start);
            step_start = clock_type::now();
        }

        decoder_pre_activation_.reshape(encoded_height_, encoded_width_, feature_maps_value_);
        d_decoder_pre_activation_.reshape(encoded_height_, encoded_width_, feature_maps_value_);
        d_decoder_pre_activation_.relu_backward(d_decoder_activation_, decoder_pre_activation_);
        decoder_pre_activation_.reshape(encoded_size_);

        compute_spatial_decoder_gradients();
        if(profiling)
        {
            profile_counters_.decoder_relu_dense += elapsed_ms(step_start);
            step_start = clock_type::now();
        }

        compute_spatial_latent_gradients();
        if(profiling)
        {
            profile_counters_.latent_gradients += elapsed_ms(step_start);
            step_start = clock_type::now();
        }

        compute_spatial_encoder_activation_gradient();
        if(profiling)
        {
            profile_counters_.encoder_activation_gradient += elapsed_ms(step_start);
            step_start = clock_type::now();
        }

        if(input_channels_ == 1)
        {
            d_encoder_filters_.conv2_valid_filterbank_backward_filters_relu(input_, d_encoder_activation_, encoder_pre_activation_, kernel_size_value_, kernel_size_value_);
            d_encoder_bias_.sum_first_two_dimensions_relu(d_encoder_activation_, encoder_pre_activation_);
        }
        else
        {
            d_decoder_activation_.relu_backward(d_encoder_activation_, encoder_pre_activation_);
            d_encoder_filters_.conv2_valid_channel_filterbank_backward_filters(input_, d_decoder_activation_, kernel_size_value_, kernel_size_value_);
            d_encoder_bias_.sum_first_two_dimensions(d_decoder_activation_);
        }
        if(profiling)
        {
            profile_counters_.encoder_filter_gradient += elapsed_ms(step_start);
            step_start = clock_type::now();
        }

        apply_spatial_optimizer_update(learning_rate, d_output_bias);
        if(profiling)
            profile_counters_.optimizer += elapsed_ms(step_start);

        d_decoder_pre_activation_.reshape(encoded_size_);
        decoder_activation_.reshape(encoded_size_);
    }

    void
    compute_spatial_decoder_gradients()
    {
        d_decoder_pre_activation_.reshape(encoded_height_, encoded_width_, feature_maps_value_);
        d_latent_.conv2_valid_channel_filterbank(d_decoder_pre_activation_, spatial_decoder_filters_);
        d_spatial_decoder_filters_.conv2_valid_channel_filterbank_backward_filters(d_decoder_pre_activation_, decoder_latent_input(), latent_kernel_size_value_, latent_kernel_size_value_);
        d_spatial_decoder_bias_.sum_first_two_dimensions(d_decoder_pre_activation_);
        d_decoder_pre_activation_.reshape(encoded_size_);
    }

    void
    compute_spatial_latent_gradients()
    {
        const float beta = beta_.as_float();
        const float kl_scale = beta / std::max(1, latent_values_.size());

        const float * latent_gradient = d_latent_.data();
        const float * mean = latent_mean_values_.data();
        const float * log_variance = latent_log_variance_values_.data();
        const float * epsilon = latent_epsilon_.data();
        const float * stddev = latent_stddev_.data();
        float * d_mean = d_mean_.data();
        float * d_log_variance = d_log_variance_.data();
        const bool source_sample = effective_reconstruction_source_is(reconstruction_source_sample);
        const bool source_mean = effective_reconstruction_source_is(reconstruction_source_mean);

        for(int i = 0; i < latent_values_.size(); ++i)
        {
            if(source_sample)
            {
                d_mean[i] = latent_gradient[i] + kl_scale * mean[i];
                d_log_variance[i] = 0.5f * latent_gradient[i] * epsilon[i] * stddev[i];
            }
            else if(source_mean)
            {
                d_mean[i] = latent_gradient[i] + kl_scale * mean[i];
                d_log_variance[i] = 0.0f;
            }
            else
            {
                d_mean[i] = kl_scale * mean[i];
                d_log_variance[i] = 0.0f;
            }

            d_log_variance[i] += 0.5f * kl_scale * (std::exp(log_variance[i]) - 1.0f);
        }
    }

    void
    compute_spatial_encoder_activation_gradient()
    {
        d_encoder_activation_.conv2_valid_channel_filterbank_backward(encoder_activation_, spatial_mean_filters_, d_mean_, d_spatial_mean_filters_, d_spatial_mean_bias_);
        d_spatial_log_variance_filters_.conv2_valid_channel_filterbank_backward_filters(encoder_activation_, d_log_variance_, latent_kernel_size_value_, latent_kernel_size_value_);
        d_spatial_log_variance_bias_.sum_first_two_dimensions(d_log_variance_);
        d_decoder_activation_.conv2_valid_channel_filterbank_backward_input(d_log_variance_, spatial_log_variance_filters_, encoded_height_, encoded_width_);
        d_encoder_activation_.add(d_decoder_activation_);
    }

    void
    apply_optimizer_update(float learning_rate, float d_output_bias, const matrix & d_decoder_bias)
    {
        const bool update_dense = should_update_dense_this_training_step();
        if(update_dense)
            ++profile_counters_.dense_updates;

        if(optimizer_.compare_string("adam"))
        {
            ++adam_step_;
            const float beta1 = std::clamp(adam_beta1_.as_float(), 0.0f, 0.999999f);
            const float beta2 = std::clamp(adam_beta2_.as_float(), 0.0f, 0.999999f);
            const float epsilon = std::max(adam_epsilon_.as_float(), 1e-12f);
            const float beta1_correction = 1.0f - std::pow(beta1, static_cast<float>(adam_step_));
            const float beta2_correction = 1.0f - std::pow(beta2, static_cast<float>(adam_step_));

            encoder_filters_.adam_update(d_encoder_filters_, encoder_filters_m_, encoder_filters_v_, learning_rate, beta1, beta2, beta1_correction, beta2_correction, epsilon);
            encoder_bias_.adam_update(d_encoder_bias_, encoder_bias_m_, encoder_bias_v_, learning_rate, beta1, beta2, beta1_correction, beta2_correction, epsilon);
            decoder_filters_.adam_update(d_decoder_filters_, decoder_filters_m_, decoder_filters_v_, learning_rate, beta1, beta2, beta1_correction, beta2_correction, epsilon);
            apply_adam_scalar_update(output_bias_, d_output_bias, output_bias_m_, output_bias_v_, learning_rate, beta1, beta2, beta1_correction, beta2_correction, epsilon);

            if(update_dense)
            {
                ++dense_adam_step_;
                const float dense_beta1_correction = 1.0f - std::pow(beta1, static_cast<float>(dense_adam_step_));
                const float dense_beta2_correction = 1.0f - std::pow(beta2, static_cast<float>(dense_adam_step_));

                mean_weights_.adam_update(d_mean_weights_, mean_weights_m_, mean_weights_v_, learning_rate, beta1, beta2, dense_beta1_correction, dense_beta2_correction, epsilon);
                mean_bias_.adam_update(d_mean_bias_, mean_bias_m_, mean_bias_v_, learning_rate, beta1, beta2, dense_beta1_correction, dense_beta2_correction, epsilon);
                log_variance_weights_.adam_update(d_log_variance_weights_, log_variance_weights_m_, log_variance_weights_v_, learning_rate, beta1, beta2, dense_beta1_correction, dense_beta2_correction, epsilon);
                log_variance_bias_.adam_update(d_log_variance_bias_, log_variance_bias_m_, log_variance_bias_v_, learning_rate, beta1, beta2, dense_beta1_correction, dense_beta2_correction, epsilon);
                decoder_weights_.adam_update(d_decoder_weights_, decoder_weights_m_, decoder_weights_v_, learning_rate, beta1, beta2, dense_beta1_correction, dense_beta2_correction, epsilon);
                decoder_bias_.adam_update(d_decoder_bias, decoder_bias_m_, decoder_bias_v_, learning_rate, beta1, beta2, dense_beta1_correction, dense_beta2_correction, epsilon);
            }
            return;
        }

        if(!optimizer_.compare_string("sgd"))
            throw exception("ConvolutionalVariationalAutoEncoder: optimizer must be adam or sgd.", path_);

        apply_sgd_update(encoder_filters_, d_encoder_filters_, learning_rate);
        apply_sgd_update(encoder_bias_, d_encoder_bias_, learning_rate);
        apply_sgd_update(decoder_filters_, d_decoder_filters_, learning_rate);
        output_bias_ -= learning_rate * d_output_bias;

        if(update_dense)
        {
            apply_sgd_update(mean_weights_, d_mean_weights_, learning_rate);
            apply_sgd_update(mean_bias_, d_mean_bias_, learning_rate);
            apply_sgd_update(log_variance_weights_, d_log_variance_weights_, learning_rate);
            apply_sgd_update(log_variance_bias_, d_log_variance_bias_, learning_rate);
            apply_sgd_update(decoder_weights_, d_decoder_weights_, learning_rate);
            apply_sgd_update(decoder_bias_, d_decoder_bias, learning_rate);
        }
    }

    void
    apply_spatial_optimizer_update(float learning_rate, float d_output_bias)
    {
        const bool update_latent = should_update_dense_this_training_step();
        if(update_latent)
            ++profile_counters_.dense_updates;

        if(optimizer_.compare_string("adam"))
        {
            ++adam_step_;
            const float beta1 = std::clamp(adam_beta1_.as_float(), 0.0f, 0.999999f);
            const float beta2 = std::clamp(adam_beta2_.as_float(), 0.0f, 0.999999f);
            const float epsilon = std::max(adam_epsilon_.as_float(), 1e-12f);
            const float beta1_correction = 1.0f - std::pow(beta1, static_cast<float>(adam_step_));
            const float beta2_correction = 1.0f - std::pow(beta2, static_cast<float>(adam_step_));

            encoder_filters_.adam_update(d_encoder_filters_, encoder_filters_m_, encoder_filters_v_, learning_rate, beta1, beta2, beta1_correction, beta2_correction, epsilon);
            encoder_bias_.adam_update(d_encoder_bias_, encoder_bias_m_, encoder_bias_v_, learning_rate, beta1, beta2, beta1_correction, beta2_correction, epsilon);
            decoder_filters_.adam_update(d_decoder_filters_, decoder_filters_m_, decoder_filters_v_, learning_rate, beta1, beta2, beta1_correction, beta2_correction, epsilon);
            apply_adam_scalar_update(output_bias_, d_output_bias, output_bias_m_, output_bias_v_, learning_rate, beta1, beta2, beta1_correction, beta2_correction, epsilon);

            if(update_latent)
            {
                ++dense_adam_step_;
                const float latent_beta1_correction = 1.0f - std::pow(beta1, static_cast<float>(dense_adam_step_));
                const float latent_beta2_correction = 1.0f - std::pow(beta2, static_cast<float>(dense_adam_step_));

                spatial_mean_filters_.adam_update(d_spatial_mean_filters_, spatial_mean_filters_m_, spatial_mean_filters_v_, learning_rate, beta1, beta2, latent_beta1_correction, latent_beta2_correction, epsilon);
                spatial_mean_bias_.adam_update(d_spatial_mean_bias_, spatial_mean_bias_m_, spatial_mean_bias_v_, learning_rate, beta1, beta2, latent_beta1_correction, latent_beta2_correction, epsilon);
                spatial_log_variance_filters_.adam_update(d_spatial_log_variance_filters_, spatial_log_variance_filters_m_, spatial_log_variance_filters_v_, learning_rate, beta1, beta2, latent_beta1_correction, latent_beta2_correction, epsilon);
                spatial_log_variance_bias_.adam_update(d_spatial_log_variance_bias_, spatial_log_variance_bias_m_, spatial_log_variance_bias_v_, learning_rate, beta1, beta2, latent_beta1_correction, latent_beta2_correction, epsilon);
                spatial_decoder_filters_.adam_update(d_spatial_decoder_filters_, spatial_decoder_filters_m_, spatial_decoder_filters_v_, learning_rate, beta1, beta2, latent_beta1_correction, latent_beta2_correction, epsilon);
                spatial_decoder_bias_.adam_update(d_spatial_decoder_bias_, spatial_decoder_bias_m_, spatial_decoder_bias_v_, learning_rate, beta1, beta2, latent_beta1_correction, latent_beta2_correction, epsilon);
            }
            return;
        }

        if(!optimizer_.compare_string("sgd"))
            throw exception("ConvolutionalVariationalAutoEncoder: optimizer must be adam or sgd.", path_);

        apply_sgd_update(encoder_filters_, d_encoder_filters_, learning_rate);
        apply_sgd_update(encoder_bias_, d_encoder_bias_, learning_rate);
        apply_sgd_update(decoder_filters_, d_decoder_filters_, learning_rate);
        output_bias_ -= learning_rate * d_output_bias;

        if(update_latent)
        {
            apply_sgd_update(spatial_mean_filters_, d_spatial_mean_filters_, learning_rate);
            apply_sgd_update(spatial_mean_bias_, d_spatial_mean_bias_, learning_rate);
            apply_sgd_update(spatial_log_variance_filters_, d_spatial_log_variance_filters_, learning_rate);
            apply_sgd_update(spatial_log_variance_bias_, d_spatial_log_variance_bias_, learning_rate);
            apply_sgd_update(spatial_decoder_filters_, d_spatial_decoder_filters_, learning_rate);
            apply_sgd_update(spatial_decoder_bias_, d_spatial_decoder_bias_, learning_rate);
        }
    }

    void
    apply_sgd_update(matrix & values, const matrix & gradients, float learning_rate)
    {
        values.multiply_and_accumulate(gradients, -learning_rate);
    }

    void
    apply_adam_scalar_update(float & value, float gradient, float & m, float & v, float learning_rate, float beta1, float beta2, float beta1_correction, float beta2_correction, float epsilon)
    {
        m = beta1 * m + (1.0f - beta1) * gradient;
        v = beta2 * v + (1.0f - beta2) * gradient * gradient;

        const float m_hat = m / beta1_correction;
        const float v_hat = v / beta2_correction;
        value -= learning_rate * m_hat / (std::sqrt(v_hat) + epsilon);
    }

    void
    reset_adam_state()
    {
        adam_step_ = 0;
        dense_adam_step_ = 0;
        output_bias_m_ = 0.0f;
        output_bias_v_ = 0.0f;

        encoder_filters_m_.reset();
        encoder_filters_v_.reset();
        encoder_bias_m_.reset();
        encoder_bias_v_.reset();
        mean_weights_m_.reset();
        mean_weights_v_.reset();
        mean_bias_m_.reset();
        mean_bias_v_.reset();
        log_variance_weights_m_.reset();
        log_variance_weights_v_.reset();
        log_variance_bias_m_.reset();
        log_variance_bias_v_.reset();
        decoder_weights_m_.reset();
        decoder_weights_v_.reset();
        decoder_bias_m_.reset();
        decoder_bias_v_.reset();
        decoder_filters_m_.reset();
        decoder_filters_v_.reset();
        spatial_mean_filters_m_.reset();
        spatial_mean_filters_v_.reset();
        spatial_mean_bias_m_.reset();
        spatial_mean_bias_v_.reset();
        spatial_log_variance_filters_m_.reset();
        spatial_log_variance_filters_v_.reset();
        spatial_log_variance_bias_m_.reset();
        spatial_log_variance_bias_v_.reset();
        spatial_decoder_filters_m_.reset();
        spatial_decoder_filters_v_.reset();
        spatial_decoder_bias_m_.reset();
        spatial_decoder_bias_v_.reset();
    }

    void
    compute_encoder_activation_gradient()
    {
        d_encoder_activation_.reshape(encoded_size_);

        const float * mean_weights = mean_weights_.data();
        const float * log_variance_weights = log_variance_weights_.data();
        const float * d_mean = d_mean_.data();
        const float * d_log_variance = d_log_variance_.data();
        float * d_encoder_activation = d_encoder_activation_.data();

        for(int i = 0; i < encoded_size_; ++i)
        {
            const int weight_base = i * latent_size_value_;
            float gradient = 0.0f;
            for(int l = 0; l < latent_size_value_; ++l)
                gradient += mean_weights[weight_base + l] * d_mean[l] +
                            log_variance_weights[weight_base + l] * d_log_variance[l];

            d_encoder_activation[i] = gradient;
        }
    }

    void
    save_weights()
    {
        std::ofstream stream(resolved_weights_filename_.empty() ? weights_filename_.as_string() : resolved_weights_filename_.string(), std::ios::binary);
        if(!stream.is_open())
            throw std::runtime_error("ConvolutionalVariationalAutoEncoder: could not open weight file for writing.");

        const int version = 3;
        const int latent_mode = latent_mode_value_ == LatentMode::Spatial ? 1 : 0;
        stream.write(reinterpret_cast<const char *>(&version), sizeof(version));
        stream.write(reinterpret_cast<const char *>(&latent_mode), sizeof(latent_mode));
        stream.write(reinterpret_cast<const char *>(&input_height_), sizeof(input_height_));
        stream.write(reinterpret_cast<const char *>(&input_width_), sizeof(input_width_));
        stream.write(reinterpret_cast<const char *>(&input_channels_), sizeof(input_channels_));
        stream.write(reinterpret_cast<const char *>(&encoded_height_), sizeof(encoded_height_));
        stream.write(reinterpret_cast<const char *>(&encoded_width_), sizeof(encoded_width_));
        stream.write(reinterpret_cast<const char *>(&latent_height_), sizeof(latent_height_));
        stream.write(reinterpret_cast<const char *>(&latent_width_), sizeof(latent_width_));
        stream.write(reinterpret_cast<const char *>(&latent_size_value_), sizeof(latent_size_value_));
        stream.write(reinterpret_cast<const char *>(&latent_maps_value_), sizeof(latent_maps_value_));
        stream.write(reinterpret_cast<const char *>(&feature_maps_value_), sizeof(feature_maps_value_));
        stream.write(reinterpret_cast<const char *>(&kernel_size_value_), sizeof(kernel_size_value_));
        stream.write(reinterpret_cast<const char *>(&latent_kernel_size_value_), sizeof(latent_kernel_size_value_));
        stream.write(reinterpret_cast<const char *>(&output_bias_), sizeof(output_bias_));

        write_matrix(stream, encoder_filters_);
        write_matrix(stream, encoder_bias_);
        if(latent_mode_value_ == LatentMode::Spatial)
        {
            write_matrix(stream, spatial_mean_filters_);
            write_matrix(stream, spatial_mean_bias_);
            write_matrix(stream, spatial_log_variance_filters_);
            write_matrix(stream, spatial_log_variance_bias_);
            write_matrix(stream, spatial_decoder_filters_);
            write_matrix(stream, spatial_decoder_bias_);
        }
        else
        {
            write_matrix(stream, mean_weights_);
            write_matrix(stream, mean_bias_);
            write_matrix(stream, log_variance_weights_);
            write_matrix(stream, log_variance_bias_);
            write_matrix(stream, decoder_weights_);
            write_matrix(stream, decoder_bias_);
        }
        write_matrix(stream, decoder_filters_);
    }

    void
    load_weights()
    {
        std::ifstream stream(resolved_weights_filename_.empty() ? weights_filename_.as_string() : resolved_weights_filename_.string(), std::ios::binary);
        if(!stream.is_open())
            throw std::runtime_error("ConvolutionalVariationalAutoEncoder: could not open weight file for reading.");

        int version = 0;
        int latent_mode = 0;
        int input_height = 0;
        int input_width = 0;
        int input_channels = 1;
        int encoded_height = 0;
        int encoded_width = 0;
        int latent_height = 0;
        int latent_width = 0;
        int latent_size = 0;
        int latent_maps = 0;
        int feature_maps = 0;
        int kernel_size = 0;
        int latent_kernel_size = 1;

        stream.read(reinterpret_cast<char *>(&version), sizeof(version));
        if(version == 1)
        {
            stream.read(reinterpret_cast<char *>(&input_height), sizeof(input_height));
            stream.read(reinterpret_cast<char *>(&input_width), sizeof(input_width));
            stream.read(reinterpret_cast<char *>(&encoded_height), sizeof(encoded_height));
            stream.read(reinterpret_cast<char *>(&encoded_width), sizeof(encoded_width));
            stream.read(reinterpret_cast<char *>(&latent_size), sizeof(latent_size));
            stream.read(reinterpret_cast<char *>(&feature_maps), sizeof(feature_maps));
            stream.read(reinterpret_cast<char *>(&kernel_size), sizeof(kernel_size));
            stream.read(reinterpret_cast<char *>(&output_bias_), sizeof(output_bias_));

            if(latent_mode_value_ != LatentMode::Dense ||
               input_height != input_height_ || input_width != input_width_ ||
               encoded_height != encoded_height_ || encoded_width != encoded_width_ ||
               latent_size != latent_size_value_ || feature_maps != feature_maps_value_ ||
               kernel_size != kernel_size_value_)
                throw std::runtime_error("ConvolutionalVariationalAutoEncoder: weight file shape does not match module parameters or input.");

            read_matrix(stream, encoder_filters_);
            read_matrix(stream, encoder_bias_);
            read_matrix(stream, mean_weights_);
            read_matrix(stream, mean_bias_);
            read_matrix(stream, log_variance_weights_);
            read_matrix(stream, log_variance_bias_);
            read_matrix(stream, decoder_weights_);
            read_matrix(stream, decoder_bias_);
            read_matrix(stream, decoder_filters_);
            return;
        }

        stream.read(reinterpret_cast<char *>(&latent_mode), sizeof(latent_mode));
        stream.read(reinterpret_cast<char *>(&input_height), sizeof(input_height));
        stream.read(reinterpret_cast<char *>(&input_width), sizeof(input_width));
        if(version >= 3)
            stream.read(reinterpret_cast<char *>(&input_channels), sizeof(input_channels));
        stream.read(reinterpret_cast<char *>(&encoded_height), sizeof(encoded_height));
        stream.read(reinterpret_cast<char *>(&encoded_width), sizeof(encoded_width));
        stream.read(reinterpret_cast<char *>(&latent_height), sizeof(latent_height));
        stream.read(reinterpret_cast<char *>(&latent_width), sizeof(latent_width));
        stream.read(reinterpret_cast<char *>(&latent_size), sizeof(latent_size));
        stream.read(reinterpret_cast<char *>(&latent_maps), sizeof(latent_maps));
        stream.read(reinterpret_cast<char *>(&feature_maps), sizeof(feature_maps));
        stream.read(reinterpret_cast<char *>(&kernel_size), sizeof(kernel_size));
        stream.read(reinterpret_cast<char *>(&latent_kernel_size), sizeof(latent_kernel_size));
        stream.read(reinterpret_cast<char *>(&output_bias_), sizeof(output_bias_));

        const int expected_latent_mode = latent_mode_value_ == LatentMode::Spatial ? 1 : 0;
        bool shape_mismatch = (version != 2 && version != 3) || latent_mode != expected_latent_mode ||
            input_height != input_height_ || input_width != input_width_ ||
            encoded_height != encoded_height_ || encoded_width != encoded_width_ ||
            feature_maps != feature_maps_value_ || kernel_size != kernel_size_value_;

        if(version == 2)
            shape_mismatch = shape_mismatch || input_channels_ != 1;
        else
            shape_mismatch = shape_mismatch || input_channels != input_channels_;

        if(latent_mode_value_ == LatentMode::Spatial)
            shape_mismatch = shape_mismatch ||
                latent_height != latent_height_ || latent_width != latent_width_ ||
                latent_maps != latent_maps_value_ || latent_kernel_size != latent_kernel_size_value_;
        else
            shape_mismatch = shape_mismatch || latent_size != latent_size_value_;

        if(shape_mismatch)
            throw std::runtime_error("ConvolutionalVariationalAutoEncoder: weight file shape does not match module parameters or input.");

        read_matrix(stream, encoder_filters_);
        read_matrix(stream, encoder_bias_);
        if(latent_mode_value_ == LatentMode::Spatial)
        {
            read_matrix(stream, spatial_mean_filters_);
            read_matrix(stream, spatial_mean_bias_);
            read_matrix(stream, spatial_log_variance_filters_);
            read_matrix(stream, spatial_log_variance_bias_);
            read_matrix(stream, spatial_decoder_filters_);
            read_matrix(stream, spatial_decoder_bias_);
        }
        else
        {
            read_matrix(stream, mean_weights_);
            read_matrix(stream, mean_bias_);
            read_matrix(stream, log_variance_weights_);
            read_matrix(stream, log_variance_bias_);
            read_matrix(stream, decoder_weights_);
            read_matrix(stream, decoder_bias_);
        }
        read_matrix(stream, decoder_filters_);
    }

    void
    report_profile_if_needed()
    {
        if(!profile_.as_bool())
            return;

        ++profile_counters_.ticks;
        const int interval = std::max(1, profile_interval_.as_int());
        if(profile_counters_.ticks < interval)
            return;

        const double ticks = std::max(1, profile_counters_.ticks);
        const double train_ticks = std::max(1, profile_counters_.train_ticks);

        std::cout << std::fixed << std::setprecision(4)
                  << "CVAE profile"
                  << " ticks=" << profile_counters_.ticks
                  << " train_ticks=" << profile_counters_.train_ticks
                  << " dense_updates=" << profile_counters_.dense_updates
                  << " avg_ms:"
                  << " encode=" << profile_counters_.encode / ticks
                  << " decode=" << profile_counters_.decode / ticks
                  << " publish=" << profile_counters_.publish / ticks
                  << " loss=" << profile_counters_.loss / ticks
                  << " train=" << profile_counters_.train / ticks
                  << " save=" << profile_counters_.save / ticks
                  << '\n';

        if(profile_counters_.train_ticks > 0)
        {
            std::cout << std::fixed << std::setprecision(4)
                      << "CVAE train profile"
                      << " avg_train_ms:"
                      << " output_grad=" << profile_counters_.output_gradient / train_ticks
                      << " decoder_filter_grad=" << profile_counters_.decoder_filter_gradient / train_ticks
                      << " decoder_activation_grad=" << profile_counters_.decoder_activation_gradient / train_ticks
                      << " decoder_relu_dense=" << profile_counters_.decoder_relu_dense / train_ticks
                      << " latent_grad=" << profile_counters_.latent_gradients / train_ticks
                      << " encoder_dense_grad=" << profile_counters_.encoder_dense_gradients / train_ticks
                      << " encoder_activation_grad=" << profile_counters_.encoder_activation_gradient / train_ticks
                      << " encoder_filter_grad=" << profile_counters_.encoder_filter_gradient / train_ticks
                      << " optimizer=" << profile_counters_.optimizer / train_ticks
                      << '\n';
        }

        profile_counters_.reset();
    }

    void
    Tick()
    {
        validate_reconstruction_source();

        if(effort_.connected() && !effort_.empty() && effort_.sum() <= 0.0f)
            return;

        if(input_.is_uninitialized() || input_.empty())
            return;
        if(input_.rank() != 2 && input_.rank() != 3)
            return;

        if(!initialized_)
            initialize_for_input();
        else if(input_.shape(0) != input_height_ || input_.shape(1) != input_width_ ||
                (input_.rank() == 3 ? input_.shape(2) : 1) != input_channels_)
            throw exception("ConvolutionalVariationalAutoEncoder: INPUT shape changed after initialization.", path_);

        const bool profiling = profile_.as_bool();
        auto phase_start = clock_type::now();

        encode();
        if(profiling)
        {
            profile_counters_.encode += elapsed_ms(phase_start);
            phase_start = clock_type::now();
        }

        decode();
        if(profiling)
        {
            profile_counters_.decode += elapsed_ms(phase_start);
            phase_start = clock_type::now();
        }

        publish_latent();
        if(profiling)
        {
            profile_counters_.publish += elapsed_ms(phase_start);
            phase_start = clock_type::now();
        }

        compute_losses();
        if(profiling)
        {
            profile_counters_.loss += elapsed_ms(phase_start);
            phase_start = clock_type::now();
        }

        if(train_.as_bool() && should_train_this_tick())
        {
            const bool use_teacher_forced_reconstruction = reconstruction_source_is(reconstruction_source_top_down);
            if(use_teacher_forced_reconstruction)
            {
                displayed_output_.copy(output_);
                training_reconstruction_ = true;
                decode();
            }

            train_step();
            if(use_teacher_forced_reconstruction)
            {
                training_reconstruction_ = false;
                output_.copy(displayed_output_);
            }

            if(profiling)
            {
                ++profile_counters_.train_ticks;
                profile_counters_.train += elapsed_ms(phase_start);
                phase_start = clock_type::now();
            }
            if(save_weights_.as_bool())
            {
                save_weights();
                if(profiling)
                    profile_counters_.save += elapsed_ms(phase_start);
            }
        }

        report_profile_if_needed();
    }

    bool
    should_train_this_tick()
    {
        train_interval_value_ = std::max(1, train_interval_.as_int());
        const bool should_train = (train_tick_ % train_interval_value_) == 0;
        ++train_tick_;
        return should_train;
    }

    bool
    should_update_dense_this_training_step()
    {
        dense_train_interval_value_ = std::max(1, dense_train_interval_.as_int());
        const bool should_update = (dense_train_tick_ % dense_train_interval_value_) == 0;
        ++dense_train_tick_;
        return should_update;
    }
};

INSTALL_CLASS(ConvolutionalVariationalAutoEncoder)
