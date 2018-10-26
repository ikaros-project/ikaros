//
//	EpiHeadExp.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2018 Birger Johansson
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
//  This example is intended as a starting point for writing new Ikaros modules
//  The example includes most of the calls that you may want to use in a module.
//  If you prefer to start with a clean example, use he module MinimalModule instead.
//

#include "EpiHeadExp.h"

#define EXP_DEBUG

// PHASES
#define WAIT_ON_EXPERIMENT 10
#define STATIC_ON_SCREEN 20
#define ONGOING_EXPERIMENT 30
#define EXPERIMENT_ENDED 40

// INDEX For looing
#define SCREEN_EYE_ID 0
//#define SCREEN_HEAD_INDEX 1
#define SCREEN_MOVIE_ID 2
#define ROBOT_ID 3

// EXPERIMENT
#define TICK_INDEX 0 				// Tick only used for debugging (not real tick)
// Subject
#define SUBJECT_COUNT_INDEX 1 		// Subject_Count. Number of subject = subject id. [0,1,2,3,3,4 -> NUMBER_OF_SUBJECT]
// Experiment
#define EXPERIMENT_TICK_INDEX 2 	// Experiment_Tick. Tick of the experiment. [1->EXPERIMENT_TIME]
#define EXPERIMENT_COUNT_INDEX 3 	// Number of experiments done. Same as EXPERIMENT_ID As the order is set and the index of experiment_id 0-3
#define EXPERIMENT_ID_INDEX 4		// Experiment id 0-3
// Trial
#define TRIAL_TICK_INDEX 5			// Trial_Tick. Tick of each trial. One experiments = NUMBER_OR_TRAILS [1,2,3,4 -> TOTAL_TIME]
#define TRIAL_COUNT_INDEX 6			// Trial_Count Counting each trail [000, 111,222,333 -> NUMBER_OR_TRAILS]
#define TRIAL_ID_INDEX 7			// Trial_id	Condition of the trial. 0 = Small 1. Large
// Data
#define PUPIL_INDEX 8 				// Uses the same channel for both robot and animation.
// Extra
#define TRIAL_PHASE_INDEX 9			// Phases during trial. wait change hold etc.


// TRIAL PHASES
#define WAIT_PHASE 0
#define CHANGE_PHASE 1
#define DISTRACTOR_PHASE 2
#define HOLD_PHASE 3
#define RETURN_PHASE 4

// TIME
#define TICK_BASE 20 // 20
#define WAIT_TIME (6*TICK_BASE) // Make it even with wait. (Float to int)
#define CHANGE_TIME (1.5*TICK_BASE)
#define HOLD_TIME (1*TICK_BASE)
#define RETURN_TIME (1.5*TICK_BASE)
#define ROBORT_DISTRACTOR_TIME (0.5*TICK_BASE)
#define TOTAL_TIME (WAIT_TIME+CHANGE_TIME+HOLD_TIME+RETURN_TIME )
#define EXPERIMENT_TIME (TOTAL_TIME*NUMBER_OF_TRIALS)

#define MOVIE_FPS 25
// PUPIL SIZES
// 0.5 == 16.8
//
#define PUPIL_SIZE_SCREEN_LOW         (6.0/32.0) // 6mm
#define PUPIL_SIZE_SCREEN_HIGH        (14.0/32.0) // 14mm
#define PUPIL_SIZE_SCREEN_NORMAL      (10.0/32.0) // 10mm
#define PUPIL_SIZE_ROBOT_LOW          280 // 6mm
#define PUPIL_SIZE_ROBOT_HIGH         237 // 14mm
#define PUPIL_SIZE_ROBOT_NORMAL       255 // 10mm

// HW limit set to 190 - 270 degrees


#define WHITE_PUPIL_SIZE_ADJUSTER   -0
#define BLUE_PUPIL_SIZE_ADJUSTER     0
#define PUPIL_SIZE_SCREEN_ADJUSTER    WHITE_PUPIL_SIZE_ADJUSTER
#define PUPIL_SIZE_ROBOT_ADJUSTER     -6

