/*
 *  EyeTracker.cc
 */

#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "EyeTracker.h"
#include "IKAROS_Utils.h"

#define TRUE	1
#define FALSE 	0

#define sqr(x)		((x)*(x))

/* maximum number of edge elements */

#define TSIZE 25000

/* Image matrices */

static float ** dx;
static float ** dy;
static float ** dGx;
static float ** dGy;
static float ** g;
static float ** edge;
static float ** acc;


static float edgeTableX[TSIZE];
static float edgeTableY[TSIZE];
static float edgeTableDX[TSIZE];
static float edgeTableDY[TSIZE];
static int	edgeCount = 0;


static int	w;
static int	h;
static int	w2;
static int	h2;
static int	fr;
static int	dns;
static float dmin;
static float dmax;
static float dmin2;
static float dmax2;



/*
	Initialize data structures and parameters   0.5, 5, 40, 5, 15, 2
*/

void
EyeTracker_Init(int image_width, int image_height, int filter_radius, float filter_scale, float dist_min, float dist_max, float dist_min2, float dist_max2, int density)
{
	int i, j;
	
	int filter_size	=	2 * filter_radius + 1;

	w		=	image_width;
	h		=	image_height;
	w2		=	image_width - 2*fr;
	h2		=	image_height- 2*fr;
	fr		=	filter_radius;
	dmax	=	dist_max;
	dmin		=	dist_min;
	dmax2	=	dist_max2;
	dmin2	=	dist_min2;
	dns		=	density;

	/* Init image matrices */
	
	dx	=	create_matrix(h2, w2);
	dy	=	create_matrix(h2, w2);
	dGx	=	create_matrix(filter_size, filter_size);
	dGy	=	create_matrix(filter_size, filter_size);
	g	=	create_matrix(h2, w2);
	edge	=	create_matrix(h2, w2);
	acc	=	create_matrix(h2, w2);

	/* Calculate filter parameters */
	
	for(j=0; j<filter_size; j++)
		for(i=0; i<filter_size; i++)
		{
			float x = filter_scale * (float)(i-filter_radius);
			float y = filter_scale * (float)(j-filter_radius);
			dGx[j][i] = -2*x*exp(-(x*x + y*y));
			dGy[j][i] = -2*y*exp(-(x*x + y*y));
		}
	
}



/*
	Enhance gray-level contrast
*/


void
enhance_contrast(int ** m)
{
	int i,j;
	
	for(j =0; j<h; j++)
		for(i=0; i<w; i++)
			m[j][i] = (m[j][i] > 128 ? 255 : 2*m[j][i]);
}



/*
	Find edges in the image
*/

void
calculate_edges(int ** input)
{
	int i,j,ii,jj;
	
	int inc = dns;	// Edge density
	
	edgeCount = 0;
	
	// Estimate gradient - dx
	
	for(j =fr; j<h-fr; j+=inc)
		for(i=fr; i<w-fr; i+=inc)
		{
			dx[j-fr][i-fr] = 0;
			for(jj=-fr; jj<=fr; jj++)
				for(ii=-fr; ii<=fr; ii++)
					dx[j-fr][i-fr] += dGx[jj+fr][ii+fr] * (float)(input[j+jj][i+ii]);
		}

	// Estimate gradient - dy
	
	for(j =fr; j<h-fr; j+=inc)
		for(i=fr; i<w-fr; i+=inc)
		{
			dy[j-fr][i-fr] = 0;
			for(jj=-fr; jj<=fr; jj++)
				for(ii=-fr; ii<=fr; ii++)
					dy[j-fr][i-fr] += dGy[jj+fr][ii+fr] * (float)(input[j+jj][i+ii]);
		}

	// Test each location for an edge
	
	reset_matrix(g, h2, w2);
	reset_matrix(edge, h2, w2);
	
	float dymax = 0;
	for(j =2; j<h-2; j++)
		for(i=2; i<w-2; i++)
			if(dy[j-2][i-2] > dymax)
				dymax = dy[j-2][i-2];

	float dxmax = 0;
	for(j =2; j<h-2; j++)
		for(i=2; i<w-2; i++)
			if(dx[j-2][i-2] > dxmax)
				dxmax = dx[j-2][i-2];

	float dT2 = 0.25 * dxmax * dymax;

	for(j =0; j<h2; j+=1)
		for(i=0; i<w2; i+=1)
			if((sqr(dx[j][i]) + sqr(dy[j][i])) > dT2 && edgeCount<TSIZE-1)
			{
				edge[j][i] = sqrt(dx[j][i]*dx[j][i] + dy[j][i]*dy[j][i]);
				edgeTableX[edgeCount] = (float)(i);
				edgeTableY[edgeCount] = (float)(j);
				edgeTableDX[edgeCount] = dx[j][i];
				edgeTableDY[edgeCount] = dy[j][i];
				edgeCount ++;
			}
			
//	printf("ET edgeCount = %d\n", edgeCount);fflush(NULL);

}



