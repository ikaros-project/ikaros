#include "ikaros.h"

#include <cmath>

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

        matrix a = make_matrix("1, 2, 3");
        matrix b = make_matrix("4, 5, 6");
        require_close(dot(a, b), 32.0f, "dot()");
    }

    void test_scalar_and_shape()
    {
        matrix values = make_matrix("1, 2; 3, 4");
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

        matrix original = make_matrix("1, 2; 3, 4");
        matrix copied;
        copied.copy(original);
        require_matrix_close(copied, original, "copy()");

        matrix transposed;
        original.transpose(transposed);
        require_matrix_close(transposed, make_matrix("1, 3; 2, 4"), "transpose()");

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

        matrix inverse = make_matrix("4, 7; 2, 6");
        inverse.inv();
        require_matrix_close(inverse, make_matrix("0.6, -0.7; -0.2, 0.4"), "inv()", 1e-3f);
    }

    void test_image()
    {
        matrix gaussian_kernel;
        gaussian_kernel.gaussian(1.0f);
        require_shape(gaussian_kernel, {7, 7}, "gaussian()");
        require_close(gaussian_kernel.sum(), 1.0f, "gaussian() normalization", 1e-3f);

        matrix image = make_matrix("1, 2, 3; 4, 5, 6; 7, 8, 9");
        matrix kernel = make_matrix("1, 0; 0, -1");

        matrix corr(2, 2);
        corr.corr(image, kernel);
        require_matrix_close(corr, make_matrix("-4, -4; -4, -4"), "corr()");

        matrix conv(2, 2);
        conv.conv_slow(image, kernel);
        require_matrix_close(conv, make_matrix("4, 4; 4, 4"), "conv_slow()");

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
        require_throws([&]() { wrong_corr.corr(image, kernel); }, "corr() should throw for wrong-sized initialized destination");

        matrix wrong_conv_slow(3, 3);
        require_throws([&]() { wrong_conv_slow.conv_slow(image, kernel); }, "conv_slow() should throw for wrong-sized initialized destination");

        matrix wrong_conv(2, 2);
        matrix conv_image = make_matrix("1, 2, 3; 4, 5, 6; 7, 8, 9");
        matrix conv_kernel = make_matrix("1, 0; 0, -1");
        require_throws([&]() { wrong_conv.conv(conv_image, conv_kernel); }, "conv() should throw for wrong-sized initialized destination");

        matrix down_source = make_matrix("1, 2, 3, 4; 5, 6, 7, 8; 9, 10, 11, 12; 13, 14, 15, 16");
        matrix wrong_downsample(3, 3);
        require_throws([&]() { wrong_downsample.downsample(down_source); }, "downsample() should throw for wrong-sized initialized destination");

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
