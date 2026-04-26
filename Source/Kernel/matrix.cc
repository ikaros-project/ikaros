// matrix.cc

#ifndef ACCELERATE_NEW_LAPACK
#define ACCELERATE_NEW_LAPACK
#endif

#if defined(__APPLE__)
#include <vecLib/vDSP.h>
#include <vecLib/vForce.h>
#include <vecLib/cblas_new.h>
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#endif
#include <vecLib/lapack.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#endif

#include "matrix.h"

namespace ikaros {

float
parse_matrix_token(const std::string & token)
{
    std::string trimmed = trim(token);
    size_t pos = 0;
    float value = std::stof(trimmed, &pos);
    if(trimmed.substr(pos).find_first_not_of(" \t\r\n") != std::string::npos)
        throw std::invalid_argument("Invalid matrix value \"" + token + "\". Values must be separated by ',' or ';'.");
    return value;
}


matrix_info::matrix_info(std::vector<int> shape):
    offset_(0), shape_(shape), stride_(shape), max_size_(shape), size_(calculate_size()), continuous(true), labels_(shape.size())
{}


void
matrix_info::print(std::string n) const
{
    print_attribute_value("name", n.empty() ? name_ : n);
    print_attribute_value("rank", shape_.size());
    print_attribute_value("shape", shape_);
    print_attribute_value("stride", stride_);
    print_attribute_value("max_size", max_size_);
    print_attribute_value("size", size_);
    print_attribute_value("offeset", offset_);
    print_attribute_value("labels", labels_);
}


matrix
matrix::iterator::operator*()
{
    return (*matrix_)[index_];
}


matrix::matrix(std::vector<int> shape)
{
    try
    {
        info_ = std::make_shared<matrix_info>(shape);
        data_ = std::make_shared<std::vector<float>>(info_->calculate_size());
    }
    catch(const std::exception &)
    {
        throw out_of_memory_matrix_error("Could not allocate memory for matrix");
    }
}


matrix::matrix(int cols, float *):
    matrix(cols)
{}


matrix::matrix(int, int, float **)
{}


void
matrix::operator=(std::string & data_string)
{
    std::string sanitized = remove_comment(data_string);

    if(try_parse_bracket_matrix_literal(*this, sanitized))
        return;

    auto & rows = split(sanitized, ";");
    auto & row = split(rows.at(0), ",");
    auto parse_value_at = [](const std::string & token, int row_index, int column_index) -> float
    {
        std::string trimmed = trim(token);
        if(trimmed.empty())
            throw std::invalid_argument(
                "Invalid matrix value at row " + std::to_string(row_index + 1) +
                ", column " + std::to_string(column_index + 1) +
                ": empty value."
            );

        try
        {
            return parse_matrix_token(token);
        }
        catch(const std::invalid_argument & e)
        {
            throw std::invalid_argument(
                "Invalid matrix value at row " + std::to_string(row_index + 1) +
                ", column " + std::to_string(column_index + 1) +
                ": " + std::string(e.what())
            );
        }
    };

    int x = row.size();
    int y = rows.size();

    if(rows.size() == 1)
    {
        realloc(x);
        for(std::size_t i = 0; i < row.size(); ++i)
            (*this)(i) = parse_value_at(row.at(i), 0, static_cast<int>(i));
    }
    else
    {
        realloc(y, x);
        for(std::size_t j = 0; j < rows.size(); ++j)
        {
            auto r = split(rows.at(j), ",");
            for(std::size_t i = 0; i < r.size(); ++i)
                (*this)(static_cast<int>(j), static_cast<int>(i)) = parse_value_at(r.at(i), static_cast<int>(j), static_cast<int>(i));
        }
    }
}


matrix::matrix(const std::string & data_string)
{
    try
    {
        std::string sanitized = remove_comment(data_string);

        if(try_parse_bracket_matrix_literal(*this, sanitized))
            return;

        auto & rows = split(sanitized, ";");
        auto & row = split(rows.at(0), ",");
        auto parse_value_at = [](const std::string & token, int row_index, int column_index) -> float
        {
            std::string trimmed = trim(token);
            if(trimmed.empty())
                throw std::invalid_argument(
                    "Invalid matrix value at row " + std::to_string(row_index + 1) +
                    ", column " + std::to_string(column_index + 1) +
                    ": empty value."
                );

            try
            {
                return parse_matrix_token(token);
            }
            catch(const std::invalid_argument & e)
            {
                throw std::invalid_argument(
                    "Invalid matrix value at row " + std::to_string(row_index + 1) +
                    ", column " + std::to_string(column_index + 1) +
                    ": " + std::string(e.what())
                );
            }
        };

        int x = row.size();
        int y = rows.size();

        if(!sanitized.empty() && sanitized.back() == ';')
            y--;

        if(rows.size() == 1)
        {
            info_ = std::make_shared<matrix_info>(std::vector<int>{x});
            data_ = std::make_shared<std::vector<float>>(info_->calculate_size());

            for(std::size_t i = 0; i < row.size(); ++i)
                (*this)(i) = parse_value_at(row.at(i), 0, static_cast<int>(i));
        }
        else
        {
            info_ = std::make_shared<matrix_info>(std::vector<int>{y, x});
            data_ = std::make_shared<std::vector<float>>(info_->calculate_size());

            for(int j = 0; j < y; ++j)
            {
                auto r = split(rows.at(j), ",");
                for(std::size_t i = 0; i < row.size(); ++i)
                    (*this)(j, static_cast<int>(i)) = parse_value_at(r.at(i), j, static_cast<int>(i));
            }
        }
    }
    catch(std::out_of_range &)
    {
        throw std::invalid_argument("Invalid matrix string: inconsistent number of values in rows.");
    }
    catch(std::invalid_argument &)
    {
        throw;
    }
}


matrix::matrix(const char * data_string):
    matrix(std::string(data_string))
{}


matrix
matrix::operator[](int i)
{
#ifndef NO_MATRIX_CHECKS
    if(i < 0 || i >= info_->shape_.front())
        throw std::out_of_range("Index out of range");
#endif
    matrix r = *this;
    r.info_ = std::make_shared<matrix_info>(this->info_->shape_);
    *r.info_ = *info_;
    int new_offset = i;
    for(int d = info_->stride_.size() - 1; d > 0; --d)
        new_offset *= info_->stride_.at(d);
    r.info_->offset_ += new_offset;
    r.info_->shape_ = {info_->shape_.begin() + 1, info_->shape_.end()};
    r.info_->stride_ = {info_->stride_.begin() + 1, info_->stride_.end()};
    r.info_->max_size_ = {info_->max_size_.begin() + 1, info_->max_size_.end()};
    r.info_->size_ = r.info_->calculate_size();
    if(r.info_->size_ == 0)
        r.info_->size_ = 1;

    if(info_->labels_.at(0).size() > static_cast<std::size_t>(i))
        r.info_->name_ += std::string(".") + info_->labels_.at(0).at(i);
    else
        r.info_->name_ += "[" + std::to_string(i) + "]";
    r.info_->labels_.erase(r.info_->labels_.begin());
    return r;
}


matrix
matrix::operator[](int i) const
{
#ifndef NO_MATRIX_CHECKS
    if(i < 0 || i >= info_->shape_.front())
        throw std::out_of_range("Index out of range");
#endif
    matrix r = *this;
    r.info_ = std::make_shared<matrix_info>(this->info_->shape_);
    *r.info_ = *info_;
    int new_offset = i;
    for(int d = info_->stride_.size() - 1; d > 0; --d)
        new_offset *= info_->stride_.at(d);
    r.info_->offset_ += new_offset;
    r.info_->shape_ = {info_->shape_.begin() + 1, info_->shape_.end()};
    r.info_->stride_ = {info_->stride_.begin() + 1, info_->stride_.end()};
    r.info_->max_size_ = {info_->max_size_.begin() + 1, info_->max_size_.end()};
    r.info_->size_ = r.info_->calculate_size();
    if(r.info_->size_ == 0)
        r.info_->size_ = 1;

    if(info_->labels_.at(0).size() > static_cast<std::size_t>(i))
        r.info_->name_ += std::string(".") + info_->labels_.at(0).at(i);
    else
        r.info_->name_ += "[" + std::to_string(i) + "]";
    r.info_->labels_.erase(r.info_->labels_.begin());
    return r;
}


void
matrix::info(std::string n) const
{
    info_->print(n);
    print_attribute_value("data size", data_->size());
    print_attribute_value("data", *data_, 0, 40);
}


void
matrix::test_fill()
{
    for(std::size_t i = 0; i < data_->size(); ++i)
        (*data_)[i] = float(i);
}


void
matrix::init(std::vector<int> & shape, std::shared_ptr<std::vector<float>> data, std::initializer_list<InitList> list, int depth)
{
    if(shape.size() <= static_cast<std::size_t>(depth))
        shape.push_back(list.size());

#ifndef NO_MATRIX_CHECKS
    if(depth < static_cast<int>(shape.size()))
    {
        if(list.size() < static_cast<std::size_t>(shape[depth]))
            throw std::out_of_range("Too few values in matrix initialization");
        else if(list.size() > static_cast<std::size_t>(shape[depth]))
            throw std::out_of_range("Too many values in matrix initialization");
    }
#endif

    int row_type = 0;
    for(auto d : list)
        if(std::holds_alternative<float>(d.value))
        {
#ifndef NO_MATRIX_CHECKS
            if(row_type != 0 && row_type != 1)
                throw std::invalid_argument("Mixed data in initialization list");
#endif
            row_type = 1;
            (*data).push_back(std::get<float>(d.value));
        }
        else if(std::holds_alternative<std::initializer_list<InitList>>(d.value))
        {
#ifndef NO_MATRIX_CHECKS
            if(row_type != 0 && row_type != 2)
                throw std::invalid_argument("Mixed data in initialization list");
#endif
            row_type = 2;
            init(shape, data, std::get<std::initializer_list<InitList>>(d.value), depth + 1);
        }
}


matrix::matrix(std::initializer_list<InitList> list):
    info_(std::make_shared<matrix_info>()),
    data_(std::make_shared<std::vector<float>>())
{
    info_->offset_ = 0;
    info_->size_ = 0;
    init(info_->shape_, data_, list);
    info_->stride_ = info_->shape_;
    info_->max_size_ = info_->shape_;
    info_->size_ = info_->calculate_size();
    data_->resize(info_->size_);
    info_->labels_.resize(info_->shape_.size());
}


range
matrix::get_range() const
{
    range r;
    for(auto b : info_->shape_)
        r.push(0, b);
    return r;
}


matrix::operator range() const
{
    return get_range();
}


matrix &
matrix::set_name(std::string n)
{
    info_->name_ = n;
    return *this;
}


std::string
matrix::get_name(std::string post) const
{
    if(!info_->name_.empty())
        return info_->name_ + post;
    else
        return "";
}


matrix &
matrix::clear_labels(int dimension)
{
    set_labels(dimension);
    return *this;
}


matrix &
matrix::push_label(int dimension, std::string label, int no_of_columns)
{
    if(no_of_columns == 1)
        info_->labels_.at(dimension).push_back(label);
    else
        for(int i = 0; i < no_of_columns; ++i)
            info_->labels_.at(dimension).push_back(label + ":" + std::to_string(i));
    return *this;
}


const std::vector<std::string>
matrix::labels(int dimension) const
{
    return info_->labels_.at(dimension);
}


int
matrix::rank() const
{
    return info_->shape_.size();
}


bool
matrix::empty() const
{
    return info_->size_ == 0;
}


bool
matrix::is_uninitialized() const
{
    return rank() == 0 && (info_->size_ == 0);
}


bool
matrix::unfilled() const
{
    return std::accumulate(info_->shape_.begin(), info_->shape_.end(), 0) == 0;
}


bool
matrix::is_scalar() const
{
    return rank() == 0 && (info_->size_ == 1);
}


bool
matrix::connected() const
{
    return !is_uninitialized();
}


bool
matrix::print_(int depth) const
{
    if(rank() == 0)
    {
        if(info_->size_ == 0)
            std::cout << "{}";
        else if(info_->size_ == 1)
            std::cout << data_->at(info_->offset_);
        return true;
    }

    std::string sep;
    bool t = false;
    std::cout << "\n" << tab(depth) << "{";
    for(int i = 0; i < info_->shape_.at(0); ++i)
    {
        std::cout << sep;
        t = (*this)[i].print_(depth + 1);
        sep = ", ";
    }
    if(t)
        std::cout << "}";
    else
        std::cout << "\n" << tab(depth) << "}";
    return false;
}


std::string
matrix::json() const
{
    if(rank() == 0)
    {
        if(info_->size_ == 0)
            return "[]";
        return format_json_number(data_->at(info_->offset_));
    }

    std::string sep;
    std::string s = "[";
    for(auto x : *this)
    {
        s += sep + x.json();
        sep = ", ";
    }
    s += "]";
    return s;
}


std::string
matrix::csv(std::string separator) const
{
    std::string sep;
    std::string s;

    if(rank() == 1)
    {
        for(auto value : *this)
        {
            s += sep + std::to_string(value);
            sep = separator;
        }
        s += "\n";
        return s;
    }

    if(rank() == 2)
    {
        if(info_->labels_.size() > 1)
        {
            for(auto & header : info_->labels_[1])
            {
                s += sep + header;
                sep = separator;
            }
            s += "\n";
        }

        for(auto row : *this)
        {
            std::string row_sep;
            for(auto value : row)
            {
                s += row_sep + std::to_string(value);
                row_sep = separator;
            }
            s += "\n";
        }
        return s;
    }

    throw exception("Matrix must have one or two dimensions for conversion to csv.");
}


void
matrix::print(std::string n) const
{
    if(!n.empty())
        std::cout << n << " = ";
    else if(!info_->name_.empty())
        std::cout << info_->name_ << " = ";
    if(rank() == 1)
    {
        std::string sep;
        std::cout << "{";
        for(auto v : *this)
        {
            std::cout << sep << v;
            sep = ", ";
        }
        std::cout << "}";
    }
    else
        print_();
    std::cout << std::endl;
}


matrix &
matrix::reduce(std::function<void(float)> f)
{
    if(empty())
        return *this;
    if(info_->continuous)
    {
        float * data = data_->data() + info_->offset_;
        for(int i = 0; i < info_->size_; ++i)
            f(data[i]);
        return *this;
    }
    if(is_scalar())
        f((*data_)[info_->offset_]);
    else
        for(int i = 0; i < info_->shape_.front(); ++i)
            (*this)[i].reduce(f);
    return *this;
}


matrix &
matrix::apply(std::function<float(float)> f)
{
    if(empty())
        return *this;
    if(info_->continuous)
    {
        float * data = data_->data() + info_->offset_;
        for(int i = 0; i < info_->size_; ++i)
            data[i] = f(data[i]);
        return *this;
    }
    if(is_scalar())
        (*data_)[info_->offset_] = f((*data_)[info_->offset_]);
    else
        for(int i = 0; i < info_->shape_.front(); ++i)
            (*this)[i].apply(f);
    return *this;
}


matrix &
matrix::apply(const matrix & A, std::function<float(float, float)> f)
{
    if(empty())
        return *this;
    else if(info_->continuous && A.info_->continuous)
    {
        float * data = data_->data() + info_->offset_;
        const float * a = A.data_->data() + A.info_->offset_;
        for(int i = 0; i < info_->size_; ++i)
            data[i] = f(data[i], a[i]);
        return *this;
    }
    else if(is_scalar())
        (*data_)[info_->offset_] = f((*data_)[info_->offset_], (*A.data_)[info_->offset_]);
    else
        for(int i = 0; i < info_->shape_.front(); ++i)
        {
            matrix X = (*this)[i];
            X.apply(A[i], f);
        }
    return *this;
}


matrix &
matrix::apply(const matrix & A, const matrix & B, std::function<float(float, float)> f)
{
    if(empty())
        return *this;
    else if(info_->continuous && A.info_->continuous && B.info_->continuous)
    {
        float * data = data_->data() + info_->offset_;
        const float * a = A.data_->data() + A.info_->offset_;
        const float * b = B.data_->data() + B.info_->offset_;
        for(int i = 0; i < info_->size_; ++i)
            data[i] = f(a[i], b[i]);
        return *this;
    }
    else if(is_scalar())
        (*data_)[info_->offset_] = f((*A.data_)[info_->offset_], (*B.data_)[info_->offset_]);
    else
        for(int i = 0; i < info_->shape_.front(); ++i)
        {
            matrix X = (*this)[i];
            X.apply(A[i], B[i], f);
        }
    return *this;
}


matrix &
matrix::set(float v)
{
    if(info_->continuous)
    {
        std::fill(data_->begin() + info_->offset_, data_->begin() + info_->offset_ + info_->size_, v);
        return *this;
    }
    else
        return apply([=](float)->float { return v; });
}


matrix &
matrix::copy(const matrix & m)
{
    if(is_uninitialized())
        realloc(m.shape());

    if(info_->shape_ != m.info_->shape_)
        throw std::out_of_range("Assignment requires matrices of the same size");

    if(info_->continuous && m.info_->continuous)
        std::copy_n(m.data_->begin() + m.info_->offset_, m.info_->size_, data_->begin() + info_->offset_);
    else if(is_scalar())
        (*data_)[info_->offset_] = (*m.data_)[m.info_->offset_];
    else
        for(int i = 0; i < info_->shape_.front(); ++i)
            (*this)[i].copy(m[i]);
    return *this;
}


matrix &
matrix::copy(const matrix & m, range & target, range & source)
{
    if(info_->continuous && m.info_->continuous && source == target && m.info_->shape_ == info_->shape_)
        return copy(m);

    source.reset();
    target.reset();

    for(; source.more() && target.more(); source++, target++)
    {
        int source_index = m.compute_index(source.index());
        int target_index = compute_index(target.index());
        (*data_).at(target_index) = (*m.data_)[source_index];
    }
    return *this;
}


matrix &
matrix::submatrix(const matrix & m, const rect & region)
{
    int height = region.height;
    int width = region.width;

    if(is_uninitialized())
        realloc(height, width);
    else if(rows() != height || cols() != width)
        throw std::invalid_argument("Destination matrix does not have size " + std::to_string(height) + "x" + std::to_string(width) + ".");

    if(rank() != 2 || m.rank() != 2)
        throw std::invalid_argument(get_name() + " Matrix must be two-dimensional.");

    float * t = this->data();
    for(int j = 0; j < height; ++j)
        for(int i = 0; i < width; ++i)
            *t++ = m(region.y + j, region.x + i);

    return *this;
}


matrix::operator float & ()
{
#ifndef NO_MATRIX_CHECKS
    if(info_->size_ != 1)
        throw empty_matrix_error(get_name() + " Not a matrix element.");
#endif
    return (*data_)[info_->offset_];
}


matrix::operator float * ()
{
    return &(*data_).data()[info_->offset_];
}


matrix::operator float ** ()
{
    if(rank() != 2)
        throw std::out_of_range(get_name() + "Matrix must be two-dimensional.");

    if(row_pointers_.empty())
        for(int i = 0; i < info_->shape_.front(); ++i)
            row_pointers_.push_back(&(*this)(i, 0));

    return row_pointers_.data();
}


float *
matrix::data()
{
    return &data_->data()[info_->offset_];
}


const float *
matrix::data() const
{
    return &data_->data()[info_->offset_];
}


matrix &
matrix::reset()
{
    return set(0);
}


void
matrix::check_bounds(const std::vector<int> & v) const
{
#ifndef NO_MATRIX_CHECKS
    if(v.size() != info_->shape_.size())
        throw std::out_of_range(get_name() + "Index has incorrect rank.");

    for(std::size_t i = 0; i < v.size(); ++i)
        if(v[i] < 0 || v[i] >= info_->shape_[i])
            throw std::out_of_range(get_name() + "Index out of range.");
#endif
}


void
matrix::check_same_size(const matrix & A) const
{
    if(info_->shape_ != A.info_->shape_)
        throw std::invalid_argument(get_name() + A.get_name() + "Matrix sizes must match.");
}


const std::vector<int> &
matrix::shape() const
{
    return info_->shape_;
}


int
matrix::size() const
{
    return info_->size_;
}


int
matrix::shape(int dim) const
{
    if(info_->shape_.size() == 0)
        return 0;

    if(dim < 0)
        dim = info_->shape_.size() + dim;

    if(dim < 0 || static_cast<std::size_t>(dim) > info_->shape_.size() - 1)
        return 0;

    return info_->shape_.at(dim);
}

int
matrix::size(int dim) const
{
    return shape(dim);
}


int matrix::rows() const { return shape(-2); }
int matrix::cols() const { return shape(-1); }
int matrix::size_x() const { return cols(); }
int matrix::size_y() const { return rows(); }
int matrix::size_z() const { return shape(-3); }


matrix &
matrix::realloc(const std::vector<int> & shape)
{
    for(int dimension : shape)
        if(dimension < 0)
            throw std::invalid_argument(get_name() + "Matrix size cannot be negative.");

    info_->offset_ = 0;
    info_->shape_ = shape;
    info_->stride_ = shape;
    info_->max_size_ = shape;
    info_->size_ = info_->calculate_size();
    info_->labels_.resize(info_->shape_.size());
    data_->resize(info_->size_);

    return *this;
}


matrix &
matrix::realloc(const range & r)
{
    return realloc(r.extent());
}


matrix &
matrix::push(const matrix & m, bool extend)
{
#ifndef NO_MATRIX_CHECKS
    if(rank() != m.rank() + 1)
        throw std::out_of_range(get_name() + "Incompatible matrix sizes");
    if(extend)
    {
        info_->shape_.front()++;
        realloc(info_->shape_);
        info_->shape_.front()--;
    }
    for(std::size_t i = 0; i < m.info_->shape_.size(); ++i)
        if(info_->shape_[i + 1] != m.info_->shape_[i])
            throw std::out_of_range(get_name() + "Pushed matrix has wrong shape.");

    if(info_->shape_.front() >= info_->max_size_.front())
        throw std::out_of_range(get_name() + "No room for additional element");
#endif
    if(info_->shape_.front() < info_->max_size_.front())
        return (*this)[info_->shape_.front()++].copy(m);
    else
        return *this;
}


matrix &
matrix::push(float v)
{
    if(rank() != 1)
        throw std::out_of_range(get_name() + "Matrix must be one-dimensional.");
    if(info_->shape_.front() >= info_->max_size_.front())
        throw std::out_of_range(get_name() + "No room for additional element");

    info_->shape_.front()++;
    (*this)[info_->shape_.front() - 1] = v;
    return *this;
}


matrix &
matrix::pop(matrix & m)
{
#ifndef NO_MATRIX_CHECKS
    if(m.info_->shape_.front() == 0)
        throw std::out_of_range(get_name() + "Nothing to pop.");
#endif
    copy(m[m.info_->shape_.front() - 1]);
    m.info_->shape_.front()--;
    return *this;
}


matrix
matrix::operator[](std::string n)
{
    if(info_->labels_.empty())
        throw std::out_of_range(get_name() + "No labels found in matrix.");

    int i = 0;
    for(auto l : info_->labels_.at(0))
    {
        if(l == n)
            return (*this)[i];
        i++;
    }
    throw std::out_of_range(get_name() + "Label " + n + " not found.");
}


matrix
matrix::operator[](const char * n)
{
    return (*this)[std::string(n)];
}


float
matrix::operator=(float v)
{
#ifndef NO_MATRIX_CHECKS
    if(info_->size_ != 1)
        throw std::out_of_range(get_name() + "Not a matrix element.");
#endif
    data_->at(info_->offset_) = v;
    return v;
}


int
matrix::compute_index(const std::vector<int> & v) const
{
    if(v.size() != info_->stride_.size())
        throw exception(get_name() + "Number of indices must match matrix rank.");

#ifndef NO_MATRIX_CHECKS
    check_bounds(v);
#endif

    int index = info_->offset_;
    int stride = 1;
    for(int i = info_->stride_.size() - 1; i >= 0; --i)
    {
        index += v[i] * stride;
        stride *= info_->stride_[i];
    }
    return index;
}


matrix &
matrix::gaussian(float sigma)
{
    if(rank() != 0)
        throw std::invalid_argument("Gaussian function requires an empty matrix.");

    int kernel_size = ceil(6 * sigma);
    if(kernel_size % 2 == 0)
        kernel_size++;
    realloc(kernel_size, kernel_size);

    int size = rows();
    int half_size = size / 2;
    float sum = 0;
    for(int i = 0; i < size; i++)
        for(int j = 0; j < size; j++)
        {
            int x = i - half_size;
            int y = j - half_size;
            (*this)(i, j) = exp(-(x * x + y * y) / (2 * sigma * sigma));
            sum += (*this)(i, j);
        }
    for(int i = 0; i < size; i++)
        for(int j = 0; j < size; j++)
            (*this)(i, j) /= sum;

    return *this;
}


matrix &
matrix::corr(const matrix & I, const matrix & K)
{
#ifndef NO_MATRIX_CHECKS
    if(rank() != 2 || I.rank() != 2 || K.rank() != 2)
        throw std::invalid_argument("Correlation requires two-dimensional matrices.");

    if(I.cols() < K.cols() || I.rows() < K.rows())
        throw std::invalid_argument("K must fit in I");
#endif

    int rr = I.rows() - K.rows() + 1;
    int rc = I.cols() - K.cols() + 1;

    if(is_uninitialized())
        realloc(rr, rc);

    if(rows() != rr || cols() != rc)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(rr) + "x" + std::to_string(rc) + ".");

