//
//	SpatialClustering.cc	This file is a part of the Ikaros project
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



#include "SpatialClustering.h"

using namespace ikaros;

void
SpatialClustering::Init()
{
    Bind(sorting , "sorting");
	no_of_clusters = GetIntValue("no_of_clusters", 1);

	input = GetInputMatrix("INPUT");

	size_x = GetInputSizeX("INPUT");
	size_y = GetInputSizeY("INPUT");
	output = GetOutputMatrix("OUTPUT");
    confidence = GetOutputArray("CONFIDENCE");
    last_output = create_matrix(2, no_of_clusters);
    
    cluster_active = create_array(no_of_clusters);

    memory_exhausted = false;
    
    // Get and scale relative values to image size (i. e. pixel)
    
    threshold = GetFloatValue("threshold", 0.1);
    cluster_radius = float(size_x)*GetFloatValue("cluster_radius", 0.1);
    max_cluster_area = 2*pi*cluster_radius*cluster_radius;
	min_cluster_area = float(size_x)*float(size_y)*GetFloatValue("min_cluster_area", 0.0001);
    tracking_distance = float(size_x)*GetFloatValue("tracking_distance", 0.25);

}



SpatialClustering::~SpatialClustering()
{
    destroy_matrix(last_output);
}



void
SpatialClustering::Cluster::Init()
{
    x_sum = 0;
	y_sum = 0;
	n = 0;
}



inline float
SpatialClustering::Cluster::Distance(float x, float y)
{
    float cx = x_sum / n;
	float cy = y_sum / n;
	return hypot(cx-x, cy-y);
}



inline void
SpatialClustering::Cluster::Update(float x, float y)
{
    x_sum += x;
	y_sum += y;

	if(n == 0)
	{
		min_x =  x;
		max_x = x;
		min_y = y;
		max_y = y;
	}
	else
	{
		if(x > max_x)
			max_x = x;
		else if(x < min_x)
			min_x = x;

		if(y > max_y)
			max_y = y;
		else if(y < min_y)
			min_y = y;
	}

	n += 1.0;
}



float SpatialClustering::Cluster::GetX()
{
	return (n>0 ? x_sum / n : 0);
}



float SpatialClustering::Cluster::GetY()
{
	return (n>0 ? y_sum / n : 0);
}



float SpatialClustering::Cluster::GetWeight()
{
	return float(n);
}



float SpatialClustering::Cluster::GetSize()
{
	return (max_x-min_x)*(max_y-min_y);
}



void
SpatialClustering::InitClusters()
{
    cluster_n = 0;
}



inline void
SpatialClustering::AddPoint(float x, float y)
{
	// Find closest cluster

	float	min_dist = 1000000;
	int     min_cluster = 0;

	for(int i=0; i<cluster_n; i++)
	{
		float d = cluster[i].Distance(x, y);
		if(d  < min_dist)
		{
			min_dist = d;
			min_cluster = i;
		}
	}

	// Check if close enough

	if(min_dist < cluster_radius)
	{
 		cluster[min_cluster].Update(x, y);
		return;
	}

	// Or add new cluster

    if(cluster_n < cluster_max_n)
    {
        cluster[cluster_n].Init();
        cluster[cluster_n].Update(x, y);
        cluster_n++;
    }
    else if(!memory_exhausted)
    {
        Notify(msg_warning, "%s (SpatialClustering): Cluster storage exhausted. Check parameters!\n", GetName());
        memory_exhausted = true;
    }
}



void
SpatialClustering::AddCluster(float x, float y, float pixelcount)
{
    int c = 0;
    float cert = pixelcount/max_cluster_area;
    if(cert > 1.0)
        cert = 1.0;
    float d = 1000;
    float t;
    for(int i=0; i<no_of_found_clusters_last; i++)
        if((t = hypot(last_output[i][0]-x, last_output[i][1]-y)) < d)
        {
            c = i;
            d = t;
        }
    
    if(d < tracking_distance)
    {
        output[c][0] = x;
        output[c][1] = y;
        confidence[c] = cert;
        cluster_active[c] += 1;
    }
    else if(no_of_found_clusters < no_of_clusters)
    {
        output[no_of_found_clusters][0] = x;
        output[no_of_found_clusters][1] = y;
        confidence[no_of_found_clusters] = cert;
        cluster_active[no_of_found_clusters] = 2;
        no_of_found_clusters++;
    }
    else // Check if it is possible to reassign an incluster_active cluster [possibly reassign smaller/smallest clusters?]
    {
        for(int i=0; i<no_of_clusters; i++)
            if(cluster_active[i] < 1)
            {
                output[i][0] = x;
                output[i][1] = y;
                confidence[i] = cert;
                cluster_active[i] = 2;
                return;
            }
    }
}


int
SpatialClustering::compare_cluster(const void * c1, const void * c2)
{
    Cluster * cluster1 = (Cluster *)(c1);
    Cluster * cluster2 = (Cluster *)(c2);
    
    if(cluster1->GetWeight() < cluster2->GetWeight())
        return -1;
    else if(cluster1->GetWeight() > cluster2->GetWeight())
        return 1;
    else
        return 0;
}


// TEST

float x0=0, x1=0, x2=0, y0=0, y1=0, y2=0;

// get center method: average / center of rect

void
SpatialClustering::Tick()
{
	InitClusters();

    int ccc = 0;
	for(int j=0; j<size_y; j++)
		for(int i=0; i<size_x; i++)
            if(input[j][i] > threshold)
            {
                AddPoint(float(i), float(j));
                ccc++;
            }
    
    no_of_found_clusters_last = no_of_found_clusters;
    copy_matrix(last_output, output, 2, no_of_clusters);
    
    reset_array(confidence, no_of_clusters);

    // Sort clusters
    
    if(sorting)
        qsort(cluster, cluster_n, sizeof(Cluster), &compare_cluster);
    
    // Choose clusters that are large enough

    float t;
	for(int c=0; c<cluster_n; c++)
    {
	    if((t = cluster[c].GetWeight()) > min_cluster_area)
            AddCluster(cluster[c].GetX()/float(size_x), cluster[c].GetY()/float(size_y), t);
    }
    
    for(int i=0; i<no_of_found_clusters; i++)
        cluster_active[i] -= 1;
/*        
    printf("\n\n");
    for(int i=0; i<no_of_clusters; i++)
        printf("%f %f\t\t%.0f\t\t%15.9f\n", output[i][0], output[i][1], cluster_active[i], confidence[i]);
*/
}

static InitClass init("SpatialClustering", &SpatialClustering::Create, "Source/Modules/VisionModules/SpatialClustering/");
