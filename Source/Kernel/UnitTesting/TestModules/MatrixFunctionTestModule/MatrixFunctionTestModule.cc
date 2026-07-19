#include "ikaros.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <limits>
#include <random>
#include <sstream>
#include <thread>
#include <type_traits>

using namespace ikaros;

namespace
{
    constexpr float kTolerance = 1e-4f;
    constexpr float kHalfPi = 1.57079632679489661923f;

    template <typename T, typename = void>
    struct has_public_data_member: std::false_type
    {};

    template <typename T>
    struct has_public_data_member<T, std::void_t<decltype(std::declval<T &>().data_)>>: std::true_type
    {};

    template <typename T, typename = void>
    struct has_public_info_member: std::false_type
    {};

    template <typename T>
    struct has_public_info_member<T, std::void_t<decltype(std::declval<T &>().info_)>>: std::true_type
    {};

    template <typename T, typename = void>
    struct has_public_last_member: std::false_type
    {};

    template <typename T>
    struct has_public_last_member<T, std::void_t<decltype(std::declval<T &>().last_)>>: std::true_type
    {};

    template <typename T, typename = void>
    struct has_public_row_pointers_member: std::false_type
    {};

    template <typename T>
    struct has_public_row_pointers_member<T, std::void_t<decltype(std::declval<T &>().row_pointers_)>>: std::true_type
    {};

    template <typename T, typename = void>
    struct accepts_resize_dimension: std::false_type
    {};

    template <typename T>
    struct accepts_resize_dimension<T, std::void_t<decltype(std::declval<matrix &>().resize(std::declval<T>()))>>:
        std::true_type
    {};

    template <typename T, typename = void>
    struct accepts_realloc_dimension: std::false_type
    {};

    template <typename T>
    struct accepts_realloc_dimension<T, std::void_t<decltype(std::declval<matrix &>().realloc(std::declval<T>()))>>:
        std::true_type
    {};

    template <typename T, typename = void>
    struct accepts_reserve_dimension: std::false_type
    {};

    template <typename T>
    struct accepts_reserve_dimension<T, std::void_t<decltype(std::declval<matrix &>().reserve(std::declval<T>()))>>:
        std::true_type
    {};

    template <typename T, typename = void>
    struct accepts_reshape_dimension: std::false_type
    {};

    template <typename T>
    struct accepts_reshape_dimension<T, std::void_t<decltype(std::declval<matrix &>().reshape(std::declval<T>()))>>:
        std::true_type
    {};

    matrix make_matrix(const std::string & data)
    {
        return matrix(data);
    }

    void require_true(bool condition, const std::string & message)
    {
        if(!condition)
            throw exception("MatrixFunctionTestModule: " + message);
    }

    void require_equal(const std::string & actual, const std::string & expected, const std::string & message)
    {
        if(actual != expected)
            throw exception(
                "MatrixFunctionTestModule: " + message +
                " (expected \"" + expected +
                "\", got \"" + actual + "\")"
            );
    }

    void require_close(float actual, float expected, const std::string & message, float tolerance = kTolerance)
    {
        if(std::fabs(actual - expected) > tolerance)
            throw exception(
                "MatrixFunctionTestModule: " + message +
                " (expected " + std::to_string(expected) +
                ", got " + std::to_string(actual) + ")"
            );
    }

    std::string shape_string(const std::vector<int> & shape)
    {
        std::string result = "[";
        for(std::size_t i = 0; i < shape.size(); ++i)
        {
            if(i > 0)
                result += ", ";
            result += std::to_string(shape[i]);
        }
        result += "]";
        return result;
    }

    template <typename MatrixLike>
    void require_shape(const MatrixLike & actual, const std::vector<int> & expected, const std::string & message)
    {
        if(actual.shape() != expected)
            throw exception(
                "MatrixFunctionTestModule: " + message +
                " (expected shape " + shape_string(expected) +
                ", got " + shape_string(actual.shape()) + ")"
            );
    }

    void require_matrix_close(matrix actual, matrix expected, const std::string & message, float tolerance = kTolerance)
    {
        if(actual.shape() != expected.shape())
            throw exception("MatrixFunctionTestModule: " + message + " (shape mismatch)");

        if(actual.rank() == 0)
        {
            require_true(expected.rank() == 0, message + " (rank mismatch)");
            if(actual.size() == 0)
                return;
            require_close(actual.data()[0], expected.data()[0], message, tolerance);
            return;
        }

        for(auto ix = actual.get_range(); ix.more(); ++ix)
            require_close(actual.at(ix.index()), expected.at(ix.index()), message, tolerance);
    }

    void fill_sequence(matrix & values)
    {
        float value = 1.0f;
        for(auto ix = values.get_range(); ix.more(); ++ix)
            values.at(ix.index()) = value++;
    }

    template <typename Fn>
    double measure_ms(Fn && fn)
    {
        auto start = std::chrono::steady_clock::now();
        fn();
        auto end = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }

    template <typename Fn>
    void require_throws(Fn && fn, const std::string & message)
    {
        try
        {
            fn();
        }
        catch(const std::exception &)
        {
            return;
        }
        throw exception("MatrixFunctionTestModule: " + message + " (expected exception)");
    }

    template <typename Expected, typename Fn>
    void require_throws_as(Fn && fn, const std::string & message)
    {
        try
        {
            fn();
        }
        catch(const Expected &)
        {
            return;
        }
        catch(const std::exception & e)
        {
            throw exception(
                "MatrixFunctionTestModule: " + message +
                " (unexpected exception: " + e.what() + ")"
            );
        }
        throw exception("MatrixFunctionTestModule: " + message + " (expected exception)");
    }
}

class MatrixFunctionTestModule : public Module
{
    parameter suite_;

    void test_reductions()
    {
        matrix values = make_matrix("1, 2, 3, 4");
        require_close(values.sum(), 10.0f, "sum()");
        require_close(values.product(), 24.0f, "product()");
        require_close(values.min(), 1.0f, "min()");
        require_close(values.max(), 4.0f, "max()");
        require_close(values.average(), 2.5f, "average()");
        require_close(values.median(), 2.5f, "median()");

        const matrix & const_values = values;
        require_close(const_values.sum(), 10.0f, "const sum()");
        require_close(const_values.product(), 24.0f, "const product()");
        require_close(const_values.min(), 1.0f, "const min()");
        require_close(const_values.max(), 4.0f, "const max()");
        require_close(const_values.average(), 2.5f, "const average()");
        require_close(const_values.median(), 2.5f, "const median()");

        matrix a = make_matrix("1, 2, 3");
        matrix b = make_matrix("4, 5, 6");
        require_close(dot(a, b), 32.0f, "dot()");
    }

