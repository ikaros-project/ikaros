#include "ikaros.h"
#include <random> // Include the header file that defines the 'Random' function
#include <fstream>
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>

using namespace ikaros;
using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

class CurrentPositionMapping: public Module
{
  
    matrix present_position;
    matrix present_current;
    matrix position_transitions;
    matrix previous_position;
    matrix start_position;
    matrix gyro;
    matrix accel;
    matrix eulerAngles;
    
    matrix goal_current;
    matrix goal_position_out;
    matrix current_controlled_servos;
    matrix max_present_current;
    matrix min_moving_current;
    matrix min_torque_current;
    matrix overshot_goal;
    matrix overshot_goal_temp;
    std::vector<std::shared_ptr<std::vector<float>>> moving_trajectory;
    std::vector<std::shared_ptr<std::vector<float>>> current_history;
    std::vector<std::shared_ptr<std::vector<float>>> gyro_history;
    std::vector<std::shared_ptr<std::vector<float>>> accel_history;
    std::vector<std::shared_ptr<std::vector<float>>> angles_history;
    matrix approximating_goal;
    matrix reached_goal;
   
 

    parameter num_transitions;
    parameter min_limits;
    parameter max_limits;
    parameter robotType;
    parameter ConsistencyTest;
    parameter MinimumTorqueCurrentSearch;
    parameter current_increment;

    std::random_device rd;
    
    int number_transitions; //Will be used to add starting and ending position
    int position_margin = 6;
    int transition = 0;
    int starting_current = 150;
    int current_limit = 1700;
    bool find_minimum_torque_current = false;
    matrix minimum_torque_current_found;
    int unique_id;
    bool second_tick = false;
    bool first_start_position = true;