void
calculate_intersections(float *e_x, float * e_y)
{
	float estimated_x, estimated_y;
	int i,j;

	if(edgeCount < 2)
	{
		estimated_x = 0;
		estimated_y = 0;
		return;
	}
	
	// Test distance between points
	
	long intCnt = 0;
	for(i=0; i<edgeCount; i++)
		for(j=0; j<edgeCount; j++)
		{
			float dist = sqrt(sqr(edgeTableX[i] - edgeTableX[j]) + sqr(edgeTableY[i] - edgeTableY[j]));
			if(i != j && dist < dmax && dist > dmin)
			{
				// Calculate intersection point
				
				float x0 = edgeTableX[i];
				float y0 = edgeTableY[i];
				float a0 = edgeTableDX[i];
				float b0 = edgeTableDY[i];

				float x1 = edgeTableX[j];
				float y1 = edgeTableY[j];
				float a1 = edgeTableDX[j];
				float b1 = edgeTableDY[j];
				
				if(-a0*b1 + b0*a1 != 0)
				{
					float ix = x0 + (a0*(-y0*a1+y1*a1+b1*x0-b1*x1))/(-a0*b1 + b0*a1);
					float iy = y0 + (b0*(-y0*a1+y1*a1+b1*x0-b1*x1))/(-a0*b1 + b0*a1);
				
					// Reject intersection on the wrong side of edges
					
					int reject = FALSE;
					
					if(a0 < 0 && ix > x0)
						reject = TRUE;

					if(a0 > 0 && ix < x0)
						reject = TRUE;
					
					if(a1 < 0 && ix > x1)
						reject = TRUE;

					if(a1 > 0 && ix < x1)
						reject = TRUE;
					
					if(b0 < 0 && iy > y0)
						reject = TRUE;

					if(b0 > 0 && iy < y0)
						reject = TRUE;
					
					if(b1 < 0 && iy > y1)
						reject = TRUE;

					if(b1 > 0 && iy < y1)
						reject = TRUE;
					
					float tilesize = 10;
					if(!reject && 0 < ix && ix < w2-tilesize-1 && 0 < iy && iy < h2-tilesize-1)
					{
						int dx, dy;
						float increment = (a0*a0+b0*b0)*(a1*a1+b1*b1);
						for(dx=0; dx<tilesize; dx++)
							for(dy=0; dy<tilesize; dy++)
								g[(int)(iy+dy)][(int)(ix+dx)] += increment;

						intCnt++;
					}
				}
			}
		}
		
//	printf("ET int cnt = %ld\n", intCnt);fflush(NULL);

	// Find maximum
	
	float e = 0;
	for(j=0; j<h2; j++)
		for(i=0; i<w2; i++)
			if(g[j][i] > e)
			{
				e = g[j][i];
				estimated_x = i;
				estimated_y = j;
			}

	estimated_x -= 3;
	estimated_y -= 3;
	
	// Recalculate estimation
	
	float ex = 0;
	float ey = 0;
	float ww = 0;
	
	for(i=0; i<edgeCount; i++)
		for(j=0; j<edgeCount; j++)
		{
			float dist = sqrt(sqr(edgeTableX[i] - edgeTableX[j]) + sqr(edgeTableY[i] - edgeTableY[j]));
			if(i != j && 5 < dist && dist < 15) ////////
			{
				// Calculate intersection point
				
				float x0 = edgeTableX[i];
				float y0 = edgeTableY[i];
				float a0 = edgeTableDX[i];
				float b0 = edgeTableDY[i];

				float x1 = edgeTableX[j];
				float y1 = edgeTableY[j];
				float a1 = edgeTableDX[j];
				float b1 = edgeTableDY[j];
				
				if(-a0*b1 + b0*a1 != 0)
				{
					float ix = x0 + (a0*(-y0*a1+y1*a1+b1*x0-b1*x1))/(-a0*b1 + b0*a1);
					float iy = y0 + (b0*(-y0*a1+y1*a1+b1*x0-b1*x1))/(-a0*b1 + b0*a1);
				
					int reject = FALSE;
					
					if(a0 < 0 && ix > x0)
						reject = TRUE;

					if(a0 > 0 && ix < x0)
						reject = TRUE;
					
					if(a1 < 0 && ix > x1)
						reject = TRUE;

					if(a1 > 0 && ix < x1)
						reject = TRUE;
					
					if(b0 < 0 && iy > y0)
						reject = TRUE;

					if(b0 > 0 && iy < y0)
						reject = TRUE;
					
					if(b1 < 0 && iy > y1)
						reject = TRUE;

					if(b1 > 0 && iy < y1)
						reject = TRUE;
					
					if(!reject  && sqrt(sqr(estimated_x-ix)+sqr(estimated_y-iy)) < 10)
					{
						ex += ix;
						ey += iy;
						ww += 1.0;
					}
				}
			}
		}
		
	estimated_x = ex/ww;
	estimated_y = ey/ww;


	// Recalculate estimation
	{
	float ex = 0;
	float ey = 0;
	float ww = 0;
	
	for(i=0; i<edgeCount; i++)
		for(j=0; j<edgeCount; j++)
		{
			float dist = sqrt(sqr(edgeTableX[i] - edgeTableX[j]) + sqr(edgeTableY[i] - edgeTableY[j]));
			if(i != j && dmin2 < dist && dist < dmax2)
			{
				// Calculate intersection point
				
				float x0 = edgeTableX[i];
				float y0 = edgeTableY[i];
				float a0 = edgeTableDX[i];
				float b0 = edgeTableDY[i];

				float x1 = edgeTableX[j];
				float y1 = edgeTableY[j];
				float a1 = edgeTableDX[j];
				float b1 = edgeTableDY[j];
				
				if(-a0*b1 + b0*a1 != 0)
				{
					float ix = x0 + (a0*(-y0*a1+y1*a1+b1*x0-b1*x1))/(-a0*b1 + b0*a1);
					float iy = y0 + (b0*(-y0*a1+y1*a1+b1*x0-b1*x1))/(-a0*b1 + b0*a1);
				
					int reject = FALSE;
					
					if(a0 < 0 && ix > x0)
						reject = TRUE;

					if(a0 > 0 && ix < x0)
						reject = TRUE;
					
					if(a1 < 0 && ix > x1)
						reject = TRUE;

					if(a1 > 0 && ix < x1)
						reject = TRUE;
					
					if(b0 < 0 && iy > y0)
						reject = TRUE;

					if(b0 > 0 && iy < y0)
						reject = TRUE;
					
					if(b1 < 0 && iy > y1)
						reject = TRUE;

					if(b1 > 0 && iy < y1)
						reject = TRUE;
					
					if(!reject && sqrt(sqr(estimated_x-ix)+sqr(estimated_y-iy)) < 5)
					{
						ex += ix;
						ey += iy;
						ww += 1.0;
					}
				}
			}
		}
	
	estimated_x = ex/ww;
	estimated_y = ey/ww;
	}
	
	(*e_x) = estimated_x;
	(*e_y) = estimated_y;
}



int **
create_matrix_from_array(int * a, int sizey, int sizex)
{
	int i;
	int ** m = (int **)malloc(sizey*sizeof(int *)); //new int * [sizey];
	for(i=0; i<sizey; i++)
		m[i] = &a[i*sizex];
	return m;
}



/*
	Search for the pupil in the image
*/

void
EyeTracker_FindIris(int * image,  float * x, float * y)
{
	int ** m = create_matrix_from_array(image, h, w);

//	enhance_contrast(m);		/* Should be optional */
	calculate_edges(m);			/* Build list of oriented edge elements*/
	calculate_intersections(x, y);
	
	free(m);	//delete [] m;
}