    void test_scalar_and_shape()
    {
        static_assert(std::is_constructible_v<matrix, int>);
        static_assert(std::is_constructible_v<matrix, long long>);
        static_assert(!std::is_constructible_v<matrix, float>);
        static_assert(!std::is_constructible_v<matrix, bool>);
        static_assert(!std::is_constructible_v<matrix, parameter>);
        static_assert(!std::is_convertible_v<int, matrix>);
        static_assert(!std::is_convertible_v<matrix &, float>);
        static_assert(!std::is_convertible_v<const matrix &, float>);
        static_assert(!std::is_convertible_v<matrix &, float *>);
        static_assert(!std::is_convertible_v<matrix &, float **>);
        static_assert(!std::is_constructible_v<float **, matrix &>);
        static_assert(!std::is_convertible_v<const matrix &, range>);
        static_assert(std::is_same_v<decltype(std::declval<matrix &>().scalar()), float &>);
        static_assert(std::is_same_v<decltype(std::declval<const matrix &>().scalar()), const float &>);
        static_assert(std::is_same_v<decltype(std::declval<matrix &>().row_data()), float **>);
        static_assert(std::is_same_v<decltype(std::declval<matrix &>() = 1.0f), matrix &>);
        static_assert(accepts_resize_dimension<int>::value);
        static_assert(accepts_realloc_dimension<long long>::value);
        static_assert(accepts_reserve_dimension<unsigned int>::value);
        static_assert(accepts_reshape_dimension<short>::value);
        static_assert(!accepts_resize_dimension<float>::value);
        static_assert(!accepts_realloc_dimension<bool>::value);
        static_assert(!accepts_reserve_dimension<parameter>::value);
        static_assert(!accepts_reshape_dimension<double>::value);

        matrix uninitialized;
        require_true(uninitialized.size() == 0, "size() is zero for uninitialized matrix");
        require_true(uninitialized.empty(), "uninitialized matrix is logically empty");
        require_true(uninitialized.unfilled(), "uninitialized matrix is unfilled");
        require_true(uninitialized.is_uninitialized(), "default matrix is uninitialized");
        require_true(uninitialized.data() == nullptr, "uninitialized matrix has no physical data pointer");
        require_true(uninitialized.is_contiguous(), "empty matrix has a vacuously contiguous layout");
        require_true(uninitialized.logical_block_count() == 0 && uninitialized.logical_block_size() == 0,
                     "uninitialized matrix has no logical blocks");
        require_throws([&]() { uninitialized.logical_block_data(0); },
                       "logical block access rejects an empty matrix");
        require_throws_as<std::invalid_argument>(
            [&]() { matrix invalid_shape(-1, 2); },
            "constructor should preserve invalid-dimension errors"
        );
        require_throws_as<std::out_of_range>(
            [&]() { matrix oversized(std::numeric_limits<int>::max(), 2); },
            "constructor should preserve shape-overflow errors"
        );
        const long long dimension_past_int = static_cast<long long>(std::numeric_limits<int>::max()) + 1;
        const unsigned long long unsigned_dimension_past_int =
            static_cast<unsigned long long>(std::numeric_limits<int>::max()) + 1;
        require_throws_as<std::out_of_range>(
            [&]() { matrix oversized_dimension(dimension_past_int); },
            "constructor rejects signed dimensions outside the supported range"
        );
        require_throws_as<std::out_of_range>(
            [&]() { matrix oversized_dimension(unsigned_dimension_past_int); },
            "constructor rejects unsigned dimensions outside the supported range"
        );
        matrix checked_dimension(static_cast<long long>(2));
        require_shape(checked_dimension, {2}, "checked integral dimensions preserve valid values");
        checked_dimension(0) = 3.0f;
        checked_dimension(1) = 4.0f;
        require_throws_as<std::out_of_range>(
            [&]() { checked_dimension.resize(dimension_past_int); },
            "resize rejects dimensions outside the supported range"
        );
        require_throws_as<std::out_of_range>(
            [&]() { checked_dimension.realloc(unsigned_dimension_past_int); },
            "realloc rejects dimensions outside the supported range"
        );
        require_throws_as<std::out_of_range>(
            [&]() { checked_dimension.reserve(dimension_past_int); },
            "reserve rejects dimensions outside the supported range"
        );
        require_throws_as<std::out_of_range>(
            [&]() { checked_dimension.reshape(unsigned_dimension_past_int); },
            "reshape rejects dimensions outside the supported range"
        );
        require_shape(checked_dimension, {2}, "rejected dimensions preserve the existing shape");
        require_matrix_close(checked_dimension, make_matrix("3, 4"),
                             "rejected dimensions preserve existing values");
        require_throws([&]() { uninitialized[0]; }, "operator[] should reject uninitialized rank-zero matrices");
        const matrix & const_uninitialized = uninitialized;
        require_throws([&]() { const_uninitialized[0]; }, "const operator[] should reject rank-zero matrices");
        matrix scalar_source(1);
        matrix scalar = scalar_source[0];
        require_true(scalar.size() == 1, "size() is one for scalar matrix");
        require_true(!scalar.empty(), "scalar matrix is not empty");
        require_true(!scalar.unfilled(), "scalar matrix is filled");
        require_true(!scalar.is_uninitialized(), "scalar matrix is initialized");
        require_true(scalar.is_contiguous(), "scalar matrix is contiguous");
        require_true(scalar.logical_block_count() == 1 && scalar.logical_block_size() == 1,
                     "scalar matrix has one one-element logical block");
        require_true(scalar.logical_block_data(0) == scalar.data(),
                     "scalar logical block starts at its physical data pointer");
        require_throws([&]() { scalar[0]; }, "operator[] should reject scalar rank-zero matrices");

        matrix rank1_access = make_matrix("1, 2");
        matrix rank2_access = make_matrix("1, 2; 3, 4");
        matrix rank3_access(std::vector<int>{2, 2, 2});
        matrix rank4_access(std::vector<int>{2, 2, 2, 2});
        matrix rank5_access(std::vector<int>{2, 2, 2, 2, 2});
        fill_sequence(rank3_access);
        fill_sequence(rank4_access);
        fill_sequence(rank5_access);
        const matrix & const_rank1_access = rank1_access;
        const matrix & const_rank2_access = rank2_access;
        const matrix & const_rank3_access = rank3_access;
        const matrix & const_rank4_access = rank4_access;
        const matrix & const_rank5_access = rank5_access;

        static_assert(!std::is_convertible_v<matrix &, float *>);
        static_assert(std::is_same_v<decltype(const_rank1_access(0)), const float &>);
        static_assert(std::is_same_v<decltype(const_rank2_access(0, 0)), const float &>);
        static_assert(std::is_same_v<decltype(const_rank3_access(0, 0, 0)), const float &>);
        static_assert(std::is_same_v<decltype(const_rank4_access(0, 0, 0, 0)), const float &>);
        static_assert(std::is_same_v<decltype(const_rank5_access(0, 0, 0, 0, 0)), const float &>);
        require_close(const_rank1_access(1), 2.0f, "const rank-1 direct scalar access");
        require_close(const_rank2_access(1, 1), 4.0f, "const rank-2 direct scalar access");
        require_close(const_rank3_access(1, 1, 1), 8.0f, "const rank-3 direct scalar access");
        require_close(const_rank4_access(1, 1, 1, 1), 16.0f, "const rank-4 direct scalar access");
        require_close(const_rank5_access(1, 1, 1, 1, 1), 32.0f,
                      "const higher-rank allocation-free scalar access");
#if IKAROS_MATRIX_CHECKS
        require_throws_as<std::invalid_argument>(
            [&]() { (void)rank2_access(0); },
            "mutable direct scalar access rejects a rank mismatch"
        );
        require_throws_as<std::out_of_range>(
            [&]() { (void)rank1_access(2); },
            "mutable direct scalar access rejects an out-of-range index"
        );
        require_throws_as<std::invalid_argument>(
            [&]() { (void)rank5_access(0, 0, 0, 0); },
            "mutable higher-rank scalar access rejects a rank mismatch"
        );
        require_throws_as<std::out_of_range>(
            [&]() { (void)rank5_access(0, 0, 0, 0, 2); },
            "mutable higher-rank scalar access rejects an out-of-range index"
        );
        require_throws_as<std::invalid_argument>(
            [&]() { (void)const_rank2_access(0); },
            "const direct scalar access rejects a rank mismatch"
        );
        require_throws_as<std::out_of_range>(
            [&]() { (void)const_rank1_access(2); },
            "const direct scalar access rejects an out-of-range index"
        );
        require_throws_as<std::out_of_range>(
            [&]() { (void)const_rank5_access(0, 0, 0, 0, 2); },
            "const higher-rank scalar access rejects an out-of-range index"
        );
        require_throws_as<std::invalid_argument>(
            [&]() { (void)rank2_access.at({0}); },
            "vector access rejects a rank mismatch"
        );
        require_throws_as<std::out_of_range>(
            [&]() { (void)rank2_access.at({0, 2}); },
            "vector access rejects an out-of-range index"
        );
#endif

        require_true(rank2_access.shape(0) == 2 && rank2_access.shape(-1) == 2,
                     "shape dimension queries accept positive and negative indices");
        require_throws_as<std::out_of_range>(
            [&]() { (void)rank2_access.shape(2); },
            "shape rejects a positive dimension past the rank"
        );
        require_throws_as<std::out_of_range>(
            [&]() { (void)rank2_access.shape(-3); },
            "shape rejects a negative dimension past the rank"
        );
        require_throws_as<std::out_of_range>(
            [&]() { (void)uninitialized.shape(0); },
            "shape rejects every dimension for a rank-zero matrix"
        );
        require_true(rank2_access.shape_or_zero(2) == 0 &&
                     rank2_access.shape_or_zero(-3) == 0 &&
                     uninitialized.shape_or_zero(0) == 0,
                     "shape_or_zero preserves explicit compatibility queries");

        matrix row_gapped_rank2_access(2, 4);
        fill_sequence(row_gapped_rank2_access);
        row_gapped_rank2_access.resize(2, 2);
        const matrix & const_row_gapped_rank2_access = row_gapped_rank2_access;
        require_close(const_row_gapped_rank2_access(1, 1), 6.0f,
                      "const rank-2 scalar access honors row gaps");
        require_true(!row_gapped_rank2_access.is_contiguous(),
                     "resized rank-2 matrix reports its row gaps");
        require_true(row_gapped_rank2_access.data() != nullptr,
                     "row-gapped matrix data() identifies its first physical element");
        require_close(row_gapped_rank2_access.data()[2], 3.0f,
                      "row-gapped data() exposes physical rather than logical flat order");
        require_throws([&]() { row_gapped_rank2_access.contiguous_data(); },
                       "contiguous_data() rejects a row-gapped matrix");
        require_true(row_gapped_rank2_access.logical_block_count() == 2 &&
                     row_gapped_rank2_access.logical_block_size() == 2,
                     "row-gapped rank-2 matrix exposes two contiguous logical blocks");
        require_close(row_gapped_rank2_access.logical_block_data(0)[1], 2.0f,
                      "first row-gapped logical block is addressable");
        require_close(row_gapped_rank2_access.logical_block_data(1)[0], 5.0f,
                      "second row-gapped logical block skips the physical gap");
        require_throws([&]() { row_gapped_rank2_access.logical_block_data(-1); },
                       "logical block access rejects a negative block");
        require_throws([&]() { row_gapped_rank2_access.logical_block_data(2); },
                       "logical block access rejects a block past the end");
        float * explicit_pointer = row_gapped_rank2_access.data();
        require_true(explicit_pointer == row_gapped_rank2_access.data(),
                     "explicit legacy pointer conversion returns physical data()");

        matrix row_gapped_rank5_access(std::vector<int>{2, 2, 2, 2, 4});
        fill_sequence(row_gapped_rank5_access);
        row_gapped_rank5_access.resize(std::vector<int>{2, 2, 2, 2, 2});
        const matrix & const_row_gapped_rank5_access = row_gapped_rank5_access;
        require_close(const_row_gapped_rank5_access(1, 1, 1, 1, 1), 62.0f,
                      "const higher-rank scalar access honors row gaps");
        require_true(row_gapped_rank5_access.logical_block_count() == 16 &&
                     row_gapped_rank5_access.logical_block_size() == 2,
                     "higher-rank row-gapped matrix exposes innermost logical blocks");
        require_close(const_row_gapped_rank5_access.logical_block_data(15)[1], 62.0f,
                      "const higher-rank logical block access honors all outer strides");
        auto const_row_gapped_view = const_row_gapped_rank5_access[0];
        static_assert(std::is_same_v<decltype(const_row_gapped_view.logical_block_data(0)), const float *>);
        require_true(!const_row_gapped_view.is_contiguous(),
                     "const matrix view reports a row-gapped layout");
        require_throws([&]() { const_row_gapped_view.contiguous_data(); },
                       "const view contiguous_data() rejects row gaps");

        matrix reshaped_scalar = make_matrix("7");
        require_close(reshaped_scalar.scalar(), 7.0f, "single-element vector scalar access");
        matrix & scalar_assignment_result = (reshaped_scalar = 8.0f);
        require_true(&scalar_assignment_result == &reshaped_scalar,
                     "scalar assignment returns its destination");
        require_close(reshaped_scalar(0), 8.0f, "single-element vector accepts scalar assignment");
        matrix nonscalar(2);
        require_throws(
            [&]() { (void)nonscalar.scalar(); },
            "scalar() rejects matrices with more than one element"
        );
        reshaped_scalar.reshape(std::vector<int>{});
        require_true(reshaped_scalar.is_scalar(), "reshape to rank zero creates a scalar");
        require_close(reshaped_scalar.scalar(), 8.0f, "reshaped scalar retains its value");

        matrix zero_vector(0);
        require_shape(zero_vector, {0}, "zero-length vector retains its rank");
        require_true(zero_vector.size() == 0, "zero-length vector has no logical elements");
        require_true(zero_vector.empty(), "zero-length vector is logically empty");
        require_true(zero_vector.unfilled(), "zero-length vector is unfilled");
        require_true(!zero_vector.is_uninitialized(), "zero-length vector is initialized");
        require_true(zero_vector.data() == nullptr && zero_vector.contiguous_data() == nullptr,
                     "initialized zero-length vector has no physical data pointer");

        matrix zero_width(2, 0);
        require_shape(zero_width, {2, 0}, "zero-width matrix retains its shape");
        require_true(zero_width.size() == 0, "zero-width matrix has no logical elements");
        require_true(zero_width.empty(), "zero-width matrix is logically empty");
        require_true(!zero_width.is_uninitialized(), "zero-width matrix is initialized");
        matrix zero_width_row = zero_width[0];
        require_shape(zero_width_row, {0}, "zero-width slice retains a zero dimension");
        require_true(zero_width_row.size() == 0, "zero-width slice has no logical elements");
        require_true(zero_width_row.empty(), "zero-width slice is logically empty");
        require_true(!zero_width_row.is_scalar(), "zero-width slice is not a scalar");
        require_true(!zero_width_row.is_uninitialized(), "zero-width slice is initialized");
        const matrix & const_zero_width = zero_width;
        auto const_zero_width_row = const_zero_width[1];
        require_true(const_zero_width_row.empty(), "const zero-width slice is logically empty");
        require_true(!const_zero_width_row.is_scalar(), "const zero-width slice is not a scalar");
        require_equal(zero_width.json(), "[[], []]", "zero-width matrix JSON preserves its shape");

        static_assert(std::is_same_v<
            decltype(std::declval<matrix &>() = std::declval<const std::string &>()),
            matrix &
        >);

        matrix shared_string_target = make_matrix("1, 2");
        matrix shared_string_alias = shared_string_target;
        const std::string legacy_values = "3, 4";
        matrix & assignment_result = (shared_string_target = legacy_values);
        require_true(&assignment_result == &shared_string_target,
                     "string assignment returns its destination");
        require_matrix_close(shared_string_alias, make_matrix("3, 4"),
                             "legacy string assignment preserves shallow sharing");
        shared_string_target = std::string("[5, 6]");
        require_matrix_close(shared_string_alias, make_matrix("5, 6"),
                             "bracket string assignment preserves shallow sharing");

        matrix fixed_string_target = make_matrix("1, 2");
        matrix fixed_string_alias = fixed_string_target;
        require_throws_as<std::invalid_argument>(
            [&]() { fixed_string_target = std::string("3, 4; 5, 6"); },
            "legacy assignment rejects a fixed-shape change"
        );
        require_matrix_close(fixed_string_alias, make_matrix("1, 2"),
                             "rejected legacy shape change preserves aliases");
        require_throws_as<std::invalid_argument>(
            [&]() { fixed_string_target = std::string("[[3, 4], [5, 6]]"); },
            "bracket assignment rejects a fixed-shape change"
        );
        require_matrix_close(fixed_string_alias, make_matrix("1, 2"),
                             "rejected bracket shape change preserves aliases");

        matrix dynamic_string_target;
        dynamic_string_target.reserve(2, 2).set_dynamic().set_fixed_capacity();
        matrix dynamic_string_alias = dynamic_string_target;
        dynamic_string_target = std::string("[[1, 2], [3, 4]]");
        require_matrix_close(dynamic_string_alias, make_matrix("1, 2; 3, 4"),
                             "dynamic bracket assignment grows within reserved capacity");
        dynamic_string_target = std::string("5, 6;");
        require_shape(dynamic_string_alias, {1, 2},
                      "dynamic legacy assignment updates shared shape metadata");
        require_matrix_close(dynamic_string_alias, make_matrix("5, 6;"),
                             "dynamic legacy assignment shrinks within reserved capacity");

        matrix string_parent = make_matrix("1, 2; 3, 4");
        matrix string_view = string_parent[1];
        string_view = std::string("7, 8");
        require_matrix_close(string_parent, make_matrix("1, 2; 7, 8"),
                             "same-shape legacy assignment updates a view");
        string_view = std::string("[9, 10]");
        require_matrix_close(string_parent, make_matrix("1, 2; 9, 10"),
                             "same-shape bracket assignment updates a view");
        require_throws_as<std::invalid_argument>(
            [&]() { string_view = std::string("11, 12, 13"); },
            "legacy assignment rejects a view shape change"
        );
        require_throws_as<std::invalid_argument>(
            [&]() { string_view = std::string("[11, 12, 13]"); },
            "bracket assignment rejects a view shape change"
        );
        require_matrix_close(string_parent, make_matrix("1, 2; 9, 10"),
                             "rejected view shape changes preserve the parent");

        matrix atomic_string_target = make_matrix("1, 2");
        matrix atomic_string_alias = atomic_string_target;
        require_throws(
            [&]() { atomic_string_target = std::string("7, invalid"); },
            "legacy assignment validates all tokens before writing"
        );
        require_matrix_close(atomic_string_alias, make_matrix("1, 2"),
                             "invalid legacy assignment is atomic");
        require_throws(
            [&]() { atomic_string_target = std::string("[7, \"invalid\"]"); },
            "bracket assignment validates all tokens before writing"
        );
        require_matrix_close(atomic_string_alias, make_matrix("1, 2"),
                             "invalid bracket assignment is atomic");
        require_throws(
            [&]() { atomic_string_target = std::string("[[7, 8], [9]]"); },
            "bracket assignment validates shape before writing"
        );
        require_matrix_close(atomic_string_alias, make_matrix("1, 2"),
                             "invalid bracket shape is atomic");

        matrix scalar_values = make_matrix("10, 42");
        matrix scalar_copy;
        scalar_copy.copy(scalar_values[1]);
        require_true(scalar_copy.is_scalar(), "copy(scalar) produces a scalar matrix");
        require_close(scalar_copy.scalar(), 42.0f, "copy(scalar) honors source offset");
        scalar_values(1) = 99.0f;
        require_close(scalar_copy.scalar(), 42.0f, "copy(scalar) performs a deep copy");

        matrix scalar_apply_target_values = make_matrix("10, 20");
        matrix scalar_apply_source_values = make_matrix("1, 2");
        matrix scalar_apply_target = scalar_apply_target_values[1];
        matrix scalar_apply_source = scalar_apply_source_values[0];
        scalar_apply_target.apply(scalar_apply_source, [](float target, float source) { return target + source; });
        require_close(scalar_apply_target_values(1), 21.0f, "apply() uses the source scalar offset");

        matrix scalar_apply_second_source_values = make_matrix("4, 5, 6");
        matrix scalar_apply_second_source = scalar_apply_second_source_values[2];
        scalar_apply_target.apply(
            scalar_apply_source,
            scalar_apply_second_source,
            [](float first, float second) { return first + second; }
        );
        require_close(scalar_apply_target_values(1), 7.0f, "ternary apply() uses both source scalar offsets");

        matrix type_erased_target = make_matrix("1, 2, 3");
        std::function<float(float)> type_erased_apply = [](float value) { return value * 2.0f; };
        type_erased_target.apply(type_erased_apply);
        require_matrix_close(type_erased_target, make_matrix("2, 4, 6"),
                             "apply() retains std::function compatibility");
        float type_erased_sum = 0.0f;
        std::function<void(float)> type_erased_reduce = [&](float value) { type_erased_sum += value; };
        type_erased_target.reduce(type_erased_reduce);
        require_close(type_erased_sum, 12.0f, "reduce() retains std::function compatibility");

        matrix mismatched_apply_target = make_matrix("1, 2, 3");
        matrix mismatched_apply_original = mismatched_apply_target.clone();
        matrix mismatched_apply_source = make_matrix("4, 5");
        require_throws_as<std::invalid_argument>(
            [&]()
            {
                mismatched_apply_target.apply(
                    mismatched_apply_source,
                    [](float target, float source) { return target + source; }
                );
            },
            "binary apply() rejects mismatched shapes"
        );
        require_matrix_close(mismatched_apply_target, mismatched_apply_original,
                             "binary apply() rejects mismatched shapes before writing");
        require_throws_as<std::invalid_argument>(
            [&]()
            {
                mismatched_apply_target.apply(
                    mismatched_apply_target,
                    mismatched_apply_source,
                    [](float first, float second) { return first + second; }
                );
            },
            "ternary apply() rejects mismatched shapes"
        );
        require_matrix_close(mismatched_apply_target, mismatched_apply_original,
                             "ternary apply() rejects mismatched shapes before writing");
        matrix uninitialized_apply_target;
        require_throws_as<std::invalid_argument>(
            [&]()
            {
                uninitialized_apply_target.apply(
                    mismatched_apply_source,
                    [](float target, float source) { return target + source; }
                );
            },
            "apply() validates source shape for an empty destination"
        );

        float legacy_vector_data[] = {1.0f, 2.0f, 3.0f};
        matrix legacy_vector(3, legacy_vector_data);
        require_matrix_close(legacy_vector, make_matrix("1, 2, 3"), "legacy vector pointer constructor");
        legacy_vector_data[0] = 9.0f;
        require_close(legacy_vector(0), 1.0f, "legacy vector pointer constructor copies data");
        require_throws(
            [&]() { matrix invalid(3, static_cast<float *>(nullptr)); },
            "legacy vector pointer constructor should reject null data"
        );

        float legacy_row_0[] = {1.0f, 2.0f, 3.0f};
        float legacy_row_1[] = {4.0f, 5.0f, 6.0f};
        float * legacy_matrix_data[] = {legacy_row_0, legacy_row_1};
        matrix legacy_matrix(2, 3, legacy_matrix_data);
        require_matrix_close(legacy_matrix, make_matrix("1, 2, 3; 4, 5, 6"), "legacy row pointer constructor");
        legacy_row_1[0] = 9.0f;
        require_close(legacy_matrix(1, 0), 4.0f, "legacy row pointer constructor copies data");
        require_throws(
            [&]() { matrix invalid(2, 3, static_cast<float **>(nullptr)); },
            "legacy row pointer constructor should reject null rows"
        );
        float * null_row_data[] = {legacy_row_0, nullptr};
        require_throws(
            [&]() { matrix invalid(2, 3, null_row_data); },
            "legacy row pointer constructor should reject null row pointers"
        );

        float ** legacy_matrix_rows = legacy_matrix.row_data();
        require_close(legacy_matrix_rows[1][2], 6.0f, "explicit row_data() compatibility helper");
        legacy_matrix.reserve(3, 3);
        legacy_matrix.resize(3, 3);
        legacy_matrix.set(0.0f);
        legacy_matrix(2, 2) = 7.0f;
        legacy_matrix_rows = legacy_matrix.row_data();
        require_close(legacy_matrix_rows[2][2], 7.0f, "row_data() refreshes row pointers");

        matrix values = make_matrix("1, 2; 3, 4");
        require_true(values.size() == 4, "size() is product of shape");
        require_throws([&]() { values(0, 2) = 0.0f; }, "rank-2 access should reject a column at the logical bound");

        std::ostringstream matrix_stream;
        matrix_stream << values;
        require_equal(matrix_stream.str(), "{{1, 2}, {3, 4}}", "operator<< writes to its stream");

        matrix rank1(2);
        require_throws([&]() { rank1(2) = 0.0f; }, "rank-1 access should reject an index at the logical bound");

        matrix rank3(2, 2, 2);
        require_throws([&]() { rank3(0, 0, 2) = 0.0f; }, "rank-3 access should reject an index at the logical bound");
        require_throws([&]() { rank3(0, 0) = 0.0f; }, "mutable access should reject the wrong number of indices");
        fill_sequence(rank3);
        require_equal(rank3.json(), "[[[1, 2], [3, 4]], [[5, 6], [7, 8]]]",
                      "rank-3 json() traverses values without slices");
        std::ostringstream rank3_stream;
        rank3_stream << rank3;
        require_equal(rank3_stream.str(), "{{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}}}",
                      "rank-3 stream output traverses values without slices");

        matrix rank4(2, 2, 2, 2);
        require_throws([&]() { rank4(0, 0, 0, 2) = 0.0f; }, "rank-4 access should reject an index at the logical bound");
        fill_sequence(rank4);
        require_equal(
            rank4.json(),
            "[[[[1, 2], [3, 4]], [[5, 6], [7, 8]]], [[[9, 10], [11, 12]], [[13, 14], [15, 16]]]]",
            "rank-4 json() traverses values without slices"
        );

        matrix strided(2, 4);
        strided.resize(2, 2);
        require_throws([&]() { strided(0, 2) = 0.0f; }, "strided access should reject an index outside the logical shape");

        values.add(1.0f);
        require_matrix_close(values, make_matrix("2, 3; 4, 5"), "add(float)");
        values.subtract(1.0f);
        require_matrix_close(values, make_matrix("1, 2; 3, 4"), "subtract(float)");
        values.scale(2.0f);
        require_matrix_close(values, make_matrix("2, 4; 6, 8"), "scale(float)");
        values.divide(2.0f);
        require_matrix_close(values, make_matrix("1, 2; 3, 4"), "divide(float)");
        values.set(7.0f);
        require_matrix_close(values, make_matrix("7, 7; 7, 7"), "set(float)");
        values.reset();
        require_matrix_close(values, make_matrix("0, 0; 0, 0"), "reset()");

        matrix reshaped = make_matrix("1, 2, 3, 4, 5, 6");
        reshaped.reshape(2, 3);
        require_shape(reshaped, {2, 3}, "reshape()");
        require_close(reshaped(1, 2), 6.0f, "reshape() preserves data order");

        matrix initializer_list_matrix{1.0f, 2.0f};
        require_true(!initializer_list_matrix.is_dynamic(), "initializer-list matrix is not dynamic");
        initializer_list_matrix.append(3.0f);
        require_matrix_close(initializer_list_matrix, make_matrix("1, 2, 3"), "initializer-list matrix can grow");

        matrix resize_target(2, 2);
        require_throws([&]() { resize_target.resize(-1, 2); }, "resize() should reject negative dimensions");
        require_shape(resize_target, {2, 2}, "resize() keeps shape after invalid dimensions");
        require_throws([&]() { resize_target.resize(3, 2); }, "resize() should reject dimensions beyond capacity");
        require_throws([&]() { resize_target.resize(4); }, "resize() should reject a changed rank");

        matrix reshape_target = make_matrix("1, 2, 3, 4");
        require_throws([&]() { reshape_target.reshape(-1, 4); }, "reshape() should reject negative dimensions");
        require_throws(
            [&]() { reshape_target.reshape(std::numeric_limits<int>::max(), 2); },
            "reshape() should reject overflowing dimensions"
        );
        require_shape(reshape_target, {4}, "reshape() keeps shape after invalid dimensions");

        matrix realloc_target;
        require_throws(
            [&]() { realloc_target.realloc(std::numeric_limits<int>::max(), 2); },
            "realloc() should reject overflowing dimensions"
        );
        require_throws([&]() { realloc_target.reserve(-1); }, "reserve() should reject negative capacity");

        matrix reshape_parent = make_matrix("1, 2; 3, 4");
        matrix reshape_view = reshape_parent[1];
        require_throws([&]() { reshape_view.reshape(4); }, "reshape() should reject matrix views");
        require_throws([&]() { reshape_view.realloc(4); }, "realloc() should reject matrix views");
        require_throws([&]() { reshape_view.reserve(4); }, "reserve() should reject matrix views");
        require_matrix_close(reshape_parent, make_matrix("1, 2; 3, 4"), "shape mutators preserve parent views");

        matrix row_gapped_reshape(2, 4);
        fill_sequence(row_gapped_reshape);
        row_gapped_reshape.resize(2, 2);
        require_throws([&]() { row_gapped_reshape.reshape(4); },
                       "reshape() should reject row-gapped logical storage");
        require_shape(row_gapped_reshape, {2, 2}, "rejected row-gapped reshape preserves shape");
        require_close(row_gapped_reshape(1, 0), 5.0f, "rejected row-gapped reshape preserves values");

        matrix reserved_reshape;
        reserved_reshape.reserve(4, 2);
        reserved_reshape.append(make_matrix("1, 2"));
        reserved_reshape.append(make_matrix("3, 4"));
        matrix reserved_reshape_alias = reserved_reshape;
        reserved_reshape.reshape(4);
        require_shape(reserved_reshape_alias, {4}, "reshape() atomically updates shared metadata");
        require_true(reserved_reshape_alias.capacity() == std::vector<int>({4}),
                     "reshape() discards incompatible excess capacity");
        require_matrix_close(reserved_reshape_alias, make_matrix("1, 2, 3, 4"),
                             "reshape() preserves contiguous reserved values");

#ifndef NDEBUG
        matrix::set_allocation_failure_countdown_for_testing(0);
        require_throws_as<out_of_memory_matrix_error>(
            [&]() { matrix failed_constructor(4); },
            "constructor should translate metadata allocation failure"
        );
        matrix::set_allocation_failure_countdown_for_testing(1);
        require_throws_as<out_of_memory_matrix_error>(
            [&]() { matrix failed_constructor(4); },
            "constructor should translate storage allocation failure"
        );

        matrix slice_allocation_parent = make_matrix("1, 2; 3, 4");
        matrix::set_allocation_failure_countdown_for_testing(0);
        require_throws_as<out_of_memory_matrix_error>(
            [&]() { matrix failed_slice = slice_allocation_parent[1]; },
            "slice creation should translate metadata allocation failure"
        );
        matrix::set_allocation_failure_countdown_for_testing(1);
        matrix single_allocation_slice = slice_allocation_parent[1];
        matrix::set_allocation_failure_countdown_for_testing(-1);
        require_matrix_close(single_allocation_slice, make_matrix("3, 4"),
                             "slice creation uses one tracked allocation");
        require_matrix_close(slice_allocation_parent, make_matrix("1, 2; 3, 4"),
                             "failed slice creation preserves its parent");

        for(int failure_point : {0, 1})
        {
            matrix failed_realloc = make_matrix("1, 2; 3, 4");
            matrix failed_realloc_alias = failed_realloc;
            float * original_storage = failed_realloc.data();
            matrix::set_allocation_failure_countdown_for_testing(failure_point);
            require_throws_as<out_of_memory_matrix_error>(
                [&]() { failed_realloc.realloc(4, 4); },
                "realloc() should translate injected allocation failure"
            );
            matrix::set_allocation_failure_countdown_for_testing(-1);
            require_true(failed_realloc.data() == original_storage,
                         "failed realloc() preserves the backing allocation");
            require_shape(failed_realloc_alias, {2, 2}, "failed realloc() preserves shared shape");
            require_matrix_close(failed_realloc_alias, make_matrix("1, 2; 3, 4"),
                                 "failed realloc() preserves shared values");
        }

        for(int failure_point : {0, 1})
        {
            matrix failed_reserve = make_matrix("1, 2");
            matrix failed_reserve_alias = failed_reserve;
            float * original_storage = failed_reserve.data();
            matrix::set_allocation_failure_countdown_for_testing(failure_point);
            require_throws_as<out_of_memory_matrix_error>(
                [&]() { failed_reserve.reserve(8); },
                "reserve() should translate injected allocation failure"
            );
            matrix::set_allocation_failure_countdown_for_testing(-1);
            require_true(failed_reserve.data() == original_storage,
                         "failed reserve() preserves the backing allocation");
            require_true(failed_reserve_alias.capacity() == std::vector<int>({2}),
                         "failed reserve() preserves shared capacity");
            require_matrix_close(failed_reserve_alias, make_matrix("1, 2"),
                                 "failed reserve() preserves shared values");
        }

        for(int failure_point : {0, 1})
        {
            matrix failed_reshape = make_matrix("1, 2; 3, 4");
            matrix failed_reshape_alias = failed_reshape;
            float * original_storage = failed_reshape.data();
            matrix::set_allocation_failure_countdown_for_testing(failure_point);
            require_throws_as<out_of_memory_matrix_error>(
                [&]() { failed_reshape.reshape(4); },
                "reshape() should translate injected allocation failure"
            );
            matrix::set_allocation_failure_countdown_for_testing(-1);
            require_true(failed_reshape.data() == original_storage,
                         "failed reshape() preserves the backing allocation");
            require_shape(failed_reshape_alias, {2, 2}, "failed reshape() preserves shared shape");
            require_matrix_close(failed_reshape_alias, make_matrix("1, 2; 3, 4"),
                                 "failed reshape() preserves shared values");
        }

        matrix failed_resize = make_matrix("1, 2; 3, 4");
        matrix failed_resize_alias = failed_resize;
        matrix::set_allocation_failure_countdown_for_testing(0);
        require_throws_as<out_of_memory_matrix_error>(
            [&]() { failed_resize.resize(1, 2); },
            "resize() should translate injected metadata failure"
        );
        matrix::set_allocation_failure_countdown_for_testing(-1);
        require_shape(failed_resize_alias, {2, 2}, "failed resize() preserves shared shape");
        require_matrix_close(failed_resize_alias, make_matrix("1, 2; 3, 4"),
                             "failed resize() preserves shared values");
#endif

        matrix original = make_matrix("1, 2; 3, 4");
        matrix copied;
        copied.copy(original);
        require_matrix_close(copied, original, "copy()");

        matrix strided_submatrix(2, 4);
        strided_submatrix.set(-1000.0f);
        strided_submatrix.resize(2, 2);
        strided_submatrix.submatrix(make_matrix("1, 2, 3; 4, 5, 6; 7, 8, 9"), {1, 1, 2, 2});
        require_matrix_close(strided_submatrix, make_matrix("5, 6; 8, 9"), "submatrix() row-gapped destination");

        matrix submatrix_source = make_matrix("1, 2, 3; 4, 5, 6; 7, 8, 9");
        matrix preserved_submatrix = make_matrix("9, 9; 9, 9");
        require_throws([&]() { preserved_submatrix.submatrix(submatrix_source, {2, 2, 2, 2}); },
                       "submatrix() should reject an out-of-bounds region");
        require_throws([&]() { preserved_submatrix.submatrix(submatrix_source, {-1, 0, 2, 2}); },
                       "submatrix() should reject negative coordinates");
        require_throws(
            [&]()
            {
                preserved_submatrix.submatrix(
                    submatrix_source,
                    {std::numeric_limits<int>::max(), 0, std::numeric_limits<int>::max(), 1}
                );
            },
            "submatrix() should reject overflowing region coordinates"
        );
        require_matrix_close(preserved_submatrix, make_matrix("9, 9; 9, 9"),
                             "invalid submatrix() regions do not partially write");
        matrix invalid_submatrix_source(3);
        require_throws([&]() { preserved_submatrix.submatrix(invalid_submatrix_source, {0, 0, 1, 1}); },
                       "submatrix() should reject a non-two-dimensional source");

        matrix empty_submatrix;
        empty_submatrix.submatrix(submatrix_source, {3, 1, 0, 2});
        require_shape(empty_submatrix, {2, 0}, "submatrix() accepts an in-bounds empty region");

        matrix transposed;
        original.transpose(transposed);
        require_matrix_close(transposed, make_matrix("1, 3; 2, 4"), "transpose()");
        const matrix & const_original = original;
        const_original.transpose(transposed);
        require_matrix_close(transposed, make_matrix("1, 3; 2, 4"), "const transpose()");

        matrix in_place_transpose = make_matrix("1, 2, 3; 4, 5, 6; 7, 8, 9");
        in_place_transpose.transpose(in_place_transpose);
        require_matrix_close(
            in_place_transpose,
            make_matrix("1, 4, 7; 2, 5, 8; 3, 6, 9"),
            "in-place transpose()"
        );
        matrix shallow_transpose_source = make_matrix("1, 2; 3, 4");
        matrix shallow_transpose_output = shallow_transpose_source;
        shallow_transpose_source.transpose(shallow_transpose_output);
        require_matrix_close(shallow_transpose_source, make_matrix("1, 3; 2, 4"),
                             "transpose() stages a distinct shallow output alias");
        matrix invalid_transpose_target;
        matrix rank_zero_transpose;
        require_throws([&]() { rank_zero_transpose.transpose(invalid_transpose_target); },
                       "transpose() should reject rank-zero input");
        matrix rank_one_transpose(3);
        require_throws([&]() { rank_one_transpose.transpose(invalid_transpose_target); },
                       "transpose() should reject rank-one input");
        matrix rank_three_transpose(2, 2, 2);
        require_throws([&]() { rank_three_transpose.transpose(invalid_transpose_target); },
                       "transpose() should reject rank-three input");
        require_true(invalid_transpose_target.is_uninitialized(),
                     "invalid transpose() leaves its destination unchanged");

        const matrix & const_scalar = scalar;
        require_close(const_scalar.scalar(), 0.0f, "const scalar access");

        matrix unlabeled(2, 3);
        require_true(unlabeled.labels(0).empty() && unlabeled.labels(1).empty(),
                     "unlabeled dimensions expose empty label lists");
        unlabeled.clear_labels(0);
        unlabeled.push_label(0, "unused", 0);
        require_true(unlabeled.labels(0).empty(),
                     "empty label mutations preserve unlabeled behavior");
        require_true(unlabeled.metadata_json().find("\"labels\": [[], []]") != std::string::npos,
                     "lazy labels preserve rank-sized metadata JSON");
        matrix unlabeled_slice = unlabeled[0];
        require_true(unlabeled_slice.labels(0).empty(),
                     "unlabeled slices expose empty label lists");

        matrix labeled(3, 4);
        labeled.set_labels(0, "row 0", "row 1");
        labeled.set_labels(-1, "column 0");
        require_true(labeled.labels(0).size() == 2 && labeled.labels(1).size() == 1,
                     "partial labels are allowed for each dimension");
        require_equal(labeled.labels(-1).at(0), "column 0",
                      "label queries accept negative dimensions");
        require_throws_as<std::invalid_argument>(
            [&]() { labeled.set_labels(0, "row 0", "row 1", "row 2", "row 3"); },
            "set_labels rejects labels beyond the dimension size"
        );
        require_true(labeled.labels(0).size() == 2,
                     "rejected set_labels leaves existing labels unchanged");
        labeled.push_label(0, "row 2");
        require_true(labeled.labels(0).size() == 3,
                     "push_label completes a partial label list");
        require_throws_as<std::invalid_argument>(
            [&]() { labeled.push_label(0, "row 3"); },
            "push_label rejects labels beyond the dimension size"
        );
        require_throws_as<std::invalid_argument>(
            [&]() { labeled.push_label(0, "row", -1); },
            "push_label rejects a negative label count"
        );
        require_throws_as<std::out_of_range>(
            [&]() { labeled.set_labels(2, "invalid"); },
            "set_labels rejects an invalid dimension"
        );
        require_true(labeled.labels(0).size() == 3,
                     "rejected label operations are atomic");

        labeled.set_labels(1, "column 0", "column 1", "column 2", "column 3");
        labeled.resize(2, 2);
        require_true(labeled.labels(0).size() == 2 && labeled.labels(1).size() == 2,
                     "resize trims labels to the new logical shape");
        labeled.resize(3, 4);
        require_true(labeled.labels(0).size() == 2 && labeled.labels(1).size() == 2,
                     "growing a shape preserves existing partial labels");

        matrix reshaped_labels(2, 3);
        reshaped_labels.set_labels(0, "first", "second");
        reshaped_labels.set_labels(1, "a", "b", "c");
        reshaped_labels.reshape(3, 2);
        require_true(reshaped_labels.labels(0).size() == 2 &&
                     reshaped_labels.labels(1).size() == 2,
                     "reshape preserves valid label prefixes and trims excess labels");

        matrix reallocated_labels(2, 2);
        reallocated_labels.set_labels(0, "first", "second");
        reallocated_labels.set_labels(1, "left", "right");
        reallocated_labels.realloc(1);
        require_true(reallocated_labels.labels(0).size() == 1,
                     "realloc removes dropped dimensions and trims retained labels");

        matrix cleared_labels(3);
        cleared_labels.set_labels(0, "a", "b", "c");
        cleared_labels.clear();
        require_true(cleared_labels.labels(0).empty(),
                     "clear removes labels from the emptied first dimension");

        matrix popped_source = make_matrix("1, 2, 3");
        popped_source.set_labels(0, "a", "b", "c");
        matrix popped_value;
        popped_value.pop(popped_source);
        require_true(popped_source.labels(0).size() == 2,
                     "pop trims labels from the shortened source");

        matrix unlabeled_csv = make_matrix("1, 2; 3, 4");
        require_equal(unlabeled_csv.csv(),
                      "1.000000,2.000000\n3.000000,4.000000\n",
                      "unlabeled CSV does not emit an empty header row");

        matrix partial_csv = make_matrix("1, 2, 3; 4, 5, 6");
        partial_csv.set_labels(1, "first");
        require_equal(partial_csv.csv(),
                      "first,,\n1.000000,2.000000,3.000000\n4.000000,5.000000,6.000000\n",
                      "partial CSV headers preserve column alignment");

        matrix quoted_csv = make_matrix("1, 2, 3, 4");
        quoted_csv.reshape(1, 4);
        quoted_csv.set_labels(1, "plain", "comma,value", "quote\"value", "line\nbreak");
        require_equal(quoted_csv.csv(),
                      "plain,\"comma,value\",\"quote\"\"value\",\"line\nbreak\"\n"
                      "1.000000,2.000000,3.000000,4.000000\n",
                      "CSV quotes separators, quotes, and newlines in header fields");

        matrix separated_csv(1, 2);
        separated_csv(0, 0) = 1.0f;
        separated_csv(0, 1) = 2.0f;
        separated_csv.set_labels(1, "plain", "semi;colon");
        require_equal(separated_csv.csv(";"),
                      "plain;\"semi;colon\"\n1.000000;2.000000\n",
                      "CSV quoting honors the requested separator");
        require_throws_as<std::invalid_argument>(
            [&]() { (void)separated_csv.csv(""); },
            "CSV rejects an empty separator"
        );

        matrix tracked = make_matrix("1, 2");
        const matrix & const_tracked = tracked;
        require_true(!const_tracked.changed(), "const changed() is false without saved state");
        {
            matrix scoped_tracked = make_matrix("1, 2");
            scoped_tracked.last();
            const matrix & const_scoped_tracked = scoped_tracked;
            require_true(!const_scoped_tracked.changed(), "const changed() is false after save");
            scoped_tracked(0) = 3.0f;
            require_true(const_scoped_tracked.changed(), "const changed() detects mutation");
            scoped_tracked.save();
            require_true(!const_scoped_tracked.changed(), "const changed() is false after resave");
        }
        {
            matrix reassigned_tracked = make_matrix("1, 2");
            reassigned_tracked.last();
            reassigned_tracked = make_matrix("3, 4");
            reassigned_tracked.last();
        }
        save_matrix_states();

        matrix copied_tracked = make_matrix("1, 2");
        copied_tracked.last();
        {
            matrix copy = copied_tracked;
            require_true(!copy.changed(), "copy construction preserves saved-state values");
        }
        copied_tracked(0) = 3.0f;
        save_matrix_states();
        require_true(!copied_tracked.changed(), "copy destruction leaves original saved-state registration intact");

        static_assert(std::is_copy_constructible_v<matrix>);
        static_assert(std::is_copy_assignable_v<matrix>);
        static_assert(std::is_nothrow_move_constructible_v<matrix>);
        static_assert(std::is_nothrow_move_assignable_v<matrix>);
        static_assert(std::is_same_v<decltype(std::declval<const matrix &>().share()), matrix>);
        static_assert(std::is_same_v<decltype(std::declval<const matrix &>().clone()), matrix>);

        matrix ownership_source = make_matrix("1, 2; 3, 4");
        ownership_source.set_name("ownership");
        ownership_source.set_labels(0, "top", "bottom");
        ownership_source.set_labels(1, "left", "right");
        matrix copy_constructed(ownership_source);
        require_true(copy_constructed.data() == ownership_source.data(),
                     "copy construction retains shallow data sharing");
        copy_constructed(0, 0) = 9.0f;
        require_close(ownership_source(0, 0), 9.0f,
                      "copy-constructed matrices share mutations");
        copy_constructed.set_labels(1, "first", "second");
        require_equal(ownership_source.labels(1).at(0), "first",
                      "copy-constructed matrices share metadata");

        matrix copy_assigned;
        copy_assigned = ownership_source;
        require_true(copy_assigned.data() == ownership_source.data(),
                     "copy assignment retains shallow data sharing");
        copy_assigned(1, 1) = 8.0f;
        require_close(ownership_source(1, 1), 8.0f,
                      "copy-assigned matrices share mutations");

        matrix explicitly_shared = ownership_source.share();
        require_true(explicitly_shared.data() == ownership_source.data(),
                     "share() explicitly creates a shallow alias");
        explicitly_shared(0, 1) = 7.0f;
        require_close(ownership_source(0, 1), 7.0f,
                      "share() aliases logical values");

        matrix explicitly_bound_storage(2, 2);
        explicitly_bound_storage.share_storage(ownership_source);
        require_true(explicitly_bound_storage.data() == ownership_source.data(),
                     "share_storage() binds compatible backing storage");
        explicitly_bound_storage(1, 0) = 14.0f;
        require_close(ownership_source(1, 0), 14.0f,
                      "share_storage() exposes shared mutations");
        explicitly_bound_storage(1, 0) = 3.0f;
        matrix incompatible_storage(4);
        require_throws_as<std::invalid_argument>(
            [&]() { incompatible_storage.share_storage(ownership_source); },
            "share_storage() rejects incompatible layouts"
        );

        matrix cloned = ownership_source.clone();
        require_true(cloned.data() != ownership_source.data(),
                     "clone() owns independent storage");
        require_matrix_close(cloned, ownership_source, "clone() copies logical values");
        require_equal(cloned.get_name(""), ownership_source.get_name(""),
                      "clone() preserves the matrix name");
        require_equal(cloned.labels(0).at(1), "bottom", "clone() preserves labels");
        cloned(0, 0) = -1.0f;
        cloned.set_labels(0, "clone-top", "clone-bottom");
        require_close(ownership_source(0, 0), 9.0f,
                      "clone() value mutations are independent");
        require_equal(ownership_source.labels(0).at(0), "top",
                      "clone() metadata mutations are independent");

        matrix cloned_view = ownership_source[1].clone();
        require_shape(cloned_view, {2}, "clone() preserves a view's logical shape");
        require_true(cloned_view.is_contiguous(), "clone() makes views contiguous");
        require_true(cloned_view.data() != ownership_source[1].data(),
                     "clone() detaches a view from parent storage");
        require_matrix_close(cloned_view, make_matrix("3, 8"),
                             "clone() copies a view's logical values");
        ownership_source(1, 0) = 6.0f;
        require_close(cloned_view(0), 3.0f, "cloned views remain independent of their parent");
        require_true(matrix().clone().is_uninitialized(),
                     "clone() preserves an uninitialized matrix");

        matrix move_source = make_matrix("11, 12");
        matrix move_alias = move_source;
        float * moved_storage = move_source.data();
        matrix move_constructed(std::move(move_source));
        require_true(move_constructed.data() == moved_storage,
                     "move construction transfers storage without copying");
        move_constructed(0) = 13.0f;
        require_close(move_alias(0), 13.0f,
                      "move construction preserves existing shallow aliases");
        move_source = make_matrix("21");
        require_close(move_source.scalar(), 21.0f,
                      "a moved-from matrix can be assigned a new value");

        matrix move_assignment_source = make_matrix("31, 32");
        float * move_assignment_storage = move_assignment_source.data();
        matrix move_assignment_target = make_matrix("0");
        move_assignment_target = std::move(move_assignment_source);
        require_true(move_assignment_target.data() == move_assignment_storage,
                     "move assignment transfers storage without copying");
        require_matrix_close(move_assignment_target, make_matrix("31, 32"),
                             "move assignment preserves values");
        move_assignment_source = make_matrix("41");
        require_close(move_assignment_source.scalar(), 41.0f,
                      "a move-assigned-from matrix can be assigned a new value");

        {
            matrix registered_move_source = make_matrix("1, 2");
            registered_move_source.last();
            matrix registered_move_target(std::move(registered_move_source));
            registered_move_target(0) = 5.0f;
            save_matrix_states();
            require_true(!registered_move_target.changed(),
                         "move construction transfers saved-state registration");
        }
        {
            matrix registered_move_source = make_matrix("3, 4");
            registered_move_source.last();
            matrix registered_move_target = make_matrix("5, 6");
            registered_move_target.last();
            registered_move_target = std::move(registered_move_source);
            registered_move_target(1) = 7.0f;
            save_matrix_states();
            require_true(!registered_move_target.changed(),
                         "move assignment transfers saved-state registration");
        }

        matrix concurrent_saved_state = make_matrix("1, 2, 3");
        concurrent_saved_state.last();
        std::vector<std::thread> saved_state_workers;
        for(int worker = 0; worker < 4; ++worker)
            saved_state_workers.emplace_back([&]() {
                for(int iteration = 0; iteration < 250; ++iteration)
                {
                    concurrent_saved_state.last();
                    concurrent_saved_state.save();
                    (void)concurrent_saved_state.changed();
                }
            });
        for(auto & worker : saved_state_workers)
            worker.join();
        require_true(!concurrent_saved_state.changed(),
                     "concurrent saved-state access remains consistent");

        std::atomic<bool> lifecycle_workers_done = false;
        std::thread registry_saver([&]() {
            while(!lifecycle_workers_done.load(std::memory_order_acquire))
                save_matrix_states();
        });
        saved_state_workers.clear();
        for(int worker = 0; worker < 4; ++worker)
            saved_state_workers.emplace_back([]() {
                for(int iteration = 0; iteration < 100; ++iteration)
                {
                    matrix temporary = make_matrix("1, 2");
                    temporary.last();
                }
            });
        for(auto & worker : saved_state_workers)
            worker.join();
        lifecycle_workers_done.store(true, std::memory_order_release);
        registry_saver.join();
        save_matrix_states();

        matrix stack(3, 2);
        stack.resize(0, 2);
        matrix first = make_matrix("1, 2");
        matrix second = make_matrix("3, 4");
        stack.push(first);
        stack.push(second);
        require_shape(stack, {2, 2}, "push(matrix)");
        require_matrix_close(stack[0], first, "push(matrix) first element");
        require_matrix_close(stack[1], second, "push(matrix) second element");

        matrix popped(2);
        popped.pop(stack);
        require_matrix_close(popped, second, "pop(matrix)");
        require_shape(stack, {1, 2}, "pop(matrix) shrinks source");

        matrix vector(3);
        vector.resize(0);
        vector.push(5.0f);
        vector.push(6.0f);
        require_matrix_close(vector, make_matrix("5, 6"), "push(float)");

        matrix dynamic_stack;
        matrix & first_append_result = dynamic_stack.append(first);
        require_true(&first_append_result == &dynamic_stack, "append(matrix) returns its destination");
        dynamic_stack.append(second);
        dynamic_stack.append(make_matrix("5, 6"));
        require_shape(dynamic_stack, {3, 2}, "append(matrix) grows uninitialized matrix");
        require_true(dynamic_stack.capacity() == std::vector<int>({4, 2}),
                     "append(matrix) grows capacity geometrically");
        require_matrix_close(dynamic_stack, make_matrix("1, 2; 3, 4; 5, 6"), "append(matrix)");

        matrix dynamic_vector;
        dynamic_vector.append(7.0f);
        dynamic_vector.append(8.0f);
        dynamic_vector.append(9.0f);
        require_matrix_close(dynamic_vector, make_matrix("7, 8, 9"), "append(float)");

        matrix fixed_push(1, 2);
        fixed_push.resize(0, 2);
        require_true(&fixed_push.push(first) == &fixed_push,
                     "push(matrix) returns its destination");
        require_throws_as<std::out_of_range>(
            [&]() { fixed_push.push(second); },
            "push(matrix) rejects growth beyond preallocated capacity"
        );
        require_shape(fixed_push, {1, 2}, "failed push(matrix) preserves shape");
        require_matrix_close(fixed_push[0], first, "failed push(matrix) preserves values");

        matrix self_append;
        self_append.append(first);
        self_append.append(self_append[0]);
        require_matrix_close(self_append, make_matrix("1, 2; 1, 2"),
                             "append(matrix) stages a source that shares growing storage");

#ifndef NDEBUG
        for(int failure_point : {0, 1, 2})
        {
            matrix failed_append;
            failed_append.append(first);
            float * original_storage = failed_append.data();
            matrix::set_allocation_failure_countdown_for_testing(failure_point);
            require_throws_as<out_of_memory_matrix_error>(
                [&]() { failed_append.append(second); },
                "append(matrix) should translate injected growth failure"
            );
            matrix::set_allocation_failure_countdown_for_testing(-1);
            require_true(failed_append.data() == original_storage,
                         "failed append(matrix) preserves the backing allocation");
            require_true(failed_append.capacity() == std::vector<int>({1, 2}),
                         "failed append(matrix) preserves capacity at failure point " +
                             std::to_string(failure_point));
            require_shape(failed_append, {1, 2},
                          "failed append(matrix) preserves shape at failure point " +
                              std::to_string(failure_point));
            require_matrix_close(failed_append[0], first,
                                 "failed append(matrix) preserves values at failure point " +
                                     std::to_string(failure_point));
        }
#endif

        matrix reserved;
        reserved.reserve(4, 2);
        require_true(reserved.empty(), "reserved matrix with zero logical rows is empty");
        require_true(!reserved.is_uninitialized(), "reserved matrix is initialized");
        reserved.append(first);
        reserved.append(second);
        reserved.clear();
        require_shape(reserved, {0, 2}, "clear() preserves slice shape");
        require_true(reserved.empty(), "clear() removes all logical elements");
        require_true(!reserved.is_uninitialized(), "clear() preserves initialized state");
        require_true(reserved.capacity() == std::vector<int>({4, 2}), "clear() preserves reserved capacity");
        reserved.append(make_matrix("9, 10"));
        require_shape(reserved, {1, 2}, "reserve()/clear()/append() shape");
        require_matrix_close(reserved[0], make_matrix("9, 10"), "reserve()/clear()/append()");

        matrix reserve_with_view = make_matrix("1, 2; 3, 4");
        matrix retained_view = reserve_with_view[0];
        reserve_with_view.reserve(4, 2);
        require_matrix_close(retained_view, make_matrix("1, 2"),
                             "reserve() preserves an existing slice view");

        std::mt19937 rng(1234);
        matrix random_values(32);
        random_values.fill_xavier_uniform(rng, 6);
        const float xavier_limit = std::sqrt(6.0f / 6.0f);
        require_true(random_values.min() >= -xavier_limit && random_values.max() <= xavier_limit, "fill_xavier_uniform() range");

        matrix guarded_values(8);
        guarded_values.fill_xavier_uniform(rng, 0);
        const float guarded_limit = std::sqrt(6.0f);
        require_true(guarded_values.min() >= -guarded_limit && guarded_values.max() <= guarded_limit, "fill_xavier_uniform() fan_in guard");
    }

