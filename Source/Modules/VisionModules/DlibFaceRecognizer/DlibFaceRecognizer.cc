#include "ikaros.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <dlib/dnn.h>
#include <dlib/image_io.h>
#include <dlib/image_processing.h>
#include <dlib/matrix.h>

using namespace ikaros;

namespace
{
    template <template <int, template<typename> class, int, typename> class block, int N, template<typename> class BN, typename SUBNET>
    using residual = dlib::add_prev1<block<N, BN, 1, dlib::tag1<SUBNET>>>;

    template <template <int, template<typename> class, int, typename> class block, int N, template<typename> class BN, typename SUBNET>
    using residual_down = dlib::add_prev2<dlib::avg_pool<2, 2, 2, 2, dlib::skip1<dlib::tag2<block<N, BN, 2, dlib::tag1<SUBNET>>>>>>;

    template <int N, template <typename> class BN, int stride, typename SUBNET>
    using block = BN<dlib::con<N, 3, 3, 1, 1, dlib::relu<BN<dlib::con<N, 3, 3, stride, stride, SUBNET>>>>>;

    template <int N, typename SUBNET> using ares = dlib::relu<residual<block, N, dlib::affine, SUBNET>>;
    template <int N, typename SUBNET> using ares_down = dlib::relu<residual_down<block, N, dlib::affine, SUBNET>>;
    template <typename SUBNET> using alevel0 = ares_down<256, SUBNET>;
    template <typename SUBNET> using alevel1 = ares<256, ares<256, ares_down<256, SUBNET>>>;
    template <typename SUBNET> using alevel2 = ares<128, ares<128, ares_down<128, SUBNET>>>;
    template <typename SUBNET> using alevel3 = ares<64, ares<64, ares<64, ares_down<64, SUBNET>>>>;
    template <typename SUBNET> using alevel4 = ares<32, ares<32, ares<32, SUBNET>>>;

    using anet_type = dlib::loss_metric<dlib::fc_no_bias<128, dlib::avg_pool_everything<
        alevel0<
        alevel1<
        alevel2<
        alevel3<
        alevel4<
        dlib::max_pool<3, 3, 2, 2, dlib::relu<dlib::affine<dlib::con<32, 7, 7, 2, 2,
        dlib::input_rgb_image_sized<150>
        >>>>>>>>>>>>;

    unsigned char
    to_byte(float v)
    {
        if(v <= 1.0f)
            v *= 255.0f;
        return static_cast<unsigned char>(std::clamp(std::lround(v), 0l, 255l));
    }

    void
    copy_to_dlib_rgb(const matrix & input, dlib::matrix<dlib::rgb_pixel> & image)
    {
        int height = input.size_y();
        int width = input.size_x();
        image.set_size(height, width);

        if(input.rank() == 3 && input.shape(0) >= 3)
        {
            for(int y = 0; y < height; ++y)
                for(int x = 0; x < width; ++x)
                    image(y, x) = dlib::rgb_pixel(to_byte(input(0, y, x)),
                                                  to_byte(input(1, y, x)),
                                                  to_byte(input(2, y, x)));
            return;
        }

        for(int y = 0; y < height; ++y)
            for(int x = 0; x < width; ++x)
            {
                unsigned char v = to_byte(input(y, x));
                image(y, x) = dlib::rgb_pixel(v, v, v);
            }
    }

    bool
    sanitize_read_path(Module & module, const std::string & path, std::filesystem::path & sanitized_path)
    {
        if(!kernel().SanitizeReadPath(path, sanitized_path))
        {
            module.Notify(msg_fatal_error,
                "DlibFaceRecognizer could not find or read \"" + path + "\".\n"
                "Download the required dlib model files and place them in UserData/models:\n"
                "  http://dlib.net/files/shape_predictor_5_face_landmarks.dat.bz2\n"
                "  http://dlib.net/files/dlib_face_recognition_resnet_model_v1.dat.bz2\n"
                "Then decompress them so these files exist:\n"
                "  UserData/models/shape_predictor_5_face_landmarks.dat\n"
                "  UserData/models/dlib_face_recognition_resnet_model_v1.dat"
            );
            return false;
        }
        return true;
    }
}

class DlibFaceRecognizer : public Module
{
    matrix input_;
    matrix boxes_;
    matrix descriptors_;
    matrix boxes_out_;
    matrix count_;
    parameter max_faces_;
    parameter shape_predictor_path_;
    parameter recognition_model_path_;

