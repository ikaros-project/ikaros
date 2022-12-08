//
//	IKAROS_Math.h		Various math functions for IKAROS
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

#ifndef IKAROS_MATH
#define IKAROS_MATH

// the first variable is often the result array or matrix
// order of sizes is ALWAYS x before y in function calls
// the sizes nearly always goes last of the arguments
// unless there are optional arguments

#include <string>
#include <cmath>

#include "IKAROS_Utils.h"

namespace ikaros
{	
	extern const float	pi;
	extern const float	sqrt2pi;
	extern const float	maxfloat;
    
    enum angle_unit {degrees, radians, tau};
	
	float eps(float x);
    
	// misc scalar
/*
	float	exp(float x);
	float	pow(float x, float y);
	
	float	log(float x);
	float	log10(float x);
	
	
    float	sin(float x);
	float	cos(float x);
	float	tan(float x);
	float	asin(float x);
	float	acos(float x);
	float	atan(float x);
*/

	
//	float		atan2(float y, float x);
	float *		atan2(float * r, float * a, float * b, int size);
	float **	atan2(float ** r, float ** a, float ** b, int sizex, int sizey);
    
	float       mod(float x, float y);
	
	float		gaussian(float x, float sigma);
    
    float *     gaussian(float * r, float * a, float sigma, int size);
    float **    gaussian(float ** r, float ** m, float sigma, int sizex, int sizey);
    
    
	float *		gaussian(float * r, float sigma, int size);
	float *		gaussian(float * r, float x, float sigma, int size);
	float **	gaussian(float ** r, float sigma, int sizex, int sizey);
	float **	gaussian(float ** r, float x, float y, float sigma, int sizex, int sizey);
	
	float		gaussian1(float x, float sigma);
    
    float *     gaussian1(float * r, float * a, float sigma, int size);
    float **    gaussian1(float ** r, float ** m, float sigma, int sizex, int sizey);
    
	float *		gaussian1(float * r, float sigma, int size);
	float *		gaussian1(float * r, float x, float sigma, int size);
	float **	gaussian1(float ** r, float sigma, int sizex, int sizey);
	float **	gaussian1(float ** r, float x, float y, float sigma, int sizex, int sizey);
	
	float		random(float low, float high);
	float *		random(float * r, float low, float high, int size);
	float **	random(float ** r, float low, float high, int sizex, int sizey);
    
	int         random(int high);   // 0 <= int < high
	int			random_int(int low, int high);
	int *		random_int(int * r, int low, int high, int size);
	int *		random_unique(int *r, int low, int high, int size);
    
	float		gaussian_noise(float mean, float sigma);

	float 		binomial_trial(float prob); // 1 if random() < prob
	float 		binomial_sample(int n, float prob); // counts successes
	int 		multinomial_trial(float *prob, int size); // return ix of high
	float *		multinomial_sample(int n, float *prob, int size); // return ix of high
	
	bool		zero(float * a, int size);
	bool		zero(float ** a, int sizex, int sizey);
	bool		non_zero(float * a, int size);
	bool		non_zero(float ** a, int sizex, int sizey);
	
    int         sgn(int x);
    float       sgn(float x);
	float *		sgn(float * a, int size);
	float **	sgn(float ** r, int sizex, int sizey);
    float *     sgn(float * r, float * a, int size);
    float **    sgn(float ** r, float ** m, int sizex, int sizey);
    
	int			sqr(int x);
	float		sqr(float x);
	float *		sqr(float * a, int size);
	float **	sqr(float ** r, int sizex, int sizey);
	float *		sqr(float * r, float * a, int size);
	float **	sqr(float ** r, float ** a, int sizex, int sizey);
	
	
	float		sqrt(int x);
//	float		sqrt(float x);
	float *		sqrt(float * a, int size);
	float **	sqrt(float ** m, int sizex, int sizey);
	float *		sqrt(float * r, float * a, int size);
	float **	sqrt(float ** r, float ** a, int sizex, int sizey);
	
	
//	float		hypot(float x, float y);
	float *		hypot(float * r, float * a, float * b, int size);
	float **	hypot(float ** r, float ** a, float ** b, int sizex, int sizey);
	
	
	float		dist(float * a, float * b, int size);
	float		dist(float ** a, float ** b, int sizex, int sizey);
	