    void test_elementwise()
    {
        matrix a = make_matrix("1, 2; 3, 4");
        matrix b = make_matrix("5, 6; 7, 8");

        matrix r1;
        r1.copy(a).add(b);
        require_matrix_close(r1, make_matrix("6, 8; 10, 12"), "add(matrix)");

        matrix r2;
        r2.copy(b).subtract(a);
        require_matrix_close(r2, make_matrix("4, 4; 4, 4"), "subtract(matrix)");

        matrix r3;
        r3.copy(a).multiply(b);
        require_matrix_close(r3, make_matrix("5, 12; 21, 32"), "multiply(matrix)");

        matrix r4;
        r4.copy(b).divide(a);
        require_matrix_close(r4, make_matrix("5, 3; 2.3333333, 2"), "divide(matrix)");

        matrix r5;
        r5.copy(a).maximum(make_matrix("0, 3; 5, 1"));
        require_matrix_close(r5, make_matrix("1, 3; 5, 4"), "maximum(matrix)");

        matrix r6;
        r6.copy(a).minimum(make_matrix("0, 3; 5, 1"));
        require_matrix_close(r6, make_matrix("0, 2; 3, 1"), "minimum(matrix)");

        matrix l1 = make_matrix("1, 0; 1, 0");
        matrix l2 = make_matrix("1, 1; 0, 0");

        matrix l_and;
        l_and.copy(l1).logical_and(l2);
        require_matrix_close(l_and, make_matrix("1, 0; 0, 0"), "logical_and(matrix)");

        matrix l_or;
        l_or.copy(l1).logical_or(l2);
        require_matrix_close(l_or, make_matrix("1, 1; 1, 0"), "logical_or(matrix)");

        matrix l_xor;
        l_xor.copy(l1).logical_xor(l2);
        require_matrix_close(l_xor, make_matrix("0, 1; 1, 0"), "logical_xor(matrix)");

        matrix c1(2, 2);
        c1.add(a, b);
        require_matrix_close(c1, make_matrix("6, 8; 10, 12"), "add(matrix, matrix)");

        matrix c2(2, 2);
        c2.subtract(b, a);
        require_matrix_close(c2, make_matrix("4, 4; 4, 4"), "subtract(matrix, matrix)");

        matrix c3(2, 2);
        c3.multiply(a, b);
        require_matrix_close(c3, make_matrix("5, 12; 21, 32"), "multiply(matrix, matrix)");

        matrix c4(2, 2);
        c4.divide(b, a);
        require_matrix_close(c4, make_matrix("5, 3; 2.3333333, 2"), "divide(matrix, matrix)");

        matrix c5(2, 2);
        c5.maximum(a, make_matrix("0, 3; 5, 1"));
        require_matrix_close(c5, make_matrix("1, 3; 5, 4"), "maximum(matrix, matrix)");

        matrix c6(2, 2);
        c6.minimum(a, make_matrix("0, 3; 5, 1"));
        require_matrix_close(c6, make_matrix("0, 2; 3, 1"), "minimum(matrix, matrix)");

        matrix c7(2, 2);
        c7.logical_and(l1, l2);
        require_matrix_close(c7, make_matrix("1, 0; 0, 0"), "logical_and(matrix, matrix)");

        matrix c8(2, 2);
        c8.logical_or(l1, l2);
        require_matrix_close(c8, make_matrix("1, 1; 1, 0"), "logical_or(matrix, matrix)");

        matrix c9(2, 2);
        c9.logical_xor(l1, l2);
        require_matrix_close(c9, make_matrix("0, 1; 1, 0"), "logical_xor(matrix, matrix)");

        matrix accumulated = make_matrix("1, 2");
        accumulated.multiply_and_accumulate(make_matrix("3, 4"), 0.5f);
        require_matrix_close(accumulated, make_matrix("2.5, 4"),
                             "multiply_and_accumulate()");

        matrix exact_accumulation_alias = make_matrix("1, 2");
        matrix exact_accumulation_input = exact_accumulation_alias;
        exact_accumulation_alias.multiply_and_accumulate(exact_accumulation_input, 2.0f);
        require_matrix_close(exact_accumulation_input, make_matrix("3, 6"),
                             "multiply_and_accumulate() accepts an exact alias");

        matrix sigmoid_values = make_matrix("-100, 0, 100");
        sigmoid_values.sigmoid();
        require_matrix_close(sigmoid_values, make_matrix("0, 0.5, 1"),
                             "sigmoid() remains stable at large magnitudes");

        matrix channel_values(std::vector<int>{2, 1, 2});
        channel_values(0, 0, 0) = 1.0f;
        channel_values(0, 0, 1) = 2.0f;
        channel_values(1, 0, 0) = 3.0f;
        channel_values(1, 0, 1) = 4.0f;
        channel_values.add_channel_bias(make_matrix("10, 20"));
        matrix expected_channel_values(std::vector<int>{2, 1, 2});
        expected_channel_values(0, 0, 0) = 11.0f;
        expected_channel_values(0, 0, 1) = 12.0f;
        expected_channel_values(1, 0, 0) = 23.0f;
        expected_channel_values(1, 0, 1) = 24.0f;
        require_matrix_close(channel_values, expected_channel_values, "add_channel_bias()");

        matrix aliased_channel_values = channel_values.clone();
        matrix overlapping_channel_bias = aliased_channel_values[0][0];
        matrix original_channel_values = aliased_channel_values.clone();
        require_throws_as<std::invalid_argument>(
            [&]() { aliased_channel_values.add_channel_bias(overlapping_channel_bias); },
            "add_channel_bias() rejects overlapping bias storage"
        );
        require_matrix_close(aliased_channel_values, original_channel_values,
                             "add_channel_bias() rejects aliases before writing");

        matrix sgd_values = make_matrix("1, 2");
        sgd_values.sgd_update(make_matrix("0.5, -1"), 0.1f);
        require_matrix_close(sgd_values, make_matrix("0.95, 2.1"), "sgd_update()");

        matrix relu_source = make_matrix("-1, 2");
        matrix relu_exact_alias = relu_source;
        relu_exact_alias.relu(relu_source);
        require_matrix_close(relu_source, make_matrix("0, 2"),
                             "relu() accepts an exact-layout alias");

        matrix scaled_source = make_matrix("1, 2");
        matrix scaled_exact_alias = scaled_source;
        scaled_exact_alias.scale(scaled_source, 3.0f);
        require_matrix_close(scaled_source, make_matrix("3, 6"),
                             "scale(A) accepts an exact-layout alias");

        matrix exponential;
        exponential.exp_scaled(make_matrix("0, 0.69314718056"), 1.0f);
        require_matrix_close(exponential, make_matrix("1, 2"), "exp_scaled()", 1e-5f);

        matrix exponential_minus_one;
        exponential_minus_one.exp_minus_one_scaled(
            make_matrix("0, 0.69314718056"),
            0.5f
        );
        require_matrix_close(exponential_minus_one, make_matrix("0, 0.5"),
                             "exp_minus_one_scaled()", 1e-5f);

        matrix add_scaled_source = make_matrix("1, 2");
        matrix add_scaled_exact_alias = add_scaled_source;
        add_scaled_exact_alias.add_scaled(
            add_scaled_source,
            make_matrix("3, 4"),
            0.5f
        );
        require_matrix_close(add_scaled_source, make_matrix("2.5, 4"),
                             "add_scaled() accepts an exact-layout input alias");

        matrix relu_backward_result;
        relu_backward_result.relu_backward(make_matrix("1, 2; 3, 4"), make_matrix("-1, 0; 0.5, 2"));
        require_matrix_close(relu_backward_result, make_matrix("0, 0; 3, 4"), "relu_backward()");

        matrix adam_values = make_matrix("1, 2");
        matrix adam_gradients = make_matrix("0.1, -0.2");
        matrix adam_m(2);
        matrix adam_v(2);
        adam_values.adam_update(adam_gradients, adam_m, adam_v, 0.001f, 0.9f, 0.999f, 0.1f, 0.001f, 1e-8f);
        require_matrix_close(adam_values, make_matrix("0.999, 2.001"), "adam_update()", 1e-5f);
    }

