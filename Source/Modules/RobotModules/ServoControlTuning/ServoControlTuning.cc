//
//	ServoControlTuning.cc		This file is a part of the IKAROS project
//
//   Most of the functions are based on the EpiServos.cc
//    See http://www.ikaros-project.org/ for more information.
//

// Dynamixel settings
#define PROTOCOL_VERSION 2.0 // See which protocol version is used in the Dynamixel
#define BAUDRATE1M 1000000   // XL-320 is limited to 1Mbit
#define BAUDRATE3M 3000000   // MX servos


#define ADDR_P 84
#define ADDR_I 82
#define ADDR_D 80
#define ADDR_TORQUE_ENABLE 64
#define ADDR_GOAL_POSITION 116
#define ADDR_GOAL_CURRENT 102
#define ADDR_PROFILE_ACCELERATION 108
#define ADDR_PROFILE_VELOCITY 112
#define ADDR_PRESENT_POSITION 132
#define ADDR_PRESENT_CURRENT 126
#define ADDR_PRESENT_TEMPERATURE 146

#define ADDR_MIN_POSITION_LIMIT 48
#define ADDR_MAX_POSITION_LIMIT 52

#define INDIRECTADDRESS_FOR_WRITE      168                  
#define INDIRECTADDRESS_FOR_READ       578                  
#define INDIRECTDATA_FOR_WRITE         224
#define INDIRECTDATA_FOR_READ          634
//#define LEN_INDIRECTDATA_FOR_WRITE     25 // Torque Enable(1) + P(2) + I(2) + D(2)  + Goal current(2)+ Profile Acceleration (4) + Profile Velocity (4) +  Goal Position(4) 
//#define LEN_INDIRECTDATA_FOR_READ      5 //27 // Present Temperature(1)+ P(2) + I(2) + D(2)  + Goal current(2)+ Present Current (2) Profile Acceleration (4) + Profile Velocity (4) +  Goal Position(4) + Present Position(4) 


// ID of each dynamixel chain.
#define HEAD_ID_MIN 2
#define HEAD_ID_MAX 5

#define ARM_ID_MIN 2
#define ARM_ID_MAX 7

#define BODY_ID_MIN 2
#define BODY_ID_MAX 2

#define PUPIL_ID_MIN 2
#define PUPIL_ID_MAX 3

#define EPI_TORSO_NR_SERVOS 6
#define EPI_NR_SERVOS 19

#define TIMER_POWER_ON 2           // Timer ramping up sec
#define TIMER_POWER_OFF 5          // Timer ramping down sec
#define TIMER_POWER_OFF_EXTENDED 3 // Timer until torque enable off sec

#define HEAD_INDEX_IO 0
#define PUPIL_INDEX_IO 4
#define LEFT_ARM_INDEX_IO 6
#define RIGHT_ARM_INDEX_IO 12
#define BODY_INDEX_IO 18

#define MAX_TEMPERATURE 65


// TODO:
// Add fast sync write feature


#include <stdio.h>
#include <vector> // Data from dynamixel sdk
#include <future> // Threads
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "json.hpp"
#include <string>
#include <algorithm>


using json = nlohmann::json;


#include "ikaros.h"

// This must be after ikaros.h
#include "dynamixel_sdk.h" // Uses Dynamixel SDK library

using namespace ikaros;

typedef struct
{
    std::string serialPortPupil;
    std::string serialPortHead;
    std::string serialPortBody;
    std::string serialPortLeftArm;
    std::string serialPortRightArm;
    std::string type;

} Robot_parameters;

class ServoControlTuning : public Module
{
    // Paramteters
    int robotType = 0;
    matrix servoParameters;
    parameter servoToTuneName;
    parameter numberTransitions;
    parameter runSequence;
    matrix minLimitPosiiton;
    matrix maxLimitPosition;
    parameter parametersToTune;
    parameter parametersToRead;
    parameter save;

    // Ikaros IO
    bool torqueEnable = true;
    matrix position;
    matrix current;
   

    matrix dynamixelParameters;

    bool EpiTorsoMode = false;
    bool EpiMode = false;

    int AngleMinLimitPupil[2];
    int AngleMaxLimitPupil[2]; 
    //std::map<std::string, int> servoIDs;
    dictionary servoIDs;
    dictionary parameterAddresses;
    dictionary parameterBytes;
    dictionary parameterTable;
    std::map<std::string, int> servoIndices;
    int servoIndex;
    
    int servoToTune;
    int len_read_data;
    int len_write_data;


    

    matrix servoTransitions;
    int transitionIndex = 0;

    
    int goalPosition;
    int goalCurrent;
    tick_count tick;
    int previousGoalPosition;

    list tuningList;
    list readingList;
    list parameter_lst;

   


    dynamixel::PortHandler *portHandlerHead;
    dynamixel::PacketHandler *packetHandlerHead;
    dynamixel::GroupSyncRead *groupSyncReadHead;
    dynamixel::GroupSyncWrite *groupSyncWriteHead;

    dynamixel::PortHandler *portHandlerPupil;
    dynamixel::PacketHandler *packetHandlerPupil;
    dynamixel::GroupSyncRead *groupSyncReadPupil;
    dynamixel::GroupSyncWrite *groupSyncWritePupil;

    dynamixel::PortHandler *portHandlerLeftArm;
    dynamixel::PacketHandler *packetHandlerLeftArm;
    dynamixel::GroupSyncRead *groupSyncReadLeftArm;
    dynamixel::GroupSyncWrite *groupSyncWriteLeftArm;

    dynamixel::PortHandler *portHandlerRightArm;
    dynamixel::PacketHandler *packetHandlerRightArm;
    dynamixel::GroupSyncRead *groupSyncReadRightArm;
    dynamixel::GroupSyncWrite *groupSyncWriteRightArm;

    dynamixel::PortHandler *portHandlerBody;
    dynamixel::PacketHandler *packetHandlerBody;
    dynamixel::GroupSyncRead *groupSyncReadBody;
    dynamixel::GroupSyncWrite *groupSyncWriteBody;

    // Vectors for iteration
    std::vector<dynamixel::PortHandler*> portHandlers;
    std::vector<dynamixel::PacketHandler*> packetHandlers;

    std::string robotName;
    std::map<std::string, Robot_parameters> robot;


