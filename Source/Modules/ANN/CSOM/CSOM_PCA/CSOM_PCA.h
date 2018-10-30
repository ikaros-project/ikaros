//
//	CSOM_PCA.h    This file is a part of the IKAROS project
//				A straight forward but inefficient convolution SOM implementation
//
//    Copyright (C) 2007-2010 Christian Balkenius
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

//
//  PCA-SOM
//

#include "IKAROS.h"
#include "../CSOM/CSOM.h"

#ifndef _CSOM_PCA
#define _CSOM_PCA

using namespace ikaros;

class CSOM_PCA: public CSOM 
{
public:

    static Module * Create(Parameter * p) {return new CSOM_PCA(p);}

    CSOM_PCA(Parameter * p);
    virtual ~CSOM_PCA();
    virtual void Init();
    
    void 		UpdateWeights();    
    void 		UpdateWeights_mmlt();    
    void        UpdateWeights_mmlt_blnc();
    void        UpdateWeights_mmlt_blnc_b();
    void 		UpdateWeights_4d();
    void 		MapFrom4d(float **target, float ****source, 
    	int sx, int sy, int kx, int ky);
    void			MapTo4d(float ****target, float **source,
    	int sx, int sy, int kx, int ky);

    float **	mapped_act;
    float **	mapped_weights;
    float **inh_prev;
    float ***inh_buffer;
	 float **tmp_outer;
	 float **delta_buf;
	 float **delta_buf_t;
	 float *tmp_act;
	 float **mapped_dw;


	 int numkernels;
	 int kernelsize;
	 int inp_rows;
     int update_algo;

};

#endif