    void test_linalg()
    {
        matrix y = make_matrix("3, 5");
        matrix x = make_matrix("4, 12");
        matrix hypot_result(2);
        hypot_result.hypot(y, x);
        require_matrix_close(hypot_result, make_matrix("5, 13"), "hypot()");

        matrix ay = make_matrix("0, 1");
        matrix ax = make_matrix("1, 0");
        matrix atan_result(2);
        atan_result.atan2(ay, ax);
        require_close(atan_result(0), 0.0f, "atan2() first element");
        require_close(atan_result(1), kHalfPi, "atan2() second element");

        matrix left = make_matrix("1, 2, 3; 4, 5, 6");
        matrix right = make_matrix("7, 8; 9, 10; 11, 12");
        matrix mmul;
        mmul.matmul(left, right);
        require_matrix_close(mmul, make_matrix("58, 64; 139, 154"), "matmul()");

        matrix mvec;
        mvec.matvec(left, make_matrix("7, 8, 9"));
        require_matrix_close(mvec, make_matrix("50, 122"), "matvec()");

        matrix outer_product_result;
        outer_product_result.outer_product(make_matrix("1, 2, 3"), make_matrix("4, 5"));
        require_matrix_close(outer_product_result, make_matrix("4, 5; 8, 10; 12, 15"), "outer_product()");

        matrix row_gapped_outer_product(3, 4);
        row_gapped_outer_product.set(-1000.0f);
        row_gapped_outer_product.resize(3, 2);
        row_gapped_outer_product.outer_product(make_matrix("1, 2, 3"), make_matrix("4, 5"));
        require_matrix_close(row_gapped_outer_product, outer_product_result,
                             "outer_product() row-gapped output");

        matrix dense_weights = make_matrix("3, 4, 5; 6, 7, 8");
        matrix row_gapped_dense_weights(2, 5);
        row_gapped_dense_weights.set(-1000.0f);
        row_gapped_dense_weights.resize(2, 3);
        row_gapped_dense_weights.copy(dense_weights);

        matrix dense_forward_result;
        dense_forward_result.dense_forward(make_matrix("1, 2"), dense_weights);
        require_matrix_close(dense_forward_result, make_matrix("15, 18, 21"), "dense_forward()");

        matrix row_gapped_dense_forward_result;
        row_gapped_dense_forward_result.dense_forward(make_matrix("1, 2"), row_gapped_dense_weights);
        require_matrix_close(row_gapped_dense_forward_result, dense_forward_result,
                             "dense_forward() row-gapped weights");

        matrix dense_backward_input_result;
        dense_backward_input_result.dense_backward_input(dense_weights, make_matrix("1, 2, 3"));
        require_matrix_close(dense_backward_input_result, make_matrix("26, 44"), "dense_backward_input()");

        matrix row_gapped_dense_backward_input_result;
        row_gapped_dense_backward_input_result.dense_backward_input(
            row_gapped_dense_weights,
            make_matrix("1, 2, 3")
        );
        require_matrix_close(row_gapped_dense_backward_input_result, dense_backward_input_result,
                             "dense_backward_input() row-gapped weights");

        matrix aliased_multiply_input = make_matrix("1, 2; 3, 4");
        matrix aliased_multiply_output = aliased_multiply_input;
        matrix aliased_multiply_rhs = make_matrix("5, 6; 7, 8");
        require_throws_as<std::invalid_argument>(
            [&]() { aliased_multiply_output.matmul(aliased_multiply_input, aliased_multiply_rhs); },
            "matmul() rejects a shallow output alias"
        );
        require_matrix_close(aliased_multiply_input, make_matrix("1, 2; 3, 4"),
                             "matmul() alias rejection occurs before writing");

        matrix aliased_hypot_input = make_matrix("3, 4");
        matrix aliased_hypot_output = aliased_hypot_input;
        require_throws_as<std::invalid_argument>(
            [&]() { aliased_hypot_output.hypot(aliased_hypot_input, make_matrix("4, 3")); },
            "hypot() rejects a shallow output alias"
        );
        require_matrix_close(aliased_hypot_input, make_matrix("3, 4"),
                             "hypot() alias rejection occurs before writing");

        matrix aliased_atan_y = make_matrix("0, 1");
        matrix aliased_atan_output = aliased_atan_y;
        require_throws_as<std::invalid_argument>(
            [&]() { aliased_atan_output.atan2(aliased_atan_y, make_matrix("1, 0")); },
            "atan2() rejects a shallow output alias"
        );
        require_matrix_close(aliased_atan_y, make_matrix("0, 1"),
                             "atan2() alias rejection occurs before writing");

        matrix aliased_matvec_input = make_matrix("7, 8");
        matrix aliased_matvec_output = aliased_matvec_input;
        require_throws_as<std::invalid_argument>(
            [&]()
            {
                aliased_matvec_output.matvec(
                    make_matrix("1, 2; 3, 4"),
                    aliased_matvec_input
                );
            },
            "matvec() rejects a shallow vector alias"
        );
        require_matrix_close(aliased_matvec_input, make_matrix("7, 8"),
                             "matvec() alias rejection occurs before writing");

        matrix outer_alias_parent(std::vector<int>{2, 2, 2});
        fill_sequence(outer_alias_parent);
        matrix overlapping_outer_output = outer_alias_parent[0];
        matrix overlapping_outer_left = outer_alias_parent[0][0];
        matrix original_outer_alias_parent = outer_alias_parent.clone();
        require_throws_as<std::invalid_argument>(
            [&]()
            {
                overlapping_outer_output.outer_product(
                    overlapping_outer_left,
                    make_matrix("3, 4")
                );
            },
            "outer_product() rejects an overlapping subview"
        );
        require_matrix_close(outer_alias_parent, original_outer_alias_parent,
                             "outer_product() alias rejection occurs before writing");

        matrix shared_dense_parent = make_matrix("1, 2; 9, 9");
        matrix shared_dense_input = shared_dense_parent[0];
        matrix shared_dense_output = shared_dense_parent[1];
        shared_dense_output.dense_forward(shared_dense_input, make_matrix("1, 0; 0, 1"));
        require_matrix_close(shared_dense_parent, make_matrix("1, 2; 1, 2"),
                             "dense_forward() accepts non-overlapping shared views");

        matrix aliased_dense_input = make_matrix("1, 2");
        matrix aliased_dense_output = aliased_dense_input;
        require_throws_as<std::invalid_argument>(
            [&]() { aliased_dense_output.dense_forward(aliased_dense_input, make_matrix("1, 0; 0, 1")); },
            "dense_forward() rejects an overlapping shallow alias"
        );
        require_matrix_close(aliased_dense_input, make_matrix("1, 2"),
                             "dense_forward() alias rejection occurs before writing");

        matrix dense_backward_alias_weights = make_matrix("1, 2; 3, 4");
        matrix overlapping_dense_backward_output = dense_backward_alias_weights[0];
        matrix original_dense_backward_weights = dense_backward_alias_weights.clone();
        require_throws_as<std::invalid_argument>(
            [&]()
            {
                overlapping_dense_backward_output.dense_backward_input(
                    dense_backward_alias_weights,
                    make_matrix("5, 6")
                );
            },
            "dense_backward_input() rejects an overlapping weight subview"
        );
        require_matrix_close(dense_backward_alias_weights, original_dense_backward_weights,
                             "dense_backward_input() alias rejection occurs before writing");

        matrix inverse = make_matrix("4, 7; 2, 6");
        inverse.inv();
        require_matrix_close(inverse, make_matrix("0.6, -0.7; -0.2, 0.4"), "inv()", 1e-3f);

        matrix strided_inverse(2, 4);
        strided_inverse.set(-1000.0f);
        strided_inverse.resize(2, 2);
        strided_inverse.copy(make_matrix("4, 7; 2, 6"));
        strided_inverse.inv();
        require_matrix_close(strided_inverse, make_matrix("0.6, -0.7; -0.2, 0.4"), "inv() row-gapped", 1e-3f);

        auto require_svd_reconstruction = [&](matrix input, const std::string & message)
        {
            matrix U;
            matrix S;
            matrix Vt;
            input.singular_value_decomposition(input, U, S, Vt);

            matrix US;
            US.matmul(U, S);
            matrix reconstructed;
            reconstructed.matmul(US, Vt);
            require_matrix_close(reconstructed, input, message, 1e-3f);
        };

        require_svd_reconstruction(
            make_matrix("1, 2; 3, 4"),
            "singular_value_decomposition() reconstruction"
        );
        require_svd_reconstruction(
            make_matrix("1, 2; 3, 4; 5, 7"),
            "singular_value_decomposition() tall reconstruction"
        );

        matrix strided_svd_input(2, 6);
        strided_svd_input.set(-1000.0f);
        strided_svd_input.resize(2, 3);
        strided_svd_input.copy(make_matrix("1, 2, 3; 4, 5, 7"));
        require_svd_reconstruction(
            strided_svd_input,
            "singular_value_decomposition() strided reconstruction"
        );

        matrix svd_alias_input = make_matrix("1, 2; 3, 4");
        matrix shared_svd_output;
        matrix shared_svd_second_output = shared_svd_output;
        matrix independent_svd_output;
        require_throws_as<std::invalid_argument>(
            [&]()
            {
                svd_alias_input.singular_value_decomposition(
                    svd_alias_input,
                    shared_svd_output,
                    shared_svd_second_output,
                    independent_svd_output
                );
            },
            "singular_value_decomposition() rejects shared output storage"
        );

        matrix input_alias_svd_output = svd_alias_input;
        matrix svd_s;
        matrix svd_vt;
        require_throws_as<std::invalid_argument>(
            [&]()
            {
                svd_alias_input.singular_value_decomposition(
                    svd_alias_input,
                    input_alias_svd_output,
                    svd_s,
                    svd_vt
                );
            },
            "singular_value_decomposition() rejects an output aliasing its input"
        );
        require_matrix_close(svd_alias_input, make_matrix("1, 2; 3, 4"),
                             "SVD input alias rejection occurs before writing");

        matrix rank_one = make_matrix("1, 2; 2, 4");
        require_close(rank_one.matrank(), 1.0f, "matrank()");

        matrix trace_and_determinant = make_matrix("1, 2; 3, 4");
        require_close(trace_and_determinant.trace(), 5.0f, "trace()");
        require_close(trace_and_determinant.det(), -2.0f, "det()");

        matrix pseudoinverse;
        pseudoinverse.pinv(make_matrix("1, 0; 0, 2; 0, 0"));
        require_matrix_close(pseudoinverse, make_matrix("1, 0, 0; 0, 0.5, 0"), "pinv()", 1e-3f);

        matrix aliased_pseudoinverse_input = make_matrix("1, 0; 0, 2");
        matrix aliased_pseudoinverse_output = aliased_pseudoinverse_input;
        aliased_pseudoinverse_output.pinv(aliased_pseudoinverse_input);
        require_matrix_close(aliased_pseudoinverse_input, make_matrix("1, 0; 0, 0.5"),
                             "pinv() stages a shallow input alias", 1e-3f);

        matrix symmetric = make_matrix("2, 1; 1, 2");
        auto [eigenvectors, eigenvalues] = symmetric.eig();
        require_matrix_close(eigenvalues, make_matrix("1, 3"), "eig() eigenvalues", 1e-3f);
        matrix eigenvectors_times_values(2, 2);
        for(int row = 0; row < 2; ++row)
            for(int col = 0; col < 2; ++col)
                eigenvectors_times_values(row, col) = eigenvectors(row, col) * eigenvalues(col);
        matrix eigenvector_product;
        eigenvector_product.matmul(symmetric, eigenvectors);
        require_matrix_close(eigenvector_product, eigenvectors_times_values, "eig() eigenvectors", 1e-3f);
        require_throws([&]() { make_matrix("1, 2; 3, 4").eig(); }, "eig() should reject non-symmetric matrices");
    }