    if(this == &I || this == &K)
        throw std::invalid_argument("Result cannot be assigned to I or K.");
    reset();

    for(int j = 0; j < rows(); j++)
        for(int i = 0; i < cols(); i++)
            for(int k = 0; k < K.rows(); k++)
                for(int l = 0; l < K.cols(); l++)
                    (*this)(j, i) += I(j + k, i + l) * K(k, l);
    return *this;
}


matrix &
matrix::conv_slow(const matrix & I, const matrix & K)
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
    int r = Ir - Kr + 1;
    int c = Ic - Kc + 1;

    if(is_uninitialized())
        realloc(r, c);

    if(rows() != r || cols() != c)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(r) + "x" + std::to_string(c) + ".");

    if(this == &I || this == &K)
        throw std::invalid_argument("Result cannot be assigned to I or K.");
    reset();

    for(int j = 0; j < r; j++)
        for(int i = 0; i < c; i++)
            for(int k = 0; k < Kr; k++)
                for(int l = 0; l < Kc; l++)
                    (*this)(j, i) += I(j + k, i + l) * K(Kr - k - 1, Kc - l - 1);
    return *this;
}


matrix &
matrix::fillReflect101Border(int wx, int wy)
{
    float * image = data();
    int width = size_x();
    int height = size_y();
    int inner_w = width - 2 * wx;
    int inner_h = height - 2 * wy;

    for(int y = 0; y < height; ++y)
    {
        int src_y = reflect101(y - wy, inner_h);
        for(int x = 0; x < width; ++x)
        {
            int src_x = reflect101(x - wx, inner_w);
            int dst_idx = y * width + x;
            int src_idx = (src_y + wy) * width + (src_x + wx);
            image[dst_idx] = image[src_idx];
        }
    }
    return *this;
}


