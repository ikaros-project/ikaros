#include "ikaros.h"
//#include "dynamixel_sdk.h"

using namespace ikaros;

class ForceCheck: public Module
{
    matrix present_current;
    matrix current_limit;
    matrix current_output;
    matrix present_position;// assumes degrees
    matrix goal_position_in;
    matrix goal_position_out;
    matrix previous_position;
    
    parameter gain_constant;
    parameter smooth_factor;
    parameter error_threshold;

    int tickCount;
    int current_increment;
    int current_value;
    int position_margin;
    int current_margin;
    int minimum_current;
   
    bool firstTick;
    bool obstacle;
    long obstacle_time;
    bool goal_reached;
    long start_time;
    int goal_time_out;

    double Tapering(double error, double threshold)
    {
        if (abs(error) < threshold)
            return sin(M_PI_2)*(abs(error)/threshold);
        else
            return 1;
    }
    
    matrix SetCurrent(matrix currents, matrix limits, matrix positions, matrix goal_position, int gain, int error_threshold, double smooth_factor)
    {
        for (int i = 0; i < currents.size(); i++) {
            int position = positions[i];
            if (position > 0) 
            {
                int current_value = abs(currents[i]);
                int limit_value = limits[i];
                int position = positions[i];
                int goal = goal_position[i];
                double error = abs(goal - position);
                double error_tapered = Tapering(error, error_threshold);
                int suggested_current = smooth_factor * current_value + (gain* error * error_tapered);
                current_output[i] = std::min(suggested_current , limit_value); // Cap at limit_value                 
            }        
             
        }
        
        return current_output;
}

    
    void ObstacleCheck(matrix positions,  matrix current_limits, matrix goal_position_in, matrix goal_position_out, bool goal_reached)
    {   
        for (int i = 0; i < positions.size(); i++) {
            float current_position = abs(positions[i]);
            float goal = goal_position_in[i];
            float current_value = abs(current_output[i]);
            float limit_value = current_limit[i];
            if (current_position >0){

                if (!obstacle && !goal_reached && current_value > limit_value*0.8 && std::time(nullptr)-start_time > goal_time_out)
                {   
                    
                    obstacle = true;
                    obstacle_time = std::time(nullptr);
                    goal_position_out[i] = current_position;
                    std::cout << "Goal position changed to " << current_position << std::endl;
                    
                }
                
            }
            else if(obstacle && abs(std::time(nullptr)- obstacle_time) > 3)
            {
                obstacle = false;
                std::cout << "Obstacle time: " << std::time(nullptr)-obstacle_time << std::endl;
            }


        }
    }

    bool GoalReached(matrix present_position, matrix goal_position, int margin)
    {
    
        for (int i = 0; i < present_position.size(); i++) {
            int position = present_position[i];
            int goal = goal_position[i];

            if (abs(position - goal) > margin)
                return false;
        }
        return true;
    }

    bool ApproximatingGoal(matrix present_position, matrix previous_position, matrix goal_position){
        for (int i =0; i < present_position.size(); i++){
            //Checking if distance to goal is decreasing
            if (abs(goal_position(i) - present_position(i)) < 0.97*abs(goal_position(i)-previous_position(i))){
                Notify(msg_debug, "Approximating Goal");
                return true;
            }
            else{
                Notify(msg_debug, "Not Approximating Goal");
                return false;
            }
        }
        return false;
    } 
    void Init()
    {
        Bind(present_current, "PresentCurrent");
        Bind(current_limit, "CurrentLimit");
        Bind(current_output, "CurrentOutput");
        Bind(present_position, "PresentPosition");
        Bind(goal_position_in, "GoalPositionIn");
        Bind(goal_position_out, "GoalPositionOut");

        Bind(gain_constant, "GainConstant");
        Bind(smooth_factor, "SmoothFactor");
        Bind(error_threshold, "ErrorThreshold");
      
    
    
        current_margin = 10;
        position_margin = 5;
        minimum_current = 15;
        start_time = std::time(nullptr);
        goal_time_out = 5;
        firstTick = true;
        obstacle = false;
        obstacle_time =0;
        goal_reached = false;
        previous_position.copy(present_position);
        previous_position.set_name("PreviousPosition");

    }


    void Tick()
    {   
        if (firstTick){
            goal_position_out.copy(goal_position_in);
        }

        if (present_position.connected() && present_current.connected() && goal_position_in.connected()) {
            if(ApproximatingGoal(present_position, previous_position, goal_position_in)){
                start_time = std::time(nullptr);
            }
            goal_reached = GoalReached(present_position, goal_position_in, position_margin);
            ObstacleCheck(present_position, current_limit, goal_position_in, goal_position_out, goal_reached);
            current_output = SetCurrent(present_current, current_limit, present_position, goal_position_in, gain_constant, error_threshold, smooth_factor);
        }
        else
        {
            Notify(msg_fatal_error, "Present position, present current and goal position must be connected");
            return;
        }

       
       
        firstTick=false;
        
        
        previous_position.copy(present_position);       
    }
};


INSTALL_CLASS(ForceCheck)

