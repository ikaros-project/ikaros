//
//	IKAROS_Math.cc		Various math functions for IKAROS
//
//    Copyright (C) 2006-2022  Christian Balkenius
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

#include "IKAROS_System.h"
#include "IKAROS_Math.h"
#include "IKAROS_Utils.h"

#include <stdlib.h>
#include <string.h>  // memset
#include <algorithm> // find
#include <stdexcept> // std::invalid_argument
#include <numeric> // std::iota
#include <random> // std::shuffle
// includes for image processing (JPEG)

#include <stdio.h>
/*
extern "C"
{
#include "jpeglib.h"
#include <setjmp.h>
}
*/



#ifdef MAC_OS_X
#include <Accelerate/Accelerate.h>
#endif

#ifdef LINUX
//#define	MAXFLOAT	3.402823466e+38f
#ifdef USE_BLAS
extern "C"
{
#include "gsl/gsl_blas.h"
}
#endif
#endif

namespace ikaros
{
	extern const float maxfloat = MAXFLOAT;
	extern const float pi = M_PI;
	extern const float sqrt2pi = sqrt(2*pi);
    
    float eps(float x)
    {
        return nextafterf(fabsf(x), MAXFLOAT)-fabsf(x);
    }
        
	// misc scalar functions
    
    // MARK: -
    // MARK: functions
/*
	float trunc(float x)
	{
		return ::truncf((int)x);
	}
	float exp(float x)
	{
		return ::expf(x);
	}
	float pow(float x, float y)
	{
		return ::powf(x, y);
	}
	float log(float x)
	{
		return ::logf(x);
	}
	float log10(float x)
	{
		return ::log10f(x);
	}
	float	sin(float x)
	{
		return ::sinf(x);
	}
	float	cos(float x)
	{
		return ::cosf(x);
	}
	float	tan(float x)
	{
		return ::tanf(x);
	}
	float	asin(float x)
	{
		return ::asinf(x);
	}
	float	acos(float x)
	{
		return ::acosf(x);
	}
	float	atan(float x)
	{
		return ::atanf(x);
	}
	float	atan2(float y, float x)
	{
		return ::atan2f(y, x);
	}
*/
	float *
	atan2(float * r, float * a, float * b, int size)
	{
#ifdef USE_VFORCE
		vvatan2f(r, a, b, &size);
#else
		for (int i=0; i<size; i++)
			r[i] = ::atan2f(a[i], b[i]);
#endif
		return r;
	}
	
	float **
	atan2(float ** r, float ** a, float ** b, int sizex, int sizey)
	{
		atan2(*r, *a, *b, sizex*sizey);
		return r;
	}
    
    float mod(float x, float y)  // follows matlab function mod
    {
        if(y == 0)
            return x;
        else if(x == y)
            return 0;
        else
            return x - y*floorf(x/y);
    }
    
    
	
    // MARK: -
    // MARK: gaussian
    
	// gaussian [not optimized]
	float
	gaussian(float x, float sigma)
	{
		return (1/(sigma*sqrt2pi))*::expf(-sqr(x)/(2*sqr(sigma)));
	}
	
	float *
	gaussian(float * r, float * a, float sigma, int size)
	{
		for (int i=0; i<size; i++)
			r[i] = gaussian(a[i], sigma);
		return r;
	}
	
	float **
	gaussian(float ** r, float ** m, float sigma, int sizex, int sizey)
	{
		gaussian(*r, *m, sigma, sizex*sizey);
		return r;
	}
	
	float gaussian1(float x, float sigma)
	{
		return ::expf(-sqr(x)/(2*sqr(sigma)));
	}
	
	float *
	gaussian1(float * r, float * a, float sigma, int size)
	{
		for (int i=0; i<size; i++)
			r[i] = gaussian1(a[i], sigma);
		return r;
	}
	
	float **
	gaussian1(float ** r, float ** m, float sigma, int sizex, int sizey)
	{
		gaussian1(*r, *m, sigma, sizex*sizey);
		return r;
	}
	
	float *
	gaussian(float * r, float x, float sigma, int size)
	{
		float center = x;
		for (int i=0; i<size; i++)
			r[i] = gaussian(center-float(i), sigma);
		return r;
	}
	
	float **
	gaussian(float ** r, float x, float y, float sigma, int sizex, int sizey)
	{
		float center_x = x;
		float center_y = y;
		for (int j=0; j<sizey; j++)
			for (int i=0; i<sizex; i++)
				r[j][i] = gaussian(hypotf(center_x-float(i), center_y-float(j)), sigma);
		return r;
	}
	
	float *
	gaussian1(float * r, float x, float sigma, int size)
	{
		float center = x;
		for (int i=0; i<size; i++)
			r[i] = gaussian1(center-float(i), sigma);
		return r;
	}
	
	float **
	gaussian1(float ** r, float x, float y, float sigma, int sizex, int sizey)
	{
		float center_x = x;
		float center_y = y;
		for (int j=0; j<sizey; j++)
			for (int i=0; i<sizex; i++)
				r[j][i] = gaussian1(hypotf(center_x-float(i), center_y-float(j)), sigma);
		return r;
	}
	
	
    // MARK: -
    // MARK: random
    
	// random
	float
	random(float low, float high)
	{
		return low + (float(::random())/float(RAND_MAX))*(high-low);
	}
	
	
	float *
	random(float * r, float low, float high, int size)
	{
        for (int i=0; i<size; i++)
			r[i] = random(low, high);
		return r;
	}
	
	
	
	float **
	random(float ** r, float low, float high, int sizex, int sizey)
	{
		random(*r, low, high, sizex*sizey);
		return r;
	}
	
	int
    random(int high)
    {
		return int(::random() % high);
    }

        int
    random_int(int low, int high)
    {
        return low + random(high-low);
    }

    int *
    random_int(int *r, int low, int high, int size)
    {
        for (int i=0; i<size; i++)
            r[i] = random(low, high);
        return r;
    }

    int *
    random_unique(int *r, int low, int high, int size)
    {
        if (high-low < size)
        {
            printf("IKAROS_Math:random_unique: ERROR value range is less than size.\n");
            return NULL;
        }
        for (int i=0; i<size; i++)
        {
            int tmp;
            do {tmp = random(low, high+1);}
            while(std::find(r, r+size, tmp) != r+size);
            r[i] = tmp;
        }
        return r;

    }
    
    
    
	// gaussian_noise() uses the Box-Muller transformation to generate random numbers with gaussian distribution
	
	float
	gaussian_noise(float mean, float sigma)
	{
		float x1, x2, w, y1;
        //float y2;
		do {
			x1 = 2.0 * random(0, 1) - 1.0;
			x2 = 2.0 * random(0, 1) - 1.0;
			w = x1 * x1 + x2 * x2;
		} while (w >= 1.0);
		
		w = sqrt((-2.0f * log(w))/w);
		y1 = x1 * w;
		//y2 = x2 * w;
		
		return mean+sigma*y1;
	}

	// MARK: -
    // MARK: distro samplers
    float binomial_trial(float prob)
    {
        float p = random(0., 1.0);
        if(p < prob) {return 1.;}
        return 0;
    }
    
    float binomial_sample(int n, float prob)
    {
        
        // Basic error checking.
        if (n <= 0) {
            return -1;
        } else if (prob >= 1) {
            return -2;
        } else if (prob <= 0) {
            return -3;
        }

        float successes = 0;
        for (int i = 0; i < n; i++) 
        {
            if (binomial_trial(prob) > 0.f) 
                successes += 1.f;
        }
        return successes;
    }

    int multinomial_trial(float *prob, int size)
    {
            // Basic error checking:
        if (prob == 0 || size == 0) {
            return -1;
        }

        float* tmp = create_array(size);
        
        int maxits = 100;
        // shuffle probs to avoid place bias
        std::vector<int> shuf(size) ; // 
        std::iota (std::begin(shuf), std::end(shuf), 0); // Fill with 0, 1, ..., 99.
        auto rng = std::default_random_engine {};
        std::shuffle(std::begin(shuf), std::end(shuf), rng);
        
        for(int j = 0; j<maxits; j++){
            
            int remainingTrials = 1;
            for (int i = 0; i < size; i++) {
                int ix = shuf[i];
                
                tmp[i] = binomial_sample(remainingTrials, prob[ix] );
                if(tmp[i] > 0) return ix; 
                // else if (tmp[i]<0){printf("==ix: %i, prob: %f==\n", ix, prob[ix]);}
        
                //remainingSum -= prob[i];
                //remainingTrials -= tmp[i];
        
                // Due to floating-point, even if the probabilities appear to sum to 1.0 the remainingSum
                // may go ever so slightly negative on the last iteration. In this case, remainingTrials will
                // also be 0. (But remainingTrials may go to 0 before the last possible iteration)
                //if (remainingSum <= 0 || remainingTrials == 0 || tmp[i] > 0) {
                //break;
                //}
            }
        } // safety stop
        printf("multinomial_trial: shouldnt happen\n");
        return size-1;
    
    }

	
	// MARK: -
    // MARK: sgn
    
    int
    sgn(int x)
    {
        if(x > 0)
            return 1;
        else if(x < 0)
            return -1;
        else
            return 0;
    }
    
    inline float
    sgn(float x)
    {
        if(x > 0)
            return 1;
        else if(x < 0)
            return -1;
        else
            return 0;
    }
    
	float *
    sgn(float * a, int size)
    {
        sgn(a, a, size);
        return a;
    }
    
	float **
    sgn(float ** m, int sizex, int sizey)
    {
        sgn(m, m, sizex, sizey);
        return m;
    }

    float *
    sgn(float * r, float * a, int size)
    {
        for(int i=0; i<size; i++)
            r[i] = sgn(a[i]);
        return r;
    }
    
    float **
    sgn(float ** r, float ** m, int sizex, int sizey)
    {
        sgn(*r, *m, sizex*sizey);
        return r;
    }

    
    
    // MARK: -
    // MARK: sqr
    
	// sqr
	int
	sqr(int x)
	{
		return x*x;
	}
	
	float
	sqr(float x)
	{
		return x*x;
	}
	
	float *
	sqr(float * a, int size)
	{
		sqr(a, a, size);
		return a;
	}
	
	float **
	sqr(float ** a, int sizex, int sizey)
	{
		sqr(*a, sizex*sizey);
		return a;
	}
	
	float *
	sqr(float * r, float * a, int size)
	{
#ifdef USE_VDSP
		vDSP_vsq(a, 1, r, 1, size);
#else
		for (int i=0; i<size; i++)
			r[i] = a[i] * a[i];
#endif
		return r;
	}
	
	float **
	sqr(float ** r, float ** a, int sizex, int sizey)
	{
		sqr(*r, *a, sizex*sizey);
		return r;
	}
	
    // MARK: -
    // MARK: sqrt
    
	// sqrt
	float
	sqrt(int x)
	{
		return ::sqrtf(float(x));
	}
	
	float
	sqrt(float x)
	{
		return ::sqrtf(x);
	}
	
	float *
	sqrt(float * a, int size)
	{
		for (int i=0; i<size; i++)
			a[i] = sqrt(a[i]);
		return a;
	}
	
	float **
	sqrt(float ** a, int sizex, int sizey)
	{
		sqrt(*a, sizex*sizey);
		return a;
	}
	
	float *
	sqrt(float * r, float * a, int size)
	{
		for (int i=0; i<size; i++)
			r[i] = sqrt(a[i]);
		return r;
	}
	
	float **
	sqrt(float ** r, float ** a, int sizex, int sizey)
	{
		sqrt(*r, *a, sizex*sizey);
		return r;
	}
	
	// hypot
	float
	hypot(float x, float y)
	{
		return ::hypotf(x, y);
	}
	
