#include "ikaros.h"

using namespace ikaros;

class LinearSplines: public Module
{
public:
    matrix     input;
    matrix     output;
    matrix     points;

    void
    Init()
    {
        Bind(points, "points");
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");

        if(points.size_x() < 2 || points.size_y() < 2)
            Notify(msg_fatal_error, "Not enough point data");
    }


    void
    Tick()
    {
        for(int i=0; i<input.size_x(); i++)
        {
            float x = input(i);
            int col = 2*(i%(points.size_x()/2));
            
            if(x < points(0,col))
            {
                output[i] = points(0,col+1);
                continue;
            }
            
            float y = points(0,col+1);
            for(int j=0; j<points.size_y()-1; j++)
            {
                float x0 = points(j,col);
                float x1 = points(j+1,col);
                
                if(x0 != x1)
                {
                    if(x0 <= x && x < x1)
                    {
                        float y0 = points(j,col+1);
                        float y1 = points(j+1,col+1);
                        float k = (y1-y0)/(x1-x0);
                        y = y0 + k*(x-x0);
                        break;
                    }
                    else
                        y = points(j+1,col+1);
                }
            }
            output(i) = y;
        }
    }
};

INSTALL_CLASS(LinearSplines)

