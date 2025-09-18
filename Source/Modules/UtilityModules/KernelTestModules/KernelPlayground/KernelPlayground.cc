
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

        matrix image(2000,2000);
        image.test_fill();
        
        matrix kernel = {
            {1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f},
            {4.0f, 5.0f, 6.0f, 1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f},
            {7.0f, 8.0f, 9.0f, 1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f},
            {1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f},
            {4.0f, 5.0f, 6.0f, 1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f},
            {7.0f, 8.0f, 9.0f, 1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f},
            {1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f},
            {4.0f, 5.0f, 6.0f, 1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f},
            {7.0f, 8.0f, 9.0f, 1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f}
        };

        matrix result(image.rows()-kernel.rows()+1, image.cols()-kernel.cols()+1);

        //Profiler p;

        for(int i=0; i< 1; i++)
        {
       // p.Reset().Start();
        result.corr(image, kernel);
        //p.Stop().Print("corr: ");
        }
        //result.print("result");

        int rr = image.rows() - kernel.rows() + 1;
        int rc = image.cols() - kernel.cols() + 1;
    
        int submatrix_size = kernel.rows() * kernel.cols();
        std::vector<float> submatrices_flat;
        submatrices_flat.reserve(rr * rc * submatrix_size); // Reserve memory to avoid reallocation
        std::vector<float> kernel_flat = flattenKernel(kernel);
        
        for(int i=0; i< 10; i++)
        {
            //p.Reset().Start();
            im2row(submatrices_flat, image, kernel);
            result.corr3(image, kernel, kernel_flat, submatrices_flat);
           // p.Stop().Print("corr3:");
     }


     matrix result2(image.rows(), image.cols());

     for(int i=0; i< 10; i++)
     {
         //p.Reset().Start();
         result2.conv(image, kernel);
         result2.fillReflect101Border(4,4);
         //p.Stop().Print("corrF:");
    }


    for(int i=0; i< 10; i++)
    {
        //p.Reset().Start();
        result2.conv(image, kernel);
        //result2.fillReflect101Border(4,4);
       // p.Stop().Print("corrF:");
   }



    //result2.print("result2");

    result2.fillReflect101Border(4,4);
    //result2.fillExtendBorder(4,4);
   // result2.print("result2");

/*
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