	float *
	hypot(float * r, float * a, float * b, int size)
	{
#ifdef USE_VDSP
		vDSP_vdist(a, 1, b, 1, r, 1, size);
#else
		for (int i=0; i<size; i++)
			r[i] = hypot(a[i], b[i]);
#endif
		return r;
	}
	
	float **
	hypot(float ** r, float ** a, float ** b, int sizex, int sizey)
	{
		hypot(*r, *a, *b, sizex*sizey);
		return r;
	}
	
    // MARK: -
    // MARK: tests
    
	// tests
	bool
	zero(float * a, int size)
	{
		for (int i=0; i<size; i++)
			if (a[i] != 0)
				return false;
		return true;
	}
	
	bool
	zero(float ** a, int sizex, int sizey)
	{
		return zero(*a, sizex*sizey);
	}
	
	bool
	non_zero(float * a, int size)
	{
		for (int i=0; i<size; i++)
			if (a[i] != 0)
				return true;
		return false;
	}
	
	bool
	non_zero(float ** a, int sizex, int sizey)
	{
		return non_zero(*a, sizex*sizey);
	}
	
    // MARK: -
    // MARK: distances and norms
    
	// distances & norms

	float
	dist(float * a, float * b, int size)
	{
		float s = 0;
		for (int i=0; i < size; i++)
			s += (a[i] - b[i]) * (a[i] - b[i]);
		return sqrt(s);
	}
	
	float
	dist(float ** a, float ** b, int sizex, int sizey)
	{
		return dist(*a, *b, sizex*sizey);
	}
	
	float
	dist1(float * a, float * b, int size)
	{
		float r = 0;
		for (int i=0; i < size; i++)
			r += fabsf(a[i] - b[i]);
		return r;
	}
	
	float
	dist1(float ** a, float ** b, int sizex, int sizey)
	{
		return dist1(*a, *b, sizex*sizey);
	}
	
	float
	norm(float * a, int size)
	{
#ifdef USE_BLAS
		return cblas_snrm2(size, a, 1);
#else
		float r = 0;
		for (int i=0; i < size; i++)
			r += a[i] * a[i];
		return sqrt(r);
#endif
	}
	
	float
	norm(float ** a, int sizex, int sizey)
	{
		return norm(*a, sizex*sizey);
	}
	
	float
	norm1(float * a, int size)
	{
#ifdef USE_BLAS
		return cblas_sasum(size, a, 1);
#else
		float r = 0;
		for (int i=0; i < size; i++)
			r += fabs(a[i]);
		return r;
#endif
	}
	
	float
	norm1(float ** a, int sizex, int sizey)
	{
		return norm1(*a, sizex*sizey);
	}
	
	float *
	normalize(float * a, int size)
	{
		float n = norm(a, size);
		if(n!=0)
			multiply(a, 1/n, size);
		return a;
	}
	
	float **
	normalize(float ** m, int sizex, int sizey)
	{
		normalize(*m, sizex*sizey);
		return m;
	}
	
	float *
	normalize1(float * a, int size)
	{
		float n = norm1(a, size);
		if(n!=0)
			multiply(a, 1/n, size);
		return a;
	}
	
	float **
	normalize1(float ** m, int sizex, int sizey)
	{
		normalize1(*m, sizex*sizey);
		return m;
	}
	
	float *
	normalize_max(float * a, int size)
	{
		float n = max(a, size);
		if(n!=0)
			multiply(a, 1/n, size);
		return a;
	}
	
	float **
	normalize_max(float ** m, int sizex, int sizey)
	{
		normalize_max(*m, sizex*sizey);
		return m;
	}
	
	// abs
/*
	int
	abs(int x)
	{
		return (x < 0 ? -x : x);
	}

	float
	abs(float x)
	{
		return (x < 0 ? -x : x);
	}
*/
	float *
	abs(float * a, int size)
	{
		//		vvfabf(r, a, &size); ***
		float t;
		for (int i=0; i<size; i++, a++)
			if ((t = *a) < 0)
				*a = -t;
		return a;
	}
	
	float **
	abs(float ** a, int sizex, int sizey)
	{
		abs(*a, *a, sizex*sizey);
		return a;
	}
	
	float *
	abs(float * r, float * a, int size)
	{
		//		vvfabf(r, a, &size); ***
#ifdef USE_VDSP_NOT_YET
		vDSP_vabs(a, 1, r, 1, size);	// slower than scalar code on Intel Xeon
#else
		float t;
		for (int i=0; i<size; i++)
			if ((t = (*a++)) < 0)
				(*r++) = -t;
			else
				(*r++) = t;
#endif
		return r;
	}
	
	float **
	abs(float ** r, float ** m, int sizex, int sizey)
	{
		abs(*r, m[0], sizex*sizey);
		return r;
	}
	
    // MARK: -
    // MARK: min and max
    
	// min
	int
	min(int x, int y)
	{
		return (x < y ? x : y);
	}
	
	float
	min(float x, float y)
	{
		return (x < y ? x : y);
	}
	
	float
	min(float * a, int size)
	{
#ifdef USE_VDSP
		float r;
		vDSP_minv(a, 1, &r, size);
		return r;
#else
		int mi = 0;
		for (int i=0; i<size; i++)
			if (a[i] < a[mi])
				mi = i;
		return a[mi];
#endif
	}
	
	float
	min(float ** a, int sizex, int sizey)
	{
		return min(*a, sizex*sizey);
	}
	
	float *
	min(float * r, float * a, int size)
	{
#ifdef USE_VDSP
		vDSP_vmin(a, 1, r, 1, r, 1, size);
#else
		for (int i=0; i<size; i++)
			if (a[i] < r[i])
				r[i] = a[i];
#endif
		return r;
	}
	
	float **
	min(float ** r, float ** a, int sizex, int sizey)
	{
		min(*r, *a, sizex*sizey);
		return r;
	}
	
	float *
	min(float * r, float * a, float * b, int size)
	{
#ifdef USE_VDSP
		vDSP_vmin(a, 1, b, 1, r, 1, size);
#else
		for (int i=0; i<size; i++)
			if (a[i] < b[i])
				r[i] = a[i];
			else
				r[i] = b[i];
#endif
		return r;
	}
	
	float **
	min(float ** r, float ** a, float ** b, int sizex, int sizey)
	{
		min(*r, *a, *b, sizex*sizey);
		return r;
	}
	
	int
	arg_min(float * a, int size)
	{
		if (size < 1)
			return -1;
		int mi = 0;
		for (int i=0; i<size; i++)
			if (a[i] < a[mi])
				mi = i;
		return mi;
	}
	
	void
	arg_min(int & x, int & y, float ** a, int sizex, int sizey)
	{
		float t;
		float min = a[x][y];
		for (int j=0; j<sizey; j++)
			for (int i=0; i<sizex; i++)
				if ((t = a[j][i]) < min)
				{
					x = i;
					y = j;
					min = t;
				}
	}
	
	// max
	int
	max(int x, int y)
	{
		return (x > y ? x : y);
	}
	
	float
	max(float x, float y)
	{
		return (x > y ? x : y);
	}
	
	float
	max(float * a, int size)
	{
#ifdef USE_VDSP
		float r;
		vDSP_maxv(a, 1, &r, size);
		return r;
#else
		int mi = 0;
		for (int i=0; i<size; i++)
			if (a[i] > a[mi])
				mi = i;
		return a[mi];
#endif
	}
	
	float
	max(float ** a, int sizex, int sizey)
	{
		return max(*a, sizex*sizey);
	}
	
	float *
	max(float * r, float * a, int size)
	{
#ifdef USE_VDSP
		vDSP_vmax(a, 1, r, 1, r, 1, size);
#else
		for (int i=0; i<size; i++)
			if (a[i] > r[i])
				r[i] = a[i];
#endif
		return r;
	}
	
	float **
	max(float ** r, float ** a, int sizex, int sizey)
	{
		max(*r, *a, sizex*sizey);
		return r;
	}
	
	float *
	max(float * r, float * a, float * b, int size)
	{
#ifdef USE_VDSP
		vDSP_vmax(a, 1, b, 1, r, 1, size);
#else
		for (int i=0; i<size; i++)
			if (a[i] > b[i])
				r[i] = a[i];
			else
				r[i] = b[i];
#endif
		return r;
	}
	
	float **
	max(float ** r, float ** a, float ** b, int sizex, int sizey)
	{
		max(*r, *a, *b, sizex*sizey);
		return r;
	}
	
	int
	arg_max(float * a, int size)
	{
		if (size < 1)
			return -1;
		float m = a[0];
		float t;
		int mi = 0;
		for (int i=1; i<size; i++)
			if ((t = a[i]) > m)
			{
				mi = i;
				m = t;
			}
		return mi;
	}
	
	void
	arg_max(int & x, int & y, float ** a, int sizex, int sizey)
	{
		float t;
		float max = a[y][x];
		for (int j=0; j<sizey; j++)
			for (int i=0; i<sizex; i++)
				if ((t = a[j][i]) > max)
				{
					x = i;
					y = j;
					max = t;
				}
	}
	
    int
    arg_max_col(float ** m, int select_col, int sizex, int sizey)
    {
        if (sizex <= select_col)
            return -1;
        float mx = m[0][select_col];
        float t;
        int mi = 0;
        for (int i=1; i<sizey; i++)
            if ((t = m[i][select_col]) > mx)
            {
                mi = i;
                mx = t;
            }
        return mi;
    }

    int
    arg_max_row(float ** m, int select_row, int sizex, int sizey)
    {
        return arg_max(m[select_row], sizex);
    }
    
	// minmax
	float *
	minmax(float & min, float & max, float * a, int size)
	{
		min = *a;
		max = *a;
		float t;
		for (int i=0; i<size; i++)
			if ((t = *a++) > max)
				max = t;
			else if (t < min)
				min = t;
		return a;
	}
	
	float **
	minmax(float & min, float & max, float ** a, int sizex, int sizey)
	{
		minmax(min, max, *a, sizex*sizey);
		return a;
	}
	
    // MARK: -
    // MARK: mean
    
	// mean
	float
	mean(float * a, int size)
	{
#ifdef USE_VDSP
		float r;
		vDSP_meanv(a, 1, &r, size);
		return r;
#else
		float s = 0;
		for (int i=0; i<size; i++)
			s += a[i];
		if (s == 0)
			return 0;
		else
			return s/float(size);
#endif
	}

	float
	mean(float ** a, int sizex, int sizey)
	{
		return mean(*a, sizex*sizey);
	}

    float *
    mean(float * r, float ** m, int sizex, int sizey, int dim)
    {
        if(dim==0)
        {
            for(int i=0; i< sizex; i++)
                r[i] = 0;
            
            for(int j=0; j<sizey; j++)
                for(int i=0; i< sizex; i++)
                    r[i] += m[j][i];
            
            for(int i=0; i< sizex; i++)
                r[i] /= float(sizey);
        }
        else
        {
            for(int j=0; j<sizey; j++)
                r[j] = mean(m[j], sizex);
        }
        return r;
    }



	float
    median(float * a, int size)
	{
        float s[size];
        copy_array(s, a, size);
        sort(s, size);
        if(size % 2 == 1) // odd
            return s[size/2];
        else
            return 0.5*(s[size/2-1] + s[size|2]);
    }
    
    
    
    float
    median(float ** a, int sizex, int sizey)
	{
        return median(*a, sizex*sizey);
    }
    
    
    
    float *
    median(float * r, float ** a, int sizex, int sizey, int dim)
    {
        if(dim==0)
        {
            printf("ERROR: median of coumns not implemented\n");
            return r;
        }
    
        for(int j=0; j<sizey; j++)
            r[j] = median(a[j], sizex);
        return r;
    }
    
    
    
    
    // MARK: -
    // MARK: clip
    
	// clip
	float
	clip(float x, float low, float high)
	{
		if (x < low)
			return low;
		else if (x > high)
			return high;
		else
			return x;
	}
    
