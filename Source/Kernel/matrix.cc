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

#include <charconv>

namespace ikaros {

float
parse_matrix_token(const std::string & token)
{
    std::string trimmed = trim(token);
    float value = 0;
    const char * begin = trimmed.data();
    const char * end = begin + trimmed.size();
    auto result = std::from_chars(begin, end, value);
    if(result.ec != std::errc() || result.ptr != end)
        throw std::invalid_argument("Invalid matrix value \"" + token + "\". Values must be separated by ',' or ';'.");
    return value;
}


matrix_info::matrix_info(std::vector<int> shape):
    offset_(0), shape_(shape), stride_(shape), max_size_(shape), size_(calculate_size()), continuous(true), dynamic_(false), fixed_capacity_(false), labels_(shape.size())
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
    print_attribute_value("dynamic", dynamic_);
    print_attribute_value("fixed_capacity", fixed_capacity_);
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
matrix::is_dynamic() const
{
    return info_->dynamic_;
}


matrix &
matrix::set_dynamic(bool dynamic)
{
    info_->dynamic_ = dynamic;
    return *this;
}


matrix &
matrix::set_fixed_capacity(bool fixed_capacity)
{
    info_->fixed_capacity_ = fixed_capacity;
    return *this;
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
matrix::metadata_json() const
{
    std::string s = "{";
    s += "\"name\": " + value(info_->name_).json();
    s += ", \"rank\": " + std::to_string(rank());
    s += ", \"shape\": [";

    std::string sep;
    for(int dim : info_->shape_)
    {
        s += sep + std::to_string(dim);
        sep = ", ";
    }

    s += "], \"labels\": [";
    sep.clear();
    for(const auto & dimension_labels : info_->labels_)
    {
        s += sep + "[";
        std::string label_sep;
        for(const auto & label : dimension_labels)
        {
            s += label_sep + value(label).json();
            label_sep = ", ";
        }
        s += "]";
        sep = ", ";
    }

    s += "]}";
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


const std::vector<int> &
matrix::capacity() const
{
    return info_->max_size_;
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
matrix::resize(const std::vector<int> & new_shape)
{
#ifndef NO_MATRIX_CHECKS
    if(new_shape.size() != info_->shape_.size())
        throw std::invalid_argument("Number of indices must match matrix rank (in call to resize).");

    for(std::size_t i = 0; i < info_->shape_.size(); ++i)
        if(new_shape[i] > info_->max_size_[i])
            throw std::out_of_range(get_name()+"New size larger than allocated space.");
#endif
    info_->shape_ = new_shape;
    return *this;
}


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
matrix::reserve(const std::vector<int> & capacity_shape)
{
    if(capacity_shape.empty())
        throw std::invalid_argument(get_name() + "Reserve requires at least one dimension.");

    for(int dimension : capacity_shape)
        if(dimension < 0)
            throw std::invalid_argument(get_name() + "Matrix capacity cannot be negative.");

    if(is_uninitialized())
    {
        realloc(capacity_shape);
        info_->shape_.front() = 0;
        return *this;
    }

    if(rank() != static_cast<int>(capacity_shape.size()))
        throw std::invalid_argument(get_name() + "Reserved capacity must have the same rank as the matrix.");

    for(std::size_t i = 1; i < capacity_shape.size(); ++i)
        if(info_->shape_[i] != capacity_shape[i])
            throw std::invalid_argument(get_name() + "Reserved capacity must match the matrix slice shape.");

    if(capacity_shape.front() < info_->shape_.front())
        throw std::out_of_range(get_name() + "Reserved capacity cannot be smaller than the current matrix size.");

    if(capacity_shape.front() > info_->max_size_.front())
    {
        std::vector<int> logical_shape = info_->shape_;
        realloc(capacity_shape);
        info_->shape_ = logical_shape;
    }

    return *this;
}


matrix &
matrix::clear()
{
    if(rank() == 0)
        return *this;

    info_->shape_.front() = 0;
    return *this;
}


matrix &
matrix::append(const matrix & m)
{
    if(is_uninitialized())
    {
        std::vector<int> capacity_shape;
        capacity_shape.reserve(m.info_->shape_.size() + 1);
        capacity_shape.push_back(1);
        capacity_shape.insert(capacity_shape.end(), m.info_->shape_.begin(), m.info_->shape_.end());
        reserve(capacity_shape);
    }

    if(rank() != m.rank() + 1)
        throw std::out_of_range(get_name() + "Incompatible matrix sizes");

    for(std::size_t i = 0; i < m.info_->shape_.size(); ++i)
        if(info_->shape_[i + 1] != m.info_->shape_[i])
            throw std::out_of_range(get_name() + "Appended matrix has wrong shape.");

    if(info_->shape_.front() >= info_->max_size_.front())
    {
        if(info_->fixed_capacity_)
            throw std::out_of_range(get_name() + "No room for additional element");

        std::vector<int> capacity_shape = info_->max_size_;
        capacity_shape.front() = std::max(1, capacity_shape.front() * 2);
        reserve(capacity_shape);
    }

    return push(m);
}


matrix &
matrix::append(float v)
{
    if(is_uninitialized())
        reserve(1);

    if(rank() != 1)
        throw std::out_of_range(get_name() + "Matrix must be one-dimensional.");

    if(info_->shape_.front() >= info_->max_size_.front())
    {
        if(info_->fixed_capacity_)
            throw std::out_of_range(get_name() + "No room for additional element");

        std::vector<int> capacity_shape = info_->max_size_;
        capacity_shape.front() = std::max(1, capacity_shape.front() * 2);
        reserve(capacity_shape);
    }

    return push(v);
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

// conv_slow is a fallback funtion. Use conv() instead of conv_slow() for better performance.

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
matrix::conv2_valid_filterbank(const matrix & I, const matrix & K)
{
#ifndef NO_MATRIX_CHECKS
    if(I.rank() != 2 || K.rank() != 3)
        throw std::invalid_argument("Filter-bank valid convolution requires input [H,W] and filters [F,KH,KW].");

    if(K.shape(1) <= 0 || K.shape(2) <= 0 || I.rows() < K.shape(1) || I.cols() < K.shape(2))
        throw std::invalid_argument("Filter kernel must fit in input.");
#endif

    const int filters = K.shape(0);
    const int kernel_rows = K.shape(1);
    const int kernel_cols = K.shape(2);
    const int output_rows = I.rows() - kernel_rows + 1;
    const int output_cols = I.cols() - kernel_cols + 1;

    if(is_uninitialized())
        realloc(output_rows, output_cols, filters);

    if(rank() != 3 || shape(0) != output_rows || shape(1) != output_cols || shape(2) != filters)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(output_rows) + "x" + std::to_string(output_cols) + "x" + std::to_string(filters) + ".");

    if(this == &I || this == &K)
        throw std::invalid_argument("Result cannot be assigned to I or K.");

    float * output = data();
    const float * input = I.data();
    const float * filters_data = K.data();

    for(int y = 0; y < output_rows; ++y)
    {
        for(int x = 0; x < output_cols; ++x)
        {
            const int output_base = (y * output_cols + x) * filters;
            for(int f = 0; f < filters; ++f)
            {
                const float * filter = filters_data + f * kernel_rows * kernel_cols;
                float sum = 0.0f;
                for(int ky = 0; ky < kernel_rows; ++ky)
                {
                    const float * input_row = input + (y + ky) * I.cols() + x;
                    const float * filter_row = filter + ky * kernel_cols;
                    for(int kx = 0; kx < kernel_cols; ++kx)
                        sum += input_row[kx] * filter_row[kx];
                }
                output[output_base + f] = sum;
            }
        }
    }

    return *this;
}


matrix &
matrix::conv2_valid_filterbank(const matrix & I, const matrix & K, const matrix & B)
{
#ifndef NO_MATRIX_CHECKS
    if(B.rank() != 1 || B.size() != K.shape(0))
        throw std::invalid_argument("Filter-bank bias must be a vector with one value per filter.");
#endif

    conv2_valid_filterbank(I, K);

    float * output = data();
    const float * bias = B.data();
    const int filters = shape(2);
    const int output_pixels = shape(0) * shape(1);

    for(int pixel = 0; pixel < output_pixels; ++pixel)
    {
        float * output_pixel = output + pixel * filters;
        for(int f = 0; f < filters; ++f)
            output_pixel[f] += bias[f];
    }

    return *this;
}


matrix &
matrix::conv2_valid_filterbank_backward_filters(const matrix & I, const matrix & dY, int kernel_rows, int kernel_cols)
{
#ifndef NO_MATRIX_CHECKS
    if(I.rank() != 2 || dY.rank() != 3)
        throw std::invalid_argument("Filter-bank filter-gradient requires input [H,W] and output gradient [OH,OW,F].");

    if(kernel_rows <= 0 || kernel_cols <= 0 || I.rows() < kernel_rows || I.cols() < kernel_cols)
        throw std::invalid_argument("Filter kernel size must fit in input.");

    if(dY.shape(0) != I.rows() - kernel_rows + 1 || dY.shape(1) != I.cols() - kernel_cols + 1)
        throw std::invalid_argument("Output gradient shape does not match input and kernel size.");
#endif

    const int filters = dY.shape(2);

    if(is_uninitialized())
        realloc(filters, kernel_rows, kernel_cols);

    if(rank() != 3 || shape(0) != filters || shape(1) != kernel_rows || shape(2) != kernel_cols)
        throw std::invalid_argument("Filter-gradient result matrix does not have size " + std::to_string(filters) + "x" + std::to_string(kernel_rows) + "x" + std::to_string(kernel_cols) + ".");

    if(this == &I || this == &dY)
        throw std::invalid_argument("Result cannot be assigned to I or dY.");

    reset();

    float * d_filters = data();
    const float * input = I.data();
    const float * d_output = dY.data();
    const int input_cols = I.cols();
    const int output_rows = dY.shape(0);
    const int output_cols = dY.shape(1);

    for(int y = 0; y < output_rows; ++y)
    {
        for(int x = 0; x < output_cols; ++x)
        {
            const float * d_output_pixel = d_output + (y * output_cols + x) * filters;
            for(int f = 0; f < filters; ++f)
            {
                const float gradient = d_output_pixel[f];
                float * d_filter = d_filters + f * kernel_rows * kernel_cols;
                for(int ky = 0; ky < kernel_rows; ++ky)
                {
                    const float * input_row = input + (y + ky) * input_cols + x;
                    float * d_filter_row = d_filter + ky * kernel_cols;
                    for(int kx = 0; kx < kernel_cols; ++kx)
                        d_filter_row[kx] += input_row[kx] * gradient;
                }
            }
        }
    }

    return *this;
}


matrix &
matrix::conv2_valid_filterbank_backward_filters_relu(const matrix & I, const matrix & dY, const matrix & pre_activation, int kernel_rows, int kernel_cols)
{
#ifndef NO_MATRIX_CHECKS
    if(I.rank() != 2 || dY.rank() != 3 || pre_activation.rank() != 3)
        throw std::invalid_argument("Fused filter-bank filter-gradient requires input [H,W], output gradient [OH,OW,F], and pre-activation [OH,OW,F].");

    if(dY.shape() != pre_activation.shape())
        throw std::invalid_argument("Output gradient and pre-activation shapes must match.");

    if(kernel_rows <= 0 || kernel_cols <= 0 || I.rows() < kernel_rows || I.cols() < kernel_cols)
        throw std::invalid_argument("Filter kernel size must fit in input.");

    if(dY.shape(0) != I.rows() - kernel_rows + 1 || dY.shape(1) != I.cols() - kernel_cols + 1)
        throw std::invalid_argument("Output gradient shape does not match input and kernel size.");
#endif

    const int filters = dY.shape(2);

    if(is_uninitialized())
        realloc(filters, kernel_rows, kernel_cols);

    if(rank() != 3 || shape(0) != filters || shape(1) != kernel_rows || shape(2) != kernel_cols)
        throw std::invalid_argument("Filter-gradient result matrix does not have size " + std::to_string(filters) + "x" + std::to_string(kernel_rows) + "x" + std::to_string(kernel_cols) + ".");

    if(this == &I || this == &dY || this == &pre_activation)
        throw std::invalid_argument("Result cannot be assigned to I, dY, or pre_activation.");

    reset();

    float * d_filters = data();
    const float * input = I.data();
    const float * d_output = dY.data();
    const float * pre = pre_activation.data();
    const int input_cols = I.cols();
    const int output_rows = dY.shape(0);
    const int output_cols = dY.shape(1);

    for(int y = 0; y < output_rows; ++y)
    {
        for(int x = 0; x < output_cols; ++x)
        {
            const int output_base = (y * output_cols + x) * filters;
            for(int f = 0; f < filters; ++f)
            {
                if(pre[output_base + f] <= 0.0f)
                    continue;

                const float gradient = d_output[output_base + f];
                float * d_filter = d_filters + f * kernel_rows * kernel_cols;
                for(int ky = 0; ky < kernel_rows; ++ky)
                {
                    const float * input_row = input + (y + ky) * input_cols + x;
                    float * d_filter_row = d_filter + ky * kernel_cols;
                    for(int kx = 0; kx < kernel_cols; ++kx)
                        d_filter_row[kx] += input_row[kx] * gradient;
                }
            }
        }
    }

    return *this;
}


matrix &
matrix::conv2_valid_filterbank_backward_input(const matrix & dY, const matrix & K, int input_rows, int input_cols)
{
#ifndef NO_MATRIX_CHECKS
    if(dY.rank() != 3 || K.rank() != 3)
        throw std::invalid_argument("Filter-bank input-gradient requires output gradient [OH,OW,F] and filters [F,KH,KW].");

    if(input_rows <= 0 || input_cols <= 0)
        throw std::invalid_argument("Input-gradient dimensions must be positive.");

    if(dY.shape(2) != K.shape(0))
        throw std::invalid_argument("Output-gradient filter count does not match filters.");

    if(dY.shape(0) != input_rows - K.shape(1) + 1 || dY.shape(1) != input_cols - K.shape(2) + 1)
        throw std::invalid_argument("Output gradient shape does not match requested input size and filters.");
#endif

    if(is_uninitialized())
        realloc(input_rows, input_cols);

    if(rank() != 2 || rows() != input_rows || cols() != input_cols)
        throw std::invalid_argument("Input-gradient result matrix does not have size " + std::to_string(input_rows) + "x" + std::to_string(input_cols) + ".");

    if(this == &dY || this == &K)
        throw std::invalid_argument("Result cannot be assigned to dY or K.");

    reset();

    const int filters = K.shape(0);
    const int kernel_rows = K.shape(1);
    const int kernel_cols = K.shape(2);
    const int output_rows = dY.shape(0);
    const int output_cols = dY.shape(1);
    float * d_input = data();
    const float * d_output = dY.data();
    const float * filters_data = K.data();

    for(int y = 0; y < output_rows; ++y)
    {
        for(int x = 0; x < output_cols; ++x)
        {
            const float * d_output_pixel = d_output + (y * output_cols + x) * filters;
            for(int f = 0; f < filters; ++f)
            {
                const float gradient = d_output_pixel[f];
                const float * filter = filters_data + f * kernel_rows * kernel_cols;
                for(int ky = 0; ky < kernel_rows; ++ky)
                {
                    float * d_input_row = d_input + (y + ky) * input_cols + x;
                    const float * filter_row = filter + ky * kernel_cols;
                    for(int kx = 0; kx < kernel_cols; ++kx)
                        d_input_row[kx] += gradient * filter_row[kx];
                }
            }
        }
    }

    return *this;
}


matrix &
matrix::sum_first_two_dimensions(const matrix & A)
{
#ifndef NO_MATRIX_CHECKS
    if(A.rank() != 3)
        throw std::invalid_argument("sum_first_two_dimensions requires a rank-3 matrix [H,W,F].");
#endif

    const int first = A.shape(0);
    const int second = A.shape(1);
    const int third = A.shape(2);

    if(is_uninitialized())
        realloc(third);

    if(rank() != 1 || size() != third)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(third) + ".");

    if(this == &A)
        throw std::invalid_argument("Result cannot be assigned to A.");

    reset();

    const float * input = A.data();
    float * output = data();
    const int rows = first * second;

    for(int row = 0; row < rows; ++row)
    {
        const float * input_row = input + row * third;
        for(int i = 0; i < third; ++i)
            output[i] += input_row[i];
    }

    return *this;
}


matrix &
matrix::sum_first_two_dimensions_relu(const matrix & A, const matrix & pre_activation)
{
#ifndef NO_MATRIX_CHECKS
    if(A.rank() != 3 || pre_activation.rank() != 3)
        throw std::invalid_argument("sum_first_two_dimensions_relu requires rank-3 matrices [H,W,F].");

    if(A.shape() != pre_activation.shape())
        throw std::invalid_argument("Input and pre-activation shapes must match.");
#endif

    const int third = A.shape(2);

    if(is_uninitialized())
        realloc(third);

    if(rank() != 1 || size() != third)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(third) + ".");

    if(this == &A || this == &pre_activation)
        throw std::invalid_argument("Result cannot be assigned to A or pre_activation.");

    reset();

    const float * input = A.data();
    const float * pre = pre_activation.data();
    float * output = data();
    const int rows = A.shape(0) * A.shape(1);

    for(int row = 0; row < rows; ++row)
    {
        const int base = row * third;
        for(int i = 0; i < third; ++i)
            if(pre[base + i] > 0.0f)
                output[i] += input[base + i];
    }

    return *this;
}


matrix &
matrix::conv1x1_map(const matrix & I, const matrix & W)
{
#ifndef NO_MATRIX_CHECKS
    if(I.rank() != 3 || W.rank() != 2)
        throw std::invalid_argument("1x1 map projection requires input [H,W,C] and weights [O,C].");

    if(I.shape(2) != W.shape(1))
        throw std::invalid_argument("Input channel count must match 1x1 weight input count.");
#endif

    const int rows = I.shape(0);
    const int cols = I.shape(1);
    const int input_channels = I.shape(2);
    const int output_channels = W.shape(0);

    if(is_uninitialized())
        realloc(rows, cols, output_channels);

    if(rank() != 3 || shape(0) != rows || shape(1) != cols || shape(2) != output_channels)
        throw std::invalid_argument("1x1 map projection result matrix has wrong shape.");

    if(this == &I || this == &W)
        throw std::invalid_argument("Result cannot be assigned to I or W.");

    const float * input = I.data();
    const float * weights = W.data();
    float * output = data();
    const int positions = rows * cols;

    for(int p = 0; p < positions; ++p)
    {
        const float * input_position = input + p * input_channels;
        float * output_position = output + p * output_channels;
        for(int o = 0; o < output_channels; ++o)
        {
            const float * weight_row = weights + o * input_channels;
            float value = 0.0f;
            for(int c = 0; c < input_channels; ++c)
                value += input_position[c] * weight_row[c];
            output_position[o] = value;
        }
    }

    return *this;
}


matrix &
matrix::conv1x1_map(const matrix & I, const matrix & W, const matrix & B)
{
#ifndef NO_MATRIX_CHECKS
    if(B.rank() != 1 || B.size() != W.shape(0))
        throw std::invalid_argument("1x1 map bias must be a vector with one value per output channel.");
#endif

    conv1x1_map(I, W);

    float * output = data();
    const float * bias = B.data();
    const int output_channels = shape(2);
    const int positions = shape(0) * shape(1);

    for(int p = 0; p < positions; ++p)
    {
        float * output_position = output + p * output_channels;
        for(int o = 0; o < output_channels; ++o)
            output_position[o] += bias[o];
    }

    return *this;
}


matrix &
matrix::conv1x1_map_backward_weights(const matrix & I, const matrix & dY)
{
#ifndef NO_MATRIX_CHECKS
    if(I.rank() != 3 || dY.rank() != 3)
        throw std::invalid_argument("1x1 map weight-gradient requires input [H,W,C] and output gradient [H,W,O].");

    if(I.shape(0) != dY.shape(0) || I.shape(1) != dY.shape(1))
        throw std::invalid_argument("Input and output gradient spatial shapes must match.");
#endif

    const int input_channels = I.shape(2);
    const int output_channels = dY.shape(2);

    if(is_uninitialized())
        realloc(output_channels, input_channels);

    if(rank() != 2 || rows() != output_channels || cols() != input_channels)
        throw std::invalid_argument("1x1 map weight-gradient result matrix has wrong shape.");

    if(this == &I || this == &dY)
        throw std::invalid_argument("Result cannot be assigned to I or dY.");

    reset();

    const float * input = I.data();
    const float * output_gradient = dY.data();
    float * weight_gradient = data();
    const int positions = I.shape(0) * I.shape(1);

    for(int p = 0; p < positions; ++p)
    {
        const float * input_position = input + p * input_channels;
        const float * gradient_position = output_gradient + p * output_channels;
        for(int o = 0; o < output_channels; ++o)
        {
            const float gradient = gradient_position[o];
            float * weight_gradient_row = weight_gradient + o * input_channels;
            for(int c = 0; c < input_channels; ++c)
                weight_gradient_row[c] += gradient * input_position[c];
        }
    }

    return *this;
}


matrix &
matrix::conv1x1_map_backward_input(const matrix & W, const matrix & dY)
{
#ifndef NO_MATRIX_CHECKS
    if(W.rank() != 2 || dY.rank() != 3)
        throw std::invalid_argument("1x1 map input-gradient requires weights [O,C] and output gradient [H,W,O].");

    if(dY.shape(2) != W.shape(0))
        throw std::invalid_argument("Output gradient channel count must match 1x1 weight output count.");
#endif

    const int rows = dY.shape(0);
    const int cols = dY.shape(1);
    const int output_channels = W.shape(0);
    const int input_channels = W.shape(1);

    if(is_uninitialized())
        realloc(rows, cols, input_channels);

    if(rank() != 3 || shape(0) != rows || shape(1) != cols || shape(2) != input_channels)
        throw std::invalid_argument("1x1 map input-gradient result matrix has wrong shape.");

    if(this == &W || this == &dY)
        throw std::invalid_argument("Result cannot be assigned to W or dY.");

    reset();

    const float * weights = W.data();
    const float * output_gradient = dY.data();
    float * input_gradient = data();
    const int positions = rows * cols;

    for(int p = 0; p < positions; ++p)
    {
        const float * gradient_position = output_gradient + p * output_channels;
        float * input_gradient_position = input_gradient + p * input_channels;
        for(int o = 0; o < output_channels; ++o)
        {
            const float gradient = gradient_position[o];
            const float * weight_row = weights + o * input_channels;
            for(int c = 0; c < input_channels; ++c)
                input_gradient_position[c] += gradient * weight_row[c];
        }
    }

    return *this;
}


matrix &
matrix::conv1x1_map_backward(const matrix & I, const matrix & W, const matrix & dY, matrix & dW, matrix & dB)
{
#ifndef NO_MATRIX_CHECKS
    if(I.rank() != 3 || W.rank() != 2 || dY.rank() != 3)
        throw std::invalid_argument("1x1 map backward requires input [H,W,C], weights [O,C], and output gradient [H,W,O].");

    if(I.shape(0) != dY.shape(0) || I.shape(1) != dY.shape(1))
        throw std::invalid_argument("Input and output gradient spatial shapes must match.");

    if(I.shape(2) != W.shape(1) || dY.shape(2) != W.shape(0))
        throw std::invalid_argument("1x1 map backward channel counts do not match.");
#endif

    const int rows = I.shape(0);
    const int cols = I.shape(1);
    const int input_channels = W.shape(1);
    const int output_channels = W.shape(0);

    if(is_uninitialized())
        realloc(rows, cols, input_channels);
    if(dW.is_uninitialized())
        dW.realloc(output_channels, input_channels);
    if(dB.is_uninitialized())
        dB.realloc(output_channels);

    if(rank() != 3 || shape(0) != rows || shape(1) != cols || shape(2) != input_channels)
        throw std::invalid_argument("1x1 map input-gradient result matrix has wrong shape.");
    if(dW.rank() != 2 || dW.rows() != output_channels || dW.cols() != input_channels)
        throw std::invalid_argument("1x1 map weight-gradient result matrix has wrong shape.");
    if(dB.rank() != 1 || dB.size() != output_channels)
        throw std::invalid_argument("1x1 map bias-gradient result matrix has wrong shape.");

    if(this == &I || this == &W || this == &dY || this == &dW || this == &dB || &dW == &dB)
        throw std::invalid_argument("1x1 map backward results must not alias inputs or each other.");

    reset();
    dW.reset();
    dB.reset();

    const float * input = I.data();
    const float * weights = W.data();
    const float * output_gradient = dY.data();
    float * input_gradient = data();
    float * weight_gradient = dW.data();
    float * bias_gradient = dB.data();
    const int positions = rows * cols;

    for(int p = 0; p < positions; ++p)
    {
        const float * input_position = input + p * input_channels;
        const float * gradient_position = output_gradient + p * output_channels;
        float * input_gradient_position = input_gradient + p * input_channels;

        for(int o = 0; o < output_channels; ++o)
        {
            const float gradient = gradient_position[o];
            const float * weight_row = weights + o * input_channels;
            float * weight_gradient_row = weight_gradient + o * input_channels;

            bias_gradient[o] += gradient;
            for(int c = 0; c < input_channels; ++c)
            {
                weight_gradient_row[c] += gradient * input_position[c];
                input_gradient_position[c] += gradient * weight_row[c];
            }
        }
    }

    return *this;
}


matrix &
matrix::conv2_valid_channel_filterbank(const matrix & I, const matrix & K)
{
#ifndef NO_MATRIX_CHECKS
    if(I.rank() != 3 || K.rank() != 4)
        throw std::invalid_argument("Channel filter-bank valid convolution requires input [H,W,C] and filters [O,KH,KW,C].");

    if(K.shape(1) <= 0 || K.shape(2) <= 0 || I.shape(0) < K.shape(1) || I.shape(1) < K.shape(2))
        throw std::invalid_argument("Channel filter kernel must fit in input.");

    if(I.shape(2) != K.shape(3))
        throw std::invalid_argument("Input channel count must match channel filter input count.");
#endif

    const int input_rows = I.shape(0);
    const int input_cols = I.shape(1);
    const int input_channels = I.shape(2);
    const int filters = K.shape(0);
    const int kernel_rows = K.shape(1);
    const int kernel_cols = K.shape(2);
    const int output_rows = input_rows - kernel_rows + 1;
    const int output_cols = input_cols - kernel_cols + 1;

    if(is_uninitialized())
        realloc(output_rows, output_cols, filters);

    if(rank() != 3 || shape(0) != output_rows || shape(1) != output_cols || shape(2) != filters)
        throw std::invalid_argument("Channel filter-bank result matrix has wrong shape.");

    if(this == &I || this == &K)
        throw std::invalid_argument("Result cannot be assigned to I or K.");

    const float * input = I.data();
    const float * filters_data = K.data();
    float * output = data();
    const int input_row_stride = input_cols * input_channels;
    const int kernel_plane = kernel_rows * kernel_cols * input_channels;

    for(int y = 0; y < output_rows; ++y)
    {
        for(int x = 0; x < output_cols; ++x)
        {
            float * output_pixel = output + (y * output_cols + x) * filters;
            for(int f = 0; f < filters; ++f)
            {
                const float * filter = filters_data + f * kernel_plane;
                float sum = 0.0f;
                for(int ky = 0; ky < kernel_rows; ++ky)
                {
                    const float * input_pixel = input + (y + ky) * input_row_stride + x * input_channels;
                    const float * filter_pixel = filter + ky * kernel_cols * input_channels;
                    for(int kx = 0; kx < kernel_cols; ++kx)
                    {
                        for(int c = 0; c < input_channels; ++c)
                            sum += input_pixel[c] * filter_pixel[c];
                        input_pixel += input_channels;
                        filter_pixel += input_channels;
                    }
                }
                output_pixel[f] = sum;
            }
        }
    }

    return *this;
}


matrix &
matrix::conv2_valid_channel_filterbank(const matrix & I, const matrix & K, const matrix & B)
{
#ifndef NO_MATRIX_CHECKS
    if(B.rank() != 1 || B.size() != K.shape(0))
        throw std::invalid_argument("Channel filter-bank bias must be a vector with one value per filter.");
#endif

    conv2_valid_channel_filterbank(I, K);

    float * output = data();
    const float * bias = B.data();
    const int filters = shape(2);
    const int output_pixels = shape(0) * shape(1);

    for(int pixel = 0; pixel < output_pixels; ++pixel)
    {
        float * output_pixel = output + pixel * filters;
        for(int f = 0; f < filters; ++f)
            output_pixel[f] += bias[f];
    }

    return *this;
}


matrix &
matrix::conv2_valid_channel_filterbank_backward_filters(const matrix & I, const matrix & dY, int kernel_rows, int kernel_cols)
{
#ifndef NO_MATRIX_CHECKS
    if(I.rank() != 3 || dY.rank() != 3)
        throw std::invalid_argument("Channel filter-bank filter-gradient requires input [H,W,C] and output gradient [OH,OW,O].");

    if(kernel_rows <= 0 || kernel_cols <= 0 || I.shape(0) < kernel_rows || I.shape(1) < kernel_cols)
        throw std::invalid_argument("Channel filter kernel size must fit in input.");

    if(dY.shape(0) != I.shape(0) - kernel_rows + 1 || dY.shape(1) != I.shape(1) - kernel_cols + 1)
        throw std::invalid_argument("Output gradient shape does not match input and channel kernel size.");
#endif

    const int input_cols = I.shape(1);
    const int input_channels = I.shape(2);
    const int filters = dY.shape(2);
    const int output_rows = dY.shape(0);
    const int output_cols = dY.shape(1);

    if(is_uninitialized())
        realloc(filters, kernel_rows, kernel_cols, input_channels);

    if(rank() != 4 || shape(0) != filters || shape(1) != kernel_rows || shape(2) != kernel_cols || shape(3) != input_channels)
        throw std::invalid_argument("Channel filter-gradient result matrix has wrong shape.");

    if(this == &I || this == &dY)
        throw std::invalid_argument("Result cannot be assigned to I or dY.");

    reset();

    const float * input = I.data();
    const float * output_gradient = dY.data();
    float * filter_gradient = data();
    const int input_row_stride = input_cols * input_channels;
    const int kernel_plane = kernel_rows * kernel_cols * input_channels;

    for(int y = 0; y < output_rows; ++y)
    {
        for(int x = 0; x < output_cols; ++x)
        {
            const float * gradient_pixel = output_gradient + (y * output_cols + x) * filters;
            for(int f = 0; f < filters; ++f)
            {
                const float gradient = gradient_pixel[f];
                float * filter = filter_gradient + f * kernel_plane;
                for(int ky = 0; ky < kernel_rows; ++ky)
                {
                    const float * input_pixel = input + (y + ky) * input_row_stride + x * input_channels;
                    float * filter_pixel = filter + ky * kernel_cols * input_channels;
                    for(int kx = 0; kx < kernel_cols; ++kx)
                    {
                        for(int c = 0; c < input_channels; ++c)
                            filter_pixel[c] += input_pixel[c] * gradient;
                        input_pixel += input_channels;
                        filter_pixel += input_channels;
                    }
                }
            }
        }
    }

    return *this;
}


matrix &
matrix::conv2_valid_channel_filterbank_backward_input(const matrix & dY, const matrix & K, int input_rows, int input_cols)
{
#ifndef NO_MATRIX_CHECKS
    if(dY.rank() != 3 || K.rank() != 4)
        throw std::invalid_argument("Channel filter-bank input-gradient requires output gradient [OH,OW,O] and filters [O,KH,KW,C].");

    if(input_rows <= 0 || input_cols <= 0)
        throw std::invalid_argument("Channel input-gradient dimensions must be positive.");

    if(dY.shape(2) != K.shape(0))
        throw std::invalid_argument("Output-gradient filter count does not match channel filters.");

    if(dY.shape(0) != input_rows - K.shape(1) + 1 || dY.shape(1) != input_cols - K.shape(2) + 1)
        throw std::invalid_argument("Output gradient shape does not match requested channel input size and filters.");
#endif

    const int input_channels = K.shape(3);

    if(is_uninitialized())
        realloc(input_rows, input_cols, input_channels);

    if(rank() != 3 || shape(0) != input_rows || shape(1) != input_cols || shape(2) != input_channels)
        throw std::invalid_argument("Channel input-gradient result matrix has wrong shape.");

    if(this == &dY || this == &K)
        throw std::invalid_argument("Result cannot be assigned to dY or K.");

    reset();

    const int filters = K.shape(0);
    const int kernel_rows = K.shape(1);
    const int kernel_cols = K.shape(2);
    const int output_rows = dY.shape(0);
    const int output_cols = dY.shape(1);
    const int input_row_stride = input_cols * input_channels;
    const int kernel_plane = kernel_rows * kernel_cols * input_channels;
    const float * output_gradient = dY.data();
    const float * filters_data = K.data();
    float * input_gradient = data();

    for(int y = 0; y < output_rows; ++y)
    {
        for(int x = 0; x < output_cols; ++x)
        {
            const float * gradient_pixel = output_gradient + (y * output_cols + x) * filters;
            for(int f = 0; f < filters; ++f)
            {
                const float gradient = gradient_pixel[f];
                const float * filter = filters_data + f * kernel_plane;
                for(int ky = 0; ky < kernel_rows; ++ky)
                {
                    float * input_pixel = input_gradient + (y + ky) * input_row_stride + x * input_channels;
                    const float * filter_pixel = filter + ky * kernel_cols * input_channels;
                    for(int kx = 0; kx < kernel_cols; ++kx)
                    {
                        for(int c = 0; c < input_channels; ++c)
                            input_pixel[c] += gradient * filter_pixel[c];
                        input_pixel += input_channels;
                        filter_pixel += input_channels;
                    }
                }
            }
        }
    }

    return *this;
}


matrix &
matrix::conv2_valid_channel_filterbank_backward(const matrix & I, const matrix & K, const matrix & dY, matrix & dK, matrix & dB)
{
#ifndef NO_MATRIX_CHECKS
    if(I.rank() != 3 || K.rank() != 4 || dY.rank() != 3)
        throw std::invalid_argument("Channel filter-bank backward requires input [H,W,C], filters [O,KH,KW,C], and output gradient [OH,OW,O].");

    if(I.shape(2) != K.shape(3) || dY.shape(2) != K.shape(0))
        throw std::invalid_argument("Channel filter-bank backward channel counts do not match.");

    if(dY.shape(0) != I.shape(0) - K.shape(1) + 1 || dY.shape(1) != I.shape(1) - K.shape(2) + 1)
        throw std::invalid_argument("Channel filter-bank backward spatial shapes do not match.");
#endif

    const int input_rows = I.shape(0);
    const int input_cols = I.shape(1);
    const int input_channels = I.shape(2);
    const int filters = K.shape(0);
    const int kernel_rows = K.shape(1);
    const int kernel_cols = K.shape(2);
    const int output_rows = dY.shape(0);
    const int output_cols = dY.shape(1);

    if(is_uninitialized())
        realloc(input_rows, input_cols, input_channels);
    if(dK.is_uninitialized())
        dK.realloc(filters, kernel_rows, kernel_cols, input_channels);
    if(dB.is_uninitialized())
        dB.realloc(filters);

    if(rank() != 3 || shape(0) != input_rows || shape(1) != input_cols || shape(2) != input_channels)
        throw std::invalid_argument("Channel input-gradient result matrix has wrong shape.");
    if(dK.rank() != 4 || dK.shape(0) != filters || dK.shape(1) != kernel_rows || dK.shape(2) != kernel_cols || dK.shape(3) != input_channels)
        throw std::invalid_argument("Channel filter-gradient result matrix has wrong shape.");
    if(dB.rank() != 1 || dB.size() != filters)
        throw std::invalid_argument("Channel bias-gradient result matrix has wrong shape.");

    if(this == &I || this == &K || this == &dY || this == &dK || this == &dB || &dK == &dB)
        throw std::invalid_argument("Channel filter-bank backward results must not alias inputs or each other.");

    reset();
    dK.reset();
    dB.reset();

    const float * input = I.data();
    const float * filters_data = K.data();
    const float * output_gradient = dY.data();
    float * input_gradient = data();
    float * filter_gradient = dK.data();
    float * bias_gradient = dB.data();
    const int input_row_stride = input_cols * input_channels;
    const int kernel_plane = kernel_rows * kernel_cols * input_channels;

    for(int y = 0; y < output_rows; ++y)
    {
        for(int x = 0; x < output_cols; ++x)
        {
            const float * gradient_pixel = output_gradient + (y * output_cols + x) * filters;
            for(int f = 0; f < filters; ++f)
            {
                const float gradient = gradient_pixel[f];
                const float * filter = filters_data + f * kernel_plane;
                float * filter_grad = filter_gradient + f * kernel_plane;
                bias_gradient[f] += gradient;

                for(int ky = 0; ky < kernel_rows; ++ky)
                {
                    const float * input_pixel = input + (y + ky) * input_row_stride + x * input_channels;
                    const float * filter_pixel = filter + ky * kernel_cols * input_channels;
                    float * filter_grad_pixel = filter_grad + ky * kernel_cols * input_channels;
                    float * input_grad_pixel = input_gradient + (y + ky) * input_row_stride + x * input_channels;

                    for(int kx = 0; kx < kernel_cols; ++kx)
                    {
                        for(int c = 0; c < input_channels; ++c)
                        {
                            filter_grad_pixel[c] += input_pixel[c] * gradient;
                            input_grad_pixel[c] += gradient * filter_pixel[c];
                        }
                        input_pixel += input_channels;
                        filter_pixel += input_channels;
                        filter_grad_pixel += input_channels;
                        input_grad_pixel += input_channels;
                    }
                }
            }
        }
    }

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
matrix::multiply_and_accumulate(const matrix & A, float c)
{
    check_same_size(A);

#if defined(__APPLE__)
    if(info_->continuous && A.info_->continuous)
    {
        cblas_saxpy(size(), c, A.data(), 1, data(), 1);
        return *this;
    }
#endif

    return apply(A, [c](float x, float y)->float { return x + c * y; });
}


matrix &
matrix::relu_backward(const matrix & gradients, const matrix & pre_activation)
{
    gradients.check_same_size(pre_activation);

    if(is_uninitialized())
        realloc(gradients.shape());

    check_same_size(gradients);

    if(this == &pre_activation)
        throw std::invalid_argument("Result cannot be assigned to pre_activation.");

    const float * gradient_values = gradients.data();
    const float * pre = pre_activation.data();
    float * output = data();

    for(int i = 0; i < size(); ++i)
        output[i] = pre[i] > 0.0f ? gradient_values[i] : 0.0f;

    return *this;
}


matrix &
matrix::adam_update(const matrix & gradients, matrix & first_moment, matrix & second_moment, float learning_rate, float beta1, float beta2, float beta1_correction, float beta2_correction, float epsilon)
{
    check_same_size(gradients);
    check_same_size(first_moment);
    check_same_size(second_moment);

    float * values = data();
    const float * gradient_values = gradients.data();
    float * first = first_moment.data();
    float * second = second_moment.data();
    const float one_minus_beta1 = 1.0f - beta1;
    const float one_minus_beta2 = 1.0f - beta2;
    const float update_scale = learning_rate / beta1_correction;
    const float inv_beta2_correction = 1.0f / beta2_correction;

    for(int i = 0; i < size(); ++i)
    {
        const float gradient = gradient_values[i];
        first[i] = beta1 * first[i] + one_minus_beta1 * gradient;
        second[i] = beta2 * second[i] + one_minus_beta2 * gradient * gradient;

        values[i] -= update_scale * first[i] / (std::sqrt(second[i] * inv_beta2_correction) + epsilon);
    }

    return *this;
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
matrix::outer_product(const matrix & left, const matrix & right)
{
    if(is_uninitialized())
        realloc(left.size(), right.size());

#ifndef NO_MATRIX_CHECKS
    if(rank() != 2 || left.rank() != 1 || right.rank() != 1)
        throw std::invalid_argument("Outer product requires two vectors and a two-dimensional result.");
#endif

    if(rows() != left.size() || cols() != right.size())
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(left.size()) + "x" + std::to_string(right.size()) + ".");

    if(this == &left || this == &right)
        throw std::invalid_argument("Result cannot be assigned to left or right.");

    reset();

#if defined(__APPLE__)
    cblas_sger(CblasRowMajor, left.size(), right.size(), 1.0f, left.data(), 1, right.data(), 1, data(), info_->stride_[1]);
#else
    const float * left_values = left.data();
    const float * right_values = right.data();
    float * output = data();
    const int output_cols = cols();

    for(int row = 0; row < left.size(); ++row)
    {
        float * output_row = output + row * output_cols;
        for(int col = 0; col < right.size(); ++col)
            output_row[col] = left_values[row] * right_values[col];
    }
#endif

    return *this;
}


matrix &
matrix::dense_forward(const matrix & input, const matrix & weights)
{
    if(is_uninitialized())
        realloc(weights.cols());

#ifndef NO_MATRIX_CHECKS
    if(rank() != 1 || input.rank() != 1 || weights.rank() != 2)
        throw std::invalid_argument("Dense forward requires input [I], weights [I,O], and result [O].");

    if(input.size() != weights.rows())
        throw std::invalid_argument("Input vector and dense weights are not compatible.");
#endif

    if(size() != weights.cols())
        throw std::invalid_argument("Result vector does not have size " + std::to_string(weights.cols()) + ".");

    if(this == &input || this == &weights)
        throw std::invalid_argument("Result cannot be assigned to input or weights.");

#if defined(__APPLE__)
    cblas_sgemv(CblasRowMajor, CblasTrans,
                weights.rows(), weights.cols(), 1.0f,
                weights.data(), weights.info_->stride_[1],
                input.data(), 1, 0.0f, data(), 1);
#else
    reset();
    const float * input_values = input.data();
    const float * weight_values = weights.data();
    float * output = data();
    const int output_size = weights.cols();

    for(int row = 0; row < weights.rows(); ++row)
    {
        const float input_value = input_values[row];
        const float * weight_row = weight_values + row * output_size;
        for(int col = 0; col < output_size; ++col)
            output[col] += input_value * weight_row[col];
    }
#endif

    return *this;
}


matrix &
matrix::dense_backward_input(const matrix & weights, const matrix & output_gradient)
{
    if(is_uninitialized())
        realloc(weights.rows());

#ifndef NO_MATRIX_CHECKS
    if(rank() != 1 || weights.rank() != 2 || output_gradient.rank() != 1)
        throw std::invalid_argument("Dense backward input requires weights [I,O], output_gradient [O], and result [I].");

    if(output_gradient.size() != weights.cols())
        throw std::invalid_argument("Output gradient vector and dense weights are not compatible.");
#endif

    if(size() != weights.rows())
        throw std::invalid_argument("Result vector does not have size " + std::to_string(weights.rows()) + ".");

    if(this == &weights || this == &output_gradient)
        throw std::invalid_argument("Result cannot be assigned to weights or output_gradient.");

#if defined(__APPLE__)
    cblas_sgemv(CblasRowMajor, CblasNoTrans,
                weights.rows(), weights.cols(), 1.0f,
                weights.data(), weights.info_->stride_[1],
                output_gradient.data(), 1, 0.0f, data(), 1);
#else
    const float * weight_values = weights.data();
    const float * gradient_values = output_gradient.data();
    float * output = data();
    const int output_size = weights.cols();

    for(int row = 0; row < weights.rows(); ++row)
    {
        const float * weight_row = weight_values + row * output_size;
        float gradient = 0.0f;
        for(int col = 0; col < output_size; ++col)
            gradient += weight_row[col] * gradient_values[col];
        output[row] = gradient;
    }
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