    void test_image()
    {
        matrix gaussian_kernel;
        gaussian_kernel.gaussian(1.0f);
        require_shape(gaussian_kernel, {7, 7}, "gaussian()");
        require_close(gaussian_kernel.sum(), 1.0f, "gaussian() normalization", 1e-3f);

        float * gaussian_storage = gaussian_kernel.data();
        gaussian_kernel.gaussian(1.0f);
        require_true(gaussian_kernel.data() == gaussian_storage,
                     "gaussian() reuses a correctly sized destination");

        matrix tiny_gaussian;
        tiny_gaussian.gaussian(std::numeric_limits<float>::denorm_min());
        require_shape(tiny_gaussian, {1, 1}, "gaussian() with tiny positive sigma");
        require_close(tiny_gaussian(0, 0), 1.0f, "tiny gaussian() remains finite");

        matrix wrong_gaussian_size(3, 3);
        wrong_gaussian_size.set(9.0f);
        require_throws([&]() { wrong_gaussian_size.gaussian(1.0f); },
                       "gaussian() should reject a wrong-sized initialized destination");
        require_matrix_close(wrong_gaussian_size, matrix(3, 3).set(9.0f),
                             "rejected gaussian() preserves its destination");

        for(float invalid_sigma : {
                0.0f,
                -1.0f,
                std::numeric_limits<float>::quiet_NaN(),
                std::numeric_limits<float>::infinity()
            })
        {
            matrix invalid_gaussian;
            require_throws([&]() { invalid_gaussian.gaussian(invalid_sigma); },
                           "gaussian() should reject non-positive or non-finite sigma");
            require_true(invalid_gaussian.is_uninitialized(),
                         "invalid gaussian() leaves its destination unchanged");
        }

        matrix huge_gaussian;
        require_throws([&]() { huge_gaussian.gaussian(std::numeric_limits<float>::max()); },
                       "gaussian() should reject an unrepresentable kernel dimension");
        require_true(huge_gaussian.is_uninitialized(),
                     "oversized gaussian() leaves its destination unchanged");

        matrix downsample_source = make_matrix("1, 2, 3, 4; 5, 6, 7, 8; 9, 10, 11, 12; 13, 14, 15, 16");
        matrix expected_downsample = make_matrix("3.5, 5.5; 11.5, 13.5");
        matrix temporary_row;
        matrix downsampled;
        downsampled.downsample(downsample_source, temporary_row);
        require_matrix_close(downsampled, expected_downsample, "downsample() with temporary row");
        require_shape(temporary_row, {4}, "downsample() temporary row allocation");

        matrix initialized_empty_row(0);
        downsampled.downsample(downsample_source, initialized_empty_row);
        require_shape(initialized_empty_row, {4}, "downsample() initialized empty row allocation");

        matrix zero_width_source(4, 0);
        matrix zero_width_temporary_row;
        matrix zero_width_downsample;
        zero_width_downsample.downsample(zero_width_source, zero_width_temporary_row);
        require_shape(zero_width_downsample, {2, 0}, "downsample() zero-width result");
        require_shape(zero_width_temporary_row, {0}, "downsample() zero-width temporary row");

        float * temporary_row_data = temporary_row.data();
        downsampled.downsample(downsample_source, temporary_row);
        require_true(temporary_row.data() == temporary_row_data, "downsample() temporary row reuse");

        matrix compatibility_downsample;
        compatibility_downsample.downsample(downsample_source);
        require_matrix_close(compatibility_downsample, expected_downsample, "downsample() compatibility overload");

        matrix strided_downsample_source(4, 8);
        strided_downsample_source.set(-1000.0f);
        strided_downsample_source.resize(4, 4);
        strided_downsample_source.copy(downsample_source);
        matrix strided_downsample(2, 4);
        strided_downsample.set(-1000.0f);
        strided_downsample.resize(2, 2);
        strided_downsample.downsample(strided_downsample_source, temporary_row);
        require_matrix_close(strided_downsample, expected_downsample, "downsample() row-gapped matrices");

        matrix downsample_shared_parent(std::vector<int>{2, 4, 4});
        fill_sequence(downsample_shared_parent);
        matrix downsample_shared_source = downsample_shared_parent[0];
        matrix downsample_shared_destination = downsample_shared_parent[1];
        downsample_shared_source.copy(downsample_source);
        downsample_shared_destination.resize(2, 2);
        matrix original_downsample_shared_parent = downsample_shared_parent.clone();
        require_throws_as<std::invalid_argument>(
            [&]() { downsample_shared_destination.downsample(downsample_shared_source); },
            "downsample() rejects shared storage even for sibling views"
        );
        require_matrix_close(downsample_shared_parent, original_downsample_shared_parent,
                             "downsample() shared-storage rejection occurs before writing");

        matrix image = make_matrix("1, 2, 3; 4, 5, 6; 7, 8, 9");
        matrix kernel = make_matrix("1, 0; 0, -1");

        matrix corr(2, 2);
        corr.corr2(image, kernel);
        require_matrix_close(corr, make_matrix("-4, -4; -4, -4"), "corr2()");

        matrix row_gapped_correlation_input(3, 5);
        row_gapped_correlation_input.set(-1000.0f);
        row_gapped_correlation_input.resize(3, 3);
        row_gapped_correlation_input.copy(image);
        std::vector<float> legacy_patches;
        im2row(legacy_patches, row_gapped_correlation_input, kernel);
        require_true(legacy_patches.size() == 16, "im2row() allocates its patch buffer");
        matrix legacy_correlation(2, 4);
        legacy_correlation.set(-1000.0f);
        legacy_correlation.resize(2, 2);
        legacy_correlation.corr3(
            row_gapped_correlation_input,
            kernel,
            flattenKernel(kernel),
            legacy_patches
        );
        require_matrix_close(legacy_correlation, make_matrix("-4, -4; -4, -4"), "corr3() row-gapped input");

        matrix same_correlation;
        same_correlation.corr2(image, kernel, matrix::convolution_padding::same);
        require_matrix_close(
            same_correlation,
            make_matrix("-4, -4, 3; -4, -4, 6; 7, 8, 9"),
            "corr2() zero-padded same correlation"
        );

        matrix conv(2, 2);
        conv.conv2_slow(image, kernel);
        require_matrix_close(conv, make_matrix("4, 4; 4, 4"), "conv2_slow()");

        matrix same_convolution;
        same_convolution.conv2(image, make_matrix("1, 1, 1; 1, 1, 1; 1, 1, 1"));
        matrix expected_same_convolution = make_matrix("12, 21, 16; 27, 45, 33; 24, 39, 28");
        require_matrix_close(same_convolution, expected_same_convolution, "conv2() zero-padded same convolution");

        matrix strided_convolution_input(3, 5);
        strided_convolution_input.set(-1000.0f);
        strided_convolution_input.resize(3, 3);
        strided_convolution_input.copy(image);
        matrix strided_convolution(3, 5);
        strided_convolution.set(-1000.0f);
        strided_convolution.resize(3, 3);
        strided_convolution.conv2(strided_convolution_input, make_matrix("1, 1, 1; 1, 1, 1; 1, 1, 1"));
        require_matrix_close(strided_convolution, expected_same_convolution, "conv2() zero-padded row-gapped convolution");

        matrix search_image = make_matrix("0, 0, 0, 0; 0, 1, 2, 0; 0, 3, 9, 0; 0, 0, 0, 0");
        matrix search_target = make_matrix("1, 2; 3, 9");
        match contiguous_match = search_image.search(search_target, rect{0, 0, 4, 4});
        require_close(contiguous_match.x, 1.0f, "search() contiguous x");
        require_close(contiguous_match.y, 1.0f, "search() contiguous y");

        matrix strided_search_image(4, 8);
        strided_search_image.set(1000.0f);
        strided_search_image.resize(4, 4);
        strided_search_image.copy(search_image);
        matrix strided_search_target(2, 4);
        strided_search_target.set(-1000.0f);
        strided_search_target.resize(2, 2);
        strided_search_target.copy(search_target);
        match strided_match = strided_search_image.search(strided_search_target, rect{0, 0, 4, 4});
        require_close(strided_match.x, contiguous_match.x, "search() row-gapped x");
        require_close(strided_match.y, contiguous_match.y, "search() row-gapped y");

        matrix offset_search_image = search_image.clone();
        offset_search_image.add(1000000.0f);
        matrix offset_search_target = search_target.clone();
        offset_search_target.add(1000000.0f);
        match offset_match = offset_search_image.search(
            offset_search_target,
            rect{0, 0, 4, 4}
        );
        require_close(offset_match.x, contiguous_match.x,
                      "search() remains stable with a large common offset x");
        require_close(offset_match.y, contiguous_match.y,
                      "search() remains stable with a large common offset y");
        require_close(offset_match.score, contiguous_match.score,
                      "search() score remains stable with a large common offset", 1e-3f);

        matrix descending_search_image("[[4, 3, 2]]");
        matrix ascending_search_target("[[1, 2]]");
        match negative_match = descending_search_image.search(
            ascending_search_target,
            rect{0, 0, 3, 1}
        );
        require_close(negative_match.x, 0.0f, "search() all-negative match x");
        require_true(negative_match.score < 0.0f,
                     "search() preserves the best all-negative correlation score");

        matrix invalid_search_source(3);
        require_throws([&]() { invalid_search_source.search(ascending_search_target, rect{0, 0, 3, 1}); },
                       "search() should reject a non-two-dimensional source");
        require_throws([&]() { search_image.search(matrix(2), rect{0, 0, 4, 4}); },
                       "search() should reject a non-two-dimensional target");
        require_throws([&]() { search_image.search(search_target, rect{0, 0, 1, 1}); },
                       "search() should require the target to fit the search rectangle");
        require_throws([&]() { search_image.search(search_target, rect{0, 0, 0, 4}); },
                       "search() should reject an empty search rectangle");
        require_throws(
            [&]()
            {
                search_image.search(
                    search_target,
                    rect{std::numeric_limits<int>::max(), 0, std::numeric_limits<int>::max(), 4}
                );
            },
            "search() should reject overflowing rectangle coordinates"
        );

        matrix filters(std::vector<int>{2, 2, 2});
        filters(0, 0, 0) = 1.0f;
        filters(0, 0, 1) = 0.0f;
        filters(0, 1, 0) = 0.0f;
        filters(0, 1, 1) = -1.0f;
        filters(1, 0, 0) = 1.0f;
        filters(1, 0, 1) = 1.0f;
        filters(1, 1, 0) = 1.0f;
        filters(1, 1, 1) = 1.0f;

        matrix filter_bias = make_matrix("1, -2");
        matrix filterbank;
        filterbank.conv2_filterbank(image, filters, filter_bias, matrix::convolution_padding::valid);
        matrix expected_filterbank(std::vector<int>{2, 2, 2});
        expected_filterbank(0, 0, 0) = -3.0f;
        expected_filterbank(0, 0, 1) = -3.0f;
        expected_filterbank(0, 1, 0) = -3.0f;
        expected_filterbank(0, 1, 1) = -3.0f;
        expected_filterbank(1, 0, 0) = 10.0f;
        expected_filterbank(1, 0, 1) = 14.0f;
        expected_filterbank(1, 1, 0) = 22.0f;
        expected_filterbank(1, 1, 1) = 26.0f;
        require_matrix_close(filterbank, expected_filterbank, "conv2_filterbank()");

        matrix dY(std::vector<int>{2, 2, 2});
        for(int y = 0; y < 2; ++y)
            for(int x = 0; x < 2; ++x)
            {
                dY(0, y, x) = 1.0f;
                dY(1, y, x) = 2.0f;
            }

        matrix d_filters;
        d_filters.conv2_filterbank_backward_filters(image, dY, 2, 2, matrix::convolution_padding::valid);
        matrix expected_d_filters(std::vector<int>{2, 2, 2});
        expected_d_filters(0, 0, 0) = 12.0f;
        expected_d_filters(0, 0, 1) = 16.0f;
        expected_d_filters(0, 1, 0) = 24.0f;
        expected_d_filters(0, 1, 1) = 28.0f;
        expected_d_filters(1, 0, 0) = 24.0f;
        expected_d_filters(1, 0, 1) = 32.0f;
        expected_d_filters(1, 1, 0) = 48.0f;
        expected_d_filters(1, 1, 1) = 56.0f;
        require_matrix_close(d_filters, expected_d_filters, "conv2_filterbank_backward_filters()");

        matrix pre_activation(std::vector<int>{2, 2, 2});
        pre_activation.set(1.0f);
        pre_activation(0, 0, 0) = -1.0f;
        pre_activation(1, 1, 1) = -1.0f;

        matrix d_filters_relu;
        d_filters_relu.conv2_filterbank_backward_filters_relu(image, dY, pre_activation, 2, 2, matrix::convolution_padding::valid);
        matrix expected_d_filters_relu(std::vector<int>{2, 2, 2});
        expected_d_filters_relu(0, 0, 0) = 11.0f;
        expected_d_filters_relu(0, 0, 1) = 14.0f;
        expected_d_filters_relu(0, 1, 0) = 20.0f;
        expected_d_filters_relu(0, 1, 1) = 23.0f;
        expected_d_filters_relu(1, 0, 0) = 14.0f;
        expected_d_filters_relu(1, 0, 1) = 20.0f;
        expected_d_filters_relu(1, 1, 0) = 32.0f;
        expected_d_filters_relu(1, 1, 1) = 38.0f;
        require_matrix_close(d_filters_relu, expected_d_filters_relu, "conv2_filterbank_backward_filters_relu()");

        matrix one_filter(std::vector<int>{1, 2, 2});
        one_filter(0, 0, 0) = 1.0f;
        one_filter(0, 0, 1) = 2.0f;
        one_filter(0, 1, 0) = 3.0f;
        one_filter(0, 1, 1) = 4.0f;
        matrix one_dY(std::vector<int>{1, 2, 2});
        one_dY.set(1.0f);
        matrix d_input;
        d_input.conv2_filterbank_backward_input(one_dY, one_filter, matrix::convolution_padding::valid);
        require_matrix_close(d_input, make_matrix("1, 3, 2; 4, 10, 6; 3, 7, 4"), "conv2_filterbank_backward_input()");

        matrix same_padding_output;
        same_padding_output.conv2_filterbank(image, filters, filter_bias, matrix::convolution_padding::same);
        matrix expected_same_padding_output(std::vector<int>{2, 3, 3});
        expected_same_padding_output(0, 0, 0) = -3.0f;
        expected_same_padding_output(0, 0, 1) = -3.0f;
        expected_same_padding_output(0, 0, 2) = 4.0f;
        expected_same_padding_output(0, 1, 0) = -3.0f;
        expected_same_padding_output(0, 1, 1) = -3.0f;
        expected_same_padding_output(0, 1, 2) = 7.0f;
        expected_same_padding_output(0, 2, 0) = 8.0f;
        expected_same_padding_output(0, 2, 1) = 9.0f;
        expected_same_padding_output(0, 2, 2) = 10.0f;
        expected_same_padding_output(1, 0, 0) = 10.0f;
        expected_same_padding_output(1, 0, 1) = 14.0f;
        expected_same_padding_output(1, 0, 2) = 7.0f;
        expected_same_padding_output(1, 1, 0) = 22.0f;
        expected_same_padding_output(1, 1, 1) = 26.0f;
        expected_same_padding_output(1, 1, 2) = 13.0f;
        expected_same_padding_output(1, 2, 0) = 13.0f;
        expected_same_padding_output(1, 2, 1) = 15.0f;
        expected_same_padding_output(1, 2, 2) = 7.0f;
        require_matrix_close(same_padding_output, expected_same_padding_output, "conv2_filterbank() same padding");

        matrix same_dY(std::vector<int>{2, 3, 3});
        for(int y = 0; y < 3; ++y)
            for(int x = 0; x < 3; ++x)
            {
                same_dY(0, y, x) = 1.0f;
                same_dY(1, y, x) = 2.0f;
            }
        matrix same_pre_activation(std::vector<int>{2, 3, 3});
        same_pre_activation.set(1.0f);
        same_pre_activation(0, 0, 0) = -1.0f;
        same_pre_activation(1, 1, 1) = -1.0f;

        matrix same_d_filters;
        same_d_filters.conv2_filterbank_backward_filters(image, same_dY, 2, 2, matrix::convolution_padding::same);
        matrix expected_same_d_filters(std::vector<int>{2, 2, 2});
        expected_same_d_filters(0, 0, 0) = 45.0f;
        expected_same_d_filters(0, 0, 1) = 33.0f;
        expected_same_d_filters(0, 1, 0) = 39.0f;
        expected_same_d_filters(0, 1, 1) = 28.0f;
        expected_same_d_filters(1, 0, 0) = 90.0f;
        expected_same_d_filters(1, 0, 1) = 66.0f;
        expected_same_d_filters(1, 1, 0) = 78.0f;
        expected_same_d_filters(1, 1, 1) = 56.0f;
        require_matrix_close(same_d_filters, expected_same_d_filters, "conv2_filterbank_backward_filters() same padding");

        matrix same_d_filters_relu;
        same_d_filters_relu.conv2_filterbank_backward_filters_relu(image, same_dY, same_pre_activation, 2, 2, matrix::convolution_padding::same);
        matrix expected_same_d_filters_relu(std::vector<int>{2, 2, 2});
        expected_same_d_filters_relu(0, 0, 0) = 44.0f;
        expected_same_d_filters_relu(0, 0, 1) = 31.0f;
        expected_same_d_filters_relu(0, 1, 0) = 35.0f;
        expected_same_d_filters_relu(0, 1, 1) = 23.0f;
        expected_same_d_filters_relu(1, 0, 0) = 80.0f;
        expected_same_d_filters_relu(1, 0, 1) = 54.0f;
        expected_same_d_filters_relu(1, 1, 0) = 62.0f;
        expected_same_d_filters_relu(1, 1, 1) = 38.0f;
        require_matrix_close(same_d_filters_relu, expected_same_d_filters_relu, "conv2_filterbank_backward_filters_relu() same padding");

        matrix same_d_input;
        same_d_input.conv2_filterbank_backward_input(same_dY, filters, matrix::convolution_padding::same);
        require_matrix_close(same_d_input, make_matrix("3, 5, 5; 5, 8, 8; 5, 8, 8"), "conv2_filterbank_backward_input() same padding");

        matrix reduced_last;
        reduced_last.sum_last_two_dimensions(dY);
        require_matrix_close(reduced_last, make_matrix("4, 8"), "sum_last_two_dimensions()");

        matrix reduced_last_relu;
        reduced_last_relu.sum_last_two_dimensions_relu(dY, pre_activation);
        require_matrix_close(reduced_last_relu, make_matrix("3, 6"), "sum_last_two_dimensions_relu()");

        matrix channel_input(std::vector<int>{2, 3, 3});
        for(int y = 0; y < 3; ++y)
            for(int x = 0; x < 3; ++x)
            {
                channel_input(0, y, x) = static_cast<float>(y * 3 + x + 1);
                channel_input(1, y, x) = static_cast<float>(y * 3 + x + 10);
            }

        matrix channel_filters(std::vector<int>{2, 2, 2, 2});
        channel_filters.reset();
        for(int ky = 0; ky < 2; ++ky)
            for(int kx = 0; kx < 2; ++kx)
                channel_filters(0, 0, ky, kx) = 1.0f;
        channel_filters(1, 1, 0, 0) = 1.0f;
        channel_filters(1, 0, 1, 1) = -1.0f;

        matrix channel_bias = make_matrix("0.5, -1");
        matrix channel_output;
        channel_output.conv2_channel_filterbank(channel_input, channel_filters, channel_bias, matrix::convolution_padding::valid);
        matrix expected_channel_output(std::vector<int>{2, 2, 2});
        expected_channel_output(0, 0, 0) = 12.5f;
        expected_channel_output(0, 0, 1) = 16.5f;
        expected_channel_output(0, 1, 0) = 24.5f;
        expected_channel_output(0, 1, 1) = 28.5f;
        expected_channel_output(1, 0, 0) = 4.0f;
        expected_channel_output(1, 0, 1) = 4.0f;
        expected_channel_output(1, 1, 0) = 4.0f;
        expected_channel_output(1, 1, 1) = 4.0f;
        require_matrix_close(channel_output, expected_channel_output, "conv2_channel_filterbank()");

        matrix channel_dY(std::vector<int>{2, 2, 2});
        for(int y = 0; y < 2; ++y)
            for(int x = 0; x < 2; ++x)
            {
                channel_dY(0, y, x) = 1.0f;
                channel_dY(1, y, x) = 2.0f;
            }

        matrix channel_dK;
        channel_dK.conv2_channel_filterbank_backward_filters(channel_input, channel_dY, 2, 2, matrix::convolution_padding::valid);
        matrix expected_channel_dK(std::vector<int>{2, 2, 2, 2});
        expected_channel_dK(0, 0, 0, 0) = 12.0f;
        expected_channel_dK(0, 0, 0, 1) = 16.0f;
        expected_channel_dK(0, 0, 1, 0) = 24.0f;
        expected_channel_dK(0, 0, 1, 1) = 28.0f;
        expected_channel_dK(0, 1, 0, 0) = 48.0f;
        expected_channel_dK(0, 1, 0, 1) = 52.0f;
        expected_channel_dK(0, 1, 1, 0) = 60.0f;
        expected_channel_dK(0, 1, 1, 1) = 64.0f;
        expected_channel_dK(1, 0, 0, 0) = 24.0f;
        expected_channel_dK(1, 0, 0, 1) = 32.0f;
        expected_channel_dK(1, 0, 1, 0) = 48.0f;
        expected_channel_dK(1, 0, 1, 1) = 56.0f;
        expected_channel_dK(1, 1, 0, 0) = 96.0f;
        expected_channel_dK(1, 1, 0, 1) = 104.0f;
        expected_channel_dK(1, 1, 1, 0) = 120.0f;
        expected_channel_dK(1, 1, 1, 1) = 128.0f;
        require_matrix_close(channel_dK, expected_channel_dK, "conv2_channel_filterbank_backward_filters()");

        matrix channel_dInput;
        channel_dInput.conv2_channel_filterbank_backward_input(channel_dY, channel_filters, matrix::convolution_padding::valid);
        matrix expected_channel_dInput(std::vector<int>{2, 3, 3});
        expected_channel_dInput(0, 0, 0) = 1.0f;
        expected_channel_dInput(0, 0, 1) = 2.0f;
        expected_channel_dInput(0, 1, 0) = 2.0f;
        expected_channel_dInput(0, 1, 1) = 2.0f;
        expected_channel_dInput(0, 2, 0) = 1.0f;
        expected_channel_dInput(0, 2, 1) = 0.0f;
        expected_channel_dInput(1, 0, 0) = 2.0f;
        expected_channel_dInput(1, 0, 1) = 2.0f;
        expected_channel_dInput(1, 1, 0) = 2.0f;
        expected_channel_dInput(1, 1, 1) = 2.0f;
        expected_channel_dInput(1, 2, 0) = 0.0f;
        expected_channel_dInput(1, 2, 1) = 0.0f;
        expected_channel_dInput(0, 0, 2) = 1.0f;
        expected_channel_dInput(0, 1, 2) = 0.0f;
        expected_channel_dInput(0, 2, 2) = -1.0f;
        expected_channel_dInput(1, 0, 2) = 0.0f;
        expected_channel_dInput(1, 1, 2) = 0.0f;
        expected_channel_dInput(1, 2, 2) = 0.0f;
        require_matrix_close(channel_dInput, expected_channel_dInput, "conv2_channel_filterbank_backward_input()");

        matrix same_padding_channel_output;
        same_padding_channel_output.conv2_channel_filterbank(channel_input, channel_filters, channel_bias, matrix::convolution_padding::same);
        matrix expected_same_padding_channel_output(std::vector<int>{2, 3, 3});
        expected_same_padding_channel_output(0, 0, 0) = 12.5f;
        expected_same_padding_channel_output(0, 0, 1) = 16.5f;
        expected_same_padding_channel_output(0, 0, 2) = 9.5f;
        expected_same_padding_channel_output(0, 1, 0) = 24.5f;
        expected_same_padding_channel_output(0, 1, 1) = 28.5f;
        expected_same_padding_channel_output(0, 1, 2) = 15.5f;
        expected_same_padding_channel_output(0, 2, 0) = 15.5f;
        expected_same_padding_channel_output(0, 2, 1) = 17.5f;
        expected_same_padding_channel_output(0, 2, 2) = 9.5f;
        expected_same_padding_channel_output(1, 0, 0) = 4.0f;
        expected_same_padding_channel_output(1, 0, 1) = 4.0f;
        expected_same_padding_channel_output(1, 0, 2) = 11.0f;
        expected_same_padding_channel_output(1, 1, 0) = 4.0f;
        expected_same_padding_channel_output(1, 1, 1) = 4.0f;
        expected_same_padding_channel_output(1, 1, 2) = 14.0f;
        expected_same_padding_channel_output(1, 2, 0) = 15.0f;
        expected_same_padding_channel_output(1, 2, 1) = 16.0f;
        expected_same_padding_channel_output(1, 2, 2) = 17.0f;
        require_matrix_close(same_padding_channel_output, expected_same_padding_channel_output, "conv2_channel_filterbank() same padding");

        matrix same_channel_dY(std::vector<int>{2, 3, 3});
        for(int y = 0; y < 3; ++y)
            for(int x = 0; x < 3; ++x)
            {
                same_channel_dY(0, y, x) = 1.0f;
                same_channel_dY(1, y, x) = 2.0f;
            }

        matrix same_channel_dK;
        same_channel_dK.conv2_channel_filterbank_backward_filters(channel_input, same_channel_dY, 2, 2, matrix::convolution_padding::same);
        matrix expected_same_channel_dK(std::vector<int>{2, 2, 2, 2});
        expected_same_channel_dK(0, 0, 0, 0) = 45.0f;
        expected_same_channel_dK(0, 0, 0, 1) = 33.0f;
        expected_same_channel_dK(0, 0, 1, 0) = 39.0f;
        expected_same_channel_dK(0, 0, 1, 1) = 28.0f;
        expected_same_channel_dK(0, 1, 0, 0) = 126.0f;
        expected_same_channel_dK(0, 1, 0, 1) = 87.0f;
        expected_same_channel_dK(0, 1, 1, 0) = 93.0f;
        expected_same_channel_dK(0, 1, 1, 1) = 64.0f;
        expected_same_channel_dK(1, 0, 0, 0) = 90.0f;
        expected_same_channel_dK(1, 0, 0, 1) = 66.0f;
        expected_same_channel_dK(1, 0, 1, 0) = 78.0f;
        expected_same_channel_dK(1, 0, 1, 1) = 56.0f;
        expected_same_channel_dK(1, 1, 0, 0) = 252.0f;
        expected_same_channel_dK(1, 1, 0, 1) = 174.0f;
        expected_same_channel_dK(1, 1, 1, 0) = 186.0f;
        expected_same_channel_dK(1, 1, 1, 1) = 128.0f;
        require_matrix_close(same_channel_dK, expected_same_channel_dK, "conv2_channel_filterbank_backward_filters() same padding");

        matrix same_channel_dInput;
        same_channel_dInput.conv2_channel_filterbank_backward_input(same_channel_dY, channel_filters, matrix::convolution_padding::same);
        matrix expected_same_channel_dInput(std::vector<int>{2, 3, 3});
        expected_same_channel_dInput(0, 0, 0) = 1.0f;
        expected_same_channel_dInput(0, 0, 1) = 2.0f;
        expected_same_channel_dInput(0, 0, 2) = 2.0f;
        expected_same_channel_dInput(0, 1, 0) = 2.0f;
        expected_same_channel_dInput(0, 1, 1) = 2.0f;
        expected_same_channel_dInput(0, 1, 2) = 2.0f;
        expected_same_channel_dInput(0, 2, 0) = 2.0f;
        expected_same_channel_dInput(0, 2, 1) = 2.0f;
        expected_same_channel_dInput(0, 2, 2) = 2.0f;
        expected_same_channel_dInput(1, 0, 0) = 2.0f;
        expected_same_channel_dInput(1, 0, 1) = 2.0f;
        expected_same_channel_dInput(1, 0, 2) = 2.0f;
        expected_same_channel_dInput(1, 1, 0) = 2.0f;
        expected_same_channel_dInput(1, 1, 1) = 2.0f;
        expected_same_channel_dInput(1, 1, 2) = 2.0f;
        expected_same_channel_dInput(1, 2, 0) = 2.0f;
        expected_same_channel_dInput(1, 2, 1) = 2.0f;
        expected_same_channel_dInput(1, 2, 2) = 2.0f;
        require_matrix_close(same_channel_dInput, expected_same_channel_dInput, "conv2_channel_filterbank_backward_input() same padding");

        matrix fused_same_channel_dInput;
        matrix fused_same_channel_dK;
        matrix fused_same_channel_dB;
        fused_same_channel_dInput.conv2_channel_filterbank_backward(channel_input, channel_filters, same_channel_dY, fused_same_channel_dK, fused_same_channel_dB, matrix::convolution_padding::same);
        require_matrix_close(fused_same_channel_dInput, expected_same_channel_dInput, "conv2_channel_filterbank_backward() same padding dI");
        require_matrix_close(fused_same_channel_dK, expected_same_channel_dK, "conv2_channel_filterbank_backward() same padding dK");
        require_matrix_close(fused_same_channel_dB, make_matrix("9, 18"), "conv2_channel_filterbank_backward() same padding dB");

        matrix fused_channel_dInput;
        matrix fused_channel_dK;
        matrix fused_channel_dB;
        fused_channel_dInput.conv2_channel_filterbank_backward(channel_input, channel_filters, channel_dY, fused_channel_dK, fused_channel_dB, matrix::convolution_padding::valid);
        require_matrix_close(fused_channel_dInput, expected_channel_dInput, "conv2_channel_filterbank_backward() dI");
        require_matrix_close(fused_channel_dK, expected_channel_dK, "conv2_channel_filterbank_backward() dK");
        require_matrix_close(fused_channel_dB, make_matrix("4, 8"), "conv2_channel_filterbank_backward() dB");

        matrix aliased_correlation_input = make_matrix("1, 2; 3, 4");
        matrix aliased_correlation_output = aliased_correlation_input;
        require_throws_as<std::invalid_argument>(
            [&]()
            {
                aliased_correlation_output.corr2(
                    aliased_correlation_input,
                    make_matrix("2"),
                    matrix::convolution_padding::same
                );
            },
            "corr2() rejects a shallow output alias"
        );
        require_matrix_close(aliased_correlation_input, make_matrix("1, 2; 3, 4"),
                             "corr2() alias rejection occurs before writing");

        matrix aliased_convolution_input = image.clone();
        matrix aliased_convolution_output = aliased_convolution_input;
        require_throws_as<std::invalid_argument>(
            [&]() { aliased_convolution_output.conv2(aliased_convolution_input, kernel); },
            "conv2() rejects a shallow output alias"
        );
        require_matrix_close(aliased_convolution_input, image,
                             "conv2() alias rejection occurs before writing");

        matrix slow_convolution_alias_parent(std::vector<int>{2, 3, 3});
        fill_sequence(slow_convolution_alias_parent);
        matrix slow_convolution_alias_input = slow_convolution_alias_parent[0];
        matrix slow_convolution_alias_output = slow_convolution_alias_parent[0];
        slow_convolution_alias_output.resize(2, 2);
        matrix original_slow_convolution_alias_parent = slow_convolution_alias_parent.clone();
        require_throws_as<std::invalid_argument>(
            [&]() { slow_convolution_alias_output.conv2_slow(slow_convolution_alias_input, kernel); },
            "conv2_slow() rejects overlapping independently-created views"
        );
        require_matrix_close(slow_convolution_alias_parent,
                             original_slow_convolution_alias_parent,
                             "conv2_slow() alias rejection occurs before writing");

        matrix shared_backward_output;
        matrix shared_backward_dK = shared_backward_output;
        matrix independent_backward_dB;
        require_throws_as<std::invalid_argument>(
            [&]()
            {
                shared_backward_output.conv2_channel_filterbank_backward(
                    channel_input,
                    channel_filters,
                    channel_dY,
                    shared_backward_dK,
                    independent_backward_dB,
                    matrix::convolution_padding::valid
                );
            },
            "channel filter-bank backward rejects shared output storage"
        );

        matrix small = make_matrix("1, 2; 3, 4");
        matrix up;
        up.upsample(small);
        require_matrix_close(up, make_matrix("1, 1, 2, 2; 1, 1, 2, 2; 3, 3, 4, 4; 3, 3, 4, 4"), "upsample()");

        matrix row_gapped_small(2, 4);
        row_gapped_small.set(-1000.0f);
        row_gapped_small.resize(2, 2);
        row_gapped_small.copy(small);
        matrix row_gapped_up(4, 6);
        row_gapped_up.set(-1000.0f);
        row_gapped_up.resize(4, 4);
        row_gapped_up.upsample(row_gapped_small);
        require_matrix_close(row_gapped_up, up, "upsample() row-gapped source and destination");

        matrix upsample_alias_parent(std::vector<int>{2, 4, 4});
        fill_sequence(upsample_alias_parent);
        matrix overlapping_upsample_output = upsample_alias_parent[0];
        matrix overlapping_upsample_source = upsample_alias_parent[0];
        overlapping_upsample_source.resize(2, 2);
        matrix original_upsample_alias_parent = upsample_alias_parent.clone();
        require_throws_as<std::invalid_argument>(
            [&]() { overlapping_upsample_output.upsample(overlapping_upsample_source); },
            "upsample() rejects overlapping independently-created views"
        );
        require_matrix_close(upsample_alias_parent, original_upsample_alias_parent,
                             "upsample() alias rejection occurs before writing");

        matrix zero_width_upsample;
        zero_width_upsample.upsample(matrix(2, 0));
        require_shape(zero_width_upsample, {4, 0}, "upsample() zero-width source");
        matrix invalid_upsample_source(2);
        require_throws([&]() { up.upsample(invalid_upsample_source); },
                       "upsample() should reject a non-two-dimensional source");

        matrix oversized_upsample_source;
        oversized_upsample_source.reserve(std::numeric_limits<int>::max(), 0);
        oversized_upsample_source.resize(std::numeric_limits<int>::max(), 0);
        matrix oversized_upsample_destination;
        require_throws([&]() { oversized_upsample_destination.upsample(oversized_upsample_source); },
                       "upsample() should reject overflowing result dimensions");
        require_true(oversized_upsample_destination.is_uninitialized(),
                     "oversized upsample() leaves its destination unchanged");

        matrix reflected = make_matrix("0, 0, 0, 0; 0, 1, 2, 0; 0, 3, 4, 0; 0, 0, 0, 0");
        reflected.fillReflect101Border(1, 1);
        require_matrix_close(reflected, make_matrix("4, 3, 4, 3; 2, 1, 2, 1; 4, 3, 4, 3; 2, 1, 2, 1"), "fillReflect101Border()");

        matrix row_gapped_reflected(4, 6);
        row_gapped_reflected.set(-1000.0f);
        row_gapped_reflected.resize(4, 4);
        row_gapped_reflected.copy(make_matrix("0, 0, 0, 0; 0, 1, 2, 0; 0, 3, 4, 0; 0, 0, 0, 0"));
        row_gapped_reflected.fillReflect101Border(1, 1);
        require_matrix_close(row_gapped_reflected, reflected,
                             "fillReflect101Border() row-gapped matrix");

        matrix extended = make_matrix("0, 0, 0, 0; 0, 1, 2, 0; 0, 3, 4, 0; 0, 0, 0, 0");
        extended.fillExtendBorder(1, 1);
        require_matrix_close(extended, make_matrix("1, 1, 2, 2; 1, 1, 2, 2; 3, 3, 4, 4; 3, 3, 4, 4"), "fillExtendBorder()");

        matrix row_gapped_extended(4, 6);
        row_gapped_extended.set(-1000.0f);
        row_gapped_extended.resize(4, 4);
        row_gapped_extended.copy(make_matrix("0, 0, 0, 0; 0, 1, 2, 0; 0, 3, 4, 0; 0, 0, 0, 0"));
        row_gapped_extended.fillExtendBorder(1, 1);
        require_matrix_close(row_gapped_extended, extended,
                             "fillExtendBorder() row-gapped matrix");

        matrix invalid_border = make_matrix("0, 0, 0; 0, 7, 0; 0, 0, 0");
        require_throws([&]() { invalid_border.fillReflect101Border(-1, 1); },
                       "fillReflect101Border() should reject negative widths");
        require_throws([&]() { invalid_border.fillExtendBorder(2, 1); },
                       "fillExtendBorder() should require a positive inner image");
        require_throws([&]() { invalid_border.fillReflect101Border(1, 1); },
                       "fillReflect101Border() should reject a repeated-reflection border");
        require_matrix_close(invalid_border, make_matrix("0, 0, 0; 0, 7, 0; 0, 0, 0"),
                             "invalid border fills leave the matrix unchanged");
        matrix invalid_border_rank(3);
        require_throws([&]() { invalid_border_rank.fillExtendBorder(0, 0); },
                       "border fill should reject a non-two-dimensional matrix");
    }

