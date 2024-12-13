#include "ikaros.h"

using namespace ikaros;


// Global variables
//const int max_gaussians = 3; // Maximum number of Gaussian components
//const int MAX_CHANNELS = 3;  // Maximum number of color channels
//const int WIDTH = 6;       // Image width
//const int HEIGHT = 4;      // Image height
//const float learning_rate = 0.01f;
//const float initial_variance = 100.0f;
//const float threshold = 2.5f;



inline float 
squared_difference(float a, float b) 
{
    return (a - b) * (a - b);
}



class StaufferGrimson: public Module
{
public:

    void 		
    Init()
    {
        Profiler p;
        p.Start();

        Bind(input, "INPUT");
        Bind(output, "OUTPUT");

        Bind(weights, "WEIGHTS");
        Bind(means, "MEANS");
        Bind(variances, "VARIANCES");

        Bind(max_gaussians, "max_gaussians");
        Bind(initial_variance, "initial_variance");
        Bind(learning_rate, "learning_rate");
        Bind(threshold, "threshold");

        // Initialize the background model

        for (int c = 0; c < input.size_z(); c++)
            for (int j = 0; j < input.size_y(); j++)
                    for (int i = 0; i < input.size_x(); i++)
                        for (int g = 0; g < max_gaussians; g++) // .as_int()
                        {
                            weights(c, j, i, g) = (g == 0) ? 1.0f : 0.0f; // Initial weight for the first Gaussian
                            means(c, j, i, g) = input(c,i);      // Initialize mean with the current pixel value
                            variances(c, j, i, g) = initial_variance;     // Initialize variance
                        }
        p.Stop().Print();
    }



    void 		
    Tick()
    {
        for (int j = 0; j < input.size_y(); j++)
            for (int i = 0; i < input.size_x(); i++)
        {
            bool isForeground = false;

            for (int c = 0; c < input.size_z(); ++c) 
            {
                bool matched = false;

                for (int g = 0; g < max_gaussians; ++g) {
                    float diff = input(c, j, i) - means(c, j, i, g);
                    float var = variances(c, j, i, g);
                    float distance = squared_difference(diff, 0) / var;

                    if (distance < threshold * threshold) {
                        // Match found: Update Gaussian
                        matched = true;

                        float lr = learning_rate * weights(c, j, i, g);
                        weights(c, j, i, g) += learning_rate * (1.0f - weights(c, j, i, g));
                        means(c, j, i, g) += lr * diff;
                        variances(c, j, i, g) += lr * (diff * diff - var);
                    } else {
                        // No match: Update weight only
                        weights(c, j, i, g) -= learning_rate * weights(c, j, i, g);
                    }
                }

                if (!matched) 
                {
                    // Replace the weakest Gaussian if no match is found
                    int weakestIdx = 0;
                    float minWeight = weights(c, j, i, 0);
                    for (int g = 1; g < max_gaussians; ++g) 
                    {
                        if (weights(c, j, i, g) < minWeight) 
                        {
                            minWeight = weights(c, j, i, g);
                            weakestIdx = g;
                        }
                    }

                    // Replace the weakest Gaussian
                    weights(c, j, i, weakestIdx) = 1.0f / max_gaussians;
                    means(c, j, i, weakestIdx) = input(c, j, i);
                    variances(c, j, i, weakestIdx) = initial_variance;
                }
            }

            // Determine if the pixel is foreground
            for (int c = 0; c < input.size_z(); ++c) 
            {
                for (int g = 0; g < max_gaussians; ++g) 
                {
                    if (weights(c, j, i, g) > 0.5f) {
                        float diff = input(c, j, i) - means(c, j, i, g);
                        float var = variances(c, j, i, g);
                        float distance = squared_difference(diff, 0) / var;
                        if (distance < threshold * threshold) {
                            isForeground = false;
                            break;
                        }
                    }
                }
            }

            output(j, i) = (isForeground ? 1 : 0);

        }
    }


private:

    matrix input; // [C][Y][X]

    matrix weights; // [MAX_CHANNELS][WIDTH * HEIGHT][max_gaussians];      // Weights of each Gaussian
    matrix means; // [MAX_CHANNELS][WIDTH * HEIGHT][max_gaussians];        // Means of each Gaussian
    matrix variances; // [MAX_CHANNELS][WIDTH * HEIGHT][max_gaussians];    // Variances of each Gaussian

    matrix output;

    parameter max_gaussians;
    parameter initial_variance;
    parameter learning_rate;
    parameter threshold;
};

INSTALL_CLASS(StaufferGrimson)