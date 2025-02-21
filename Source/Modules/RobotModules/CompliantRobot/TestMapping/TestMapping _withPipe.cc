#include "ikaros.h"
#include <random> // Include the header file that defines the 'Random' function
#include <fstream>
#include <vector>
#include <memory>
#include "json.hpp"
#include <string>
#include <algorithm>
#include <chrono>

using namespace ikaros;


class TestMapping: public Module
{
    //pipe
    FILE* ann_pipe;
    // Inputs
    matrix present_position;
    matrix present_current;
    matrix gyro;
    matrix accel;
    matrix eulerAngles;

    // Outputs
    matrix goal_current;
    matrix goal_position_out;
    

    // Internal
    matrix current_controlled_servos;
    matrix max_present_current;
    matrix min_moving_current;
    matrix min_torque_current;
    matrix overshot_goal;
    matrix overshot_goal_temp;
    matrix position_transitions;
    std::vector<std::shared_ptr<std::vector<float>>> moving_trajectory;
    matrix approximating_goal;
    matrix reached_goal;
    dictionary current_coefficients;
    matrix coeffcient_matrix;
    
   
    // Parameters
    parameter num_transitions;
    parameter min_limits;
    parameter max_limits;
    parameter robotType;
    parameter prediction_model;

    // Internal
    std::random_device rd;
    
    int number_transitions; //Will be used to add starting and ending position
    int position_margin = 3;
    int transition = 0;
    int current_increment = 1;
    int starting_current = 30;
    int current_limit = 1700;
    bool find_minimum_torque_current = false;
  
    int unique_id;
    bool second_tick = false;
    bool first_start_position = true;

    matrix transition_start_time;
    matrix transition_duration;
    double time_prev_position;
    matrix ann_output;
    

    std::vector<std::string> servo_names;

    matrix initial_currents;
    matrix current_differences;

    matrix RandomisePositions(int num_transitions, matrix min_limits, matrix max_limits, std::string robotType)
    {
        // Initialize the random number generator with the seed
        std::mt19937 gen(rd());
        int num_columns = (robotType == "Torso") ? 2 : 12;
        matrix generated_positions(num_transitions, present_position.size());
        generated_positions.set(180); // Neutral position
        // set pupil servos to 12 of all rows
        for (int i = 0; i < generated_positions.rows(); i++)
        {
            generated_positions(i, 4) = 12;
            generated_positions(i, 5) = 12;
        }
        if (num_columns != min_limits.size() || num_columns != max_limits.size())
        {
            Error("Min and Max limits must have the same number of columns as the current controlled servos in the robot type (2 for Torso, 12 for full body)");
            return -1;
        }

        for (int i = 0; i < num_transitions; i++)
        {
            for (int j = 0; j < num_columns; j++)
            {
                std::uniform_real_distribution<double> distr(min_limits(j), max_limits(j));
                generated_positions(i, j) = int(distr(gen));
                if (i == num_transitions - 1)
                {
                    generated_positions(i, j) = 180;
                }
            }
        }

        return generated_positions;
    }

    void ReachedGoal(matrix present_position, matrix goal_positions, matrix reached_goal, int margin){
        for (int i = 0; i < current_controlled_servos.size(); i++){
            if (reached_goal(current_controlled_servos(i)) == 0 &&
                abs(present_position(current_controlled_servos(i)) - goal_positions(current_controlled_servos(i))) < margin){

                reached_goal(current_controlled_servos(i)) = 1;

                double current_time_ms = GetTime();
                
                transition_duration(current_controlled_servos(i)) = current_time_ms - transition_start_time[current_controlled_servos(i)];
                // Reset start time for future transitions
     

            }
          

        }
    }