    void test_submatrix_views()
    {
        matrix base = make_matrix("1, 2, 3; 4, 5, 6; 7, 8, 9");

        matrix row = base[1];
        row.set(99.0f);
        require_matrix_close(base, make_matrix("1, 2, 3; 99, 99, 99; 7, 8, 9"), "row.set() honors offset");

        matrix source = make_matrix("1, 2, 3; 4, 5, 6; 7, 8, 9");
        matrix target = make_matrix("10, 11, 12; 13, 14, 15; 16, 17, 18");
        target[2].copy(source[0]);
        require_matrix_close(target, make_matrix("10, 11, 12; 13, 14, 15; 1, 2, 3"), "row.copy() honors offset");

        matrix a = make_matrix("1, 2, 3; 4, 5, 6; 7, 8, 9");
        matrix b = make_matrix("10, 20, 30; 40, 50, 60; 70, 80, 90");
        matrix add_result = make_matrix("100, 101, 102; 103, 104, 105; 106, 107, 108");
        add_result[1].add(a[2], b[0]);
        require_matrix_close(add_result, make_matrix("100, 101, 102; 17, 28, 39; 106, 107, 108"), "add(row, row) honors offset");

        matrix sub_result = make_matrix("100, 101, 102; 103, 104, 105; 106, 107, 108");
        sub_result[0].subtract(b[2], a[1]);
        require_matrix_close(sub_result, make_matrix("66, 75, 84; 103, 104, 105; 106, 107, 108"), "subtract(row, row) honors offset");

        matrix mul_result = make_matrix("100, 101, 102; 103, 104, 105; 106, 107, 108");
        mul_result[2].multiply(a[0], b[1]);
        require_matrix_close(mul_result, make_matrix("100, 101, 102; 103, 104, 105; 40, 100, 180"), "multiply(row, row) honors offset");

        matrix div_result = make_matrix("100, 101, 102; 103, 104, 105; 106, 107, 108");
        div_result[1].divide(b[2], a[0]);
        require_matrix_close(div_result, make_matrix("100, 101, 102; 70, 40, 30; 106, 107, 108"), "divide(row, row) honors offset");
    }

