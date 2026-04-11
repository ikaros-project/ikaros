// matrix.cc

#ifndef ACCELERATE_NEW_LAPACK
#define ACCELERATE_NEW_LAPACK
#endif

#include "matrix.h"

namespace ikaros {

float
matrix::sum()
{
#if defined(__APPLE__)
    if(info_->continuous)
    {
        float s = 0;
        vDSP_sve(data(), 1, &s, static_cast<vDSP_Length>(info_->size_));
        return s;
    }
#endif
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
#if defined(__APPLE__)
    if(info_->continuous)
    {
        float s = 0;
        vDSP_minv(data(), 1, &s, static_cast<vDSP_Length>(info_->size_));
        return s;
    }
#endif
    float s = std::numeric_limits<float>::max();
    reduce([&s](float x) { if(x<s) s=x;});
    return s; 
}


float
matrix::max()
{
    if(empty())
        throw std::domain_error("Empty matrix has no max");
#if defined(__APPLE__)
    if(info_->continuous)
    {
        float s = 0;
        vDSP_maxv(data(), 1, &s, static_cast<vDSP_Length>(info_->size_));
        return s;
    }
#endif
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
#if defined(__APPLE__)
    if(info_->continuous)
    {
        float s = 0;
        vDSP_meanv(data(), 1, &s, static_cast<vDSP_Length>(info_->size_));
        return s;
    }
#endif
    return sum()/size();
}


matrix &
matrix::scale(float c)
{
    if(info_->continuous)
    {
        vDSP_vsmul(data_->data(), 1, &c, data_->data(), 1, info_->size_);
        return *this;
    }
    else
    {
        return apply([c](float x)->float { return x * c; });
    }
}


matrix &
matrix::add(matrix A, matrix B)
{
    check_same_size(A);
    check_same_size(B);

    if(info_->continuous && A.info_->continuous && B.info_->continuous)
    {
        float *a = A.data();
        float *b = B.data();
        float *r = this->data();

        vDSP_vadd(b, 1, a, 1, r, 1, this->data_->size());
        return *this;
    }
    else
    {
        return apply(A, B, [](float x, float y)->float { return x + y; });
    }
}


matrix &
matrix::subtract(matrix A, matrix B)
{
    check_same_size(A);
    check_same_size(B);

    if(info_->continuous && A.info_->continuous && B.info_->continuous)
    {
        float *a = A.data();
        float *b = B.data();
        float *r = this->data();

        vDSP_vsub(b, 1, a, 1, r, 1, this->data_->size());
        return *this;
    }
    else
    {
        return apply(A, B, [](float x, float y)->float { return x - y; });
    }
}


matrix &
matrix::multiply(matrix A, matrix B)
{
    check_same_size(A);
    check_same_size(B);

    if(info_->continuous && A.info_->continuous && B.info_->continuous)
    {
        float *a = A.data();
        float *b = B.data();
        float *r = this->data();

        vDSP_vmul(b, 1, a, 1, r, 1, this->data_->size());
        return *this;
    }
    else
    {
        return apply(A, B, [](float x, float y)->float { return x * y; });
    }
}


matrix &
matrix::divide(matrix A, matrix B)
{
    check_same_size(A);
    check_same_size(B);

    if(info_->continuous && A.info_->continuous && B.info_->continuous)
    {
        float *a = A.data();
        float *b = B.data();
        float *r = this->data();

        vDSP_vdiv(b, 1, a, 1, r, 1, this->data_->size());
        return *this;
    }
    else
    {
        return apply(A, B, [](float x, float y)->float { return x / y; });
    }
}


matrix &
matrix::maximum(matrix A)
{
    check_same_size(A);

#if defined(__APPLE__)
    if(info_->continuous && A.info_->continuous)
    {
        vDSP_vmax(data(), 1, A.data(), 1, data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, [](float x, float y)->float { return std::max(x, y); });
}


matrix &
matrix::minimum(matrix A)
{
    check_same_size(A);

#if defined(__APPLE__)
    if(info_->continuous && A.info_->continuous)
    {
        vDSP_vmin(data(), 1, A.data(), 1, data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, [](float x, float y)->float { return std::min(x, y); });
}


matrix &
matrix::maximum(matrix A, matrix B)
{
    check_same_size(A);
    check_same_size(B);

#if defined(__APPLE__)
    if(info_->continuous && A.info_->continuous && B.info_->continuous)
    {
        vDSP_vmax(A.data(), 1, B.data(), 1, data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, B, [](float x, float y)->float { return std::max(x, y); });
}


matrix &
matrix::minimum(matrix A, matrix B)
{
    check_same_size(A);
    check_same_size(B);

#if defined(__APPLE__)
    if(info_->continuous && A.info_->continuous && B.info_->continuous)
    {
        vDSP_vmin(A.data(), 1, B.data(), 1, data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, B, [](float x, float y)->float { return std::min(x, y); });
}


matrix &
matrix::hypot(matrix & x, matrix & y)
{
    if(empty())
        realloc(x.shape());

    check_same_size(x);
    check_same_size(y);

    if(this == &x || this == &y)
        throw std::invalid_argument("Result cannot be assigned to x or y.");

#if defined(__APPLE__)
    vDSP_vdist(x.data(), 1, y.data(), 1, data(), 1, size());
#else
    for(int j = 0; j < size(); ++j)
        (*this)(j) = std::sqrt(x(j) * x(j) + y(j) * y(j));
#endif

    return *this;
}


matrix &
matrix::atan2(matrix & y, matrix & x)
{
    if(empty())
        realloc(x.shape());

    check_same_size(x);
    check_same_size(y);

    if(this == &x || this == &y)
        throw std::invalid_argument("Result cannot be assigned to x or y.");

#if defined(__APPLE__)
    int n = size();
    vvatan2f(data(), y.data(), x.data(), &n);
#else
    for(int j = 0; j < size(); ++j)
        (*this)(j) = std::atan2(y(j), x(j));
#endif

    return *this;
}


matrix &
matrix::matmul(matrix & A, matrix & B)
{
    if(empty())
        realloc(A.rows(), B.cols());

#ifndef NO_MATRIX_CHECKS
    if(rank() != 2 || A.rank() != 2 || B.rank() != 2)
        throw std::invalid_argument("Multiplication requires two-dimensional matrices.");

    if(A.cols() != B.rows())
        throw std::invalid_argument("Matrices are not compatible for multiplication.");
#endif

    if(rows() != A.rows() || cols() != B.cols())
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(A.rows()) + "x" + std::to_string(B.cols()) + ".");

    if(this == &A || this == &B)
        throw std::invalid_argument("Result cannot be assigned to A or B.");

#if defined(__APPLE__)
    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                A.rows(), B.cols(), A.cols(), 1.0f,
                A.data(), A.info_->stride_[1],
                B.data(), B.info_->stride_[1],
                0.0f,
                this->data(), this->info_->stride_[1]);
#else
    reset();
    for(int j = 0; j < A.rows(); ++j)
        for(int i = 0; i < B.cols(); ++i)
            for(int k = 0; k < B.rows(); ++k)
                (*this)(j, i) += A(j, k) * B(k, i);
#endif

    return *this;
}


matrix &
matrix::inv()
{
    if(rank() != 2)
        throw std::invalid_argument("Matrix must be two-dimensional.");

    if(size_x() != size_y())
        throw std::invalid_argument("Matrix must be square for inversion.");

    int n = size_x();
    int lda = size_y();
    int info;

    std::vector<int> ipiv(n);
    int lwork = n * 64;
    std::vector<float> work(lwork);

    sgetrf_(&n, &n, data(), &lda, ipiv.data(), &info);

    if(info != 0)
        throw std::runtime_error("LU decomposition failed with info = " + std::to_string(info));

    sgetri_(&n, data(), &lda, ipiv.data(), work.data(), &lwork, &info);

    if(info != 0)
        throw std::runtime_error("Matrix inversion failed with info = " + std::to_string(info));

    return *this;
}


matrix &
matrix::corr3(matrix &I, matrix &K, const std::vector<float> &kernel_flat, const std::vector<float> &submatrices_flat)
{
    if(empty())
        realloc(I.rows() - K.rows() + 1, I.cols() - K.cols() + 1);

#ifndef NO_MATRIX_CHECKS
    if(rank() != 2 || I.rank() != 2 || K.rank() != 2)
        throw std::invalid_argument("Correlation requires two-dimensional matrices.");

    if(I.cols() < K.cols() || I.rows() < K.rows())
        throw std::invalid_argument("K must fit in I");
#endif

    int rr = I.rows() - K.rows() + 1;
    int rc = I.cols() - K.cols() + 1;

    if(rows() != rr || cols() != rc)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(rr) + "x" + std::to_string(rc) + ".");

    if(this == &I || this == &K)
        throw std::invalid_argument("Result cannot be assigned to I or K.");

    reset();

#if defined(__APPLE__)
    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                rr * rc, 1, K.rows() * K.cols(),
                1.0f,
                submatrices_flat.data(), K.rows() * K.cols(),
                kernel_flat.data(), 1,
                0.0f,
                this->data(), 1);
#else
    for(int idx = 0; idx < rr * rc; ++idx)
    {
        float sum = 0.0f;
        int base = idx * K.rows() * K.cols();
        for(int k = 0; k < K.rows() * K.cols(); ++k)
            sum += submatrices_flat[base + k] * kernel_flat[k];
        this->data()[idx] = sum;
    }
#endif

    return *this;
}


matrix &
matrix::conv(matrix &I, matrix &K)
{
#ifndef NO_MATRIX_CHECKS
    if(rank() != 2 || I.rank() != 2 || K.rank() != 2)
        throw std::invalid_argument("Convolution requires two-dimensional matrices.");

    if(I.cols() < K.cols() || I.rows() < K.rows())
        throw std::invalid_argument("K must fit in I");
#endif

    int Ir = I.rows();
    int Ic = I.cols();
    int Kr = K.rows();
    int Kc = K.cols();

    if(this == &I || this == &K)
        throw std::invalid_argument("Result cannot be assigned to I or K.");

    reset();

#if defined(__APPLE__)
    const float *input = I.data();
    const float *kernel = K.data();
    float *output = this->data();

    vDSP_imgfir(input, Ir, Ic, kernel, output, Kr, Kc);
#else
    return conv_slow(I, K);
#endif

    return *this;
}


void
matrix::singular_value_decomposition(matrix& inputMatrix, matrix& U, matrix& S, matrix& Vt)
{
    int m = inputMatrix.size_y();
    int n = inputMatrix.size_x();

    std::vector<float> a(inputMatrix.data(), inputMatrix.data() + m * n);
    int min_mn = std::min(m, n);

    matrix singularValues(min_mn);
    matrix u(m, m);
    matrix vt(n, n);

    int lda = m;
    int ldu = m;
    int ldvt = n;
    int info;

    int lwork = -1;
    float work_size;
    sgesvd_("A", "A", &m, &n, a.data(), &lda, singularValues.data(), u.data(), &ldu, vt.data(), &ldvt, &work_size, &lwork, &info);

    lwork = static_cast<int>(work_size);
    std::vector<float> work(lwork);
    sgesvd_("A", "A", &m, &n, a.data(), &lda, singularValues.data(),
            u.data(), &ldu, vt.data(), &ldvt, work.data(), &lwork, &info);

    if(info > 0)
        throw std::runtime_error("SVD did not converge.");

    U.copy(u);
    Vt.copy(vt);

    std::vector<float> s(m * n, 0.0f);
    for(int i = 0; i < min_mn; ++i)
        s[i * n + i] = singularValues[i];

    S.resize(m, n);
    std::copy(s.begin(), s.end(), S.data());
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


    // Copy a submatrix to a flat vector
    static inline void 
    extract_flat_submatrix(const matrix& src, int y, int x, int h, int w, std::vector<float>& out) 
    {
        float * v = out.data();
        const float * s = src.data();
        int cols = src.cols();
        for (int j = 0; j < h; ++j)
        {
            const float * row = s + (y + j) * cols + x;
            for (int i = 0; i < w; ++i)
                *v++ = *row++;
        }
    }


    // Computes  cross-correlation between kernel and normalized submatrix; kernel is not normalized

    static float half_normalized_correlation(const float* kernel, const float* submatrix, float * buffer, size_t len) 
    {
        if (len == 0) 
            return 0.0f;

        float submatrix_mean = 0.0f;
        vDSP_meanv(submatrix, 1, &submatrix_mean, len);

        float neg_submatrix_mean = -submatrix_mean;
        vDSP_vsadd(submatrix, 1, &neg_submatrix_mean, buffer, 1, len);

        float submatrix_norm2 = 0.0f;
        vDSP_svesq(buffer, 1, &submatrix_norm2, len);
        if(submatrix_norm2 < 0.001)
            return 0;

        float dot = 0.0f;
        vDSP_dotpr(kernel, 1, buffer, 1, &dot, len);

        float denom = std::sqrt(submatrix_norm2);

        return denom > 0.0f ? dot / denom : 0.0f;
    }


    // Search for a kernel in this matrix
    // Returns the best matching point in the search rectangle together with its matching score
    // TODO: Add non-Apple implementation for other platforms
    
    match
    matrix::search(const matrix & target, const rect & search_rectangle) const
    {
        #ifndef NO_MATRIX_CHECKS
        if (target.rank() != 2)
            throw std::invalid_argument("Search requires a 2D target matrix.");
        if (target.rows() == 0 || target.cols() == 0)
            throw std::invalid_argument("Kernel cannot be empty.");
        if (search_rectangle.width <= 0 || search_rectangle.height <= 0)
            throw std::invalid_argument("Search rectangle must have positive dimensions.");
        if (search_rectangle.x < 0 || search_rectangle.y < 0 ||
            search_rectangle.x + search_rectangle.width > cols() ||
            search_rectangle.y + search_rectangle.height > rows())
            throw std::out_of_range("Search rectangle is out of bounds of the matrix.");
         #endif

        match best_match = {0, 0, 0}; // -std::numeric_limits<float>::max()
        int search_top = search_rectangle.y;
        int search_bottom = search_rectangle.y + search_rectangle.height;
        int search_left = search_rectangle.x;
        int search_right = search_rectangle.x + search_rectangle.width;

        int target_rows = target.rows();
        int target_cols = target.cols();
        int target_size = target.size();

        // Prepare target data
       float target_mean = 0.0f;
        vDSP_meanv(target.data(), 1, &target_mean, target_size);
        std::vector<float> target_zero_mean(target_size);
        std::vector<float> buffer(target_size);
        float neg_target_mean = -target_mean;
        vDSP_vsadd(target.data(), 1, &neg_target_mean, target_zero_mean.data(), 1, target_size);

        // Compute target norm
        float target_norm2 = 0.0f;
        vDSP_svesq(target_zero_mean.data(), 1, &target_norm2, target_size);
        float norm_k = sqrt(target_norm2);
        if (norm_k < 0.001) 
            return match{0, 0, 0}; // If target has almost zero norm, return zero match

            std::vector<float> flat_submatrix(target_rows*target_cols);

        // Iterate over the search rectangle
        for (int y = search_top; y <= search_bottom - target_rows; ++y) {
            for (int x = search_left; x <= search_right - target_cols; ++x) 
            {                     
                extract_flat_submatrix(*this, y, x, target_rows, target_cols, flat_submatrix); // Extract the submatrix at the current position
                float score = half_normalized_correlation(target.data(), flat_submatrix.data(), buffer.data(), flat_submatrix.size());

                if (score > best_match.score) 
                {
                    best_match.x = x;
                    best_match.y = y;
                    best_match.score = score;
                }
            }
        }

        best_match.score /= norm_k; // Normalize the score by the target norm
        return best_match;
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
