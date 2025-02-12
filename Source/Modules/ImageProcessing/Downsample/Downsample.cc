#include "ikaros.h"

using namespace ikaros;


class Downsample: public Module
{
public:

    void 		
    Init()
    {
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
    }



    void 		
    Tick()
    {
        if(input.rank() == 2)
        {
            size_t originalWidth = input.size_x();
            size_t originalHeight = input.size_y();
            size_t newWidth = originalWidth / 2;
            size_t newHeight = originalHeight / 2;

            const float* originalPtr = input.data();
            float* downsampledPtr = output.data();

            const float* rowEnd = originalPtr + originalHeight * originalWidth;
            for (const float* rowPtr = originalPtr; rowPtr < rowEnd; rowPtr += 2 * originalWidth) 
            {
                const float* colEnd = rowPtr + originalWidth;
                for (const float* colPtr = rowPtr; colPtr < colEnd; colPtr += 2)
                    *downsampledPtr++ = (*colPtr + *(colPtr + 1) + *(colPtr + originalWidth) + *(colPtr + originalWidth + 1)) / 4.0f;
            }
        }
        else if(input.rank() == 3)
        {
            for(int z=0; z<input.size_z(); z++)
            {
                size_t originalWidth = input.size_x();
                size_t originalHeight = input.size_y();
                size_t newWidth = originalWidth / 2;
                size_t newHeight = originalHeight / 2;

                const float* originalPtr = input[z];
                float* downsampledPtr = output[z];

                const float* rowEnd = originalPtr + originalHeight * originalWidth;
                for (const float* rowPtr = originalPtr; rowPtr < rowEnd; rowPtr += 2 * originalWidth) 
                {
                    const float* colEnd = rowPtr + originalWidth;
                    for (const float* colPtr = rowPtr; colPtr < colEnd; colPtr += 2)
                        *downsampledPtr++ = (*colPtr + *(colPtr + 1) + *(colPtr + originalWidth) + *(colPtr + originalWidth + 1)) / 4.0f;
                } 
            }
        }

    }


private:

    matrix input;
    matrix output;
};

INSTALL_CLASS(Downsample)