matrix &
matrix::fillExtendBorder(int wx, int wy)
{
    float * image = data();
    int width = size_x();
    int height = size_y();
    int inner_w = width - 2 * wx;
    int inner_h = height - 2 * wy;

    for(int y = 0; y < height; ++y)
    {
        int src_y = std::clamp(y - wy, 0, inner_h - 1);
        for(int x = 0; x < width; ++x)
        {
            int src_x = std::clamp(x - wx, 0, inner_w - 1);
            int dst_idx = y * width + x;
            int src_idx = (src_y + wy) * width + (src_x + wx);
            image[dst_idx] = image[src_idx];
        }
    }
    return *this;
}


std::ostream &
operator<<(std::ostream & os, const matrix & m)
{
    if(m.rank() == 0)
    {
        if(m.info_->size_ == 0)
            os << "{}";
        else if(m.info_->size_ == 1)
            os << m.data_->at(m.info_->offset_);
    }
    else
        m.print();
    return os;
}


float matrix::matrank() { throw std::logic_error("matrank(). Not implemented."); }
float matrix::trace() { throw std::logic_error("Not implemented."); }
float matrix::det() { throw std::logic_error("trace(). Not implemented."); }


matrix &
matrix::inv(const matrix & m)
{
    copy(m);
    return inv();
}


