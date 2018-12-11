//
//	BackProp.cpp	This file is a part of the IKAROS project
//
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
//    along with this program; if not, BackProp to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
//    Stefan Winberg & Christian Balkenius

#include "BackProp.h"

using namespace ikaros;

Module* BackProp::Create(Parameter *pParam)
{
	return new BackProp(pParam);
}

BackProp::~BackProp()
{
	destroy_array(m_pInputData_v);
	destroy_array(m_pHiddenActivation_v);
	destroy_array(m_pHiddenErrors_v);
	destroy_array(m_pOutputErrors_v);
	destroy_array(m_pTrainOutput_v);
	destroy_matrix(m_pInHiddenWeight_m);
	destroy_matrix(m_pInHiddenMomentum_m);
	destroy_matrix(m_pHiddenOutWeight_m);
	destroy_matrix(m_pHiddenOutMomentum_m);
}


BackProp::BackProp(Parameter *pParam): Module(pParam)
{
/*
    AddInput("IN_INPUT");
	AddInput("IN_TRAIN_INPUT");
	AddInput("IN_TARGET");
	
	AddOutput("OUT_OUTPUT");
	
	m_flLearningRate = GetFloatValue("lr");
	m_flMomentum = GetFloatValue("momentum");
*/
}

void BackProp::SetSizes()
{
    m_iHiddenUnits = GetIntValue("hidden_units");
    m_iInputLength = GetInputSize("INPUT");
	m_iOutputLength = GetInputSize("T_TARGET");
	
	if(m_iInputLength != unknown_size &&
		m_iOutputLength != unknown_size)
	{
		SetOutputSize("OUTPUT", m_iOutputLength);
		
		// Add bias node and verify input
		m_iInputUnits  = m_iInputLength + 1;
		if(m_iHiddenUnits == unknown_size)
		{
			m_iHiddenUnits = (int)max((m_iInputLength + m_iOutputLength) / 3.0f + 1.0f, 3.0f);
		}
	}
    
    SetOutputSize("ERROR", 1);
}

void BackProp::Init()
{
    m_flLearningRate = GetFloatValue("lr");
    m_flMomentum = GetFloatValue("momentum");
    
    m_pInInput_v		= GetInputArray("INPUT");
	m_pInTrainInput_v   = GetInputArray("T_INPUT");
	m_pInTarget_v		= GetInputArray("T_TARGET");
	m_pOutOutput_v		= GetOutputArray("OUTPUT");
    error               = GetOutputArray("ERROR");
	
	// Initialize temporary storage arrays
	m_pInputData_v		  = create_array(m_iInputUnits);
	m_pHiddenActivation_v = create_array(m_iHiddenUnits);
	m_pHiddenErrors_v	  = create_array(m_iHiddenUnits);
	m_pOutputErrors_v	  = create_array(m_iOutputLength);
	m_pTrainOutput_v	  = create_array(m_iOutputLength);
	
	// Initialize links
	int i, h, o;
	
	m_pInHiddenWeight_m	   = create_matrix(m_iHiddenUnits, m_iInputUnits);
	m_pInHiddenMomentum_m  = create_matrix(m_iHiddenUnits, m_iInputUnits);
	m_pHiddenOutWeight_m   = create_matrix(m_iOutputLength, m_iHiddenUnits);
	m_pHiddenOutMomentum_m = create_matrix(m_iOutputLength, m_iHiddenUnits);
	
	for(h = 0; h < m_iHiddenUnits; h++)
	{
		for(i = 0; i < m_iInputUnits; i++)
			m_pInHiddenWeight_m[i][h] = random(-0.1f, 0.1f);
		
		for(o = 0; o < m_iOutputLength; o++)
			m_pHiddenOutWeight_m[h][o] = random(-0.1f, 0.1f);
	}
}

void BackProp::Tick()
{
	AddBias(m_pInInput_v, m_pInputData_v);
	Ask(m_pInputData_v, m_pOutOutput_v);
	
	AddBias(m_pInTrainInput_v, m_pInputData_v);
	Ask(m_pInputData_v, m_pTrainOutput_v);
	Train(m_pInputData_v, m_pTrainOutput_v, m_pInTarget_v);
    
    // Calculate total output error
    *error = dist(m_pTrainOutput_v, m_pInTarget_v, m_iOutputLength);
}

