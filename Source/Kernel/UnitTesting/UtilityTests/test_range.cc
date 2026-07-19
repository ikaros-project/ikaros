
//

// Everything is included here to allow compilation of this file on its own

#include <string>
#include <iostream>
#include "../../utilities.cc"
#include "../../range.h"
#include "../../range.cc"

using namespace ikaros;



bool 
ResolveConnection(range & output, range & source, range & target)
{
    source.extend(output.rank());
    source.fill(output);
    range reduced_source = source.strip().trim();

    if(target.empty())
        target = reduced_source;
    else
    {
        int j=0;
        for(int i=0; i<target.rank()-1; i++)    // CHECK EMPTY DIMENSION
            if(target.empty(i) && j<reduced_source.rank())
            {
                target.set(i, reduced_source.start(j),
                           reduced_source.stop(j), reduced_source.step(j));
                reduced_source.set(j, 0, 0, 0); // mark as used
                j++;
            }

        int s = 1;
        for(int i=0; i<reduced_source.rank(); i++)
        {
            int si = reduced_source.size(i);
            s *= (si >0?si:1);
        }

        if(target.empty(target.rank()-1) && j<reduced_source.rank())
            target.set(target.rank()-1, 0, s, 1);
    }
    return source.size() == target.size();
}



int
main()
{
    range matrix("[:3][:5][:5]");
    range source("[][2:4]][0:3]");
    range target("[2][][][]");    // Number of reduced dims or less - filled in with rest must be last (mostly) ********

    if(ResolveConnection(matrix, source, target))
    {
        source.print("source");
        target.print("target");
    }
    else
        std::cout << "-- resolution failed" << std::endl;
}
