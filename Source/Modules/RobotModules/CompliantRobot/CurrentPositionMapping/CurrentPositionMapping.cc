#include "ikaros.h"
#include <random> // Include the header file that defines the 'Random' function


using namespace ikaros;

class CurrentPositionMapping: public Module
{
  
    matrix present_position;
    matrix present_current;
    matrix position_transitions;
    matrix previous_position;
    matrix start_position;
    
    matrix goal_current;
    matrix goal_positions_out;
    matrix current_controlled_servos;
    matrix max_present_current;
    matrix min_moving_current;
    matrix min_torque_current;
    matrix overshot_goal;
    matrix overshot_goal_temp;
   



     

    parameter number_num_transitions;
    parameter min_limits;
    parameter max_limits;
    parameter robotType;


    std::random_device rd;
    
   
    int position_margin = 4;
    int transition = 0;
    int current_increment = 2;
    int starting_current = 30;
    int current_limit = 1200;
    bool find_minimum_torque_current = false;
    bool minimum_torque_current_finsih =false;
    
  
    

    double time_prev_position;
    double position_sampling_interval = 0.05;

 
    matrix SetGoalCurrent(matrix present_current,  int increment, int limit, matrix position, matrix goal_position, int margin){
        matrix current_output(present_current.size());
        Notify(msg_debug, "Inside SetGoalCurrent()");
        if(present_current.size() != current_output.size()){
            std::cout << present_current.size() << std::endl;
            std::cout << current_output.size() << std::endl;
            Notify(msg_fatal_error, "Present current and Goal current must be the same size");
            return -1;
        }
        for (int i = 0; i < current_controlled_servos.size(); i++) {
            if ( goal_current(current_controlled_servos(i)) < limit && abs(goal_position(current_controlled_servos(i))- position(current_controlled_servos(i)))> margin){
                Notify(msg_debug, "Increasing current");
                current_output(current_controlled_servos(i)) = goal_current(current_controlled_servos(i))+ increment;
                if(abs(present_current(current_controlled_servos(i))) > max_present_current(current_controlled_servos(i))){
                    max_present_current(current_controlled_servos(i)) = abs(present_current(current_controlled_servos(i)));
                }
                
            }
            else
            {
                current_output(i) = goal_current(i);
            }
        }    
        return current_output;

    }

    matrix RandomisePositions(matrix present_position, int num_transitions, matrix min_limits, matrix max_limits, std::string robotType){
        // Initialize the random number generator with the seed
        std::mt19937 gen(rd());
        int num_columns = (robotType == "Torso") ? 2 : 12;
        
        matrix goal_positions(num_transitions, present_position.size());
        goal_positions.set(0);
        if (num_columns != min_limits.size() && num_columns != max_limits.size()){
    
            Notify(msg_fatal_error, "Min and Max limits must have the same number of columns as the current controlled servos in the robot type (2 for Torso, 12 for full body)");
            return -1;
        }
        if (robotType=="Torso"){
            for (int i = 0; i < num_transitions; i++) {
                for (int j = 0; j < num_columns; j++) {
                    std::uniform_real_distribution<double> distr(min_limits(j), max_limits(j));
                    goal_positions(i, j) = int(distr(gen));
                }
            }
        }
        
        
    
        return goal_positions;
    }

    bool ReachedGoal(matrix present_position, matrix goal_positions, int margin){
        for (int i = 0; i < current_controlled_servos.size(); i++) {
            if (abs(present_position(current_controlled_servos(i)) - goal_positions(current_controlled_servos(i))) > margin){
                Notify(msg_debug, "Not reached goal");
                return false;
            }
        }
        std::cout << "Reached goal" << std::endl;
        return true;
        
    }

