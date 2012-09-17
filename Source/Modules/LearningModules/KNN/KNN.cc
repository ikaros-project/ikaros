//
//	KNN.cc	This file is a part of the IKAROS project
// 					<K Nearest Neighbors implemented with KD-Tree>
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

#include "KNN.h"
#include <string.h>
#include "KD_Tree.h"


Module *
KNN::Create(Parameter * p)
{
	return new KNN(p);
}


KNN::KNN(Parameter * p): Module(p)
{
    AddInput("T_INPUT");
    AddInput("T_OUTPUT");
    AddInput("LEARN");

    AddInput("INPUT");

    AddOutput("INPUT_TABLE", unknown_size);
    AddOutput("OUTPUT_TABLE", unknown_size);
    AddOutput("DISTANCE_TABLE", unknown_size);

    CheckParameters();
}


KNN::~KNN()
{
    delete kd_tree;

    #ifdef CHECK_WITH_QS
    // destroy the instances (arrays) in the database
    int i;
    for (i = 0; i < db_sizes.instances; i++)
        destroy_array(database[i]);
    if (verbose)
        printf("destroyed %i instances in qs-database\n", i);
    free(database);
    #endif
}


void
KNN::SetSizes()
{
    input_size = GetInputSize("INPUT");
    if (input_size == unknown_size)
        return;

    class_size = GetInputSize("T_OUTPUT");
    if (class_size == unknown_size)
        return;

    SetOutputSize("DISTANCE_TABLE", k);
    SetOutputSize("OUTPUT_TABLE", class_size, k);
    SetOutputSize("INPUT_TABLE", input_size, k);
}


void
KNN::Init()
{
    t_input = GetInputArray("T_INPUT");
    t_output = GetInputArray("T_OUTPUT");
    input = GetInputArray("INPUT");
    learn = GetInputArray("LEARN");

    input_table = GetOutputMatrix("INPUT_TABLE");
    output_table = GetOutputMatrix("OUTPUT_TABLE");
    distance_table = GetOutputArray("DISTANCE_TABLE");

    #ifdef CHECK_WITH_QS
    database = (float**) malloc(sizeof(float*) * DB_INIT_SIZE);
    if (database == NULL)
        Notify(msg_fatal_error, "KNN - failed to malloc initial qs-database size to %i\n", DB_INIT_SIZE);
    db_sizes.container_size = DB_INIT_SIZE;
    db_sizes.instances = 0;
    #endif

    kd_tree = new KD_Tree(input_size, class_size, distance_type);

    kd_tree->SetMinimumTreeSizeForRebuild(minimum_tree_size_for_rebuild);
    kd_tree->SetVerbose(verbose);
    kd_tree->SetAutoRebuild(auto_rebuild);
    kd_tree->SetCheckForClones(check_for_clones);
    kd_tree->SetUnbalancedTreeSizeRatioLimit(unbalanced_tree_size_ratio_limit);
}

void
KNN::Tick()
{

    if (learn[0] > 0){
        kd_tree->InsertInstance(t_input, t_output);

        #ifdef CHECK_WITH_QS
        if (! InsertInstance(t_input, t_output)){
            printf("KNN - instance not saved! (already in qs-database?)\n");
            PrintTable("illegal clone", &t_input, 1, input_size);
        }
        #endif

        //return;
    }

    /**            // this is an example of how to delete
    if (blabla){
        kd_tree->DeleteInstance(...); // deletes from kd_tree
        #ifdef CHECK_WITH_QS
        DeleteInstance(...); // deletes from quicksort database
        #endif
    }
    **/

    #ifdef CHECK_WITH_QS
    CalculateDistances(database, db_sizes.instances);
    QuicksortDatabase(database, 0, db_sizes.instances-1, 0);
    #endif

    neighbors_matrix = kd_tree->FindKNearestNeighbors(input, k);
    FillOutputArraysFromNeighborsMatrix();

    #ifdef CHECK_WITH_QS
    CheckResultsWithQS();
    #endif
}


