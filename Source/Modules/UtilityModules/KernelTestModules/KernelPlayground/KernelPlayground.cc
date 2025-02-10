
        //#define MATRIX_FULL_BOUNDS_CHECK
        //#define MATRIX_NO_BOUNDS_CHECK

#include "ikaros.h"

using namespace ikaros;

class KernelPlayground: public Module
{
    void SetParameters()
    {
   
    }

// BEFORE OPTIMIZATION: 73.73300 a
// AFTER OPTIMIZATION 0.28 s 250x speedup

    void Init()
    {

        matrix MM = {
        {4.0f, 7.0f, 2.0f},
        {3.0f, 6.0f, 1.0f},
        {2.0f, 5.0f, 8.0f}
     };


    matrix CC;

    CC.copy(MM);

    matrix XX(3,3);


    MM.print("M");

    MM.inv();

    MM.print("I");

    XX.matmul(CC,MM);

    XX.print("XX");

       matrix RR;
        RR.inv(MM);
        RR.print("Inverse");



        Profiler p;

        p.Reset().Start();
        matrix M(10, 1000, 1000);
        p.Stop().Print("Time allocate first:");  // 0.5

        M.test_fill();
        matrix X;

        M.info();
 
 
        p.Reset().Start();
        X.copy(M);
        p.Stop().Print("Time 1:");  // 0.5

        //M.print();
        //X.print();


        p.Reset().Start();
        X.copy(M);
        p.Stop().Print("Time 2:");  // 0.28 => 0.009 med std::copy_n
        p.Reset().Start();
        X.copy(M);
        p.Stop().Print("Time 3:");  
        p.Reset().Start();
        X.copy(M);
        p.Stop().Print("Time 4:");  
    /*
        matrix M(2, 3, 4, 5);

        M.info();

        int sz = M.size_z();
        int sy = M.size_y();
        int sx = M.size_x();

       // M.info();

        Profiler p;

        p.Reset().Start();
        for(int k=0; k<sz; k++)
            for(int j=0; j<sy; j++)
                for(int i=0; i<sx; i++)
                M(0, k,j,i) = (1+i)+10*(j+1)+100*(k+1);



        p.Stop().Print("Loop time :");  

        M.print();
        */
    }



    void Tick()
    {
    
    }



    ~KernelPlayground()
    {

    }
};

INSTALL_CLASS(KernelPlayground)

