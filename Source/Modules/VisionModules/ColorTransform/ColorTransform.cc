//
//	ColorTransform.cc		This file is a part of the IKAROS project
//						A module to convert color coordinates
//
//    Copyright (C) 2003  Christian Balkenius, Anders J Johansson
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//	Partially based on code by Mark Ruzon from C code by Yossi Rubner, 23 September 1997.
//	Updated for MATLAB 5 28 January 1998. Moved to IKAROS February 2003.


#include "ColorTransform.h"

using namespace ikaros;


// Transformation types

#define 	RGB2LAB		0
#define 	RGB2XYZ		1
#define 	LAB2RGB		2
#define 	XYZ2RGB		3
#define 	RGB2rgI		4



static inline
float
limit(float x, float a, float b)
{
    return(x < a ? a :  (x > b ? b : x));
}



//inline
static void
transform_rgb2xyz(float red, float green, float blue, float & X, float & Y, float & Z)
{
    X 	= 0.412453*red	+ 0.357581*green	+ 0.180423*blue;
    Y	= 0.212670*red	+ 0.715160*green	+ 0.072169*blue;
    Z	= 0.019334*red	+ 0.119193*green	+ 0.950227*blue;
}



//inline
static void
transform_xyz2rgb(float X, float Y, float Z, float & red, float & green, float & blue)
{
    red 		=   3.240479*X	- 1.537150*Y		- 0.498535*Z;
    green       = -0.969256*X	+ 1.875992*Y		+ 0.041556*Z;
    blue		=  0.055648*X		- 0.204043*Y		+ 1.057311*Z;
}



const float T = 0.008856;



static float
f(float t)
{
    if (t > T)
        return ikaros::pow((t), 1.0/3.0);
    else
        return 7.787*(t) + 16.0/116.0;
}



// transform_rgb2lab transforms from RGB into CIE Lab.
// Based on ITU-R Recommendation  BT.709 using the D65 white point reference.

inline
static void
transform_rgb2lab(float red, float green, float blue, float & L, float & a, float & b)
{
    float X, Y, Z;

    X 	= 0.412453*red	+ 0.357581*green	+ 0.180423*blue;
    Y	= 0.212670*red	+ 0.715160*green	+ 0.072169*blue;
    Z	= 0.019334*red	+ 0.119193*green	+ 0.950227*blue;

    X 	/= 0.950456;
    Y 	/= 1.000000;
    Z 	/= 1.088754;

    if (Y > T)
        L = 116.0*ikaros::pow(Y, 1.0/3.0)-16.0;
    else
        L = 903.3*Y;

    if (L < 0)
        L = 0;

    a = 500.0*( f(X) - f(Y) );
    b = 200.0*( f(Y) - f(Z) );
}



inline
static void
transform_lab2rgb(float L, float a, float b, float & red, float & green, float & blue, float scale = 1.0)
{
    float T1 = 0.008856;
    float T2 = 0.206893;

    // Compute Y

    float	fY = ikaros::pow(((L + 16) / 116), 3.0);
    bool YT = fY > T1;

    if (!YT)
        fY = L / 903.3;

    float Y = fY;

    if (YT)
        fY = ikaros::pow(fY, 1.0/3.0);
    else
        fY = (7.787 * fY + 16/116);

    // Compute X

    float	fX = a / 500 + fY;
    bool	XT = fX > T2;
    float X;

    if (XT)
        X = ikaros::pow(fX, 3.0);
    else
        X =(fX - 16/116) / 7.787;

    // Compute Z

    float	fZ = fY - b / 200;
    bool	ZT = fZ > T2;

    float Z;
    if (ZT)
        Z = ikaros::pow(fZ, 3.0);
    else
        Z =  (fZ - 16/116) / 7.787;

    X *= 0.950456;
    Y *= 1.000000;
    Z *= 1.088754;

    red 		=   3.240479*X		- 1.537150*Y		- 0.498535*Z;
    green	= -0.969256*X		+ 1.875992*Y		+ 0.041556*Z;
    blue		=  0.055648*X		- 0.204043*Y		+ 1.057311*Z;

    red		=	scale * limit(red, 0.0, 1.0);
    green	=	scale * limit(green, 0.0, 1.0);
    blue	=	scale * limit(blue, 0.0, 1.0);
}



void
ColorTransform::Init()
{
    transform	=	GetIntValueFromList("transform");
    scale		=	GetFloatValue("scale");
    
    Notify(msg_verbose, "transform = %d\n", transform);

    size_x	 	= GetInputSizeX("INPUT0");
    size_y	 	= GetInputSizeY("INPUT0");

    int size_x1	 	= GetInputSizeX("INPUT1");
    int size_y1	 	= GetInputSizeY("INPUT1");

    int size_x2	 	= GetInputSizeX("INPUT2");
    int size_y2	 	= GetInputSizeY("INPUT2");

    if(size_x != size_x1 || size_x1 != size_x2 || size_y != size_y1 || size_y1 != size_y2)
    {
        Notify(msg_fatal_error, "ColorTransform: the sizes of the inputs to \"%s\"are not equal.", GetName());
    }

    input0		= GetInputMatrix("INPUT0");
    input1		= GetInputMatrix("INPUT1");
    input2		= GetInputMatrix("INPUT2");

    output0		= GetOutputMatrix("OUTPUT0");
    output1		= GetOutputMatrix("OUTPUT1");
    output2		= GetOutputMatrix("OUTPUT2");
}



void
ColorTransform::Tick()
{
    switch (transform)
    {
        case RGB2LAB:
            for (int j=0; j<size_y; j++)
                for (int i=0; i<size_x; i++)
                    transform_rgb2lab(input0[j][i]/scale, input1[j][i]/scale, input2[j][i]/scale, output0[j][i], output1[j][i], output2[j][i]);
        break;

        case RGB2XYZ:
            for (int j=0; j<size_y; j++)
                for (int i=0; i<size_x; i++)
                    transform_rgb2xyz(input0[j][i]/scale, input1[j][i]/scale, input2[j][i]/scale, output0[j][i], output1[j][i], output2[j][i]);
        break;

        case XYZ2RGB:
            for (int j=0; j<size_y; j++)
                for (int i=0; i<size_x; i++)
                    transform_xyz2rgb(input0[j][i]/scale, input1[j][i]/scale, input2[j][i]/scale, output0[j][i], output1[j][i], output2[j][i]);
        break;

        case LAB2RGB:
            for (int j=0; j<size_y; j++)
                for (int i=0; i<size_x; i++)
                    transform_lab2rgb(input0[j][i]/scale, input1[j][i]/scale, input2[j][i]/scale, output0[j][i], output1[j][i], output2[j][i]);
        break;
        
        case RGB2rgI:
            for (int j=0; j<size_y; j++)
                for (int i=0; i<size_x; i++)
                    if(input0[j][i] == 0 && input1[j][i] == 0 && input2[j][i] == 0)
                    {
                        input1[j][i] = 0.000001;    // map black points on (dark) gray
                        input1[j][i] = 0.000001;
                        input1[j][i] = 0.000001;
                    }
                    
            add(add(output2, input0, input1, size_x, size_y), input2, size_x, size_y);
            divide(output0, input0, output2, size_x, size_y);
            divide(output1, input1, output2, size_x, size_y);
        break;
    }
}