    matrix ApproximatingGoal(matrix &present_position,  matrix goal_position, int margin){
        matrix previous_position = present_position.last();
        for (int i =0; i < current_controlled_servos.size(); i++){
            //Checking if distance to goal is decreasing
            if (abs(goal_position(current_controlled_servos(i)) - present_position(current_controlled_servos(i))) < 0.97*abs(goal_position(current_controlled_servos(i))-previous_position(current_controlled_servos(i)))){
                Debug( "Approximating Goal");
                approximating_goal(current_controlled_servos(i)) = 1;
                
            }
            else{
                approximating_goal(current_controlled_servos(i)) = 0;
            }
        }
        return approximating_goal;
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

    // Helper function to find column index case-insensitive
    int findColumnCaseInsensitive( ikaros::matrix &m,  std::string &label)
    {
        std::string lowerLabel = label;
        std::transform(lowerLabel.begin(), lowerLabel.end(), lowerLabel.begin(), ::tolower);

        auto labels = m.labels(1); // Get column labels
        for (size_t i = 0; i < labels.size(); i++)
        {
            std::string currentLabel = labels[i];
            std::transform(currentLabel.begin(), currentLabel.end(), currentLabel.begin(), ::tolower);
            if (currentLabel == lowerLabel)
            {
                return i;
            }
        }
        throw std::runtime_error("Label not found: " + label);
    }

    std::vector<double> extractModelParameters(ikaros::matrix &m, std::string &model_name)
    {
        // Define all possible parameters
        std::vector<std::string> parameter_labels = {
            "CurrentMean",
            "CurrentStd",
            "betas_linear[DistanceToGoal]",
            "betas_linear[Position]",
            "betas_quad[DistanceToGoal_squared]",
            "betas_quad[Position_squared]",
            "intercept"};

        std::vector<double> values(parameter_labels.size(), 0.0);

        try
        {
            for (size_t i = 0; i < parameter_labels.size(); i++)
            {
                // Skip quadratic terms if model is linear
                if (model_name != "Quadratic" &&
                    (parameter_labels[i].find("quad") != std::string::npos ||
                     parameter_labels[i].find("squared") != std::string::npos))
                {
                    values[i] = 0.0;
                    continue;
                }

                try
                {
                    values[i] = static_cast<double>(m(0, findColumnCaseInsensitive(m, parameter_labels[i])));
                }
                catch (const std::runtime_error &e)
                {
                    // Set to 0.0 for non-required parameters in linear model
                    values[i] = 0.0;
                }
            }
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("Error extracting parameters: " + std::string(e.what()));
        }

        return values;
    }
    matrix CreateCoefficientsMatrix(dictionary coefficients, matrix current_controlled_servos, std::string model, std::vector<std::string> servo_names){
        // mapping that defines number of coefficients for each model
        //  Mapping that defines number of coefficients for each model
        std::map<std::string, int> model_coefficients = {{"Linear", 5}, {"Quadratic", 7}};
        int num_coefficients = model_coefficients[model];

        // Initialize matrix with proper dimensions
        matrix coefficients_matrix(current_controlled_servos.size(), num_coefficients);
        
        // Iterate through each servo
        for (int i = 0; i < current_controlled_servos.size(); i++)
        {
            std::string servo_name = servo_names[current_controlled_servos(i)];

            

            // Get the coefficients dictionary for this servo
            dictionary servo_coeffs = coefficients[model][servo_name];

            // Skip "sigma" if it exists and iterate over the coefficient values
            int coeff_idx = 0;
            for ( auto &coeff : servo_coeffs)
            {
                if (coeff.first != "sigma")
                { // Skip sigma parameter
                    coefficients_matrix(i, coeff_idx) = (coeff.second).as_float();
                    if (i == 0)
                        coefficients_matrix.push_label(1, coeff.first);
                    coeff_idx++;
                }
            }
        }
        coefficients_matrix.set_name("CoefficientMatrix");
       

        return coefficients_matrix;
    }

    matrix SetGoalCurrent(matrix present_current, int increment, int limit, matrix position, matrix goal_position, int margin, matrix coefficients, std::string model_name)
    {
        matrix current_output(present_current.size());
        current_output.copy(present_current);

        // Early validation
        if (present_current.size() != current_output.size())
        {
            Error("Present current and Goal current must be the same size");
            return current_output;
        }

        // Only process servos that are current-controlled
        for (int i = 0; i < current_controlled_servos.size(); i++)
        {
            int servo_idx = current_controlled_servos(i);

            // Skip processing if servo has reached its goal
            if (abs(goal_position(servo_idx) - position(servo_idx)) <= margin)
            {
                continue;
            }

            // Calculate estimated current based on model type
            double estimated_current = 0.0;

            if (model_name == "ANN")
            {
                // Use existing ANN output for this servo
                estimated_current = ann_output[servo_idx];
            }
            else if (model_name == "Linear" || model_name == "Quadratic")
            {
                // Get standardization parameters
                double current_mean = coefficients(i, 0);
                double current_std = coefficients(i, 1);

                // Calculate standardized inputs
                double distance_to_goal = goal_position(servo_idx) - position(servo_idx);
                double std_position = (position(servo_idx) - current_mean) / current_std;
                double std_distance = (distance_to_goal - current_mean) / current_std;

                // Calculate linear terms
                estimated_current = coefficients(i, 6) + // intercept
                                    coefficients(i, 2) * std_distance +
                                    coefficients(i, 3) * std_position;

                // Add quadratic terms if applicable
                if (model_name == "Quadratic")
                {
                    estimated_current += coefficients(i, 4) * std::pow(std_distance, 2) +
                                         coefficients(i, 5) * std::pow(std_position, 2);
                }

                // Unstandardize the result
                estimated_current = estimated_current * current_std + current_mean;
            }
            else
            {
                Warning("Unsupported model type: %s - using default current", model_name.c_str());
                estimated_current = present_current(servo_idx);
            }

            // Apply current limits and set output
            current_output(servo_idx) = std::min(std::max(estimated_current, 0.0), static_cast<double>(limit));
        }

        return current_output;
    }
    void Init()
    {
        //IO
        Bind(present_current, "PresentCurrent");
        Bind(present_position, "PresentPosition");
        Bind(gyro, "GyroData");
        Bind(accel, "AccelData");
        Bind(eulerAngles, "EulerAngles");
        Bind(goal_current, "GoalCurrent");
        Bind(num_transitions, "NumberTransitions");
        Bind(goal_position_out, "GoalPositionOut");
        
        

        //parameters
        Bind(min_limits, "MinLimits");
        Bind(max_limits, "MaxLimits");
        Bind(robotType, "RobotType");
        Bind(prediction_model, "CurrentPrediction");

        std::string scriptPath = __FILE__;
        
        //go up in the directory to get to the folder containing the coefficients.json file
        std::string coefficientsPath = scriptPath.substr(0, scriptPath.find_last_of("/"));
        coefficientsPath = coefficientsPath.substr(0, coefficientsPath.find_last_of("/"));
        coefficientsPath = coefficientsPath + "/CurrentPositionMapping/models/coefficients.json";
        current_coefficients.load_json(coefficientsPath);



        number_transitions = num_transitions.as_int()+1; // Add one to the number of transitions to include the ending position
        position_transitions.set_name("PositionTransitions");
        position_transitions = RandomisePositions(number_transitions, min_limits, max_limits, robotType);
        goal_position_out.copy(position_transitions[0]);

        goal_current.set(starting_current);
        max_present_current.set_name("MaxPresentCurrent");
        max_present_current.copy(present_current);
        min_moving_current.set_name("MinMovingCurrent");
        min_moving_current.copy(present_current);
        min_moving_current.set(1000);

        if (present_position.size() == 0)
        {
            Error("present_position is empty. Ensure that the input is connected and has valid dimensions.");
            return;
        }

        approximating_goal.set_name("ApproximatingGoal");
        approximating_goal.copy(present_position);
        approximating_goal.set(0);
        transition_start_time.set_name("StartedTransitionTime");
        transition_start_time.copy(approximating_goal);
        transition_duration.set_name("TransitionDuration");
        transition_duration.copy(approximating_goal);

        std::string robot = robotType;
                
        servo_names = {"NeckTilt", "NeckPan", "LeftEye", "RightEye", "LeftPupil", "RightPupil", "LeftArmJoint1", "LeftArmJoint2", "LeftArmJoint3", "LeftArmJoint4", "LeftArmJoint5", "LeftHand", "RightArmJoint1", "RightArmJoint2", "RightArmJoint3", "RightArmJoint4", "RightArmJoint5", "RightHand", "Body"};
        

        if(robot == "Torso"){
            current_controlled_servos.set_name("CurrentControlledServosTorso");
            current_controlled_servos = {0, 1};
            overshot_goal_temp = {false, false};
            reached_goal = {0, 0};
            
        }
        else if(robot == "FullBody"){
            current_controlled_servos.set_name("CurrentControlledServosFullBody");
            current_controlled_servos = {0, 1, 6, 7, 8, 9, 10, 12, 13, 14, 15, 16, 18};
            overshot_goal_temp = {false, false, false, false, false, false, false, false, false, false, false, false, false};
            reached_goal = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        }
        else{
            Error("Robot type not recognized");
        }
        coeffcient_matrix = CreateCoefficientsMatrix(current_coefficients, current_controlled_servos, prediction_model, servo_names);
        reached_goal.set_name("ReachedGoal");

        overshot_goal.set_name("OvershotGoal");
        unique_id = GenerateRandomNumber(0, 1000000);

    

        // Initialize with zeros
        initial_currents = matrix(current_controlled_servos.size(), number_transitions);
        current_differences = matrix(current_controlled_servos.size(), number_transitions);
        initial_currents.set(0);
        current_differences.set(0);

        ann_output.set_name("ANN_output");
        ann_output.copy(present_current);
        ann_output.set(0);
        

        if (std::string(prediction_model) == "ANN"){
            // Open the pipe for reading
            // using interpreter from .tensorflow_venv, same directory as the script
            std::string directory = scriptPath.substr(0, scriptPath.find_last_of("/"));
            // pythonscript path same directory as current file
            std::string pythonPath = directory + "/ANN_prediction.py";
            std::string venvPath = directory + "/.tensorflow_venv/bin/python3";
            std::string command = venvPath + " " + pythonPath;
           

            ann_pipe = popen(command.c_str(), "r");
            if (!ann_pipe) {
                Error("Failed to open pipe to ANN_prediction.py");
                return;
            }
              
        }
    }

    
    void Tick()
    {   
        
      

        if (present_current.connected() && present_position.connected())
        {
            // Ensure matrices are properly sized before operations
            if (goal_position_out.size() != position_transitions.cols())
            {
                goal_position_out.resize(position_transitions.cols());
            }
            
            ReachedGoal(present_position, goal_position_out, reached_goal, position_margin);
            approximating_goal = ApproximatingGoal(present_position, goal_position_out, position_margin);

            if (GetTick() > 1)
            {
                if (std::string(prediction_model) == "ANN" && gyro.connected() && accel.connected())
                {

                    

                    // Send the following to the ANN:
                    // TiltPosition	PanPosition	GyroX	GyroY	GyroZ	AccelX	AccelY	AccelZ	tilt_distance	pan_distance
                    // concatenate the matrix present_position, gyro, accel, goal_position_out and use .json to send it to the ANN
                    matrix goal_distance = goal_position_out;
                    goal_distance.subtract(present_position);

                    // Create a single string with all values in correct order
                    std::string pipe_data = "";
                    
                    // Add positions
                    for (int i = 0; i < current_controlled_servos.size(); i++) {
                        int idx = current_controlled_servos(i);
                        pipe_data += std::to_string(present_position[idx]);
                        pipe_data += ",";
                    }
                    
                    if (gyro.size() > 0){
                    
                        // Add gyro values
                        pipe_data += std::to_string(gyro[0]) + ","; // GyroX
                        pipe_data += std::to_string(gyro[1]) + ","; // GyroY 
                        pipe_data += std::to_string(gyro[2]) + ","; // GyroZ
                    }
                    
                    if (accel.size() > 0){
                        // Add accel values
                        pipe_data += std::to_string(accel[0]) + ","; // AccelX
                        pipe_data += std::to_string(accel[1]) + ","; // AccelY
                        pipe_data += std::to_string(accel[2]) + ","; // AccelZ
                    }
                    
                    // Add distances
                    for (int i = 0; i < current_controlled_servos.size(); i++) {
                        int idx = current_controlled_servos(i);
                        pipe_data += std::to_string(abs(goal_distance[idx]));
                        if (i < current_controlled_servos.size() - 1) {
                            pipe_data += ",";
                        }
                    }
                    pipe_data += "\n";
                    

           
                    fputs(pipe_data.c_str(), ann_pipe);
                    fflush(ann_pipe);

                    // Add timeout for reading from pipe
                    fd_set fds;
                    struct timeval tv;
                    int fd = fileno(ann_pipe);

                    FD_ZERO(&fds);
                    FD_SET(fd, &fds);
                    tv.tv_sec = 1;  // 0.1 second timeout
                    tv.tv_usec = 0;

                    if (select(fd + 1, &fds, NULL, NULL, &tv) > 0) {
                        char buffer[1024];
                        if (fgets(buffer, sizeof(buffer), ann_pipe) != NULL) {
                            ann_output = atof(buffer);
                        } else {
                            Warning("Failed to read from ANN pipe");
                        }
                    } else {
                        Warning("Timeout waiting for ANN response");
                    }
                }

                if (reached_goal.sum() == current_controlled_servos.size())
                {
                    // Store the final current differences for this transition before moving to next
                    for (int i = 0; i < current_controlled_servos.size(); i++)
                    {
                        int servo_idx = current_controlled_servos(i);
                        if (current_differences(i, transition) == 0)
                        {
                            current_differences(i, transition) = present_current[servo_idx] - initial_currents(i, transition);
                        }
                    }
                    
                    transition++;
                    if (transition < number_transitions)
                    {
                        goal_position_out.copy(position_transitions[transition]);
                        reached_goal.set(0);
                        approximating_goal.set(0);
                        transition_start_time.set(GetTime());
                    }
                    else{
                        SaveCurrentData();
                        Notify(msg_terminate, "Transition finished");
                        return;
                    }
                }

                for (int i = 0; i < current_controlled_servos.size(); i++)
                {
                    int servo_idx = current_controlled_servos(i);
                    bool timeout = GetNominalTime() - transition_start_time(servo_idx) > 7.0;

                    if (approximating_goal(servo_idx) == 0 && reached_goal(servo_idx) == 0)
                    {
                        if (!timeout || GetTick() == 2)
                        {
                            goal_current.copy(SetGoalCurrent(present_current, current_increment, current_limit,
                                                             present_position, goal_position_out, position_margin,
                                                             coeffcient_matrix, std::string(prediction_model)));
                            if (initial_currents(i, transition) == 0)
                            {
                                initial_currents(i, transition) = goal_current[servo_idx];
                            }
                        }
                        else
                        {
                            goal_current(servo_idx) = std::min(abs(goal_current[servo_idx]) + 2, (float)current_limit);
                        }
                    }
                    // Update current differences continuously during movement
                    if (initial_currents(i, transition) != 0)  // Only update if we have an initial current
                    {
                        current_differences(i, transition) = present_current[servo_idx] - initial_currents(i, transition);
                    }
                }
            }
        }
        else{
            Error("Present current and present position must be connected");
            return;
        }
    }

    void SaveCurrentData()
    {
        try
        {
            std::string scriptPath = __FILE__;
            std::string scriptDirectory = scriptPath.substr(0, scriptPath.find_last_of("/\\"));
            std::string filepath = scriptDirectory + "/results/current_data_" + std::to_string(unique_id) + ".json";
            std::ofstream file(filepath);
            file << "{\n";
            file << "  \"transitions\": [\n";

            file << "    {\n";
            
            for (int i = 0; i < current_controlled_servos.size(); i++)
            {
                file << "      \"" << servo_names[current_controlled_servos(i)] << "\": {\n";
                
                file << "        \"goal_positions\": [";
                for (int t = 0; t < transition; t++) {
                    file << position_transitions(t, current_controlled_servos(i));
                    if (t < transition - 1) file << ", ";
                }
                file << "],\n";
                
                file << "        \"initial_currents\": [";
                for (int t = 0; t < transition; t++) {
                    file << initial_currents(i, t);
                    if (t < transition - 1) file << ", ";
                }
                file << "],\n";
                
                file << "        \"current_differences\": [";
                for (int t = 0; t < transition; t++) {
                    file << current_differences(i, t);
                    if (t < transition - 1) file << ", ";
                }
                file << "]\n";
                
                file << "      }";
                if (i < current_controlled_servos.size() - 1) file << ",";
                file << "\n";
            }
            
            file << "    }";
            file << "\n";
            
            file << "  ]\n";
            file << "}\n";
            file.close();
        }
        catch (const std::exception &e)
        {
            Error("Failed to save current data: " + std::string(e.what()));
        }
    }

    ~TestMapping()
    {
        if (ann_pipe) {
            pclose(ann_pipe);
            ann_pipe = nullptr;
        }
    }
};





INSTALL_CLASS(TestMapping)

