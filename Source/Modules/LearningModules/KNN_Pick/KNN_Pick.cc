//
//	KNN_Pick.cc	This file is a part of the IKAROS project
// 					This module provides a few simple ways to pick a class based on K neighbors.
//
//    Copyright (C) 2007 Alexander Kolodziej
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
//	Created: July 2007
//
//	<Additional description of the module>

#include "KNN_Pick.h"
//#include <values.h>

#define EMPTY -1


Module *
KNN_Pick::Create(Parameter * p)
{
	return new KNN_Pick(p);
}


KNN_Pick::KNN_Pick(Parameter * p): Module(p)
{
    AddInput("OUTPUT_TABLE");
    AddInput("DISTANCE_TABLE");

    AddOutput("CLASS_OUTPUT", 1);

    CheckParameters();
}


void
KNN_Pick::SetSizes()
{
    int tmp;

    k = GetInputSize("OUTPUT_TABLE");
    if (k == unknown_size)
        return;

    tmp = GetInputSize("DISTANCE_TABLE");

    if (tmp != k)
        Notify(msg_fatal_error, "KNN_Pick - Sizes for DISTANCE_TABLE (size=%i) and OUTPUT_TABLE (size=%i) do not match!\n", tmp, k);
}


void
KNN_Pick::Init()
{
    output_table = GetInputArray("OUTPUT_TABLE");
    distance_table = GetInputArray("DISTANCE_TABLE");

    class_output = GetOutputArray("CLASS_OUTPUT");

    categories = create_array(k);
}


KNN_Pick::~KNN_Pick()
{
    destroy_array(categories);
}


/** calculate the class_output depending on what is chosen in the IKC file.
    if an element with zero distance is found, the class of that element
    is chosen (the first zero distance element is chosen if there would be
    more than one). **/
void
KNN_Pick::Tick()
{
    //printf("\n NEW TICK \n");
    //PrintTable("input table", input_table, 1, k);
    //PrintTable("output_table", &output_table, 1, k);
    //PrintTable("distance_table", &distance_table, 1, k);

    #ifdef CHECK_FOR_NEGATIVE_DISTANCE
    CheckForNegativeDistance();
    #endif

    if (distance_table[GetClosestIndex()] == 0)
        class_output[0] = output_table[GetClosestIndex()];
    else{
        if (categorical)
            class_output[0] = CountAmount();
        else
            class_output[0] = CalculateMean();
    }

    //printf("\nchose class: %.1f\n\n", class_output[0]);
}


/** distances shouldnt be allowed to be negative, so it might be a good
    idea to check for that. this function CAN be skipped by not having
    CHECK_FOR_NEGATIVE_DISTANCES defined in the header file. **/
void
KNN_Pick::CheckForNegativeDistance()
{
    int i;

    for (i = 0; i < k; i++)
        if (distance_table[i] < 0)
            Notify(msg_fatal_error, "KNN_Pick - ERROR: No negative distances allowed!\n");
}


/** this function returns a factor with which we can weigh our elements
    depending on their distance. returns 1 if elements are not supposed to be
    weighed. feel free to edit if you want some other weighing. **/
inline float
KNN_Pick::GetWeightFactor(float distance)
{
    if (weighed)
        //return weight_divisor / distance; // beware of dividing by zero!
        return weight_divisor / ikaros::log(1.0001 + distance);
    else
        return 1.0;
}


/** return the class of the category with the most elements, or if there
    are two or more categories with an equal amount of elements, return
    the class of the one which has the closes element.
    the amount can be weighed.
    NOTE: assuming categorical classes. qualitative classes not allowed.
    this should be checked in CheckParameters() though. **/
float
KNN_Pick::CountAmount()
{
    int i, j, m;
    float this_category;
    float this_size;
    float biggest_size;
    int this_closest;
    int biggest_closest = 0;
    int c = 0;

    // categories is an array where the categories that have been
    // found are entered, so they are not counted more than once
    set_array(categories, EMPTY, k);
    biggest_size = 0;

    //printf("count amount\n");
    for (i = 0; i < k; i++){
        this_category = output_table[i];

        //printf("looking at output_table[%i]=%f\n", i, output_table[i]);

        // look for this category in categories
        for (j = 0; j < k; j++)
            // break if we find it or run out of entries
            if ((categories[j] == this_category) || (categories[j] == EMPTY))
                break;

        // if this category already is calculated, continue
        if (categories[j] == this_category)
            continue;

        // else we have (categories[j] == EMPTY) and hence we have found
        // a new category. the for loop above always breaks before j == k

        //printf("new category: %.1f \t", this_category);
        c++;

        // index i is the first (and hence closest) we have found of this category
        categories[j] = this_category;
        this_size = 1 * GetWeightFactor(distance_table[i]);
        this_closest = i;

        // since this category wasnt found earlier, we can start looping from the
        // next element
        for (m = i+1; m < k; m++)
            if (output_table[m] == this_category){
                this_size += 1 * GetWeightFactor(distance_table[m]);
                if (distance_table[m] < distance_table[this_closest])
                    this_closest = m;
            }

        //printf("size: %.2f\n", this_size);

        // see if this category is bigger than the last biggest was
        // if so, make this the biggest
        if (this_size > biggest_size){
            //printf("new biggest cat: %.1f\n", output_table[this_closest]);
            biggest_size = this_size;
            biggest_closest = this_closest;
        }

        // if its the same size as the previous biggest
        // make the one with the closest element the biggest
        else if (this_size == biggest_size)
            if (distance_table[this_closest] < distance_table[biggest_closest]){
                //printf("new biggest/closest cat: %.1f\n", output_table[this_closest]);
                biggest_size = this_size;
                biggest_closest = this_closest;
            }

    }
    //printf("found %i categories\n", c);
    return output_table[biggest_closest];
}


/** calculate mean of classes (this function is not used when classes are categorical).
    the amount of instances might be weighed. **/
float
KNN_Pick::CalculateMean()
{
    float tot = 0, instances = 0;
    int i;

    for (i = 0; i < k; i++){
        tot += output_table[i] * GetWeightFactor(distance_table[i]);
        instances += 1 * GetWeightFactor(distance_table[i]);
    }
    //printf("tot=%f   instances=%f", tot, instances);
    return tot / instances;
}


/** return index to the (first) closest element. **/
int
KNN_Pick::GetClosestIndex()
{
    int i, closest_index;

    closest_index = 0;

    for (i = 1; i < k; i++)
        if (distance_table[i] < distance_table[closest_index])
            closest_index = i;

    return closest_index;
}


/** prints some matrix with fancy headers. yeah, debugging... **/
void
KNN_Pick::PrintTable(char *title, float **array, int amount, int elements)
{
    int i, j;

    if (title == NULL)
        printf("--- NULL --- rows: %i --- columns: %i ---\n", amount, elements);
    else
        printf("--- %s --- rows: %i --- columns: %i ---\n", title, amount, elements);

    for (i = 0; i < amount; i++)
        for (j = 0; j < elements; j++){
            if (j < elements - 1)
                printf("%.2f ", array[i][j]);
            else
                printf("%.2f\n", array[i][j]);
        }

}


void
KNN_Pick::CheckParameters()
{
    categorical = GetBoolValue("categorical", true);
    weighed = GetBoolValue("weighed", false);
    weight_divisor = GetFloatValue("weight_divisor", 1.0);
}

