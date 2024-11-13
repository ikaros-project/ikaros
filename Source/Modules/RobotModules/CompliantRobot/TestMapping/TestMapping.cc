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
  
    matrix present_position;
    matrix present_current;

    
    matrix goal_current;
    matrix goal_position_out;
    matrix current_controlled_servos;
    matrix max_present_current;
    matrix min_moving_current;
    matrix min_torque_current;
    matrix overshot_goal;
    matrix overshot_goal_temp;
    matrix gyro;
    matrix accel;
    matrix eulerAngles;
    matrix position_transitions;
    std::vector<std::shared_ptr<std::vector<float>>> moving_trajectory;
  
    matrix approximating_goal;
    matrix reached_goal;
   
    dictionary current_coefficients;
    matrix coeffcient_matrix;

    parameter num_transitions;
    parameter min_limits;
    parameter max_limits;
    parameter robotType;
    parameter current_function;

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

    std::vector<std::string> servo_names;
  
   



    matrix RandomisePositions(int num_transitions, matrix min_limits, matrix max_limits, std::string robotType){
        // Initialize the random number generator with the seed
        std::mt19937 gen(rd());
        int num_columns = (robotType == "Torso") ? 2 : 12;
        matrix generated_positions(num_transitions, num_columns);
        generated_positions.set(180);
        generated_positions.info();
        if (num_columns != min_limits.size() && num_columns != max_limits.size()){
    
            Error("Min and Max limits must have the same number of columns as the current controlled servos in the robot type (2 for Torso, 12 for full body)");
            return -1;
        }
        
        min_limits.print();
        max_limits.print();

        for (int i = 0; i < num_transitions; i++) {
            for (int j = 0; j < num_columns; j++) {
                std::uniform_real_distribution<double> distr(min_limits(j), max_limits(j));
                generated_positions(i, j) = int(distr(gen));
                if (i == num_transitions-1){
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

    matrix ApproximatingGoal(matrix present_position,  matrix goal_position, int margin){
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
        coefficients_matrix.info();

        return coefficients_matrix;
    }

    matrix SetGoalCurrent(matrix present_current, int increment, int limit, matrix position, matrix goal_position, int margin, matrix coefficients, std::string model_name)
    {
        Debug("Inside SetGoalCurrent()");
        matrix current_output(present_current.size());
        double estimate_std;
        // Use the values (they are in the same order as parameter_labels)
        double CurrentMean;
        double CurrentStd;
        double betas_linear_DistanceToGoal;
        double betas_linear_Position;
        double betas_DistanceToGoal_squared; 
        double betas_Position_squared ;       
        double intercept;
        double distanceToGoal;;
        if (present_current.size() != current_output.size())
        {
            Error("Present current and Goal current must be the same size");
            return -1;
        }
        for (int i = 0; i < current_controlled_servos.size(); i++)
        {
            int servo_indx = current_controlled_servos(i);
            if (goal_current(servo_indx) < limit && abs(goal_position(servo_indx) - position(servo_indx)) > margin)
            {

                try
                {
                    
                    // Use row i of the coefficients matrix instead of row 0
                    CurrentMean = coefficients(i, 0);
                    CurrentStd = coefficients(i, 1);
                    betas_linear_DistanceToGoal = coefficients(i, 2);
                    betas_linear_Position = coefficients(i, 3);
                    betas_DistanceToGoal_squared = coefficients(i, 4);
                    betas_Position_squared = coefficients(i, 5);
                    intercept = coefficients(i, 6);
                    distanceToGoal = goal_position(servo_indx) - position(servo_indx);

                    estimate_std = intercept + betas_linear_Position * position(servo_indx) +
                                          betas_linear_DistanceToGoal * distanceToGoal +
                                          betas_Position_squared * std::pow(position(servo_indx), 2) +
                                          betas_DistanceToGoal_squared * std::pow(distanceToGoal, 2);
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Error: " << e.what() << std::endl;
                }
                
                // Convert the estimated current from standardised scale to the actual current
                current_output(servo_indx) = estimate_std * CurrentStd + CurrentMean;
            }
            else
            {
                current_output(servo_indx) = min(goal_current(servo_indx), limit);
            }
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
        Bind(current_function, "CurrentFunction");

        std::string scriptPath = __FILE__;
        
        //go up in the directory to get to the folder containing the coefficients.json file
        std::string coefficientsPath = scriptPath.substr(0, scriptPath.find_last_of("/"));
        coefficientsPath = coefficientsPath.substr(0, coefficientsPath.find_last_of("/"));
        coefficientsPath = coefficientsPath + "/CurrentPositionMapping/models/coefficients.json";
        current_coefficients.load_json(coefficientsPath);

        current_coefficients.print();

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
        coeffcient_matrix = CreateCoefficientsMatrix(current_coefficients, current_controlled_servos, current_function, servo_names);
        reached_goal.set_name("ReachedGoal");

        overshot_goal.set_name("OvershotGoal");
        unique_id = GenerateRandomNumber(0, 1000000);

        position_transitions.print();
    }

    
    void Tick()
    {   
        if (present_current.connected() && present_position.connected() ){
            ReachedGoal(present_position, goal_position_out, reached_goal, position_margin);
            approximating_goal = ApproximatingGoal(present_position, goal_position_out, position_margin);

            if (GetTick() > 1){
                if (reached_goal.sum() == current_controlled_servos.size())
                {
                    Debug("Reached goal");
                    transition++;
                    if (transition < number_transitions)
                    {
                        goal_position_out.copy(position_transitions[transition]);
                        Debug("New goal position");

                        reached_goal.set(0);
                        approximating_goal.set(0);
                        transition_start_time.set(GetTime());
                    }
                    else{
                    Print( "All transitions completed");
                }

                }
                for (int i = 0; i < current_controlled_servos.size(); i++){
                    if (approximating_goal(current_controlled_servos(i)) == 0){
                        goal_current = SetGoalCurrent(present_current, current_increment, current_limit, present_position, goal_position_out, position_margin, coeffcient_matrix, current_function);
                        goal_current.print();
                    }
                    else{
                        Trace("Approximating goal");
                    }
                }
                
            }
        }

        else{
            Error("Present current and present position must be connected");
            return;
        }
    }

};





INSTALL_CLASS(TestMapping)

