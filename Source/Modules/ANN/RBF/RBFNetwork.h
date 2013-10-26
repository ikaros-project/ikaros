//
//	  RBFNetwork.h		This file is a part of the IKAROS project
//
//    Copyright (C) 2005 Magnus Johnsson
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
//	Created: 2005-04-04 by Magnus Johnsson
//

#ifndef RBFNETWORK
#define RBFNETWORK

#include "IKAROS.h"
#include "NeuronRBF.h"

class RBFNetwork: public Module 
{
public:

    RBFNetwork(Parameter * p);
    virtual ~RBFNetwork();
        
    static Module * Create(Parameter * p);

    void Init();
    void Tick();

private:
	int INPUTSIZE;
	int HIDDENSIZE;
	int OUTPUTSIZE;
	double INPUT_TRESHOLD;
	double HIDDEN_TRESHOLD;
	double PW;
	double PU;
	double PS;
	int TRAINING_SET_SIZE;
	float * input;
	float * target_output;
	NeuronRBF * inputLayer;
	NeuronRBF * hiddenLayer;
	NeuronRBF * outputLayer;
	double * dEdW;
	double * dEdU;
	double * dEdS;
	double sumI;
	double sumP;
	int iteration;
	int epoch;
	double MEAN_ERROR_LIMIT;
	bool fullyTrained;
	
	void backpropagate(float * input, float * targetOutput);
	void testNetwork();
};

#endif
