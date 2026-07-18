#include "ikaros.h"

#include <chrono>
#include <cmath>
#include <limits>
#include <random>
#include <sstream>

using namespace ikaros;

namespace
{
    constexpr float kTolerance = 1e-4f;
    constexpr float kHalfPi = 1.57079632679489661923f;

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

    void require_shape(matrix actual, const std::vector<int> & expected, const std::string & message)
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

        for(auto ix = actual.get_range(); ix.more(); ix++)
        {
            int actual_index = actual.compute_index(ix.index());
            int expected_index = expected.compute_index(ix.index());
            require_close((*actual.data_)[actual_index], (*expected.data_)[expected_index], message, tolerance);
        }
    }

    void fill_sequence(matrix & values)
    {
        float value = 1.0f;
        for(auto ix = values.get_range(); ix.more(); ix++)
            (*values.data_)[values.compute_index(ix.index())] = value++;
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
        matrix uninitialized;
        require_true(uninitialized.size() == 0, "size() is zero for uninitialized matrix");
        require_throws_as<std::invalid_argument>(
            [&]() { matrix invalid_shape(-1, 2); },
            "constructor should preserve invalid-dimension errors"
        );
        require_throws_as<std::out_of_range>(
            [&]() { matrix oversized(std::numeric_limits<int>::max(), 2); },
            "constructor should preserve shape-overflow errors"
        );
        require_throws([&]() { uninitialized[0]; }, "operator[] should reject uninitialized rank-zero matrices");
        const matrix & const_uninitialized = uninitialized;
        require_throws([&]() { const_uninitialized[0]; }, "const operator[] should reject rank-zero matrices");
        matrix scalar_source(1);
        matrix scalar = scalar_source[0];
        require_true(scalar.size() == 1, "size() is one for scalar matrix");
        require_throws([&]() { scalar[0]; }, "operator[] should reject scalar rank-zero matrices");

        matrix scalar_values = make_matrix("10, 42");
        matrix scalar_copy;
        scalar_copy.copy(scalar_values[1]);
        require_true(scalar_copy.is_scalar(), "copy(scalar) produces a scalar matrix");
        require_close(static_cast<float>(scalar_copy), 42.0f, "copy(scalar) honors source offset");
        scalar_values(1) = 99.0f;
        require_close(static_cast<float>(scalar_copy), 42.0f, "copy(scalar) performs a deep copy");

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

        float ** legacy_matrix_rows = legacy_matrix;
        require_close(legacy_matrix_rows[1][2], 6.0f, "legacy float** conversion");
        legacy_matrix.reserve(3, 3);
        legacy_matrix.resize(3, 3);
        legacy_matrix.set(0.0f);
        legacy_matrix(2, 2) = 7.0f;
        legacy_matrix_rows = legacy_matrix;
        require_close(legacy_matrix_rows[2][2], 7.0f, "legacy float** conversion refreshes row pointers");

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

        matrix rank4(2, 2, 2, 2);
        require_throws([&]() { rank4(0, 0, 0, 2) = 0.0f; }, "rank-4 access should reject an index at the logical bound");

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

        matrix original = make_matrix("1, 2; 3, 4");
        matrix copied;
        copied.copy(original);
        require_matrix_close(copied, original, "copy()");

        matrix strided_submatrix(2, 4);
        strided_submatrix.set(-1000.0f);
        strided_submatrix.resize(2, 2);
        strided_submatrix.submatrix(make_matrix("1, 2, 3; 4, 5, 6; 7, 8, 9"), {1, 1, 2, 2});
        require_matrix_close(strided_submatrix, make_matrix("5, 6; 8, 9"), "submatrix() row-gapped destination");

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

        const matrix & const_scalar = scalar;
        require_close(static_cast<float>(const_scalar), 0.0f, "const scalar conversion");

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
        dynamic_stack.append(first);
        dynamic_stack.append(second);
        dynamic_stack.append(make_matrix("5, 6"));
        require_shape(dynamic_stack, {3, 2}, "append(matrix) grows uninitialized matrix");
        require_matrix_close(dynamic_stack, make_matrix("1, 2; 3, 4; 5, 6"), "append(matrix)");

        matrix dynamic_vector;
        dynamic_vector.append(7.0f);
        dynamic_vector.append(8.0f);
        dynamic_vector.append(9.0f);
        require_matrix_close(dynamic_vector, make_matrix("7, 8, 9"), "append(float)");

        matrix reserved;
        reserved.reserve(4, 2);
        reserved.append(first);
        reserved.append(second);
        reserved.clear();
        require_shape(reserved, {0, 2}, "clear() preserves slice shape");
        reserved.append(make_matrix("9, 10"));
        require_shape(reserved, {1, 2}, "reserve()/clear()/append() shape");
        require_matrix_close(reserved[0], make_matrix("9, 10"), "reserve()/clear()/append()");

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

        matrix dense_weights = make_matrix("3, 4, 5; 6, 7, 8");

        matrix dense_forward_result;
        dense_forward_result.dense_forward(make_matrix("1, 2"), dense_weights);
        require_matrix_close(dense_forward_result, make_matrix("15, 18, 21"), "dense_forward()");

        matrix dense_backward_input_result;
        dense_backward_input_result.dense_backward_input(dense_weights, make_matrix("1, 2, 3"));
        require_matrix_close(dense_backward_input_result, make_matrix("26, 44"), "dense_backward_input()");

        matrix inverse = make_matrix("4, 7; 2, 6");
        inverse.inv();
        require_matrix_close(inverse, make_matrix("0.6, -0.7; -0.2, 0.4"), "inv()", 1e-3f);

        matrix strided_inverse(2, 4);
        strided_inverse.set(-1000.0f);
        strided_inverse.resize(2, 2);
        strided_inverse.copy(make_matrix("4, 7; 2, 6"));
        strided_inverse.inv();
        require_matrix_close(strided_inverse, make_matrix("0.6, -0.7; -0.2, 0.4"), "inv() row-gapped", 1e-3f);

#if defined(__APPLE__)
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

        matrix rank_one = make_matrix("1, 2; 2, 4");
        require_close(rank_one.matrank(), 1.0f, "matrank()");

        matrix trace_and_determinant = make_matrix("1, 2; 3, 4");
        require_close(trace_and_determinant.trace(), 5.0f, "trace()");
        require_close(trace_and_determinant.det(), -2.0f, "det()");

        matrix pseudoinverse;
        pseudoinverse.pinv(make_matrix("1, 0; 0, 2; 0, 0"));
        require_matrix_close(pseudoinverse, make_matrix("1, 0, 0; 0, 0.5, 0"), "pinv()", 1e-3f);

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
#endif
    }

    void test_image()
    {
        matrix gaussian_kernel;
        gaussian_kernel.gaussian(1.0f);
        require_shape(gaussian_kernel, {7, 7}, "gaussian()");
        require_close(gaussian_kernel.sum(), 1.0f, "gaussian() normalization", 1e-3f);

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

#if defined(__APPLE__)
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
#endif

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

        matrix small = make_matrix("1, 2; 3, 4");
        matrix up;
        up.upsample(small);
        require_matrix_close(up, make_matrix("1, 1, 2, 2; 1, 1, 2, 2; 3, 3, 4, 4; 3, 3, 4, 4"), "upsample()");

        matrix reflected = make_matrix("0, 0, 0, 0; 0, 1, 2, 0; 0, 3, 4, 0; 0, 0, 0, 0");
        reflected.fillReflect101Border(1, 1);
        require_matrix_close(reflected, make_matrix("4, 3, 4, 3; 2, 1, 2, 1; 4, 3, 4, 3; 2, 1, 2, 1"), "fillReflect101Border()");

        matrix extended = make_matrix("0, 0, 0, 0; 0, 1, 2, 0; 0, 3, 4, 0; 0, 0, 0, 0");
        extended.fillExtendBorder(1, 1);
        require_matrix_close(extended, make_matrix("1, 1, 2, 2; 1, 1, 2, 2; 3, 3, 4, 4; 3, 3, 4, 4"), "fillExtendBorder()");
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
        const matrix & const_tensor = tensor;
        require_equal(const_tensor.labels(0).at(0), "front", "const labels() first label");
        require_equal(const_tensor.labels(0).at(1), "back", "const labels() second label");

        matrix front = tensor[0];
        matrix back = tensor["back"];
        matrix const_front = const_tensor[0];
        const std::string back_label = "back";
        matrix const_back = const_tensor[back_label];
        matrix const_back_char = const_tensor["back"];

        require_shape(front, {3, 4}, "rank-3 slice shape");
        require_close(front.sum(), 78.0f, "slice sum()");
        require_close(back.sum(), 222.0f, "labeled slice sum()");
        require_close(const_front.sum(), 78.0f, "const numeric slice sum()");
        require_close(const_back.sum(), 222.0f, "const labeled slice sum()");
        require_close(const_back_char.sum(), 222.0f, "const labeled char slice sum()");
        require_close(dot(front, back), 1586.0f, "dot(slice, slice)");

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

        matrix source(std::vector<int>{channels, rows, cols});
        fill_sequence(source);
        matrix target(std::vector<int>{channels, rows, cols});
        matrix reduced(channels);

        double checksum = 0.0;

        double contiguous_copy_ms = measure_ms([&]()
        {
            for(int i = 0; i < iterations; ++i)
                target.copy(source);
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

        double row_gapped_copy_ms = measure_ms([&]()
        {
            for(int i = 0; i < row_gap_iterations; ++i)
                row_gapped_target.copy(row_gapped_source);
            checksum += row_gapped_target.sum();
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
                  << " contiguous_copy_ms=" << contiguous_copy_ms
                  << " slice_copy_ms=" << slice_copy_ms
                  << " slice_apply_ms=" << slice_apply_ms
                  << " full_sum_ms=" << full_sum_ms
                  << " slice_sum_ms=" << slice_sum_ms
                  << " temporary_views_ms=" << temporary_views_ms
                  << " sum_last_two_dimensions_ms=" << sum_last_two_dimensions_ms
                  << " dot_ms=" << dot_ms
                  << " serialization_ms=" << serialization_ms
                  << " row_gapped_copy_ms=" << row_gapped_copy_ms
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