#define SCREEN_DISTRACTOR_ON 0
#define SCREEN_DISTRACTOR_OFF 1

#define ROBOT_DISTRACTOR_ON 0
#define ROBOT_DISTRACTOR_OFF 180

// Movments

// Look at subject (Waiting on marker)
#define TILT_SUBJECT    165
#define PAN_SUBJECT     180
#define EYE_SUBJECT     180


// LOG
#define LOG_NORMAL 0
#define LOG_SMALL 1
#define LOG_LARGE 2


using namespace ikaros;

class Movement
{
public:
	
	float * nextStep();
	float * pauseMovment();
	int length; // y
	
	Movement(float ** d, int sx, int sy);
	void start();
	void printStatus();
	void printData();
	
	void pause();
	void unpause();
	void loop();
	void unloop();
	float * GetNextStep();
	~Movement();
	
	int step;
	bool finnished;
	
private:
	bool started;
	bool paused;
	bool looping;
	float ** data;
	int sizeX;
	int sizeY;
};


Movement::Movement(float ** d, int sx, int sy)
{
#ifdef EXP_DEBUG
	printf("Creating movement\n");
#endif
	this->length = sx;
	this->sizeX = sx;
	this->sizeY = sy;
	this->step = 0;
	this->started = true;
	data = create_matrix(sx, sy);
	copy_matrix(data, d, sx, sy);
};

void Movement::start()
{
	this->step = 0;
	this->started = true;
	this->finnished = false;
}

void Movement::printStatus()
{
	printf("Step %i (%i, %i), started %i, finnished %i, paused %i, looping %i\n", this->step, this->sizeX, this->sizeY, this->started,this->finnished,this->paused,this->looping);
}

void Movement::printData()
{
	this->printStatus();
	print_matrix("Animation data", this->data, this->sizeX, this->sizeY);
}


float * Movement::GetNextStep()
{
	this->started = false; // if we are in next step we are not in start any more.
	
	if (this->paused || (this->finnished && !this->looping))
		return this->data[this->step]; // return previous step
	
	if (this->finnished && this->looping)
		this->start();
	else
		this->step++;
	
	if (this->step == this->sizeY-1) // End of movement
	{
		this->finnished = true;
	}
	return this->data[this->step];
}

void Movement::pause()
{
	this->paused = true;
}
void Movement::unpause()
{
	this->paused = false;
}
void Movement::loop()
{
	this->looping = true;
}
void Movement::unloop()
{
	this->looping = false;
}


Movement::~Movement()
{
#ifdef EXP_DEBUG
	printf("Deleteing movement\n");
#endif
	destroy_matrix(this->data);
};