    matrix transition_start_time;
    matrix transition_duration;
    double time_prev_position;
    double position_sampling_interval = 50;
    matrix number_ticks;
 
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
            int servo_idx = current_controlled_servos(i);
            if ( goal_current(servo_idx) < limit && abs(goal_position(servo_idx)- position(servo_idx))> margin){
                Notify(msg_debug, "Increasing current");
                current_output(servo_idx) = abs(goal_current(servo_idx))+ increment;
                          
            }
            else
            {
                current_output(servo_idx) = current_output(servo_idx);
            }
        }    
        return current_output;

    }

    matrix RandomisePositions(matrix present_position, int num_transitions, matrix min_limits, matrix max_limits, std::string robotType){
        // Initialize the random number generator with the seed
        std::mt19937 gen(rd());
        int num_columns = (robotType == "Torso") ? 2 : 12;
        matrix goal_positions(num_transitions, present_position.size());
        goal_positions.set(180);
        if (num_columns != min_limits.size() && num_columns != max_limits.size()){
    
            Notify(msg_fatal_error, "Min and Max limits must have the same number of columns as the current controlled servos in the robot type (2 for Torso, 12 for full body)");
            return -1;
        }
        
        if (robotType=="Torso"){
            for (int i = 0; i < num_transitions; i++) {
                for (int j = 0; j < num_columns; j++) {
                    std::uniform_real_distribution<double> distr(min_limits(j), max_limits(j));
                    goal_positions(i, j) = int(distr(gen));
                    if (i == num_transitions-1){
                        goal_positions(i, j) = 180;
                    }
                }
            }
        }
        
    

        return goal_positions;
    }

    void ReachedGoal(matrix present_position, matrix goal_positions, matrix reached_goal, int margin){
        for (int i = 0; i < current_controlled_servos.size(); i++){
            if (reached_goal(current_controlled_servos(i)) == 0 &&
                abs(present_position(current_controlled_servos(i)) - goal_positions(current_controlled_servos(i))) < margin){

                reached_goal(current_controlled_servos(i)) = 1;

                auto now = Clock::now();
                float current_time_ms = std::chrono::duration<float, std::milli>(now.time_since_epoch()).count();
                transition_duration(current_controlled_servos(i)) = current_time_ms - transition_start_time[current_controlled_servos(i)];
                // Reset start time for future transitions
                transition_start_time(current_controlled_servos(i)) = current_time_ms;      

            }
            else if (reached_goal(current_controlled_servos(i)) == 0){
                number_ticks[current_controlled_servos(i)] ++;
            }

        }
    }

    matrix ApproximatingGoal(matrix present_position, matrix previous_position, matrix goal_position, int margin){
        for (int i =0; i < current_controlled_servos.size(); i++){
            //Checking if distance to goal is decreasing
            if (abs(goal_position(current_controlled_servos(i)) - present_position(current_controlled_servos(i))) < 0.97*abs(goal_position(current_controlled_servos(i))-previous_position(current_controlled_servos(i)))){
                Notify(msg_debug, "Approximating Goal");
                approximating_goal(current_controlled_servos(i)) = 1;
                
            }
            else{
                approximating_goal(current_controlled_servos(i)) = 0;
            }
        }
        return approximating_goal;
    }   

    // Helper function to check if ID exists in file
    bool checkIdExists(const std::string& filePath, int id) {
        std::ifstream file(filePath);
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("\"UniqueID\":" + std::to_string(id)) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    void SaveMetricsToJson(matrix goal_positions, matrix start_position, matrix max_current, matrix min_current, matrix min_torque, matrix overshot, matrix time, matrix ticks, std::string robotType, int transition, int unique_id)
    {
        std::string scriptPath = __FILE__;
        std::string scriptDirectory = scriptPath.substr(0, scriptPath.find_last_of("/\\"));
        std::string filePath = scriptDirectory + "/data/CurrentPositionMapping" + robotType;
        std::string suffix;
        if (ConsistencyTest) {
            suffix += "_ConsistencyTest_Increment_" + std::to_string((int)current_increment);
        }
        if (MinimumTorqueCurrentSearch) {
            suffix += "_TorqueSearch";
        }
        filePath += suffix + ".json";
        // Use stringstream for efficient string building
        std::stringstream json;
        
        if (transition == 0) {
            // Check if file exists and is non-empty
            std::ifstream checkFile(filePath);
            bool fileEmpty = checkFile.peek() == std::ifstream::traits_type::eof();
            checkFile.close();
            
            if (fileEmpty) {
                json << "{\n\"Mapping\": [\n";
            } else {
                // Truncate the file to remove last "]\n}" and add a comma
                std::fstream file(filePath, std::ios::in | std::ios::out);
                file.seekp(-3, std::ios::end);
                file << ",\n";
                file.close();
                return;
            }
        }

        // Build JSON string efficiently
        json << "\n{\"UniqueID\":" << unique_id << ",";

        // Helper lambda for array serialization
        auto writeArray = [&json](const std::string& name, matrix& mat, matrix& servos) {
            json << "\"" << name << "\":[";
            for (int i = 0; i < servos.size(); i++) {
                json << int(mat(servos(i)));
                if (i < servos.size() - 1) json << ",";
            }
            json << "]";
        };
        
        writeArray("StartingPosition", start_position, current_controlled_servos);
        json << ",";
        writeArray("GoalPosition", goal_positions, current_controlled_servos);
        json << ",";
        writeArray("OvershotGoal", overshot, current_controlled_servos);
        json << ",";
        writeArray("MaxCurrent", max_current, current_controlled_servos);
        json << ",";
        writeArray("MinCurrentToMoveFromStart", min_current, current_controlled_servos);
        json << ",";
        writeArray("MinCurrentForTorqueAtGoal", min_torque, current_controlled_servos);
        json << ",";
        writeArray("Time(ms)", time, current_controlled_servos);
        json << ",";
        writeArray("Number of Ticks", ticks, current_controlled_servos);
        json << "}";
        
        if (transition == number_transitions - 1) {
            json << "\n]\n}";
        } else {
            json << ",";
        }
        
        // Single file write operation
        std::ofstream file(filePath, std::ios::app);
        file << json.str();
        file.close();
    }

    void SaveTrajectory(std::vector<std::shared_ptr<std::vector<float>>>& trajectory,
                        std::vector<std::shared_ptr<std::vector<float>>>& current,
                        std::vector<std::shared_ptr<std::vector<float>>>& gyro,
                        std::vector<std::shared_ptr<std::vector<float>>>& accel,
                        std::vector<std::shared_ptr<std::vector<float>>>& angles,
                        std::string robotType, int id)
    {
        // Check if there's data to save
        if (trajectory.empty() || current.empty() || gyro.empty() || accel.empty() || angles.empty()) {
            Notify(msg_warning, "No trajectory data to save");
            return;
        }

        std::string scriptPath = __FILE__;
        std::string scriptDirectory = scriptPath.substr(0, scriptPath.find_last_of("/\\"));
        std::string filePath = scriptDirectory + "/data/Trajectories" + robotType;
        std::string suffix;
        if (ConsistencyTest) {
            suffix += "_ConsistencyTest_Increment_" + std::to_string((int)current_increment);
        }
        if (MinimumTorqueCurrentSearch) {
            suffix += "_TorqueSearch";
        }
        filePath += suffix + ".json";
        
        // Ensure ID is unique
        while (checkIdExists(filePath, id)) {
            id++;
        }

        // Use stringstream for efficient string building
        std::stringstream json;
        
        // Check if file exists and is non-empty
        std::ifstream checkFile(filePath);
        bool fileExists = checkFile.good();
        bool fileEmpty = checkFile.peek() == std::ifstream::traits_type::eof();
        checkFile.close();
        
        if (!fileExists || fileEmpty) {
            json << "{\n\"Trajectories\": [\n";
            json << "{\n\"UniqueID\":" << id << ",\n";
        } else {
            // Remove the closing brackets and add a comma
            std::fstream file(filePath, std::ios::in | std::ios::out);
            file.seekp(-3, std::ios::end);
            file << ",{\n\"UniqueID\":" << id << ",\n";
            file.close();
        }

        // Write trajectory data
        
        
        // Helper lambda for writing arrays
        auto writeArray = [&json](const std::string& name, const std::vector<std::shared_ptr<std::vector<float>>>& data, matrix servos) {
            json << "\"" << name << "\":[";
            for (size_t i = 0; i < data.size(); i++) {
                json << "[";
                if (name == "Trajectory" || name == "Current") {
                    // For trajectory and current, only include controlled servos
                    for (size_t j = 0; j < servos.size(); j++) {
                        int servo_idx = servos[j];
                        json << std::fixed << std::setprecision(2) << (*data[i])[servo_idx];
                        if (j < servos.size() - 1) json << ",";
                    }
                } else {
                    // For gyro, accel, and angles, include all values
                    for (size_t j = 0; j < (*data[i]).size(); j++) {
                        json << std::fixed << std::setprecision(2) << (*data[i])[j];
                        if (j < (*data[i]).size() - 1) json << ",";
                    }
                }
                json << "]";
                if (i < data.size() - 1) json << ",";
            }
            json << "]";
        };

        writeArray("Trajectory", trajectory, current_controlled_servos);
        json << ",\n";
        writeArray("Current", current, current_controlled_servos);
        json << ",\n";
        writeArray("Gyro", gyro, current_controlled_servos);
        json << ",\n";
        writeArray("Accel", accel, current_controlled_servos);
        json << ",\n";
        writeArray("Angles", angles, current_controlled_servos);
        json << "\n}";

        if(transition == number_transitions -1){
            json << "\n]\n}";
        }
        else{
            json << "},\n";
        }

        // Single file write operation
        std::ofstream file(filePath, std::ios::app);
        file << json.str();
        file.close();

        unique_id = id;
    }

    // FindMinimumTorqueCurrent(present_current, present_position, previous_position, current_limit), returns current and boolean value
    std::pair<matrix, matrix> FindMinimumTorqueCurrent(matrix present_current, matrix present_position, matrix previous_position, int decreasing_step) {
        matrix current_output(present_current.size());
        matrix torque_found_temp;
        torque_found_temp.copy(minimum_torque_current_found);
        

        for (int i = 0; i < current_controlled_servos.size(); i++) {       
            int servo_idx = current_controlled_servos(i);
            
            if (torque_found_temp[servo_idx] == 0) {

                // If servo is at goal position and not moving, it may not need current
                if (abs(present_position[servo_idx]) - goal_position_out[servo_idx] < 2 &&
                    abs(present_current[servo_idx]) < 10)
                {
                    current_output[servo_idx] = 0;
                    std::cout << "Servo " << servo_idx << " at goal and stable without current" << std::endl;
                    torque_found_temp[servo_idx] = 1;
                }
                else if (int(present_position[servo_idx]) == int(previous_position[servo_idx]))  
                {
                    current_output[servo_idx] = abs(present_current[servo_idx])- decreasing_step;

                    //std::cout << "Decreasing goal current to: " << (float)current_output(current_controlled_servos(i)) << " of servo: " << current_controlled_servos(i)+2 << std::endl;

                    if (current_output[servo_idx] < 0) {
                        current_output[servo_idx] = 0;
                        torque_found_temp[servo_idx] = 1;
                        
                    }
                
                } 
                
                    
                
            }
        }

        return std::make_pair(current_output, torque_found_temp);
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
    
    int GenerateRandomNumber(int min, int max){
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> distr(min, max);
        return distr(gen);
    }

    // FUnction to print a progress bar of the current transition
    void PrintProgressBar(int transition, int number_transitions){
        int barWidth = 70;
        float progress = (float)transition/number_transitions;
        std::cout << "[";
        int pos = barWidth * progress;
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << int(progress * 100.0) << " %\r" << std::endl;
        std::cout.flush();
    }

    matrix DeterministicPositionTransitions(matrix present_position, int number_transitions, std::string robotType){
        matrix position_transitions(number_transitions, present_position.size());

        //Only first servo is moved, 3 and 4 are pupils and fixed
        for (int i = 0; i < number_transitions; i++){
            position_transitions(i, 0) = (i % 2 == 0) ? 180 : 237;
            position_transitions(i, 1) = 188;
            position_transitions(i, 2) = 180;
            position_transitions(i, 3) = 180;
            position_transitions(i, 4) = 12;
            position_transitions(i, 5) = 12;

   
        }
        return position_transitions;
    }
    
    void Init()
    {
    
        Bind(present_current, "PresentCurrent");
        Bind(present_position, "PresentPosition");
        Bind(gyro, "GyroData");
        Bind(accel, "AccelData");
        Bind(eulerAngles, "EulerAngles");
        Bind(goal_current, "GoalCurrent");
        Bind(num_transitions, "NumberTransitions");
        Bind(goal_position_out, "GoalPosition");

        Bind(min_limits, "MinLimits");
        Bind(max_limits, "MaxLimits");
        Bind(robotType, "RobotType");
        Bind(ConsistencyTest, "ConsistencyTest");
        Bind(MinimumTorqueCurrentSearch, "MinimumTorqueCurrentSearch");
        Bind(current_increment, "CurrentIncrement");

        number_transitions = num_transitions.as_int()+1; // Add one to the number of transitions to include the ending position
       
        position_transitions.set_name("PositionTransitions");
        if (ConsistencyTest){
            position_transitions = DeterministicPositionTransitions(present_position, number_transitions, robotType);
        }
        else{
            position_transitions = RandomisePositions(present_position, number_transitions, min_limits, max_limits, robotType);
        }
        goal_position_out.copy(position_transitions[0]);
        previous_position.set_name("PreviousPosition");
        previous_position.copy(present_position);
        goal_current.set(starting_current);
        max_present_current.set_name("MaxPresentCurrent");
        max_present_current.copy(present_current);
        min_moving_current.set_name("MinMovingCurrent");
        min_moving_current.copy(present_current);
        min_moving_current.set(1000);
        min_torque_current.set_name("MinTorqueCurrent");
        min_torque_current.copy(present_current);
        min_torque_current.set(1000);
        minimum_torque_current_found.set_name("MinimumTorqueCurrentFound");
        minimum_torque_current_found.copy(present_current);
        minimum_torque_current_found.set(0);
        

        approximating_goal.set_name("ApproximatingGoal");
        approximating_goal.copy(present_position);
        approximating_goal.set(0);
        transition_start_time.set_name("StartedTransitionTime");
        transition_start_time.copy(approximating_goal);
        transition_duration.set_name("TransitionDuration");
        transition_duration.copy(approximating_goal);
        number_ticks.set_name("NumberTicks");



        time_prev_position = std::chrono::duration<double, std::milli>(Clock::now().time_since_epoch()).count();
        

        std::string robot = robotType;

        if(robot == "Torso"){
            current_controlled_servos.set_name("CurrentControlledServosTorso");
            current_controlled_servos = {0, 1};
            overshot_goal_temp = {false, false};
            reached_goal = {0, 0};
        }
        else{
            current_controlled_servos.set_name("CurrentControlledServosFullBody");
            current_controlled_servos = {0, 1, 6, 7, 8, 9, 10, 12, 13, 14, 15, 16, 18};
            overshot_goal_temp = {false, false, false, false, false, false, false, false, false, false, false, false, false};
            reached_goal = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        }

        reached_goal.set_name("ReachedGoal");

        overshot_goal.set_name("OvershotGoal");
        unique_id = GenerateRandomNumber(0, 1000000);
        number_ticks.copy(reached_goal);
    }


    void Tick()
    {   
        
        if (present_current.connected() && present_position.connected() && second_tick){

           

            approximating_goal = ApproximatingGoal(present_position, previous_position, goal_position_out, position_margin);
            
            
        
            if (approximating_goal.sum()>0 && !find_minimum_torque_current) {
                
                if(second_tick && first_start_position){
                    start_position.copy(present_position);
                    auto now = Clock::now();
                    float now_ms = std::chrono::duration<float, std::milli>(now.time_since_epoch()).count();

                    transition_start_time.set(now_ms);
                    first_start_position = false;
                }
                
                moving_trajectory.push_back(std::make_shared<std::vector<float>>(present_position.data_->begin(), present_position.data_->end()));
                
                current_history.push_back(std::make_shared<std::vector<float>>(present_current.data_->begin(), present_current.data_->end()));
                gyro_history.push_back(std::make_shared<std::vector<float>>(gyro.data_->begin(), gyro.data_->end()));
                accel_history.push_back(std::make_shared<std::vector<float>>(accel.data_->begin(), accel.data_->end()));
                angles_history.push_back(std::make_shared<std::vector<float>>(eulerAngles.data_->begin(), eulerAngles.data_->end()));
                

                // Update min_moving_current only at the start of movement for each servo
                for (int i = 0; i < current_controlled_servos.size(); i++) {
                    int servo = current_controlled_servos(i);
                    float abs_current = abs(present_current(servo));
                    
                    // Capture the current when the servo starts moving
                    if (min_moving_current(servo)==1000 && abs_current > 0 && approximating_goal(servo) == 1) {
                        min_moving_current(servo) = abs_current;
                        auto now = Clock::now();
                        float now_ms = std::chrono::duration<float, std::milli>(now.time_since_epoch()).count();
                        transition_start_time(current_controlled_servos(i)) = now_ms;

                       
                    }
                    // Update the max current for each servo
                    if(abs(present_current(current_controlled_servos(i))) > max_present_current(current_controlled_servos(i))){
                    max_present_current(current_controlled_servos(i)) = abs(present_current(current_controlled_servos(i)));
                    }
                }
          
            }

            
            else if (find_minimum_torque_current)
            {
                if (MinimumTorqueCurrentSearch)
                {
                    std::pair<matrix, matrix> result = FindMinimumTorqueCurrent(present_current, present_position, previous_position, 1);
                    goal_current.copy(result.first);
                    minimum_torque_current_found.copy(result.second);
                    if (minimum_torque_current_found.sum() == current_controlled_servos.size())
                    {
                        find_minimum_torque_current = false;
                        min_torque_current.copy(present_current);
                        goal_current.add(10);
                        Notify(msg_debug, "Minimum torque current found");

                        SaveMetricsToJson(goal_position_out, start_position, max_present_current, min_moving_current, min_torque_current, overshot_goal, transition_duration, number_ticks, robotType, transition, unique_id);
        
                        // Move to next transition after finding minimum torque
                        transition++;
                        auto now = Clock::now();
                        float current_time_ms = std::chrono::duration<float, std::milli>(now.time_since_epoch()).count();
                        transition_start_time.set(current_time_ms);
                        transition_duration.set(0);
                        
                        PrintProgressBar(transition, number_transitions);

                        if (transition < number_transitions) {
                            goal_position_out.copy(position_transitions[transition]);
                            std::cout << "New goal position" << std::endl;
                            goal_current.set(starting_current);
                            min_moving_current.copy(present_current);
                            max_present_current.reset();
                            overshot_goal_temp.reset();
                            min_torque_current.reset();
                            moving_trajectory.clear();
                            current_history.clear();
                            gyro_history.clear();
                            accel_history.clear();
                            angles_history.clear();
                            reached_goal.reset();
                            number_ticks.set(0);
                        } else {
                            Notify(msg_end_of_file, "All transitions completed");
                            Sleep(1);
                            Notify(msg_terminate, "Shutting down");
                        }
                    }
                }
                else {
                    find_minimum_torque_current = false;
                }
            }
            
            else if (reached_goal.sum() == current_controlled_servos.size()){
                // Save trajectory data before any other operations
                if (!moving_trajectory.empty()) {
                    SaveTrajectory(moving_trajectory, current_history, gyro_history, accel_history, angles_history, robotType, unique_id);
                }

                if (MinimumTorqueCurrentSearch) {
                    find_minimum_torque_current = true;
                } else {
                    // Move to next transition
                    transition++;
                    auto now = Clock::now();
                    float current_time_ms = std::chrono::duration<float, std::milli>(now.time_since_epoch()).count();
                    transition_start_time.set(current_time_ms);
                    transition_duration.set(0);
                    
                    PrintProgressBar(transition, number_transitions);

                    if (transition < number_transitions) {
                        goal_position_out.copy(position_transitions[transition]);
                        std::cout << "New goal position" << std::endl;
                        goal_current.set(starting_current);
                        min_moving_current.copy(present_current);
                        max_present_current.reset();
                        overshot_goal_temp.reset();
                        min_torque_current.reset();
                        moving_trajectory.clear();
                        current_history.clear();
                        gyro_history.clear();
                        accel_history.clear();
                        angles_history.clear();
                        reached_goal.reset();
                        number_ticks.set(0);
                    } else {
                        Notify(msg_end_of_file, "All transitions completed");
                        Sleep(1);
                        Notify(msg_terminate, "Shutting down");
                    }
                }
                
                // first row of the trajectory matrix is the starting position
                if (!moving_trajectory.empty()) {
                    std::copy(moving_trajectory.front()->begin(), moving_trajectory.front()->end(), start_position.begin());
                }

                moving_trajectory.clear();
                current_history.clear();
                gyro_history.clear();
                accel_history.clear();
                angles_history.clear();
                reached_goal.set(0);
            }
            else{
                goal_current.copy(SetGoalCurrent(present_current, current_increment, current_limit, present_position, goal_position_out, position_margin));
                //temporary, remove later
                //if(MinimumTorqueCurrentSearch){
                //    goal_current[0] = max(goal_current[0], 500);
                //}
               

                overshot_goal.copy(OvershotGoal(present_position, goal_position_out, start_position, overshot_goal_temp));
                overshot_goal_temp.copy(overshot_goal);
                ReachedGoal(present_position, goal_position_out, reached_goal, position_margin);
                // Save present positions into trajectory matrix
                moving_trajectory.push_back(std::make_shared<std::vector<float>>(present_position.data_->begin(), present_position.data_->end()));
                current_history.push_back(std::make_shared<std::vector<float>>(present_current.data_->begin(), present_current.data_->end()));
                gyro_history.push_back(std::make_shared<std::vector<float>>(gyro.data_->begin(), gyro.data_->end()));
                accel_history.push_back(std::make_shared<std::vector<float>>(accel.data_->begin(), accel.data_->end()));
                angles_history.push_back(std::make_shared<std::vector<float>>(eulerAngles.data_->begin(), eulerAngles.data_->end()));
            }

            auto now = Clock::now();
            float now_ms = std::chrono::duration<float, std::milli>(now.time_since_epoch()).count();
            
            if (abs(now_ms - time_prev_position) > position_sampling_interval)
            {
                time_prev_position = now_ms;
                previous_position.copy(present_position);
            }
        }
        
        second_tick = true;
    }

};





INSTALL_CLASS(CurrentPositionMapping)

