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



Trainer::~Trainer()
{
    destroy_matrix(training_data);
    destroy_matrix(testing_data);
}



void
Trainer::Init()
{
    Bind(input_size, "input_size");
    Bind(output_size, "output_size");
    
    test_output = GetInputArray("TEST_OUTPUT", false);
    test_output = GetOutputArray("TEST_INPUT");
    train_input = GetOutputArray("TRAIN_INPUT");
    train_target = GetOutputArray("TRAIN_TARGET");
    
    // Read training files
    
    const char * filename = GetValue("training_file");
    if(!filename)
    {
        Notify(msg_fatal_error, "Trainer: no training file.");
        return;
    }
    
    FILE * f = fopen(filename, "r");
    if(!f)
    {
        Notify(msg_fatal_error, "Trainer: training file \"%s\" could not be opened.", filename);
        return;
    }
    
    // Count lines
    
    training_examples = 0;
    while(!feof(f))
    {
        fscanf(f, "%*[^\n]\n");
        training_examples++;
    }
    
    training_data = create_matrix(input_size+output_size, training_examples);
    
    // Read data
    
    fseek(f, 0, SEEK_SET);
    for(int i=0; i<input_size+output_size; i++)
        for(int j=0; j<training_examples; j++)
            fscanf(f, "%f", &training_data[j][i]);
    
    fclose(f);
/*
    printf("training_examples = %d\n", training_examples);
    
    for(int i=0; i<input_size+output_size; i++)
    {
        for(int j=0; j<training_examples; j++)
            printf("%f", training_data[j][i]);
        printf("\n");
    }
    printf("\n");
*/


    // Read testing files
    
    filename = GetValue("testing_file");
    if(!filename)
    {
        Notify(msg_fatal_error, "Trainer: no testing file.");
        return;
    }
    
    f = fopen(filename, "r");
    if(!f)
    {
        Notify(msg_fatal_error, "Trainer: testing file \"%s\" could not be opened.", filename);
        return;
    }
    
    // Count lines
    
    testing_examples = 0;
    while(!feof(f))
    {
        fscanf(f, "%*[^\n]\n");
        testing_examples++;
    }
    
    testing_data = create_matrix(input_size+output_size, testing_examples);
    
    // Read data
    
    fseek(f, 0, SEEK_SET);
    for(int i=0; i<input_size+output_size; i++)
        for(int j=0; j<testing_examples; j++)
            fscanf(f, "%f", &testing_data[j][i]);
    
    fclose(f);

}



void
Trainer::Tick()
{

}

static InitClass init("Trainer", &Trainer::Create, "Source/Modules/IOModules/FileInput/Trainer/");
