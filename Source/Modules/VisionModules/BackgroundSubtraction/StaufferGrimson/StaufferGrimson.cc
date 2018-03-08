//
//    StaufferGrimson.cc    This file is a part of the IKAROS project
//                          Implementation of the Stauffer-Grimson forground/background segmentation for grayscale images
//
//    Copyright (C) 2009 Christian Balkenius
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
//    See http://www.ikaros-project.org/ for more information.
//
//	Created: 2009-05-19
//
//  Based on code from unkown source



#include "StaufferGrimson.h"

using namespace ikaros;



int **
create_matrix_int(int sizex, int sizey)
{
    int * a = (int *)calloc(sizex*sizey, sizeof(int));
    int ** b = (int **)malloc(sizey*sizeof(int *));
    for (int j=0; j<sizey; j++)
        b[j] = &a[j*sizex];
    return b;
}




static int ***
create_matrix_int(int x, int y, int z)
{
    int ** a = create_matrix_int(x, y*z);
    int *** b = (int ***)malloc(z*sizeof(int *));
    for (int j=0; j<z; j++)
        b[j] = &a[j*y];
    return b;
}





static double NormPDF(float * norm_data, double x, double mean, double stdDev)
{
    int y = (int)(abs(float((x-mean)/stdDev)) * 250 + 0.5 );
    if (y>999)
        return 0;
    else
        return norm_data[y] / stdDev;
}



void
StaufferGrimson::Init()
{
    Bind(alpha, "alpha");
    Bind(backGroundPortion, "T");
    gaussPerPixel = GetIntValue("gaussPerPixel");
    Bind(initialStandardDeviation, "initialStandardDeviation");
    Bind(deviationMultiplier, "threshold");

    sizex = GetInputSizeX("INPUT");
    sizey = GetInputSizeY("INPUT");
    input = GetInputMatrix("INPUT");
    output = GetOutputMatrix("OUTPUT");
    
    weightOrder = create_matrix_int(sizex, sizey, gaussPerPixel);
    for(int i=0; i<sizex; i++)
        for(int j=0; j<sizey; j++)
            for(int k=0; k<gaussPerPixel; k++)
                weightOrder[k][j][i] = k;
    
    weight = create_matrix(sizex, sizey, gaussPerPixel);
    average = create_matrix(sizex, sizey, gaussPerPixel);
    stdDev = create_matrix(sizex, sizey, gaussPerPixel);
    
    set_array(**stdDev, initialStandardDeviation, sizex*sizey*gaussPerPixel);
//    set_array(**average, 0, sizex*sizey*gaussPerPixel);
    
    norm_data = create_array(1000);
    for(int i=0; i<1000; i++)
        norm_data[i] = gaussian(float(i)/250.0, 1);
}




/* StaufferData contains gaussians describing the background in
 * the scenery. This data is updated with inImage. All pixels in inImage
 * that are background will be set to 1. Foreground pixels will be
 * set to 128 * backgroundintensity / foregroundintensity */

void
StaufferGrimson::Tick()
{
    reset_matrix(output, sizex, sizey);
    long fgcnt = 0, bgcnt = 0;
    float rho;

    int spacer = 1;
    for (int y=0; y<sizey; y+=spacer)
    {
        for (int x=0; x<sizex; x+=spacer)
        {
            int index, weightIndex;
            char foundHit;
            // Search gaussians with largest weight first to avoid having several
            //gaussians competing for the same pixel values and hopefully speed up search.
            foundHit = 0;
            for (weightIndex=0; weightIndex<gaussPerPixel; weightIndex++)
            {
                if (abs(input[y][x] - average[weightOrder[weightIndex][y][x]][y][x]) <=
                        deviationMultiplier * stdDev[weightOrder[weightIndex][y][x]][y][x])
                {
                    foundHit = 1;
                    break;
                }
            }

            // If the pixel value doesn't fit in any of the Gaussians: get rid of least
            // probable Gaussian and create new one with the pixel value as average.
            if (!foundHit)
            {
                weightIndex = gaussPerPixel-1;
                index = weightOrder[weightIndex][y][x];
                weight[index][y][x] = 0.001;
                average[index][y][x] = input[y][x];
                stdDev[index][y][x] = initialStandardDeviation;
            }
            else
            {
                index = weightOrder[weightIndex][y][x];
            }

           // Update weights
            float weightSum = 0;
            for (int a=0; a<gaussPerPixel; a++)
            {
                weight[a][y][x] = (1-alpha)*weight[a][y][x];
                if (a==index)
                {
                    weight[a][y][x] += alpha;
                }
                weightSum += weight[a][y][x];
            }
            
            // Always make sure the sum of the weights is 1.0
            for (int a=0; a<gaussPerPixel; a++)
            {
                weight[a][y][x] = weight[a][y][x]/weightSum;
            }
            
            // Check if the Gaussian that was hit should advance in the weightOrder
            while (weightIndex>0)
            {
                weightIndex--;
                if (weight[index][y][x] > weight[ weightOrder[weightIndex][y][x] ][y][x])
                {
                    weightOrder[weightIndex+1][y][x] = weightOrder[weightIndex][y][x];
                    weightOrder[weightIndex][y][x] = index;
                } else break;
            }

            // Update averages and standard deviation for the Gaussian that was hit (or created)
            rho = alpha * NormPDF(norm_data, input[y][x], average[index][y][x], max(stdDev[index][y][x], 0.5)); // Eq. 8
            average[index][y][x] = (1-rho) * average[index][y][x] + rho * input[y][x]; // Eq. 6
            stdDev[index][y][x] = sqrt((1-rho)*sqr(stdDev[index][y][x]) + rho * sqr(input[y][x] - average[index][y][x])); // Eq. 7
            
            // Decide whether the pixel belongs to background or foreground. 
            // (if one of the Gaussians has almost all of the weight it will be considered
            // background no matter what, but normally the Gaussian(s) in the end of the
            // weightOrder will be foreground)
            
            weightIndex = 0;
            weightSum = 0;
            while (weightOrder[weightIndex][y][x]!=index)
            {
                weightSum += weight[weightOrder[weightIndex][y][x]][y][x];
                weightIndex++;
            }
            if (weightSum > backGroundPortion)
            {
                /* Pixel is foreground */
                int ww = spacer-1;
                for(int i=-ww; i<=ww; i++)
                    for(int j=-ww; j<=ww; j++)
                        if(0 <= x+i && x+i < sizex && 0 <= y+j && y+j < sizey)
                            output[y+j][x+i] = 1;
                fgcnt++;
            }
            else
            {
                /* Pixel is background (set to 1 instead of 0 to avoid division by 0 later) */
                output[y][x] = 0;
                bgcnt++;
            }
        }
    }
    
// DRAW ONE GASSUAN MODEL FOR ONE PIXEL FOR DEBUGGING

}



static InitClass init("StaufferGrimson", &StaufferGrimson::Create, "Source/Modules/VisionModules/BackgroundSubtraction/StaufferGrimson/");


