// matrix.cc

#include "matrix.h"


namespace ikaros {

float
matrix::sum()
{
    float s = 0;
    reduce([&s](float x) { s+=x;});
    return s; 
}


float
matrix::product()
{
    float s = 1;
    reduce([&s](float x) { s*=x;});
    return s; 
}


float
matrix::min()
{
    if(empty())
        throw std::domain_error("Empty matrix has no min");
    float s = std::numeric_limits<float>::max();
    reduce([&s](float x) { if(x<s) s=x;});
    return s; 
}


float
matrix::max()
{
    if(empty())
        throw std::domain_error("Empty matrix has no max");
    float s = -std::numeric_limits<float>::max();
    reduce([&s](float x) { if(x>s) s=x;});
    return s; 
}


float
matrix::median()
{
    if(empty())
        throw std::domain_error("Empty matrix has no median");
    std::vector<float> vec;
    reduce([&vec](float x) { vec.push_back(x);});
    std::sort(vec.begin(), vec.end());
    size_t size = vec.size();
    size_t mid = size / 2;
    if (size % 2 == 0)
        return (vec[mid - 1] + vec[mid]) / 2.0;
    else
        return vec[mid];
}


float
matrix::average()
{
    if(empty())
        return 0;
    else
        return sum()/size();
}



matrix &
matrix::downsample(const matrix &source) 
{
    if (source.rank() != 2)
        throw std::invalid_argument("downsample() requires 2D input.");
    if (source.rows() % 2 != 0 || source.cols() % 2 != 0)
        throw std::invalid_argument("Source dimensions must be even.");

    int src_rows = source.rows();
    int src_cols = source.cols();
    int new_rows = src_rows / 2;
    int new_cols = src_cols / 2;

    if (rank() == 0) {
        realloc(new_rows, new_cols);
    } else if (rows() != new_rows || cols() != new_cols) {
        throw std::invalid_argument("Destination matrix has incorrect size.");
    }

#ifdef __APPLE__
    // Use a 3x3 averaging kernel for vDSP_imgfir (which requires odd-sized kernels)
    float kernel[9] = {
        0.25f, 0.25f, 0,
        0.25f, 0.25f, 0,
        0, 0, 0
    };

    std::vector<float> filtered(src_rows * src_cols, 0.0f);

    vDSP_imgfir(
        source.data(),
        src_rows,
        src_cols,
        kernel,
        filtered.data(),
        3,
        3
    );

    // Manual decimation: take every second pixel starting from (1,1) to avoid border effects
    float* dst = this->data();
    for (int y = 0; y < new_rows; ++y) {
        float* filtered_row = filtered.data() + (2 * y + 1) * src_cols;
        for (int x = 0; x < new_cols; ++x, ++dst) {
            int fx = 2 * x + 1;
            *dst = filtered_row[fx];
        }
    }
#else
    std::cerr << "[DEBUG] Using fallback manual downsampling." << std::endl;

    for (int y = 0; y < new_rows; ++y) {
        for (int x = 0; x < new_cols; ++x) {
            float a = source(2 * y,     2 * x);
            float b = source(2 * y + 1, 2 * x);
            float c = source(2 * y,     2 * x + 1);
            float d = source(2 * y + 1, 2 * x + 1);
            (*this)(y, x) = (a + b + c + d) * 0.25f;
        }
    }
#endif

    return *this;
}



    matrix &
    matrix::upsample(const matrix &source) 
    {
        if (source.rank() != 2)
            throw std::invalid_argument("upsample() requires a 2D source matrix.");

        int src_rows = source.rows();
        int src_cols = source.cols();
        int new_rows = src_rows * 2;
        int new_cols = src_cols * 2;

        if (rank() == 0) {
            realloc(new_rows, new_cols);
        } else if (rows() != new_rows || cols() != new_cols) {
            throw std::invalid_argument("Destination matrix has incorrect size for upsample.");
        }

    #ifdef __APPLE__
        // Temporary row buffers for horizontal replication
        std::vector<float> row_double(new_cols);

        for (int y = 0; y < src_rows; ++y) {
            const float *src_row = &source(y, 0);

            // Replicate each element horizontally (e.g., A B → A A B B)
            for (int x = 0; x < src_cols; ++x) {
                row_double[2 * x]     = src_row[x];
                row_double[2 * x + 1] = src_row[x];
            }

            // Copy replicated row into two destination rows
            float *dst_row1 = &(*this)(2 * y, 0);
            float *dst_row2 = &(*this)(2 * y + 1, 0);

            // Use vDSP_mmov for fast row copy
            vDSP_mmov(row_double.data(), dst_row1, new_cols, 1, new_cols, new_cols);
            vDSP_mmov(row_double.data(), dst_row2, new_cols, 1, new_cols, new_cols);
        }
    #else
        // Portable C++ fallback
        for (int y = 0; y < src_rows; ++y) {
            for (int x = 0; x < src_cols; ++x) {
                float val = source(y, x);
                (*this)(2 * y,     2 * x)     = val;
                (*this)(2 * y + 1, 2 * x)     = val;
                (*this)(2 * y,     2 * x + 1) = val;
                (*this)(2 * y + 1, 2 * x + 1) = val;
            }
        }
    #endif

        return *this;
    }



    // Matrix saving list

    std::vector<matrix *> saving_matrices;


    void
    save_matrix_states()
    {
        for(auto m : saving_matrices)
            m->save();
    }


    void
    clear_matrix_states()
    {
        saving_matrices.clear();
    }



        void 
        matrix::save()
        {
            if(last_!=nullptr)
                last_->copy(*this);
        }

        matrix & 
        matrix::last()
        {
            if(last_==nullptr)
            {
                saving_matrices.push_back(this);
                last_ = std::make_shared<matrix>();
                save();
            }
            return *last_;
        }

        bool 
        matrix::changed()
        {
            if(last_==nullptr)
                return false;
            return !(*last_ == *this);
        }
}

