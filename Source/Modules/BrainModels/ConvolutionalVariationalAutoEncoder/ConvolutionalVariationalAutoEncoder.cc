#include "ikaros.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <string>

using namespace ikaros;

namespace
{
    // CVAE rank-3 convention follows Ikaros image tensors: channels, height, width.
    int
    cvae_channels(const matrix & values)
    {
        return values.rank() == 3 ? values.shape(0) : 1;
    }
}

class ConvolutionalVariationalAutoEncoder: public Module
{
    static constexpr int reconstruction_source_sample = 0;
    static constexpr int reconstruction_source_mean = 1;
    static constexpr int reconstruction_source_top_down = 2;
    static constexpr int output_activation_linear = 0;
    static constexpr int output_activation_sigmoid = 1;
    static constexpr int padding_valid = 0;
    static constexpr int padding_same = 1;

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
    parameter padding_;
    parameter learning_rate_;
    parameter optimizer_;
    parameter adam_beta1_;
    parameter adam_beta2_;
    parameter adam_epsilon_;
    parameter beta_;
    parameter train_;
    parameter train_interval_;
    parameter dense_train_interval_;
    parameter sample_;
    parameter reconstruction_source_;
    parameter output_activation_;

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

    struct AdamStepParameters
    {
        float beta1 = 0.0f;
        float beta2 = 0.0f;
        float epsilon = 0.0f;
        float beta1_correction = 0.0f;
        float beta2_correction = 0.0f;
    };

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
    matrix reconstruction_values_;
    matrix reconstruction_error_;
    matrix kl_terms_;
    matrix exp_log_variance_;

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
    matrix d_mean_weights_;
    matrix d_log_variance_weights_;
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
    matrix output_bias_;
    matrix d_output_bias_;
    matrix output_bias_m_;
    matrix output_bias_v_;

    std::mt19937 rng_{std::random_device{}()};