    dlib::shape_predictor shape_predictor_;
    anet_type recognition_net_;
    dlib::matrix<dlib::rgb_pixel> image_;
    bool models_loaded_ = false;

public:
    void Init() override
    {
        Bind(input_, "INPUT");
        Bind(boxes_, "BOXES");
        Bind(descriptors_, "DESCRIPTORS");
        Bind(boxes_out_, "BOXES_OUT");
        Bind(count_, "COUNT");
        Bind(max_faces_, "max_faces");
        Bind(shape_predictor_path_, "shape_predictor");
        Bind(recognition_model_path_, "recognition_model");

        std::filesystem::path predictor_path;
        std::filesystem::path model_path;
        if(!sanitize_read_path(*this, std::string(shape_predictor_path_), predictor_path) ||
           !sanitize_read_path(*this, std::string(recognition_model_path_), model_path))
            return;

        try
        {
            dlib::deserialize(predictor_path.string()) >> shape_predictor_;
            dlib::deserialize(model_path.string()) >> recognition_net_;
            models_loaded_ = true;
        }
        catch(const std::exception & e)
        {
            Notify(msg_fatal_error,
                std::string("DlibFaceRecognizer could not load dlib model files: ") + e.what() + "\n"
                "Expected decompressed dlib model files, for example:\n"
                "  UserData/models/shape_predictor_5_face_landmarks.dat\n"
                "  UserData/models/dlib_face_recognition_resnet_model_v1.dat\n"
                "Download compressed originals from:\n"
                "  http://dlib.net/files/shape_predictor_5_face_landmarks.dat.bz2\n"
                "  http://dlib.net/files/dlib_face_recognition_resnet_model_v1.dat.bz2"
            );
        }
    }

    void Tick() override
    {
        descriptors_.clear();
        boxes_out_.clear();
        count_.reset();

        if(!models_loaded_)
            return;

        copy_to_dlib_rgb(input_, image_);

        int limit = std::min(static_cast<int>(max_faces_), boxes_.rows());
        std::vector<dlib::matrix<dlib::rgb_pixel>> face_chips;
        std::vector<int> source_rows;
        face_chips.reserve(limit);
        source_rows.reserve(limit);

        for(int i = 0; i < limit; ++i)
        {
            if(boxes_.cols() < 4)
                break;

            int x = std::lround((boxes_(i, 0) + 1.0f) * 0.5f * input_.size_x());
            int y = std::lround((boxes_(i, 1) + 1.0f) * 0.5f * input_.size_y());
            int w = std::lround(boxes_(i, 2) * 0.5f * input_.size_x());
            int h = std::lround(boxes_(i, 3) * 0.5f * input_.size_y());

            if(w <= 0 || h <= 0)
                continue;

            dlib::rectangle rect(x, y, x + w - 1, y + h - 1);
            rect = rect.intersect(dlib::rectangle(0, 0, input_.size_x() - 1, input_.size_y() - 1));
            if(rect.is_empty())
                continue;

            dlib::full_object_detection shape = shape_predictor_(image_, rect);
            dlib::matrix<dlib::rgb_pixel> face_chip;
            dlib::extract_image_chip(image_, dlib::get_face_chip_details(shape, 150, 0.25), face_chip);
            face_chips.push_back(face_chip);
            source_rows.push_back(i);
        }

        if(face_chips.empty())
            return;

        std::vector<dlib::matrix<float, 0, 1>> face_descriptors = recognition_net_(face_chips);

        for(std::size_t i = 0; i < face_descriptors.size(); ++i)
        {
            matrix descriptor(128);
            for(int j = 0; j < 128; ++j)
                descriptor(j) = face_descriptors[i](j);
            descriptors_.append(descriptor);

            matrix box_row(5);
            int source = source_rows[i];
            box_row(0) = boxes_(source, 0);
            box_row(1) = boxes_(source, 1);
            box_row(2) = boxes_(source, 2);
            box_row(3) = boxes_(source, 3);
            box_row(4) = static_cast<float>(source);
            boxes_out_.append(box_row);
        }

        count_(0) = static_cast<float>(descriptors_.rows());
    }
};

INSTALL_CLASS(DlibFaceRecognizer)