matrix &
matrix::pinv(const matrix &)
{
    throw std::logic_error("pinv(). Not implemented.");
}


matrix &
matrix::transpose(matrix & ret)
{
    int rows = this->rows();
    int cols = this->cols();

    if(ret.is_uninitialized())
        ret.realloc(cols, rows);
    else if(ret.rows() != cols || ret.cols() != rows)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(cols) + "x" + std::to_string(rows) + ".");

    for(int i = 0; i < rows; ++i)
        for(int j = 0; j < cols; ++j)
            ret(j, i) = (*this)(i, j);
    return ret;
}


matrix &
matrix::eig(const matrix &)
{
    throw std::logic_error("eig(). Not implemented.");
}


bool
matrix::operator==(float v) const
{
    if(!is_scalar())
        throw std::invalid_argument("Matrix must be scalar.");
    return ((*data_)[info_->offset_] == v);
}


bool
matrix::operator==(int v) const
{
    if(!is_scalar())
        throw std::invalid_argument("Matrix must be scalar.");
    return ((*data_)[info_->offset_] == v);
}


bool
matrix::operator==(const matrix & other) const
{
    if(this->shape() != other.shape())
        return false;

    if(rank() == 0)
        return size() == other.size() && (size() == 0 || (*data_)[info_->offset_] == (*other.data_)[other.info_->offset_]);

    for(auto ix = get_range(); ix.more(); ix++)
        if((*data_)[compute_index(ix.index())] != (*other.data_)[other.compute_index(ix.index())])
            return false;

    return true;
}


