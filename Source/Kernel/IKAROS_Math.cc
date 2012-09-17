//
//	IKAROS_Math.cc		Various math functions for IKAROS
//
//    Copyright (C) 2006-2011  Christian Balkenius
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

#include <math.h>
#include <stdlib.h>

//#include <malloc/malloc.h> // FIXME: Remove

// includes for image processing (JPEG)

#include <stdio.h>
extern "C"
{
#include "jpeglib.h"
#include <setjmp.h>
}




#ifdef MAC_OS_X
#include <Accelerate/Accelerate.h>
#endif

#ifdef LINUX
#define	MAXFLOAT	3.402823466e+38f
#ifdef USE_BLAS
extern "C"
{
#include "gsl/gsl_blas.h"
}
#endif
#endif

#ifdef WINDOWS
#define	MAXFLOAT	3.402823466e+38f
#ifdef USE_BLAS
extern "C"
{
#include "gsl/gsl_blas.h"
}
#endif
#endif

#ifdef WINDOWS32
#define M_PI 3.14159265358979323846264338328f
#undef min
#undef max
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
    
	float trunc(float x)
	{
#ifdef WINDOWS32
		return (float)((int)x);
#else
		return ::truncf((int)x);
#endif
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
	float	atan2(float x, float y)
	{
		return ::atan2f(x, y);
	}
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
				r[j][i] = gaussian(hypot(center_x-float(i), center_y-float(j)), sigma);
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
				r[j][i] = gaussian1(hypot(center_x-float(i), center_y-float(j)), sigma);
		return r;
	}
	
	
    // MARK: -
    // MARK: random
    
	// random
	float
	random(float low, float high)
	{
#ifdef WINDOWS
		return low + (float(::rand())/float(RAND_MAX))*(high-low);
#else
		return low + (float(::random())/float(RAND_MAX))*(high-low);
#endif
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
#ifdef WINDOWS
		return int(::rand() % high);
#else
		return int(::random() % high);
#endif
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
#ifdef WINDOWS32
		return (float)_hypot(x, y);
#else
		return ::hypotf(x, y);
#endif
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
	add(float * a, int size)	// sum a
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
	add(float ** a, int sizex, int sizey)	// sum a
	{
		return add(*a, sizex*sizey);
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
    
    // MARK: -
    // MARK: multiply and divide
    
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
		for (int j=0; j<sizey; j++)
			for (int i=0; i<sizex; i++)
				r[j][i] = a[i] * b[j];
		return r;
	}
	
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
                if(abs(m[i][j]) > abs(m[p][j]))
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
                return false;
            
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
        
        delete t;
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
        
        delete t;
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
        if (d[size] == 0.0) singular=true;
        
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
        delete pivots;
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

        float ** a_t = transpose(create_matrix(m, n), a, m, n);
        
        // Query optimal working array(s) size
        
        sgesdd_(&jobz, &m, &n, *a_t, &m, s, *u, &m, *v, &n, &work_query, &lwork, iwork, &info);
        
        if(info < 0)
            info -= 1;
       
        if(info != 0)
            return -1;
        
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
     
        int i,j,k,l,l1,iter;
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
    
    
    // conversion
    // MARK: -
    // MARK: conversion
    
	int
	lround(float x)
	{
#ifdef WINDOWS32
		if ( x >= (int)x + 0.5f )
			return (int)x + 1;
		else
			return (int)x;
#else
		return int(::lroundf(x));
#endif
	}
	
	void
	float_to_byte(unsigned char * r, float * a, float min, float max, int size)
	{
#ifdef USE_VIMAGE
		struct vImage_Buffer src =
        {
            a, 1, size, size*sizeof(float)
        };
		struct vImage_Buffer dest =
        {
            r, 1, size, size*sizeof(float)
        };
		vImage_Error err = vImageConvert_PlanarFtoPlanar8 (&src, &dest, max, min, 0);
		if (err < 0) printf("IKAROS_Math:float_to_byte vImage_Error = %ld\n", err);
#else
		for (int i=0; i<size; i++)
			if (a[i] < min)
				r[i] = 0;
			else if (a[i] > max)
				r[i] = 255;
			else
				r[i] = int(255.0*(a[i]-min)/(max-min));
#endif
	}
	
	void
	byte_to_float(float * r, unsigned char * a, float min, float max, int size)
	{
		for (int i=0; i<size; i++)
			r[i] = ((max-min)/255.0) * float(a[i]) + min;
	}
	
	int
	string_to_int(const char * s, int d)
	{
		if (s == NULL)
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
#ifdef WINDOWS32
			return (float)strtod(s, NULL);
#else
        return strtof(s, NULL);
#endif
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
    
    
    
    // image processing
    // MARK: -
    // MARK: image processing
    
	float **
	convolve(float ** result, float ** source, float ** kernel, int rsizex, int rsizey, int ksizex, int ksizey, float bias)
	{
#ifdef USE_VIMAGE
		if (ksizex % 2 == 1 && ksizey % 2 == 1)
		{
			struct vImage_Buffer src =
            {
                *source, rsizey+(ksizey-1), rsizex+(ksizex-1), sizeof(float)*(rsizex+(ksizex-1))
            };
			struct vImage_Buffer dest =
            {
                *result, rsizey, rsizex, sizeof(float)*rsizex
            };
			vImage_Error err;
			if (bias != 0)
				err = vImageConvolveWithBias_PlanarF(&src, &dest, NULL, (ksizex-1)/2, (ksizey-1)/2, *kernel, ksizex, ksizey, bias, 0, kvImageTruncateKernel);
			else
				err = vImageConvolve_PlanarF(&src, &dest, NULL, (ksizex-1)/2, (ksizey-1)/2, *kernel, ksizex, ksizey, 0, kvImageTruncateKernel);
			if (err < 0) printf("IKAROS_Math:convolve vImage_Error = %ld\n", err);
		}
		else
		{
			// If kernel sizes are not odd use scalar version instead
			for (int j=0; j<rsizey; j++)
				for (int i=0; i<rsizex; i++)
				{
					float s = 0.0;
					for (int l=0; l<ksizey; l++)
					{
						float * src = &source[j+l][i];
						float * krn = kernel[l];
						for (int k=0; k<ksizex; k++)
							s += krn[k] * (*src++);
					}
					result[j][i] = s + bias;
				}
		}
#else
		for (int j=0; j<rsizey; j++)
			for (int i=0; i<rsizex; i++)
			{
				float s = 0.0;
				for (int l=0; l<ksizey; l++)
				{
					float * src = &source[j+l][i];
					float * krn = kernel[l];
					for (int k=0; k<ksizex; k++)
						s += krn[k] * (*src++);
				}
				result[j][i] = s + bias;
			}
#endif
		return result;
	}
	
	/*
	 Reasonably fast implementation of the box filter that sums all the
	 matrix elements within each box of size [boxsize] x [boxsize].
	 r - matrix with the sums; [sizex] x [sizey]
	 a - source matrix; [sizex + boxsize - 1] x [sizey + boxsize - 1]
	 t - temporary storage; size [sizex] x [sizey + boxsize - 1]; allocated and deallocated if t == NULL
	 */
    // TODO: Use intergal image instead
	float **
	box_filter(float ** r, float ** a, int sizex, int sizey, int boxsize, bool scale, float ** t)
	{
		bool owns_temp = false;
		if (t == NULL)
		{
			t = create_matrix(sizex, sizey+boxsize-1);	// sizex-boxsize+1
			owns_temp = true;
		}
		for (int j=0; j<sizey+boxsize-1; j++)
		{
			t[j][0] = 0;
			for (int i=0; i < boxsize; i++)
				t[j][0] += a[j][i];
		}
		for (int j=0; j<sizey+boxsize-1; j++)
		{
			for (int i=1; i < sizex; i++)
				t[j][i] = t[j][i-1] - a[j][i-1] + a[j][i+boxsize-1];
		}
		for (int i=0; i<sizex; i++)
		{
			r[0][i] = 0;
			for (int j=0; j < boxsize; j++)
				r[0][i] += t[j][i];
		}
		for (int j=1; j < sizey; j++)
		{
			float * sj0 = r[j];
			float * sj1 = r[j-1];
			float * tj1 = t[j-1];
			float * tjb = t[j+boxsize-1];
			for (int i=0; i<sizex; i++)
				*sj0++ = (*sj1++) - (*tj1++) + (*tjb++);
		}
		if (owns_temp)
			destroy_matrix(t);
		if (scale)
			multiply(r, 1.0/sqr(boxsize), sizex, sizey);
		return r;
	}
	
	// mics functions
	
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
    
    
    // image file formats
    // MARK: -
    // MARK: image file formats
    //
    // these functions will be moved to a separate file in the future
    //
    
    // create jpeg functions
    
    struct jpeg_destination { 
        struct jpeg_destination_mgr dest_mgr; 
        JOCTET * buffer; 
        size_t   size; 
        size_t   used;
    }; 
    
    
    static void 
    dst_init(j_compress_ptr cinfo) 
    { 
        struct jpeg_destination *dst = (jpeg_destination *)cinfo->dest; 
        dst->used = 0; 
        dst->size = 2048+cinfo->image_width * cinfo->image_height * cinfo->input_components * 2; // arbitrary initial size ; 2048 for header in case of very small images
        dst->buffer = (JOCTET *)malloc(dst->size * sizeof *dst->buffer);
        dst->dest_mgr.next_output_byte = dst->buffer; 
        dst->dest_mgr.free_in_buffer = dst->size; 
        return; 
    }
    
    
    // FIXME:   Something goes wrong when dst_empty is called more than once
    //          Fortunately it never happens since the size set by dst_init is large enough
    
    static boolean
    dst_empty(j_compress_ptr cinfo) 
    { 
        struct jpeg_destination *dst = (jpeg_destination *)cinfo->dest; 
        dst->used = dst->size - dst->dest_mgr.free_in_buffer;
        dst->size *= 2; 
        dst->buffer = (JOCTET *)realloc(dst->buffer, dst->size * sizeof *dst->buffer);
        dst->dest_mgr.next_output_byte = &dst->buffer[dst->used];
        dst->dest_mgr.free_in_buffer = dst->size - dst->used; 
        return true; 
    }
    
    
    static void
    dst_term(j_compress_ptr cinfo) 
    { 
        struct jpeg_destination *dst = (jpeg_destination *)cinfo->dest; 
        dst->used = dst->size - dst->dest_mgr.free_in_buffer;
        return; 
    } 
    
    
    static void
    jpeg_set_destination(j_compress_ptr cinfo, struct jpeg_destination *dst) 
    { 
        dst->dest_mgr.init_destination = dst_init; 
        dst->dest_mgr.empty_output_buffer = dst_empty; 
        dst->dest_mgr.term_destination = dst_term; 
        cinfo->dest = (jpeg_destination_mgr *)dst; 
        return; 
    } 
    
    
    
    char *
    create_jpeg(long int & size, float ** matrix, int sizex, int sizey, float minimum, float maximum)
    {
        if (matrix == NULL)
        {
            size = 0;
            return NULL;
        }
        
        JSAMPLE *   image_buffer = new JSAMPLE [sizex];
        JSAMPROW    row_pointer[1];
        
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr       jerr;
        struct jpeg_destination     dst;
        
        //int    row_stride;				// physical row width in image buffer 
        
        cinfo.err = jpeg_std_error(&jerr);
        
        jpeg_create_compress(&cinfo);	// Replace with ikaros error handler later
        
        cinfo.image_width = sizex; 	//* image width and height, in pixels
        cinfo.image_height = sizey;
        cinfo.input_components = 1;	// # of color components per pixel
        cinfo.in_color_space = JCS_GRAYSCALE;
        
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, 100, true);	// Highest quality
        jpeg_set_destination(&cinfo, &dst);
        
        // Do the compression
        
        jpeg_start_compress(&cinfo, true);
        
        //row_stride = sizex * 1;	// JSAMPLEs per row in image_buffer
        int j=0;
        
        while (cinfo.next_scanline < cinfo.image_height)
        {
            // Convert row to image buffer (assume max == 1 for now)
            
            JSAMPLE * ib = image_buffer;
            if (maximum != minimum)
                float_to_byte(image_buffer, matrix[j], minimum, maximum, sizex);
            else
                for (int i=0; i<sizex; i++)
                    *ib++ = 0;
            
            // Write to compressor
            row_pointer[0] = image_buffer;
            (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            j++;
        }
        
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        
        delete [] image_buffer;
        
        size = dst.used;
        return (char *)dst.buffer;
    }
    
    
    
    char *
    create_jpeg(long int & size, float ** matrix, int sizex, int sizey, int color_table[256][3])
    {
        if (matrix == NULL)
        {
            size = 0;
            return NULL;
        }
        
        JSAMPLE *   image_buffer = new JSAMPLE [3*sizex];
        JSAMPROW    row_pointer[1];
        
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr       jerr;
        struct jpeg_destination     dst;
        
        //int    row_stride;				// physical row width in image buffer 
        
        cinfo.err = jpeg_std_error(&jerr);
        
        jpeg_create_compress(&cinfo);	// Replace with ikaros error handler later
        
        cinfo.image_width = sizex; 	//* image width and height, in pixels
        cinfo.image_height = sizey;
        cinfo.input_components = 3;	// # of color components per pixel
        cinfo.in_color_space = JCS_RGB;
        
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, 100, true);	// Highest quality
        jpeg_set_destination(&cinfo, &dst);
        
        // Do the compression
        
        jpeg_start_compress(&cinfo, true);
        
        //row_stride = sizex * 3;		/* JSAMPLEs per row in image_buffer */
        int j=0;
        
        unsigned char * z = new unsigned char [sizex];
        while (cinfo.next_scanline < cinfo.image_height)
        {
            float_to_byte(z, matrix[j], 0.0, 1.0, sizex);	// TODO: allow specification of min and max later
            int x = 0;
            unsigned char * zz = z;
            
            for (int i=0; i<sizex; i++)
            {
                image_buffer[x++] = color_table[*zz][0];
                image_buffer[x++] = color_table[*zz][1];
                image_buffer[x++] = color_table[*zz][2];
                zz++;
            }
            
            // Write to compressor
            row_pointer[0] = image_buffer;
            (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            j++;
        }
        
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        
        delete [] image_buffer;
        
        size = dst.used;
        return (char *)dst.buffer;
    }
    
    
    
    char *
    create_jpeg(long int & size, float ** r, float ** g, float ** b, int sizex, int sizey)
    {
        size = 0;
        if (r ==NULL) return NULL;
        if (g ==NULL) return NULL;
        if (b ==NULL) return NULL;
        
        JSAMPLE *   image_buffer = new JSAMPLE [3*sizex];
        JSAMPROW    row_pointer[1];
        
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr       jerr;
        struct jpeg_destination     dst;
        
        //int    row_stride;				// physical row width in image buffer 
        
        cinfo.err = jpeg_std_error(&jerr);
        
        jpeg_create_compress(&cinfo);	// Replace with ikaros error handler later
        
        cinfo.image_width = sizex; 	//* image width and height, in pixels
        cinfo.image_height = sizey;
        cinfo.input_components = 3;	// # of color components per pixel
        cinfo.in_color_space = JCS_RGB;
        
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, 100, true);	// Highest quality
        jpeg_set_destination(&cinfo, &dst);
        
        // Do the compression
        
        jpeg_start_compress(&cinfo, true);
        
        //row_stride = sizex * 3;		/* JSAMPLEs per row in image_buffer */
        int j=0;
        
        while (cinfo.next_scanline < cinfo.image_height)
        {
            int x = 0;
            for (int i=0; i<sizex; i++)
            {
                // TODO: use clip function instead
                float rr = (r[j][i] < 0.0 ? 0.0 : (r[j][i] > 1.0 ? 1.0 : r[j][i])); 
                float gg = (g[j][i] < 0.0 ? 0.0 : (g[j][i] > 1.0 ? 1.0 : g[j][i]));
                float bb = (b[j][i] < 0.0 ? 0.0 : (b[j][i] > 1.0 ? 1.0 : b[j][i]));
                
                // clipping |= (rr != r[j][i]) || (gg != g[j][i]) || (bb != b[j][i]);
                
                image_buffer[x++] = int(255.0*rr);
                image_buffer[x++] = int(255.0*gg);
                image_buffer[x++] = int(255.0*bb);
            }
            
            // Write to compressor
            row_pointer[0] = image_buffer;
            (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            j++;
        }
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        
        delete [] image_buffer;
        
        size = dst.used;
        return (char *)dst.buffer;
    }
    
    
    
    void
    destroy_jpeg(char * jpeg)
    {
        free(jpeg);
    }
    
    
    // decode jpeg functions
    
    typedef struct {
        struct jpeg_source_mgr pub;	// public fields
        JOCTET eoi_buffer[2];         // a place to put a dummy EOI
    } my_source_mgr;
    
    typedef my_source_mgr * my_src_ptr;
    
    
    static void
    init_source (j_decompress_ptr cinfo)
    {
    }
    
    
    static boolean
    fill_input_buffer (j_decompress_ptr cinfo)
    {
        my_src_ptr src = (my_src_ptr) cinfo->src;
        
        //      WARNMS(cinfo, JWRN_JPEG_EOF);
        
        // Create a fake EOI marker 
        src->eoi_buffer[0] = (JOCTET) 0xFF;
        src->eoi_buffer[1] = (JOCTET) JPEG_EOI;
        src->pub.next_input_byte = src->eoi_buffer;
        src->pub.bytes_in_buffer = 2;
        
        return TRUE;
    }
    
    
    static void
    skip_input_data (j_decompress_ptr cinfo, long num_bytes)
    {
        my_src_ptr src = (my_src_ptr) cinfo->src;
        
        if (num_bytes > 0) {
            while (num_bytes > (long) src->pub.bytes_in_buffer) {
                num_bytes -= (long) src->pub.bytes_in_buffer;
                (void) fill_input_buffer(cinfo);
                // note we assume that fill_input_buffer will never return FALSE, so suspension need not be handled.
            }
            src->pub.next_input_byte += (size_t) num_bytes;
            src->pub.bytes_in_buffer -= (size_t) num_bytes;
        }
    }
    
    
    static void
    term_source (j_decompress_ptr cinfo)
    {
    }
    
    
    static void
    jpeg_memory_src (j_decompress_ptr cinfo, const JOCTET * buffer, size_t bufsize)
    {
        my_src_ptr src;
        
        if (cinfo->src == NULL) // first time for this JPEG object?
        {	
            cinfo->src = (struct jpeg_source_mgr *)
            (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(my_source_mgr));
        }
        
        src = (my_src_ptr) cinfo->src;
        src->pub.init_source = init_source;
        src->pub.fill_input_buffer = fill_input_buffer;
        src->pub.skip_input_data = skip_input_data;
        src->pub.resync_to_restart = jpeg_resync_to_restart; // use default method 
        src->pub.term_source = term_source;
        
        src->pub.next_input_byte = buffer;
        src->pub.bytes_in_buffer = bufsize;
    }
    
    
    struct my_error_mgr
    {
        struct jpeg_error_mgr pub;	// "public" fields 
        jmp_buf setjmp_buffer;		// for return to caller
    };
    
    
    typedef struct my_error_mgr * my_error_ptr;
    
    
    static void
    my_error_exit (j_common_ptr cinfo)
    {
        my_error_ptr myerr = (my_error_ptr) cinfo->err;
        (*cinfo->err->output_message) (cinfo);
        longjmp(myerr->setjmp_buffer, 1);
    }
    
    
    bool
    jpeg_get_info(int & sizex, int & sizey, int & planes, char * data, long int size)
    {
        struct jpeg_decompress_struct cinfo;
        struct my_error_mgr jerr;
        
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = my_error_exit;
        
        if (setjmp(jerr.setjmp_buffer))
        {
            jpeg_destroy_decompress(&cinfo);
            sizex = 0;
            sizey = 0;
            planes = 0;
            return false;
        }
        
        jpeg_create_decompress(&cinfo);
        jpeg_memory_src(&cinfo, (JOCTET *)data, size);
        (void) jpeg_read_header(&cinfo, TRUE);
        (void) jpeg_start_decompress(&cinfo);
        
        sizex  = cinfo.output_width;
        sizey  = cinfo.output_height;
        planes = cinfo.output_components;
        return true;
    }
    
    
    void
    jpeg_decode(float ** matrix, int sizex, int sizey, char * data, long int size)
    {
        
    }
    
    
    void
    jpeg_decode(float ** red_matrix, float ** green_matrix, float ** blue_matrix, float ** intensity_matrix, int sizex, int sizey, char * data, long int size)
    {
        struct jpeg_decompress_struct cinfo;
        struct my_error_mgr jerr;
        FILE * infile = NULL;				/* source file */
        JSAMPARRAY buffer;			/* Output row buffer */
        int row_stride;				/* physical row width in output buffer */
        
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = my_error_exit;
        
        if (setjmp(jerr.setjmp_buffer))
        {
            jpeg_destroy_decompress(&cinfo);
            fclose(infile);
            return;
        }
        
        jpeg_create_decompress(&cinfo);
        jpeg_memory_src(&cinfo, (JOCTET *)data, size);
        
        (void) jpeg_read_header(&cinfo, TRUE);
        (void) jpeg_start_decompress(&cinfo);
        row_stride = cinfo.output_width * cinfo.output_components;
        
        if(cinfo.output_components != 3)
        {
            printf("IKAROS_Math:jpeg_decode: ERROR Not a colot image.\n");
            return;
        }
        
        if (cinfo.output_width != (unsigned int)(sizex) ||  cinfo.output_height != (unsigned int)(sizey))
        {
            printf("IKAROS_Math:jpeg_decode: ERROR Incorresct size for destination.\n");
            return;
        }
        
        buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
        
        // RGB Color Image
        
        float c255 = 1.0/255.0;
        float c3 = 1.0/3.0;
        while (cinfo.output_scanline < cinfo.output_height)
        {
            (void) jpeg_read_scanlines(&cinfo, buffer, 1);
            unsigned char * buf = buffer[0];
            int ix = (cinfo.output_scanline-1);
            float * r = red_matrix[ix];
            float * g = green_matrix[ix];
            float * b = blue_matrix[ix];
            float * iy = intensity_matrix[ix];
            for (int i=0; i<sizex; i++) // FIXME: CHECK BOUNDS
            {
                *r		= c255*float(*buf++);
                *g		= c255*float(*buf++);
                *b		= c255*float(*buf++);
                *iy++	= c3*((*r++)+(*g++)+(*b++));	// FIXME: Do this correctly later!!!
            }
        }
        
        (void) jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
    }
    
    
    
    // drawing functions
    // MARK: -
    // MARK: drawing functions
    
    // Simple minded implemetation of the Bresenham algorithm
    
    static inline void
    swap(int & x, int & y)
    {
        int t = x;
        x = y;
        y = t;
    }
    
    
    static inline void
    plot(float ** image, int sizex, int sizey, int x, int y, float color)
    {
        if(x<0) return;
        if(y<0) return;
        if(x>=sizex) return;
        if(y>=sizey) return;
        image[y][x] = color;
    }
    
    void
    draw_line(float ** image, int sizex, int sizey, int x0, int y0, int x1, int y1, float color)
    {
        bool steep = abs(y1 - y0) > abs(x1 - x0);
        if(steep)
        {
            swap(x0, y0);
            swap(x1, y1);
        }
        
        if(x0 > x1)
        {
            swap(x0, x1);
            swap(y0, y1);
        }
        
        int deltax = x1 - x0;
        int deltay = abs(y1 - y0);
        float error = 0;
        float deltaerr = float(deltay) / float(deltax);
        int ystep;
        int y = y0;
        
        if(y0 < y1)
            ystep = 1;
        else
            ystep = -1;
        
        for(int x=x0; x<=x1; x++)
        {
            if(steep)
                plot(image, sizex, sizey, x, y, color);
            else
                plot(image, sizex, sizey, y, x, color);
            
            error = error + deltaerr;
            
            if(error >= 0.5)
            {
                y = y + ystep;
                error = error - 1.0;
            }
        }
    }
    
    
    void
    draw_line(float ** red_image, float ** green_image, float ** blue_image, int sizex, int sizey, int x0, int y0, int x1, int y1, float red, float green, float blue)
    {
        draw_line(red_image, sizex, sizey, x0, y0, x1, y1, red);
        draw_line(green_image, sizex, sizey, x0, y0, x1, y1, green);
        draw_line(blue_image, sizex, sizey, x0, y0, x1, y1, blue);
    }
    
    
    void
    draw_circle(float ** image, int sizex, int sizey, int x0, int y0, int radius, float color)
    {
        int f = 1 - radius;
        int ddF_x = 1;
        int ddF_y = -2 * radius;
        int x = 0;
        int y = radius;
        
        plot(image, sizex, sizey, x0, y0 + radius, color);
        plot(image, sizex, sizey, x0, y0 - radius, color);
        plot(image, sizex, sizey, x0 + radius, y0, color);
        plot(image, sizex, sizey, x0 - radius, y0, color);
        
        while(x < y)
        {
            if(f >= 0) 
            {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;    
            plot(image, sizex, sizey, x0 + x, y0 + y, color);
            plot(image, sizex, sizey, x0 - x, y0 + y, color);
            plot(image, sizex, sizey, x0 + x, y0 - y, color);
            plot(image, sizex, sizey, x0 - x, y0 - y, color);
            plot(image, sizex, sizey, x0 + y, y0 + x, color);
            plot(image, sizex, sizey, x0 - y, y0 + x, color);
            plot(image, sizex, sizey, x0 + y, y0 - x, color);
            plot(image, sizex, sizey, x0 - y, y0 - x, color);
        }
    }
    
    
    void
    draw_circle(float ** red, float ** green, float ** blue, int sizex, int sizey, int x, int y, int radius, float r, float g, float b)
    {
        draw_circle(red, sizex, sizey, x, y, radius, r);
        draw_circle(green, sizex, sizey, x, y, radius, g);
        draw_circle(blue, sizex, sizey, x, y, radius, b);
    }
    
}