void BackProp::AddBias(float *pStimInput_v, float *pBiasedData_v)
{
	int i;
	
	pBiasedData_v[0] = 1.0f;
	
	for(i = 0; i < m_iInputLength; i++)
		pBiasedData_v[i + 1] = pStimInput_v[i];
}

void BackProp::Ask(float *pInput_v, float *pOutput_v)
{
	int h, i, o;
	
	// Propagate the activations from the input layer to the hidden layer
	m_pHiddenActivation_v[0] = 1.0f;	// Threshold unit
	
	for(h = 1; h < m_iHiddenUnits; h++)
	{
		m_pHiddenActivation_v[h] = 0;	// Used as temp storage
		for(i = 0; i < m_iInputUnits; i++)
			m_pHiddenActivation_v[h] += m_pInHiddenWeight_m[i][h] * pInput_v[i];
		
		m_pHiddenActivation_v[h] = (float)(1.0f / (1.0f + exp(-m_pHiddenActivation_v[h])));
	}
	
	// Propagate the activations from the hidden layer to the output layer
	for(o = 0; o < m_iOutputLength; o++)
	{
		pOutput_v[o] = 0;	// Used as temp storage
		for(h = 0; h < m_iHiddenUnits; h++)
			pOutput_v[o] += m_pHiddenOutWeight_m[h][o] * m_pHiddenActivation_v[h];
		
		pOutput_v[o] = (float)(1.0f / (1.0f + exp(-pOutput_v[o])));
	}
}

void BackProp::Train(float *pInput_v, float *pOutput_v, float *pTarget_v)
{
	int	  h, i, o;
	
	float flErrorSum;
	
	// Compute the errors of the units in the output layer
	for(o = 0; o < m_iOutputLength; o++)
		m_pOutputErrors_v[o] = pOutput_v[o] * (1.0f - pOutput_v[o]) * (pTarget_v[o] - pOutput_v[o]); // Error = 0 when response is 1 or 0
	
	// Compute the errors of the units in the hidden layer
	for(h = 1; h < m_iHiddenUnits; h++)	// There are no links from the input layer to the threshold unit
	{
		flErrorSum = 0;	// Error sum of the output layer for this hidden node
		for(o = 0; o < m_iOutputLength; o++)
			flErrorSum += m_pOutputErrors_v[o] * m_pHiddenOutWeight_m[h][o];
		
		m_pHiddenErrors_v[h] = m_pHiddenActivation_v[h] * (1.0f - m_pHiddenActivation_v[h]) * flErrorSum;
	}
	
	// Adjust the weights between the hidden layer and the output layer
	for(h = 0; h < m_iHiddenUnits; h++)
	{
		if(m_pHiddenActivation_v[h] != 0)
		{
			for(o = 0; o < m_iOutputLength; o++)
			{
				m_pHiddenOutMomentum_m[h][o] = m_flLearningRate * m_pOutputErrors_v[o] * m_pHiddenActivation_v[h] + m_flMomentum * m_pHiddenOutMomentum_m[h][o];
				m_pHiddenOutWeight_m[h][o] += m_pHiddenOutMomentum_m[h][o];
			}
		}
	}
	
	// Adjust the weights between the input layer and the hidden layer
	for(i = 0; i < m_iInputUnits; i++)
	{
		if(pInput_v[i] > 0)
		{
			for(h = 1; h < m_iHiddenUnits; h++)	// There are no links from the input layer to the threshold unit
			{
				m_pInHiddenMomentum_m[i][h] = m_flLearningRate * m_pHiddenErrors_v[h] * pInput_v[i] + m_flMomentum * m_pInHiddenMomentum_m[i][h];
				m_pInHiddenWeight_m[i][h] += m_pInHiddenMomentum_m[i][h];
			}
		}
	}
}

static InitClass init("BackProp", &BackProp::Create, "Source/Modules/ANN/BackProp/");