bool
matrix::operator!=(const matrix & other) const
{
    return !(*this == other);
}


bool
matrix::operator!=(float v) const
{
    if(!is_scalar())
        throw std::invalid_argument("Matrix must be scalar.");
    return ((*data_)[info_->offset_] != v);
}


bool
matrix::operator!=(int v) const
{
    if(!is_scalar())
        throw std::invalid_argument("Matrix must be scalar.");
    return ((*data_)[info_->offset_] != v);
}


std::vector<float>
flattenKernel(const matrix & K)
{
    std::vector<float> kernel_flat(K.rows() * K.cols());
    for(int k = 0; k < K.rows(); ++k)
        for(int l = 0; l < K.cols(); ++l)
            kernel_flat[k * K.cols() + l] = K(k, l);
    return kernel_flat;
}


void
im2row(std::vector<float> & submatrices_flat, const matrix & I, const matrix & K)
{
    int rr = I.rows() - K.rows() + 1;
    int rc = I.cols() - K.cols() + 1;

    const float * I_data = I.data();
    int I_cols = I.cols();
    int K_cols = K.cols();
    int K_rows = K.rows();

    size_t offset = 0;
    for(int j = 0; j < rr; ++j)
        for(int i = 0; i < rc; ++i)
            for(int k = 0; k < K_rows; ++k)
            {
                const float * input_row_start = I_data + (j + k) * I_cols + i;
                float * output_row_start = submatrices_flat.data() + offset;
                for(int l = 0; l < K_cols; ++l)
                    output_row_start[l] = input_row_start[l];
                offset += K_cols;
            }
}