void
KNN::FillOutputArraysFromNeighborsMatrix()
{
    int i;

    //PrintTable("matrix", neighbors_matrix, k, input_size+class_size+1);

    for (i = 0; i < k; i++)
        if (neighbors_matrix[i][input_size+class_size] == -1.0)
            distance_table[i] = -1.0;
        else{
            copy_array(input_table[i], neighbors_matrix[i], input_size);
            copy_array(output_table[i], neighbors_matrix[i]+input_size, class_size);
            distance_table[i] = neighbors_matrix[i][input_size+class_size];
        }
}


void
KNN::CheckParameters()
{
    k = GetIntValue("k", 5);
    if (k < 1)
        Notify(msg_fatal_error, "KNN - ** ERROR ** Paramater k can not be smaller than 1.");
    distance_type = GetIntValueFromList("distance_type", "euclidian/manhattan");
    minimum_tree_size_for_rebuild = GetIntValue("minimum_tree_size_for_rebuild", 100);
    verbose = GetBoolValue("verbose", false);
    auto_rebuild = GetBoolValue("auto_rebuild", true);
    check_for_clones = GetBoolValue("check_for_clones", false);
    unbalanced_tree_size_ratio_limit = GetFloatValue("unbalanced_tree_size_ratio_limit", 2.4);
}




#ifdef CHECK_WITH_QS
bool
KNN::CheckResultsWithQS(void)
{
    int i, j, instances;
    bool same1, same2, same = true;

    instances = db_sizes.instances < k ? db_sizes.instances : k;

    for (i = 0; i < instances; i++){
        same1 = false;
        same2 = false;

        if (neighbors_matrix[i][input_size+class_size] == -1)
            break;

        for (j = 0; j < instances; j++){
            if (neighbors_matrix[i][input_size+class_size] == database[j][input_size+class_size])
                same1 = true;
            if (neighbors_matrix[j][input_size+class_size] == database[i][input_size+class_size])
                same2 = true;
        }
        if (same1 && same2)
            same = true;
        else{
            same = false;
            break;
        }
    }

    if (!same){
        printf(" ***** NOT SAME K NEIGHBORS FROM TREE AS FROM QUICKSORT! *****\n");
        PrintTable("qs neighbors", database, instances, input_size+class_size+1);
        printf("Neighbors from KD_Tree\n");
    }

    return same;
}
void
KNN::QuicksortDatabase(float **db, int left, int right, int pivot_index)
{
    int new_pivot_index;

    if (right > left){
        new_pivot_index = Partition(db, left, right, pivot_index);
        QuicksortDatabase(db, left, new_pivot_index - 1, left);
        QuicksortDatabase(db, new_pivot_index + 1, right, new_pivot_index + 1);
    }
}
int
KNN::Partition(float **db, int left, int right, int pivot_index)
{
    float pivot_value;
    float *tmp;
    int store_index, i;

    pivot_value = db[pivot_index][input_size+class_size];

    tmp = db[right];
    db[right] = db[pivot_index];
    db[pivot_index] = tmp;

    store_index = left;

    for (i = left; i < right; i++){
        if (db[i][input_size+class_size] <= pivot_value){
            tmp = db[i];
            db[i] = db[store_index];
            db[store_index] = tmp;
            store_index++;
        }
    }

    tmp = db[right];
    db[right] = db[store_index];
    db[store_index] = tmp;

    return store_index;
}
void
KNN::PrintTable(char *title, float **array, int amount, int elements)
{
    int i, j;

    if (title == NULL)
        printf("--- NULL --- rows: %i --- columns: %i ---\n", amount, elements);
    else
        printf("--- %s --- rows: %i --- columns: %i ---\n", title, amount, elements);

    for (i = 0; i < amount; i++)
        for (j = 0; j < elements; j++){
            if (j < elements - 1)
                printf("%.4f ", array[i][j]);
            else
                printf("%.4f\n", array[i][j]);
        }

}
/** this function (and its children) seemed to take a lot (the most?) time
    when doing the first profile, so all the code is now put in here instead
    of a lot of subcalls **/
