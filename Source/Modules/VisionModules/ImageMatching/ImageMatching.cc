#include "ikaros.h"
#include <Accelerate/Accelerate.h>
using namespace ikaros;



class ImageMatching: public Module
{
    matrix      left;
    matrix      right;
    matrix      profile;
    matrix      output;

    matrix      focus;
    matrix      target;
    matrix      kernel;

    parameter focus_size;
    parameter search_size;

    int focus_top;      // Focus area – target
    int focus_bottom;
    int focus_left;
    int focus_right;

    int search_top; // Upper left coordinate forsearch area
    int search_bottom;
    int search_left;
    int search_right;

    void Init()
    {
        Bind(left, "LEFT");
        Bind(right, "RIGHT");
        Bind(output, "OUTPUT"); // Single disparity value: -0.5 to 0.5
        Bind(profile, "PROFILE");
        Bind(focus, "FOCUS"); // Focus area
        Bind(target, "TARGET"); // Target area

        Bind(focus_size, "focus_size");
        Bind(search_size, "search_size");


    }



struct Disparity {
    float dx;
    float dy;
};


/*
Disparity ScanImage()
{
    int patch_w = focus_right - focus_left;
    int patch_h = focus_bottom - focus_top;
    int patch_size = patch_w * patch_h;

    int best_match_x = left.size_x() / 2;
    int best_match_y = left.size_y() / 2;
    float best_match = -std::numeric_limits<float>::max();

    for (int y = search_top; y <= search_bottom - patch_h; ++y)
    {
        for (int x = search_left; x <= search_right - patch_w; ++x)
        {
            float sum_xy = 0.0f;
            float sum_xx = 0.0f;
            float sum_yy = 0.0f;

            float mean_x = 0.0f;
            float mean_y = 0.0f;

            // Compute means
            for (int j = 0; j < patch_h; ++j)
            {
                for (int i = 0; i < patch_w; ++i)
                {
                    mean_x += left(focus_top + j, focus_left + i);
                    mean_y += right(y + j, x + i);
                }
            }

            mean_x /= float(patch_size);
            mean_y /= float(patch_size);

            // Compute NCC numerator and denominators
            for (int j = 0; j < patch_h; ++j)
            {
                for (int i = 0; i < patch_w; ++i)
                {
                    float left_val = left(focus_top + j, focus_left + i) - mean_x;
                    float right_val = right(y + j, x + i) - mean_y;

                    sum_xy += left_val * right_val;
                    sum_xx += left_val * left_val;
                    sum_yy += right_val * right_val;
                }
            }

            float denom = std::sqrt(sum_xx * sum_yy);
            float match = (denom > 1e-6f) ? (sum_xy / denom) : -1.0f;

            // Update profile (optional)
            if (x < profile.size() && match > profile(x))
                profile(x) = match;

            // Track best match
            if (match > best_match)
            {
                best_match = match;
                best_match_x = x;
                best_match_y = y;
            }
        }
    }

    // Return disparity relative to image center, normalized to [-0.5, 0.5]
    int center_x = left.size_x() / 2;
    int center_y = left.size_y() / 2;

    int patch_half_w = patch_w / 2;
    int patch_half_h = patch_h / 2;

    float dx = float(best_match_x + patch_half_w - center_x) / float(right.size_x());
    float dy = float(best_match_y + patch_half_h - center_y) / float(right.size_y());

    // You can return dx only (as before) or dy separately if you need 2D disparity
    std::cout << "Best match at (" << best_match_x << ", " << best_match_y << ") with NCC = " << best_match << "\n";
    std::cout << "Disparity dx = " << dx << ", dy = " << dy << "\n";

    return Disparity{dx, dy};  // or return both dx, dy if needed
}
*/

    void Tick()
    {
        // Recalculate focus area /when focus_size or search_size changes)

            int h = left.size_y();

            focus_top = 0.5*h - 0.5*focus_size*h;
            focus_bottom = 0.5*h + 0.5*focus_size*h;
            focus_left = 0.5*left.size_x() - 0.5*focus_size*h;
            focus_right = 0.5*left.size_x() + 0.5*focus_size*h;

            h = right.size_y();
            search_top =    0.5*h - 0.5*search_size*h;
            search_bottom = 0.5*h + 0.5*search_size*h;
            search_left = 0;
            search_right = right.size_x() - (focus_right-focus_left) - 1;
            if(search_bottom - search_top < 1)
                search_bottom = search_top+1;

//        std::cout << "Focus area: " << focus_top << " " << focus_bottom << " " << focus_left << " " << focus_right << std::endl;
//        std::cout << "Search area: " << search_top << " " << search_bottom << " " << search_left << " " <<search_right << std::endl;


        profile.reset();

        int focus_width = focus_right - focus_left;
        int focus_height = focus_bottom - focus_top;

        if(kernel.size_x() != focus_width || kernel.size_y() != focus_height)
            kernel.realloc(focus_height, focus_width); // Reallocate kernel if size has changed

       kernel.submatrix(left, {focus_left, focus_top, focus_width, focus_height});
       match best_match = right.search(kernel, {search_left, search_top, search_right-search_left, search_bottom-search_top});

       // std::cout << "Match found at: (" << best_match.x << ", " << best_match.y << ") with score: " << best_match.score << std::endl;

           // Return disparity relative to image center, normalized to [-0.5, 0.5]

    int patch_w = focus_right - focus_left;
    int patch_h = focus_bottom - focus_top;

    int center_x = left.size_x() / 2;
    int center_y = left.size_y() / 2;

    int patch_half_w = patch_w / 2;
    int patch_half_h = patch_h / 2;

    float dx = float(best_match.x + patch_half_w - center_x) / float(right.size_x());
    float dy = float(best_match.y + patch_half_h - center_y) / float(right.size_y());

        output(0) = dx;    // TODO: output = {dx, dy};
        output(1) = dy;


        float low = 0.5-focus_size;
        float high = 0.5+focus_size;

        focus[0].copy({low, low, high, low});  // TODO: Fix "=" assignment
        focus[1].copy({high, low, high, high}); 
        focus[2].copy({high, high, low, high});
        focus[3].copy({low, high, low, low}); 
    }
};


INSTALL_CLASS(ImageMatching)