void
parse_bracket_matrix_value(const value & v, std::vector<int> & shape, std::vector<float> & data, int depth)
{
    if(v.is_number())
    {
        if(depth != static_cast<int>(shape.size()))
            throw std::invalid_argument("Invalid matrix string");
        data.push_back(v.as_float());
        return;
    }

    if(!v.is_list())
        throw std::invalid_argument("Invalid matrix string");

    const list & items = std::get<list>(v.value_);
    int item_count = static_cast<int>(items.size());

    if(static_cast<int>(shape.size()) <= depth)
        shape.push_back(item_count);
    else if(shape[depth] != item_count)
        throw std::invalid_argument("Invalid matrix string");

    bool contains_numbers = false;
    bool contains_lists = false;
    for(const auto & item : items)
    {
        contains_numbers = contains_numbers || item.is_number();
        contains_lists = contains_lists || item.is_list();
        if(!item.is_number() && !item.is_list())
            throw std::invalid_argument("Invalid matrix string");
    }

    if(contains_numbers && contains_lists)
        throw std::invalid_argument("Invalid matrix string");

    for(const auto & item : items)
        parse_bracket_matrix_value(item, shape, data, depth + 1);
}


bool
try_parse_bracket_matrix_literal(matrix & out, const std::string & data_string)
{
    std::string trimmed = trim(data_string);
    if(trimmed.empty() || trimmed.front() != '[')
        return false;

    value parsed = parse_json(trimmed);
    if(!parsed.is_list())
        throw std::invalid_argument("Invalid matrix string");

    std::vector<int> shape;
    std::vector<float> data;
    parse_bracket_matrix_value(parsed, shape, data);

    out = matrix(shape);
    for(std::size_t i = 0; i < data.size(); ++i)
        (*out.data_)[i] = data[i];

    return true;
}


