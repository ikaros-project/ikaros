#include "ikaros.h"
using namespace ikaros;

class ServosSimulation: public Module
{
    matrix current;
    matrix current_limit;
    matrix current_output;
    matrix position;// assumes degrees
    matrix goal_position;

    int tickCount;
    int current_increment;
    int current_value;
    int position_margin;
    int position_increment;
    int current_margin;
    
    matrix SetCurrent(matrix currents, matrix limits, int increment)
    {
        for (int i = 0; i < currents.rows(); i++) {
                for (int j = 0; j < currents.cols(); j++) {
                    int current_value = currents(i, j);
                    int limit_value = limits(i, j);   
                    if ( current_value< limit_value) {
                        current_output(i, j) = std::min(current_value + increment, limit_value); // Cap at limit_value              
                    } else {
                        current_output(i, j)= current_output(i, j);
                    }
                }
        }
        return current_output;
    }

    //Could also be done by using dynamixel_sdk
    matrix SetPosition(matrix positions, matrix goal_positions, int increment)
    {
        for (int i = 0; i < positions.rows(); i++) {
                for (int j = 0; j < positions.cols(); j++) {
                    float current_position = positions(i,j);
                    float goal = goal_positions(i,j);
                    if (abs(current_position - goal) > position_margin )
                    {
                        positions(i,j) = std::min(current_position + increment, goal); 
                    }
                }
        }
        return positions;
    }

    void Init()
    {
        Bind(current, "PresentCurrent");
        Bind(current_limit, "CurrentLimit");
        Bind(current_output, "GoalCurrent");
        Bind(position, "PresentPosition");
        Bind(goal_position, "GoalPosition");
        tickCount =0;
        current_output.reset();
        current_increment = 20;
        position_increment = 10;
        current_margin = 100;
        position_margin = 5;

    }


    void Tick()
    {    
        if (tickCount > 0){  
            position = SetPosition(position, goal_position, position_increment);
            current_output = SetCurrent(current, current_limit, current_increment);
        }
        tickCount ++;
            
    }
};


INSTALL_CLASS(ServosSimulation)