	float *
	clip(float * a, float low, float high, int size)
	{
#ifdef USE_VDSP
		vDSP_vclip(a, 1, &low, &high, a, 1, size);
#else
		for (int i=0; i<size; i++)
			if (a[i] <low )
				a[i] = low;
			else if (a[i] > high)
				a[i] = high;
#endif
		return a;
	}
	
	float **
	clip(float ** a, float low, float high, int sizex, int sizey)
	{
		clip(*a, low, high, sizex*sizey);
		return a;
	}
	
	float *
	clip(float * r, float * a, float low, float high, int size)
	{
#ifdef USE_VDSP
		vDSP_vclip(a, 1, &low, &high, r, 1, size);
#else
		for (int i=0; i<size; i++)
			if (a[i] <low )
				r[i] = low;
			else if (a[i] > high)
				r[i] = high;
			else
				r[i] = a[i];
#endif
		return r;
	}
	
	float **
	clip(float ** r, float ** a, float low, float high, int sizex, int sizey)
	{
		clip(*r, *a, low, high, sizex*sizey);
		return r;
	}
	
    // MARK: -
    // MARK: add and subtract
    
	// add
	float
	sum(float * a, int size)	// sum a
	{
		float r = 0;
#ifdef USE_VDSP
		vDSP_sve(a, 1, &r, size);
#else
		for (int i=0; i<size; i++)
			r += a[i];
#endif
		return r;
	}
	
	float
	sum(float ** a, int sizex, int sizey)	// sum a
	{
		return sum(*a, sizex*sizey);
	}
    
    // sum by row, i.e. in y direction
    // r(sizex) = sum(a(sizey, sizex))
    float *
    sum(float *r, float **a, int sizex, int sizey )
    {

        float ** tmp = create_matrix(sizey, 1);
        set_array(tmp[0], 1, sizey);
        multiply(&r, tmp, a, sizex, 1, sizey);
        destroy_matrix(tmp);
        return r;        
        
    }  

    // sum by axis, i.e. either by row or column
    float *
    sum(float *r, float **a, int sizex, int sizey, int axis)
    {
        if(axis==0)
            r = sum(r, a, sizex, sizey);
        else // if(axis==1)
        {
            
            for(int i = 0; i < sizey; i++)
            {
                r[i] = sum(a[i], sizex);
            }
            
        }
        return r;
    }
	
	float *
	add(float * r, float alpha, int size)	// r = r + alpha
	{
#ifdef USE_VDSP
		vDSP_vsadd(r, 1, &alpha, r, 1, size);
#else
		for (int i=0; i<size; i++)
			r[i] += alpha;
#endif
		return r;
	}
	
	float **
	add(float ** r, float alpha, int sizex, int sizey)	// r = r + alpha
	{
		add(*r, alpha, sizex*sizey);
		return r;
	}
	
	float *
	add(float * r, float * a, int size)	// r = r + a
	{
		return add(r, r, a, size);
	}
	
	float **
	add(float ** r, float ** a, int sizex, int sizey)	// r = r + a
	{
		add(*r, *r, *a, sizex*sizey);
		return r;
	}
	
	float *
	add(float * r, float * a, float * b, int size)	// r = a + b
	{
#ifdef USE_VDSP
		vDSP_vadd(a, 1, b, 1, r, 1, size);
#else
		for (int i=0; i<size; i++)
			r[i] = a[i] + b[i];
#endif
		return r;
	}
	
	float **
	add(float ** r, float ** a, float ** b, int sizex, int sizey)	// r = a + b
	{
		add(*r, *a, *b, sizex*sizey);
		return r;
	}
	
	float *
	add(float * r, float alpha, float * a, int size)	// r = r + alpha * a
	{
#ifdef USE_VDSP
		vDSP_vsma(a, 1, &alpha, r, 1, r, 1, size);
#else
		for (int i=0; i<size; i++)
			r[i] = r[i] + alpha * a[i];
#endif
		return r;
	}
	
	float **
	add(float ** r, float alpha, float ** a, int sizex, int sizey)	// r = r + alpha * a
	{
		add(*r, alpha, *a, sizex*sizey);
		return r;
	}
	
	float *
	add(float * r, float alpha, float * a, float beta, float * b, int size)	// r = alpha * a + beta * b
	{
#ifdef USE_VDSP_NOT
		vDSP_vsmul(a, 1, & alpha, r, 1, size);
		vDSP_vsma(b, 1, &beta, r, 1, r, 1, size);
#else
		for (int i=0; i<size; i++)
			(*r++) = alpha*(*a++) + beta*(*b++);
#endif
		return r;
	}
	
	float **
	add(float ** r, float alpha, float ** a, float beta, float ** b, int sizex, int sizey)	// r = alpha * a + beta * b
	{
		add(*r, alpha, *a, beta, *b, sizex*sizey);
		return r;
	}
	
	float *
	add(float * r, float alpha, float * a, float beta, int size)	// r = alpha * a + beta
	{
#ifdef USE_VDSP
		vDSP_vsmsa(a, 1, &alpha, &beta, r, 1, size);
#else
		for (int i=0; i<size; i++)
			r[i] = alpha * a[i] + beta;
#endif
		return r;
	}
	
	float **
	add(float ** r, float alpha, float ** a, float beta, int sizex, int sizey)	// r = alpha * a + beta
	{
		add(*r, alpha, *a, beta, sizex*sizey);
		return r;
	}
	
	// subtract
	
	float *
	subtract(float * r, float alpha, int size)	// r = r - alpha
	{
		return add(r, -alpha, size);
	}
	
	float **
	subtract(float ** r, float alpha, int sizex, int sizey)	// r = r - alpha
	{
		return add(r, -alpha, sizex, sizey);
	}
	
	float *
	subtract(float * r, float * a, float alpha, int size)	// r = a - alpha  //TODO: *** undocumented ***
	{
		for (int i=0; i<size; i++)
			r[i] = a[i] - alpha;
		return r;
	}
	
	float **
	subtract(float ** r, float ** a, float alpha, int sizex, int sizey)	// r = a - alpha  //TODO: *** undocumented ***
	{
		subtract(*r, *a, alpha, sizex*sizey);
		return r;
	}
	
	float *
	subtract(float alpha, float * r, int size)	// r = alpha - r
	{
		for (int i=0; i<size; i++)
			r[i] = alpha - r[i];
		return r;
	}
	
	float **
	subtract(float alpha, float ** r, int sizex, int sizey)	// r = alpha - r
	{
		subtract(alpha, *r, sizex*sizey);
		return r;
	}
	
	float *
	subtract(float * r, float * a, int size)
	{
		return subtract(r, r, a, size);
	}
	
	float **
	subtract(float ** r, float ** a, int sizex, int sizey)
	{
		subtract(*r, *a, sizex*sizey);
		return r;
	}
	
	
	float *
	subtract(float * r, float * a, float * b, int size)
	{
		//#ifdef USE_VDSP
		//    vDSP_vsub(a, 1, b, 1, r, 1, size);	// This function does not work in 10.4, a and b changed
		//#else
		for (int i=0; i<size; i++)
			r[i] = a[i] - b[i];
		//#endif
		return r;
	}
	
	
	float **
	subtract(float ** r, float ** a, float ** b, int sizex, int sizey)
	{
		subtract(*r, *a, *b, sizex*sizey);
		return r;
	}
    
    float **
    subtract(float ** r, float ** m, float * a, int sizex, int sizey)
    {
        for(int j=0; j<sizey; j++)
            subtract(r[j], m[j], a, sizex);
    
        return r;
    }

    // MARK: -
    // MARK: multiply and divide
    
    
    float
    product(float * a, int size)
    {
        float p = 1;
        for(int i=0; i<size; i++)
            p *= a[i];
        return p;
    }
    
    float
    product(float ** m, int sizex, int sizey)
    {
        return product(*m, sizex*sizey);
    }

    
	// multiply
	float *
	multiply(float * a, float alpha, int size)
	{
#ifdef USE_BLAS
		cblas_sscal(size, alpha, a, 1);
#elif defined USE_VDSP
		vDSP_vsmul(a, 1, &alpha, a, 1, size);
#else
		for (int i=0; i<size; i++)
			a[i] *= alpha;
#endif
		return a;
	}
	
	float **
	multiply(float ** a, float alpha, int sizex, int sizey)
	{
		multiply(*a, alpha, sizex*sizey);
		return a;
	}
	
	float *
	multiply(float * r, float * a, float alpha, int size)
	{
#ifdef USE_BLAS
		reset_array(r, size);
		cblas_saxpy(size, alpha, a, 1, r, 1);
#elif defined USE_VDSP
		vDSP_vsmul(a, 1, &alpha, r, 1, size);
#else
		for (int i=0; i<size; i++)
			r[i] = alpha * a[i];
#endif
		return r;
	}
	
	float **
	multiply(float ** r, float ** a, float alpha, int sizex, int sizey)
	{
		multiply(*r, *a, alpha, sizex*sizey);
		return r;
	}
	
	float *
	multiply(float * r, float * a, int size)
	{
		return multiply(r, r, a, size);
	}
	
	float **
	multiply(float ** r, float ** a, int sizex, int sizey)
	{
		multiply(*r, *a, sizex*sizey);
		return r;
	}
	
	float *
	multiply(float * r, float * a, float * b, int size)
	{
#ifdef USE_VDSP
		vDSP_vmul(a, 1, b, 1, r, 1, size);
#else
		for (int i=0; i<size; i++)
			r[i] = a[i] * b[i];
#endif
		return r;
	}

    float *
	multiply(float * r, float * a, float * b, float alpha, int size) 
    {
        for (int i=0; i<size; i++)
			r[i] = alpha * a[i] * b[i];

		return r;
    }
	
	float **
	multiply(float ** r, float ** a, float ** b, int sizex, int sizey)
	{
		multiply(*r, *a, *b, sizex*sizey);
		return r;
	}
	
	// matrix(sizex, sizey) * vector(sizex)
    //
    // 2011-12-07 it is now possible to use the same vector for r and b, but with some memory allocation overhead, i.e. x=Ax.
    //
    
	float *
	multiply(float * r, float ** a, float * b, int sizex, int sizey)
	{
        float * bb = b;
        
        if(r==b)
            bb = copy_array(create_array(sizex), b, sizex);
            
#ifdef USE_BLAS
		cblas_sgemv(CblasRowMajor, CblasNoTrans, sizey, sizex, 1.0, *a, sizex, bb, 1, 0.0, r, 1);
#else
        float * aj = *a;
		for (int j=0; j<sizey; j++)
		{
			float s = 0;
            //			float * aj = a[j];
			float * bi = bb;
			for (int i=0; i<sizex; i++)
				s +=  (*aj++) * (*bi++);
			(*r++) = s;
		}
#endif
        if(b!=bb)
            destroy_array(bb);

		return r;
	}
	
	// multiply - matrix * matrix
    //
    // 2011-12-07 it is now possible to multiply a matrix with itself or into itself but this adds some memory allocation overhead, e.g.
    //              A=A*B, B=A*B, A=A*A
    
	float **
	multiply(float ** r, float ** a, float ** b, int sizex, int sizey, int n)
	{
        float ** aa = a;
        float ** bb = b;

        if(r==a)
            aa = copy_matrix(create_matrix(sizex, sizey), a, sizex, sizey);

        if(r==b)
            bb = copy_matrix(create_matrix(sizex, sizey), b, sizex, sizey);
        
#ifdef USE_BLAS
		cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
					sizey, sizex, n, 1.0, *aa, n, *bb, sizex, 0.0, *r, sizex);
#else
		float s;
		int n1 = n-1;
		for (int j=0; j<sizey; j++)
		{
			for (int i=0; i<sizex; i++)
			{
				s = 0;
				float * aj = aa[j];
				float * bki = &bb[0][i];
				for (int k=n1; k>=0; k--)
				{
					s += (*aj++) * (*bki);
					bki += sizex;
				}
				r[j][i] = s;
			}
		}
#endif
        if(a!=aa)
            destroy_matrix(aa);
            