void
EpiHeadExp::Init()
{
	// Buttons
	Bind(s1, "s1");
	Bind(c1, "c1");
	Bind(c2, "c2");
	Bind(c3, "c3");
	Bind(c4, "c4");
	Bind(visibleFace, "visibleFace");
	Bind(subjectID, "subject_id");
	Bind(infoText,"info_text");
	infoText = "";
	
	// Button enable output
	s1Enable = GetOutputArray("S1Enable");
	c1Enable = GetOutputArray("C1Enable");
	c2Enable = GetOutputArray("C2Enable");
	c3Enable = GetOutputArray("C3Enable");
	c4Enable = GetOutputArray("C4Enable");
	
	// Outputs (Motor)
	screenPupilL = GetOutputArray("SCREEN_PUPIL_LEFT");
	screenPupilR = GetOutputArray("SCREEN_PUPIL_RIGHT");
	robotPupilL = GetOutputArray("ROBOT_PUPIL_LEFT");
	robotPupilR = GetOutputArray("ROBOT_PUPIL_RIGHT");
	
	motorNeckEyes = GetOutputArray("NECKEYES");
	
	// Image choser
	imageID = GetOutputArray("IMAGE_ID");

	// LOG
	logWrite = GetOutputArray("LOG_WRITE");
	logNewFile = GetOutputArray("LOG_NEW_FILE");
	// Subject
	logSubjectCount = GetOutputArray("LOG_SUBJECT");
	// Experiment
	logExperimentTickIndex = GetOutputArray("LOG_EXPERIMENT_TICK");
	logExperimentCountIndex = GetOutputArray("LOG_EXPERIMENT_COUNT");
	logExperimentIdIndex = GetOutputArray("LOG_EXPERIMENT_ID");
	// Trial
	logTrialTickIndex = GetOutputArray("LOG_TRIAL_TICK");
	logTrialCountIndex = GetOutputArray("LOG_TRIAL_COUNT");
	logTrialIdIndex = GetOutputArray("LOG_TRIAL_ID");
	// Extra
	logExperimentPhase = GetOutputArray("LOG_EXPERIMENT_PHASE");
	logTrialPhase = GetOutputArray("LOG_TRIAL_PHASE");
	// Data (Pupil converted to mm)
	logScreenPupil = GetOutputArray("LOG_SCREEN_PUPIL");
	logRobotPupil = GetOutputArray("LOG_ROBOT_PUPIL");
	
	// Reset
	*logScreenPupil = 0;
	*logRobotPupil = 0;
	
	
	curExpPhase = WAIT_ON_NEW_SUBJECT;
	
	subjectID = 0;
	
	////////////////////////////////////////////////////////////////
	
	
	// Setting all experiments to available
	for (int i = 0; i < NUMBER_OF_SUBJECTS; i++)
		for (int j = 0; j < NUMBER_OF_EXPERIMENTS; j++)
			randomExperimentsAvailable[i][j] = true;
	
	
	float **** motionData  = create_matrix(MOTOR_DATA_SIZE, EXPERIMENT_TIME, NUMBER_OF_EXPERIMENTS, NUMBER_OF_SUBJECTS); // ORDER!!
	
	for (int sIndex = 0; sIndex < NUMBER_OF_SUBJECTS; sIndex++)
		for (int curTimeStepExp = 0; curTimeStepExp < EXPERIMENT_TIME; curTimeStepExp++)
			for (int expIndex = 0; expIndex < NUMBER_OF_EXPERIMENTS; expIndex++)
				for (int typIndex = 0; typIndex < MOTOR_DATA_SIZE; typIndex++)
					motionData[sIndex][expIndex][curTimeStepExp][typIndex] = -1;
	
	
	// Not randomize Experiment order for each subject
	// Randomize Trail
	
	int tick = 0;
	int sTick = 0;
	int eTick = 0;
	int tTick = 0;
	
	int phase = -1;
	for (int sIndex = 0; sIndex < (NUMBER_OF_SUBJECTS); sIndex++)
	{
		for (int eIndex = 0; eIndex < (NUMBER_OF_EXPERIMENTS); eIndex++)
		{
			for (int tIndex = 0; tIndex < (NUMBER_OF_TRIALS); tIndex++)
			{
				int pWaitTick = 0;
				int pChangeTick = 0;
				int pHoldTick = 0;
				int pReturnTick = 0;
				
				for (int tIIndex = 0; tIIndex < (TOTAL_TIME); tIIndex++)
				{
					// Figure out phase
					if (tTick < WAIT_TIME)
					{
						phase = WAIT_PHASE;
						pWaitTick++;
					}
					else if (tTick >= WAIT_TIME && tTick < (WAIT_TIME + CHANGE_TIME))
					{
						phase = CHANGE_PHASE;
						pChangeTick++;
					}
					else if (tTick >= WAIT_TIME + CHANGE_TIME && tTick < (WAIT_TIME + CHANGE_TIME + HOLD_TIME))
					{
						phase = HOLD_PHASE;
						pHoldTick++;
					}
					else if (tTick >= WAIT_TIME + CHANGE_TIME + HOLD_TIME && tTick < (WAIT_TIME + CHANGE_TIME + HOLD_TIME + RETURN_TIME))
					{
						phase = RETURN_PHASE;
						pReturnTick++;
					}
					else
						phase = -1;
					
					// Common for all phases
					motionData[sIndex][eIndex][eTick][TICK_INDEX] 				= tick;
					motionData[sIndex][eIndex][eTick][SUBJECT_COUNT_INDEX] 		= sIndex;
					
					motionData[sIndex][eIndex][eTick][EXPERIMENT_TICK_INDEX] 	= eTick;
					//motionData[sIndex][eIndex][eTick][EXPERIMENT_COUNT_INDEX] 	= randomExperiments[sIndex][eIndex]; // Same as id. See define note
					//motionData[sIndex][eIndex][eTick][EXPERIMENT_ID_INDEX] 		= randomExperiments[sIndex][eIndex];
					motionData[sIndex][eIndex][eTick][EXPERIMENT_COUNT_INDEX] 	= -1; // We do not know the order
					motionData[sIndex][eIndex][eTick][EXPERIMENT_ID_INDEX] 		= eIndex;

					
					motionData[sIndex][eIndex][eTick][TRIAL_TICK_INDEX] 		= tTick;
					motionData[sIndex][eIndex][eTick][TRIAL_COUNT_INDEX] 		= tIndex;
					// Chose a proper random trial schema. Just rotate the schame
					motionData[sIndex][eIndex][eTick][TRIAL_ID_INDEX] 			= randomTrails[((eIndex+sIndex)%NUMBER_OF_RANDOM_SCHEMAS)][tIndex]; // All condition for the complete trial
					
					// Motion in phases
					int trailCondition = motionData[sIndex][eIndex][eTick][TRIAL_ID_INDEX];
					int experimentID = motionData[sIndex][eIndex][eTick][EXPERIMENT_ID_INDEX]; // Robot or screen
					
					float dataPupilSize = -1;
					switch (phase) {
						case WAIT_PHASE:
						{
							if (experimentID != ROBOT_ID)
								dataPupilSize = PUPIL_SIZE_SCREEN_NORMAL;
							else
								dataPupilSize = PUPIL_SIZE_ROBOT_NORMAL;
							
						}
							break;
						case CHANGE_PHASE:
						{
							
							// Screen or Robot?
							if (experimentID != ROBOT_ID)
							{
								if (trailCondition == 0)					// Smaller pupills
									dataPupilSize = PUPIL_SIZE_SCREEN_LOW;
								else
									dataPupilSize = PUPIL_SIZE_SCREEN_HIGH;
								dataPupilSize = PUPIL_SIZE_SCREEN_NORMAL + pChangeTick * (dataPupilSize - PUPIL_SIZE_SCREEN_NORMAL)/CHANGE_TIME;
							}
							else
							{
								if (trailCondition == 1)
									dataPupilSize = PUPIL_SIZE_ROBOT_LOW;
								else
									dataPupilSize = PUPIL_SIZE_ROBOT_HIGH;
								dataPupilSize = PUPIL_SIZE_ROBOT_NORMAL + pChangeTick * (dataPupilSize - PUPIL_SIZE_ROBOT_NORMAL)/CHANGE_TIME;
							}
							
						}
							break;
						case HOLD_PHASE:
						{
							// Screen or Robot?
							if (experimentID != ROBOT_ID)
							{
								if (trailCondition == 0)					// Smaller pupills
									dataPupilSize = PUPIL_SIZE_SCREEN_LOW;
								else
									dataPupilSize = PUPIL_SIZE_SCREEN_HIGH;
								dataPupilSize = dataPupilSize;
							}
							else
							{
								if (trailCondition == 1)
									dataPupilSize = PUPIL_SIZE_ROBOT_LOW;
								else
									dataPupilSize = PUPIL_SIZE_ROBOT_HIGH;
								dataPupilSize = dataPupilSize;
							}
							
						}
							break;
						case RETURN_PHASE:
						{
							// Screen or Robot?
							if (experimentID != ROBOT_ID)
							{
								if (trailCondition == 0)					// Smaller pupills
									dataPupilSize = PUPIL_SIZE_SCREEN_LOW;
								else
									dataPupilSize = PUPIL_SIZE_SCREEN_HIGH;
								dataPupilSize = PUPIL_SIZE_SCREEN_NORMAL + (RETURN_TIME-pReturnTick) * (dataPupilSize - PUPIL_SIZE_SCREEN_NORMAL)/RETURN_TIME;
							}
							else
							{
								if (trailCondition == 1)
									dataPupilSize = PUPIL_SIZE_ROBOT_LOW;
								else
									dataPupilSize = PUPIL_SIZE_ROBOT_HIGH;
								dataPupilSize = PUPIL_SIZE_ROBOT_NORMAL + (RETURN_TIME-pReturnTick) * (dataPupilSize - PUPIL_SIZE_ROBOT_NORMAL)/RETURN_TIME;
							}
						}
							break;
						default:
							break;
					}
					
					// Store values
					motionData[sIndex][eIndex][eTick][PUPIL_INDEX] = dataPupilSize;
					motionData[sIndex][eIndex][eTick][TRIAL_PHASE_INDEX] = phase;
					
					// Ticking
					tick++;
					sTick++;
					eTick++;
					tTick++;
				}
				tTick = 0;
			}
			eTick = 0;
		}
		sTick = 0;
	}
	
	//Print all motion for all subject
	//	for (int sIndex = 0; sIndex<NUMBER_OF_SUBJECTS; sIndex++) {
	//		printf("Subject %i:\n", sIndex);
	//		for (int eIndex = 0; eIndex<NUMBER_OF_EXPERIMENTS; eIndex++)
	//			print_matrix("Exp Data", motionData[sIndex][eIndex], MOTOR_DATA_SIZE, EXPERIMENT_TIME,0);
	//	}
	
	// Adding motions
	for (int sIndex = 0; sIndex<NUMBER_OF_SUBJECTS; sIndex++) {
		animation[sIndex][0] = new Movement(motionData[sIndex][0], MOTOR_DATA_SIZE, EXPERIMENT_TIME);
		animation[sIndex][1] = new Movement(motionData[sIndex][1], MOTOR_DATA_SIZE, EXPERIMENT_TIME);
		animation[sIndex][2] = new Movement(motionData[sIndex][2], MOTOR_DATA_SIZE, EXPERIMENT_TIME);
		animation[sIndex][3] = new Movement(motionData[sIndex][3], MOTOR_DATA_SIZE, EXPERIMENT_TIME);
		//		animation[sIndex][0]->printData();
		//		animation[sIndex][1]->printData();
		//		animation[sIndex][2]->printData();
		//		animation[sIndex][3]->printData();
	}
}