void
KNN::CalculateDistances(float **db, int amount)
{
    int i, j;
    float a, b;

    switch (distance_type){
        case EUCLIDIAN:
            for (i = 0; i < amount; i++){
                db[i][input_size+class_size] = 0; // neccessary ???
                for (j = 0; j < input_size; j++){
                    db[i][input_size+class_size] += (db[i][j] - input[j]) * (db[i][j] - input[j]);
                }
                db[i][input_size+class_size] = ikaros::sqrt(db[i][input_size+class_size]);
            }
            break;
        case MANHATTAN:
            for (i = 0; i < amount; i++){
                db[i][input_size+class_size] = 0; // neccessary ???
                for (j = 0; j < input_size; j++){
                    a = db[i][j];
                    b = input[j];
                    db[i][input_size+class_size] += (a-b) > 0 ? (a-b) : - (a-b);
                }
            }
            break;
        default:
            Notify(msg_fatal_error, "KNN - weird value for distance_type (%f)\n", distance_type);
            break;
    }

}
float
KNN::Distance(float *sample, float *instance)
{
    int i;
    float a, b, d = 0;

    switch (distance_type){
        case EUCLIDIAN:
            for (i = 0; i < input_size; i++){
                a = sample[i];
                b = instance[i];
                d += (a-b) * (a-b);
            }
            d = ikaros::sqrt(d);
            break;

        case MANHATTAN:
            for (i = 0; i < input_size; i++){
                a = sample[i];
                b = instance[i];
                d += (a-b) > 0 ? (a-b) : - (a-b);
            }
            break;
    }
    return d;
}
bool
KNN::InsertInstance(float *instance, float *classification)
{
    float * tmp;
    int i;

    if (check_for_clones)
        // check if instance already is in database. if so, return false
        for (i = 0; i < db_sizes.instances; i++)
            if (Equals(instance, database[i]))
                return false;

    // create array for the instance
    tmp = create_array(input_size + class_size + 1); // two last values are used for: class, distance
    copy_array(tmp, instance, input_size);
    copy_array(tmp+input_size, classification, class_size);

    // make database bigger if neccessary
    if ( ! (db_sizes.instances < db_sizes.container_size) )
            ReallocDatabase();

    // link the instance into the database
    database[db_sizes.instances] = tmp;
    db_sizes.instances++;
    //printf("now %i instances in qs database\n", db_sizes.instances);

    //PrintTable("InsertInstance()", &tmp, 1, input_size+class_size);

    return true;
}
bool
KNN::DeleteInstance(float *instance)
{
    int i;
    bool move = false;

    for (i = 0; i < db_sizes.instances; i++)
        if (move)
            database[i-1] = database[i];
        else
            if (Equals(instance, database[i])){
                destroy_array(database[i]);
                move = true;
            }

    if (move)
        db_sizes.instances--;

    return move;
}
void
KNN::ReallocDatabase()
{
    db_sizes.container_size = db_sizes.container_size + 100;

    database = (float**) realloc(database, sizeof(float*) * db_sizes.container_size);
    if (verbose)
        printf("realloced qs-database to size %i\n", db_sizes.container_size);

    if (database == NULL)
        Notify(msg_fatal_error, "KNN - failed to realloc qs-database size to %i\n", db_sizes.container_size);
}
/** fills the output tables with the K nearest neighbors.
    if the database isnt large enough, it fills upp the rest
    with zeroes (at the end of the arrays) **/
void
KNN::FillTablesFromDB(float **db, struct tbl_sizes *sizes)
{
    int i, j, max_i;

    if (sizes->instances < k){
        max_i = sizes->instances;
        if (verbose)
            printf("KNN - in FillTablesFromDB(). qs-database size (%i) is smaller than k (%i). filling up with zero values.\n", db_sizes.instances, k);

        for (i = max_i; i < k; i++){
            for (j = 0; j < input_size; j++)
                input_table[i][j] = 0;
            for (j = 0; j < class_size; j++)
                output_table[i][j] = 0;
            distance_table[i] = 0;
        }
    }
    else
        max_i = k;

    for (i = 0; i < max_i; i++){
        for (j = 0; j < input_size; j++)
            input_table[i][j] = db[i][j];
        for (j = 0; j < class_size; j++)
            output_table[i][j] = db[i][input_size+j];
        distance_table[i] = db[i][input_size+class_size];
    }
}
bool
KNN::Equals(float * sample, float * instance)
{
    int i;

    // check if the two instances are equal. NOTE: not checking the classification variable
    // to do that would be to allow for errors.
    // but perhaps should warn when we an instance with the same coordinates, but a different
    // class, is found ???
    for (i = 0; i < input_size; i++)
        if (sample[i] != instance[i])
            return false;

    return true;
}
#endif