        if(b!=bb)
            destroy_matrix(bb);

		return r;
	}
	
	// dot - scalar product
	float
	dot(float * a, float * b, int size)
	{
#ifdef USE_BLAS
		return cblas_sdot(size, a, 1, b, 1);
#elif defined USE_VDSP
		float r = 0;
		vDSP_dotpr(a, 1, b, 1, &r, size);
		return r;
#else
		float r = 0;
		for (int i=0; i<size; i++)
			r += a[i] * b[i];
		return r;
#endif
	}
	
	float
	dot(float ** a, float ** b, int sizex, int sizey)
	{
		return dot(*a, *b, sizex*sizey);
	}
	
	// outer - tensor product
	float **
	outer(float ** r, float * a, float * b, int sizex, int sizey)
    {
 #ifdef USE_BLAS
        cblas_sger(CblasRowMajor, sizey, sizex, 1.0, a, 1, b, 1, *r, sizex); // A := alpha*x*y' + A
        return r;
 #else
        for (int j=0; j<sizey; j++)
            for (int i=0; i<sizex; i++)
                r[j][i] = a[j] * b[i]; //was a[i]*b[j]
        return r;
 #endif
    }
 /* OLD
    {
		for (int j=0; j<sizey; j++)
			for (int i=0; i<sizex; i++)
				r[j][i] = a[i] * b[j];
		return r;
	}
*/

	// divide
	float *
	divide(float * r, float * a, int size)
	{
		return divide(r, r, a, size);
	}
	
	float **
	divide(float ** r, float ** a, int sizex, int sizey)
	{
		divide(*r, *a, sizex*sizey);
		return r;
	}
	
	float *
	divide(float * r, float * a, float * b, int size)
	{
#ifdef USE_VDSP
		vDSP_vdiv(b, 1, a, 1, r, 1, size);  // *** WARNING *** This function do c = b/a but is documented to do c = a/b
#else
		for (int i=0; i<size; i++)
			r[i] = a[i] / b[i];
#endif
		return r;
	}
	
	float **
	divide(float ** r, float ** a, float ** b, int sizex, int sizey)
	{
		divide(*r, *a, *b, sizex*sizey);
		return r;
	}
    
    
    
    // linear algebra
    // MARK: -
    // MARK: linear algebra
    
    
    float **
    transpose(float ** a_T, float ** a, int sizex, int sizey)
    {
        for(int i=0; i<sizex; i++)
            for(int j=0; j<sizey; j++)
                a_T[j][i] = a[i][j];
        return a_T;
    }
    
    
    
    float **
    transpose(float ** a, int size)
    {
        for(int i=0; i<size; i++)
            for(int j=i+1; j<size; j++)
            {
                float t = a[j][i];
                a[j][i] = a[i][j];
                a[i][j] = t;
            }
        return a;
    }
    
    
    // eye sets a to the identity matrix

    float **
    eye(float ** a, int size)
    {
        reset_matrix(a, size, size);
        for(int i=0; i<size; i++)
            a[i][i] = 1;
        return a;
    }
    
    
    
    static void
    swap_rows(float ** a, int i, int j, int sizex)
    {
        for(int k=0; k<sizex; k++)
        {
            float t = a[i][k];
            a[i][k] = a[j][k];
            a[j][k] =  t;
        }
    }
    
    
    static void
    swap_columns(float ** a, int i, int j, int sizex)
    {
        for(int k=0; k<sizex; k++)
        {
            float t = a[k][j];
            a[k][j] = a[k][i];
            a[k][i] =  t;
        }
    }
    
    
    
    static void
    convert_pivots(int * pivots, int size)
    {
        int * p = new int [size];
        for(int i=0; i<size; i++)
            p[i] = i;
        for(int i=0; i<size; i++)
        {
            int tmp = p[pivots[i]];
            p[pivots[i]] = p[i];
            p[i] = tmp;
        }
        for(int i=0; i<size; i++)
            pivots[i] = p[i];
        delete[] p;
    }


    // LU decomposition in place
    // based on sgetf2.f
    // detp: determinant of the permutation matrix
    // pivots: row exchanges list; size = min(sizex, sizey)
    //
    // NOTE: The interface for this function may change in the future and it is not yet included in the header file
    //
    
    static float **
    lu(float ** m, int sizex, int sizey, float * detp=NULL, int * pivots=NULL)
    {
        int i, j, p, pd = 1;
        
        int sizemin = (sizey<sizex ? sizey : sizex); // min ***
        
        if(sizey==0 || sizex==0)
            return m;
        
        for(j = 0; j < sizemin; j++)
        {
            p = j;
            for(i = j+1; i < sizey; i++)
                if(::abs(m[i][j]) > ::abs(m[p][j]))
                    p = i;
            
            if(pivots)
                pivots[j] = p;
            
            if(m[p][j] != 0)
            {
                if(p != j)
                {
                    swap_rows(m, j, p, sizex);
                    pd = -pd;
                }
                
                if(j < sizey)
                {
                    p = j+1;
                    double s = 1/m[j][j]; // could test for overflow here
                    for(int k=j+1; k<sizey; k++)
                        m[k][j] *= float(s);
                }
            }
            
            if(j < sizemin)
            {
                for(i = j+1; i<sizey; i++)
                {
                    double s = m[i][j];
                    for(int k=p; k<sizex; k++)
                        m[i][k] -=  float(s * m[j][k]);
                }
            }
        }
        
        if(detp)
            *detp = pd;
        
        return m;
    }



    void
    lu(float ** l, float ** u, float ** p, float ** a, int sizex, int sizey)  // ****** YOU ARE HERE *********
    {
        int sizemin = min(sizex, sizey);
        int * pivots = new int [sizey]; // FIXME: check that sizey is correct here
        float ** ludc = copy_matrix(create_matrix(sizex, sizey), a, sizex, sizey);

        lu(ludc, sizex, sizey, NULL, pivots);
        
        convert_pivots(pivots, sizey);
        reset_matrix(p, sizey, sizey);
        for(int j=0; j<sizey; j++)
            p[j][pivots[j]] = 1;
        
        reset_matrix(u, sizex, sizemin);
        for(int i=0; i<sizex; i++)
            for(int j=0; j<=i && j<sizemin; j++)
                u[j][i] = ludc[j][i];
                
        reset_matrix(l, sizemin, sizey);
        for(int j=0; j<sizey; j++)
        {
            l[j][j] = 1;                            // CHECK j < sizemin somewhere!!!
            for(int i=0; i<j; i++)
                l[j][i] = ludc[j][i];
        }

        destroy_matrix(ludc);
    }



    void
    lu(float ** l, float ** u, float ** a, int sizex, int sizey)
    {
        int sizemin = min(sizex, sizey);
        int * pivots = new int [sizemin];
        float ** ludc = copy_matrix(create_matrix(sizex, sizey), a, sizex, sizey);

        lu(ludc, sizex, sizey, NULL, pivots);
        
        convert_pivots(pivots, sizemin);

        reset_matrix(u, sizex, sizemin);
        for(int i=0; i<sizex; i++)
            for(int j=0; j<=i && j<sizemin; j++)
                u[j][i] = ludc[j][i];
                
        reset_matrix(l, sizemin, sizey);
        for(int j=0; j<sizey; j++)
        {
            l[j][pivots[j]] = 1;
            for(int i=0; i<pivots[j]; i++)
                l[j][i] = ludc[pivots[j]][i];
        }

        destroy_matrix(ludc);
    }



    void
    lu(float ** m, float ** a, int sizex, int sizey)
    {
        copy_matrix(m, a, sizex, sizey);
        lu(m, sizex, sizey);
    }
    

/*
*   Old implementation
*
*/

