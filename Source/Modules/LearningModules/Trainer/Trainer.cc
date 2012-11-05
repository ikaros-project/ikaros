//
//	Trainer.cc		This file is a part of the IKAROS project
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

#include "Trainer.h"



void
Trainer::SetSizes()
{
    int sx = GetInputSizeX("TRAINING_DATA_X");
    int sy = GetInputSizeX("TRAINING_DATA_Y");

    if (sx != unknown_size && sy != unknown_size)
    {
        SetOutputSize("TRAIN_X", sx);
        SetOutputSize("TRAIN_Y", sy);
        SetOutputSize("TEST_X", sx);
        
        SetOutputSize("error", 1);
        SetOutputSize("accumulated_error", 1);
    }
}



void
Trainer::Init()
{
    training_data_x = GetInputMatrix("TRAINING_DATA_X");
    training_data_y = GetInputMatrix("TRAINING_DATA_Y");
    training_no_of_examples = GetInputSizeY("TRAINING_DATA_X");
    training_current = 0;

    testing_data_x = GetInputMatrix("TESTING_DATA_X", false);
    testing_data_y = GetInputMatrix("TESTING_DATA_Y", false);
    testing_no_of_examples = GetInputSizeY("TESTING_DATA_X");
    testing_current = 0;
    
    size_x = GetInputSizeX("TRAINING_DATA_X");
    size_y = GetInputSizeX("TRAINING_DATA_Y"); // yes, this is correct
    
    train_x = GetOutputArray("TRAIN_X");
    train_y = GetOutputArray("TRAIN_Y");
    
    test_x = GetOutputArray("TEST_X", false);
    test_y = GetInputArray("TEST_Y", false);
}



void
Trainer::Tick() // TODO: Random order
{
    copy_array(train_x, training_data_x[training_current], size_x);
    copy_array(train_y, training_data_y[training_current], size_y);

    training_current++;
    if(training_current >= training_no_of_examples)
        training_current = 0;
    
    if(testing_data_x)
    {
        copy_array(test_y_last, test_y, size_y);
        copy_array(test_x, testing_data_x[testing_current], size_x);
        copy_array(test_y, testing_data_y[testing_current], size_y);

        testing_current++;
        if(testing_current >= testing_no_of_examples)
            testing_current = 0;
        
        // Calculate error
        
        error = dist(test_y_last);
    }
    
    // TODO: Check iterations / infinite / criteria
}



static InitClass init("Trainer", &Trainer::Create, "Source/Modules/LearningModules/Trainer/");