    bool CommunicationPupil(int position)
    {
        
        int dxl_comm_result = COMM_TX_FAIL; // Communication result
        bool dxl_addparam_result = false;   // addParam result
        bool dxl_getdata_result = false;    // GetParam result
        uint8_t dxl_error = 0;              // Dynamixel error

        // Send to pupil. No feedback

        uint16_t param_default = position; // Not using degrees.
        // Goal postiion feature/bug. If torque enable = 0 and goal position is sent. Torque enable will be 1.
        for (int i = PUPIL_ID_MIN; i <= PUPIL_ID_MAX; i++)
        {
             if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, i, 30, param_default, &dxl_error))
        {
            Notify(msg_warning, std::string("[ID:%03d] write2ByteTxRx failed") + std::to_string(i)); 
            portHandlerPupil->clearPort();
            return false;
        }
         
        }
       
    
        
    
        // XL 320 has no current position mode. Ignores goal current input
        // No feedback from pupils. Also no temperature check. Bad idea?
    
        return (true);
    }

    bool Communication(int ID, int IOIndex, matrix parameterValues, list parameterToTune, dynamixel::PortHandler *portHandler, dynamixel::PacketHandler *packetHandler, dynamixel::GroupSyncRead *groupSyncRead, dynamixel::GroupSyncWrite *groupSyncWrite)
    {   
        Notify(msg_debug, "Communication\n");
        if (portHandler == NULL) // If no port handler return true. Only return false if communication went wrong.
            return true;

        int index = servoIndex;
        int dxl_comm_result = COMM_TX_FAIL; // Communication result
        bool dxl_addparam_result = false;   // addParam result
        bool dxl_getdata_result = false;    // GetParam result

        uint8_t dxl_error = 0;       // Dynamixel error
        uint8_t param_sync_write[len_write_data]; //
        uint8_t dxl_present_temperature = 0;
        uint8_t dxl_torque_enable = 0;

        int16_t dxl_present_current = 0;
        uint16_t dxl_goal_current = 0;
        uint32_t dxl_present_position = 0;
        int byte_increment = 0;
        int val;

        if (!groupSyncRead->addParam(ID))
        {
            std::cout << "Add param for syncread failed" << std::endl;
            groupSyncWrite->clearParam();
            groupSyncRead->clearParam();
            return false;
        }
      

      

        // Sync read
        dxl_comm_result = groupSyncRead->txRxPacket();

        if (dxl_comm_result != COMM_SUCCESS) {
            std::cout << "Sync read failed" << std::endl;
            // Display a detailed communication error
            std::cout << "Communication Error: " 
            << groupSyncRead->getPacketHandler()->getTxRxResult(dxl_comm_result) 
            << std::endl;
            // Optionally show the packet error if applicable
            std::cout << "Packet Error: " 
            << groupSyncRead->getPacketHandler()->getRxPacketError(dxl_error) 
            << std::endl;
            groupSyncWrite->clearParam();
            groupSyncRead->clearParam();
            return false;
        }

        // Check if data is available
        dxl_comm_result = groupSyncRead->isAvailable(ID, INDIRECTDATA_FOR_READ, len_read_data);
        if (!dxl_comm_result)
        {
            std::cout << "Data not available" << std::endl;
            groupSyncWrite->clearParam();
            groupSyncRead->clearParam();
            return false;
        }

      

        //iterate through all the dxl_read_data
        dxl_present_temperature = groupSyncRead->getData(ID, INDIRECTDATA_FOR_READ, 1); // Present temperature    
        byte_increment += 1;
        dxl_present_current = groupSyncRead->getData(ID, INDIRECTDATA_FOR_READ + byte_increment, 2); // Present current
        byte_increment += 2;
        std::cout << "adress for present position data = " << INDIRECTDATA_FOR_READ + byte_increment << std::endl;
        dxl_present_position = groupSyncRead->getData(ID, INDIRECTDATA_FOR_READ + byte_increment, 4); // Present position
        

        std::cout << "Present position (raw): " << dxl_present_position << std::endl;
        std::cout << "Present current (raw): " << dxl_present_current << std::endl;
        std::cout << "Present temperature (raw): " << dxl_present_temperature << std::endl;

       

        std::cout << "Position (degrees): " << dxl_present_position / 4095.0 * 360.0 << std::endl;
        std::cout << "Current (mA): " << abs(dxl_present_current) * 3.36 << std::endl;

        // dxl_comm_result = packetHandler->read4ByteTxRx(portHandlerHead, ID, ADDR_PRESENT_POSITION, &dxl_present_position, &dxl_error);
        // if (dxl_comm_result != COMM_SUCCESS) {
        //     std::cout << "Failed to get present position! Result: " << packetHandler->getTxRxResult(dxl_comm_result) << std::endl;
        // } else if (dxl_error != 0) {
        //     std::cout << "Error getting present position! Error: " << packetHandler->getRxPacketError(dxl_error) << std::endl;
        // }
        // else{
        //     std::cout << "Present position with reading functiton: " << dxl_present_position << std::endl;
        // }

        position[0] = dxl_present_position / 4095.0 * 360.0; // degrees
        current[0] = abs(dxl_present_current) * 3.36; // mA
        // Send (sync write)
        index = IOIndex;
        param_sync_write[0] = 1; // Torque on

        if (parameterValues.connected())
        {   
        
            int paramIndex = 1;
            for (int i = 0; i < parameterValues.size(); i++)
            {
             
                if(parameterToTune[i].equals("Goal Position")){

                    //Goal position
                    if(!runSequence){
                        val = parameterValues[i] / 360.0 * 4096.0;
                    }
                    else{
                        val = goalPosition / 360.0 * 4096.0;
                    }
                    std::cout << "Goal position: " << val * 360 /4096 << std::endl;
                }
                else if (parameterToTune[i].equals("Goal Current")){
                    val = parameterValues[i] / 3.36;
                    std::cout << "Goal current: " << val << std::endl;
                }
                else if (parameterToTune[i].equals("Goal PWM")){
                    val = parameterValues[i] /0.11299; // 0.11299 is the unit scale from percentage to goal PWM 
                    std::cout << "Goal PWM: " << val << std::endl;
                }
                else{
                    val = parameterValues[i];
                }
                                
                if (parameterTable[parameterToTune[i]]["Bytes"] == 2){
                    param_sync_write[paramIndex] = DXL_LOBYTE(DXL_LOWORD(val));
                    param_sync_write[paramIndex + 1] = DXL_HIBYTE(DXL_LOWORD(val));
                }
                else if (parameterTable[parameterToTune[i]]["Bytes"] == 4){
                    param_sync_write[paramIndex] = DXL_LOBYTE(DXL_LOWORD(val));
                    param_sync_write[paramIndex + 1] = DXL_HIBYTE(DXL_LOWORD(val));
                    param_sync_write[paramIndex + 2] = DXL_LOBYTE(DXL_HIWORD(val));
                    param_sync_write[paramIndex + 3] = DXL_HIBYTE(DXL_HIWORD(val));
                }
                paramIndex += parameterTable[parameterToTune[i]]["Bytes"];
            }
        }
        else
        {
            Notify(msg_fatal_error, "ParameterValues not connected\n");
            groupSyncWrite->clearParam();
            groupSyncRead->clearParam();
            return false;
        }

        dxl_addparam_result = groupSyncWrite->addParam(ID, param_sync_write);
        if (!dxl_addparam_result)
        {
            Notify(msg_fatal_error, "Add param failed\n");
            std::cout << "result: "
            << groupSyncWrite->getPacketHandler()->getTxRxResult(dxl_comm_result) 
            << "error: " << groupSyncWrite->getPacketHandler()->getRxPacketError(dxl_error) << std::endl;
            groupSyncWrite->clearParam();
            groupSyncRead->clearParam();
            return false;
        }

        // Syncwrite
        dxl_comm_result = groupSyncWrite->txPacket();
        if (dxl_comm_result != COMM_SUCCESS)
        {
            std::cout << "Sync write failed" << std::endl;
            // Display a detailed communication error
            std::cout << "Communication Error: "
            << groupSyncWrite->getPacketHandler()->getTxRxResult(dxl_comm_result)
            << std::endl;
            groupSyncWrite->clearParam();
            groupSyncRead->clearParam();
            return false;
        }
        //read goal current
        dxl_comm_result = packetHandler->read2ByteTxRx(portHandler, ID, ADDR_GOAL_CURRENT, &dxl_goal_current, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS) {
            std::cout << "Failed to get goal current! Result: " << packetHandler->getTxRxResult(dxl_comm_result) << std::endl;
        } else if (dxl_error != 0) {
            std::cout << "Error getting goal current! Error: " << packetHandler->getRxPacketError(dxl_error) << std::endl;
        }
        else{
            std::cout << "dxl Goal current: " << dxl_goal_current << std::endl;
        }

        groupSyncWrite->clearParam();
        groupSyncRead->clearParam();
        return true;
    }

    bool ReachedGoal(int present_position, int goal_position, int margin){   
            if (abs(present_position - goal_position) > margin){
                Notify(msg_debug, "Not reached goal");
                return false;
            }
        
            std::cout << "Reached goal" << std::endl;
            return true;
        }

    
    void SaveParameterValuesAsJson(int ID, matrix parameterValues, const std::string& robotType, const std::string& servoChain, list parameternames) {
        // Construct the filename
        std::string filename = "ServoParameters" + robotType + ".json";

        // Initialize JSON object
        nlohmann::json jsonData;

        // Read existing JSON data if the file exists
        std::ifstream infile(filename);
        if (infile.is_open() && infile.peek() != std::ifstream::traits_type::eof()) {
            infile >> jsonData;
            infile.close();
        }

        // Access or create the robot type and servo chain structure
        nlohmann::json& robotData = jsonData[robotType];
        nlohmann::json& servoChainData = robotData[servoChain];

        // Check if the servo ID already exists
        bool servoExists = false;
        for (auto& servo : servoChainData) {
            if (servo["servoID"] == ID) {
                servoExists = true;
                // Update the parameter values
                for (size_t i = 0; i < parameternames.size(); ++i) {
                    std::string parameterName = parameternames[i];
                    float parameterValue = parameterValues[i];
                    servo[parameterName] = parameterValue;
                }
                break;
            }
        }

        // If the servo ID does not exist, add a new entry
        if (!servoExists) {
            nlohmann::json newServo;
            newServo["servoID"] = ID;
            for (size_t i = 0; i < parameternames.size(); ++i) {
                std::string parameterName = parameternames[i];
                float parameterValue = parameterValues[i];
                newServo[parameterName] = parameterValue;
            }
            servoChainData.push_back(newServo);
        }

        // Write the updated JSON data back to the file
        std::ofstream outfile(filename);
        if (!outfile.is_open()) {
            std::cerr << "Could not open file for writing: " << filename << std::endl;
            return;
        }
        outfile << jsonData.dump(4); // Pretty print with an indent of 4 spaces
        outfile.close();
    }

    
        // Helper function to trim whitespace from both ends
    static inline std::string trim(const std::string& str) {
        auto start = str.find_first_not_of(" \t\n\r\f\v");
        auto end = str.find_last_not_of(" \t\n\r\f\v");
        return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
    }

    list StringSplit(const std::string& str, const std::string& delimiter) {
        list result;
        size_t pos = 0;
        size_t prev = 0;
        while ((pos = str.find(delimiter, prev)) != std::string::npos) {
            result.push_back(trim(str.substr(prev, pos - prev)));
            prev = pos + delimiter.length();
        }
        result.push_back(trim(str.substr(prev)));
        return result;
    }


    // Helper function to convert string to lowercase
    static inline std::string toLowerCase(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                    [](unsigned char c) { return std::tolower(c); });
        return result;
    }
        
    matrix CreateDynamixelParameterMatrix(list parameter_lst, dictionary controlTable){
        matrix result(2, parameter_lst.size());
        std::cout << "CreateDynamixelParameterMatrix\n";
        for (int i=0; i<parameter_lst.size(); i++){
            std::string parameter = parameter_lst[i];
            if (!controlTable.contains(parameter)){

                Notify(msg_fatal_error, "Parameter not found in control table: " + parameter);
                continue;
            }
            parameter = toLowerCase(trim(parameter)); // Normalize search key
           
            // Normalize all dictionary keys for comparison
            for (const auto& [key, value] : controlTable) {
                if (toLowerCase(trim(key)) == parameter) {
                    //insert value into two different rows
                    result(0,i) = controlTable[key]["Address"];
                    result(1, i) = controlTable[key]["Bytes"];
                    
                    break;
                }

            } 
        }
        return result;
    }

    void CalculateByteLengths(dictionary controlTable, list readingParameters, list tuningParameters){
        len_read_data = 1; // Present temperature
        len_write_data = 1; // Torque enable
        for (int i=0; i<readingParameters.size(); i++){
            std::string parameter = readingParameters[i];
            parameter = toLowerCase(trim(parameter)); // Normalize search key

            // Normalize all dictionary keys for comparison
            for (const auto& [key, value] : controlTable) {
                if (toLowerCase(trim(key)) == parameter) {
                    //insert value into two different rows
                    len_read_data += controlTable[key]["Bytes"];
                    break;
                }
            } 
        }
        for(int i=0; i<tuningParameters.size(); i++){
            std::string parameter = tuningParameters[i];
            parameter = toLowerCase(trim(parameter)); // Normalize search key

            // Normalize all dictionary keys for comparison
            for (const auto& [key, value] : controlTable) {
                if (toLowerCase(trim(key)) == parameter) {
                    //insert value into two different rows
                    len_write_data += controlTable[key]["Bytes"];
                    break;
                }
            } 
        }        
    }

    void Init()
    {
        
        // Robots configurations
        robot["EpiWhite"] = {.serialPortPupil = "/dev/cu.usbserial-FT66WV4A",
                             .serialPortHead = "/dev/cu.usbserial-FT6S4JL9",
                             .serialPortBody = "",
                             .serialPortLeftArm = "",
                             .serialPortRightArm = "",
                             .type = "EpiTorso"};

        robot["EpiRed"] = {.serialPortPupil = "/dev/cu.usbserial-FT4TCJXI",
                           .serialPortHead = "/dev/cu.usbserial-FT4TCGUT",
                           .serialPortBody = "",
                           .serialPortLeftArm = "",
                           .serialPortRightArm = "",
                           .type = "EpiTorso"};

        robot["EpiRedDemo"] = {.serialPortPupil = "/dev/ttyUSB0",
                               .serialPortHead = "/dev/ttyUSB1",
                               .serialPortBody = "",
                               .serialPortLeftArm = "",
                               .serialPortRightArm = "",
                               .type = "EpiTorso"};

        robot["EpiOrange"] = {.serialPortPupil = "/dev/cu.usbserial-FT3WI2WH",
                              .serialPortHead = "/dev/cu.usbserial-FT3WI2K2",
                              .serialPortBody = "",
                              .serialPortLeftArm = "",
                              .serialPortRightArm = "",
                              .type = "EpiTorso"};

        robot["EpiYellow"] = {.serialPortPupil = "/dev/cu.usbserial-FT6S4IFI",
                              .serialPortHead = "/dev/cu.usbserial-FT6RW7PH",
                              .serialPortBody = "",
                              .serialPortLeftArm = "",
                              .serialPortRightArm = "",
                              .type = "EpiTorso"};

        robot["EpiGreen"] = {.serialPortPupil = "/dev/cu.usbserial-FT6S4JMH",
                             .serialPortHead = "/dev/cu.usbserial-FT66WT6W",
                             .serialPortBody = "",
                             .serialPortLeftArm = "",
                             .serialPortRightArm = "",
                             .type = "EpiTorso"};

        robot["EpiBlue"] = {.serialPortPupil = "/dev/cu.usbserial-FT66WS1F",
                            .serialPortHead = "/dev/cu.usbserial-FT4THUNJ",
                            .serialPortBody = "/dev/cu.usbserial-FT4THV1M",
                            .serialPortLeftArm = "/dev/cu.usbserial-FT4TFSV0",
                            .serialPortRightArm = "/dev/cu.usbserial-FT4TCVTT",
                            .type = "Epi"};

        robot["EpiGray"] = {.serialPortPupil = "/dev/cu.usbserial-FT6S4JL5",
                            .serialPortHead = "/dev/cu.usbserial-FT66WV43",
                            .serialPortBody = "",
                            .serialPortLeftArm = "",
                            .serialPortRightArm = "",
                            .type = "EpiTorso"};

        robot["EpiBlack"] = {.serialPortPupil = "/dev/cu.usbserial-FT1SEOJY",
                             .serialPortHead = "/dev/cu.usbserial-FT3WHSCR",
                             .serialPortBody = "",
                             .serialPortLeftArm = "",
                             .serialPortRightArm = "",
                             .type = "EpiTorso"};

        robotName = GetValue("robot");

        // Check if robotname exist in configuration
        if (robot.find(robotName) == robot.end())
        {
            Notify(msg_fatal_error, std::string("%s is not supported") + robotName);
            return;
        }

        // Check type of robot
        EpiTorsoMode = (robot[robotName].type.compare("EpiTorso") == 0);
        EpiMode = (robot[robotName].type.compare("Epi") == 0);
        //Notify(msg_debug, "Connecting to %s (%s)\n", robotName.c_str(), robot[robotName].type.c_str());
        
        Notify(msg_debug, "Connecting to " + robotName + " " + robot[robotName].type + "\n");

        // Ikaros input
        Bind(servoParameters, "ParameterValues");
        Bind(parametersToTune, "ParametersToTune");
        Bind(parametersToRead, "ParametersToRead");
        Bind(minLimitPosiiton, "MinLimitPosition");
        Bind(maxLimitPosition, "MaxLimitPosition");
        Bind(servoToTuneName, "Servo");
        Bind(numberTransitions, "NumberTransitions");
        Bind(position, "Position");
        Bind(current, "Current");
        Bind(runSequence, "RunSequence");
        Bind(save, "Save");

        position.set_labels(0, "Present Position", "Goal Position");
        current.set_labels(0, "Present Current", "Goal Current");

        servoParameters.set_labels(0, parametersToTune);


        std::string sID = R"({"NeckTilt": 2, "NeckPan": 3, "LeftEye": 4, "RightEye": 5, "LeftPupil": 2, "RightPupil": 3, "LeftArmJoint1": 2, "LeftArmJoint2": 3, "LeftArmJoint3": 4, "LeftArmJoint4": 5, "LeftArmJoint5": 6, "LeftHand": 7,"RightArmJoint1": 2, "RightArmJoint2": 3, "RightArmJoint3": 4, "RightArmJoint4": 5, "RightArmJoint5": 6, "RightHand": 7, "Body": 2})";
        servoIDs = parse_json(sID);

        std::string sTable = R"({"Torque Enable": {"Address": 64,"Bytes": 1},
                                    "LED": {"Address": 65,"Bytes": 1},
                                    "Status Return Level": {
                                        "Address": 68,
                                        "Bytes": 1
                                    },
                                    "Registered Instruction": {
                                        "Address": 69,
                                        "Bytes": 1
                                    },
                                    "Hardware Error Status": {
                                        "Address": 70,
                                        "Bytes": 1
                                    },
                                    "Velocity I Gain": {
                                        "Address": 76,
                                        "Bytes": 2
                                    },
                                    "Velocity P Gain": {
                                        "Address": 78,
                                        "Bytes": 2
                                    },
                                    "Position D Gain": {
                                        "Address": 80,
                                        "Bytes": 2
                                    },
                                    "Position I Gain": {
                                        "Address": 82,
                                        "Bytes": 2
                                    },
                                    "Position P Gain": {
                                        "Address": 84,
                                        "Bytes": 2
                                    },
                                    "Feedforward 2nd Gain": {
                                        "Address": 88,
                                        "Bytes": 2
                                    },
                                    "Feedforward 1st Gain": {
                                        "Address": 90,
                                        "Bytes": 2
                                    },
                                    "BUS Watchdog": {
                                        "Address": 98,
                                        "Bytes": 1
                                    },
                                    "Goal PWM": {
                                        "Address": 100,
                                        "Bytes": 2
                                    },
                                    "Goal Current": {
                                        "Address": 102,
                                        "Bytes": 2
                                    },
                                    "Goal Velocity": {
                                        "Address": 104,
                                        "Bytes": 4
                                    },
                                    "Profile Acceleration": {
                                        "Address": 108,
                                        "Bytes": 4
                                    },
                                    "Profile Velocity": {
                                        "Address": 112,
                                        "Bytes": 4
                                    },
                                    "Goal Position": {
                                        "Address": 116,
                                        "Bytes": 4
                                    },
                                    "Realtime Tick": {
                                        "Address": 120,
                                        "Bytes": 2
                                    },
                                    "Moving": {
                                        "Address": 122,
                                        "Bytes": 1
                                    },
                                    "Moving Status": {
                                        "Address": 123,
                                        "Bytes": 1
                                    },
                                    "Present PWM": {
                                        "Address": 124,
                                        "Bytes": 2
                                    },
                                    "Present Current": {
                                        "Address": 126,
                                        "Bytes": 2
                                    },
                                    "Present Velocity": {
                                        "Address": 128,
                                        "Bytes": 4
                                    },
                                    "Present Position": {
                                        "Address": 132,
                                        "Bytes": 4
                                    },
                                    "Velocity Trajectory": {
                                        "Address": 136,
                                        "Bytes": 4
                                    },
                                    "Position Trajectory": {
                                        "Address": 140,
                                        "Bytes": 4
                                    },
                                    "Present Input Voltage": {
                                        "Address": 144,
                                        "Bytes": 2
                                    },
                                    "Present Temperature": {
                                        "Address": 146,
                                        "Bytes": 1
                                    }
                                    }
                                    )" ;
        
        parameterTable = parse_json(sTable);  
        
        servoIndices = {
        {"NeckTilt", 0},
        {"NeckPan", 1},
        {"LeftEye", 2},
        {"RightEye", 3},
        {"LeftPupil", 4},
        {"RightPupil", 5},
        {"LeftArmJoint1", 6},
        {"LeftArmJoint2", 7},
        {"LeftArmJoint3", 8},
        {"LeftArmJoint4", 9},
        {"LeftArmJoint5", 10},
        {"LeftHand", 11},
        {"RightArmJoint1", 12},
        {"RightArmJoint2", 13},
        {"RightArmJoint3", 14},
        {"RightArmJoint4", 15},
        {"RightArmJoint5", 16},
        {"RightHand", 17},
        {"Body", 18}
        };

        
        servoToTune = servoIDs[servoToTuneName];
        servoIndex = servoIndices[servoToTuneName];
        tuningList = StringSplit(parametersToTune, ",");
        readingList = StringSplit(parametersToRead, ",");

       
        //combine the two lists

        for (int i = 0; i < tuningList.size(); i++)
        {
            parameter_lst.push_back(trim(tuningList[i]));
        }
        for (int i = 0; i < readingList.size(); i++)
        {
            parameter_lst.push_back(trim(readingList[i]));
        }
        
        dynamixelParameters = CreateDynamixelParameterMatrix(parameter_lst, parameterTable);

        
        

        dynamixelParameters.set_name("DynamixelParameters");
        dynamixelParameters.set_labels(1, "Address", "Bytes");


        parameter_lst.print();

        
        CalculateByteLengths(parameterTable, readingList, tuningList);
        std::cout << "len_read_data: " << len_read_data << std::endl;
        std::cout << "len_write_data: " << len_write_data << std::endl;
        
        
        std::cout << "ServoToTuneName: " << servoToTuneName << std::endl;
        std::cout << "servoToTune: " << servoToTune << std::endl;
        std::cout << "servoIndex: " << servoIndex << std::endl;
        std::cout << "Parameter to tune: " << parametersToTune << std::endl;


        servoTransitions = GenerateServoTransitions(numberTransitions, minLimitPosiiton[servoIndex], maxLimitPosition[servoIndex]);
        servoTransitions.set_name("ServoTransitions");
       

        // Epi torso
        // =========
        if (EpiTorsoMode || EpiMode)
        {
            int dxl_comm_result;
            std::vector<uint8_t> vec;

            // Neck/Eyes (id 2,3) =  2x MX106R Eyes = 2xMX28R (id 3,4)

            // Init Dynaxmixel SDK
            portHandlerHead = dynamixel::PortHandler::getPortHandler(robot[robotName].serialPortHead.c_str());
            packetHandlerHead = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);

            // Open port
            if (portHandlerHead->openPort())
                Notify(msg_debug, "Succeeded to open serial port!\n");
            else
            {
                Notify(msg_fatal_error, "Failed to open serial port!\n");
                return;
            }

            // Set port baudrate
            if (portHandlerHead->setBaudRate(BAUDRATE3M))
                Notify(msg_debug, "Succeeded to change baudrate!\n");
            else
            {
                Notify(msg_fatal_error, "Failed to change baudrate!\n");
                return;
            }

            

            // Ping all the servos to make sure they are all there.
            dxl_comm_result = packetHandlerHead->broadcastPing(portHandlerHead, vec);
            if (dxl_comm_result != COMM_SUCCESS)
                std::cout << "ping failed for head. Error: " << packetHandlerHead->getTxRxResult(dxl_comm_result) << std::endl;

            Notify(msg_debug, "Detected Dynamixel (Head): \n");
            // for (int i = 0; i < (int)vec.size(); i++)
            //     Notify(msg_debug, "[ID:%03d]\n", vec.at(i));

            // Pupil (id 2,3) = XL320 Left eye, right eye

            // Init Dynaxmixel SDK
            portHandlerPupil = dynamixel::PortHandler::getPortHandler(robot[robotName].serialPortPupil.c_str());
            packetHandlerPupil = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);

            portHandlers = {portHandlerHead};
            packetHandlers = {packetHandlerHead};

            // Open port
            if (portHandlerPupil->openPort())
                Notify(msg_debug, "Succeeded to open serial port!\n");
            else
            {
                Notify(msg_fatal_error, "\n");
                return;
            }
            // Set port baudrate
            if (portHandlerPupil->setBaudRate(BAUDRATE1M))
                Notify(msg_debug, "Succeeded to change baudrate!\n");
            else
            {
                Notify(msg_fatal_error, "Failed to change baudrate!\n");
                return;
            }
            // Ping all the servos to make sure they are all there.
            dxl_comm_result = packetHandlerPupil->broadcastPing(portHandlerPupil, vec);
            // if (dxl_comm_result != COMM_SUCCESS)
            //     Notify(msg_warning, "%s\n", packetHandlerPupil->getTxRxResult(dxl_comm_result));

            Notify(msg_debug, "Detected Dynamixel (Pupil): \n");
            // for (int i = 0; i < (int)vec.size(); i++)
            //     Notify(msg_debug, "[ID:%03d]\n", vec.at(i));
        }
        else
        {
            Notify(msg_fatal_error, "Robot type is not yet implementet\n");
        }
        if (EpiMode)
        {
            int dxl_comm_result;
            std::vector<uint8_t> vec;

            // Left arm 6x MX106R 1 MX28R

            // Init Dynaxmixel SDK
            portHandlerLeftArm = dynamixel::PortHandler::getPortHandler(robot[robotName].serialPortLeftArm.c_str());
            packetHandlerLeftArm = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);

            // Open port
            if (portHandlerLeftArm->openPort())
                Notify(msg_debug, "Succeeded to open serial port!\n");
            else
            {
                Notify(msg_fatal_error, "Failed to open serial port!\n");
                return;
            }
            // Set port baudrate
            if (portHandlerLeftArm->setBaudRate(BAUDRATE3M))
                Notify(msg_debug, "Succeeded to change baudrate!\n");
            else
            {
                Notify(msg_fatal_error, "Failed to change baudrate!\n");
                return;
            }
            // Ping all the servos to make sure they are all there.
            dxl_comm_result = packetHandlerLeftArm->broadcastPing(portHandlerLeftArm, vec);
            // if (dxl_comm_result != COMM_SUCCESS)
            //     Notify(msg_warning, "%s\n", packetHandlerLeftArm->getTxRxResult(dxl_comm_result));

            Notify(msg_debug, "Detected Dynamixel (Left arm): \n");
            // for (int i = 0; i < (int)vec.size(); i++)
            //     Notify(msg_debug, "[ID:%03d]\n", vec.at(i));

            // Right arm 6x MX106R 1 MX28R

            // Init Dynaxmixel SDK
            portHandlerRightArm = dynamixel::PortHandler::getPortHandler(robot[robotName].serialPortRightArm.c_str());
            packetHandlerRightArm = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);

            // Open port
            if (portHandlerRightArm->openPort())
                Notify(msg_debug, "Succeeded to open serial port!\n");
            else
            {
                Notify(msg_fatal_error, "Failed to open serial port!\n");
                return;
            }
            // Set port baudrate
            if (portHandlerRightArm->setBaudRate(BAUDRATE3M))
                Notify(msg_debug, "Succeeded to change baudrate!\n");
            else
            {
                Notify(msg_fatal_error, "Failed to change baudrate!\n");
                return;
            }
            // Ping all the servos to make sure they are all there.
            dxl_comm_result = packetHandlerRightArm->broadcastPing(portHandlerRightArm, vec);
            // if (dxl_comm_result != COMM_SUCCESS)
            //     Notify(msg_warning, "%s\n", packetHandlerRightArm->getTxRxResult(dxl_comm_result));

            Notify(msg_debug, "Detected Dynamixel (Right arm): \n");
            // for (int i = 0; i < (int)vec.size(); i++)
            //     Notify(msg_debug, "[ID:%03d]\n", vec.at(i));

            // Body MX106R

            // Init Dynaxmixel SDK
            portHandlerBody = dynamixel::PortHandler::getPortHandler(robot[robotName].serialPortBody.c_str());
            packetHandlerBody = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);

            // Open port
            if (portHandlerBody->openPort())
                Notify(msg_debug, "Succeeded to open serial port!\n");
            else
            {
                Notify(msg_fatal_error, "Failed to open serial port!\n");
                return;
            }
            // Set port baudrate
            if (portHandlerBody->setBaudRate(BAUDRATE3M))
                Notify(msg_debug, "Succeeded to change baudrate!\n");
            else
            {
                Notify(msg_fatal_error, "Failed to change baudrate!\n");
                return;
            }
            // Ping all the servos to make sure they are all there.
            dxl_comm_result = packetHandlerBody->broadcastPing(portHandlerBody, vec);
            if (dxl_comm_result != COMM_SUCCESS)
                Notify(msg_warning, "%s\n", packetHandlerBody->getTxRxResult(dxl_comm_result));

            Notify(msg_debug, "Detected Dynamixel (Body): \n");
            // for (int i = 0; i < (int)vec.size(); i++)
            //     Notify(msg_debug, "[ID:%03d]\n", vec.at(i));
            
            //Vectors to iterate throuh all the ports
            portHandlers = {portHandlerHead, portHandlerLeftArm, portHandlerRightArm, portHandlerBody};
            packetHandlers = {packetHandlerHead, packetHandlerLeftArm, packetHandlerRightArm, packetHandlerBody};
            
        }
        
        // Create dynamixel objects
        if (EpiTorsoMode || EpiMode)
        {
            groupSyncWriteHead = new dynamixel::GroupSyncWrite(portHandlerHead, packetHandlerHead, INDIRECTDATA_FOR_WRITE, len_write_data); // Torque enable, goal position, goal current, P, I, D, Profile acceleration, Profile velocity
            groupSyncReadHead = new dynamixel::GroupSyncRead(portHandlerHead, packetHandlerHead, INDIRECTDATA_FOR_READ,len_read_data );   // Present poistion, present current, temperature, goal current
            
        }
        if (EpiMode)
        {
            groupSyncWriteLeftArm = new dynamixel::GroupSyncWrite(portHandlerLeftArm, packetHandlerLeftArm, INDIRECTADDRESS_FOR_WRITE, len_write_data);
            groupSyncReadLeftArm = new dynamixel::GroupSyncRead(portHandlerLeftArm, packetHandlerLeftArm, INDIRECTDATA_FOR_READ, len_read_data);
            groupSyncWriteRightArm = new dynamixel::GroupSyncWrite(portHandlerRightArm, packetHandlerRightArm, INDIRECTADDRESS_FOR_WRITE, len_write_data);
            groupSyncReadRightArm = new dynamixel::GroupSyncRead(portHandlerRightArm, packetHandlerRightArm, INDIRECTDATA_FOR_READ, len_read_data);
            groupSyncWriteBody = new dynamixel::GroupSyncWrite(portHandlerBody, packetHandlerBody, INDIRECTADDRESS_FOR_WRITE, len_write_data);
            groupSyncReadBody = new dynamixel::GroupSyncRead(portHandlerBody, packetHandlerBody, INDIRECTDATA_FOR_READ, len_read_data);
            groupSyncWritePupil = new dynamixel::GroupSyncWrite(portHandlerPupil, packetHandlerPupil, 30, 2); // no read..
        }
        


        AutoCalibratePupil();

        Notify(msg_debug, "torque down servos and prepering servos for write defualt settings\n");
        if (!PowerOffRobot())
            Notify(msg_fatal_error, "Unable torque down servos\n");

        if (!SetParameterAddresses()){
            //terminate the program
            Notify(msg_fatal_error, "Unable to set indirect addresses on servos\n");
        }
        Notify(msg_debug, "Setting up indirect addresses done\n");
        if(!SetPupilParameters()){
            Notify(msg_fatal_error, "Unable to set pupil parameters\n");
            std::cout << "Unable to set pupil parameters" << std::endl;
            return;
        }
        Notify(msg_debug, "Setting up pupil done\n");

        Notify(msg_debug, "Setting up min/max limits\n");
        if(!SetMinMaxLimits())
            Notify(msg_fatal_error, "Unable to set min/max limits\n");
        Notify(msg_debug, "Setting up min/max limits done\n");
        
        

        //CheckIndirectAddressSettings(HEAD_ID_MIN, HEAD_ID_MAX);
        

        

        //std::exit(1);
        if (!PowerOnRobot())
            Notify(msg_fatal_error, "Unable torque up servos\n");
        
        Notify(msg_debug,"Init() done\n");
    }

    float PupilMMToDynamixel(float mm, int min, int max)
    {
        // Quick fix
        float minMM = 4.2;
        float maxMM = 19.7;
        float deltaMM = maxMM - minMM;

        float minDeg = min;
        float maxDeg = max;
        float deltDeg = maxDeg - minDeg;

        if (mm < minMM)
            mm = minMM;
        if (mm > maxMM)
            mm = maxMM;

        return (-(mm - minMM) / deltaMM * deltDeg + maxDeg); // Higher goal position gives smaller pupil
    }

    matrix GenerateServoTransitions(int numTransitions, int minLimitRev, int maxLimitRev)
    {
        matrix Positions(numTransitions, 1);
        float maxLimit = maxLimitRev * 360 / 4095; // Convert to degrees
        float minLimit = minLimitRev * 360 / 4095; // Convert to degrees
  
       
        // calculate the middle position
        float middle = (maxLimit + minLimit) / 2;
        float totalRange = maxLimit - minLimit;
        
        // first transitions are from middle then oscillating in small steps that increase with time to the limits
        for (float i = 0; i < numTransitions; i++)
        {
          
            float step = totalRange / 2 * (i/numTransitions);
            Positions[i] = middle + step;
            if(i+1 < numTransitions)
            {
                Positions[i+1] = middle - step;
                i++;
            }
        }
        
        return Positions;

    }

    bool SetParameterAddresses() {
        uint32_t param_default_4Byte;
        uint16_t param_default_2Byte;
        uint8_t param_default_1Byte;
        uint8_t dxl_error = 0;
        int default_value = 0;
    
        std::vector<int> maxMinPositionLimitIndex;
      

        int dxl_comm_result = COMM_TX_FAIL;
        int idMin;
        int idMax;
        int addressRead = INDIRECTADDRESS_FOR_READ;
        int addressWrite = INDIRECTADDRESS_FOR_WRITE;
        int directAddress;

        int byteLength;
        value parameterName;

        Notify(msg_debug, "Setting control table on servos\n");

        //loop through all portHandlers
        for (int p = 0; p < portHandlers.size(); p++) {

            // Ensure p is within the valid range
            if (p < 0 || p >= portHandlers.size() || p >= packetHandlers.size()) {
                std::cout << "Invalid index for portHandlers or packetHandlers: " << p << std::endl;
                return false;
            }

            // Ensure pointers are not null
            if (!portHandlers[p] || !packetHandlers[p]) {
                std::cout << "Null pointer encountered in portHandlers or packetHandlers at index: " << p << std::endl;
                return false;
            }
            //switch statement for different p values. 
            switch (p) {
                case 0:
                idMin = HEAD_ID_MIN;
                idMax = HEAD_ID_MAX;
                break;
                //same for p==1 and p==2 (right and left arm)
                case 1:
                idMin = ARM_ID_MIN;
                idMax = ARM_ID_MAX;
                break;
                case 2:
                idMin = ARM_ID_MIN;
                idMax = ARM_ID_MAX;
                break;
                case 3:
                idMin = BODY_ID_MIN;
                idMax = BODY_ID_MAX;
                break;
            }
         
            for (int id = idMin; id <= idMax; id++) 
            {   
                // Disable Dynamixel Torque :
                // Indirect address would not accessible when the torque is already enabled
                dxl_comm_result = packetHandlers[p]->write1ByteTxRx(portHandlers[p], id, ADDR_TORQUE_ENABLE, 0, &dxl_error);
                if (dxl_comm_result != COMM_SUCCESS)
                {
                    packetHandlers[p]->getTxRxResult(dxl_comm_result);
                }
                else if (dxl_error != 0)
                {
                    packetHandlers[p]->getRxPacketError(dxl_error);
                }
                

                //Setting indirect addresses for all servos
                // Torque Enable
                dxl_comm_result = packetHandlers[p]->write2ByteTxRx(portHandlers[p],id , addressWrite, ADDR_TORQUE_ENABLE, &dxl_error);
                if (dxl_comm_result != COMM_SUCCESS) {
                    std::cout << "Failed to set torque enable for servo ID: " << id 
                            << " of port:" << portHandlers[p]->getPortName() 
                            << " Error: " << packetHandlers[p]->getTxRxResult(dxl_comm_result)
                            << " DXL Error: " << packetHandlers[p]->getRxPacketError(dxl_error) << std::endl;
                    return false;
                }            
                Notify(msg_debug,"Indirect addresses Torque enable set for all servos" );
                
                // present temperature
                dxl_comm_result = packetHandlers[p]->write2ByteTxRx(portHandlers[p], id, addressRead, ADDR_PRESENT_TEMPERATURE, &dxl_error);
                if (dxl_comm_result != COMM_SUCCESS) {
                    std::cout << "Failed to set present temperature for servo ID: " << id 
                            << " of port:" << portHandlers[p]->getPortName() 
                            << " Error: " << packetHandlers[p]->getTxRxResult(dxl_comm_result)
                            << " DXL Error: " << packetHandlers[p]->getRxPacketError(dxl_error) << std::endl;
                    return false;
                }
            }

            //Setting indirect addresses for all servos
            for (int id = idMin; id <= idMax; id++) {
                addressRead = INDIRECTADDRESS_FOR_READ +2;  // Reset to base address for each servo
                addressWrite = INDIRECTADDRESS_FOR_WRITE +2; // Reset to base address for each servo

                printf("Setting control table for servo ID: %d\n", id);
                for (int param = 0; param < parameter_lst.size(); param++) {
                    byteLength = dynamixelParameters(1, param);
                    parameterName = parameter_lst[param];
                    for (int byte = 0; byte < byteLength; byte++) {
                        directAddress = dynamixelParameters(0, param) + byte;
                        
                        // Writing Indirect Addresses
                        if (!parameterName.equals("Present Current") && !parameterName.equals("Present Position")) { // Present current and present position is not used for writing
                            std::cout << "Setting indirect writing address for " 
                            << parameterName << " indirect address: "
                            << addressWrite << " direct address: " 
                            << directAddress <<std::endl;
                            
                            if (COMM_SUCCESS != packetHandlers[p]->write2ByteTxRx(portHandlers[p], id, addressWrite, directAddress, &dxl_error)) {
                                std::cout << "Failed to set indirect writing address for " << parameterName
                                        << " for servo ID: " << id
                                        << " of port:" << portHandlers[p]->getPortName()
                                        << " Error: " << packetHandlers[p]->getTxRxResult(dxl_comm_result)
                                        << " DXL Error: " << packetHandlers[p]->getRxPacketError(dxl_error) << std::endl;
                                return false;
                            }
                        
                            // Increment address after each byte
                            addressWrite += 2;
                        }
                        else {// Only present position and present current is used for reading
                            std::cout << "Setting indirect reading address for " 
                            << parameterName << " indirect address: "
                            << addressRead << " direct address: " 
                            << directAddress <<std::endl;
                            // Reading Indirect Addresses
                            
                            if (COMM_SUCCESS != packetHandlers[p]->write2ByteTxRx(portHandlers[p], id, addressRead, directAddress, &dxl_error)) {
                                std::cout << "Failed to set indirect reading address for " << dynamixelParameters.labels()[param]
                                        << " for servo ID: " << id
                                        << " of port:" << portHandlers[p]->getPortName()
                                        << " Error: " << packetHandlers[p]->getTxRxResult(dxl_comm_result)
                                        << " DXL Error: " << packetHandlers[p]->getRxPacketError(dxl_error) << std::endl;
                                return false;
                            }

                            // Increment address after each byte
                            addressRead += 2;

                        }
                       

                        
                        
                    }//for byte
                }
            }//for id
            
            
           
            
        }//for portHandlers
      
        


        return true; // Yay we manage to set everything we needed.
    }

    bool SetPupilParameters(){
        Notify(msg_debug, "Settting parameters for pupils (servo XL320)\n");
        uint16_t pupil_moving_speed = 150;
        uint8_t pupil_p_gain = 100;
        uint8_t pupil_i_gain = 20;
        uint8_t pupil_d_gain = 5;
        uint8_t dxl_error = 0;

        Timer t;
        double xlTimer = 0.01; // Timer in sec. XL320 need this. Not sure why.

        // PUPIL ID 2 (Left pupil)
        // Limit position min
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 6, AngleMinLimitPupil[0], &dxl_error))
            return false;
        Sleep(xlTimer);
        // Limit position max
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 8, AngleMaxLimitPupil[0], &dxl_error))
            return false;
        Sleep(xlTimer);

        // Moving speed
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 32, pupil_moving_speed, &dxl_error))
            return false;
        Sleep(xlTimer);

        // P
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 29, pupil_p_gain, &dxl_error))
            return false;
        Sleep(xlTimer);


        // I
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 28, pupil_i_gain, &dxl_error))
            return false;
        Sleep(xlTimer);


        // D
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 27, pupil_d_gain, &dxl_error))
            return false;
        Sleep(xlTimer);


        // PUPIL ID 3 (Right pupil)
        // Limit position in
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 6, AngleMinLimitPupil[1], &dxl_error))
            return false;
        Sleep(xlTimer);


        // Limit position max
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 8, AngleMaxLimitPupil[1], &dxl_error))
            return false;
        Sleep(xlTimer);


        // Moving speed
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 32, pupil_moving_speed, &dxl_error))
            return false;
        Sleep(xlTimer);

        // P
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 29, pupil_p_gain, &dxl_error))
            return false;
        Sleep(xlTimer);


        // I
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 28, pupil_i_gain, &dxl_error))
            return false;
        Sleep(xlTimer);


        // D
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 27, pupil_d_gain, &dxl_error))
            return false;
        Sleep(xlTimer);
        std::cout << "Pupil parameters set" << std::endl;

        Notify(msg_debug, "Pupil parameters done\n");

        return true;
    }

    bool SetMinMaxLimits(){
        uint32_t param_default_4Byte;
        uint8_t dxl_error = 0;
        int idMin;
        int idMax;
    
        std::vector<int> maxMinPositionLimitIndex;
        int i = 0;
        //min and max limits
        for (int p = 0; p < portHandlers.size(); p++) {

            // Ensure p is within the valid range
            if (p < 0 || p >= portHandlers.size() || p >= packetHandlers.size()) {
                std::cout << "Invalid index for portHandlers or packetHandlers: " << p << std::endl;
                return false;
            }

            // Ensure pointers are not null
            if (!portHandlers[p] || !packetHandlers[p]) {
                std::cout << "Null pointer encountered in portHandlers or packetHandlers at index: " << p << std::endl;
                return false;
            }
            //switch statement for different p values. 
            switch (p) {
                case 0:
                maxMinPositionLimitIndex = {0, 1, 2, 3};
                idMin = HEAD_ID_MIN;
                idMax = HEAD_ID_MAX;
                break;
                //same for p==1 and p==2 (right and left arm)
                case 1:
                maxMinPositionLimitIndex = {4, 5, 6, 7, 8, 9};
                idMin = ARM_ID_MIN;
                idMax = ARM_ID_MAX;
                break;
                case 2:
                maxMinPositionLimitIndex = {10, 11, 12, 13, 14, 15};
                idMin = ARM_ID_MIN;
                idMax = ARM_ID_MAX;
                break;
                case 3:
                maxMinPositionLimitIndex = {16};
                idMin = BODY_ID_MIN;
                idMax = BODY_ID_MAX;
                break;
                }
            for (int id = idMin; id <= idMax; id++) 
            {
                //min and max limits
                param_default_4Byte = minLimitPosiiton[maxMinPositionLimitIndex[i]];
                if (COMM_SUCCESS != packetHandlers[p]->write4ByteTxRx(portHandlers[p], id, ADDR_MIN_POSITION_LIMIT, param_default_4Byte, &dxl_error)){
                    std::cout << "Failed to set indirect address for min position limit for servo ID: " 
                    << id << " of port:" 
                    << portHandlers[p]->getPortName() 
                    << ", DXL Error: " << packetHandlers[p]->getRxPacketError(dxl_error) << std::endl;
                    
                    return false;
                }
                param_default_4Byte = maxLimitPosition[maxMinPositionLimitIndex[i]];
                if (COMM_SUCCESS != packetHandlers[p]->write4ByteTxRx(portHandlers[p], id, ADDR_MIN_POSITION_LIMIT, param_default_4Byte, &dxl_error)){
                    std::cout << "Failed to set indirect address for max position limit for servo ID: " 
                    << id << " of port:" 
                    << portHandlers[p]->getPortName() 
                    << ", DXL Error: " << packetHandlers[p]->getRxPacketError(dxl_error) << std::endl;
                
                    return false;
                }
                i++;
            }
        }
        return true;
    }
    
    void CheckIndirectAddressSettings(int idMin, int idMax) {
        uint16_t read_address_value_2bytes;
        uint16_t read_indirect_data_value_2bytes;
        uint32_t read_address_value_4bytes;
        uint32_t read_indirect_data_value_4bytes;
        uint8_t dxl_error = 0;
        int dxl_comm_result;
        int expected_address_value[dynamixelParameters.cols()];
        int indirect_data_address = INDIRECTADDRESS_FOR_WRITE +2;

        for (int p = 0; p < portHandlers.size(); p++) {
            std::cout << "Checking port: " << portHandlers[p]->getPortName() << std::endl;

            for (int id = idMin; id <= idMax; id++) {
                std::cout << "Checking servo ID: " << id << std::endl;

                for (int param = 0; param < dynamixelParameters.cols(); param++) {
                    int byteLength = dynamixelParameters(1, param);
                    std::string parameterName = dynamixelParameters.labels(0)[param];

                        int address = dynamixelParameters(0, param);

                        if (byteLength == 2){
                            // Read the current indirect address setting
                            dxl_comm_result = packetHandlers[p]->read2ByteTxRx(portHandlers[p], id, address, &read_address_value_2bytes, &dxl_error);
                            if (dxl_comm_result != COMM_SUCCESS) {
                                std::cout << "Failed to read indirect address at " << address
                                        << " for servo ID: " << id
                                        << " Error: " << packetHandlers[p]->getTxRxResult(dxl_comm_result)
                                        << " DXL Error: " << packetHandlers[p]->getRxPacketError(dxl_error) << std::endl;
                                return;
                            }
                            dxl_comm_result = packetHandlers[p]->read2ByteTxRx(portHandlers[p], id, indirect_data_address, &read_indirect_data_value_2bytes, &dxl_error);
                            if (dxl_comm_result != COMM_SUCCESS) {
                                std::cout << "Failed to read indirect data at " << indirect_data_address
                                        << " for servo ID: " << id
                                        << " Error: " << packetHandlers[p]->getTxRxResult(dxl_comm_result)
                                        << " DXL Error: " << packetHandlers[p]->getRxPacketError(dxl_error) << std::endl;
                                return;
                            }
                            if (read_address_value_2bytes != read_indirect_data_value_2bytes) {
                                std::cout << "Address at " << address << " = " << read_address_value_2bytes << std::endl;
                                std::cout << "Indirect data at " << indirect_data_address << " = " << read_indirect_data_value_2bytes << std::endl;
                                std::cout << "Indirect address does not match the expected value." << std::endl;
                            }
                            else {
                                std::cout << "Indirect data at " << indirect_data_address << " matches the expected value." << std::endl;
                            }
                            
                        }
                        else if (byteLength == 4){
                            // Read the current indirect address setting
                            dxl_comm_result = packetHandlers[p]->read4ByteTxRx(portHandlers[p], id, address, &read_address_value_4bytes, &dxl_error);
                            if (dxl_comm_result != COMM_SUCCESS) {
                                std::cout << "Failed to read address at " << address
                                        << " for servo ID: " << id
                                        << " Error: " << packetHandlers[p]->getTxRxResult(dxl_comm_result)
                                        << " DXL Error: " << packetHandlers[p]->getRxPacketError(dxl_error) << std::endl;
                                return;
                            }
                            dxl_comm_result = packetHandlers[p]->read4ByteTxRx(portHandlers[p], id, indirect_data_address, &read_indirect_data_value_4bytes, &dxl_error);
                            if (dxl_comm_result != COMM_SUCCESS) {
                                std::cout << "Failed to read indirect address at " << indirect_data_address
                                        << " for servo ID: " << id
                                        << " Error: " << packetHandlers[p]->getTxRxResult(dxl_comm_result)
                                        << " DXL Error: " << packetHandlers[p]->getRxPacketError(dxl_error) << std::endl;
                                return;
                            }

                         
                            if (read_address_value_4bytes != read_indirect_data_value_4bytes) {
                                std::cout << "Address at " << address << " = " << read_address_value_4bytes << std::endl;
                                std::cout << "Indirect data at " << indirect_data_address << " = " << read_indirect_data_value_4bytes << std::endl;
                                std::cout << "Indirect data does not match the expected value." << std::endl;
                            }
                            else {
                                std::cout << "Indirect Address at " << indirect_data_address << "matches the expected value." << std::endl;
                            }
                        }
                        indirect_data_address += byteLength;
                        
                       
                    
                }
            }
        }

        std::cout << "Completed checking indirect addresses." << std::endl;
    }

    bool PowerOn(int IDMin, int IDMax, dynamixel::PortHandler *portHandler, dynamixel::PacketHandler *packetHandler)
    {   

        
        if (portHandler == NULL){// If no port handler return true. Only return false if communication went wrong.
            std::cout << "Port handler in PowerOn() is null" << std::endl;
            return true;

        } 

        

        Timer t;
        const int nrOfServos = IDMax - IDMin + 1;
        int dxl_comm_result = COMM_TX_FAIL; // Communication result
        uint8_t dxl_error = 0;              // Dynamixel error
        uint16_t start_p_value[7] = {0, 0, 0, 0, 0, 0, 0};
        uint32_t present_postition_value[7] = {0, 0, 0, 0, 0, 0, 0};
       


        // Get P values
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->read2ByteTxRx(portHandler, IDMin + i, 84, &start_p_value[i], &dxl_error))
                return false;
        // Set P value to 0
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->write2ByteTxRx(portHandler, IDMin + i, 84, 0, &dxl_error))
                return false;
        // Set torque value to 1
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->write1ByteTxRx(portHandler, IDMin + i, 64, 1, &dxl_error))
                return false;
        while (t.GetTime() < TIMER_POWER_ON)
        {
            // Get present position
            for (int i = 0; i < nrOfServos; i++)
                if (COMM_SUCCESS != packetHandler->read4ByteTxRx(portHandler, IDMin + i, 132, &present_postition_value[i], &dxl_error))
                    return false;
            // Set goal position to present postiion
            for (int i = 0; i < nrOfServos; i++)
                if (COMM_SUCCESS != packetHandler->write4ByteTxRx(portHandler, IDMin + i, 116, present_postition_value[i], &dxl_error))
                    return false;
            // Ramping up P
            for (int i = 0; i < nrOfServos; i++)
                if (COMM_SUCCESS != packetHandler->write2ByteTxRx(portHandler, IDMin + i, 84, int(float(start_p_value[i]) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
                    return false;
        }

        // Set P value to start value
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->write2ByteTxRx(portHandler, IDMin + i, 84, start_p_value[i], &dxl_error))
                return false;

        return true;
        
    }

    bool PowerOnPupil()
    {
        uint8_t dxl_error = 0; // Dynamixel error

        // Enable torque. No fancy ramping
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 24, 1, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 24, 1, &dxl_error))
            return false;
        return true;
    }

    bool PowerOnRobot()
    {
        // Trying to torque up the power of the servos.
        // Dynamixel protocel 2.0
        // In current base position control mode goal current can be used.
        // In poistion control mode P can be used (PID).
        // Torqing up the servos? This can not be done in 2.0 and position mode only in position-current mode.
        // 1. Set P (PID) = 0. Store start P value
        // 2. Set goal poistion to present position
        // 3. Increase current or P (PID)
        // 4. Repeat 2,3 for X seconds.



        auto headThread = std::async(std::launch::async, &ServoControlTuning::PowerOn, this, HEAD_ID_MIN, HEAD_ID_MAX, std::ref(portHandlerHead), std::ref(packetHandlerHead));
        Notify(msg_trace, "Power on head");
        auto pupilThread = std::async(std::launch::async, &ServoControlTuning::PowerOnPupil, this); // Different control table.
        Notify(msg_trace, "Power on pupil");
         if (!headThread.get())
            Notify(msg_fatal_error, "Can not power on head");
        if (!pupilThread.get())
            Notify(msg_fatal_error, "Can not power on pupil");
        if(EpiMode)
        {
        auto leftArmThread = std::async(std::launch::async, &ServoControlTuning::PowerOn, this, ARM_ID_MIN, ARM_ID_MAX, std::ref(portHandlerLeftArm), std::ref(packetHandlerLeftArm));
        std::cout << "LeftArmThread done" << std::endl;
        auto rightArmThread = std::async(std::launch::async, &ServoControlTuning::PowerOn, this, ARM_ID_MIN, ARM_ID_MAX, std::ref(portHandlerRightArm), std::ref(packetHandlerRightArm));
        std::cout << "RightArmThread done" << std::endl;
        auto bodyThread = std::async(std::launch::async, &ServoControlTuning::PowerOn, this, BODY_ID_MIN, BODY_ID_MAX, std::ref(portHandlerBody), std::ref(packetHandlerBody));
        std::cout << "BodyThread done" << std::endl;
        if (!leftArmThread.get())
            Notify(msg_fatal_error, "Can not power on left arm");
        if (!rightArmThread.get())
            Notify(msg_fatal_error, "Can not power on right arm");
        if (!bodyThread.get())
            Notify(msg_fatal_error, "Can not power on body");
        }
       
        

        return true;
    }

    
    void Tick()
    {   
        std::cout << "Run sequence: " << runSequence << std::endl;
        
        int servoToTune = servoIDs[servoToTuneName];   
        std::cout << "Servo to tune: " << servoToTune << std::endl;
        std::cout << "Servo to tune name: " << servoToTuneName << std::endl;
       

        if (previousGoalPosition != servoParameters[tuningList.get_index("Goal Position")])
        {
            Notify(msg_debug, "Goal position changed\n");
            runSequence = false;
        }

        if (runSequence && transitionIndex < numberTransitions && tick > 0)
        {   
            Notify(msg_debug, "Starting moving sequence\n");
            std::cout << "Transition index: " << transitionIndex << std::endl;
            goalPosition = servoTransitions[transitionIndex];
            if (ReachedGoal(position[0], goalPosition, 5 )&& tick> 0){
                Notify(msg_debug, "Reached goal position\n");
                Sleep(1);
                transitionIndex++;

        }
        }
        else
        {
            goalPosition = servoParameters[tuningList.get_index("Goal Position")];
            
     
        }
        
        if (transitionIndex == numberTransitions)
        {
            transitionIndex = 0;
            Notify(msg_debug, "Restarting the transition sequence\n");
            
        }
        
        
        goalCurrent = servoParameters[3];
        current[1] = goalCurrent;
        position[1] = goalPosition;

        
        
        if (servoToTuneName.compare_string("RightPupil") || servoToTuneName.compare_string("LeftPupil")) 
        {   
            Notify(msg_debug, "Compare parameter with strings method works");
            
            position[1] = clip(position[1], 5, 16); // Pupil size must be between 5 mm to 16 mm.

            // Special case. As pupil does not have any feedback we just return goal position
            
            position[0].copy(position[1]);
            
        
            // Special case for pupil uses mm instead of degrees
            position[1] = PupilMMToDynamixel(position[1], AngleMinLimitPupil[0], AngleMaxLimitPupil[0]);
            
        
            auto pupilThread = std::async(std::launch::async, &ServoControlTuning::CommunicationPupil, this, servoTransitions[transitionIndex] ); // Special!
            if (!pupilThread.get())
        {
            Notify(msg_warning, "Can not communicate with pupil");
            portHandlerPupil->clearPort();
        }
        }
        
        if(servoToTuneName.compare_string("NeckTilt") || servoToTuneName.compare_string("NeckPan") || servoToTuneName.compare_string("LeftEye") || servoToTuneName.compare_string("RightEye"))
        {   
            
            Notify(msg_debug, "Compare parameter with strings method works");
            auto headThread = std::async(std::launch::async, &ServoControlTuning::Communication, this, servoToTune, HEAD_INDEX_IO,servoParameters, tuningList, std::ref(portHandlerHead), std::ref(packetHandlerHead), std::ref(groupSyncReadHead), std::ref(groupSyncWriteHead));
            if (save){
                std::string servoChain ="Head";
                SaveParameterValuesAsJson(servoToTune, servoParameters, robot[robotName].type,servoChain, tuningList);
                save = false;
            }
        if (!headThread.get())
            {
                Notify(msg_warning, "Can not communicate with head");
                portHandlerLeftArm->clearPort();
            }
            
        }
        
        
        if(EpiMode)
        {
            auto leftArmThread = std::async(std::launch::async, &ServoControlTuning::Communication, this, servoToTune , LEFT_ARM_INDEX_IO, servoParameters, tuningList, std::ref(portHandlerLeftArm), std::ref(packetHandlerLeftArm), std::ref(groupSyncReadLeftArm), std::ref(groupSyncWriteLeftArm));
            auto rightArmThread = std::async(std::launch::async, &ServoControlTuning::Communication, this, servoToTune , RIGHT_ARM_INDEX_IO,servoParameters, tuningList, std::ref(portHandlerRightArm), std::ref(packetHandlerRightArm), std::ref(groupSyncReadRightArm), std::ref(groupSyncWriteRightArm));
            auto bodyThread = std::async(std::launch::async, &ServoControlTuning::Communication, this, servoToTune , BODY_INDEX_IO,servoParameters, tuningList,std::ref(portHandlerBody), std::ref(packetHandlerBody), std::ref(groupSyncReadBody), std::ref(groupSyncWriteBody));
        
            if (!leftArmThread.get())
            {
                Notify(msg_warning, "Can not communicate with left arm");
                portHandlerLeftArm->clearPort();
            }
            if (!rightArmThread.get())
            {
                Notify(msg_warning, "Can not communicate with right arm");
                portHandlerRightArm->clearPort();
            }
            if (!bodyThread.get())
            {
                Notify(msg_warning, "Can not communicate with body");
                portHandlerBody->clearPort();
            }
        }
        tick++;
        previousGoalPosition = servoParameters[tuningList.get_index("Goal Position")];
    }

    // A function that set importat parameters in the control table.
    // Baud rate and ID needs to be set manually.

    bool PowerOff(int IDMin, int IDMax, dynamixel::PortHandler *portHandler, dynamixel::PacketHandler *packetHandler)
    {
        if (portHandler == NULL) // If no port handler return true. Only return false if communication went wrong.
            return true;

        Timer t;
        const int nrOfServos = IDMax - IDMin + 1;
        int dxl_comm_result = COMM_TX_FAIL; // Communication result
        uint8_t dxl_error = 0;              // Dynamixel error
        uint16_t start_p_value[7] = {0, 0, 0, 0, 0, 0, 0};
        uint32_t present_postition_value[7] = {0, 0, 0, 0, 0, 0, 0};

        // Get P values
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->read2ByteTxRx(portHandler, IDMin + i, 84, &start_p_value[i], &dxl_error))
                return false;

        // t.Reset();
        Notify(msg_warning, "Power off servos. If needed, support the robot while power off the servos");

        // Ramp down p value
        // while (t.GetTime() < TIMER_POWER_OFF)
        //     for (int i = 0; i < nrOfServos; i++)
        //         if (COMM_SUCCESS != packetHandler->write2ByteTxRx(portHandler, IDMin + i, 84, int(float(start_p_value[i]) * (float(TIMER_POWER_OFF) - t.GetTime()) / float(TIMER_POWER_OFF)), &dxl_error))
        //             return false;
        // Turn p to zero
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->write2ByteTxRx(portHandler, IDMin + i, 84, 0, &dxl_error))
                return false;

        Sleep(TIMER_POWER_OFF);

        // Get present position
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->read4ByteTxRx(portHandler, IDMin + i, 132, &present_postition_value[i], &dxl_error))
                return false;
        // Set goal position to present postiion
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->write4ByteTxRx(portHandler, IDMin + i, 116, present_postition_value[i], &dxl_error))
                return false;

        // t.Restart();
        Sleep(TIMER_POWER_OFF_EXTENDED);

        // Enable torque off
        Notify(msg_debug, "Enable torque off");
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->write1ByteTxRx(portHandler, IDMin + i, 64, 0, &dxl_error))
                return false;
        // Set P value to start value
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->write2ByteTxRx(portHandler, IDMin + i, 84, start_p_value[i], &dxl_error))
                return false;

        return true;
    }

    bool PowerOffPupil()
    {
        uint8_t dxl_error = 0; // Dynamixel error

        // Torque off. No fancy rampiong
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 24, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 24, 0, &dxl_error))
            return false;

        return true;
    }

    bool PowerOffRobot()
    {

        auto headThread = std::async(std::launch::async, &ServoControlTuning::PowerOff, this, HEAD_ID_MIN, HEAD_ID_MAX, std::ref(portHandlerHead), std::ref(packetHandlerHead));
        auto pupilThread = std::async(std::launch::async, &ServoControlTuning::PowerOffPupil, this);
        auto leftArmThread = std::async(std::launch::async, &ServoControlTuning::PowerOff, this, ARM_ID_MIN, ARM_ID_MAX, std::ref(portHandlerLeftArm), std::ref(packetHandlerLeftArm));
        auto rightArmThread = std::async(std::launch::async, &ServoControlTuning::PowerOff, this, ARM_ID_MIN, ARM_ID_MAX, std::ref(portHandlerRightArm), std::ref(packetHandlerRightArm));
        auto bodyThread = std::async(std::launch::async, &ServoControlTuning::PowerOff, this, BODY_ID_MIN, BODY_ID_MAX, std::ref(portHandlerBody), std::ref(packetHandlerBody));

        if (!headThread.get())
            Notify(msg_fatal_error, "Can not power off head");
        if (!pupilThread.get())
            Notify(msg_fatal_error, "Can not power off pupil");
        if (!leftArmThread.get())
            Notify(msg_fatal_error, "Can not power off left arm");
        if (!rightArmThread.get())
            Notify(msg_fatal_error, "Can not power off right arm");
        if (!bodyThread.get())
            Notify(msg_fatal_error, "Can not power off body");

        // Power down servos.
        // 1. Store P (PID) value
        // 2. Ramp down P
        // 3. Turn of torque enable
        // 4. Set P (PID) valued from 1.

        return (true);
    }
    bool AutoCalibratePupil()
    {
        int dxl_comm_result = COMM_TX_FAIL; // Communication result
        uint8_t dxl_error = 0;              // Dynamixel error
        Timer t;
        double xlTimer = 0.010; // Timer in sec. XL320 need this. Not sure why.

        // Torque off. No fancy rampiong
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 24, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 24, 0, &dxl_error))
            return false;
        Sleep(xlTimer);

        // Reset min and max limit
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 6, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 6, 0, &dxl_error))
            return false;
        Sleep(xlTimer);

        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 8, 1023, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 8, 1023, &dxl_error))
            return false;
        Sleep(xlTimer);

        // Turn down torue limit
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 35, 500, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 35, 500, &dxl_error))
            return false;
        Sleep(xlTimer);

        // Torque off. No fancy rampiong
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 24, 1, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 24, 1, &dxl_error))
            return false;
        Sleep(xlTimer);

        // Go to min pos
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 30, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 30, 0, &dxl_error))
            return false;
        // Sleep for 300 ms
        Sleep(xlTimer);

        // Read pressent position
        uint16_t present_postition_value[2] = {0, 0};
        if (COMM_SUCCESS != packetHandlerPupil->read2ByteTxRx(portHandlerPupil, 2, 37, &present_postition_value[0], &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->read2ByteTxRx(portHandlerPupil, 3, 37, &present_postition_value[1], &dxl_error))
            return false;

        AngleMinLimitPupil[0] = present_postition_value[0] + 10;
        AngleMinLimitPupil[1] = present_postition_value[1] + 10;

        AngleMaxLimitPupil[0] = AngleMinLimitPupil[0] + 280;
        AngleMaxLimitPupil[1] = AngleMinLimitPupil[1] + 280;

       // Not implemented.
       //Notify(msg_debug, "Position limits pupil servos (auto calibrate): min %i %i max %i %i \n", AngleMinLimitPupil[0], AngleMinLimitPupil[1], AngleMaxLimitPupil[0], AngleMaxLimitPupil[1]);

        // Torque off. No fancy rampiong
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 24, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 24, 0, &dxl_error))
            return false;
        Sleep(xlTimer);

        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 35, 1023, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 35, 1023, &dxl_error))
            return false;
        Sleep(xlTimer);


        return true;
    }
    ~ServoControlTuning()
    {
       
        // Torque down
        PowerOffRobot();

        // Close ports
        if (EpiTorsoMode || EpiMode)
        {
            portHandlerHead->closePort();
            portHandlerPupil->closePort();
        }
        if (EpiMode)
        {
            portHandlerLeftArm->closePort();
            portHandlerRightArm->closePort();
            portHandlerBody->closePort();
        }

        // Free memory
        delete groupSyncWriteHead;
        delete groupSyncReadHead;
        delete groupSyncWriteLeftArm;
        delete groupSyncReadLeftArm;
        delete groupSyncWriteRightArm;
        delete groupSyncReadRightArm;
        delete groupSyncWriteBody;
        delete groupSyncReadBody;
        delete groupSyncWritePupil;
    }

};

INSTALL_CLASS(ServoControlTuning)


  