/*
    void
    lu(float ** l, float ** u, float ** a, int sizex, int sizey)
    {
        int sizemin = min(sizex, sizey);
        float ** ludc = copy_matrix(create_matrix(sizex, sizey), a, sizex, sizey);

        lu(ludc, sizex, sizey);

        reset_matrix(u, sizex, sizemin);
        for(int i=0; i<sizex; i++)
            for(int j=0; j<=i && j<sizemin; j++)
                u[j][i] = ludc[j][i];
                
        for(int i=0; i<sizemin; i++)
            for(int j=0; j<=sizey; j++)
                l[j][i] = 0;

        destroy_matrix(ludc);
    }
*/


    static bool
    inv_upper_nounit(float ** a, int size)
    {
        if(a[0][0] == 0)
            return false;
        
        a[0][0] = 1/a[0][0];
        
        float * t = new float [size];
        
        for(int j=1; j<size; j++)
        {
            if(a[j][j]==0)
            {
                delete[] t;
                return false;
            }
            
            a[j][j] = 1/a[j][j];
            float ajj = -a[j][j];
            
            for(int k=0; k<j; k++)
                t[k] = a[k][j];
            
            for(int i=0; i<j; i++)
            {
                double v = 0;
                if(i+1<j)
                    for(int k=i+1; k<j; k++)
                        v += a[i][k]*t[k];
                
                a[i][j] = float(v+a[i][i]*t[i]);
            }
            
            for(int k=0; k<j; k++)
                a[k][j] *= ajj;
        }
        
        delete[] t;
        return true;
    }
    
    
    
    
    // calculate inverse based on LU decomposed matrix
    // by solving the equation inv(a)*L = inv(U) for inv(a)
    
    static bool
    inv_lu(float ** a, int * pivots, int n)
    {
        if(n==0)
            return true;
        
        if(!inv_upper_nounit(a, n))
            return false;
        
        float * t = new float [n];
        
        for(int j=n-1; j>=0; j--)
        {    
            for(int i=j+1; i<n; i++)
            {
                t[i] = a[i][j];
                a[i][j] = 0;
            }
            
            if(j<n-1)
                for(int i=0; i<n; i++)
                {
                    float v = 0;
                    for(int k=j+1; k<n; k++)
                        v = v + a[i][k]*t[k];
                    a[i][j] = a[i][j]-v;
                }
        }
        
        for(int j=n-1; j>=0; j--)
        {
            int jp = pivots[j];
            if(jp != j)
                swap_columns(a, j, jp, n);
        }
        
        delete[] t;
        return true;
    }
    
    
    bool
    qr(float ** q, float ** r, float **a, int size)
    {
        int i,j,k;
        float scale,sigma,sum,tau;
        float c[size];
        float d[size];
        
        copy_matrix(r, a, size, size);
        
        bool singular=false;
        for (k=0;k<size-1;k++)
        {
            scale=0.0;
            
            for (i=k;i<size;i++)
                scale=max(scale, fabsf(r[i][k]));
            
            if (scale == 0.0)
            {
                singular=true;
                c[k]=0.0;
                d[k]=0.0;
            } 
            else
            {
                for(i=k;i<size;i++) r[i][k] /= scale;
                
                for(sum=0.0,i=k;i<size;i++)
                    sum += sqr(r[i][k]);
                
                sigma = r[k][k] > 0 ? sqrt(sum) : fabsf(r[k][k]);
                
                r[k][k] += sigma;
                c[k]=sigma*r[k][k];
                d[k] = -scale*sigma;
                for (j=k+1;j< size;j++) {
                    for (sum=0.0,i=k;i<size;i++) sum += r[i][k]*r[i][j];
                    tau=sum/c[k];
                    for (i=k;i<size;i++) r[i][j] -= tau*r[i][k];
                }
            }
        }
        d[size-1]=r[size-1][size-1];
        if (d[size-1] == 0.0) singular=true;    // -1 added
        
        for (i=0;i<size;i++) {
            for (j=0;j<size;j++) 
                q[j][i]=0.0;
            q[i][i]=1.0;
        }

        for (k=0;k<size-1;k++) {
            if (c[k] != 0.0) {
                for (j=0;j<size;j++) {
                    sum = 0.0;
                    for (i=k;i<size;i++)
                        sum += r[i][k]*q[j][i];
                    sum /= c[k];
                    for (i=k;i<size;i++)
                        q[j][i] -= sum*r[i][k];
                }
            }
        }
        
        for(i=0;i<size;i++)
        {
            r[i][i]=d[i];
            for (j=0;j<i;j++) 
                r[i][j]=0.0;
        }
        
        // make diagonal of r positive
        
        for(int i=0; i<size; i++)
            if(r[i][i] < 0)
                for(int j=0; j<size; j++)
                {
                    r[i][j] = -r[i][j];
                    q[j][i] = - q[j][i];
                }

        return singular;
    }




    // det calculates the determinant of the matrix m using lu decomposition

    float
    det(float ** m, int size)
    {
        float d;
        m = copy_matrix(create_matrix(size, size), m, size, size);
        lu(m, size, size, &d);
        for(int i=0; i<size; i++)
            d *= m[i][i];
        destroy_matrix(m);
        return d;
    }
    
    
    
    // trace calculates the trace of the matrix m

    float
    trace(float ** m, int size)
    {
        float s = 0;
        for(int i=0; i<size; i++)
            s += m[i][i];
        return s;
    }



    // calculate the rank of matrix using svd
	
    float
    rank(float ** m, int sizex, int sizey, float tol)
    {
        int sizemin = min(sizex, sizey);
        float ** u = create_matrix(sizey, sizey);
        float ** v = create_matrix(sizex, sizex);
        float * s = create_array(sizemin);
        svd(u, s, v, m, sizex, sizey);
        if(tol == 0)
            tol = max(sizex, sizey)*eps(max(s, sizemin));
        float r = 0;
        for(int i=0; i<sizemin; i++)    
            if(s[i] > tol)
                r += 1.0;
        destroy_matrix(u);
        destroy_matrix(v);
        destroy_array(s);
        return r;
    }
    

    
    // inv inverts the square matrix b using lu decomposition and store the results in a

    bool
    inv(float ** a, float ** b, int size)
    {
        int * pivots = new int [size];
        copy_matrix(a, b, size, size);
        lu(a, size, size, NULL, pivots);
        bool r = inv_lu(a, pivots, size);
        delete[] pivots;
        return r;
    }



    // pinv calculates the psuedoinverse of b using singular value decomposition
    //      the results is stored in a that has size sizex * sizey
    //      the size of matrix b is sizey x sizex

    void
    pinv(float ** a, float ** b, int sizex, int sizey)
    {
        int sizemin = min(sizex, sizey);
        float ** u = create_matrix(sizex, sizex);
        float ** v = create_matrix(sizey, sizey);
        float * s = create_array(sizemin);
        svd(u, s, v, b, sizey, sizex);
        float ** S = create_matrix(sizex, sizey);
        for(int i=0; i<sizemin; i++)
            if(s[i] != 0) // FIXME: > epsilon?
                S[i][i] = 1/s[i];

        transpose(u, sizex);
        multiply(S, v, S, sizex, sizey, sizey);
        multiply(a, S, u, sizex, sizey, sizex);
        destroy_matrix(u);
        destroy_matrix(v);
        destroy_matrix(S);
        destroy_array(s);
    }



    // chol calculates the Cholesky decomposition of a symmetric positive definite matrix a
    
    float **
    chol(float ** r, float ** a, int size, bool & posdef)
    {    
        posdef = true;
        
        for(int j=0; j<size; j++)
        {
            float s = 0;
            for(int k=0; k<j; k++)
                s = s + r[k][j] * r[k][j];
            
            r[j][j] = a[j][j] - s;
            
            if(r[j][j] <= 0)
            {
                posdef = false;
                return r;
            }
            
            r[j][j] = sqrtf(r[j][j]);
            
            for(int i = j+1; i<size; i++)
            {
                s = 0;
                for(int k = 0; k<j; k++)
                    s = s + r[k][i] * r[k][j];
                
                r[j][i] = (a[j][i] - s) / r[j][j];
            }
        }
        
        return r;
    }
    
    
    float **
    chol(float ** r, float ** a, int size)
    { 
        bool pd;
        return chol(r, a, size, pd);
    }

    
    // (sgetrs.f with arrays x and a only)
    // destroys its argument a
    // r = U(m)\(L(m)\b)
    
    static bool
    mldivide_lu(float * r, float ** m, float * a, int * pivots, int size)
    {
        for(int i=0; i<size; i++)
            if(m[i][i] == 0)
                return false;
        
        for(int i=0; i<size; i++)
            if(pivots[i] != i)
            {
                float t = a[i];
                a[i] = a[pivots[i]];
                a[pivots[i]] = t;
            }
        
        // r = L\b (sgetrs: do 90)
        for(int i=1; i<size; i++)
        {
            float v = 0.0;
            for(int k = 0; k<i; k++)
                v = v + m[i][k]*a[k];
            a[i] = a[i]-v;
        }
        
        // r = U\r (sgetrs: do 130)
        r[size-1] = a[size-1]/m[size-1][size-1];
        for(int i=size-2; i>=0; i--)
        {
            float v = 0.0;
            for(int k = i; k<size; k++)
                v = v + m[i][k]*r[k];
            
            r[i] = (a[i]-v)/m[i][i];
        }
        
        return true;
    }
    
    
    
    float **
    mldivide(float ** r, float ** m, float ** a, int sizex, int sizey, int n)
    {
        float ** p = create_matrix(n, sizey);
        pinv(p, m, n, sizey);
        multiply(r, p, a, sizex, sizey, n);
        destroy_matrix(p);
        return r;
    }


    
    // solves the equation mr = a
    // r and a arrays
    // m square matrix
    
    float *
    mldivide(float * r, float ** m, float * a, int size)
    {
        int * pivots = new int [size];
        m = copy_matrix(create_matrix(size, size), m, size, size);
        a = copy_array(create_array(size), a, size);
        lu(m, size, size, NULL, pivots);
        mldivide_lu(r, m, a, pivots, size);
        destroy_matrix(m);
        destroy_array(a);
        return r;
    }
    
    
    
    float *
    mldivide(float * r, float ** m, float * a, int sizex, int sizey)
    {
        if(sizex == sizey)
            return mldivide(r, m, a, sizex);

        float ** rp = (float **)malloc(sizey*sizeof(float *));
        for (int j=0; j<sizey; j++)
            rp[j] = &r[j];

        float ** ap = (float **)malloc(sizey*sizeof(float *));
        for (int j=0; j<sizey; j++)
            ap[j] = &a[j];

        
        r = *mldivide(rp, m, ap, 1, sizex, sizey);
        
        free(rp);
        free(ap);
        
        return r;
    }
    

    
    
    // singular value decomposition

#ifdef USE_LAPACK
    static int
    svd_lapack(float ** u, float * s, float ** v, float ** a, int n, int m)
    {
        char    jobz = 'a';
        int     info = 0;
        float   work_query;
        int     lwork = -1;
        int *   iwork = (int*)malloc(sizeof(int)*max(1,8*min(m,n)));

        float ** a_t = transpose(create_matrix(m, n), a, m, n); // TODO: set parameters to avaoid this; see https://software.intel.com/sites/products/documentation/doclib/mkl_sa/11/mkl_lapack_examples/lapacke_sgesdd_row.c.htm
        
        // Query optimal working array(s) size
        
        sgesdd_(&jobz, &m, &n, *a_t, &m, s, *u, &m, *v, &n, &work_query, &lwork, iwork, &info);
        
        if(info < 0)
            info -= 1;
       
        if(info != 0)
        {
            free(iwork);
            return -1;
        }
        
        lwork = (int)work_query;
        float * work = (float*)malloc(sizeof(float)*lwork);
        
        sgesdd_(&jobz, &m, &n, *a_t, &m, s, *u, &m, *v, &n, work, &lwork, iwork, &info);
        
        if( info < 0 )
            info -= 1;

        // Transpose output matrices

        transpose(u, m);

        destroy_matrix(a_t);
        free(work);
        free(iwork);

        return info;
    }