    void test_matrix_hotspots()
    {
        matrix tensor(std::vector<int>{2, 3, 4});
        fill_sequence(tensor);
        tensor.set_labels(0, "front", "back");

        matrix exact_alias_source = make_matrix("1, 2, 3");
        matrix exact_alias_output = exact_alias_source;
        exact_alias_output.add(exact_alias_source, make_matrix("4, 5, 6"));
        require_matrix_close(exact_alias_source, make_matrix("5, 7, 9"),
                             "element-wise output accepts an exact-layout shallow alias");

        matrix aliased_pre_activation = make_matrix("-1, 2");
        matrix aliased_relu_output = aliased_pre_activation;
        require_throws_as<std::invalid_argument>(
            [&]() { aliased_relu_output.relu_backward(make_matrix("3, 4"), aliased_pre_activation); },
            "relu_backward() rejects a shallow pre-activation alias"
        );
        require_matrix_close(aliased_pre_activation, make_matrix("-1, 2"),
                             "relu_backward() alias rejection occurs before writing");

        matrix subview_alias_parent = make_matrix("-1, 2; 7, 8");
        matrix subview_pre_activation = subview_alias_parent[0];
        matrix subview_relu_output = subview_alias_parent[0];
        require_throws_as<std::invalid_argument>(
            [&]() { subview_relu_output.relu_backward(make_matrix("3, 4"), subview_pre_activation); },
            "relu_backward() rejects overlapping independently-created subviews"
        );
        require_matrix_close(subview_alias_parent, make_matrix("-1, 2; 7, 8"),
                             "subview alias rejection occurs before writing");

        matrix shared_latent_output;
        matrix shared_log_variance_output = shared_latent_output;
        require_throws_as<std::invalid_argument>(
            [&]()
            {
                shared_latent_output.latent_mean_gradients(
                    shared_log_variance_output,
                    make_matrix("1, 2"),
                    make_matrix("3, 4"),
                    make_matrix("0, 0"),
                    0.1f
                );
            },
            "latent gradients reject shared output storage"
        );

        matrix aliased_adam_values = make_matrix("1, 2");
        matrix aliased_first_moment = aliased_adam_values;
        matrix independent_second_moment(2);
        require_throws_as<std::invalid_argument>(
            [&]()
            {
                aliased_adam_values.adam_update(
                    make_matrix("0.1, 0.2"),
                    aliased_first_moment,
                    independent_second_moment,
                    0.001f,
                    0.9f,
                    0.999f,
                    0.1f,
                    0.001f,
                    1e-8f
                );
            },
            "adam_update() rejects shared parameter and moment storage"
        );
        require_matrix_close(aliased_adam_values, make_matrix("1, 2"),
                             "adam_update() alias rejection occurs before writing");

        const matrix & const_tensor = tensor;
        require_equal(const_tensor.labels(0).at(0), "front", "const labels() first label");
        require_equal(const_tensor.labels(0).at(1), "back", "const labels() second label");

        matrix front = tensor[0];
        matrix back = tensor["back"];
        auto const_front = const_tensor[0];
        const std::string back_label = "back";
        auto const_back = const_tensor[back_label];
        auto const_back_char = const_tensor["back"];

        static_assert(std::is_same_v<decltype(const_front), const_matrix_view>);
        static_assert(std::is_same_v<decltype(const_tensor.begin()), matrix::const_iterator>);
        static_assert(std::is_same_v<
            typename std::iterator_traits<matrix::iterator>::iterator_category,
            std::input_iterator_tag
        >);
        static_assert(std::is_same_v<
            typename std::iterator_traits<matrix::iterator>::reference,
            matrix
        >);
        static_assert(std::is_same_v<
            typename std::iterator_traits<matrix::const_iterator>::reference,
            const_matrix_view
        >);
        static_assert(std::is_same_v<
            typename std::iterator_traits<const_matrix_view::const_iterator>::difference_type,
            std::ptrdiff_t
        >);
        static_assert(std::is_same_v<decltype(const_front(0, 0)), const float &>);
        static_assert(!std::is_assignable_v<decltype(const_front(0, 0)), float>);
        static_assert(!std::is_constructible_v<matrix, const_matrix_view>);
        static_assert(!has_public_data_member<matrix>::value);
        static_assert(!has_public_info_member<matrix>::value);
        static_assert(!has_public_last_member<matrix>::value);
        static_assert(!has_public_row_pointers_member<matrix>::value);

        require_shape(front, {3, 4}, "rank-3 slice shape");
        require_close(front.sum(), 78.0f, "slice sum()");
        require_close(back.sum(), 222.0f, "labeled slice sum()");
        require_close(const_front.sum(), 78.0f, "const numeric slice sum()");
        require_close(const_back.sum(), 222.0f, "const labeled slice sum()");
        require_close(const_back_char.sum(), 222.0f, "const labeled char slice sum()");
        require_close((*const_tensor.begin()).sum(), 78.0f, "const iterator returns a read-only slice");
        require_true(std::distance(const_tensor.begin(), const_tensor.end()) == 2,
                     "const iterators work with standard algorithms");
        const float iterated_sum = std::accumulate(
            const_tensor.begin(),
            const_tensor.end(),
            0.0f,
            [](float sum, const_matrix_view slice) { return sum + slice.sum(); }
        );
        require_close(iterated_sum, tensor.sum(), "const iterator standard-algorithm traversal");
        require_close(dot(front, back), 1586.0f, "dot(slice, slice)");
        require_close(dot(const_front, const_back), 1586.0f, "dot(const slice, const slice)");

        matrix mutable_iteration = make_matrix("1, 2; 3, 4");
        std::for_each(mutable_iteration.begin(), mutable_iteration.end(),
                      [](matrix slice) { slice.add(1.0f); });
        require_matrix_close(mutable_iteration, make_matrix("2, 3; 4, 5"),
                             "mutable iterator standard-algorithm traversal");

        matrix rank_zero_iteration;
        require_true(rank_zero_iteration.begin() == rank_zero_iteration.end(),
                     "uninitialized rank-zero matrix has no slices");
        rank_zero_iteration.realloc(std::vector<int>{});
        require_true(rank_zero_iteration.begin() == rank_zero_iteration.end(),
                     "initialized rank-zero scalar has no first-axis slices");

        matrix zero_first_dimension(0, 3);
        require_true(zero_first_dimension.begin() == zero_first_dimension.end(),
                     "zero first dimension has no slices");
        matrix zero_inner_dimension(2, 0);
        require_true(std::distance(zero_inner_dimension.begin(), zero_inner_dimension.end()) == 2,
                     "zero inner dimension retains its first-axis slices");
        require_true((*zero_inner_dimension.begin()).empty(),
                     "zero inner-dimension iterator yields an empty slice");

        matrix unrelated_iteration_a(1, 1);
        matrix unrelated_iteration_b(1, 1);
        require_true(unrelated_iteration_a.begin() != unrelated_iteration_b.begin(),
                     "iterators from different matrices do not compare equal");

        const matrix const_rank_one = make_matrix("1, 2");
        const_matrix_view const_scalar_view = const_rank_one[0];
        require_true(const_scalar_view.begin() == const_scalar_view.end(),
                     "rank-zero const view has no slices");

        matrix combined;
        combined.copy(front);
        combined.add(back);
        require_matrix_close(combined, make_matrix("14, 16, 18, 20; 22, 24, 26, 28; 30, 32, 34, 36"), "copy()/add() on slices");

        matrix scaled;
        scaled.copy(front);
        scaled.scale(2.0f);
        require_matrix_close(scaled, make_matrix("2, 4, 6, 8; 10, 12, 14, 16; 18, 20, 22, 24"), "scale() on copied slice");

        matrix row = front[1];
        require_equal(row.json(), "[5, 6, 7, 8]", "json() on row slice");
        require_equal(row.csv(), "5.000000,6.000000,7.000000,8.000000\n", "csv() on row slice");
        require_equal(front.json(), "[[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12]]", "json() on matrix slice");

        back.add(1.0f);
        require_close(tensor[1].sum(), 234.0f, "labeled slice mutation updates parent");

        matrix reduced;
        reduced.sum_last_two_dimensions(tensor);
        require_matrix_close(reduced, make_matrix("78, 234"), "sum_last_two_dimensions() after slice mutation");

        matrix strided_source(2, 4);
        fill_sequence(strided_source);
        strided_source.resize(2, 2);
        require_true(strided_source.size() == 4, "size() is product of logical shape after resize()");
        matrix strided_target(2, 4);
        strided_target.resize(2, 2);
        strided_target.copy(strided_source);
        require_matrix_close(strided_target, make_matrix("1, 2; 5, 6"), "copy() row blocks with logical row smaller than stride");
        require_close(strided_source.sum(), 14.0f, "sum() skips row gaps after resize()");
        require_close(strided_source.average(), 3.5f, "average() uses logical size after resize()");
        require_close(dot(strided_source, strided_source), 66.0f, "dot() skips row gaps after resize()");

        matrix strided_scaled;
        strided_scaled.copy(strided_source);
        strided_scaled.add(1.0f);
        require_matrix_close(strided_scaled, make_matrix("2, 3; 6, 7"), "add(float) skips row gaps after resize()");
        strided_scaled.scale(2.0f);
        require_matrix_close(strided_scaled, make_matrix("4, 6; 12, 14"), "scale(float) skips row gaps after resize()");

        matrix strided_added;
        strided_added.copy(strided_source);
        strided_added.add(strided_source);
        require_matrix_close(strided_added, make_matrix("2, 4; 10, 12"), "add(matrix) skips row gaps after resize()");
        strided_added.set(3.0f);
        require_matrix_close(strided_added, make_matrix("3, 3; 3, 3"), "set() skips row gaps after resize()");
        strided_added.clear();
        require_true(strided_added.size() == 0, "size() is zero after clear()");

        matrix strided_tensor_source(std::vector<int>{2, 3, 4});
        fill_sequence(strided_tensor_source);
        strided_tensor_source.resize({2, 2, 2});
        matrix expected_tensor(std::vector<int>{2, 2, 2});
        expected_tensor(0, 0, 0) = 1.0f;
        expected_tensor(0, 0, 1) = 2.0f;
        expected_tensor(0, 1, 0) = 5.0f;
        expected_tensor(0, 1, 1) = 6.0f;
        expected_tensor(1, 0, 0) = 13.0f;
        expected_tensor(1, 0, 1) = 14.0f;
        expected_tensor(1, 1, 0) = 17.0f;
        expected_tensor(1, 1, 1) = 18.0f;

        matrix strided_tensor_target(std::vector<int>{2, 4, 5});
        strided_tensor_target.set(100.0f);
        strided_tensor_target.resize({2, 2, 2});
        strided_tensor_target.copy(strided_tensor_source);
        require_matrix_close(strided_tensor_target, expected_tensor, "copy() rank-3 row blocks with different strides");
        require_true(strided_tensor_target == expected_tensor, "equality compares rank-3 row blocks");
        require_true(expected_tensor == strided_tensor_target, "equality compares rank-3 row blocks in either direction");
        require_close(strided_tensor_target.sum(), 76.0f, "reduce() traverses rank-3 row blocks");
        require_close(dot(strided_tensor_target, strided_tensor_source), 1044.0f,
                      "dot() traverses rank-3 row gaps without slices");
        require_equal(
            strided_tensor_target.json(),
            "[[[1, 2], [5, 6]], [[13, 14], [17, 18]]]",
            "json() traverses rank-3 row gaps without slices"
        );
        std::ostringstream strided_tensor_stream;
        strided_tensor_stream << strided_tensor_target;
        require_equal(
            strided_tensor_stream.str(),
            "{{{1, 2}, {5, 6}}, {{13, 14}, {17, 18}}}",
            "stream output traverses rank-3 row gaps without slices"
        );

        matrix ranged_source(4, 6);
        fill_sequence(ranged_source);

        matrix contiguous_range_target(4, 6);
        contiguous_range_target.set(-1.0f);
        range contiguous_source_range("[1:3][0:6]");
        range contiguous_target_range("[0:2][0:6]");
        contiguous_range_target.copy(ranged_source, contiguous_target_range, contiguous_source_range);
        require_matrix_close(
            contiguous_range_target,
            make_matrix("7, 8, 9, 10, 11, 12; 13, 14, 15, 16, 17, 18; -1, -1, -1, -1, -1, -1; -1, -1, -1, -1, -1, -1"),
            "ranged copy uses a contiguous selected span"
        );
        require_true(contiguous_source_range.index_ == std::vector<int>{1, 0} &&
                     contiguous_target_range.index_ == std::vector<int>{0, 0},
                     "contiguous ranged copy restores caller iterator state");

        matrix row_block_target(4, 8);
        row_block_target.set(-1.0f);
        range row_block_source_range("[0:4][1:5]");
        range row_block_target_range("[0:4][2:6]");
        row_block_target.copy(ranged_source, row_block_target_range, row_block_source_range);
        require_matrix_close(
            row_block_target,
            make_matrix("-1, -1, 2, 3, 4, 5, -1, -1; -1, -1, 8, 9, 10, 11, -1, -1; -1, -1, 14, 15, 16, 17, -1, -1; -1, -1, 20, 21, 22, 23, -1, -1"),
            "ranged copy uses contiguous row blocks"
        );
        require_true(row_block_source_range.index_ == std::vector<int>{0, 1} &&
                     row_block_target_range.index_ == std::vector<int>{0, 2},
                     "row-block ranged copy restores caller iterator state");

        matrix row_gapped_range_source(4, 10);
        fill_sequence(row_gapped_range_source);
        row_gapped_range_source.resize(4, 6);
        matrix row_gapped_range_target(4, 12);
        row_gapped_range_target.set(-1.0f);
        row_gapped_range_target.resize(4, 8);
        range row_gapped_range_source_selection("[0:4][1:5]");
        range row_gapped_range_target_selection("[0:4][2:6]");
        row_gapped_range_target.copy(row_gapped_range_source,
                                     row_gapped_range_target_selection,
                                     row_gapped_range_source_selection);
        require_matrix_close(
            row_gapped_range_target,
            make_matrix("-1, -1, 2, 3, 4, 5, -1, -1; -1, -1, 12, 13, 14, 15, -1, -1; -1, -1, 22, 23, 24, 25, -1, -1; -1, -1, 32, 33, 34, 35, -1, -1"),
            "ranged copy honors source and target row gaps"
        );
        row_gapped_range_target.resize(4, 12);
        for(int row_index = 0; row_index < row_gapped_range_target.rows(); ++row_index)
        {
            require_close(row_gapped_range_target(row_index, 6), -1.0f, "ranged copy leaves logical row remainder unchanged");
            require_close(row_gapped_range_target(row_index, 11), -1.0f, "ranged copy leaves physical row gap unchanged");
        }

        matrix cross_rank_source = make_matrix("1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12");
        matrix cross_rank_target(3, 4);
        cross_rank_target.set(-1.0f);
        range cross_rank_source_range("[0:12]");
        range cross_rank_target_range("[0:3][0:4]");
        cross_rank_target.copy(cross_rank_source, cross_rank_target_range, cross_rank_source_range);
        require_matrix_close(cross_rank_target,
                             make_matrix("1, 2, 3, 4; 5, 6, 7, 8; 9, 10, 11, 12"),
                             "contiguous ranged copy supports different ranks");

        matrix stepped_range_target(4, 7);
        stepped_range_target.set(-1.0f);
        range stepped_range_source("[0:4][0:6:2]");
        range stepped_range_target_selection("[0:4][1:7:2]");
        stepped_range_target.copy(ranged_source, stepped_range_target_selection, stepped_range_source);
        require_matrix_close(
            stepped_range_target,
            make_matrix("-1, 1, -1, 3, -1, 5, -1; -1, 7, -1, 9, -1, 11, -1; -1, 13, -1, 15, -1, 17, -1; -1, 19, -1, 21, -1, 23, -1"),
            "stepped ranged copy preserves selection order"
        );

        matrix reverse_range_target(2, 6);
        range reverse_range_source("[0:2][0:6:-1]");
        range reverse_range_target_selection("[0:2][0:6]");
        reverse_range_target.copy(ranged_source, reverse_range_target_selection, reverse_range_source);
        require_matrix_close(reverse_range_target,
                             make_matrix("6, 5, 4, 3, 2, 1; 12, 11, 10, 9, 8, 7"),
                             "reverse ranged copy preserves selection order");

        matrix reverse_row_target(4, 4);
        range reverse_row_source_range("[0:4:-1][1:5]");
        range reverse_row_target_range("[0:4][0:4]");
        reverse_row_target.copy(ranged_source, reverse_row_target_range, reverse_row_source_range);
        require_matrix_close(reverse_row_target,
                             make_matrix("20, 21, 22, 23; 14, 15, 16, 17; 8, 9, 10, 11; 2, 3, 4, 5"),
                             "row-block ranged copy supports reversed outer dimensions");
        require_true(reverse_row_source_range.index_ == std::vector<int>{3, 1},
                     "reverse row-block copy restores caller iterator state");

        matrix incompatible_block_target(8);
        range incompatible_block_source_range("[0:2][1:5]");
        range incompatible_block_target_range("[0:8]");
        incompatible_block_target.copy(ranged_source,
                                       incompatible_block_target_range,
                                       incompatible_block_source_range);
        require_matrix_close(incompatible_block_target,
                             make_matrix("2, 3, 4, 5, 8, 9, 10, 11"),
                             "incompatible ranged row widths retain scalar fallback behavior");

        matrix partial_equal_target = make_matrix("-1, -1, -1, -1, -1, -1");
        matrix partial_equal_source = make_matrix("1, 2, 3, 4, 5, 6");
        range partial_equal_source_range("[2:4]");
        range partial_equal_target_range("[2:4]");
        partial_equal_target.copy(partial_equal_source,
                                  partial_equal_target_range,
                                  partial_equal_source_range);
        require_matrix_close(partial_equal_target,
                             make_matrix("-1, -1, 3, 4, -1, -1"),
                             "equal partial ranges do not trigger a whole-matrix copy");

        matrix mismatch_target = make_matrix("9, 9, 9, 9");
        range longer_source_range("[0:3]");
        range shorter_target_range("[0:2]");
        require_throws_as<std::invalid_argument>(
            [&]() { mismatch_target.copy(partial_equal_source, shorter_target_range, longer_source_range); },
            "ranged copy rejects a longer source selection"
        );
        require_matrix_close(mismatch_target, make_matrix("9, 9, 9, 9"),
                             "longer source rejection occurs before writing");

        range shorter_source_range("[0:2]");
        range longer_target_range("[0:3]");
        require_throws_as<std::invalid_argument>(
            [&]() { mismatch_target.copy(partial_equal_source, longer_target_range, shorter_source_range); },
            "ranged copy rejects a longer target selection"
        );
        require_matrix_close(mismatch_target, make_matrix("9, 9, 9, 9"),
                             "longer target rejection occurs before writing");

        range empty_source_range("[0:0]");
        range empty_target_range("[2:2]");
        mismatch_target.copy(partial_equal_source, empty_target_range, empty_source_range);
        require_matrix_close(mismatch_target, make_matrix("9, 9, 9, 9"),
                             "equal empty ranged selections are a no-op");

        range invalid_source_range("[0:7]");
        range valid_target_range("[0:4]");
        require_throws_as<std::out_of_range>(
            [&]() { mismatch_target.copy(partial_equal_source, valid_target_range, invalid_source_range); },
            "ranged copy rejects out-of-bounds source selections"
        );
        require_matrix_close(mismatch_target, make_matrix("9, 9, 9, 9"),
                             "invalid bounds rejection occurs before writing");

        range valid_source_range("[0:4]");
        range invalid_target_range("[0:5]");
        require_throws_as<std::out_of_range>(
            [&]() { mismatch_target.copy(partial_equal_source, invalid_target_range, valid_source_range); },
            "ranged copy rejects out-of-bounds target selections"
        );
        require_matrix_close(mismatch_target, make_matrix("9, 9, 9, 9"),
                             "invalid target bounds rejection occurs before writing");

        matrix overlapping_range = make_matrix("1, 2, 3, 4, 5, 6, 7, 8");
        range overlapping_source_range("[0:6]");
        range overlapping_target_range("[2:8]");
        overlapping_range.copy(overlapping_range, overlapping_target_range, overlapping_source_range);
        require_matrix_close(overlapping_range,
                             make_matrix("1, 2, 1, 2, 1, 2, 1, 2"),
                             "overlapping ranged copy retains element-order behavior");

        matrix backward_overlap = make_matrix("1, 2, 3, 4, 5, 6, 7, 8");
        range backward_overlap_source_range("[2:8]");
        range backward_overlap_target_range("[0:6]");
        backward_overlap.copy(backward_overlap,
                              backward_overlap_target_range,
                              backward_overlap_source_range);
        require_matrix_close(backward_overlap,
                             make_matrix("3, 4, 5, 6, 7, 8, 7, 8"),
                             "backward overlapping ranged copy retains element-order behavior");

        strided_tensor_target.apply([](float value) { return value + 1.0f; });
        expected_tensor.apply([](float value) { return value + 1.0f; });
        require_matrix_close(strided_tensor_target, expected_tensor, "apply() traverses rank-3 row blocks");

        matrix strided_sum(2, 4);
        strided_sum.set(100.0f);
        strided_sum.resize(2, 2);
        strided_sum.add(strided_source, strided_source);
        require_matrix_close(strided_sum, make_matrix("2, 4; 10, 12"), "add(A, B) row blocks with logical row smaller than stride");
        strided_sum.resize(2, 4);
        require_matrix_close(strided_sum, make_matrix("2, 4, 100, 100; 10, 12, 100, 100"), "add(A, B) leaves row gaps unchanged");

        matrix strided_activation = make_matrix("-1, 2, 100, 100; -5, 6, 100, 100");
        strided_activation.resize(2, 2);
        matrix strided_relu(2, 4);
        strided_relu.set(100.0f);
        strided_relu.resize(2, 2);
        strided_relu.relu(strided_activation);
        require_matrix_close(strided_relu, make_matrix("0, 2; 0, 6"), "relu(A) row blocks with logical row smaller than stride");
        strided_relu.resize(2, 4);
        require_matrix_close(strided_relu, make_matrix("0, 2, 100, 100; 0, 6, 100, 100"), "relu(A) leaves row gaps unchanged");

        matrix strided_scaled_output(2, 4);
        strided_scaled_output.set(100.0f);
        strided_scaled_output.resize(2, 2);
        strided_scaled_output.scale(strided_source, 3.0f);
        require_matrix_close(strided_scaled_output, make_matrix("3, 6; 15, 18"), "scale(A) row blocks with logical row smaller than stride");

        matrix strided_add_scaled(2, 4);
        strided_add_scaled.set(100.0f);
        strided_add_scaled.resize(2, 2);
        strided_add_scaled.add_scaled(strided_source, strided_source, 0.5f);
        require_matrix_close(strided_add_scaled, make_matrix("1.5, 3; 7.5, 9"), "add_scaled(A, B) row blocks with logical row smaller than stride");

        matrix strided_hypot_x = make_matrix("3, 4, 100, 100; 5, 12, 100, 100");
        strided_hypot_x.resize(2, 2);
        matrix strided_hypot_y = make_matrix("4, 3, 100, 100; 12, 5, 100, 100");
        strided_hypot_y.resize(2, 2);
        matrix strided_hypot(2, 4);
        strided_hypot.set(100.0f);
        strided_hypot.resize(2, 2);
        strided_hypot.hypot(strided_hypot_x, strided_hypot_y);
        require_matrix_close(strided_hypot, make_matrix("5, 5; 13, 13"), "hypot() row blocks with logical row smaller than stride");
        strided_hypot.resize(2, 4);
        require_matrix_close(strided_hypot, make_matrix("5, 5, 100, 100; 13, 13, 100, 100"), "hypot() leaves row gaps unchanged");

        matrix strided_angle_y = make_matrix("0, 1, 100, 100; 0, -1, 100, 100");
        strided_angle_y.resize(2, 2);
        matrix strided_angle_x = make_matrix("1, 0, 100, 100; -1, 0, 100, 100");
        strided_angle_x.resize(2, 2);
        matrix strided_angle(2, 4);
        strided_angle.set(100.0f);
        strided_angle.resize(2, 2);
        strided_angle.atan2(strided_angle_y, strided_angle_x);
        require_matrix_close(strided_angle, make_matrix("0, 1.5707963; 3.1415927, -1.5707963"), "atan2() row blocks with logical row smaller than stride");
        strided_angle.resize(2, 4);
        require_matrix_close(strided_angle, make_matrix("0, 1.5707963, 100, 100; 3.1415927, -1.5707963, 100, 100"), "atan2() leaves row gaps unchanged");

        matrix strided_backward(2, 4);
        strided_backward.set(100.0f);
        strided_backward.resize(2, 2);
        strided_backward.relu_backward(strided_source, strided_activation);
        require_matrix_close(strided_backward, make_matrix("0, 2; 0, 6"), "relu_backward() row blocks with logical row smaller than stride");

        matrix strided_stddev(2, 4);
        strided_stddev.set(2.0f);
        strided_stddev.resize(2, 2);
        matrix strided_epsilon(2, 4);
        strided_epsilon.set(0.5f);
        strided_epsilon.resize(2, 2);
        matrix strided_sample(2, 4);
        strided_sample.set(100.0f);
        strided_sample.resize(2, 2);
        strided_sample.sample_gaussian(strided_source, strided_stddev, strided_epsilon);
        require_matrix_close(strided_sample, make_matrix("2, 3; 6, 7"), "sample_gaussian() skips row gaps after resize()");
        strided_sample.resize(2, 4);
        require_matrix_close(strided_sample, make_matrix("2, 3, 100, 100; 6, 7, 100, 100"), "sample_gaussian() leaves row gaps unchanged");

        matrix strided_mean(2, 4);
        strided_mean.set(2.0f);
        strided_mean.resize(2, 2);
        matrix strided_log_variance(2, 4);
        strided_log_variance.set(0.0f);
        strided_log_variance.resize(2, 2);

        matrix strided_log_variance_gradient(2, 4);
        strided_log_variance_gradient.set(100.0f);
        strided_log_variance_gradient.resize(2, 2);
        strided_log_variance_gradient.latent_log_variance_gradient(strided_source, strided_epsilon, strided_stddev, strided_log_variance, 2.0f);
        require_matrix_close(strided_log_variance_gradient, make_matrix("0.5, 1; 2.5, 3"), "latent_log_variance_gradient() writes row blocks");
        strided_log_variance_gradient.resize(2, 4);
        require_matrix_close(strided_log_variance_gradient, make_matrix("0.5, 1, 100, 100; 2.5, 3, 100, 100"), "latent_log_variance_gradient() leaves row gaps unchanged");

        matrix strided_d_mean(2, 4);
        strided_d_mean.set(100.0f);
        strided_d_mean.resize(2, 2);
        matrix strided_d_log_variance(2, 4);
        strided_d_log_variance.set(100.0f);
        strided_d_log_variance.resize(2, 2);
        strided_d_mean.latent_sample_gradients(strided_d_log_variance, strided_source, strided_mean, strided_epsilon, strided_stddev, strided_log_variance, 2.0f);
        require_matrix_close(strided_d_mean, make_matrix("5, 6; 9, 10"), "latent_sample_gradients() writes d_mean row blocks");
        require_matrix_close(strided_d_log_variance, make_matrix("0.5, 1; 2.5, 3"), "latent_sample_gradients() writes d_log_variance row blocks");
        strided_d_mean.resize(2, 4);
        strided_d_log_variance.resize(2, 4);
        require_matrix_close(strided_d_mean, make_matrix("5, 6, 100, 100; 9, 10, 100, 100"), "latent_sample_gradients() leaves d_mean row gaps unchanged");
        require_matrix_close(strided_d_log_variance, make_matrix("0.5, 1, 100, 100; 2.5, 3, 100, 100"), "latent_sample_gradients() leaves d_log_variance row gaps unchanged");

        strided_d_mean.set(100.0f);
        strided_d_mean.resize(2, 2);
        strided_d_log_variance.set(100.0f);
        strided_d_log_variance.resize(2, 2);
        strided_d_mean.latent_mean_gradients(strided_d_log_variance, strided_source, strided_mean, strided_log_variance, 2.0f);
        require_matrix_close(strided_d_mean, make_matrix("5, 6; 9, 10"), "latent_mean_gradients() writes d_mean row blocks");
        require_matrix_close(strided_d_log_variance, make_matrix("0, 0; 0, 0"), "latent_mean_gradients() writes d_log_variance row blocks");
        strided_d_mean.resize(2, 4);
        strided_d_log_variance.resize(2, 4);
        require_matrix_close(strided_d_mean, make_matrix("5, 6, 100, 100; 9, 10, 100, 100"), "latent_mean_gradients() leaves d_mean row gaps unchanged");
        require_matrix_close(strided_d_log_variance, make_matrix("0, 0, 100, 100; 0, 0, 100, 100"), "latent_mean_gradients() leaves d_log_variance row gaps unchanged");

        strided_d_mean.set(100.0f);
        strided_d_mean.resize(2, 2);
        strided_d_log_variance.set(100.0f);
        strided_d_log_variance.resize(2, 2);
        strided_d_mean.latent_kl_gradients(strided_d_log_variance, strided_mean, strided_log_variance, 2.0f);
        require_matrix_close(strided_d_mean, make_matrix("4, 4; 4, 4"), "latent_kl_gradients() writes d_mean row blocks");
        require_matrix_close(strided_d_log_variance, make_matrix("0, 0; 0, 0"), "latent_kl_gradients() writes d_log_variance row blocks");
        strided_d_mean.resize(2, 4);
        strided_d_log_variance.resize(2, 4);
        require_matrix_close(strided_d_mean, make_matrix("4, 4, 100, 100; 4, 4, 100, 100"), "latent_kl_gradients() leaves d_mean row gaps unchanged");
        require_matrix_close(strided_d_log_variance, make_matrix("0, 0, 100, 100; 0, 0, 100, 100"), "latent_kl_gradients() leaves d_log_variance row gaps unchanged");

        matrix strided_adam_values(2, 4);
        strided_adam_values.set(10.0f);
        strided_adam_values.resize(2, 2);
        matrix strided_adam_gradient(2, 4);
        strided_adam_gradient.set(2.0f);
        strided_adam_gradient.resize(2, 2);
        matrix strided_first_moment(2, 4);
        strided_first_moment.set(0.0f);
        strided_first_moment.resize(2, 2);
        matrix strided_second_moment(2, 4);
        strided_second_moment.set(0.0f);
        strided_second_moment.resize(2, 2);
        strided_adam_values.adam_update(strided_adam_gradient, strided_first_moment, strided_second_moment, 0.1f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
        require_matrix_close(strided_adam_values, make_matrix("9.9, 9.9; 9.9, 9.9"), "adam_update() updates values row blocks");
        require_matrix_close(strided_first_moment, make_matrix("2, 2; 2, 2"), "adam_update() updates first moment row blocks");
        require_matrix_close(strided_second_moment, make_matrix("4, 4; 4, 4"), "adam_update() updates second moment row blocks");
        strided_adam_values.resize(2, 4);
        require_matrix_close(strided_adam_values, make_matrix("9.9, 9.9, 10, 10; 9.9, 9.9, 10, 10"), "adam_update() leaves value row gaps unchanged");

        matrix strided_tensor(std::vector<int>{2, 2, 4});
        fill_sequence(strided_tensor);
        strided_tensor.resize(2, 2, 2);
        matrix strided_reduced;
        strided_reduced.sum_last_two_dimensions(strided_tensor);
        require_matrix_close(strided_reduced, make_matrix("14, 46"), "sum_last_two_dimensions() skips row gaps after resize()");

        matrix reserved_clip(4, 2);
        fill_sequence(reserved_clip);
        reserved_clip.resize(1, 2);
        reserved_clip.clip(0.0f, 1.5f);
        require_matrix_close(reserved_clip[0], make_matrix("1, 1.5"), "clip() uses logical size for contiguous dynamic capacity");
        reserved_clip.resize(4, 2);
        require_matrix_close(reserved_clip, make_matrix("1, 1.5; 3, 4; 5, 6; 7, 8"), "clip() leaves capacity outside logical shape unchanged");

        matrix reserved_gradient(4, 2);
        reserved_gradient.set(10.0f);
        reserved_gradient.resize(1, 2);
        matrix reserved_output(4, 2);
        reserved_output.set(0.5f);
        reserved_output.resize(1, 2);
        reserved_gradient.multiply_sigmoid_derivative(reserved_output);
        require_matrix_close(reserved_gradient[0], make_matrix("2.5, 2.5"), "multiply_sigmoid_derivative() uses logical size for contiguous dynamic capacity");
        reserved_gradient.resize(4, 2);
        require_matrix_close(reserved_gradient, make_matrix("2.5, 2.5; 10, 10; 10, 10; 10, 10"), "multiply_sigmoid_derivative() leaves capacity outside logical shape unchanged");
    }

    void benchmark_hotspots()
    {
        constexpr int channels = 8;
        constexpr int rows = 64;
        constexpr int cols = 64;
        constexpr int iterations = 200;
        double checksum = 0.0;

        constexpr int construction_iterations = 100000;
        double rank_two_construction_ms = measure_ms([&]()
        {
            for(int i = 0; i < construction_iterations; ++i)
            {
                matrix temporary(2, 2);
                if((i & 1023) == 0)
                    temporary(0, 0) = static_cast<float>(i);
                checksum += temporary(0, 0);
            }
        });

        constexpr int input_file_rows = 10000;
        constexpr int input_file_columns = 16;
        matrix input_file_row(input_file_columns);
        fill_sequence(input_file_row);
        double input_file_growth_ms = measure_ms([&]()
        {
            matrix loaded_data(1, input_file_columns);
            loaded_data.resize(0, input_file_columns);
            for(int row = 0; row < input_file_rows; ++row)
                loaded_data.append(input_file_row);
            checksum += loaded_data(input_file_rows - 1, input_file_columns - 1);
        });

        matrix source(std::vector<int>{channels, rows, cols});
        fill_sequence(source);
        matrix target(std::vector<int>{channels, rows, cols});
        matrix reduced(channels);
        matrix rank_four_source(std::vector<int>{4, 4, 32, 32});
        fill_sequence(rank_four_source);

        matrix image_source(64, 64);
        fill_sequence(image_source);
        matrix downsample_target(32, 32);
        matrix upsample_target(128, 128);
        constexpr int image_iterations = 500;
        double downsample_ms = measure_ms([&]()
        {
            for(int i = 0; i < image_iterations; ++i)
                downsample_target.downsample(image_source);
        });
        checksum += downsample_target.sum();
        double upsample_ms = measure_ms([&]()
        {
            for(int i = 0; i < image_iterations; ++i)
                upsample_target.upsample(image_source);
        });
        checksum += upsample_target.sum();

        matrix search_target(8, 8);
        search_target.submatrix(image_source, {17, 23, 8, 8});
        constexpr int search_iterations = 20;
        double search_ms = measure_ms([&]()
        {
            for(int i = 0; i < search_iterations; ++i)
            {
                const match result = image_source.search(search_target, {0, 0, 64, 64});
                checksum += result.x + result.y + result.score;
            }
        });

        matrix decomposition_source(16, 16);
        decomposition_source.set(0.0f);
        for(int row = 0; row < 16; ++row)
            for(int col = 0; col < 16; ++col)
                decomposition_source(row, col) = row == col ?
                    2.0f + 0.01f * row : 0.001f * (1 + std::min(row, col));

        constexpr int decomposition_iterations = 100;
        double determinant_ms = measure_ms([&]()
        {
            for(int i = 0; i < decomposition_iterations; ++i)
                checksum += decomposition_source.det();
        });

        matrix inverse_target(16, 16);
        double inverse_ms = measure_ms([&]()
        {
            for(int i = 0; i < decomposition_iterations; ++i)
            {
                inverse_target.copy(decomposition_source);
                inverse_target.inv();
                checksum += inverse_target(0, 0);
            }
        });

        double eigen_ms = measure_ms([&]()
        {
            for(int i = 0; i < decomposition_iterations; ++i)
            {
                auto [eigenvectors, eigenvalues] = decomposition_source.eig();
                checksum += eigenvectors(0, 0) + eigenvalues(0);
            }
        });

        matrix svd_u(16, 16);
        matrix svd_s(16, 16);
        matrix svd_vt(16, 16);
        double svd_ms = measure_ms([&]()
        {
            for(int i = 0; i < decomposition_iterations; ++i)
            {
                decomposition_source.singular_value_decomposition(
                    decomposition_source, svd_u, svd_s, svd_vt
                );
                checksum += svd_s(0, 0);
            }
        });

        matrix templated_apply_target = source.clone();
        double templated_apply_ms = measure_ms([&]()
        {
            for(int i = 0; i < iterations; ++i)
                templated_apply_target.apply([](float value) { return value * 1.000001f; });
        });
        checksum += templated_apply_target.sum();

        matrix type_erased_apply_target = source.clone();
        std::function<float(float)> type_erased_apply =
            [](float value) { return value * 1.000001f; };
        double type_erased_apply_ms = measure_ms([&]()
        {
            for(int i = 0; i < iterations; ++i)
                type_erased_apply_target.apply(type_erased_apply);
        });
        checksum += type_erased_apply_target.sum();

        double templated_reduce_checksum = 0.0;
        double templated_reduce_ms = measure_ms([&]()
        {
            for(int i = 0; i < iterations; ++i)
                source.reduce([&](float value) { templated_reduce_checksum += value; });
        });
        checksum += templated_reduce_checksum;

        double type_erased_reduce_checksum = 0.0;
        std::function<void(float)> type_erased_reduce =
            [&](float value) { type_erased_reduce_checksum += value; };
        double type_erased_reduce_ms = measure_ms([&]()
        {
            for(int i = 0; i < iterations; ++i)
                source.reduce(type_erased_reduce);
        });
        checksum += type_erased_reduce_checksum;

        constexpr int small_apply_iterations = 200000;
        matrix small_templated_apply_target(32);
        small_templated_apply_target.set(1.0f);
        double small_templated_apply_ms = measure_ms([&]()
        {
            for(int i = 0; i < small_apply_iterations; ++i)
                small_templated_apply_target.apply([](float value) { return value * 1.000001f; });
        });
        checksum += small_templated_apply_target.sum();

        matrix small_type_erased_apply_target(32);
        small_type_erased_apply_target.set(1.0f);
        double small_type_erased_apply_ms = measure_ms([&]()
        {
            for(int i = 0; i < small_apply_iterations; ++i)
                small_type_erased_apply_target.apply(type_erased_apply);
        });
        checksum += small_type_erased_apply_target.sum();

        double small_templated_reduce_checksum = 0.0;
        double small_templated_reduce_ms = measure_ms([&]()
        {
            for(int i = 0; i < small_apply_iterations; ++i)
                small_templated_apply_target.reduce(
                    [&](float value) { small_templated_reduce_checksum += value; }
                );
        });
        checksum += small_templated_reduce_checksum;

        double small_type_erased_reduce_checksum = 0.0;
        std::function<void(float)> small_type_erased_reduce =
            [&](float value) { small_type_erased_reduce_checksum += value; };
        double small_type_erased_reduce_ms = measure_ms([&]()
        {
            for(int i = 0; i < small_apply_iterations; ++i)
                small_type_erased_apply_target.reduce(small_type_erased_reduce);
        });
        checksum += small_type_erased_reduce_checksum;

        matrix access_rank1(std::vector<int>{2});
        matrix access_rank2(std::vector<int>{2, 2});
        matrix access_rank3(std::vector<int>{2, 2, 2});
        matrix access_rank4(std::vector<int>{2, 2, 2, 2});
        matrix access_rank5(std::vector<int>{2, 2, 2, 2, 2});
        fill_sequence(access_rank1);
        fill_sequence(access_rank2);
        fill_sequence(access_rank3);
        fill_sequence(access_rank4);
        fill_sequence(access_rank5);
        const matrix & const_access_rank1 = access_rank1;
        const matrix & const_access_rank2 = access_rank2;
        const matrix & const_access_rank3 = access_rank3;
        const matrix & const_access_rank4 = access_rank4;
        const matrix & const_access_rank5 = access_rank5;

        matrix row_gapped_access_rank2(std::vector<int>{2, 4});
        matrix row_gapped_access_rank5(std::vector<int>{2, 2, 2, 2, 4});
        fill_sequence(row_gapped_access_rank2);
        fill_sequence(row_gapped_access_rank5);
        row_gapped_access_rank2.resize(std::vector<int>{2, 2});
        row_gapped_access_rank5.resize(std::vector<int>{2, 2, 2, 2, 2});
        const matrix & const_row_gapped_access_rank2 = row_gapped_access_rank2;
        const matrix & const_row_gapped_access_rank5 = row_gapped_access_rank5;

        constexpr int scalar_access_iterations = 5000000;
        double mutable_rank1_access_ms = measure_ms([&]()
        {
            for(int i = 0; i < scalar_access_iterations; ++i)
                checksum += access_rank1(i & 1);
        });
        double const_rank1_access_ms = measure_ms([&]()
        {
            for(int i = 0; i < scalar_access_iterations; ++i)
                checksum += const_access_rank1(i & 1);
        });
        double const_rank2_access_ms = measure_ms([&]()
        {
            for(int i = 0; i < scalar_access_iterations; ++i)
                checksum += const_access_rank2(i & 1, i & 1);
        });
        double const_rank3_access_ms = measure_ms([&]()
        {
            for(int i = 0; i < scalar_access_iterations; ++i)
                checksum += const_access_rank3(i & 1, i & 1, i & 1);
        });
        double const_rank4_access_ms = measure_ms([&]()
        {
            for(int i = 0; i < scalar_access_iterations; ++i)
                checksum += const_access_rank4(i & 1, i & 1, i & 1, i & 1);
        });
        double const_rank5_access_ms = measure_ms([&]()
        {
            for(int i = 0; i < scalar_access_iterations; ++i)
                checksum += const_access_rank5(i & 1, i & 1, i & 1, i & 1, i & 1);
        });
        double row_gapped_const_rank2_access_ms = measure_ms([&]()
        {
            for(int i = 0; i < scalar_access_iterations; ++i)
                checksum += const_row_gapped_access_rank2(i & 1, i & 1);
        });
        double row_gapped_const_rank5_access_ms = measure_ms([&]()
        {
            for(int i = 0; i < scalar_access_iterations; ++i)
                checksum += const_row_gapped_access_rank5(i & 1, i & 1, i & 1, i & 1, i & 1);
        });

        double contiguous_copy_ms = measure_ms([&]()
        {
            for(int i = 0; i < iterations; ++i)
                target.copy(source);
            checksum += target(0, 0, 0);
        });

        range contiguous_source_range("[2:6][0:64][0:64]");
        range contiguous_target_range("[0:4][0:64][0:64]");
        double ranged_contiguous_copy_ms = measure_ms([&]()
        {
            for(int i = 0; i < iterations; ++i)
                target.copy(source, contiguous_target_range, contiguous_source_range);
            checksum += target(0, 0, 0);
        });

        range row_block_source_range("[0:8][0:64][8:56]");
        range row_block_target_range("[0:8][0:64][4:52]");
        double ranged_row_block_copy_ms = measure_ms([&]()
        {
            for(int i = 0; i < iterations; ++i)
                target.copy(source, row_block_target_range, row_block_source_range);
            checksum += target(0, 0, 4);
        });

        range stepped_source_range("[0:8][0:64][0:64:2]");
        range stepped_target_range("[0:8][0:64][1:64:2]");
        for(int i = 0; i < 20; ++i)
            target.copy(source, stepped_target_range, stepped_source_range);
        double ranged_stepped_copy_ms = measure_ms([&]()
        {
            for(int i = 0; i < iterations; ++i)
                target.copy(source, stepped_target_range, stepped_source_range);
            checksum += target(0, 0, 1);
        });

        range scalar_source_range("[0][0][0]");
        range scalar_target_range("[0][0][0]");
        constexpr int scalar_copy_iterations = 200000;
        double ranged_scalar_copy_ms = measure_ms([&]()
        {
            for(int i = 0; i < scalar_copy_iterations; ++i)
                target.copy(source, scalar_target_range, scalar_source_range);
            checksum += target(0, 0, 0);
        });

        double slice_copy_ms = measure_ms([&]()
        {
            for(int i = 0; i < iterations; ++i)
                target[i % channels].copy(source[(i + 1) % channels]);
            checksum += target[0].sum();
        });

        double slice_apply_ms = measure_ms([&]()
        {
            for(int i = 0; i < iterations; ++i)
                target[i % channels].add(source[(i + 2) % channels]);
            checksum += target[1].sum();
        });

        double full_sum_ms = measure_ms([&]()
        {
            for(int i = 0; i < iterations; ++i)
                checksum += source.sum();
        });

        double slice_sum_ms = measure_ms([&]()
        {
            for(int i = 0; i < iterations; ++i)
                checksum += source[i % channels].sum();
        });

        constexpr int temporary_view_iterations = 200000;
        double temporary_views_ms = measure_ms([&]()
        {
            for(int i = 0; i < temporary_view_iterations; ++i)
            {
                matrix view = source[i % channels];
                checksum += view(0, 0);
            }
        });

        double chained_temporary_views_ms = measure_ms([&]()
        {
            for(int i = 0; i < temporary_view_iterations; ++i)
            {
                matrix view = source[i % channels][i % rows];
                checksum += view(0);
            }
        });

        constexpr int iterator_view_iterations = temporary_view_iterations / channels;
        double iterator_views_ms = measure_ms([&]()
        {
            for(int i = 0; i < iterator_view_iterations; ++i)
                for(matrix view : source)
                    checksum += view(0, 0);
        });

        const matrix & const_source = source;
        double const_iterator_views_ms = measure_ms([&]()
        {
            for(int i = 0; i < iterator_view_iterations; ++i)
                for(const_matrix_view view : const_source)
                    checksum += view(0, 0);
        });

        double sum_last_two_dimensions_ms = measure_ms([&]()
        {
            for(int i = 0; i < iterations; ++i)
                reduced.sum_last_two_dimensions(source);
            checksum += reduced.sum();
        });

        double dot_ms = measure_ms([&]()
        {
            for(int i = 0; i < iterations; ++i)
                checksum += dot(source[i % channels], source[(i + 3) % channels]);
        });

        double serialization_ms = measure_ms([&]()
        {
            for(int i = 0; i < 20; ++i)
            {
                checksum += source[i % channels].json().size();
                checksum += source[i % channels].csv().size();
            }
        });

        double rank_three_json_ms = measure_ms([&]()
        {
            for(int i = 0; i < 10; ++i)
                checksum += source.json().size();
        });

        double rank_four_json_ms = measure_ms([&]()
        {
            for(int i = 0; i < 10; ++i)
                checksum += rank_four_source.json().size();
        });

        std::ostringstream rank_four_stream;
        double rank_four_stream_ms = measure_ms([&]()
        {
            for(int i = 0; i < 10; ++i)
            {
                rank_four_stream.str("");
                rank_four_stream.clear();
                rank_four_stream << rank_four_source;
                checksum += static_cast<std::streamoff>(rank_four_stream.tellp());
            }
        });

        matrix row_gapped_source(rows, cols * 2);
        fill_sequence(row_gapped_source);
        row_gapped_source.resize(rows, cols);
        matrix row_gapped_target(rows, cols * 2);
        row_gapped_target.resize(rows, cols);
        matrix row_gapped_other(rows, cols * 2);
        fill_sequence(row_gapped_other);
        row_gapped_other.scale(0.5f);
        row_gapped_other.resize(rows, cols);
        matrix row_gapped_aux(rows, cols * 2);
        row_gapped_aux.set(0.25f);
        row_gapped_aux.resize(rows, cols);
        matrix row_gapped_log_variance(rows, cols * 2);
        row_gapped_log_variance.set(0.0f);
        row_gapped_log_variance.resize(rows, cols);
        matrix row_gapped_d_log_variance(rows, cols * 2);
        row_gapped_d_log_variance.resize(rows, cols);
        matrix row_gapped_first_moment(rows, cols * 2);
        row_gapped_first_moment.set(0.0f);
        row_gapped_first_moment.resize(rows, cols);
        matrix row_gapped_second_moment(rows, cols * 2);
        row_gapped_second_moment.set(0.0f);
        row_gapped_second_moment.resize(rows, cols);
        matrix row_gapped_adam_values(rows, cols * 2);
        row_gapped_adam_values.set(1.0f);
        row_gapped_adam_values.resize(rows, cols);

        constexpr int row_gap_iterations = 500;

        double row_gapped_sum_ms = measure_ms([&]()
        {
            for(int i = 0; i < row_gap_iterations; ++i)
                checksum += row_gapped_source.sum();
        });

        double row_gapped_dot_ms = measure_ms([&]()
        {
            for(int i = 0; i < row_gap_iterations; ++i)
                checksum += dot(row_gapped_source, row_gapped_other);
        });

        double row_gapped_copy_ms = measure_ms([&]()
        {
            for(int i = 0; i < row_gap_iterations; ++i)
                row_gapped_target.copy(row_gapped_source);
            checksum += row_gapped_target.sum();
        });

        range row_gapped_source_range("[0:64][8:56]");
        range row_gapped_target_range("[0:64][4:52]");
        double ranged_row_gapped_copy_ms = measure_ms([&]()
        {
            for(int i = 0; i < row_gap_iterations; ++i)
                row_gapped_target.copy(row_gapped_source, row_gapped_target_range, row_gapped_source_range);
            checksum += row_gapped_target(0, 4);
        });

        double row_gapped_unary_apply_ms = measure_ms([&]()
        {
            for(int i = 0; i < row_gap_iterations; ++i)
                row_gapped_target.copy(row_gapped_source).scale(1.001f);
            checksum += row_gapped_target.sum();
        });

        double row_gapped_binary_apply_ms = measure_ms([&]()
        {
            for(int i = 0; i < row_gap_iterations; ++i)
                row_gapped_target.copy(row_gapped_source).add(row_gapped_other);
            checksum += row_gapped_target.sum();
        });

        double row_gapped_ternary_apply_ms = measure_ms([&]()
        {
            for(int i = 0; i < row_gap_iterations; ++i)
                row_gapped_target.add(row_gapped_source, row_gapped_other);
            checksum += row_gapped_target.sum();
        });

        double row_gapped_hypot_ms = measure_ms([&]()
        {
            for(int i = 0; i < row_gap_iterations; ++i)
                row_gapped_target.hypot(row_gapped_source, row_gapped_other);
            checksum += row_gapped_target.sum();
        });

        double row_gapped_sample_gaussian_ms = measure_ms([&]()
        {
            for(int i = 0; i < row_gap_iterations; ++i)
                row_gapped_target.sample_gaussian(row_gapped_source, row_gapped_other, row_gapped_aux);
            checksum += row_gapped_target.sum();
        });

        double row_gapped_latent_log_variance_ms = measure_ms([&]()
        {
            for(int i = 0; i < row_gap_iterations; ++i)
                row_gapped_target.latent_log_variance_gradient(row_gapped_source, row_gapped_aux, row_gapped_other, row_gapped_log_variance, 0.1f);
            checksum += row_gapped_target.sum();
        });

        double row_gapped_latent_sample_gradients_ms = measure_ms([&]()
        {
            for(int i = 0; i < row_gap_iterations; ++i)
                row_gapped_target.latent_sample_gradients(row_gapped_d_log_variance, row_gapped_source, row_gapped_other, row_gapped_aux, row_gapped_other, row_gapped_log_variance, 0.1f);
            checksum += row_gapped_target.sum() + row_gapped_d_log_variance.sum();
        });

        double row_gapped_latent_mean_gradients_ms = measure_ms([&]()
        {
            for(int i = 0; i < row_gap_iterations; ++i)
                row_gapped_target.latent_mean_gradients(row_gapped_d_log_variance, row_gapped_source, row_gapped_other, row_gapped_log_variance, 0.1f);
            checksum += row_gapped_target.sum() + row_gapped_d_log_variance.sum();
        });

        double row_gapped_latent_kl_gradients_ms = measure_ms([&]()
        {
            for(int i = 0; i < row_gap_iterations; ++i)
                row_gapped_target.latent_kl_gradients(row_gapped_d_log_variance, row_gapped_other, row_gapped_log_variance, 0.1f);
            checksum += row_gapped_target.sum() + row_gapped_d_log_variance.sum();
        });

        double row_gapped_adam_ms = measure_ms([&]()
        {
            for(int i = 0; i < row_gap_iterations; ++i)
                row_gapped_adam_values.adam_update(row_gapped_aux, row_gapped_first_moment, row_gapped_second_moment, 0.001f, 0.9f, 0.999f, 0.1f, 0.001f, 1e-8f);
            checksum += row_gapped_adam_values.sum();
        });

        std::cout << "MATRIX BENCHMARK hotspots"
                  << " rank_two_construction_ms=" << rank_two_construction_ms
                  << " input_file_growth_ms=" << input_file_growth_ms
                  << " downsample_ms=" << downsample_ms
                  << " upsample_ms=" << upsample_ms
                  << " search_ms=" << search_ms
                  << " determinant_ms=" << determinant_ms
                  << " inverse_ms=" << inverse_ms
                  << " eigen_ms=" << eigen_ms
                  << " svd_ms=" << svd_ms
                  << " templated_apply_ms=" << templated_apply_ms
                  << " type_erased_apply_ms=" << type_erased_apply_ms
                  << " templated_reduce_ms=" << templated_reduce_ms
                  << " type_erased_reduce_ms=" << type_erased_reduce_ms
                  << " small_templated_apply_ms=" << small_templated_apply_ms
                  << " small_type_erased_apply_ms=" << small_type_erased_apply_ms
                  << " small_templated_reduce_ms=" << small_templated_reduce_ms
                  << " small_type_erased_reduce_ms=" << small_type_erased_reduce_ms
                  << " mutable_rank1_access_ms=" << mutable_rank1_access_ms
                  << " const_rank1_access_ms=" << const_rank1_access_ms
                  << " const_rank2_access_ms=" << const_rank2_access_ms
                  << " const_rank3_access_ms=" << const_rank3_access_ms
                  << " const_rank4_access_ms=" << const_rank4_access_ms
                  << " const_rank5_access_ms=" << const_rank5_access_ms
                  << " row_gapped_const_rank2_access_ms=" << row_gapped_const_rank2_access_ms
                  << " row_gapped_const_rank5_access_ms=" << row_gapped_const_rank5_access_ms
                  << " contiguous_copy_ms=" << contiguous_copy_ms
                  << " ranged_contiguous_copy_ms=" << ranged_contiguous_copy_ms
                  << " ranged_row_block_copy_ms=" << ranged_row_block_copy_ms
                  << " ranged_stepped_copy_ms=" << ranged_stepped_copy_ms
                  << " ranged_scalar_copy_ms=" << ranged_scalar_copy_ms
                  << " slice_copy_ms=" << slice_copy_ms
                  << " slice_apply_ms=" << slice_apply_ms
                  << " full_sum_ms=" << full_sum_ms
                  << " slice_sum_ms=" << slice_sum_ms
                  << " temporary_views_ms=" << temporary_views_ms
                  << " chained_temporary_views_ms=" << chained_temporary_views_ms
                  << " iterator_views_ms=" << iterator_views_ms
                  << " const_iterator_views_ms=" << const_iterator_views_ms
                  << " sum_last_two_dimensions_ms=" << sum_last_two_dimensions_ms
                  << " dot_ms=" << dot_ms
                  << " serialization_ms=" << serialization_ms
                  << " rank_three_json_ms=" << rank_three_json_ms
                  << " rank_four_json_ms=" << rank_four_json_ms
                  << " rank_four_stream_ms=" << rank_four_stream_ms
                  << " row_gapped_sum_ms=" << row_gapped_sum_ms
                  << " row_gapped_dot_ms=" << row_gapped_dot_ms
                  << " row_gapped_copy_ms=" << row_gapped_copy_ms
                  << " ranged_row_gapped_copy_ms=" << ranged_row_gapped_copy_ms
                  << " row_gapped_unary_apply_ms=" << row_gapped_unary_apply_ms
                  << " row_gapped_binary_apply_ms=" << row_gapped_binary_apply_ms
                  << " row_gapped_ternary_apply_ms=" << row_gapped_ternary_apply_ms
                  << " row_gapped_hypot_ms=" << row_gapped_hypot_ms
                  << " row_gapped_sample_gaussian_ms=" << row_gapped_sample_gaussian_ms
                  << " row_gapped_latent_log_variance_ms=" << row_gapped_latent_log_variance_ms
                  << " row_gapped_latent_sample_gradients_ms=" << row_gapped_latent_sample_gradients_ms
                  << " row_gapped_latent_mean_gradients_ms=" << row_gapped_latent_mean_gradients_ms
                  << " row_gapped_latent_kl_gradients_ms=" << row_gapped_latent_kl_gradients_ms
                  << " row_gapped_adam_ms=" << row_gapped_adam_ms
                  << " checksum=" << checksum
                  << std::endl;
    }

    void test_output_size_contracts()
    {
        matrix original = make_matrix("1, 2; 3, 4");
        matrix wrong_copy(3);
        require_throws([&]() { wrong_copy.copy(original); }, "copy() should throw for wrong-sized initialized destination");

        matrix wrong_submatrix(1, 3);
        matrix submatrix_source = make_matrix("1, 2, 3; 4, 5, 6; 7, 8, 9");
        require_throws([&]() { wrong_submatrix.submatrix(submatrix_source, {0, 0, 2, 2}); }, "submatrix() should throw for wrong-sized initialized destination");

        matrix transpose_target(3, 1);
        require_throws([&]() { original.transpose(transpose_target); }, "transpose() should throw for wrong-sized initialized destination");

        matrix left = make_matrix("1, 2, 3; 4, 5, 6");
        matrix right = make_matrix("7, 8; 9, 10; 11, 12");
        matrix wrong_matmul(3, 3);
        require_throws([&]() { wrong_matmul.matmul(left, right); }, "matmul() should throw for wrong-sized initialized destination");

        matrix wrong_matvec(3);
        require_throws([&]() { wrong_matvec.matvec(left, make_matrix("7, 8, 9")); }, "matvec() should throw for wrong-sized initialized destination");

        matrix hx = make_matrix("3, 4");
        matrix hy = make_matrix("4, 3");
        matrix wrong_hypot(3);
        require_throws([&]() { wrong_hypot.hypot(hx, hy); }, "hypot() should throw for wrong-sized initialized destination");

        matrix ax = make_matrix("1, 0");
        matrix ay = make_matrix("0, 1");
        matrix wrong_atan2(3);
        require_throws([&]() { wrong_atan2.atan2(ay, ax); }, "atan2() should throw for wrong-sized initialized destination");

        matrix image = make_matrix("1, 2, 3; 4, 5, 6; 7, 8, 9");
        matrix kernel = make_matrix("1, 0; 0, -1");
        matrix wrong_corr(3, 3);
        require_throws([&]() { wrong_corr.corr2(image, kernel); }, "corr2() should throw for wrong-sized initialized destination");

        matrix wrong_conv_slow(3, 3);
        require_throws([&]() { wrong_conv_slow.conv2_slow(image, kernel); }, "conv2_slow() should throw for wrong-sized initialized destination");

        matrix wrong_conv(2, 2);
        matrix conv_image = make_matrix("1, 2, 3; 4, 5, 6; 7, 8, 9");
        matrix conv_kernel = make_matrix("1, 0; 0, -1");
        require_throws([&]() { wrong_conv.conv2(conv_image, conv_kernel); }, "conv2() should throw for wrong-sized initialized destination");

        matrix down_source = make_matrix("1, 2, 3, 4; 5, 6, 7, 8; 9, 10, 11, 12; 13, 14, 15, 16");
        matrix wrong_downsample(3, 3);
        require_throws([&]() { wrong_downsample.downsample(down_source); }, "downsample() should throw for wrong-sized initialized destination");

        matrix downsample_target(2, 2);
        matrix wrong_temporary_row(3);
        require_throws([&]() { downsample_target.downsample(down_source, wrong_temporary_row); }, "downsample() should throw for wrong-sized temporary row");

        matrix wrong_temporary_rank(1, 4);
        require_throws([&]() { downsample_target.downsample(down_source, wrong_temporary_rank); }, "downsample() should throw for wrong-rank temporary row");

        require_throws([&]() { downsample_target.downsample(down_source, down_source); }, "downsample() should reject a source alias as temporary row");
        require_throws([&]() { downsample_target.downsample(down_source, downsample_target); }, "downsample() should reject a destination alias as temporary row");

        matrix up_source = make_matrix("1, 2; 3, 4");
        matrix wrong_upsample(3, 3);
        require_throws([&]() { wrong_upsample.upsample(up_source); }, "upsample() should throw for wrong-sized initialized destination");

        matrix svd_input = make_matrix("1, 2; 3, 4");
        matrix U(3, 3);
        matrix S;
        matrix Vt;
        require_throws([&]() { svd_input.singular_value_decomposition(svd_input, U, S, Vt); }, "singular_value_decomposition() should throw for wrong-sized U");

        matrix U_ok;
        matrix S_wrong(3, 3);
        matrix Vt_ok;
        require_throws([&]() { svd_input.singular_value_decomposition(svd_input, U_ok, S_wrong, Vt_ok); }, "singular_value_decomposition() should throw for wrong-sized S");

        matrix U_ok2;
        matrix S_ok2;
        matrix Vt_wrong(3, 3);
        require_throws([&]() { svd_input.singular_value_decomposition(svd_input, U_ok2, S_ok2, Vt_wrong); }, "singular_value_decomposition() should throw for wrong-sized Vt");
    }

    void Init() override
    {
        Bind(suite_, "suite");

        const std::string suite = suite_.as_string();

        if(suite == "reductions")
            test_reductions();
        else if(suite == "scalar")
            test_scalar_and_shape();
        else if(suite == "elementwise")
            test_elementwise();
        else if(suite == "linalg")
            test_linalg();
        else if(suite == "image")
            test_image();
        else if(suite == "submatrix")
            test_submatrix_views();
        else if(suite == "hotspots")
            test_matrix_hotspots();
        else if(suite == "benchmark")
            benchmark_hotspots();
        else if(suite == "contracts")
            test_output_size_contracts();
        else
            throw exception("MatrixFunctionTestModule: Unknown suite \"" + suite + "\"");

        std::cout << "MATRIX TEST " << suite << " OK" << std::endl;
    }
};

INSTALL_CLASS(MatrixFunctionTestModule)
    std::string shape_string(const std::vector<int> & shape)
    {
        std::string result = "[";
        for(std::size_t i = 0; i < shape.size(); ++i)
        {
            if(i > 0)
                result += ", ";
            result += std::to_string(shape[i]);
        }
        result += "]";
        return result;
    }
