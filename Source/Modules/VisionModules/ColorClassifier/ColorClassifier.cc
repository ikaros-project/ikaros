//
//	ColorClassifier.cc	This file is a part of the IKAROS project
//
//
//    Copyright (C) 2005-2008 Christian Balkenius
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



#include "ColorClassifier.h"

using namespace ikaros;

#define HMAX	256

void
ColorClassifier::Init()
{
	compensation = GetBoolValue("compensation", false);
    diagnostics = GetBoolValue("diagnostics", false);
    
	color = 2*pi*GetFloatValue("color", 225.0)/360.0;
	width = cos(2*pi*GetFloatValue("width", 20)/360.0);
    
	saturation_min = GetFloatValue("saturation_min", 0.05);
	saturation_max = GetFloatValue("saturation_max", 0.50);
    
	cr = sin(color);
	cg = cos(color);

	r = GetInputMatrix("R");
	g = GetInputMatrix("G");
	I = GetInputMatrix("I");

	size_x = GetInputSizeX("R");
	size_y = GetInputSizeY("R");

	output = GetOutputMatrix("OUTPUT");

    r_prim = create_matrix(size_x, size_y);
    g_prim = create_matrix(size_x, size_y);

    Dm = create_matrix(size_x, size_y);

	colorspace_store_r = create_matrix(HMAX, HMAX);
	colorspace_store_g = create_matrix(HMAX, HMAX);
	colorspace_store_b = create_matrix(HMAX, HMAX);

	colorspace_r = GetOutputMatrix("COLORSPACE_R", false);
	colorspace_g = GetOutputMatrix("COLORSPACE_G", false);
	colorspace_b = GetOutputMatrix("COLORSPACE_B", false);

	InitColorSpace();
}



ColorClassifier::~ColorClassifier()
{
    destroy_matrix(r_prim);
    destroy_matrix(g_prim);
    destroy_matrix(Dm);
}



//  float L2 = s(*Xp++)+s(*Yp++);
//  if(saturation_min2 < L2 && L2 < saturation_max2 && *Dp > 0 && (s(*Dp) > width2 * L2) )

void
ColorClassifier::InitColorSpace()
{
	float white_red = 1.0/3.0;
	float white_green = 1.0/3.0;

	for(float _r=0; _r < 255; _r+=2)
		for(float _g=0; _g < 255; _g+=2)
			for(float _b=0; _b < 255; _b+=2)
			{
				float X = _r/(_r+_g+_b) - white_red;
				float Y = _g/(_r+_g+_b) - white_green;
				float L = sqrt(X*X+Y*Y);

				int hx = int(HMAX*(1.0/3.0+X));
				int hy = int(HMAX*(1.0/3.0+Y));

				X /= L;
				Y /= L;

				if(hx < HMAX-1 && hy < HMAX-1 && hx >= 0 && hy >= 0)
				{
					if(saturation_min < L && L < saturation_max && cr * X + cg * Y > width)
					{
						colorspace_store_r[hx][hy] = _r/255.0;
						colorspace_store_g[hx][hy] = _g/255.0;
						colorspace_store_b[hx][hy] = _b/255.0;

						colorspace_store_r[hx+1][hy] = _r/255.0;
						colorspace_store_g[hx+1][hy] = _g/255.0;
						colorspace_store_b[hx+1][hy] = _b/255.0;

						colorspace_store_r[hx][hy+1] = _r/255.0;
						colorspace_store_g[hx][hy+1] = _g/255.0;
						colorspace_store_b[hx][hy+1] = _b/255.0;

						colorspace_store_r[hx+1][hy+1] = _r/255.0;
						colorspace_store_g[hx+1][hy+1] = _g/255.0;
						colorspace_store_b[hx+1][hy+1] = _b/255.0;
					}
					else
					{
						colorspace_store_r[hx][hy] = _r/512.0;
						colorspace_store_g[hx][hy] = _g/512.0;
						colorspace_store_b[hx][hy] = _b/512.0;

						colorspace_store_r[hx+1][hy] = _r/512.0;
						colorspace_store_g[hx+1][hy] = _g/512.0;
						colorspace_store_b[hx+1][hy] = _b/512.0;

						colorspace_store_r[hx][hy+1] = _r/512.0;
						colorspace_store_g[hx][hy+1] = _g/512.0;
						colorspace_store_b[hx][hy+1] = _b/512.0;

						colorspace_store_r[hx+1][hy+1] = _r/512.0;
						colorspace_store_g[hx+1][hy+1] = _g/512.0;
						colorspace_store_b[hx+1][hy+1] = _b/512.0;
					}
				}
			}

	for(int i=0; i<HMAX; i++) // Draw cross
	{
		colorspace_store_r[i][HMAX/3] = 0.5;
		colorspace_store_r[HMAX/3][i] = 0.5;

		colorspace_store_g[i][HMAX/3] = 0.5;
		colorspace_store_g[HMAX/3][i] = 0.5;

		colorspace_store_b[i][HMAX/3] = 0.5;
		colorspace_store_b[HMAX/3][i] = 0.5;
	}
}