#endif


    // svd helper functions

    int
    svd_float(float **u, float *q, float **v, float **a, int n, int m)
    {
        float eps = 1e-16;          // error threshold (e.g. 1.e-6)
        const float tol = 1e-16;    // tolerance threshold)
     
        int i,j,k,l=0,l1,iter;
        double c,f,h,s,y,z;
        
        int retval = 0;
        double *e = (double *)calloc(n, sizeof(double));

        for (int j=0;j<m;j++)
            for (int i=0;i<n;i++)
                u[j][i] = a[j][i];

    //  Householder's reduction to bidiagonal form

        double g = 0.0;
        double x = 0.0;    
        for (i=0;i<n;i++)
        {
            e[i] = g;
            s = 0.0;
            l = i+1;
            
            for (j=i;j<m;j++)
                s += u[j][i]*u[j][i];
                
            if (s < tol)
                g = 0.0;
            else
            {
                f = u[i][i];
                g = (f < 0) ? sqrtf(s) : -sqrtf(s);
                h = f * g - s;
                u[i][i] = f - g;
                
                for (j=l;j<n;j++)
                {
                    s = 0.0;
                    for (k=i;k<m;k++)
                        s += u[k][i]*u[k][j];
                        
                    f = s / h;
                    for (k=i;k<m;k++)
                        u[k][j] += f*u[k][i];
                }
            } 
            
            q[i] = g;
            s = 0.0;
            
            for (j=l;j<n;j++)
                s += u[i][j]*u[i][j];
                
            if (s < tol)
                g = 0.0;
            else
            {
                f = u[i][i+1];
                g = (f < 0) ? sqrtf(s) : -sqrtf(s);
                h = f * g - s;
                u[i][i+1] = f - g;
                
                for (j=l;j<n;j++) 
                    e[j] = u[i][j]/h;
                    
                for (j=l;j<m;j++)
                {
                    s = 0.0;
                    
                    for (k=l;k<n;k++) 
                        s += u[j][k]*u[i][k];
                        
                    for (k=l;k<n;k++)
                        u[j][k] += s*e[k];
                } 
            }
            
            y = fabs(q[i]) + fabs(e[i]);                         
            if (y > x)
                x = y;
        } 

    // Accumulation of right-hand transformations

            for (i=n-1;i>=0;i--)
            {
                if (g != 0.0) {
                    h = u[i][i+1] * g;
                    
                    for (j=l;j<n;j++)
                        v[j][i] = u[i][j]/h;
                        
                    for (j=l;j<n;j++) 
                    {
                        s = 0.0;
                        for (k=l;k<n;k++) 
                            s += u[i][k]*v[k][j];
                            
                        for (k=l;k<n;k++)
                            v[k][j] += s*v[k][i];
                    } 
                } 
                
                for (j=l;j<n;j++)
                {
                    v[i][j] = 0.0;
                    v[j][i] = 0.0;
                }
                
                v[i][i] = 1.0;
                g = e[i];
                l = i;
            } 
     
        

    // Accumulation of left-hand transformations
        
            for (i=n;i<m;i++)
            {
                for (j=n;j<m;j++)
                    u[i][j] = 0.0;
                    
                u[i][i] = 1.0;
            }
        
        
            for (i=n-1;i>=0;i--)
            {
                l = i + 1;
                g = q[i];
                for (j=l;j<m;j++)  // upper limit was 'n' 
                    u[i][j] = 0.0;
                    
                if (g != 0.0)
                {
                    h = u[i][i] * g;
                    
                    for (j=l;j<m;j++) // upper limit was 'n'
                    { 
                        s = 0.0;
                        for (k=l;k<m;k++)
                            s += u[k][i]*u[k][j];
                            
                        f = s / h;
                        for (k=i;k<m;k++) 
                            u[k][j] += f*u[k][i];
                    } 
                    
                    for (j=i;j<m;j++) 
                        u[j][i] /= g;
                } 
                
                else
                {
                    for (j=i;j<m;j++)
                        u[j][i] = 0.0;
                }
                
                u[i][i] += 1.0;
            } 
        
    // Diagonalization of the bidiagonal form

        eps *= x;
        for (k=n-1;k>=0;k--)
        {
            iter = 0;
            
    test_f_splitting:

            for (l=k;l>=0;l--)
            {
                if (fabs(e[l]) <= eps) goto test_f_convergence;
                if (fabs(q[l-1]) <= eps) goto cancellation;
            }

    // Cancellation of e[l] if l > 0

    cancellation:

            c = 0.0;
            s = 1.0;
            l1 = l - 1;
            for (i=l;i<=k;i++)
            {
                f = s * e[i];
                e[i] *= c;
                if (fabs(f) <= eps) goto test_f_convergence;
                g = q[i];
                h = q[i] = hypotf(f, g);
                c = g / h;
                s = -f / h;
                
                for (j=0;j<m;j++)
                {
                    y = u[j][l1];
                    z = u[j][i];
                    u[j][l1] = y * c + z * s;
                    u[j][i] = -y * s + z * c;
                } 
            }
            
    test_f_convergence:

            z = q[k];
            if (l == k) goto convergence;

    // Shift from bottom 2x2 minor

            iter++;
            if (iter > 30)
            {
                retval = k;
                break;
            }
            
            x = q[l];
            y = q[k-1];
            g = e[k-1];
            h = e[k];
            f = ((y-z)*(y+z) + (g-h)*(g+h)) / (2*h*y);
            g = hypotf(f, 1.0);
            f = ((x-z)*(x+z) + h*(y/((f<0)?(f-g):(f+g))-h))/x;

    // QR transformation

            c = s = 1.0;
            for (i=l+1;i<=k;i++)
            {
                g = e[i];
                y = q[i];
                h = s * g;
                g *= c;
                e[i-1] = z = hypotf(f, h);
                c = f / z;
                s = h / z;
                f = x * c + g * s;
                g = -x * s + g * c;
                h = y * s;
                y *= c;
                
                for (j=0;j<n;j++)
                {
                    x = v[j][i-1];
                    z = v[j][i];
                    v[j][i-1] = x * c + z * s;
                    v[j][i] = -x * s + z * c;
                } 
            
                q[i-1] = z = hypotf(f, h);
                c = f/z;
                s = h/z;
                f = c * g + s * y;
                x = -s * g + c * y;
                
                for (j=0;j<m;j++)
                {
                    y = u[j][i-1];
                    z = u[j][i];
                    u[j][i-1] = y * c + z * s;
                    u[j][i] = -y * s + z * c;
                } 
                 
            }
            e[l] = 0.0;
            e[k] = f;
            q[k] = x;
            goto test_f_splitting;
            
    convergence:
            if (z < 0.0) 
            {
                q[k] = - z; //q[k] is made non-negative
                for (j=0;j<n;j++)
                    v[j][k] = -v[j][k];
            } 
        } 
        
        free(e);
        
    // Sort singular values and vectors
        
        int inc=1;
        float sw;
        float * su = create_array(m);
        float * sv = create_array(n);
        do {inc *= 3; inc++;} while (inc <= n);
        
        do {
            inc /= 3;
            for (i=inc;i<n;i++)
            {
                sw = q[i];
                for (k=0;k<m;k++) su[k] = u[k][i]; 
                for (k=0;k<n;k++) sv[k] = v[k][i]; 
                j = i;
                while (q[j-inc] < sw)
                {
                    q[j] = q[j-inc];            
                    for (k=0;k<m;k++) u[k][j] = u[k][j-inc];
                    for (k=0;k<n;k++) v[k][j] = v[k][j-inc];
                    j -= inc;
                    if (j < inc) break;
                }
                q[j] = sw;
                
                for (k=0;k<m;k++) u[k][j] = su[k];
                for (k=0;k<n;k++) v[k][j] = sv[k];
            }
        } while (inc > 1);

        for (k=0;k<n;k++)
        {
            int s=0;
            
            for (i=0;i<m;i++) if (u[i][k] < 0.) s++; 
            for (j=0;j<n;j++) if (v[j][k] < 0.) s++;
             
            if (s > (m+n)/2)
            {
                for (i=0;i<m;i++)
                    u[i][k] = -u[i][k];
                    
                for (j=0;j<n;j++)
                    v[j][k] = -v[j][k];
            }
        }
       
        destroy_array(su);
        destroy_array(sv);
        
        return retval;
    }



#ifdef USE_LAPACK
    #define svd_implementation  svd_lapack
#else
    #define svd_implementation  svd_float
#endif            

    int
    svd(float ** u, float * s, float ** v, float ** m, int sizex, int sizey)
    {
        int k = 0;
        
        if(sizex > sizey)
        {
            // Transpose a and switch u and v in calls below
         
            float ** mt = transpose(create_matrix(sizey, sizex), m, sizey, sizex);
            k = svd_implementation(v, s, u, mt, sizey, sizex);
            destroy_matrix(mt);
            return k;
        }
        
        else
        {
            return svd_implementation(u, s, v, m, sizex, sizey);
        }
    }
    


    float **
    pca(float ** c, float * v, float ** m, int size_x, int size_y)
    {
        float ** x = copy_matrix(create_matrix(size_x, size_y), m, size_x, size_y);
        float * mx = create_array(size_x);
        mean(mx, x, size_x, size_y);
        subtract(x, x, mx, size_x, size_y);
        float ** xT = transpose(create_matrix(size_y, size_x), x, size_y, size_x);
        float ** cov = multiply(multiply(create_matrix(size_x, size_x), xT, x, size_x, size_x, size_y), 1/float(size_y-1), size_x, size_x);
        float ** V = create_matrix(size_x, size_x);

        svd(c, v, V, cov, size_x, size_x);

        destroy_matrix(V);
        destroy_matrix(cov);
        destroy_matrix(xT);
        destroy_matrix(x);

        return c;
    }



    
    // conversion
    // MARK: -
    // MARK: conversion