	float		norm(float * a, int size);
	float		norm(float ** a, int sizex, int sizey);
	
	float		dist1(float * a, float * b, int size);
	float		dist1(float ** a, float ** b, int sizex, int sizey);
	
	float		norm1(float * a, int size);
	float		norm1(float ** a, int sizex, int sizey);
	
	float *     normalize(float * a, int size);
	float **    normalize(float ** m, int sizex, int sizey);
	
	float *     normalize1(float * a, int size);
	float **    normalize1(float ** m, int sizex, int sizey);
	
	float *     normalize_max(float * a, int size);
	float **    normalize_max(float ** m, int sizex, int sizey);
	
//	int			abs(int x);
//	float		abs(float x);
	float *		abs(float * a, int size);
	float **	abs(float ** m, int sizex, int sizey);
	float *		abs(float * r, float * a, int size);
	float **	abs(float ** r, float ** m, int sizex, int sizey);


	int			min(int x, int y);
	float		min(float x, float y);
	float		min(float * a, int size);
	float		min(float ** a, int sizex, int sizey);
	float *		min(float * r, float * a, int size);
	float **	min(float ** r, float ** a, int sizex, int sizey);
	float *		min(float * r, float * a, float * b, int size);
	float **	min(float ** r, float ** a, float ** b, int sizex, int sizey);
	
	int			arg_min(float * a, int size);
	void		arg_min(int & x, int & y, float ** a, int sizex, int sizey);
	
	
	int			max(int x, int y);
	float		max(float x, float y);
	float		max(float * a, int size);
	float		max(float ** a, int sizex, int sizey);
	float *		max(float * r, float * a, int size);
	float **	max(float ** r, float ** a, int sizex, int sizey);
	float *		max(float * r, float * a, float * b, int size);
	float **	max(float ** r, float ** a, float ** b, int sizex, int sizey);
	
	int			arg_max(float * a, int size);
	void		arg_max(int & x, int & y, float ** a, int sizex, int sizey);
	
    int         arg_max_col(float ** m, int select_col, int sizex, int sizey);  // arg_max along a selected column; returns y
    int         arg_max_row(float ** m, int select_row, int sizex, int sizey);  // arg_max along a selected row; returns y

	float *		minmax(float & min, float & max, float * a, int size);
	float **	minmax(float & min, float & max, float ** a, int sizex, int sizey);
	
	float		mean(float * a, int size);
	float		mean(float ** a, int sizex, int sizey);
    float *     mean(float * r, float ** m, int sizex, int sizey, int dim=0); // mean over columns (if dim==0) or rows (if dim==1); r must have size sizex or sizey depending om dim

	float		median(float * a, int size);
	float		median(float ** a, int sizex, int sizey);
	float *		median(float * r, float ** a, int sizex, int sizey, int dim=0); // median over columns (if dim==0) or rows (if dim==1); r must have size sizex or sizey depending om dim

	float		clip(float x, float low, float high);
	float *		clip(float * a, float low, float high, int size);
	float **	clip(float ** a, float low, float high, int sizex, int sizey);
	float *		clip(float * r, float * a, float low, float high, int size);
	float **	clip(float ** r, float ** a, float low, float high, int sizex, int sizey);
	
	
	float		sum(float * a, int size);	// sum a
	float		sum(float ** a, int sizex, int sizey);	// sum a
	float *	sum(float *r, float **a, int sizex, int sizey ); // by row
	float * sum(float *r, float **a, int sizex, int sizey, int axis); 
	
	float *		add(float * r, float alpha, int size);	// r = r + alpha
	float **	add(float ** r, float alpha, int sizex, int sizey);	// r = r + alpha
	
