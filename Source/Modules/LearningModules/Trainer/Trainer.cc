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

using namespace ikaros;

const int random_order = 0;
//const int sequential_order = 1;

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
    }
    
    Module::SetSizes();
}



void
Trainer::Init()
{
    Bind(order, "order");
    Bind(crossvalidation, "crossvalidation");

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
    test_y_last = create_array(size_y);
    
    error = GetOutputArray("ERROR"); // error for current test data point
    
    if(testing_data_x && crossvalidation != 4)
        Notify(msg_warning, "Trainer: testing data ignored since crossvalidation is not set to 'input'");
    
    // Set start data point for training abd validation

    if(crossvalidation == 2) // even
        training_current = 1;
    else if(crossvalidation == 3) // odd
        testing_current = 1;
}



void
Trainer::Tick()
{
    switch(crossvalidation)
    {
        case 0: // none
            copy_array(train_x, training_data_x[training_current], size_x);
            copy_array(train_y, training_data_y[training_current], size_y);
            if(order == random_order) // random
                training_current = random(training_no_of_examples);
            else if(++training_current >= training_no_of_examples)
                    training_current = 0;
            break;
        
        case 1: // all
            *error = dist(test_y_last, test_y, size_y);
            copy_array(train_x, training_data_x[training_current], size_x);
            copy_array(train_y, training_data_y[training_current], size_y);
            copy_array(test_x, train_x, size_x);
            copy_array(test_y_last, train_y, size_y);

            if(order == random_order)
                training_current = random(training_no_of_examples);
            else if(++training_current >= training_no_of_examples)
                    training_current = 0;
            break;
        
        case 2: // even
            *error = dist(test_y_last, test_y, size_y);
            copy_array(train_x, training_data_x[training_current], size_x);
            copy_array(train_y, training_data_y[training_current], size_y);
            copy_array(test_x, training_data_x[testing_current], size_x);
            copy_array(test_y_last, training_data_y[testing_current], size_y);
            
            if(order == random_order)
            {
                while((training_current = random(training_no_of_examples)) % 2 == 1) ;
                while((testing_current = random(training_no_of_examples)) % 2 == 0) ;
            }
            else
            {
                training_current+=2;
                testing_current+=2;
                
                if(training_current >= training_no_of_examples)
                    training_current = 1;
               if(testing_current >= training_no_of_examples)
                    testing_current = 0;
            }
            break;
        
        case 3: // odd
            *error = dist(test_y_last, test_y, size_y);
            copy_array(train_x, training_data_x[training_current], size_x);
            copy_array(train_y, training_data_y[training_current], size_y);
            copy_array(test_x, training_data_x[testing_current], size_x);
            copy_array(test_y_last, training_data_y[testing_current], size_y);
            
            if(order == random_order)
            {
                while((training_current = random(training_no_of_examples)) % 2 == 0) ;
                while((testing_current = random(training_no_of_examples)) % 2 == 1) ;
            }
            else
            {
                training_current+=2;
                testing_current+=2;
                
                if(training_current >= training_no_of_examples)
                    training_current = 0;
               if(testing_current >= training_no_of_examples)
                    testing_current = 1;
            }
           break;
        
        case 4: // input
            *error = dist(test_y_last, test_y, size_y);
            copy_array(train_x, training_data_x[training_current], size_x);
            copy_array(train_y, training_data_y[training_current], size_y);
            copy_array(test_x, testing_data_x[testing_current], size_x);
            copy_array(test_y_last, testing_data_y[testing_current], size_y);
            
            if(order == random_order) // random
                training_current = random(training_no_of_examples);
            else if(++training_current >= training_no_of_examples)
                    training_current = 0;
            break;
    }
}



static InitClass init("Trainer", &Trainer::Create, "Source/Modules/LearningModules/Trainer/");