/*
	int
	lround(float x)
	{
		return int(::lroundf(x));
	}
*/

    int 
    map_to_bin(float x, int bins, float range_low, float range_high)
    {
        int y = floor(float(bins)*(x-range_low)/(range_high-range_low));
        if(y<0)
            return 0;
        else if(y>bins-1)
            return bins-1;
        else
            return y;
    }

	void
	float_to_byte(unsigned char * r, float * a, float min, float max, long size)
	{
#ifdef USE_VIMAGE
		struct vImage_Buffer src =
        {
            a, 1, static_cast<vImagePixelCount>(size), size*sizeof(float)
        };
		struct vImage_Buffer dest =
        {
            r, 1, static_cast<vImagePixelCount>(size), size*sizeof(float)
        };
		vImage_Error err = vImageConvert_PlanarFtoPlanar8 (&src, &dest, max, min, 0);
		if (err < 0) printf("IKAROS_Math:float_to_byte vImage_Error = %ld\n", err);
#else
		for (long i=0; i<size; i++)
			if (a[i] < min)
				r[i] = 0;
			else if (a[i] > max)
				r[i] = 255;
			else
				r[i] = int(255.0*(a[i]-min)/(max-min));
#endif
	}
	
	void
	byte_to_float(float * r, unsigned char * a, float min, float max, long size)
	{
		for (long i=0; i<size; i++)
			r[i] = ((max-min)/255.0) * float(a[i]) + min;
	}
	

    int
    string_to_int(const std::string & s, int d)
    {
        try
        {
            if (s.empty())
                return d;
            else
                return stoi(s, NULL, 0);
        }
        catch(const std::invalid_argument& ia)
        {
            printf("Conversion error: %s cannot be converted to int.", s.c_str());
            return d;
        }
    }

    long
    string_to_long(const std::string & s, long d)
    {
        try
        {
            if (s.empty())
                return d;
            else
                return stol(s, NULL, 0);
        }
        catch(const std::invalid_argument& ia)
        {
            printf("Conversion error: %s cannot be converted to long.", s.c_str());
            return d;
        }
    }

    float
    string_to_float(const std::string & s, float d)
    {
        try
        {
            if (s.empty())
                return d;
            else
                return stof(s);
        }
        catch(const std::invalid_argument& ia)
        {
            printf("Conversion error: %s cannot be converted to float.", s.c_str());
            return d;
        }
    }

    bool
    string_to_bool(const std::string & s, bool d)
    {
        if (s.empty())
            return d;
        static std::string str = "trueTrueTRUEyesYesYES1"; // values like "rueTE" will also evaluate to true but that is ok
        return str.find(s) != std::string::npos;
    }


	int
	string_to_int(const char * s, int d)
	{
		if (s == NULL || s[0] == 0) // FIXME: check that this does not destreoy something
			return d;
		else
			return int(strtol(s, NULL, 0));	// allows hex and octal formats as well as decimal
	}
	
	float
	string_to_float(const char * s, float d)
	{
		if (s == NULL)
			return d;
		else
            return strtof(s, NULL);
	}
	
	char *
	int_to_string(char * s, int i, int n)
	{
		snprintf(s, n, "%d", i);
		return s;
	}
	
	char *
	float_to_string(char * s, float v, int decimals, int n)
	{
		snprintf(s, n, "%.*f", decimals, v);
		return s;
	}
    
    float angle_to_angle(float angle, int from_angle_unit, int to_angle_unit)
    {
        switch(from_angle_unit)
        {
            default:
            case 0: angle = angle; break;
            case 1: angle = (angle/(2.0*pi))*360; break;
            case 2: angle = angle*360; break;
        }
        switch(to_angle_unit)
        {
            default:
            case 0: angle = angle;break;
            case 1: angle = angle/360*(2.0*pi);break;
            case 2: angle = angle/360;break;
        }
        return angle;
    }


    float
    short_angle(float a1, float a2)
    {
        return ::atan2(sin(a2-a1), cos(a2-a1));
    }



    //
    // homogenous 4x4 matrices represented as h_matrix = float[16]
    // MARK: -
    // MARK: homogenous matrices
    
    float *
    h_reset(h_matrix r)
    {
        r[ 0] = 0; r[ 1] = 0; r[ 2] = 0; r[ 3] = 0;
        r[ 4] = 0; r[ 5] = 0; r[ 6] = 0; r[ 7] = 0;
        r[ 8] = 0; r[ 9] = 0; r[10] = 0; r[11] = 0;
        r[12] = 0; r[13] = 0; r[14] = 0; r[15] = 0;
        return r;
    }
    
    float *
    h_eye(h_matrix r)
    {
        r[ 0] = 1; r[ 1] = 0; r[ 2] = 0; r[ 3] = 0;
        r[ 4] = 0; r[ 5] = 1; r[ 6] = 0; r[ 7] = 0;
        r[ 8] = 0; r[ 9] = 0; r[10] = 1; r[11] = 0;
        r[12] = 0; r[13] = 0; r[14] = 0; r[15] = 1;
        return r;
    }
    
    float *
    h_translation_matrix(h_matrix r, float tx, float ty, float tz)
    {
        r[ 0] = 1; r[ 1] = 0; r[ 2] = 0; r[ 3] = tx;
        r[ 4] = 0; r[ 5] = 1; r[ 6] = 0; r[ 7] = ty;
        r[ 8] = 0; r[ 9] = 0; r[10] = 1; r[11] = tz;
        r[12] = 0; r[13] = 0; r[14] = 0; r[15] = 1;
        return r;
    }
    

    float *
    h_rotation_matrix(h_matrix r, axis a, float alpha)
    {
        float s = sin(alpha);
        float c = cos(alpha);
        
        switch(a)
        {
            case X:
                r[ 0] = 1; r[ 1] = 0; r[ 2] = 0; r[ 3] = 0;
                r[ 4] = 0; r[ 5] = c; r[ 6] =-s; r[ 7] = 0;
                r[ 8] = 0; r[ 9] = s; r[10] = c; r[11] = 0;
                r[12] = 0; r[13] = 0; r[14] = 0; r[15] = 1;
                break;
            
            case Y:
                r[ 0] = c; r[ 1] = 0; r[ 2] = s; r[ 3] = 0;
                r[ 4] = 0; r[ 5] = 1; r[ 6] = 0; r[ 7] = 0;
                r[ 8] =-s; r[ 9] = 0; r[10] = c; r[11] = 0;
                r[12] = 0; r[13] = 0; r[14] = 0; r[15] = 1;
                break;
            
            case Z:
                r[ 0] = c; r[ 1] = -s; r[ 2] = 0; r[ 3] = 0;
                r[ 4] = s; r[ 5] = c; r[ 6] = 0; r[ 7] = 0;
                r[ 8] = 0; r[ 9] = 0; r[10] = 1; r[11] = 0;
                r[12] = 0; r[13] = 0; r[14] = 0; r[15] = 1;
                break;
        }
        return r;
    }

    float *
    h_rotation_matrix(h_matrix r, float x, float y, float z)
    {
        h_matrix rX,rY,rZ;
        h_rotation_matrix(rX, X, x);
        h_rotation_matrix(rY, Y, y);
        h_rotation_matrix(rZ, Z, z);
        h_copy(r, rZ);
        h_multiply(r, r, rY);
        h_multiply(r, r, rX);
        return r;
    }

    float *
    h_reflection_matrix(h_matrix r, axis a)
    {
        float x = (a == X ? -1 : 1);
        float y = (a == Y ? -1 : 1);
        float z = (a == Z ? -1 : 1);
        r[ 0] = x; r[ 1] = 0; r[ 2] = 0; r[ 3] = 0;
        r[ 4] = 0; r[ 5] = y; r[ 6] = 0; r[ 7] = 0;
        r[ 8] = 0; r[ 9] = 0; r[10] = z; r[11] = 0;
        r[12] = 0; r[13] = 0; r[14] = 0; r[15] = 1;
        return r;
    }
    

    float *
    h_scaling_matrix(h_matrix r, float sx, float sy, float sz)
    {
        r[ 0] =sx; r[ 1] = 0; r[ 2] = 0; r[ 3] = 0;
        r[ 4] = 0; r[ 5] =sy; r[ 6] = 0; r[ 7] = 0;
        r[ 8] = 0; r[ 9] = 0; r[10] =sz; r[11] = 0;
        r[12] = 0; r[13] = 0; r[14] = 0; r[15] = 1;
        return r;
    }


    void
    h_get_translation(const h_matrix m, float & x, float & y, float &z)
    {
        x = m[3];
        y = m[7];
        z = m[11];
    }
    
    void
    h_get_euler_angles(const h_matrix m, float & x, float & y, float &z) // ZYX
    {
        if (m[8] < +1)
        {
            if (m[8] > -1)
            {
                y = asin(-m[8]);
                z = ::atan2(m[4],m[0]);
                x = ::atan2(m[9],m[10]);
            }
            else // m8 = -1
            {
                y = +pi/2;
                z = -::atan2(-m[6],m[5]);
                x = 0;
            }
        }
        else // m8 = +1
        {
            y = -pi/2;
            z = ::atan2(-m[6],m[5]);
            x = 0;
        }
    }

    
    float
    h_get_euler_angle(const h_matrix m, axis a)
    {
        switch(a)
        {
            case X:
                if (-1 < m[8] && m[8] < +1)
                    return ::atan2(m[9], m[10]);
                else
                    return  0;
            
            case Y:
                if(m[8] >= +1)
                    return  -pi/2;
                else if (m[8] > -1)
                    return asin(-m[8]);
                else
                    return +pi/2;

            case Z:
                if(m[8] >= +1)
                    return  ::atan2(-m[6], m[5]);
                else if (m[8] > -1)
                    return ::atan2(m[4], m[0]);
                else
                    return -::atan2(-m[6], m[5]);
        }
        
        return 0;
    }

    void 
    h_get_scale(const h_matrix m, float &x, float &y, float &z)
    {
        x = m[0];
        y = m[5];
        z = m[10];
    }    
    

    // operations

    float *
    h_multiply(h_matrix r, h_matrix a, h_matrix b)
    {
        h_matrix t;
        
        t[0]  = b[0]*a[0]  + b[4]*a[1]  + b[8]*a[2]  + b[12]*a[3];
        t[1]  = b[1]*a[0]  + b[5]*a[1]  + b[9]*a[2]  + b[13]*a[3];
        t[2]  = b[2]*a[0]  + b[6]*a[1]  + b[10]*a[2]  + b[14]*a[3];
        t[3]  = b[3]*a[0]  + b[7]*a[1]  + b[11]*a[2]  + b[15]*a[3];

        t[4]  = b[0]*a[4]  + b[4]*a[5]  + b[8]*a[6]  + b[12]*a[7];
        t[5]  = b[1]*a[4]  + b[5]*a[5]  + b[9]*a[6]  + b[13]*a[7];
        t[6]  = b[2]*a[4]  + b[6]*a[5]  + b[10]*a[6]  + b[14]*a[7];
        t[7]  = b[3]*a[4]  + b[7]*a[5]  + b[11]*a[6]  + b[15]*a[7];

        t[8]  = b[0]*a[8]  + b[4]*a[9]  + b[8]*a[10] + b[12]*a[11];
        t[9]  = b[1]*a[8]  + b[5]*a[9]  + b[9]*a[10] + b[13]*a[11];
        t[10] = b[2]*a[8]  + b[6]*a[9]  + b[10]*a[10] + b[14]*a[11];
        t[11] = b[3]*a[8]  + b[7]*a[9]  + b[11]*a[10] + b[15]*a[11];

        // The following lines could be optimized away if it really is
        // a homogenous matrix. Should always be [0, 0, 0, 1]

        t[12] = b[0]*a[12] + b[4]*a[13] + b[8]*a[14] + b[12]*a[15];
        t[13] = b[1]*a[12] + b[5]*a[13] + b[9]*a[14] + b[13]*a[15];
        t[14] = b[2]*a[12] + b[6]*a[13] + b[10]*a[14] + b[14]*a[15];
        t[15] = b[3]*a[12] + b[7]*a[13] + b[11]*a[14] + b[15]*a[15];
        
        return copy_array(r, t, 16);
    }
    
    
    float *
    h_multiply_v(h_vector r, h_matrix m, h_vector v)
    {
        float t[4];
        t[0] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3] * v[3];
        t[1] = m[4] * v[0] + m[5] * v[1] + m[6] * v[2] + m[7] * v[3];
        t[2] = m[8] * v[0] + m[9] * v[1] + m[10] * v[2] + m[11] * v[3];
        t[3] = m[12] * v[0] + m[13] * v[1] + m[14] * v[2] + m[15] * v[3];

        if(t[3] != /* DISABLES CODE */ (0) && false) // normalize vector
        {
            r[0] = t[0] / t[3];
            r[1] = t[1] / t[3];
            r[2] = t[2] / t[3];
            r[3] = 1;
        }
        else // should never happen
        {
            r[0] = t[0];
            r[1] = t[1];
            r[2] = t[2];
            r[3] = 1;
        }
        
        return r;
    }
    
    
    float *
    h_transpose(h_matrix r, h_matrix a)
    {
        h_matrix t;

        t[ 0] = a[ 0]; t[ 1] = a[ 4]; t[ 2] = a[ 8]; t[ 3] = a[12];
        t[ 4] = a[ 1]; t[ 5] = a[ 5]; t[ 6] = a[ 9]; t[ 7] = a[13];
        t[ 8] = a[ 2]; t[ 9] = a[ 6]; t[10] = a[10]; t[11] = a[14];
        t[12] = a[ 3]; t[13] = a[ 7]; t[14] = a[11]; t[15] = a[15];

        return copy_array(r, t, 16);
    }
    
    
    float *
    h_inv(h_matrix r, h_matrix a)
    {
        float * tr[4];
        float * ta[4];
        
        inv(h_temp_matrix(r, tr), h_temp_matrix(a, ta), 4);
        return r;
    }
    
    float *
    h_copy(h_matrix r, h_matrix a)
    {
        for(int i=0; i<16; i++)
            r[i] = a[i];
        return r;
    }

    float *
    h_copy_v(h_vector r, h_vector a)
    {
        r[0] = a[0];
        r[1] = a[1];
        r[2] = a[2];
        r[3] = a[3];
        return r;
    }
    
    float *
    h_add(h_matrix r, h_matrix a)
    {
        for(int i=0; i<16; i++)
            r[i] += a[i];
        return r;
    }
    
    float *
    h_add(h_matrix r, float alpha, h_matrix a, float beta, h_matrix b)
    {
        for(int i=0; i<16; i++)
            r[i] = alpha*a[i] + beta*b[i];
        return r;
    }

    float
    h_dist(h_matrix a, h_matrix b)
    {
        return sqrt(sqr(h_get_x(a)-h_get_x(b)) + sqr(h_get_y(a)-h_get_y(b)) + sqr(h_get_z(a)-h_get_z(b)));
    }

    float *
    h_multiply(h_matrix r, float c)
    {
        for(int i=0; i<16; i++)
            r[i] *= c;
        return r;
    }

    float *
    h_normalize_rotation(h_matrix m)
    {
        float s[3];
        float * t[4];

        float ** u = create_matrix(3, 3);
        float ** v = create_matrix(3, 3);
        float ** r = create_matrix(3, 3);
        
        svd(u, s, v, h_temp_matrix(m, t), 3, 3);

        float ** vt = transpose(create_matrix(3, 3), v, 3, 3);

        multiply(r, u, vt, 3, 3, 3);

        for(int i=0; i<3; i++)
            for(int j=0; j<3; j++)
                m[j*4+i] = r[j][i];

        destroy_matrix(vt);
        destroy_matrix(r);
        destroy_matrix(v);
        destroy_matrix(u);

        return m;
    }

    // utilities
    
    float **
    h_temp_matrix(h_matrix r, float * (&p)[4])
    {
        p[0] = &r[0];
        p[1] = &r[4];
        p[2] = &r[8];
        p[3] = &r[12];
        return p;
    }


    void
    h_print_matrix(const char * name, h_matrix m, int decimals)
    {
        float * t[4];
        print_matrix(name, h_temp_matrix(m, t), 4, 4, decimals);
    };
    

    float **
    h_create_matrix(h_matrix h)
    {
        float ** r = create_matrix(4, 4);
        int ix = 0;
        for(int j=0; j<4; j++)
            for(int i=0; i<4; i++)
                r[j][i] = h[ix++];
        return r;
    }
 
    float *
    h_create_matrix(h_matrix &r, float *a)
    {
        memcpy(r, a, 16*sizeof(float));
        return r;
    }

    
    float **
    h_set_matrix(float ** m, h_matrix h)
    {
        int ix = 0;
        for(int j=0; j<4; j++)
            for(int i=0; i<4; i++)
                m[j][i] = h[ix++];
        return m;
    }

    
    
    
    
    

    
	// misc functions
	
	int
	select_boltzmann(float * a, int size, float T)
	{
		double sum = 0;
		double alpha = 1/T;
		
		for (int i=0; i<size; i++)
			sum += ::exp(alpha*a[i]);
		
		if (sum == 0)
			return size/2;
		
		double	rs = (double)random(0.0f, (float)sum);
		double	ss = 0;
		for (int i=0; i<size; i++)
		{
			ss += ::exp(alpha*a[i]);
			if (ss >= rs)
				return i;
		}
		
		return size-1;
	}
	
	void
	select_boltzmann(int & x, int & y, float ** m, int sizex, int sizey, float T)
	{
		double sum = 0;
		double alpha = 1/T;
		for (int j=0; j<sizey; j++)
			for (int i=0; i<sizex; i++)
				sum += ::exp(alpha*m[j][i]);
		if (sum == 0)
		{
			x = sizex/2;
			y = sizey/2;
			return;
		}
		double	rs = (double)random(0.0f, (float)sum);
		double	ss = 0;
		for (int j=0; j<sizey; j++)
			for (int i=0; i<sizex; i++)
				if ((ss += ::exp(alpha*m[j][i])) >= rs)
				{
					x = i;
					y = j;
					return;
				}
		
		x = sizex-1;
		y = sizey-1;
	}
	
	// Simple minded recursive implementation
	//
	// FIXME: It will not follow a gradient along the edges of the matrix/image ***
	void
	ascend_gradient(int & x, int & y, float ** m, int sizex, int sizey)
	{
		if (x == 0)		return;
		if (y == 0)		return;
		if (x == sizex-1)	return;
		if (y == sizey-1)	return;
		int mx = x;
		int my = y;
		for (int i=-1; i<=1; i++)
			for (int j=-1; j<=1; j++)
				if (m[y+j][x+i] > m[my][mx])
				{
					mx = x+i;
					my = y+j;
				}
		if (mx != x || my != y)
		{
			x = mx;
			y = my;
			ascend_gradient(x, y, m, sizex, sizey);
		}
	}
	
	void
	descend_gradient(int & x, int & y, float ** m, int sizex, int sizey)
	{
		if (x == 0)		return;
		if (y == 0)		return;
		if (x == sizex-1)	return;
		if (y == sizey-1)	return;
		int mx = x;
		int my = y;
		for (int i=-1; i<=1; i++)
			for (int j=-1; j<=1; j++)
				if (m[y+j][x+i] < m[my][mx])
				{
					mx = x+i;
					my = y+j;
				}
		if (mx != x || my != y)
		{
			x = mx;
			y = my;
			descend_gradient(x, y, m, sizex, sizey);
		}
	}
    
    
    //
    // sorting
    //
    
    // insertion sort

    float *
    sort(float * a, long size)
    {
        int i , j;
        float t;
        for(i = 1; i < size; i++)
        {
            t = a[i];
            for(j = i; j > 0 && t < a[j-1]; j--)
                a[j] = a[j-1];
            a[j] = t;
        }
        return a;
    }


    float **
    sort(float ** a, long sizex, long sizey)
    {
        sort(*a, sizex*sizey);
        return a;
    }



    // algorithm from http://rosettacode.org/wiki/Sorting_algorithms/Quicksort#C

    float *
    quick_sort (float * a, long size) // TODO: Test
        {
        if (size < 2)
            return a;
        float p = a[size / 2];
        float * l = a;
        float *r = a + size - 1;
        while (l <= r)
        {
            if (*l < p)
            {
                l++;
                continue;
            }
            if (*r > p)
            {
                r--;
                continue;
            }
            int t = *l;
            *l++ = *r;
            *r-- = t;
        }
        quick_sort(a, r - a + 1);
        quick_sort(l, a + size - l);
        
        return a;
    }



    
    // mark - TAT additions
    float *
    soft_max(float *r, float *a, int size)
    {
        // TODO check if vector op exists
        for (int i=0; i < size; ++i)
            r[i] = exp(a[i]);
        float s=0;
        for (int i=0; i < size; ++i)
            s += r[i];
        if(s>0)
            for (int i=0; i < size; ++i)
                r[i] = r[i]/s;
        else
            reset_array(r, size);
        return r;
    }

    float *
    soft_max_pw(float *r, float *a, float pw, int size)
    {
        // TODO check if vector op exists
        for (int i=0; i < size; ++i)
            r[i] = pow(a[i], pw);
        float s=0;
        for (int i=0; i < size; ++i)
            s += r[i];
        if(s>0)
            for (int i=0; i < size; ++i)
                r[i] = r[i]/s;
        else
            reset_array(r, size);
        return r;
    }
	
    // per element multiplication where a is a col vec and b is a row vec
    // interpreted as b being replicated to size of a and scaled by it
    // r(sizey, sizex) = a(sizey)*b(sizex)
    float **
    multiply(float **r, float *a, float *b, int sizex, int sizey)
    {

        for (int i = 0; i < sizey; ++i)
            multiply(r[i], b, a[i], sizex);
        
        return r;
    }
	
    // per element multiplication where a is col vec and b is matrix
    // r(sizey, sizex) = a(sizey) * b(sizey, sizex)
    float **
    multiply(float **r, float *a, float **b, int sizex, int sizey)
    {
        for (int i = 0; i < sizey; ++i)
            multiply(r[i], b[i], a[i] , sizex);
        return r;
    }
	
    //
	// Multiply with transpose. Input b will be transposed by BLAS call
	// Expects: 
	// a rows:sizey, cols:n
	// b rows:n, cols:sizex (will be transposed to have rows: sizex cols:n)
	// r rows:sizex cols:sizey
    float **
    multiply_t(float ** r, float ** a, float ** b, int sizex, int sizey, int n)
    {
        float ** aa = a;
        float ** bb = b;

        if(r==a)
            aa = copy_matrix(create_matrix(sizex, sizey), a, sizex, sizey);

        if(r==b)
            bb = copy_matrix(create_matrix(sizex, sizey), b, sizex, sizey);
        
#ifdef USE_BLAS
        cblas_sgemm(CblasRowMajor, CblasTrans, CblasTrans,
                    sizex, sizey, n, 1.0, *bb, sizex, *aa, n, 0.0, *r, sizey);
#else
		printf("WARNING: multiply_t currently only works with blas enabled\n");
#endif

        if(a!=aa)
            destroy_matrix(aa);
            
        if(b!=bb)
            destroy_matrix(bb);

        return r;
	}

	// float **	multiply(float ** r, float alpha, float ** a, float ** b, int sizex, int sizey, int n);	// alpha*matrix x matrix; size of result, n = columns of a = rows of b
    float **
    multiply(float ** r, float alpha, float ** a, float ** b, int sizex, int sizey, int n)
    {
        float ** aa = a;
        float ** bb = b;

        if(r==a)
            aa = copy_matrix(create_matrix(sizex, sizey), a, sizex, sizey);

        if(r==b)
            bb = copy_matrix(create_matrix(sizex, sizey), b, sizex, sizey);
        
#ifdef USE_BLAS
        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    sizey, sizex, n, alpha, *aa, n, *bb, sizex, 0.0, *r, sizex);
