/*
 *  NeuronRBF.cpp
 *  RBFNetwork
 *
 *  Created by Magnus Johnsson on 2005-04-05.
 *  Copyright 2005 __MyCompanyName__. All rights reserved.
 *
 */
 
#ifndef NEURONRBF
#define NEURONRBF

#include <Carbon/Carbon.h>

class NeuronRBF
{
public:
	NeuronRBF();
	void setNbrOfInputs(int nbrOfInputs);		// tell the neuron its number of inputs
	void setPreLayer(NeuronRBF * preLay);		// provides a pointer to the previous layer 
	double getActivationLevel();	// Returns activationLevel.
	void setActivationLevel(double activation);	// Set the activation level explicitly.
	void activate();	// Activate the unit.
	void activateHidden();	// Activate a hidden unit
	void setWeight(int i, double weight);	// Set the weight that belongs to input i. 
	double getWeight(int i);	// Returns the weight that belongs to input i.
	~NeuronRBF();	// destructor
	double getU(int i);	// Returns the U (i.e. the component of the center of the receptive field) that belongs to input i.
	double getS();	// Returns the width of the receptive field.
	void setU(int i, double aU);	// Set the u value that belongs to input i.
	void setS(double aS);	// Set the s value.
private:
	double TRESHOLD;	// constant
	double activationLevel;
	double * weights;
	double * u;
	double s;
	double * inputs;
	NeuronRBF * preceedingLayer;
	int nbrOfIn;	// number of inputs
	
	void getInputs();	// Get the inputs from the units in the preceeding layer and put it in an array.
	double Ran(double low, double high);	// Returns a random double between low and high.
};

#endif