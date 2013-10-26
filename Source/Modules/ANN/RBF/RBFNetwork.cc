//
//	  RBFNetwork.h		This file is a part of the IKAROS project
// 							This module implements a RBF network.
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
//	<Additional description of the module>


#include "RBFNetwork.h"
#include <math.h>
#include <iostream>
#include "IKAROS_Utils.h"

//#define DEBUG

using namespace std;

Module * RBFNetwork::Create(Parameter * p)
{
	return new RBFNetwork(p);
}



RBFNetwork::RBFNetwork(Parameter * p):
	Module(p)
{ 
/*
     INPUTSIZE =  GetIntValue(p, "inputsize", 9);	// input size + 1, because of the treshold
     HIDDENSIZE =  GetIntValue(p, "hiddensize", 10);
     OUTPUTSIZE =  GetIntValue(p, "outputsize", 3);
     INPUT_TRESHOLD = (double) GetFloatValue(p, "input_treshold", 1.0);
     HIDDEN_TRESHOLD = (double) GetFloatValue(p, "hidden_treshold", 1.0);
     PW = (double) GetFloatValue(p, "pw", 1);
     PU = (double) GetFloatValue(p, "pu", 1);
     PS = (double) GetFloatValue(p, "ps", 1);
     TRAINING_SET_SIZE = GetIntValue(p, "training_set_size", 12);
     MEAN_ERROR_LIMIT = (double) GetFloatValue(p, "mean_error_limit", 0.001);
*/
    
    INPUTSIZE =  GetIntValue("inputsize", 9);	// input size + 1, because of the treshold
	HIDDENSIZE =  GetIntValue("hiddensize", 10);
	OUTPUTSIZE =  GetIntValue("outputsize", 3);
	INPUT_TRESHOLD = (double) GetFloatValue("input_treshold", 1.0);
	HIDDEN_TRESHOLD = (double) GetFloatValue("hidden_treshold", 1.0);
	PW = (double) GetFloatValue("pw", 1);
	PU = (double) GetFloatValue("pu", 1);
	PS = (double) GetFloatValue("ps", 1);
	TRAINING_SET_SIZE = GetIntValue("training_set_size", 12);
	MEAN_ERROR_LIMIT = (double) GetFloatValue("mean_error_limit", 0.001);
	
	AddInput("INPUT");
	AddInput("INPUT_TARGET");
}



void
RBFNetwork::Init()
{
	input = GetInputArray("INPUT");
	target_output = GetInputArray("INPUT_TARGET");

	inputLayer = new NeuronRBF[INPUTSIZE + 1];		// create the input layer (plus one because of the treshold)
	hiddenLayer = new NeuronRBF[HIDDENSIZE + 1];	// create the hidden layer (plus one because of the treshold)
	outputLayer = new NeuronRBF[OUTPUTSIZE];		// create the output layer
	dEdW = new double[(HIDDENSIZE + 1) * OUTPUTSIZE];
	dEdU = new double[(INPUTSIZE + 1) * (HIDDENSIZE + 1)];
	dEdS = new double[HIDDENSIZE + 1];
	
	// initialize the input units
	for (int i = 0; i < (INPUTSIZE + 1); i++) { 
		inputLayer[i].setNbrOfInputs(0);	
		inputLayer[i].setPreLayer(NULL);	
	}						
		
	// initialize the hidden units
	for (int i = 0; i < (HIDDENSIZE + 1); i++) { 
		hiddenLayer[i].setNbrOfInputs(INPUTSIZE + 1);
		hiddenLayer[i].setPreLayer(inputLayer);
	}
		
	// initialize the output units
	for (int i = 0; i < OUTPUTSIZE; i++) {
		outputLayer[i].setNbrOfInputs(HIDDENSIZE + 1);
		outputLayer[i].setPreLayer(hiddenLayer);
	}

	// initialize the activation for the tresholds
	inputLayer[0].setActivationLevel(INPUT_TRESHOLD);
	hiddenLayer[0].setActivationLevel(HIDDEN_TRESHOLD);
	
	// initialize dEdW with zeroes
	for (int i = 0; i < ((HIDDENSIZE + 1) * OUTPUTSIZE); i++)
		dEdW[i] = 0;
	// initialize dEdU with zeroes
	for (int i = 0; i < ((INPUTSIZE + 1) * (HIDDENSIZE + 1)); i++)
		dEdU[i] = 0;
	// initialize dEdS with zeroes
	for (int i = 0; i < (HIDDENSIZE + 1); i++)
		dEdS[i] = 0;

	sumI = 0;	// for the mean error
	sumP = 0;	// for the mean error
	iteration = 0;	
	epoch = 0;
	fullyTrained = false;	// a flag to stop the training phase
}



RBFNetwork::~RBFNetwork()
{
	delete[] inputLayer;
	delete[] hiddenLayer;
	delete[] outputLayer;
	delete[] dEdW;
	delete[] dEdU;
	delete[] dEdS;
	inputLayer = 0;
	hiddenLayer = 0;
	outputLayer = 0;
	dEdW = 0;
	dEdU = 0;
	dEdS = 0;
}