    void
    Init()
    {
        Bind(latent_mode_, "latent_mode");
        Bind(latent_size_, "latent_size");
        Bind(latent_maps_, "latent_maps");
        Bind(latent_kernel_size_, "latent_kernel_size");
        Bind(feature_maps_, "feature_maps");
        Bind(kernel_size_, "kernel_size");
        Bind(padding_, "padding");
        Bind(learning_rate_, "learning_rate");
        Bind(optimizer_, "optimizer");
        Bind(adam_beta1_, "adam_beta1");
        Bind(adam_beta2_, "adam_beta2");
        Bind(adam_epsilon_, "adam_epsilon");
        Bind(beta_, "beta");
        Bind(train_, "train");
        Bind(train_interval_, "train_interval");
        Bind(dense_train_interval_, "dense_train_interval");
        Bind(sample_, "sample");
        Bind(reconstruction_source_, "reconstruction_source");
        Bind(output_activation_, "output_activation");

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

        Bind(encoder_filters_, "ENCODER_FILTERS");
        Bind(encoder_bias_, "ENCODER_BIAS");
        Bind(decoder_filters_, "DECODER_FILTERS");
        Bind(output_bias_, "OUTPUT_BIAS");
        Bind(mean_weights_, "MEAN_WEIGHTS");
        Bind(mean_bias_, "MEAN_BIAS");
        Bind(log_variance_weights_, "LOG_VARIANCE_WEIGHTS");
        Bind(log_variance_bias_, "LOG_VARIANCE_BIAS");
        Bind(decoder_weights_, "DECODER_WEIGHTS");
        Bind(decoder_bias_, "DECODER_BIAS");
        Bind(spatial_mean_filters_, "SPATIAL_MEAN_FILTERS");
        Bind(spatial_mean_bias_, "SPATIAL_MEAN_BIAS");
        Bind(spatial_log_variance_filters_, "SPATIAL_LOG_VARIANCE_FILTERS");
        Bind(spatial_log_variance_bias_, "SPATIAL_LOG_VARIANCE_BIAS");
        Bind(spatial_decoder_filters_, "SPATIAL_DECODER_FILTERS");
        Bind(spatial_decoder_bias_, "SPATIAL_DECODER_BIAS");

        Bind(reconstruction_values_, "RECONSTRUCTION_VALUES");
        Bind(reconstruction_error_, "RECONSTRUCTION_ERROR");
        Bind(d_output_, "D_OUTPUT");
        Bind(latent_mean_values_, "LATENT_MEAN_VALUES");
        Bind(latent_log_variance_values_, "LATENT_LOG_VARIANCE_VALUES");
        Bind(latent_stddev_, "LATENT_STDDEV");
        Bind(latent_epsilon_, "LATENT_EPSILON");
        Bind(latent_values_, "LATENT_VALUES");
        Bind(kl_terms_, "KL_TERMS");
        Bind(exp_log_variance_, "EXP_LOG_VARIANCE");

        latent_mode_value_ = parse_latent_mode(latent_mode_.as_string());
        latent_size_value_ = std::max(1, latent_size_.as_int());
        latent_maps_value_ = std::max(1, latent_maps_.as_int());
        latent_kernel_size_value_ = std::max(1, latent_kernel_size_.as_int());
        feature_maps_value_ = std::max(1, feature_maps_.as_int());
        kernel_size_value_ = std::max(1, kernel_size_.as_int());
        train_interval_value_ = std::max(1, train_interval_.as_int());
        dense_train_interval_value_ = std::max(1, dense_train_interval_.as_int());
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
        if(!sample_.as_bool() && reconstruction_source_is(reconstruction_source_sample))
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

    void
    validate_output_activation() const
    {
        const int activation = output_activation_.as_int();
        if(activation >= output_activation_linear && activation <= output_activation_sigmoid)
            return;
        throw exception("ConvolutionalVariationalAutoEncoder: output_activation must be linear or sigmoid.", path_);
    }

    void
    validate_padding() const
    {
        const int padding = padding_.as_int();
        if(padding >= padding_valid && padding <= padding_same)
            return;
        throw exception("ConvolutionalVariationalAutoEncoder: padding must be valid or same.", path_);
    }

    matrix::convolution_padding
    convolution_padding() const
    {
        return padding_.as_int() == padding_same ? matrix::convolution_padding::same : matrix::convolution_padding::valid;
    }

    int
    convolution_output_size(int input_size, int kernel_size) const
    {
        return padding_.as_int() == padding_same ? input_size : input_size - kernel_size + 1;
    }

    void
    require_output_shape(const matrix & output, const matrix & values, const std::string & name) const
    {
        if(output.is_uninitialized() || output.shape() != values.shape())
            throw exception("ConvolutionalVariationalAutoEncoder: output \"" + name + "\" has the wrong startup shape.", path_);
    }

    void
    require_state_shape(const matrix & state, const std::vector<int> & shape, const std::string & name) const
    {
        if(state.is_uninitialized() || state.shape() != shape)
            throw exception("ConvolutionalVariationalAutoEncoder: state \"" + name + "\" has the wrong startup shape.", path_);
    }

    void
    require_latent_state_shapes(const std::vector<int> & shape) const
    {
        require_state_shape(latent_mean_values_, shape, "LATENT_MEAN_VALUES");
        require_state_shape(latent_log_variance_values_, shape, "LATENT_LOG_VARIANCE_VALUES");
        require_state_shape(latent_stddev_, shape, "LATENT_STDDEV");
        require_state_shape(latent_epsilon_, shape, "LATENT_EPSILON");
        require_state_shape(latent_values_, shape, "LATENT_VALUES");
        require_state_shape(kl_terms_, shape, "KL_TERMS");
        require_state_shape(exp_log_variance_, shape, "EXP_LOG_VARIANCE");
    }

    void
    initialize_shared_weights()
    {
        encoder_filters_.fill_xavier_uniform(rng_, kernel_size_value_ * kernel_size_value_ * input_channels_);
        decoder_filters_.fill_xavier_uniform(rng_, kernel_size_value_ * kernel_size_value_ * feature_maps_value_);
    }

    void
    initialize_spatial_weights()
    {
        initialize_shared_weights();
        spatial_mean_filters_.fill_xavier_uniform(rng_, latent_kernel_size_value_ * latent_kernel_size_value_ * feature_maps_value_);
        spatial_log_variance_filters_.fill_xavier_uniform(rng_, latent_kernel_size_value_ * latent_kernel_size_value_ * feature_maps_value_);
        spatial_decoder_filters_.fill_xavier_uniform(rng_, latent_kernel_size_value_ * latent_kernel_size_value_ * latent_maps_value_);
    }

    void
    initialize_dense_weights()
    {
        initialize_shared_weights();
        mean_weights_.fill_xavier_uniform(rng_, encoded_size_);
        log_variance_weights_.fill_xavier_uniform(rng_, encoded_size_);
        decoder_weights_.fill_xavier_uniform(rng_, latent_size_value_);
    }

    void
    reset_common_initial_state()
    {
        encoder_bias_.reset();
        output_bias_.reset();
        latent_stddev_.set(1.0f);
        latent_epsilon_.reset();
        latent_values_.reset();
        reset_adam_state();
    }

    void
    finish_initialization()
    {
        require_output_shape(output_, input_, "OUTPUT");
        require_output_shape(latent_mean_, latent_mean_values_, "LATENT_MEAN");
        require_output_shape(latent_log_variance_, latent_log_variance_values_, "LATENT_LOG_VARIANCE");
        require_output_shape(latent_sample_, latent_values_, "LATENT_SAMPLE");

        initialized_ = true;
        train_tick_ = 0;
        dense_train_tick_ = 0;
    }

    void
    require_common_state_shapes()
    {
        const std::vector<int> filter_shape = input_channels_ == 1 ?
            std::vector<int>{feature_maps_value_, kernel_size_value_, kernel_size_value_} :
            std::vector<int>{feature_maps_value_, input_channels_, kernel_size_value_, kernel_size_value_};
        require_state_shape(encoder_filters_, filter_shape, "ENCODER_FILTERS");
        require_state_shape(encoder_bias_, {feature_maps_value_}, "ENCODER_BIAS");
        require_state_shape(decoder_filters_, filter_shape, "DECODER_FILTERS");
        require_state_shape(output_bias_, {input_channels_}, "OUTPUT_BIAS");
        require_state_shape(reconstruction_values_, input_.shape(), "RECONSTRUCTION_VALUES");
        require_state_shape(reconstruction_error_, input_.shape(), "RECONSTRUCTION_ERROR");
        require_state_shape(d_output_, input_.shape(), "D_OUTPUT");
    }

    void
    allocate_common_work_buffers()
    {
        encoder_pre_activation_.realloc(feature_maps_value_, encoded_height_, encoded_width_);
        encoder_activation_.realloc(feature_maps_value_, encoded_height_, encoded_width_);
        d_output_bias_.realloc(input_channels_);
        decoder_activation_.realloc(encoded_size_);
        decoder_pre_activation_.realloc(encoded_size_);
        d_decoder_filters_.realloc(decoder_filters_.shape());
        d_decoder_activation_.realloc(feature_maps_value_, encoded_height_, encoded_width_);
        d_encoder_filters_.realloc(encoder_filters_.shape());
        d_encoder_bias_.realloc(feature_maps_value_);
    }

    void
    allocate_shared_optimizer_buffers()
    {
        encoder_filters_m_.realloc(encoder_filters_.shape());
        encoder_filters_v_.realloc(encoder_filters_.shape());
        encoder_bias_m_.realloc(encoder_bias_.shape());
        encoder_bias_v_.realloc(encoder_bias_.shape());
        decoder_filters_m_.realloc(decoder_filters_.shape());
        decoder_filters_v_.realloc(decoder_filters_.shape());
        output_bias_m_.realloc(output_bias_.shape());
        output_bias_v_.realloc(output_bias_.shape());
    }

    void
    require_spatial_state_shapes()
    {
        const std::vector<int> spatial_filter_shape{latent_maps_value_, feature_maps_value_, latent_kernel_size_value_, latent_kernel_size_value_};
        require_state_shape(spatial_mean_filters_, spatial_filter_shape, "SPATIAL_MEAN_FILTERS");
        require_state_shape(spatial_mean_bias_, {latent_maps_value_}, "SPATIAL_MEAN_BIAS");
        require_state_shape(spatial_log_variance_filters_, spatial_filter_shape, "SPATIAL_LOG_VARIANCE_FILTERS");
        require_state_shape(spatial_log_variance_bias_, {latent_maps_value_}, "SPATIAL_LOG_VARIANCE_BIAS");
        require_state_shape(spatial_decoder_filters_, spatial_filter_shape, "SPATIAL_DECODER_FILTERS");
        require_state_shape(spatial_decoder_bias_, {feature_maps_value_}, "SPATIAL_DECODER_BIAS");

        const std::vector<int> spatial_latent_shape{latent_maps_value_, latent_height_, latent_width_};
        require_latent_state_shapes(spatial_latent_shape);
    }

    void
    allocate_spatial_work_buffers()
    {
        d_decoder_pre_activation_.realloc(feature_maps_value_, encoded_height_, encoded_width_);
        d_latent_.realloc(latent_maps_value_, latent_height_, latent_width_);
        d_mean_.realloc(latent_maps_value_, latent_height_, latent_width_);
        d_log_variance_.realloc(latent_maps_value_, latent_height_, latent_width_);
        d_encoder_activation_.realloc(feature_maps_value_, encoded_height_, encoded_width_);
        d_spatial_mean_filters_.realloc(spatial_mean_filters_.shape());
        d_spatial_mean_bias_.realloc(spatial_mean_bias_.shape());
        d_spatial_log_variance_filters_.realloc(spatial_log_variance_filters_.shape());
        d_spatial_log_variance_bias_.realloc(spatial_log_variance_bias_.shape());
        d_spatial_decoder_filters_.realloc(spatial_decoder_filters_.shape());
        d_spatial_decoder_bias_.realloc(spatial_decoder_bias_.shape());
    }

    void
    allocate_spatial_optimizer_buffers()
    {
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
    }

    void
    initialize_spatial_mode()
    {
        require_spatial_state_shapes();
        allocate_spatial_work_buffers();
        allocate_shared_optimizer_buffers();
        allocate_spatial_optimizer_buffers();

        reset_common_initial_state();
        spatial_mean_bias_.reset();
        spatial_log_variance_bias_.reset();
        spatial_decoder_bias_.reset();
        initialize_spatial_weights();
        finish_initialization();
    }

    void
    require_dense_state_shapes()
    {
        require_state_shape(mean_weights_, {encoded_size_, latent_size_value_}, "MEAN_WEIGHTS");
        require_state_shape(mean_bias_, {latent_size_value_}, "MEAN_BIAS");
        require_state_shape(log_variance_weights_, {encoded_size_, latent_size_value_}, "LOG_VARIANCE_WEIGHTS");
        require_state_shape(log_variance_bias_, {latent_size_value_}, "LOG_VARIANCE_BIAS");
        require_latent_state_shapes({latent_size_value_});
        require_state_shape(decoder_weights_, {latent_size_value_, encoded_size_}, "DECODER_WEIGHTS");
        require_state_shape(decoder_bias_, {encoded_size_}, "DECODER_BIAS");
    }

    void
    allocate_dense_work_buffers()
    {
        latent_mean_projection_.realloc(latent_size_value_);
        latent_log_variance_projection_.realloc(latent_size_value_);
        decoder_projection_.realloc(encoded_size_);
        d_decoder_pre_activation_.realloc(encoded_size_);
        d_decoder_weights_.realloc(decoder_weights_.shape());
        d_latent_.realloc(latent_size_value_);
        d_mean_.realloc(latent_size_value_);
        d_log_variance_.realloc(latent_size_value_);
        d_mean_weights_.realloc(mean_weights_.shape());
        d_log_variance_weights_.realloc(log_variance_weights_.shape());
        d_encoder_activation_.realloc(encoded_size_);
    }

    void
    allocate_dense_optimizer_buffers()
    {
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
    }

    void
    initialize_dense_mode()
    {
        require_dense_state_shapes();
        allocate_dense_work_buffers();
        allocate_shared_optimizer_buffers();
        allocate_dense_optimizer_buffers();

        reset_common_initial_state();
        mean_bias_.reset();
        log_variance_bias_.reset();
        decoder_bias_.reset();
        initialize_dense_weights();
        finish_initialization();
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

        input_height_ = input_.rows();
        input_width_ = input_.cols();
        input_channels_ = cvae_channels(input_);

        if(padding_.as_int() == padding_valid && (input_height_ < kernel_size_value_ || input_width_ < kernel_size_value_))
            throw exception("ConvolutionalVariationalAutoEncoder: kernel_size must fit inside INPUT.", path_);

        encoded_height_ = convolution_output_size(input_height_, kernel_size_value_);
        encoded_width_ = convolution_output_size(input_width_, kernel_size_value_);
        encoded_size_ = encoded_height_ * encoded_width_ * feature_maps_value_;

        if(latent_mode_value_ == LatentMode::Spatial)
        {
            if(padding_.as_int() == padding_valid && (encoded_height_ < latent_kernel_size_value_ || encoded_width_ < latent_kernel_size_value_))
                throw exception("ConvolutionalVariationalAutoEncoder: latent_kernel_size must fit inside the encoded feature map.", path_);

            latent_height_ = convolution_output_size(encoded_height_, latent_kernel_size_value_);
            latent_width_ = convolution_output_size(encoded_width_, latent_kernel_size_value_);
        }

        require_common_state_shapes();
        allocate_common_work_buffers();

        if(latent_mode_value_ == LatentMode::Spatial)
        {
            initialize_spatial_mode();
            return;
        }

        initialize_dense_mode();
    }

    void
    encode()
    {
        encode_input_to_features();
        encoder_activation_.relu(encoder_pre_activation_);

        if(latent_mode_value_ == LatentMode::Spatial)
        {
            encode_spatial_latent();
            return;
        }

        encoder_activation_.reshape(encoded_size_);
        latent_mean_projection_.dense_forward(encoder_activation_, mean_weights_);
        latent_log_variance_projection_.dense_forward(encoder_activation_, log_variance_weights_);
        encoder_activation_.reshape(feature_maps_value_, encoded_height_, encoded_width_);

        latent_mean_values_.add(latent_mean_projection_, mean_bias_);
        latent_log_variance_values_.add(latent_log_variance_projection_, log_variance_bias_);
        latent_log_variance_values_.clip(-20.0f, 10.0f);

        if(sample_.as_bool())
        {
            latent_stddev_.exp_scaled(latent_log_variance_values_, 0.5f);
            latent_epsilon_.fill_gaussian(rng_);
            latent_values_.sample_gaussian(latent_mean_values_, latent_stddev_, latent_epsilon_);
        }
        else
            latent_values_.copy(latent_mean_values_);
    }

    void
    decode(bool publish_output = true)
    {
        if(latent_mode_value_ == LatentMode::Spatial)
        {
            decode_spatial_latent(publish_output);
            return;
        }

        decoder_projection_.dense_forward(decoder_latent_input(), decoder_weights_);
        decoder_pre_activation_.add(decoder_projection_, decoder_bias_);
        decoder_activation_.relu(decoder_pre_activation_);

        decode_features_to_output(publish_output);
    }

    void
    encode_input_to_features()
    {
        if(input_channels_ == 1)
            encoder_pre_activation_.conv2_filterbank(input_, encoder_filters_, encoder_bias_, convolution_padding());
        else
            encoder_pre_activation_.conv2_channel_filterbank(input_, encoder_filters_, encoder_bias_, convolution_padding());
    }

    void
    decode_features_to_output(bool publish_output)
    {
        decoder_activation_.reshape(feature_maps_value_, encoded_height_, encoded_width_);
        if(input_channels_ == 1)
            reconstruction_values_.conv2_filterbank_backward_input(decoder_activation_, decoder_filters_, convolution_padding());
        else
            reconstruction_values_.conv2_channel_filterbank_backward_input(decoder_activation_, decoder_filters_, convolution_padding());
        add_output_bias();
        apply_output_activation();
        if(publish_output)
            output_.copy(reconstruction_values_);
        decoder_activation_.reshape(encoded_size_);
    }

    void
    add_output_bias()
    {
        if(reconstruction_values_.rank() == 2)
        {
            reconstruction_values_.add(output_bias_(0));
            return;
        }

        reconstruction_values_.add_channel_bias(output_bias_);
    }

    void
    apply_output_activation()
    {
        if(output_activation_.as_int() != output_activation_sigmoid)
            return;

        reconstruction_values_.sigmoid();
    }

    const matrix &
    compute_output_gradient(float output_scale)
    {
        d_output_.subtract(reconstruction_values_, input_).scale(output_scale);

        if(output_activation_.as_int() == output_activation_sigmoid)
        {
            d_output_.multiply_sigmoid_derivative(reconstruction_values_);
        }

        if(d_output_.rank() == 2)
            d_output_bias_(0) = d_output_.sum();
        else
            d_output_bias_.sum_last_two_dimensions(d_output_);

        return d_output_bias_;
    }

    void
    encode_spatial_latent()
    {
        latent_mean_values_.conv2_channel_filterbank(encoder_activation_, spatial_mean_filters_, spatial_mean_bias_, convolution_padding());
        latent_log_variance_values_.conv2_channel_filterbank(encoder_activation_, spatial_log_variance_filters_, spatial_log_variance_bias_, convolution_padding());
        latent_log_variance_values_.clip(-20.0f, 10.0f);

        if(sample_.as_bool())
        {
            latent_stddev_.exp_scaled(latent_log_variance_values_, 0.5f);
            latent_epsilon_.fill_gaussian(rng_);
            latent_values_.sample_gaussian(latent_mean_values_, latent_stddev_, latent_epsilon_);
        }
        else
            latent_values_.copy(latent_mean_values_);
    }

    void
    decode_spatial_latent(bool publish_output)
    {
        decoder_pre_activation_.reshape(feature_maps_value_, encoded_height_, encoded_width_);
        decoder_activation_.reshape(feature_maps_value_, encoded_height_, encoded_width_);

        decoder_pre_activation_.conv2_channel_filterbank_backward_input(decoder_latent_input(), spatial_decoder_filters_, convolution_padding());
        decoder_pre_activation_.add_channel_bias(spatial_decoder_bias_);
        decoder_activation_.relu(decoder_pre_activation_);
        decode_features_to_output(publish_output);
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

        kl_terms_.multiply(latent_mean_values_, latent_mean_values_);
        exp_log_variance_.exp_scaled(latent_log_variance_values_, 1.0f);
        kl_terms_.add(exp_log_variance_).subtract(latent_log_variance_values_).subtract(1.0f);
        const float kl = 0.5f * kl_terms_.sum() / std::max(1, latent_values_.size());

        reconstruction_loss_(0) = reconstruction;
        kl_loss_(0) = kl;
        loss_(0) = reconstruction + beta_.as_float() * kl;
    }

    void
    compute_decoder_output_gradients(float output_scale)
    {
        compute_output_gradient(output_scale);

        decoder_activation_.reshape(feature_maps_value_, encoded_height_, encoded_width_);
        if(input_channels_ == 1)
            d_decoder_filters_.conv2_filterbank_backward_filters(d_output_, decoder_activation_, kernel_size_value_, kernel_size_value_, convolution_padding());
        else
            d_decoder_filters_.conv2_channel_filterbank_backward_filters(d_output_, decoder_activation_, kernel_size_value_, kernel_size_value_, convolution_padding());

        d_decoder_activation_.reshape(feature_maps_value_, encoded_height_, encoded_width_);
        if(input_channels_ == 1)
            d_decoder_activation_.conv2_filterbank(d_output_, decoder_filters_, convolution_padding());
        else
            d_decoder_activation_.conv2_channel_filterbank(d_output_, decoder_filters_, convolution_padding());
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

        const float output_scale = 1.0f / std::max(1, input_.size());
        compute_decoder_output_gradients(output_scale);
        decoder_activation_.reshape(encoded_size_);
        d_decoder_activation_.reshape(encoded_size_);

        d_decoder_pre_activation_.relu_backward(d_decoder_activation_, decoder_pre_activation_);

        d_decoder_weights_.outer_product(decoder_latent_input(), d_decoder_pre_activation_);
        d_latent_.dense_backward_input(decoder_weights_, d_decoder_pre_activation_);

        const float beta = beta_.as_float();
        const float kl_scale = beta / std::max(1, latent_size_value_);
        const bool source_sample = effective_reconstruction_source_is(reconstruction_source_sample);
        const bool source_mean = effective_reconstruction_source_is(reconstruction_source_mean);

        if(source_sample)
        {
            d_mean_.add_scaled(d_latent_, latent_mean_values_, kl_scale);
            d_log_variance_.latent_log_variance_gradient(d_latent_, latent_epsilon_, latent_stddev_, latent_log_variance_values_, kl_scale);
        }
        else if(source_mean)
        {
            d_mean_.add_scaled(d_latent_, latent_mean_values_, kl_scale);
            d_log_variance_.reset();
        }
        else
        {
            d_mean_.scale(latent_mean_values_, kl_scale);
        }
        if(!source_sample)
            d_log_variance_.exp_minus_one_scaled(latent_log_variance_values_, 0.5f * kl_scale);

        encoder_activation_.reshape(encoded_size_);
        d_mean_weights_.outer_product(encoder_activation_, d_mean_);
        d_log_variance_weights_.outer_product(encoder_activation_, d_log_variance_);

        compute_encoder_activation_gradient();

        encoder_activation_.reshape(feature_maps_value_, encoded_height_, encoded_width_);
        d_mean_.reshape(latent_size_value_);
        d_log_variance_.reshape(latent_size_value_);

        d_encoder_activation_.reshape(feature_maps_value_, encoded_height_, encoded_width_);
        if(input_channels_ == 1)
        {
            d_encoder_filters_.conv2_filterbank_backward_filters_relu(input_, d_encoder_activation_, encoder_pre_activation_, kernel_size_value_, kernel_size_value_, convolution_padding());
            d_encoder_bias_.sum_last_two_dimensions_relu(d_encoder_activation_, encoder_pre_activation_);
        }
        else
        {
            d_decoder_activation_.reshape(feature_maps_value_, encoded_height_, encoded_width_);
            d_decoder_activation_.relu_backward(d_encoder_activation_, encoder_pre_activation_);
            d_encoder_filters_.conv2_channel_filterbank_backward_filters(input_, d_decoder_activation_, kernel_size_value_, kernel_size_value_, convolution_padding());
            d_encoder_bias_.sum_last_two_dimensions(d_decoder_activation_);
        }

        d_encoder_activation_.reshape(encoded_size_);
        d_decoder_activation_.reshape(encoded_size_);

        apply_optimizer_update(learning_rate, d_decoder_pre_activation_);
    }

    void
    train_step_spatial()
    {
        const float learning_rate = learning_rate_.as_float();
        if(learning_rate <= 0.0f)
            return;

        const float output_scale = 1.0f / std::max(1, input_.size());
        compute_decoder_output_gradients(output_scale);

        decoder_pre_activation_.reshape(feature_maps_value_, encoded_height_, encoded_width_);
        d_decoder_pre_activation_.reshape(feature_maps_value_, encoded_height_, encoded_width_);
        d_decoder_pre_activation_.relu_backward(d_decoder_activation_, decoder_pre_activation_);
        decoder_pre_activation_.reshape(encoded_size_);

        compute_spatial_decoder_gradients();

        compute_spatial_latent_gradients();

        compute_spatial_encoder_activation_gradient();

        if(input_channels_ == 1)
        {
            d_encoder_filters_.conv2_filterbank_backward_filters_relu(input_, d_encoder_activation_, encoder_pre_activation_, kernel_size_value_, kernel_size_value_, convolution_padding());
            d_encoder_bias_.sum_last_two_dimensions_relu(d_encoder_activation_, encoder_pre_activation_);
        }
        else
        {
            d_decoder_activation_.relu_backward(d_encoder_activation_, encoder_pre_activation_);
            d_encoder_filters_.conv2_channel_filterbank_backward_filters(input_, d_decoder_activation_, kernel_size_value_, kernel_size_value_, convolution_padding());
            d_encoder_bias_.sum_last_two_dimensions(d_decoder_activation_);
        }

        apply_spatial_optimizer_update(learning_rate);

        d_decoder_pre_activation_.reshape(encoded_size_);
        decoder_activation_.reshape(encoded_size_);
    }

    void
    compute_spatial_decoder_gradients()
    {
        d_decoder_pre_activation_.reshape(feature_maps_value_, encoded_height_, encoded_width_);
        d_latent_.conv2_channel_filterbank(d_decoder_pre_activation_, spatial_decoder_filters_, convolution_padding());
        d_spatial_decoder_filters_.conv2_channel_filterbank_backward_filters(d_decoder_pre_activation_, decoder_latent_input(), latent_kernel_size_value_, latent_kernel_size_value_, convolution_padding());
        d_spatial_decoder_bias_.sum_last_two_dimensions(d_decoder_pre_activation_);
        d_decoder_pre_activation_.reshape(encoded_size_);
    }

    void
    compute_spatial_latent_gradients()
    {
        const float beta = beta_.as_float();
        const float kl_scale = beta / std::max(1, latent_values_.size());

        if(effective_reconstruction_source_is(reconstruction_source_sample))
            d_mean_.latent_sample_gradients(d_log_variance_, d_latent_, latent_mean_values_, latent_epsilon_, latent_stddev_, latent_log_variance_values_, kl_scale);
        else if(effective_reconstruction_source_is(reconstruction_source_mean))
            d_mean_.latent_mean_gradients(d_log_variance_, d_latent_, latent_mean_values_, latent_log_variance_values_, kl_scale);
        else
            d_mean_.latent_kl_gradients(d_log_variance_, latent_mean_values_, latent_log_variance_values_, kl_scale);
    }

    void
    compute_spatial_encoder_activation_gradient()
    {
        d_encoder_activation_.conv2_channel_filterbank_backward(encoder_activation_, spatial_mean_filters_, d_mean_, d_spatial_mean_filters_, d_spatial_mean_bias_, convolution_padding());
        d_spatial_log_variance_filters_.conv2_channel_filterbank_backward_filters(encoder_activation_, d_log_variance_, latent_kernel_size_value_, latent_kernel_size_value_, convolution_padding());
        d_spatial_log_variance_bias_.sum_last_two_dimensions(d_log_variance_);
        d_decoder_activation_.conv2_channel_filterbank_backward_input(d_log_variance_, spatial_log_variance_filters_, convolution_padding());
        d_encoder_activation_.add(d_decoder_activation_);
    }

    AdamStepParameters
    adam_step_parameters(int step) const
    {
        AdamStepParameters parameters;
        parameters.beta1 = std::clamp(adam_beta1_.as_float(), 0.0f, 0.999999f);
        parameters.beta2 = std::clamp(adam_beta2_.as_float(), 0.0f, 0.999999f);
        parameters.epsilon = std::max(adam_epsilon_.as_float(), 1e-12f);
        parameters.beta1_correction = 1.0f - std::pow(parameters.beta1, static_cast<float>(step));
        parameters.beta2_correction = 1.0f - std::pow(parameters.beta2, static_cast<float>(step));
        return parameters;
    }

    void
    apply_shared_adam_update(float learning_rate, const AdamStepParameters & adam)
    {
        encoder_filters_.adam_update(d_encoder_filters_, encoder_filters_m_, encoder_filters_v_, learning_rate, adam.beta1, adam.beta2, adam.beta1_correction, adam.beta2_correction, adam.epsilon);
        encoder_bias_.adam_update(d_encoder_bias_, encoder_bias_m_, encoder_bias_v_, learning_rate, adam.beta1, adam.beta2, adam.beta1_correction, adam.beta2_correction, adam.epsilon);
        decoder_filters_.adam_update(d_decoder_filters_, decoder_filters_m_, decoder_filters_v_, learning_rate, adam.beta1, adam.beta2, adam.beta1_correction, adam.beta2_correction, adam.epsilon);
        output_bias_.adam_update(d_output_bias_, output_bias_m_, output_bias_v_, learning_rate, adam.beta1, adam.beta2, adam.beta1_correction, adam.beta2_correction, adam.epsilon);
    }

    void
    apply_shared_sgd_update(float learning_rate)
    {
        encoder_filters_.sgd_update(d_encoder_filters_, learning_rate);
        encoder_bias_.sgd_update(d_encoder_bias_, learning_rate);
        decoder_filters_.sgd_update(d_decoder_filters_, learning_rate);
        output_bias_.sgd_update(d_output_bias_, learning_rate);
    }

    void
    apply_optimizer_update(float learning_rate, const matrix & d_decoder_bias)
    {
        const bool update_dense = should_update_dense_this_training_step();

        if(optimizer_.compare_string("adam"))
        {
            const AdamStepParameters shared_adam = adam_step_parameters(++adam_step_);
            apply_shared_adam_update(learning_rate, shared_adam);

            if(update_dense)
            {
                const AdamStepParameters dense_adam = adam_step_parameters(++dense_adam_step_);
                mean_weights_.adam_update(d_mean_weights_, mean_weights_m_, mean_weights_v_, learning_rate, dense_adam.beta1, dense_adam.beta2, dense_adam.beta1_correction, dense_adam.beta2_correction, dense_adam.epsilon);
                mean_bias_.adam_update(d_mean_, mean_bias_m_, mean_bias_v_, learning_rate, dense_adam.beta1, dense_adam.beta2, dense_adam.beta1_correction, dense_adam.beta2_correction, dense_adam.epsilon);
                log_variance_weights_.adam_update(d_log_variance_weights_, log_variance_weights_m_, log_variance_weights_v_, learning_rate, dense_adam.beta1, dense_adam.beta2, dense_adam.beta1_correction, dense_adam.beta2_correction, dense_adam.epsilon);
                log_variance_bias_.adam_update(d_log_variance_, log_variance_bias_m_, log_variance_bias_v_, learning_rate, dense_adam.beta1, dense_adam.beta2, dense_adam.beta1_correction, dense_adam.beta2_correction, dense_adam.epsilon);
                decoder_weights_.adam_update(d_decoder_weights_, decoder_weights_m_, decoder_weights_v_, learning_rate, dense_adam.beta1, dense_adam.beta2, dense_adam.beta1_correction, dense_adam.beta2_correction, dense_adam.epsilon);
                decoder_bias_.adam_update(d_decoder_bias, decoder_bias_m_, decoder_bias_v_, learning_rate, dense_adam.beta1, dense_adam.beta2, dense_adam.beta1_correction, dense_adam.beta2_correction, dense_adam.epsilon);
            }
            return;
        }

        if(!optimizer_.compare_string("sgd"))
            throw exception("ConvolutionalVariationalAutoEncoder: optimizer must be adam or sgd.", path_);

        apply_shared_sgd_update(learning_rate);

        if(update_dense)
        {
            mean_weights_.sgd_update(d_mean_weights_, learning_rate);
            mean_bias_.sgd_update(d_mean_, learning_rate);
            log_variance_weights_.sgd_update(d_log_variance_weights_, learning_rate);
            log_variance_bias_.sgd_update(d_log_variance_, learning_rate);
            decoder_weights_.sgd_update(d_decoder_weights_, learning_rate);
            decoder_bias_.sgd_update(d_decoder_bias, learning_rate);
        }
    }

    void
    apply_spatial_optimizer_update(float learning_rate)
    {
        const bool update_latent = should_update_dense_this_training_step();

        if(optimizer_.compare_string("adam"))
        {
            const AdamStepParameters shared_adam = adam_step_parameters(++adam_step_);
            apply_shared_adam_update(learning_rate, shared_adam);

            if(update_latent)
            {
                const AdamStepParameters latent_adam = adam_step_parameters(++dense_adam_step_);
                spatial_mean_filters_.adam_update(d_spatial_mean_filters_, spatial_mean_filters_m_, spatial_mean_filters_v_, learning_rate, latent_adam.beta1, latent_adam.beta2, latent_adam.beta1_correction, latent_adam.beta2_correction, latent_adam.epsilon);
                spatial_mean_bias_.adam_update(d_spatial_mean_bias_, spatial_mean_bias_m_, spatial_mean_bias_v_, learning_rate, latent_adam.beta1, latent_adam.beta2, latent_adam.beta1_correction, latent_adam.beta2_correction, latent_adam.epsilon);
                spatial_log_variance_filters_.adam_update(d_spatial_log_variance_filters_, spatial_log_variance_filters_m_, spatial_log_variance_filters_v_, learning_rate, latent_adam.beta1, latent_adam.beta2, latent_adam.beta1_correction, latent_adam.beta2_correction, latent_adam.epsilon);
                spatial_log_variance_bias_.adam_update(d_spatial_log_variance_bias_, spatial_log_variance_bias_m_, spatial_log_variance_bias_v_, learning_rate, latent_adam.beta1, latent_adam.beta2, latent_adam.beta1_correction, latent_adam.beta2_correction, latent_adam.epsilon);
                spatial_decoder_filters_.adam_update(d_spatial_decoder_filters_, spatial_decoder_filters_m_, spatial_decoder_filters_v_, learning_rate, latent_adam.beta1, latent_adam.beta2, latent_adam.beta1_correction, latent_adam.beta2_correction, latent_adam.epsilon);
                spatial_decoder_bias_.adam_update(d_spatial_decoder_bias_, spatial_decoder_bias_m_, spatial_decoder_bias_v_, learning_rate, latent_adam.beta1, latent_adam.beta2, latent_adam.beta1_correction, latent_adam.beta2_correction, latent_adam.epsilon);
            }
            return;
        }

        if(!optimizer_.compare_string("sgd"))
            throw exception("ConvolutionalVariationalAutoEncoder: optimizer must be adam or sgd.", path_);

        apply_shared_sgd_update(learning_rate);

        if(update_latent)
        {
            spatial_mean_filters_.sgd_update(d_spatial_mean_filters_, learning_rate);
            spatial_mean_bias_.sgd_update(d_spatial_mean_bias_, learning_rate);
            spatial_log_variance_filters_.sgd_update(d_spatial_log_variance_filters_, learning_rate);
            spatial_log_variance_bias_.sgd_update(d_spatial_log_variance_bias_, learning_rate);
            spatial_decoder_filters_.sgd_update(d_spatial_decoder_filters_, learning_rate);
            spatial_decoder_bias_.sgd_update(d_spatial_decoder_bias_, learning_rate);
        }
    }

    void
    reset_adam_state()
    {
        adam_step_ = 0;
        dense_adam_step_ = 0;
        output_bias_m_.reset();
        output_bias_v_.reset();

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
        d_decoder_activation_.reshape(encoded_size_);

        d_encoder_activation_.dense_backward_input(mean_weights_, d_mean_);
        d_decoder_activation_.dense_backward_input(log_variance_weights_, d_log_variance_);
        d_encoder_activation_.add(d_decoder_activation_);
    }

    void
    Tick()
    {
        validate_reconstruction_source();
        validate_output_activation();
        validate_padding();

        if(effort_.connected() && !effort_.empty() && effort_.sum() <= 0.0f)
            return;

        if(input_.is_uninitialized() || input_.empty())
            return;
        if(input_.rank() != 2 && input_.rank() != 3)
            return;

        if(!initialized_)
            initialize_for_input();
        else if(input_.rows() != input_height_ || input_.cols() != input_width_ ||
                cvae_channels(input_) != input_channels_)
            throw exception("ConvolutionalVariationalAutoEncoder: INPUT shape changed after initialization.", path_);

        encode();
        decode();
        publish_latent();
        compute_losses();

        if(train_.as_bool() && should_train_this_tick())
        {
            const bool use_teacher_forced_reconstruction = reconstruction_source_is(reconstruction_source_top_down);
            if(use_teacher_forced_reconstruction)
            {
                training_reconstruction_ = true;
                decode(false);
            }

            train_step();
            if(use_teacher_forced_reconstruction)
                training_reconstruction_ = false;
        }
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
