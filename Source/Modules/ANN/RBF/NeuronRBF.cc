/*
 *  NeuronRBF.cpp
 *  RBFNetwork
 *
 *  Created by Magnus Johnsson on 2005-04-05.
 *  Copyright 2005 __MyCompanyName__. All rights reserved.
 *
 */

#include "NeuronRBF.h"
#include <iostream>
#include <time.h>
#include "IKAROS_Utils.h"

using namespace ikaros;

NeuronRBF::NeuronRBF()
{
	srand(time(0));		// to seed rand().
}

void NeuronRBF::setNbrOfInputs(int nbrOfInputs)
{
	nbrOfIn = nbrOfInputs;
	inputs = new double[nbrOfInputs];
	weights = new double[nbrOfInputs];
	u = new double[nbrOfInputs];
	
	// initialize the neurons weights and u:s to random values in [-0.1, 0.1]
	for (int i = 0; i < nbrOfInputs; i++) {
		weights[i] = Ran(-0.1, 0.1);
		u[i] = Ran(-0.1, 0.1);
	}
	
	//initialize s
	s = 1;
}

void NeuronRBF::setPreLayer(NeuronRBF * preLay)
{
	preceedingLayer = preLay;
}

NeuronRBF::~NeuronRBF()
{
	delete[] inputs;
	delete[] weights;
	delete[] u;
	inputs = 0;
	weights = 0;
	u = 0;
}

double NeuronRBF::getActivationLevel()
{
	return activationLevel;
}

void NeuronRBF::setActivationLevel(double activation)
{
	activationLevel = activation;
}

void NeuronRBF::activate()
{
	getInputs();
	double sum = TRESHOLD;
	
	for (int i = 1; i < nbrOfIn; i++) 
		sum += weights[i] * inputs[i];
		
	activationLevel = 1 / (1 + exp(-sum));
}

void NeuronRBF::activateHidden()
{
	getInputs();
	float * dist = new float[nbrOfIn];
	
	for (int i = 0; i < nbrOfIn; i++) 
		dist[i] = inputs[i] - u[i];
		
	double e_norm = (double)norm(dist, nbrOfIn);
		
	activationLevel = exp(-((e_norm * e_norm) / (2 * s * s)));
	
	delete[] dist;
	dist = 0;
}

void NeuronRBF::setWeight(int i, double weight)
{
	weights[i] = weight;
}

double NeuronRBF::getWeight(int i)
{
	return weights[i];
}

void NeuronRBF::setU(int i, double aU)
{
	u[i] = aU;
}

double NeuronRBF::getU(int i) 
{
	return u[i];
}

void NeuronRBF::setS(double aS)
{
	s = aS;
}

double NeuronRBF::getS()
{
	return s;
}

void NeuronRBF::getInputs()
{
	for (int i = 0; i < nbrOfIn; i++)
		inputs[i] = preceedingLayer[i].getActivationLevel();
}

double NeuronRBF::Ran(double low, double high) {
	double x =  low + (double(rand())/double(RAND_MAX))*(high-low);
	return x;
}

