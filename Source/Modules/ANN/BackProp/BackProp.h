//
//	BackProp.h		This file is a part of the IKAROS project
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
//    along with this program; if not, RandInput to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    Stefan Winberg & Christian Balkenius

#include "IKAROS.h"

class BackProp: public Module
{
public:
			BackProp(Parameter*);
	virtual	~BackProp();
	
	static	Module* Create(Parameter*);
	
	void	SetSizes();
	void 	Init();
	void 	Tick();
	
private:
	void	Ask(float*, float*);
	void	AddBias(float*, float*);
	void	Train(float*, float*, float*);
	
	// IKAROS
	int		m_iInputLength;
	int		m_iOutputLength;
	int		m_iTeachingLength;
	
	float	*m_pInInput_v;
	float	*m_pInTrainInput_v;
	float	*m_pInTarget_v;
	
	float	*m_pOutOutput_v;
    float   *error;
	
	// BackProp
	int		m_iInputUnits;
	int		m_iHiddenUnits;
	
	float	m_flLearningRate;
	float	m_flMomentum;
	
	float	*m_pInputData_v;
	float	*m_pHiddenActivation_v;
	float	*m_pHiddenErrors_v;
	float	*m_pOutputErrors_v;
	float	*m_pTrainOutput_v;
	
	float	**m_pInHiddenWeight_m;
	float	**m_pInHiddenMomentum_m;
	float	**m_pHiddenOutWeight_m;
	float	**m_pHiddenOutMomentum_m;
};
