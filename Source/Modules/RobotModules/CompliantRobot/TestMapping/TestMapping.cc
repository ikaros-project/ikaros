#include "ikaros.h"
#include <random> // Include the header file that defines the 'Random' function
#include <fstream>
#include <vector>
#include <memory>
#include "json.hpp"
#include <string>
#include <algorithm>
#include <chrono>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <sys/wait.h>
#include <atomic>

using namespace ikaros;



#define SHM_NAME "/ikaros_ann_shm"
#define FLOAT_COUNT 12  // 10 inputs + 2 predictions (tilt and pan)
#define MEM_SIZE ((FLOAT_COUNT * sizeof(float)) + sizeof(bool)*2)  // Add explicit size calculation including two flags

// Structure for shared memory between Python and C++
struct SharedData
{
    float data[FLOAT_COUNT]; 
    bool new_data;           // Flag for synchronization
};

class TestMapping: public Module
{
 

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

    // Shared memory variables
    int shm_fd;
    SharedData* shared_data;

    std::string shm_name;
    
    // Add these parameters to class variables
    const double TIMEOUT_DURATION = 7.0;    // Timeout before incrementing current
    const int CURRENT_INCREMENT = 2;        // Amount to increment current during timeout
    
    // Add these to track timeout statistics
    matrix timeout_occurred;

    // New dedicated variables for prediction tracking
    matrix predicted_goal_current;
    matrix prediction_error;

    matrix starting_positions; // New matrix to store starting positions

    // Add a member variable to hold the child process ID
    pid_t child_pid = -1;

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
                
                transition_duration(transition, i) = current_time_ms - transition_start_time(transition, i);
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

        // Initialize matrix with coefficients for each servo
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

    matrix SetGoalCurrent(matrix present_current, int increment, int limit, matrix position, matrix goal_position, int margin, matrix coefficients, std::string model_name, matrix ann_output)
    {
        matrix current_output(present_current.size());
        current_output.set(0);

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
                Warning("Unsupported model type: " + model_name + " - using default current");
                estimated_current = present_current(servo_idx);
            }