    bool ApproximatingGoal(matrix present_position, matrix previous_position, matrix goal_position, int margin){
        for (int i =0; i < current_controlled_servos.size(); i++){
            //Checking if distance to goal is decreasing
            if (abs(goal_position(current_controlled_servos(i)) - present_position(current_controlled_servos(i))) < 0.97*abs(goal_position(current_controlled_servos(i))-previous_position(current_controlled_servos(i)))){
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
    //saving starting position, goal position and current in json file
    void SavePositionsJson(matrix goal_positions, matrix start_position, matrix max_current, matrix min_current, matrix min_torque, matrix overshot, std::string robotType, int transition){
        Notify(msg_debug, "Inside SavePositionsJson()");
        std:: cout << "Saving positions in json file" << std::endl;
        std::ofstream file;
        std::string scriptPath = __FILE__;
        std::string scriptDirectory = scriptPath.substr(0, scriptPath.find_last_of("/\\"));
        std::string filePath = scriptDirectory + "/CurrentPositionMapping" + robotType + ".json";
        
        // Check if file exists
        std::ifstream checkFile(filePath);
        bool fileExists = checkFile.good();
        checkFile.close();
        
        file.open(filePath, std::ios::app);
    
        if (transition==0 && !fileExists){
            file << "{\n";
            file << "\"Mapping\": [\n";
        }
        else if(transition>0){
            file << "{\"StartingPosition\": [";
            for (int i = 0; i < current_controlled_servos.size(); i++){
                file << int(start_position(current_controlled_servos(i)));
                if (i < current_controlled_servos.size()-1){
                    file << ", ";
                }     
            }
            file << "],";
            file << "\"GoalPosition\": [";
            for (int i = 0; i < current_controlled_servos.size(); i++){
                file << int(goal_positions(current_controlled_servos(i)));
                if (i < current_controlled_servos.size()-1){
                    file << ", ";
                }
            }
            file << "],";
            file << "\"OvershotGoal\": [";
            for (int i = 0; i < current_controlled_servos.size(); i++){
                file << overshot(current_controlled_servos(i));
                if (i < current_controlled_servos.size()-1){
                    file << ", ";
                }
            }
           
            file << "],";
            file << "\"MaxCurrent\": ["; 
            for (int i = 0; i < current_controlled_servos.size(); i++){
                file << int(max_current(current_controlled_servos(i)));
                if (i < current_controlled_servos.size()-1){
                    file << ", ";
                }
            }
            
            file << "],";
            file << "\"MinMovingCurrent\": ["; 
            for (int i = 0; i < current_controlled_servos.size(); i++){
                file << int(abs(min_current(current_controlled_servos(i))));
                if (i < current_controlled_servos.size()-1){
                    file << ", ";
                }
            }
            file << "],";
            file << "\"MinCurrentForTorque\": ["; 
            for (int i = 0; i < current_controlled_servos.size(); i++){
                file << int(abs(min_torque(current_controlled_servos(i))));
                if (i < current_controlled_servos.size()-1){
                    file << ", ";
                }
            }
            //if transiiton is not the last one, add a comma
            if (transition < number_num_transitions-1){
                file << "]},";
            }
            else{
                file << "]}\n]\n}";
            }
        }
        file << std::endl;
    
        file.close();
    }

    // FindMinimumTorqueCurrent(present_current, present_position, previous_position, current_limit), returns current and boolean value
    std::pair<matrix, bool> FindMinimumTorqueCurrent(matrix present_current, matrix present_position, matrix previous_position, int decreasing_step) {
        matrix current_output(present_current.size());
        bool minimum_torque_current_found;

        for (int i = 0; i < current_controlled_servos.size(); i++) {
            minimum_torque_current_found = false;
            
            if (abs(int(present_position(current_controlled_servos(i)))) == abs(int(previous_position(current_controlled_servos(i))))) {
                current_output(i) = (abs(present_current(current_controlled_servos(i))) - decreasing_step);
                if (current_output(current_controlled_servos(i)) < 0) {
                    current_output(current_controlled_servos(i)) = 0;
                    minimum_torque_current_found = true;
                }
            } else {
                current_output(current_controlled_servos(i)) = abs(present_current(current_controlled_servos(i)));
                std::cout << "Minimum torque current found for servo: " << current_controlled_servos(i) << std::endl;
                std::cout << "Minimum Current: " << current_output(i) << std::endl;
                minimum_torque_current_found = true;
            }
        }

        return std::make_pair(current_output, minimum_torque_current_found);
    }
    
    //Check if the robot has overshot the goal by comparing the sign of the difference between the starting position and the goal position and the difference between the current position and the goal position
    matrix OvershotGoal (matrix position, matrix goal_position, matrix starting_position, matrix overshot_goal){
        //Matrix of booleans to check if the robot has overshot the goal
        for (int i = 0; i < current_controlled_servos.size(); i++){
            
            if (overshot_goal(i) == false && int((goal_position(current_controlled_servos(i)) - starting_position(current_controlled_servos(i)))) * int((goal_position(current_controlled_servos(i)) - position(current_controlled_servos(i))) ) < 0){
                overshot_goal(i)= true;
            }
        }
        return overshot_goal;
    }
    
    
    void Init()
    {
    
        Bind(present_current, "PresentCurrent");
        Bind(present_position, "PresentPosition");
        Bind(goal_current, "GoalCurrent");
        Bind(number_num_transitions, "NumberTransitions");
        Bind(goal_positions_out, "GoalPosition");

        Bind(min_limits, "MinLimits");
        Bind(max_limits, "MaxLimits");
        Bind(robotType, "RobotType");


       
        position_transitions.set_name("PositionTransitions");
        position_transitions = RandomisePositions(present_position, number_num_transitions, min_limits, max_limits, robotType);
        goal_positions_out.copy(position_transitions[0]);

        previous_position.set_name("PreviousPosition");
        previous_position.copy(present_position);
        position_transitions.print();
        goal_current.set(starting_current);
        start_position.copy(present_position);
        max_present_current.set_name("MaxPresentCurrent");
        max_present_current.copy(present_current);
        min_moving_current.set_name("MinMovingCurrent");
        min_moving_current.copy(starting_current);
        min_torque_current.set_name("MinTorqueCurrent");
        min_torque_current.set(starting_current);
        
     
        time_prev_position= std::time(nullptr);

        std::string robot = robotType;

        if(robot == "Torso"){
            current_controlled_servos.set_name("CurrentControlledServosTorso");
            current_controlled_servos = {0, 1};
            overshot_goal_temp = {false, false};
        }
        else{
            current_controlled_servos.set_name("CurrentControlledServosFullBody");
            current_controlled_servos = {0, 1, 2, 6, 7, 8, 9, 10, 12, 13, 14, 15, 16, 18};
            overshot_goal_temp = {false, false, false, false, false, false, false, false, false, false, false, false, false, false};
        }
        
        
        overshot_goal.set_name("OvershotGoal");
        
        
    }


    void Tick()
    {   
        if (present_current.connected() && present_position.connected()){
            
            if (ApproximatingGoal(present_position, previous_position, goal_positions_out, position_margin)){
                
                //Check minumum current while approximating goal
                for (int i = 0; i < current_controlled_servos.size(); i++){
                    if(abs(present_current(current_controlled_servos(i))) < min_moving_current(current_controlled_servos(i)) && abs(present_current(current_controlled_servos(i))) > 0)
                    {
                        min_moving_current(current_controlled_servos(i)) = abs(present_current(current_controlled_servos(i)));
                    }
                } 
                     
            }
            else if (find_minimum_torque_current)
            {
                std::pair<matrix, bool> result = FindMinimumTorqueCurrent(present_current, present_position, previous_position, 1);
                goal_current.copy(result.first);
                minimum_torque_current_finsih = result.second;
                if(minimum_torque_current_finsih){
                    find_minimum_torque_current = false;
                    minimum_torque_current_finsih = false;
                    min_torque_current.copy(present_current);
                    goal_current(0) = (abs(min_torque_current(0)) +2);
                    
                    //save starting position, goal position and current in json file
                    SavePositionsJson(goal_positions_out,start_position, max_present_current, min_moving_current, min_torque_current, overshot_goal, robotType, transition);
                    transition++;
                    std::cout << "Transition: " << transition << std::endl;
                    if (transition < number_num_transitions){
                        goal_positions_out.copy(position_transitions[transition]);
                        start_position.copy(present_position);
                        std::cout << "New goal position" << std::endl;
                        goal_current.copy(present_current);
                        min_moving_current.set(starting_current);
                        max_present_current.set(starting_current);
                        overshot_goal_temp.reset();
                        
                        
                        
                    }
                    else{
                        Notify(msg_end_of_file, "All transitions completed");
                        std::exit(1); //terminate the program
                    }
                }
            }
            
            else if (ReachedGoal(present_position, goal_positions_out, position_margin)){

                find_minimum_torque_current = true;
    
            }
            else{
                goal_current.copy(SetGoalCurrent(present_current, current_increment, current_limit, present_position, goal_positions_out, position_margin));
                overshot_goal.copy(OvershotGoal(present_position, goal_positions_out, start_position, overshot_goal_temp));
                overshot_goal_temp.copy(overshot_goal);
            }

            
            



            //goal_positions_out.print();
            //previous_position.print();
            //present_position.print();
            
            if(abs(time_prev_position - std::time(nullptr)) > position_sampling_interval){
                time_prev_position = std::time(nullptr);
                previous_position.copy(present_position);
            
            }
         
        }
    }

};





INSTALL_CLASS(CurrentPositionMapping)

