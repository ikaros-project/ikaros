#include "../../utilities.cc"
#include "../../xml.cc"
#include "../../dictionary.cc"
#include "../../range.cc"
#include "../../matrix.cc"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace ikaros;

namespace
{
void
require(bool condition, const std::string & message)
{
    if(!condition)
        throw std::runtime_error(message);
}


void
require_close(float actual, float expected, const std::string & message,
              float tolerance = 1e-5f)
{
    if(std::fabs(actual - expected) > tolerance)
        throw std::runtime_error(message + ": expected " +
                                 std::to_string(expected) + ", got " +
                                 std::to_string(actual));
}


void
require_matrix_close(const matrix & actual, const matrix & expected,
                     const std::string & message)
{
    require(actual.shape() == expected.shape(), message + ": shape mismatch");
    for(auto index = actual.get_range(); index.more(); ++index)
        require_close(actual.at(index.index()), expected.at(index.index()), message);
}
}


int
main()
{
    try
    {
        matrix uninitialized;
        require(uninitialized.empty() && uninitialized.is_uninitialized(),
                "default matrix invariants");
        require(uninitialized.data() == nullptr,
                "empty matrices expose no data pointer");

        matrix left = {{1, 2, 3}, {-1, -2, -3}};
        matrix right = {{1, 1}, {2, 2}, {3, 3}};
        matrix product;
        product.matmul(left, right);
        require_matrix_close(product, matrix("14, 14; -14, -14"),
                             "matrix multiplication");

        const matrix & const_product = product;
        auto const_row = const_product[0];
        require_close(const_row(1), 14.0f, "const slice access");

        matrix string_target = matrix("1, 2");
        matrix string_alias = string_target;
        string_target = std::string("[3, 4]");
        require_matrix_close(string_alias, matrix("3, 4"),
                             "atomic string assignment preserves aliases");

        matrix ranged = matrix("1, 2, 3, 4");
        range short_target("[0:2]");
        range long_source("[0:3]");
        bool cardinality_rejected = false;
        try
        {
            ranged.copy(ranged, short_target, long_source);
        }
        catch(const std::invalid_argument &)
        {
            cardinality_rejected = true;
        }
        require(cardinality_rejected, "ranged copy cardinality validation");

        matrix transposed = matrix("1, 2; 3, 4");
        transposed.transpose(transposed);
        require_matrix_close(transposed, matrix("1, 3; 2, 4"),
                             "in-place transpose");

        matrix gaussian;
        gaussian.gaussian(1.0f);
        require_close(gaussian.sum(), 1.0f, "normalized Gaussian", 1e-4f);
        bool invalid_sigma_rejected = false;
        try
        {
            gaussian.gaussian(0.0f);
        }
        catch(const std::invalid_argument &)
        {
            invalid_sigma_rejected = true;
        }
        require(invalid_sigma_rejected, "Gaussian sigma validation");

        matrix row_gapped(2, 4);
        row_gapped.set(-1.0f);
        row_gapped.resize(2, 2);
        row_gapped.copy(matrix("1, 2; 3, 4"));
        require(!row_gapped.is_contiguous(), "row-gapped layout detection");
        require_matrix_close(row_gapped, matrix("1, 2; 3, 4"),
                             "row-gapped logical traversal");

        std::cout << "MATRIX UTILITY TEST OK\n";
        return 0;
    }
    catch(const std::exception & error)
    {
        std::cerr << "MATRIX UTILITY TEST FAILED: " << error.what() << '\n';
        return 1;
    }
}