#else
        float s;
        int n1 = n-1;
        for (int j=0; j<sizey; j++)
        {
            for (int i=0; i<sizex; i++)
            {
                s = 0;
                float * aj = aa[j];
                float * bki = &bb[0][i];
                for (int k=n1; k>=0; k--)
                {
                    s += (*aj++) * (*bki);
                    bki += sizex;
                }
                r[j][i] = alpha*s;
            }
        }
#endif
        if(a!=aa)
            destroy_matrix(aa);
            
        if(b!=bb)
            destroy_matrix(bb);

        return r;
    }

	// // linear algebra - TAT
    // int 		eigs(float **result, float **matrix, int sizex, int sizey);
    // void 		sprand(float *array, int size, float fillfactor, float min, float max, float meanval, float var);
    // void 		gen_weight_matrix(float **returnmat, int dim, float fillfactor);
    
    // // various - TAT
    // float *		tanh(float *array, int size);
	// float *		tanh(float *r, float *a, int size);
    // float **    tanh(float ** matrix, int sizex, int sizey);
	// float **	tanh(float **r, float **a, int sizex, int sizey);
	// float *		atanh(float *array, int size);
	// float		sigmoidf(float a);
	// float *		sigmoid(float *array, int size);
    // bool        equal(float a, float b, float tolerance);
    // bool        equal(float *a, float *b, int size, float tolerance);
    // bool        equal(float *a, float b, int size, float tolerance);
    // bool        equal(float **a, float **b, int size_x, int size_y, float tolerance);
    float *
    tanh(float *array, int size)
    {
        for (int i=0; i<size; i++) {
            array[i] = tanhf(array[i]);
        }
        return array;
    }

    float*
    tanh(float *r, float*a, int size)
    {
        copy_array(r, a, size);
        return tanh(r, size);
    }
    
    float **
    tanh(float ** matrix, int sizex, int sizey)
    {
        for (int i=0; i < sizey; i++) {
            tanh(matrix[i], sizex);
        }
        return matrix;
    }

    float **
    tanh(float **r, float **a, int sizex, int sizey)
    {
        copy_matrix(r, a, sizex, sizey);
        return tanh(r, sizex, sizey);
    }

    float *     
    atanh(float *array, int size)
    {
        for (int i = 0; i < size; ++i)
            array[i] = atanhf(array[i]);
        return array;
    }

    float
    sigmoidf(float a)
    {
        return 1.f/(1+exp(-a));
    }

    float *
    sigmoid(float *array, int size)
    {
        for (int i = 0; i < size; ++i)
            array[i] = sigmoidf(array[i]);
        return array;
    }

    bool 
    equal(float a, float b, float tolerance)
    { 
        return ::abs((float)a-b) <= tolerance;
    }
    
    bool 
     equal(float *a, float *b, int size, float tolerance)
     {
         bool retval = true;
        //for (int i = size; i-- && retval;)
        for (int i = 0; i < size && retval; ++i)
        {
            retval = equal(a[i], b[i], tolerance);
              if(!retval) break;
        }
        return retval;
     }

     bool
     equal(float *a, float b, int size, float tolerance)
     {
        bool retval = true;
        for (int i = 0; i < size; ++i)
        {
            retval = equal(a[i], b, tolerance);
            if(!retval) break;
        }
        return retval;
     }

     bool 
     equal(float **a, float **b, int size_x, int size_y, float tolerance)
     {
        bool retval = true;
        for (int i = 0; i < size_y; ++i)
        {
            retval = equal(a[i], b[i], size_x, tolerance);
            if(!retval) break;
        }
        return retval;
     }
    
	//  void		map(float *r, float *i, float lo_src, float hi_src, float lo_trg, float hi_trg, int size);
    void map(float *r, float *a, float lo_src, float hi_src, float lo_trg, float hi_trg, int size)
    {
        for(int i=0; i < size; ++i)
        r[i] = lo_trg + 
            (hi_trg - lo_trg) * (a[i] - lo_src) 
            / (hi_src - lo_src);
    }

    // returns array containing only values >= threshold
    float *		
    threshold_gteq(float *r, float *a, float threshold, float size)
    {
        
        for(int i = 0; i < size; i++)
        {
            if(a[i] >= threshold)
                r[i] = a[i];
            else
                r[i] = 0.f;
        }
		return r;

    }

    // returns array containing only values > threshold
    float *		
    threshold_gt(float *r, float *a, float threshold, float size)
    {
        
        for(int i = 0; i < size; i++)
        {
            if(a[i] > threshold)
                r[i] = a[i];
            else
                r[i] = 0.f;
        }
		return r;

    }
    // returns array containing only values < threshold
    float *		
    threshold_lt(float *r, float *a, float threshold, float size)
    {
        
        for(int i = 0; i < size; i++)
        {
            if(a[i] < threshold)
                r[i] = a[i];
            else
                r[i] = 0.f;
        }
		return r;

    }





    vector
    closest_point_on_line_segment(vector & A, vector & B, vector & P)
    {

        float dx = B[0]-A[0];
        float dy = B[1]-A[1];
            
        float det = dx*dx + dy*dy;
        
        float k = (dy*(P[1]-A[1])+dx*(P[0]-A[0]))/det;
        
        float x0 = A[0]+k*dx;
        float x1 = A[1]+k*dy;
        
        if(x0<A[0] || x1<A[1])
            return A;
        else if(x0>B[0] || x1>B[1])
            return B;
        else 
        {
            vector x(2);
            x[0] = x0;
            x[1] = x1;
            return x;
        }
    }

}