void RBFNetwork::backpropagate(float * input, float * targetOutput)
{
	// assign activation levels to the input units
	for (int i = 1; i < (INPUTSIZE + 1); i++) 
		inputLayer[i].setActivationLevel((double)input[i - 1]);
		
	// activate the hidden layer
	for (int i = 1; i < (HIDDENSIZE + 1); i++)
		hiddenLayer[i].activateHidden();
	
	// activate the output layer
	for (int i = 0; i < OUTPUTSIZE; i++)
		outputLayer[i].activate();
		
	// compute dEdW
	int k = 0;
	double temp = 0;
	double out = 0;
	for (int i = 0; i < OUTPUTSIZE; i++) {
		out = outputLayer[i].getActivationLevel();
		temp = out * (1 - out) * (out - (double)targetOutput[i]);
		for (int j = 0; j < (HIDDENSIZE + 1); j++) {
			dEdW[k] = temp * hiddenLayer[j].getActivationLevel();
			k++;
		}
	}
	
	// compute dEdU and dEdS
	k = 0;
	for (int i = 0; i < (HIDDENSIZE + 1); i++) {
		temp = 0;
		for (int n = 0; n < OUTPUTSIZE; n++)
			temp += outputLayer[n].getWeight(i) * dEdW[((HIDDENSIZE + 1) * n) + i];
		dEdS[i] = ((-2 / hiddenLayer[i].getS()) * log(hiddenLayer[i].getActivationLevel())) * temp;
		for (int j = 0; j < (INPUTSIZE + 1); j++) {
			dEdU[k] = ((inputLayer[j].getActivationLevel() - hiddenLayer[i].getU(j)) / (hiddenLayer[i].getS() * hiddenLayer[i].getS())) * temp;
			k++;
		}
	}
	
	// adjust the weights between the hidden layer and the output layer
	double weight;
	k = 0;
	for (int j = 0; j < OUTPUTSIZE; j++) {
		for (int i = 0; i < (HIDDENSIZE + 1); i++) {
			weight = outputLayer[j].getWeight(i) + ((-1) * PW * dEdW[k]);
			outputLayer[j].setWeight(i, weight);
			k++;
		}
	}
	
	// adjust the U:s (i.e. the center of the receptive fields) for the hidden nodes
	double u;
	k = 0;
	for (int j = 0; j < (HIDDENSIZE + 1); j++) {
		for (int i = 0; i < (INPUTSIZE + 1); i++) {
			u = hiddenLayer[j].getU(i) + ((-1) * PU * dEdU[k]);
			hiddenLayer[j].setU(i, u);
			k++;
		}
	}
	
	// adjust the S:s (i.e. the width of the receptive fields) for the hidden nodes
	double s;
	for (int j = 0; j < (HIDDENSIZE + 1); j++) {
		s = hiddenLayer[j].getS() + ((-1) * PS * dEdS[j]);
		hiddenLayer[j].setS(s);
	}
}



void
RBFNetwork::Tick()
{
	if (!fullyTrained) {
		backpropagate(input, target_output);
		iteration++;
	
		// for the mean error
		for (int i = 0; i < OUTPUTSIZE; i++)
			sumI += pow((double)target_output[i] - outputLayer[i].getActivationLevel(), 2);
	
		// for the mean error
		sumP += sumI;
		sumI = 0;
	
		if (iteration % TRAINING_SET_SIZE) {
			epoch++;
			
			cout << "m_error " << (sumP / TRAINING_SET_SIZE) << endl;	// temporary
			cout << "epoch: " << epoch << endl;
		
			if ((sumP / TRAINING_SET_SIZE) < MEAN_ERROR_LIMIT) {
				cout << "\n\nThe network is fully trained" << endl;
				cout << "Mean error = " << (sumP / TRAINING_SET_SIZE) << endl;
				cout << epoch << " epochs have been run" << endl;
				fullyTrained = true; 
				cout << "Type the input to the network or q to quit\n" << endl;
			}
		
			sumP = 0;
		}
	}	
	else {	// test the fully trained network
		testNetwork();
	}
}

void RBFNetwork::testNetwork()
{
	char * str = new char[100];
	char * subStr = new char[2];
	double * in = new double[INPUTSIZE - 1];
	
	// get input from user and transform it to an array of doubles
	cin.getline(str, 100);
	
	// quit the program by typing q
	if (str[0] == 'q')
		exit(0);
	
	for (int i = 0; i < (INPUTSIZE - 1); i++) {
		subStr[0] = str[i];
		subStr[1] = '\0';
		in[i] = (double) atoi(subStr);
	}
		
	// activate the BP-network with the input from the user
		
	// assign activation levels to the input units
	for (int i = 1; i < (INPUTSIZE + 1); i++) 
		inputLayer[i].setActivationLevel((double)in[i - 1]);
		
	// activate the hidden layer
	for (int i = 1; i < (HIDDENSIZE + 1); i++)
		hiddenLayer[i].activateHidden();
		
	// activate the output layer
	for (int i = 0; i < OUTPUTSIZE; i++)
		outputLayer[i].activate();
		
	cout << "output: ";
	
	// get the activation of the output layer and output it to standard output after rounding
	for (int i = 0; i < OUTPUTSIZE; i++)
		cout << round(outputLayer[i].getActivationLevel()) << " ";
		
	cout << endl;
	
	delete[] str;
	str = 0;
	delete[] subStr;
	subStr = 0;
	delete[] in;
	in = 0;
}


