#include "ikaros.h"
#include <cmath>
#include <utility>
#include <vector>

#include <Accelerate/Accelerate.h>

using namespace ikaros;

class CannyEdgeDetector : public Module 
{
    static constexpr float direction_horizontal = 0.0;    // 0 radians (0°)
    static constexpr float direction_diagonal_45 = 1.0;   // π/4 radians (45°)
    static constexpr float direction_vertical = 2.0;      // π/2 radians (90°)
    static constexpr float direction_diagonal_135 = 3.0;  // 3π/4 radians (135°)
    static constexpr float tan_pi_over_8 = 0.41421356237f;
    static constexpr float tan_3pi_over_8 = 2.41421356237f;
    static constexpr float weak_edge = 1.0;   // Weak edge marker
    static constexpr float strong_edge = 2.0; // Strong edge marker

    matrix input;          // Grayscale input image
    matrix output;         // Binary edge map output
    matrix smoothed;       // Image after Gaussian smoothing
    matrix grad_x;         // Gradient in x-direction
    matrix grad_y;         // Gradient in y-direction
    matrix magnitude;      // Gradient magnitude
    matrix direction;      // Quantized gradient direction
    matrix thresholded;    // After double thresholding
    matrix gaussian;       // Gaussian kernel
    std::vector<std::pair<int, int>> edge_stack;

    matrix sobel_x = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} }; // Sobel x kernel
    matrix sobel_y = { {-1, -2, -1}, {0, 0, 0}, {1, 2, 1} }; // Sobel y kernel

    parameter sigma;            // Standard deviation for Gaussian blur
    parameter low_threshold;    // Low threshold for hysteresis
    parameter high_threshold;   // High threshold for hysteresis

public:
    void 
    Init() 
    {
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
        Bind(sigma, "sigma");
        Bind(low_threshold, "low_threshold");
        Bind(high_threshold, "high_threshold");

        int rows = input.rows();  // Get input image dimensions
        int cols = input.cols();

        smoothed.realloc(rows, cols); // realloc all intermediate matrices
        grad_x.realloc(rows, cols);
        grad_y.realloc(rows, cols);
        magnitude.realloc(rows, cols);
        direction.realloc(rows, cols);
        thresholded.realloc(rows, cols);
        output.realloc(rows, cols);
        gaussian.gaussian(sigma); // Create gaussian matrix with the specified sigma
        edge_stack.reserve(rows * cols);
    }



    void Tick() 
    {
        int rows = input.rows();
        int cols = input.cols();

        smoothed.conv(input, gaussian);  // Gaussian smoothing
        grad_x.conv(smoothed, sobel_x);  // Compute gradients using Sobel filters
        grad_y.conv(smoothed, sobel_y);
        magnitude.hypot(grad_x, grad_y); // Compute gradient magnitude
        const float* grad_x_ptr = grad_x.data();
        const float* grad_y_ptr = grad_y.data();
        float* dir_ptr = direction.data();
        size_t total = rows * cols;
        for (size_t idx = 0; idx < total; ++idx)
        {
            float gx = grad_x_ptr[idx];
            float gy = grad_y_ptr[idx];
            float abs_gx = std::fabs(gx);
            float abs_gy = std::fabs(gy);

            if (abs_gy <= abs_gx * tan_pi_over_8)
                dir_ptr[idx] = direction_horizontal;
            else if (abs_gy >= abs_gx * tan_3pi_over_8)
                dir_ptr[idx] = direction_vertical;
            else if ((gx >= 0) == (gy >= 0))
                dir_ptr[idx] = direction_diagonal_45;
            else
                dir_ptr[idx] = direction_diagonal_135;
        }

        // Non-maximum suppression and double thresholding
        thresholded.reset();
        for (int i = 1; i < rows - 1; i++)
            for (int j = 1; j < cols - 1; j++)
            {
                float mag = magnitude(i, j);
                float dir = direction(i, j);
                float n1, n2;

                if (dir == direction_horizontal)
                {
                    n1 = magnitude(i, j-1);
                    n2 = magnitude(i, j+1);
                }
                else if (dir == direction_vertical)
                {
                    n1 = magnitude(i-1, j);
                    n2 = magnitude(i+1, j);
                }
                else if (dir == direction_diagonal_45)
                {
                    n1 = magnitude(i+1, j-1);
                    n2 = magnitude(i-1, j+1);
                }
                else
                {
                    n1 = magnitude(i-1, j-1);
                    n2 = magnitude(i+1, j+1);
                }
                if (mag >= n1 && mag >= n2)
                {
                    if (mag >= high_threshold)
                        thresholded(i, j) = strong_edge;
                    else if (mag >= low_threshold)
                        thresholded(i, j) = weak_edge;
                }
            }

        // Edge tracking by hysteresis: follow complete 8-connected weak-edge chains from strong edges.
        output.reset();
        edge_stack.clear();

        for (int i = 1; i < rows - 1; i++)
            for (int j = 1; j < cols - 1; j++)
                if (thresholded(i, j) == strong_edge)
                {
                    output(i, j) = 1;
                    edge_stack.push_back({i, j});
                }

        while (!edge_stack.empty())
        {
            auto [i, j] = edge_stack.back();
            edge_stack.pop_back();

            for (int di = -1; di <= 1; di++)
                for (int dj = -1; dj <= 1; dj++)
                {
                    if (di == 0 && dj == 0)
                        continue;

                    int ni = i + di;
                    int nj = j + dj;

                    if (ni <= 0 || ni >= rows - 1 || nj <= 0 || nj >= cols - 1)
                        continue;

                    if (thresholded(ni, nj) == weak_edge && output(ni, nj) == 0)
                    {
                        output(ni, nj) = 1;
                        edge_stack.push_back({ni, nj});
                    }
                }
        }
    }
};

// Register the module with Ikaros
INSTALL_CLASS(CannyEdgeDetector)
