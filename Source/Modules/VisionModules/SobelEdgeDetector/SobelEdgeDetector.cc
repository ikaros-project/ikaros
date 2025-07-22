#include "ikaros.h"
#include <cmath>

#include <Accelerate/Accelerate.h>

using namespace ikaros;

class SobelEdgeDetector : public Module 
{
    matrix input;          // Grayscale input image
    matrix output;         // Binary edge map output
    matrix smoothed;       // Image after Gaussian smoothing
    matrix grad_x;         // Gradient in x-direction
    matrix grad_y;         // Gradient in y-direction
    matrix magnitude;      // Gradient magnitude
    matrix gaussian;       // Gaussian kernel

    matrix sobel_x = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} }; // Sobel x kernel
    matrix sobel_y = { {-1, -2, -1}, {0, 0, 0}, {1, 2, 1} }; // Sobel y kernel

    parameter sigma;            // Standard deviation for Gaussian blur
  
public:
    void 
    Init() 
    {
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
        Bind(sigma, "sigma");

        int rows = input.rows();  // Get input image dimensions
        int cols = input.cols();

        smoothed.realloc(rows, cols); // realloc all intermediate matrices
        grad_x.realloc(rows, cols);
        grad_y.realloc(rows, cols);
        //output.realloc(rows, cols);
        gaussian.gaussian(sigma); // Create gaussian matrix with the specified sigma
    }



    void Tick() 
    {
        smoothed.conv(input, gaussian);  // Gaussian smoothing
        grad_x.conv(smoothed, sobel_x);  // Compute gradients using Sobel filters
        grad_y.conv(smoothed, sobel_y);
        output.hypot(grad_x, grad_y);// Compute gradient magnitude and direction
    }
};

// Register the module with Ikaros
INSTALL_CLASS(SobelEdgeDetector)