inline float s(float x) {return x*x;}

void
ColorClassifier::Tick_Slow()
{
	copy_matrix(colorspace_r, colorspace_store_r, HMAX, HMAX);
	copy_matrix(colorspace_g, colorspace_store_g, HMAX, HMAX);
	copy_matrix(colorspace_b, colorspace_store_b, HMAX, HMAX);

	reset_matrix(output, size_x, size_y);

	// Pass 0: Calculate White Point

	float white_red = 1.0/3.0;
	float white_green = 1.0/3.0;
	float w_red = 1.0/3.0;
	float w_green = 1.0/3.0;

	if(compensation)
	{
        white_red = mean(r, size_x, size_y);
        white_green = mean(g, size_x, size_y);
	}

	// Pass 1: Find Target Color

    subtract(r_prim, r, white_red, size_x, size_y);
    subtract(g_prim, g, white_green, size_x, size_y);
    add(Dm, cr, r_prim, cg, g_prim, size_x, size_y);
    
    float saturation_min2 = sqr(saturation_min);
    float saturation_max2 = sqr(saturation_max);
    float width2 = sqr(width);

    float * Xp = r_prim[0];
    float * Yp = g_prim[0];
    float * Dp = Dm[0];

	for(int j=0; j<size_y; j++)
		for(int i=0; i<size_x; i++)
		{
			// Draw points in colorspace

			int hx = int(HMAX*(w_red+r_prim[j][i]));
			int hy = int(HMAX*(w_green+g_prim[j][i]));
			if(hx < HMAX && hy < HMAX && hx >= 0 && hy >= 0)
			{
 				colorspace_r[hx][hy] = 0.25;
				colorspace_g[hx][hy] = 0.25;
				colorspace_b[hx][hy] = 0.25;
			}

            float L2 = s(*Xp++)+s(*Yp++);
			if(saturation_min2 < L2 && L2 < saturation_max2 && *Dp > 0 && (s(*Dp) > width2 * L2) )
            {
                if(hx < HMAX && hy < HMAX && hx >= 0 && hy >= 0)
                {
                    colorspace_r[hx][hy] = 0;
                    colorspace_g[hx][hy] = 0;
                    colorspace_b[hx][hy] = 0;
                }

				output[j][i] = 1.0;
            }
            Dp++;
        }
}



void
ColorClassifier::Tick_Fast()
{
	reset_matrix(output, size_x, size_y);

	// Pass 0: Calculate White Point

	float white_red = 1.0f/3.0f;
	float white_green = 1.0f/3.0f;

	if(compensation)
	{
        white_red = mean(r, size_x, size_y);
        white_green = mean(g, size_x, size_y);
	}

	// Pass 1: Find Target Color

    subtract(r_prim, r, white_red, size_x, size_y);
    subtract(g_prim, g, white_green, size_x, size_y);
    add(Dm, cr, r_prim, cg, g_prim, size_x, size_y);
    
    float saturation_min2 = sqr(saturation_min);
    float saturation_max2 = sqr(saturation_max);
    float width2 = sqr(width);

    float * Xp = r_prim[0];
    float * Yp = g_prim[0];
    float * Dp = Dm[0];

	for(int j=0; j<size_y; j++)
		for(int i=0; i<size_x; i++)
		{
            float L2 = s(*Xp++)+s(*Yp++);
			if(saturation_min2 < L2 && L2 < saturation_max2 && *Dp > 0 && (s(*Dp) > width2 * L2) )
				output[j][i] = 1.0;
            Dp++;
		}
}



void
ColorClassifier::Tick()
{
    if(!diagnostics)
        Tick_Fast();
    else
        Tick_Slow();
}