float
dot(const matrix & A, const matrix & B)
{
    A.check_same_size(B);

    if(A.info_->continuous && B.info_->continuous)
    {
        const float * a = A.data_->data() + A.info_->offset_;
        const float * b = B.data_->data() + B.info_->offset_;
#if defined(__APPLE__)
        float s = 0;
        vDSP_dotpr(a, 1, b, 1, &s, static_cast<vDSP_Length>(A.info_->size_));
        return s;
#else
        float s = 0;
        for(int i = 0; i < A.info_->size_; ++i)
            s += a[i] * b[i];
        return s;
#endif
    }

    if(A.is_scalar())
        return (*A.data_)[A.info_->offset_] * (*B.data_)[B.info_->offset_];

    float s = 0;
    for(int i = 0; i < A.info_->shape_.front(); ++i)
        s += dot(A[i], B[i]);
    return s;
}

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
matrix::add(float c)
{
#if defined(__APPLE__)
    if(info_->continuous)
    {
        vDSP_vsadd(data(), 1, &c, data(), 1, static_cast<vDSP_Length>(info_->size_));
        return *this;
    }
#endif
    return apply([c](float x)->float { return x + c; });
}


matrix &
matrix::subtract(float c)
{
#if defined(__APPLE__)
    if(info_->continuous)
    {
        float negated = -c;
        vDSP_vsadd(data(), 1, &negated, data(), 1, static_cast<vDSP_Length>(info_->size_));
        return *this;
    }
#endif
    return apply([c](float x)->float { return x - c; });
}


matrix &
matrix::scale(float c)
{
#if defined(__APPLE__)
    if(info_->continuous)
    {
        vDSP_vsmul(data(), 1, &c, data(), 1, info_->size_);
        return *this;
    }
#endif
    return apply([c](float x)->float { return x * c; });
}


matrix &
matrix::divide(float c)
{
#if defined(__APPLE__)
    if(info_->continuous)
    {
        vDSP_vsdiv(data(), 1, &c, data(), 1, static_cast<vDSP_Length>(info_->size_));
        return *this;
    }
#endif
    return apply([c](float x)->float { return x / c; });
}


