#include "ikaros.h"
#include <cmath>

#include <Accelerate/Accelerate.h>

using namespace ikaros;

class CannyEdgeDetector : public Module 
{
    static constexpr float direction_horizontal = 0.0;    // 0 radians (0°)
    static constexpr float direction_diagonal_45 = 1.0;   // π/4 radians (45°)
    static constexpr float direction_vertical = 2.0;      // π/2 radians (90°)
    static constexpr float direction_diagonal_135 = 3.0;  // 3π/4 radians (135°)
    static constexpr float angle_threshold_0 = M_PI / 8;      // π/8 ≈ 0.3927 radians
    static constexpr float angle_threshold_45 = 3 * M_PI / 8; // 3π/8 ≈ 1.1781 radians
    static constexpr float angle_threshold_90 = 5 * M_PI / 8; // 5π/8 ≈ 1.9635 radians
    static constexpr float angle_threshold_135 = 7 * M_PI / 8; // 7π/8 ≈ 2.7489 radians
    static constexpr float weak_edge = 1.0;   // Weak edge marker
    static constexpr float strong_edge = 2.0; // Strong edge marker

    matrix input;          // Grayscale input image
    matrix output;         // Binary edge map output
    matrix smoothed;       // Image after Gaussian smoothing
    matrix grad_x;         // Gradient in x-direction
    matrix grad_y;         // Gradient in y-direction
    matrix magnitude;      // Gradient magnitude
    matrix direction;      // Quantized gradient direction
    matrix suppressed;     // After non-maximum suppression
    matrix thresholded;    // After double thresholding
    matrix gaussian;       // Gaussian kernel

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
        suppressed.realloc(rows, cols);
        thresholded.realloc(rows, cols);
        output.realloc(rows, cols);
        gaussian.gaussian(sigma); // Create gaussian matrix with the specified sigma
    }



    void Tick() 
    {
        int rows = input.rows();
        int cols = input.cols();

        smoothed.conv(input, gaussian);  // Gaussian smoothing
        grad_x.conv(smoothed, sobel_x);  // Compute gradients using Sobel filters
        grad_y.conv(smoothed, sobel_y);
        magnitude.hypot(grad_x, grad_y);// Compute gradient magnitude and direction
        direction.atan2(grad_y, grad_x); 
       
        float* dir_ptr = direction.data();
        size_t total = rows * cols;
        for (size_t idx = 0; idx < total; ++idx) {
            float angle = dir_ptr[idx];
            if (angle < 0)
                angle += M_PI; // Map to [0, π)
            if (angle < angle_threshold_0 || angle >= angle_threshold_135)
                dir_ptr[idx] = direction_horizontal;  // 0 radians
            else if (angle < angle_threshold_45)
                dir_ptr[idx] = direction_diagonal_45; // π/4 radians
            else if (angle < angle_threshold_90)
                dir_ptr[idx] = direction_vertical;    // π/2 radians
            else
                dir_ptr[idx] = direction_diagonal_135;// 3π/4 radians
        }

        // Non-maximum suppression
        suppressed.copy(magnitude);
        for (int i = 1; i < rows - 1; i++)
            for (int j = 1; j < cols - 1; j++)
            {
                float mag = magnitude(i, j);
                float dir = direction(i, j);
                float n1, n2;
                if (dir == direction_horizontal) // 0 radians
                {
                    n1 = magnitude(i, j-1);
                    n2 = magnitude(i, j+1);
                }
                else if (dir == direction_diagonal_45) // π/4 radians
                {
                    n1 = magnitude(i+1, j-1);
                    n2 = magnitude(i-1, j+1);
                }
                else if (dir == direction_vertical) // π/2 radians
                {
                    n1 = magnitude(i-1, j);
                    n2 = magnitude(i+1, j);
                }
                else // 3π/4 radians (direction_diagonal_135)
                {
                    n1 = magnitude(i-1, j-1);
                    n2 = magnitude(i+1, j+1);
                }
                if (mag < n1 || mag < n2)
                    suppressed(i, j) = 0;
            }

        // Double thresholding
        thresholded.reset();
        for (int i = 0; i < rows; i++)
            for (int j = 0; j < cols; j++)
            {
                float mag = suppressed(i, j);
                if (mag >= high_threshold)
                    thresholded(i, j) = strong_edge; // Strong edge
                else if (mag >= low_threshold)
                    thresholded(i, j) = weak_edge;   // Weak edge
            }

        // Edge tracking by hysteresis
        output.reset();
        for (int i = 1; i < rows - 1; i++)
            for (int j = 1; j < cols - 1; j++)
                if (thresholded(i, j) == strong_edge) 
                {
                    output(i, j) = 1;
                    for (int di = -1; di <= 1; di++)  // Check 8-connected neighbors for weak edges
                        for (int dj = -1; dj <= 1; dj++)
                            if (thresholded(i + di, j + dj) == weak_edge)
                                output(i + di, j + dj) = 1;
                }
    }
};

// Register the module with Ikaros
INSTALL_CLASS(CannyEdgeDetector)

