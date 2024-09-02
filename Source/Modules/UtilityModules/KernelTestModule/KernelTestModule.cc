#include "ikaros.h"

using namespace ikaros;

class KernelTestModule: public Module
{
    parameter a;
    parameter b;
    parameter c;
    parameter d;
    parameter e;

    parameter f1; // number
    parameter f2; // string
    parameter f3; // bool

    parameter g;

    parameter data;
    parameter mdata;

    parameter x;
    parameter y;

    matrix input;
    matrix output;

    matrix output1;
    matrix output2;
    matrix output3;
    matrix output4;
    matrix output5;
    matrix output6;
    matrix output7;
    matrix output8;
    matrix output9;

    void Init()
    {
        Bind(a, "a");
        Bind(b, "b");
        Bind(c, "c");
        Bind(d, "d");
        Bind(e, "e");
        Bind(f1, "f1");
        Bind(f2, "f2");
        Bind(f3, "f3");
        Bind(g, "g");
        Bind(data, "data");
        Bind(mdata, "mdata");

        Bind(x, "x");
        Bind(y, "y");

        a.print("a");
        b.print("b");
        c.print("c");
        d.print("d");
        e.print("e");
        f1.print("f1");
        f2.print("f2");
        f3.print("f3");
        g.print("g");
        x.print("x");
        y.print("y");    

        data.print("data");
        mdata.print("mdata");
    }


    void Tick()
    {
        output.copy(input);
    }
};

INSTALL_CLASS(KernelTestModule)