	float *		add(float * r, float * a, int size);	// r = r + a
	float **	add(float ** r, float ** a, int sizex, int sizey);	// r = r + a
	float *		add(float * r, float * a, float * b, int size);	// r = a + b
	float **	add(float ** r, float ** a, float ** b, int sizex, int sizey);	// r = a + b
	float *		add(float * r, float alpha, float * a, int size);	// r = r + alpha * a
	float **	add(float ** r, float alpha, float ** a, int sizex, int sizey);	// r = r + alpha * a
	float *		add(float * r, float alpha, float * a, float beta, float * b, int size);	// r = alpha * a + beta * b
	float **	add(float ** r, float alpha, float ** a, float beta, float ** b, int sizex, int sizey);	// r = alpha * a + beta * b
	float *		add(float * r, float alpha, float * a, float beta, int size);	// r = alpha * a + beta
	float **	add(float ** r, float alpha, float ** a, float beta, int sizex, int sizey);	// r = alpha * a + beta
	
	float *     subtract(float * r, float alpha, int size);	// r = r - alpha
	float **    subtract(float ** r, float alpha, int sizex, int sizey);	// r = r - alpha
	
	float *     subtract(float alpha, float * r, int size);	// r = alpha - r
	float **    subtract(float alpha, float ** r, int sizex, int sizey);	// r = alpha - r
	
	float *     subtract(float * r, float * a, float alpha, int size);	// r = a - alpha  *** undocumented ***
	float **    subtract(float ** r, float ** a, float alpha, int sizex, int sizey);	// r = a - alpha  *** undocumented ***
	
	
	float *		subtract(float * r, float * a, int size);
	float **	subtract(float ** r, float ** a, int sizex, int sizey);
	float *		subtract(float * r, float * a, float * b, int size);
	float **	subtract(float ** r, float ** a, float ** b, int sizex, int sizey);
	
    float **    subtract(float ** r, float ** m, float * a, int sizex, int sizey); // subtract array a from each row in m
    
    float       product(float * a, int size);
    float       product(float ** m, int sizex, int sizey);
    
	float *		multiply(float * a, float alpha, int size);
	float **	multiply(float ** a, float alpha, int sizex, int sizey);
	float *		multiply(float * r, float * a, float alpha, int size);
    float **    multiply(float ** r, float ** a, float alpha, int sizex, int sizey);
	float *		multiply(float * r, float * a, int size);
	float * 	multiply(float * r, float * a, float * b, float alpha, int size); 
	float **	multiply(float ** r, float ** a, int sizex, int sizey);
	float *		multiply(float * r, float * a, float * b, int size);
	float **	multiply(float ** r, float ** a, float ** b, int sizex, int sizey);
	
	float *		multiply(float * r, float ** a, float * b, int sizex, int sizey);	// matrix x vector
	float **	multiply(float ** r, float ** a, float ** b, int sizex, int sizey, int n);	// matrix x matrix; size of result, n = columns of a = rows of b
	
	float		dot(float * a, float * b, int size);
	float		dot(float ** a, float ** b, int sizex, int sizey);
	
	float **	outer(float ** r, float * a, float * b, int sizex, int sizey);		// outer product;  a*b'  sizey = sizeof(a), sizex = sizeof(b), (sizex,sizey) = sizeof(r)
	
	float *     divide(float * r, float * a, int size);
	float **	divide(float ** r, float ** a, int sizex, int sizey);
	float *     divide(float * r, float * a, float * b, int size);
	float **	divide(float ** r, float ** a, float ** b, int sizex, int sizey);
	
    // linear algebra
    
	float **    eye(float ** a, int size); // identity matrix
    float **    transpose(float ** a_T, float ** a, int sizex, int sizey);
    float **    transpose(float ** a, int size); // transpose square matrix in place
    float       det(float ** m, int size); // determinant of m
    float       trace(float ** m, int size); // trace of m
    float       rank(float ** m, int sizex, int sizey, float tol=0); // the rank of the matrix
    bool        inv(float ** a, float ** b, int size);  // invert matrix b and store in a; returns false if singular
    void        pinv(float ** a, float ** b, int sizex, int sizey); // pseudoinverse of b is stored in a; sizeof(a) = sizex x sizey; sizeof(b) = sizey x sizex