void
EpiHeadExp::Tick()
{
	writeToLogFile();
	
	// Main loop
	switch (curExpPhase) {
			// MARK: WAIT_ON_NEW_SUBJECT
		case WAIT_ON_NEW_SUBJECT:
		{
			infoText = "Waiting on new subject";
			nextStep = NULL; // No movement and no output
			
			// Buttons
			*c1Enable = *c2Enable = *c3Enable = *c4Enable = 0;
			*s1Enable = 1;
			
			if (s1 == 1)
			{
				curExpPhase = WAIT_ON_EXPERIMENT;
				curSubjectIndex = subjectID;  		// The subject ID chosen from webUI
			}
			
			if (curSubjectIndex > NUMBER_OF_SUBJECTS-1)
			{
				*c1Enable = *c2Enable = *c3Enable = *c4Enable = *s1Enable = 0;
				infoText = "All subjects done!";
				Notify(msg_terminate,"All subjects done!");
			}
			break;
		}
			
			// MARK: CHOSE CONDITION
		case WAIT_ON_EXPERIMENT:
		{
			// Disable buttons
			*c1Enable = *c1Enable = *c2Enable = *c3Enable = *c4Enable = 0;
			nextStep = NULL; // No movement and no output

			if (!waitngOnInput)
			{
				curExperimentIndex = -1;
				curExperimentCount = -1;
				infoText = "Pick one experiment";
				
				// Figure out the next experiment
				for (int i = 0; i < NUMBER_OF_EXPERIMENTS; i++)
				{
					if (randomExperimentsAvailable[curSubjectIndex][i] and randomExperiments[curSubjectIndex][i] < nextExperiment)
					{
						nextExperiment = randomExperiments[curSubjectIndex][i];
						curExperimentIndex = i;
					}
				}
				
				// No more experiments
				if (nextExperiment == 100)
				{
					curExpPhase = EXPERIMENT_ENDED;
					waitngOnInput = false;
					printf("Subject %i: No more experiments\n",curSubjectIndex);
				}
				else
				{
					randomExperimentsAvailable[curSubjectIndex][curExperimentIndex] = false; // Disable this experiment from list
					curExperimentCount = nextExperiment; // Number of experiments for subject
					
					waitngOnInput = true;
					printf("Subject %i: Next experiment %i count %i\n",curSubjectIndex, curExperimentIndex, curExperimentCount);
				}
			}
			
			// Only enable right Condition
			switch (curExperimentIndex) {
				case 0:
					*c1Enable = 1;
					break;
				case 1:
					*c2Enable = 1;
					break;
				case 2:
					*c3Enable = 1;
					break;
				case 3:
					*c4Enable = 1;
					break;
				default:
					break;
			}
			
			if (c1 == 1 or c2 == 1 or c3 == 1 or c4 == 1 )
			{
				curExpPhase = ONGOING_EXPERIMENT;
				waitngOnInput = false;
				nextExperiment = 100;
				curExperimentTrial++;
			}
			break;
		}
		case ONGOING_EXPERIMENT:
		{
			// MARK: ONGOING_EXPERIMENT
			infoText = "Running experiment";
			nextStep = animation[curSubjectIndex][curSubjectIndex]->GetNextStep();
			
			// Check if phase is done
			if (animation[curSubjectIndex][curSubjectIndex]->finnished)
			{
				curExpPhase = WAIT_ON_EXPERIMENT;
				animation[curSubjectIndex][curSubjectIndex]->start();
				waitngOnInput = false;
			}
			break;
		}
		case EXPERIMENT_ENDED:
		{
			// MARK: EXPERIMENT_ENDED
			infoText = "Experiment Done";
			curExpPhase = WAIT_ON_NEW_SUBJECT;
			break;
		}
		default:
			break;
	}
	// Send Normal
	if (!nextStep)
	{
		motorNeckEyes[0]   	= TILT_SUBJECT;
		motorNeckEyes[1]   	= PAN_SUBJECT;
		motorNeckEyes[2]   	= EYE_SUBJECT;
		motorNeckEyes[3]   	= EYE_SUBJECT;
		*screenPupilL 	    = PUPIL_SIZE_SCREEN_NORMAL;
		*screenPupilR 	   	= PUPIL_SIZE_SCREEN_NORMAL + PUPIL_SIZE_SCREEN_ADJUSTER;
		*robotPupilL      	= PUPIL_SIZE_ROBOT_NORMAL;
		*robotPupilR    	= PUPIL_SIZE_ROBOT_NORMAL + PUPIL_SIZE_ROBOT_ADJUSTER;
		*imageID			 = 0;
	}
	else
	{
		motorNeckEyes[0]   = TILT_SUBJECT;
		motorNeckEyes[1]   = PAN_SUBJECT;
		motorNeckEyes[2]   = EYE_SUBJECT;
		motorNeckEyes[3]   = EYE_SUBJECT;
		
		// Override
		
		curExperimentIndex = ROBOT_ID;
		if (curExperimentIndex != ROBOT_ID) // Which output should we send on?
		{
			*screenPupilL    = nextStep[PUPIL_INDEX];
			*screenPupilR    = nextStep[PUPIL_INDEX] + PUPIL_SIZE_SCREEN_ADJUSTER;
			*robotPupilL     = PUPIL_SIZE_ROBOT_NORMAL;
			*robotPupilR     = PUPIL_SIZE_ROBOT_NORMAL + PUPIL_SIZE_ROBOT_ADJUSTER;
		}
		else
		{
			*screenPupilL    = PUPIL_SIZE_SCREEN_NORMAL;
			*screenPupilR    = PUPIL_SIZE_SCREEN_NORMAL + PUPIL_SIZE_SCREEN_ADJUSTER;
			*robotPupilL     = nextStep[PUPIL_INDEX];
			*robotPupilR     = nextStep[PUPIL_INDEX] + PUPIL_SIZE_ROBOT_ADJUSTER;
		}
		
		// Image output
		if (curExperimentIndex == SCREEN_MOVIE_ID) // Which output should we send on?
			*imageID	= int(((curExperimentCount*EXPERIMENT_TIME)+nextStep[EXPERIMENT_TICK_INDEX])/2); // Divade this with something good
		else
			*imageID 	= 0;
		
		*imageID 	= GetTick()%220;
		
		// Eye/head paramter
		if (curExperimentIndex == SCREEN_EYE_ID) // Which output should we send on?
			visibleFace = true;
		else
			visibleFace = false;

		visibleFace = false;

	}
	*logWrite = 1;
	// Log
	if (!nextStep)
	{
		// EXPERIMENT
		// Subject
		*logSubjectCount = -1.0;
		// Experiment
		*logExperimentTickIndex = -1.0;
		*logExperimentCountIndex = -1.0;
		*logExperimentIdIndex = -1.0;
		// Trial
		*logTrialTickIndex = -1.0;
		*logTrialCountIndex = -1.0;
		*logTrialIdIndex = -1.0;
		// Extra
		*logExperimentPhase = curExpPhase; 	// Logging phases // Will get an end signal. Maybe usefull.
		*logTrialPhase = -1; 				// Logging phases
		
		*logScreenPupil = -1;				// Formated output instead of RAW
		*logRobotPupil = -1;
	}
	else
	{
		// EXPERIMENT
		// Subject
		*logSubjectCount = nextStep[SUBJECT_COUNT_INDEX];
		// Experiment
		*logExperimentTickIndex  = nextStep[EXPERIMENT_TICK_INDEX];
		*logExperimentCountIndex  = nextStep[EXPERIMENT_COUNT_INDEX];
		*logExperimentIdIndex  = nextStep[EXPERIMENT_ID_INDEX];
		// Trial
		*logTrialTickIndex  = nextStep[TRIAL_TICK_INDEX];
		*logTrialCountIndex  = nextStep[TRIAL_COUNT_INDEX];
		*logTrialIdIndex  = nextStep[TRIAL_ID_INDEX];
		// Extra
		*logExperimentPhase = curExpPhase;				// Logging phases // Will get an end signal. Maybe usefull.
		*logTrialPhase = nextStep[TRIAL_PHASE_INDEX]; 	// Logging phases
		
		// Format output. Uses only left eye
		if (curExperimentIndex != ROBOT_ID)
		{
			*logScreenPupil = *screenPupilL*32.0f;
			*logRobotPupil = -1;
		}
		else
		{
			*logScreenPupil = -1;
			*logRobotPupil = *screenPupilL*32.0f; // inverting Assuming both robot and screen is identical
		}
	}
}



EpiHeadExp::~EpiHeadExp()
{
#ifdef EXP_DEBUG
	printf("Deleteing memory\n");
#endif
};

void EpiHeadExp::writeToLogFile()
{
	*logWrite = 1;
	*logNewFile = 0;
}
void EpiHeadExp::writeToNewLogFile()
{
	*logWrite = 1;
	*logNewFile = 1;
}
void EpiHeadExp::resetWriteToLogFile()
{
	*logWrite = 0;
	*logNewFile = 0;
}

// Install the module. This code is executed during start-up.
static InitClass init("EpiHeadExp", &EpiHeadExp::Create, "Source/UserModules/2018/AdjustablePupilInRobots/EpiHeadExp/");
