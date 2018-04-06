//
//	KNN.h		This file is a part of the IKAROS project
// 							<K Nearest Neighbors implemented with KD-Tree>
//
//    Copyright (C) 2007 <Alexander Kolodziej>
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
//	Created: <August 2007>
//
//	<K Nearest Neighbors implemented with KD-Tree>

#ifndef KNN_H
#define KNN_H

#include "IKAROS.h"
#include "KD_Tree.h"

#define EUCLIDIAN 0
#define MANHATTAN 1

//#define CHECK_WITH_QS


struct tbl_sizes {
    int     container_size;
    int     instances;
};


class KNN: public Module
{
public:

    KNN(Parameter * p);
    virtual ~KNN();

    static Module * Create(Parameter * p);

    void		SetSizes();
    void 		Init();
    void 		Tick();

    KD_Tree     *kd_tree;
    float       **neighbors_matrix;

    void        FillOutputArraysFromNeighborsMatrix(void);

    int         k;
    int         distance_type;
    int         minimum_tree_size_for_rebuild;
    bool        verbose;
    bool        auto_rebuild;
    bool        check_for_clones;
    float       unbalanced_tree_size_ratio_limit;

    float       *input;
    float       *t_input;
    float       *t_output;
    float       *learn;
    int         input_size;
    int         class_size;

    float       **input_table;
    float       **output_table;
    float       *distance_table;

    void        CheckParameters(void);


    #ifdef CHECK_WITH_QS
    #define DB_INIT_SIZE 100
    float       **database;
    tbl_sizes   db_sizes;
    bool        InsertInstance(float *instance, float *classification);
    bool        DeleteInstance(float *instance);
    void        ReallocDatabase();
    bool        Equals(float * sample, float * instance);
    void        CalculateDistances(float **db, int amount);
    float       Distance(float *sample, float *instance);
    void        FillTablesFromDB(float **db, struct tbl_sizes *sizes);
    void        PrintTable(char *title, float **array, int amount, int elements);
    void        QuicksortDatabase(float **db, int left, int right, int pivot_index);
    int         Partition(float **db, int left, int right, int pivot_index);
    bool        CheckResultsWithQS(void);
    #endif
};


#endif
