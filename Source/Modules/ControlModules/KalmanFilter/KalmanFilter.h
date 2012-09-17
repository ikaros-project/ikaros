//
//    KalmanFilter.h		This file is a part of the IKAROS project
// 							The module implements a standard Kalman filter
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
//	Created: 2009-05-27
//
//	Inspired by the very clear implementation of the Kalman filter
//  by Naba Kumar at http://libnxt.sourceforge.net and the
//  explanation on Wikipedia
//

#ifndef _KalmanFilter_
#define _KalmanFilter_

#include "IKAROS.h"

class KalmanFilter: public Module
    {
    public:
        KalmanFilter(Parameter * p) : Module(p) {}
        virtual ~KalmanFilter();
        
        static Module * Create(Parameter * p) { return new KalmanFilter(p); }
        
        void 		Init();
        void 		Tick();
        
        float       process_noise;
        float       observation_noise;
        
        int         u_size;
        int         x_size;
        int         z_size;
        
        float *     u;      // input
        float *     x;      // state
        float *     z;      // observation (measurement)
        float *     y;      // innovation
        
        float *     x_last;
        float *     x_in;
    
        float **     A;     // state gain
        float **     B;     // input gain   
        float **     H;     // measurement gain

        float **     A_T;   // transposed matrices
        float **     B_T;
        float **     H_T;
        
        float **     P;     // measurement gain
        float **     Q;     // state transition uncertainity covariance
        float **     R;     // obervation uncertainity covariance

        float **     K;     // Kalman gain
        
        float **     I;      // identity matrix
};

#endif