    float *     mldivide(float * r, float ** m, float * a, int size); // left divide; solves mr = a; square matrix; vector r and a
    float *     mldivide(float * r, float ** m, float * a, int sizex, int sizey); // left divide; solves mr = a; non square matrix; vector r and a
    float **    mldivide(float ** r, float ** m, float ** a, int sizex, int sizey, int n);  // left divide; solves mr = a for three matrices; exact or using least squares
    
    void        lu(float ** m, float ** a, int sizex, int sizey);  // LU-decomposition of a; L and U are stored in compact form similar to MATLAB function lu(b)
    void        lu(float ** l, float ** u, float ** a, int sizex, int sizey); // LU-decomposition of a; l and u are permuted like MATLAB function [L U] = lu(a)
    void        lu(float ** l, float ** u, float ** p, float ** a, int sizex, int sizey);  // LU-decomposition of a; p is a permutation matrix like MATLAB function [L U P] = lu(a)

    bool        qr(float ** q, float ** r, float ** a, int size); // QR-decomposition of a (square matrix), returns false if a is ingular
    float **    chol(float ** r, float ** a, int size, bool & posdef); // Cholesky decomposition of a symmetric positive definite matrix a; posdef = false if not positive definite
    float **    chol(float ** r, float ** a, int size);
    int         svd(float ** u, float * s, float ** v, float ** m, int sizex, int sizey); // Singular value decomposition: u*diag(s)*v' = m; m has size sizex x sizey
    float **    pca(float ** c, float * v, float ** m, int sizex, int sizey); // Principal component analysis; data in rows, c: coefficients/principal axes; v = variances; (map to new space using c*m)

    // homogenous 4x4 matrices represented as float[16]
    
    typedef float h_matrix[16];
    typedef float h_vector[4];
    
    enum axis { X, Y, Z };
    
    // set matrix data
    
    float *     h_reset(h_matrix r);
    float *     h_eye(h_matrix r);
    float *     h_rotation_matrix(h_matrix r, axis a, float alpha);
    float *     h_rotation_matrix(h_matrix r, float x, float y, float z);
    float *     h_translation_matrix(h_matrix r, float tx, float ty, float tz);
    float *     h_reflection_matrix(h_matrix r, axis a);
    float *     h_scaling_matrix(h_matrix r, float sx, float sy, float sz);
    
    // get matrix data
    
    void        h_get_translation(const h_matrix m, float & x, float & y, float &z);
    void        h_get_euler_angles(const h_matrix m, float & x, float & y, float &z);
    float       h_get_euler_angle(const h_matrix m, axis a);
	void 		h_get_scale(const h_matrix m, float &x, float &y, float &z);
    
    inline float h_get_x(h_matrix m) { return m[3]; }
    inline float h_get_y(h_matrix m) { return m[7]; }
    inline float h_get_z(h_matrix m) { return m[11]; }

    // operations
    
    float *     h_add(h_matrix r, h_matrix a); // r = r + a
    float *     h_add(h_matrix r, float alpha, h_matrix a, float beta, h_matrix b);

    float *     h_multiply(h_matrix r, h_matrix a, h_matrix b);  // matrix x matrix
    float *     h_multiply_v(h_vector r, h_matrix m, h_vector v);  // matrix x vector + scaling to make v[3] == 1.
    float *     h_multiply(h_matrix r, float c); // matrix x scalar
    float *     h_transpose(h_matrix r, h_matrix a);
    float *     h_inv(h_matrix r, h_matrix a);
    float *     h_normalize_rotation(h_vector m); // uses svd to orthogonalize the 3x3 rotation part of the matrix
    
    float       h_dist(h_matrix a, h_matrix b);

    // utilities
    
    float *     h_copy(h_matrix r, h_matrix m);
    float *     h_copy_v(h_vector r, h_vector a);
    float **    h_temp_matrix(h_matrix r, float * (&p)[4]); // return pointer to  matrix that last as long as temporary storage p is available (float * p[4])
    void        h_print_matrix(const char * name, h_matrix m, int decimals=2);