            // Apply current limits and set output
            current_output(servo_idx) = std::min(abs(estimated_current), static_cast<double>(limit));
        }

        return current_output;
    }

    void StartPythonProcess()
    {
        std::string directory = __FILE__;
        // Get directory path just like you already do
        directory = directory.substr(0, directory.find_last_of("/"));
        std::string envPath = directory + "/.tensorflow_venv/bin/python3";
        std::string pythonPath = directory + "/ANN_prediction.py";

        child_pid = fork();
        if (child_pid < 0) {
             Error("Failed to fork process for Python script");
             return;
        }
        if (child_pid == 0) {
             // Child process: replace the current process image with the Python script.
             execl(envPath.c_str(), "python", pythonPath.c_str(), (char *)nullptr);
             // If exec returns, then an error occurred.
             Error("execl failed: " + std::string(strerror(errno)));
             exit(1);
        }
        // Parent process: continue execution and store child_pid for later cleanup.
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
        Bind(ann_output, "ANN_output");
        
        

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

    

        // Initialize new matrices for tracking prediction data
        initial_currents = matrix(number_transitions, current_controlled_servos.size());
        current_differences = matrix(number_transitions, current_controlled_servos.size());
        initial_currents.set(0);
        current_differences.set(0);

        // New: Initialize matrices to track the model's predicted values and prediction errors
        predicted_goal_current.set_name("PredictedGoalCurrent");
        predicted_goal_current = matrix(number_transitions, current_controlled_servos.size());
        predicted_goal_current.set(0);

        prediction_error.set_name("PredictionError");
        prediction_error = matrix(number_transitions, current_controlled_servos.size());
        prediction_error.set(0);

        ann_output.set_name("ANN_output");
        ann_output.copy(present_current);
        ann_output.set(0);
        

        if (std::string(prediction_model) == "ANN") {
            Debug("Attempting to create shared memory at " + std::string(SHM_NAME));
            
            // First, ensure any existing shared memory is removed
            shm_unlink(SHM_NAME);
            
            // Create shared memory
            shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
            if (shm_fd == -1) {
                Error("Failed to create shared memory: " + std::string(strerror(errno)) + " (errno: " + std::to_string(errno) + ")");
                return;
            }
            Debug("Successfully created shared memory. File descriptor: " + std::to_string(shm_fd));

            // Calculate size
            size_t required_size = sizeof(SharedData);
            Debug("Attempting to set shared memory size to " + std::to_string(required_size) + " bytes\n");
            
            // Set the size of shared memory
            if (ftruncate(shm_fd, required_size) == -1) {
                Error("Failed to set shared memory size (" + std::to_string(required_size) + " bytes): " + strerror(errno) + " (errno: " + std::to_string(errno) + ")");
                close(shm_fd);
                shm_unlink(SHM_NAME);
                return;
            }

            // Map the shared memory
            void* ptr = mmap(nullptr, required_size, PROT_READ | PROT_WRITE, 
                           MAP_SHARED, shm_fd, 0);
            if (ptr == MAP_FAILED) {
                Error("Failed to map shared memory: " + std::string(strerror(errno)));
                close(shm_fd);
                shm_unlink(SHM_NAME);
                return;
            }

            // Initialize the shared memory
            shared_data = static_cast<SharedData*>(ptr);
            memset(shared_data, 0, required_size);
            shared_data->new_data = false;

            // Print debug information
            Debug("Shared memory initialized:\n");
            Debug("- Required size: " + std::to_string(required_size) + " bytes\n");
            Debug("- SharedData size: " + std::to_string(sizeof(SharedData)) + " bytes\n");

            // Start Python process
            StartPythonProcess();
        }

  
        timeout_occurred.set_name("TimeoutOccurred");
        timeout_occurred = matrix(number_transitions, current_controlled_servos.size());
        timeout_occurred.set(0);

        // Initialize transition_start_time as a 2D matrix
        transition_start_time.set_name("StartedTransitionTime");
        transition_start_time = matrix(number_transitions, current_controlled_servos.size());
        transition_start_time.set(0);

        starting_positions = matrix( number_transitions, current_controlled_servos.size());
        starting_positions.set(0);
    }

    
    void Tick()
    {   
        // First check if required inputs are connected
        if (!present_current.connected() || !present_position.connected()) {
            Error("Present current and present position must be connected");
            return;
        }

        // Ensure matrices are properly sized before operations
        if (goal_position_out.size() != position_transitions.cols()) {
            goal_position_out.resize(position_transitions.cols());
        }
        
        ReachedGoal(present_position, goal_position_out, reached_goal, position_margin);
        approximating_goal = ApproximatingGoal(present_position, goal_position_out, position_margin);

        // Skip processing on first tick, just initialize start time
        if (GetTick() <= 1) {
            transition_start_time[transition].set(GetNominalTime());
            starting_positions[transition] = present_position;
            return;
        }

        // Check if all servos reached their goals for current transition
        if (reached_goal.sum() == current_controlled_servos.size()) {
            // For each current controlled servo, record the final current difference and prediction error
            for (int i = 0; i < current_controlled_servos.size(); i++) {
                int servo_idx = current_controlled_servos(i);
                if (current_differences(transition, servo_idx) == 0) {
                    current_differences(transition, servo_idx) = present_current[servo_idx] - initial_currents(transition, servo_idx);
                    goal_current[servo_idx] = present_current[servo_idx];
                    // New: Record the prediction error as (final applied current - model's predicted current)
                    prediction_error(transition, servo_idx) = abs(present_current[servo_idx] - predicted_goal_current(transition, servo_idx));
                }
            }

            transition++;
            if (transition < number_transitions) {
                // Store the starting position for the new transition
                for (int i = 0; i < current_controlled_servos.size(); i++) {
                    int servo_idx = current_controlled_servos(i);
                    starting_positions(i, transition) = present_position[servo_idx];
                    transition_start_time(transition, i) = GetNominalTime();
                    reached_goal(servo_idx) = 0;
                    approximating_goal(servo_idx) = 0;
                }

                goal_position_out.copy(position_transitions[transition]);
                goal_current.copy(present_current);

                // Reset timeout-related matrices to reuse model predictions properly
                timeout_occurred.set(0);
         

                // Compute and store the predicted goal current from the model for this new transition
                matrix all_predicted = SetGoalCurrent(present_current, current_increment, current_limit,
                                                      present_position, goal_position_out, position_margin,
                                                      coeffcient_matrix, std::string(prediction_model), ann_output);
                for (int i = 0; i < current_controlled_servos.size(); i++){
                    int servo_idx = current_controlled_servos(i);
                    predicted_goal_current(transition, servo_idx) = all_predicted[servo_idx];
                }
            } else {
                SaveCurrentData();
                Notify(msg_terminate, "Transition finished");
                return;
            }
        }

        // Process ANN predictions if using that model
        if (std::string(prediction_model) == "ANN" && gyro.connected() && accel.connected()) {
            // Write input data
            int idx = 0;
            shared_data->data[idx] = present_position[0];  // Tilt
            idx++;
            shared_data->data[idx] = present_position[1];  // Pan
            idx++;
            for (int i = 0; i < gyro.size(); i++) {
                shared_data->data[idx++] = gyro[i];
            }
            for (int i = 0; i < accel.size(); i++) {
                shared_data->data[idx++] = accel[i];
            }
            for (int i = 0; i < current_controlled_servos.size(); i++) {
                shared_data->data[idx++] = abs((float)goal_position_out[i] - (float)present_position[i]);
            }
            
            // Set new_data flag to request new predictions
            std::atomic_thread_fence(std::memory_order_release);
            shared_data->new_data = true;
            
            // Wait for Python to process (with timeout)
            int timeout_ms = 5;  // 5ms timeout
            auto start = std::chrono::steady_clock::now();
            bool got_response = false;
            
            while (!got_response) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
                
                if (elapsed > timeout_ms) {
                    Debug("Timeout waiting for ANN prediction. Elapsed time: " + std::to_string(elapsed) + "ms");
                    break;
                }

                // Read the predictions and check if they've been updated
                std::atomic_thread_fence(std::memory_order_acquire);
                if (shared_data->new_data) {
                    // Read predictions and update ann_output
                    ann_output[0] = abs(shared_data->data[FLOAT_COUNT - 2]);  // Tilt prediction
                    ann_output[1] = abs(shared_data->data[FLOAT_COUNT - 1]);  // Pan prediction
                    
                    // Clear the flag after reading
                    shared_data->new_data = false;
                    got_response = true;
                    
                    Debug("Received predictions: Tilt=" + std::to_string(ann_output[0]) + 
                          ", Pan=" + std::to_string(ann_output[1]));
                }

                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }

            if (!got_response) {
                Debug("Using previous ANN predictions due to timeout");
            }
        }

        // Process servo movements and timeouts
        for (int i = 0; i < current_controlled_servos.size(); i++) {
            int servo_idx = current_controlled_servos(i);
            double current_time = GetNominalTime();
            bool timeout = current_time - transition_start_time(transition, servo_idx) > TIMEOUT_DURATION;
            //std::cout << "Servo: " << servo_idx << " Timeout: " << current_time - transition_start_time(transition, servo_idx) << std::endl;

            // If servo hasn't reached goal yet
            if (reached_goal(servo_idx) == 0) {
                if (!timeout) {
                    // Use ANN output directly as goal current if using ANN model
                    if (std::string(prediction_model) == "ANN") {
                        goal_current(servo_idx) = std::min<float>(ann_output[servo_idx], (float)current_limit);
                    } else {
                        // For other models, use the existing prediction method
                        goal_current.copy(SetGoalCurrent(present_current, current_increment, current_limit,
                                                   present_position, goal_position_out, position_margin,
                                                   coeffcient_matrix, std::string(prediction_model), ann_output));
                    }
                } else {
                    // Handle timeout case
                    if (timeout_occurred(transition, i) == 0) {
                        timeout_occurred(transition, i) = 1;
                        Warning("Timeout started for servo " + std::string(servo_names[servo_idx]) + 
                               " in transition " + std::to_string(transition));
                    }
                    if (approximating_goal(servo_idx) == 0) {
                        goal_current(servo_idx) = std::min(goal_current[servo_idx] + CURRENT_INCREMENT, (float)current_limit);
                    }
                }
                
                // Store prediction error when servo reaches goal
                if (reached_goal(servo_idx) == 1 && prediction_error(transition, i) == 0) {
                    prediction_error(transition, i) = abs(present_current[servo_idx] - predicted_goal_current(transition, i));
                }
            }
        }

        if (GetTick() == 2 && predicted_goal_current.sum() == 0) {
            matrix all_predicted = SetGoalCurrent(present_current, current_increment, current_limit,
                                                  present_position, goal_position_out, position_margin,
                                                  coeffcient_matrix, std::string(prediction_model), ann_output);
            
            predicted_goal_current[0] = all_predicted;
            
        }
    }

    void SaveCurrentData()
    {
        try
        {
            std::string scriptPath = __FILE__;
            std::string scriptDirectory = scriptPath.substr(0, scriptPath.find_last_of("/\\"));
            std::string filepath = scriptDirectory + "/results/" + std::string(prediction_model) + "/current_data_" + std::to_string(unique_id) + ".json";
            std::ofstream file(filepath);
            file << "{\n";
            file << "  \"transitions\": [\n";

            file << "    {\n";

            for (int i = 0; i < current_controlled_servos.size(); i++)
            {
                file << "      \"" << servo_names[current_controlled_servos(i)] << "\": {\n";

                // New: Log the starting positions
                file << "        \"starting_positions\": [";
                for (int t = 0; t < transition; t++) {
                    file << starting_positions(i, t);
                    if (t < transition - 1) file << ", ";
                }
                file << "],\n";

                file << "        \"goal_positions\": [";
                for (int t = 0; t < transition; t++) {
                    file << position_transitions(t, current_controlled_servos(i));
                    if (t < transition - 1) file << ", ";
                }
                file << "],\n";

                file << "        \"predicted_goal_currents\": [";
                for (int t = 0; t < transition; t++) {
                    file << predicted_goal_current(t, i);
                    if (t < transition - 1) file << ", ";
                }
                file << "],\n";

                file << "        \"current_differences\": [";
                for (int t = 0; t < transition; t++) {
                    file << current_differences(i, t);
                    if (t < transition - 1) file << ", ";
                }
                file << "],\n";

                // Add timeout statistics
                file << "        \"timeout_occurred\": [";
                for (int t = 0; t < transition; t++) {
                    file << timeout_occurred(t, i);
                    if (t < transition - 1) file << ", ";
                }
                file << "],\n";

            

                // New: Log the prediction errors
                file << "        \"prediction_errors\": [";
                for (int t = 0; t < transition; t++) {
                    file << prediction_error(t, i);
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

    void SignalShutdownToANN()
    {
        if (shared_data) {
            // Set shutdown flag (e.g., to 1) at the last byte of shared memory
            char shutdown_val = 1;
            // The shutdown flag is at offset MEM_SIZE - 1
            memcpy(((char*)shared_data) + MEM_SIZE - 1, &shutdown_val, sizeof(shutdown_val));
            // Ensure the write completes before proceeding
            std::atomic_thread_fence(std::memory_order_release);
        }
    }

    ~TestMapping()
    {
        if (std::string(prediction_model) == "ANN") {
            // Signal shutdown through shared memory, so the Python script can exit cleanly
            SignalShutdownToANN();
            sleep(1);  // Give the Python process time to notice the shutdown flag

            // If we have a valid child process ID, terminate it
            if (child_pid > 0) {
                kill(child_pid, SIGTERM);
                waitpid(child_pid, nullptr, 0);
            }

            if (shared_data != nullptr) {
                munmap(shared_data, sizeof(SharedData));
            }
            if (shm_fd != -1) {
                close(shm_fd);
            }
            shm_unlink(SHM_NAME);

            // Additionally, kill any lingering process using port 8000 as a fallback
            system("lsof -ti:8000 | xargs kill -9 2>/dev/null || true");
        }
    }
};





INSTALL_CLASS(TestMapping)


