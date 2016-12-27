//
//	  SpatialClustering.h		This file is a part of the IKAROS project
//
//
//    Copyright (C) 2007 Christian Balkenius
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

#ifndef SPATIALCLUSTERING
#define SPATIALCLUSTERING

#include "IKAROS.h"

class SpatialClustering: public Module
{
public:

	class Cluster
	{
	public:
		float x_sum;
		float y_sum;
		float n;

		float min_x;
		float max_x;
		float min_y;
		float max_y;

		void	Init();
		inline float Distance(float x, float y);
		inline void Update(float x, float y);

		float GetX();
		float GetY();

		float GetWeight();
		float GetSize();
	};

    static int  compare_cluster(const void * c1, const void * c2);
    
    SpatialClustering(Parameter * p) : Module(p) {}
	virtual			~SpatialClustering();

	static Module *	Create(Parameter * p) { return new SpatialClustering(p); }

	void			Init();

	void			InitColorSpace();
	void			InitClusters();
	inline  void	AddPoint(float x, float y);
    
    void            AddCluster(float x, float y, float pixelcount);
	void			Tick();
    
	float **	input;
	float **	output;
    float *     confidence;

    bool         memory_exhausted;
    static const int    cluster_max_n = 100;

	Cluster		cluster[cluster_max_n];
	int			cluster_n;

    float *     cluster_active;
    float **    last_output;
    int         no_of_found_clusters;
    int         no_of_found_clusters_last;
	int			no_of_clusters;
    float       tracking_distance;
    float       threshold;
    bool        sorting;

	int         size_x;
	int         size_y;
	float		max_cluster_area;       // area in pixels within cluster_radius
	float		min_cluster_area;       // minimum filled area within cluster_radius for a cluster
	float		cluster_radius;
};

#endif