    float **    h_create_matrix(h_matrix m); // create an ordinary 4x4 matrix; data is copied
	float *     h_create_matrix(h_matrix &r, float *a);
    float **    h_set_matrix(float ** m, h_matrix h); // set top left 4x4 elements of a regular matrix from a h_matrix

    inline bool h_matrix_is_valid(h_matrix m) { return (m[15] > 0.1); } // Matrix is valid if bottom right element is 1 (but we do not want to make an exact match)

	// conversion
	
//	float		trunc(float x);
//	int         lround(float x);
	void		float_to_byte(unsigned char * r, float * a, float min, float max, long size);	// min, max of float; byte is always 0-255
	void		byte_to_float(float * r, unsigned char * a, float min, float max, long size);	// min, max of float; byte is always 0-255

	int			map_to_bin(float x, int bins, float range_low=0.0, float range_high=1.0); // Map float between range_low and range_hight (inclusive) to 0..bins-1

	int         string_to_int(const char * s, int d=0);  // convert to int; use d if char is NULL
	float		string_to_float(const char * s, float d=0.0); // convert to float; use d if char is NULL

    int         string_to_int(const std::string & s, int d = 0);
    long        string_to_long(const std::string & s, long d = 0);
    float       string_to_float(const std::string & s, float d = 0.0); // convert to float; use d if char is NULL
    bool        string_to_bool(const std::string & s, bool d = false);
    
	char *      int_to_string(char * s, int n, int i);	// put the string representation of the value i into s using a maximum of n characters
	char *      float_to_string(char * s, float v, int decimals, int n);

    float       angle_to_angle(float angle, int from_angle_unit, int to_angle_unit);
    float       short_angle(float a1, float a2); // in radians

	// misc

	int         select_boltzmann(float * a, int size, float T);
	void		select_boltzmann(int & x, int & y, float ** m, int sizex, int sizey, float T);

	void		ascend_gradient(int & x, int & y, float ** m, int sizex, int sizey);
	void		descend_gradient(int & x, int & y, float ** m, int sizex, int sizey);

    //
    // sorting
    //

    float *     sort(float * a, long size);
    float **    sort(float ** a, long sizex, long sizey);   // as if array



	// mark - TAT additions
	float *		soft_max(float *r, float * a, int size); // use exp - vuln to overflow
	float *		soft_max_pw(float *r, float *a, float pw, int size); // safer for overflow
	float **		multiply(float **r, float *a, float *b, int sizex, int sizey); //tat
	float ** multiply(float **r, float *a, float **b, int sizex, int sizey); // tat
	float **	multiply_t(float ** r, float ** a, float ** b, int sizex, int sizey, int n); // matrix x  matrix' tat
	float **	multiply(float ** r, float alpha, float ** a, float ** b, int sizex, int sizey, int n);	// alpha*matrix x matrix; size of result, n = columns of a = rows of b

    // various - TAT
    float *		tanh(float *array, int size);
	float *		tanh(float *r, float *a, int size);
    float **    tanh(float ** matrix, int sizex, int sizey);
	float **	tanh(float **r, float **a, int sizex, int sizey);
	float *		atanh(float *array, int size);
	float		sigmoidf(float a);
	float *		sigmoid(float *array, int size);
    bool        equal(float a, float b, float tolerance);
    bool        equal(float *a, float *b, int size, float tolerance);
    bool        equal(float *a, float b, int size, float tolerance);
    bool        equal(float **a, float **b, int size_x, int size_y, float tolerance);
	void		map(float *r, float *i, float lo_src, float hi_src, float lo_trg, float hi_trg, int size);
	float *		threshold_gteq(float *r, float *a, float threshold, float size);
	float *		threshold_gt(float *r, float *a, float threshold, float size);
	float *		threshold_lt(float *r, float *a, float threshold, float size);


	vector	closest_point_on_line_segment(vector & A, vector & B, vector & P); // temporary version
}	

#endif