matrix &
matrix::add(const matrix & A)
{
    check_same_size(A);

#if defined(__APPLE__)
    if(info_->continuous && A.info_->continuous)
    {
        vDSP_vadd(data(), 1, A.data(), 1, data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, [](float x, float y)->float { return x + y; });
}


matrix &
matrix::subtract(const matrix & A)
{
    check_same_size(A);

#if defined(__APPLE__)
    if(info_->continuous && A.info_->continuous)
    {
        vDSP_vsub(A.data(), 1, data(), 1, data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, [](float x, float y)->float { return x - y; });
}


matrix &
matrix::multiply(const matrix & A)
{
    check_same_size(A);

#if defined(__APPLE__)
    if(info_->continuous && A.info_->continuous)
    {
        vDSP_vmul(data(), 1, A.data(), 1, data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, [](float x, float y)->float { return x * y; });
}


matrix &
matrix::divide(const matrix & A)
{
    check_same_size(A);

#if defined(__APPLE__)
    if(info_->continuous && A.info_->continuous)
    {
        vDSP_vdiv(A.data(), 1, data(), 1, data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, [](float x, float y)->float { return x / y; });
}


matrix &
matrix::logical_and(const matrix & A)
{
    check_same_size(A);

    if(info_->continuous && A.info_->continuous)
    {
        float * lhs = data();
        const float * rhs = A.data();
        for(int i = 0; i < size(); ++i)
            lhs[i] = (lhs[i] != 0.0f && rhs[i] != 0.0f) ? 1.0f : 0.0f;
        return *this;
    }

    return apply(A, [](float x, float y)->float { return (x != 0.0f && y != 0.0f) ? 1.0f : 0.0f; });
}


matrix &
matrix::logical_or(const matrix & A)
{
    check_same_size(A);

    if(info_->continuous && A.info_->continuous)
    {
        float * lhs = data();
        const float * rhs = A.data();
        for(int i = 0; i < size(); ++i)
            lhs[i] = (lhs[i] != 0.0f || rhs[i] != 0.0f) ? 1.0f : 0.0f;
        return *this;
    }

    return apply(A, [](float x, float y)->float { return (x != 0.0f || y != 0.0f) ? 1.0f : 0.0f; });
}


matrix &
matrix::logical_xor(const matrix & A)
{
    check_same_size(A);

    if(info_->continuous && A.info_->continuous)
    {
        float * lhs = data();
        const float * rhs = A.data();
        for(int i = 0; i < size(); ++i)
            lhs[i] = ((lhs[i] != 0.0f) != (rhs[i] != 0.0f)) ? 1.0f : 0.0f;
        return *this;
    }

    return apply(A, [](float x, float y)->float { return ((x != 0.0f) != (y != 0.0f)) ? 1.0f : 0.0f; });
}


matrix &
matrix::add(const matrix & A, const matrix & B)
{
    check_same_size(A);
    check_same_size(B);

#if defined(__APPLE__)
    if(info_->continuous && A.info_->continuous && B.info_->continuous)
    {
        const float *a = A.data();
        const float *b = B.data();
        float *r = this->data();

        vDSP_vadd(b, 1, a, 1, r, 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif
    return apply(A, B, [](float x, float y)->float { return x + y; });
}


matrix &
matrix::subtract(const matrix & A, const matrix & B)
{
    check_same_size(A);
    check_same_size(B);

#if defined(__APPLE__)
    if(info_->continuous && A.info_->continuous && B.info_->continuous)
    {
        const float *a = A.data();
        const float *b = B.data();
        float *r = this->data();

        vDSP_vsub(b, 1, a, 1, r, 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif
    return apply(A, B, [](float x, float y)->float { return x - y; });
}


matrix &
matrix::multiply(const matrix & A, const matrix & B)
{
    check_same_size(A);
    check_same_size(B);

#if defined(__APPLE__)
    if(info_->continuous && A.info_->continuous && B.info_->continuous)
    {
        const float *a = A.data();
        const float *b = B.data();
        float *r = this->data();

        vDSP_vmul(b, 1, a, 1, r, 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif
    return apply(A, B, [](float x, float y)->float { return x * y; });
}


matrix &
matrix::divide(const matrix & A, const matrix & B)
{
    check_same_size(A);
    check_same_size(B);

#if defined(__APPLE__)
    if(info_->continuous && A.info_->continuous && B.info_->continuous)
    {
        const float *a = A.data();
        const float *b = B.data();
        float *r = this->data();

        vDSP_vdiv(b, 1, a, 1, r, 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif
    return apply(A, B, [](float x, float y)->float { return x / y; });
}


matrix &
matrix::maximum(const matrix & A)
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
matrix::minimum(const matrix & A)
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
matrix::maximum(const matrix & A, const matrix & B)
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
matrix::minimum(const matrix & A, const matrix & B)
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
matrix::logical_and(const matrix & A, const matrix & B)
{
    check_same_size(A);
    check_same_size(B);

    if(info_->continuous && A.info_->continuous && B.info_->continuous)
    {
        float * out = data();
        const float * lhs = A.data();
        const float * rhs = B.data();
        for(int i = 0; i < size(); ++i)
            out[i] = (lhs[i] != 0.0f && rhs[i] != 0.0f) ? 1.0f : 0.0f;
        return *this;
    }

    return apply(A, B, [](float x, float y)->float { return (x != 0.0f && y != 0.0f) ? 1.0f : 0.0f; });
}


matrix &
matrix::logical_or(const matrix & A, const matrix & B)
{
    check_same_size(A);
    check_same_size(B);

    if(info_->continuous && A.info_->continuous && B.info_->continuous)
    {
        float * out = data();
        const float * lhs = A.data();
        const float * rhs = B.data();
        for(int i = 0; i < size(); ++i)
            out[i] = (lhs[i] != 0.0f || rhs[i] != 0.0f) ? 1.0f : 0.0f;
        return *this;
    }

    return apply(A, B, [](float x, float y)->float { return (x != 0.0f || y != 0.0f) ? 1.0f : 0.0f; });
}


matrix &
matrix::logical_xor(const matrix & A, const matrix & B)
{
    check_same_size(A);
    check_same_size(B);

    if(info_->continuous && A.info_->continuous && B.info_->continuous)
    {
        float * out = data();
        const float * lhs = A.data();
        const float * rhs = B.data();
        for(int i = 0; i < size(); ++i)
            out[i] = ((lhs[i] != 0.0f) != (rhs[i] != 0.0f)) ? 1.0f : 0.0f;
        return *this;
    }

    return apply(A, B, [](float x, float y)->float { return ((x != 0.0f) != (y != 0.0f)) ? 1.0f : 0.0f; });
}


matrix &
matrix::hypot(const matrix & x, const matrix & y)
{
    if(is_uninitialized())
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
matrix::atan2(const matrix & y, const matrix & x)
{
    if(is_uninitialized())
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
matrix::matmul(const matrix & A, const matrix & B)
{
    if(is_uninitialized())
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

#if !defined(__APPLE__)
    throw std::runtime_error("matrix::inv() requires Apple Accelerate LAPACK today. A portable fallback could use Eigen, LAPACK/OpenBLAS, or a local Gauss-Jordan/LU implementation.");
#else
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
#endif
}


matrix &
matrix::corr3(const matrix &I, const matrix &K, const std::vector<float> &kernel_flat, const std::vector<float> &submatrices_flat)
{
    if(is_uninitialized())
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
matrix::conv(const matrix &I, const matrix &K)
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

    if(is_uninitialized())
        realloc(Ir, Ic);

    if(rows() != Ir || cols() != Ic)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(Ir) + "x" + std::to_string(Ic) + ".");

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
matrix::singular_value_decomposition(const matrix& inputMatrix, matrix& U, matrix& S, matrix& Vt)
{
#if !defined(__APPLE__)
    throw std::runtime_error("matrix::singular_value_decomposition() requires Apple Accelerate LAPACK today. A portable fallback could use Eigen, LAPACK/OpenBLAS, or another SVD-capable linear algebra library.");
#else
    int m = inputMatrix.size_y();
    int n = inputMatrix.size_x();
    int min_mn = std::min(m, n);

    if(U.is_uninitialized())
        U.realloc(m, m);
    else if(U.rows() != m || U.cols() != m)
        throw std::invalid_argument("U matrix does not have size " + std::to_string(m) + "x" + std::to_string(m) + ".");

    if(Vt.is_uninitialized())
        Vt.realloc(n, n);
    else if(Vt.rows() != n || Vt.cols() != n)
        throw std::invalid_argument("Vt matrix does not have size " + std::to_string(n) + "x" + std::to_string(n) + ".");

    if(S.is_uninitialized())
        S.realloc(m, n);
    else if(S.rows() != m || S.cols() != n)
        throw std::invalid_argument("S matrix does not have size " + std::to_string(m) + "x" + std::to_string(n) + ".");

    std::vector<float> a(inputMatrix.data(), inputMatrix.data() + m * n);

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

    std::copy(s.begin(), s.end(), S.data());
#endif
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

    if (is_uninitialized()) {
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

        if (is_uninitialized()) {
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

#if !defined(__APPLE__)
        (void)kernel;
        (void)submatrix;
        (void)buffer;
        (void)len;
        throw std::runtime_error("matrix::search() requires Apple Accelerate today. A portable fallback could use Eigen, OpenCV, or a local scalar normalized-correlation implementation.");
#else
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
#endif
    }


    // Search for a kernel in this matrix
    // Returns the best matching point in the search rectangle together with its matching score
    // TODO: Add non-Apple implementation for other platforms
    
    match
    matrix::search(const matrix & target, const rect & search_rectangle) const
    {
#if !defined(__APPLE__)
        (void)target;
        (void)search_rectangle;
        throw std::runtime_error("matrix::search() requires Apple Accelerate today. A portable fallback could use Eigen, OpenCV, or a local scalar normalized-correlation implementation.");
#else
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
#endif
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
