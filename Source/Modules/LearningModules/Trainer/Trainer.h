//
//	Trainer.h			This file is a part of the IKAROS project
//
//    Copyright (C) 2012  Christian Balkenius
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

#ifndef Trainer_
#define Trainer_

#include "IKAROS.h"

class Trainer: public Module
{
public:
    float **    training_data_x;
    float **    training_data_y;
    int         training_no_of_examples;
    int         training_current;

    float **    testing_data_x;
    float **    testing_data_y;
    int         testing_no_of_examples;
    int         testing_current;
    
    float *     train_x;
    float *     train_y;
    
    float *     test_x;
    float *     test_y;
    float *     test_y_last;
    
    int         size_x; // not the usual meaning
    int         size_y;
    
    Trainer(Parameter * p): Module(p) {};
    virtual ~Trainer() {};

    static Module * Create(Parameter * p) { return new Trainer(p); };

    void SetSizes();
    void Init();
    void Tick();
};

#endif

