//
//	EpiHeadExp.h		This file is a part of the IKAROS project
//
//    Copyright (C) 2012 <Author Name>
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
//    See http://www.ikaros-project.org/ for more information.
//

#ifndef EpiHeadExp_
#define EpiHeadExp_

#define NUMBER_OF_TRIALS 24 // Number of times pupil will cange during a condition.
#define NUMBER_OF_SUBJECTS 40
#define NUMBER_OF_CONDITIONS 2
#define NUMBER_OF_EXPERIMENTS 4
#define NUMBER_OF_RANDOM_SCHEMAS 4

#define MOTOR_DATA_SIZE 10

#define WAIT_ON_NEW_SUBJECT 0


#include "IKAROS.h"
class Movement;

class EpiHeadExp: public Module
{
public:
    static Module * Create(Parameter * p) { return new EpiHeadExp(p); }

    EpiHeadExp(Parameter * p) : Module(p) {}
    virtual ~EpiHeadExp();

    void 		Init();
    void 		Tick();
    
//    bool        SystemStatus
	// Outputs
	float *     expID;
	float *     motorNeckEyes;
	float *     screenPupilL;
	float *     screenPupilR;
	float *     robotPupilL;
	float *     robotPupilR;

	float *		imageID;
	
	// Text to WebUI
	std::string infoText;
	
    // New Subject
    int newSubject;
	
	// Start Conditions
	int startExperiment;
	bool syncSingal;
	bool visibleFace;
	
	float * subjectBtnEnable;
	float * expBtnEnable;
	float * syncSingalEnable;

	float * enableFace;
	
	
	// Subject ID
	int subjectID = 0;
	
	// MARK: Random schemes
//	int randomExperiments[NUMBER_OF_SUBJECTS][NUMBER_OF_EXPERIMENTS] =
//	{
//		{3,2,1,0},
//		{0,1,3,2}
//	};

//	int randomTrails[NUMBER_OF_RANDOM_SCHEMAS][NUMBER_OF_TRIALS] =
//	{
//		{0,0},
//		{1,1},
//		{1,0},
//		{1,1}
//	};
	
		// Random Order of Experiments
		int randomExperiments[NUMBER_OF_SUBJECTS][NUMBER_OF_EXPERIMENTS] =
		{
			{3,2,1,0},
			{0,1,3,2},
			{3,0,2,1},
			{0,2,3,1},
			{3,0,1,2},
			{2,3,1,0},
			{0,3,2,1},
			{0,1,3,2},
			{2,3,1,0},
			{3,1,0,2},
			{0,1,2,3},
			{0,2,3,1},
			{3,1,2,0},
			{0,3,2,1},
			{2,3,1,0},
			{3,0,1,2},
			{1,3,0,2},
			{3,0,2,1},
			{2,3,0,1},
			{2,1,3,0},
			{2,1,0,3},
			{3,1,0,2},
			{2,1,0,3},
			{3,2,1,0},
			{3,0,2,1},
			{3,1,2,0},
			{0,3,2,1},
			{2,3,0,1},
			{1,3,2,0},
			{0,1,3,2},
			{3,1,0,2},
			{2,1,0,3},
			{1,2,0,3},
			{0,2,3,1},
			{0,2,1,3},
			{0,3,2,1},
			{3,2,0,1},
			{0,3,2,1},
			{1,0,3,2},
			{2,3,0,1}
		};

		int randomTrails[NUMBER_OF_RANDOM_SCHEMAS][NUMBER_OF_TRIALS] =
		{
			{0,0,1,1,0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1,1,0,1,0},
			{1,1,0,1,0,0,1,0,0,1,0,1,0,1,0,1,0,1,0,0,1,0,1,1},
			{1,0,1,1,0,1,0,1,1,0,1,0,1,0,1,0,1,0,0,1,0,1,0,0},
			{1,1,0,1,1,0,1,0,0,1,0,1,0,0,1,0,1,0,1,0,0,1,0,1}
		};

	int randomExperimentsAvailable[NUMBER_OF_SUBJECTS][NUMBER_OF_EXPERIMENTS];

	bool waitngOnInput = false;
	int nextExperiment = 100;
    //int expPhase = WAIT_ON_NEW_SUBJECT;
	
	// Current (index)
	int curSubjectIndex = -1;
	int curExperimentIndex = -1;
	int curExperimentCount = -1;
	int curExperimentID = -1;

	
	int curExpPhase = WAIT_ON_NEW_SUBJECT;
	
	// Current
	int curSubject = -1;
	
	// Trials
	int curConditionTrial = -1; 	// Subject's condition trials so far

	// Total
	int curTotalTrial = -1; 		// Experiment current

	// Random schema
	int curExperimentRandomID = -1;

	
    // Animations
	Movement * animation[NUMBER_OF_SUBJECTS][100]; // ORDER!!
    float * nextStep;
	
    // Log
    float * logWrite;
    float * logNewFile;
	
	float *logTick;
	// Subject
	float *logSubjectCount;
	// Experiment
	float *logExperimentTickIndex;
	float *logExperimentRandomizeIndex;
	float *logExperimentIdIndex;

	// Trial
	float *logTrialTickIndex;
	float *logTrialCountIndex;
	float *logTrialIdIndex;
	// Extra
	float *logExperimentPhase;
	float * logTrialPhase;
	
	// Data
	float * logScreenPupil;
	float * logRobotPupil;
	
	
	// Syncing
	bool syncing = false;
	int syncTick = 0;
	float *logSyncSignal;
	
	
	// Progress
	int expProgress = 0;
	int subjectProgress = 0;
	
    void writeToLogFile();
    void writeToNewLogFile();
    void resetWriteToLogFile();
};

#endif
