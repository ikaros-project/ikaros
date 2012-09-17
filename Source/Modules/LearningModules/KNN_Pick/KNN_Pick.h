//
//	KNN_Pick.h   This file is a part of the IKAROS project
//               This module provides a few simple ways to pick a class based on K neighbors.
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

#ifndef KNN_Pick_H
#define KNN_Pick_H

// if this is defined, a fatal error occurs when distances <0 are found
#define CHECK_FOR_NEGATIVE_DISTANCE
#include "IKAROS.h"


class KNN_Pick: public Module
{
public:

    KNN_Pick(Parameter * p);
    virtual ~KNN_Pick();

    static Module * Create(Parameter * p);

    void		SetSizes();
    void 		Init();
    void 		Tick();

    void        PrintTable(char *title, float **array, int amount, int elements);
    void        CheckParameters(void);

                // used when outputs are categorical. class will be the class of
                // the elements we found the most of. might be weighed.
    float       CountAmount(void);
                // used when outputs are qualitative (non-categorical). class is
                // the mean of the neighbors. might be weighed.
    float       CalculateMean(void);
                // returns a multiplier with which to manipulate the amount of
                // instances we found. returns 1 if weighed=false
    inline float    GetWeightFactor(float distance);
    void        CheckForNegativeDistance(void);
    int         GetClosestIndex(void);

    float       *output_table;
    float       *distance_table;
    float       *class_output;

    int         k;
    float       *categories;

    bool        categorical;
    bool        weighed;
    float       weight_divisor;
};

#endif
