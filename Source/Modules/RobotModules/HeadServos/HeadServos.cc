#include "dynamixel_sdk.h" 
#include <iostream>
#include <string>

#include "ikaros.h"

// Dynamixel settings

// install_name_tool -add_rpath /usr/local/lib your_executable


#define BAUDRATE1M 1000000   // XL-320 is limited to 1Mbit
#define BAUDRATE3M 3000000   // MX servos

#define ADDR_MX_TORQUE_ENABLE           24
#define ADDR_MX_GOAL_POSITION           30
#define ADDR_MX_PRESENT_POSITION        36

#define PROTOCOL_VERSION                1.0



using namespace ikaros;

class HeadServos : public Module
 {
private:
    matrix goal_position;
    matrix present_position;
    matrix torque_enable;

    dynamixel::PortHandler *portHandler;
    dynamixel::PacketHandler *packetHandler;

    bool connected = false;
public:

    void Init() 
    {
        Bind(goal_position, "GOAL_POSITION");
        Bind(present_position, "PRESENT_POSITION");
        Bind(torque_enable, "TORQUE_ENABLE");

        const std::string serialPort = "/dev/cu.usbserial-A40129WB";
        int baudRate = BAUDRATE1M; // Set your desired baud rate here
        
        // Initialize PortHandler and PacketHandler
        portHandler = dynamixel::PortHandler::getPortHandler(serialPort.c_str());
        packetHandler = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION); // Protocol version 1.0

        // Open port
        if (!portHandler->openPort())
        {
            std::cerr << "Failed to open port: " << serialPort << std::endl;
            connected = false;
            return;
        }

        // Set baud rate
        if (!portHandler->setBaudRate(baudRate)) 
        {
            std::cerr << "Failed to set baud rate: " << baudRate << std::endl;
            return;
        }

        std::cout << "Successfully connected to port: " << serialPort << " with baud rate: " << baudRate << std::endl;

        connected = true;
    }

    ~HeadServos() 
    {
        if (portHandler) 
        {
            portHandler->closePort();
            delete portHandler;
        }
    }

    void SendPositionCommands(uint16_t positionServo0, uint16_t positionServo1) 
    {
        uint8_t dxl_error = 0;
        int dxl_comm_result;

        // Send position command to servo ID 0
        dxl_comm_result = packetHandler->write2ByteTxRx(portHandler, 0, ADDR_MX_GOAL_POSITION, positionServo0, &dxl_error);

        if (dxl_comm_result != COMM_SUCCESS) {
            std::cerr << "Failed to send position to servo ID 0: " << packetHandler->getTxRxResult(dxl_comm_result) << std::endl;
        } else if (dxl_error != 0) {
            std::cerr << "Servo ID 0 error: " << packetHandler->getRxPacketError(dxl_error) << std::endl;
        }

        // Send position command to servo ID 1
        dxl_comm_result = packetHandler->write2ByteTxRx(portHandler, 1, ADDR_MX_GOAL_POSITION, positionServo1, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS) {
            std::cerr << "Failed to send position to servo ID 1: " << packetHandler->getTxRxResult(dxl_comm_result) << std::endl;
        } else if (dxl_error != 0) 
        {
            std::cerr << "Servo ID 1 error: " << packetHandler->getRxPacketError(dxl_error) << std::endl;
        }
    }

    void SetServoPosition(int servoID, uint16_t position)
    {
        uint8_t dxl_error = 0;
        int dxl_comm_result;

        // Send position command to the specified servo
        dxl_comm_result = packetHandler->write2ByteTxRx(portHandler, servoID, ADDR_MX_GOAL_POSITION, position, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS) {
            std::cerr << "Failed to send position to servo ID " << servoID << ": " << packetHandler->getTxRxResult(dxl_comm_result) << std::endl;
        } else if (dxl_error != 0) {
            std::cerr << "Servo ID " << servoID << " error: " << packetHandler->getRxPacketError(dxl_error) << std::endl;
        }
    }

    void ReadPositions() 
    {
        uint8_t dxl_error = 0;
        int dxl_comm_result;
        uint16_t positionServo0 = 0;
        uint16_t positionServo1 = 0;

        // Read position of servo ID 0
        dxl_comm_result = packetHandler->read2ByteTxRx(portHandler, 0, ADDR_MX_PRESENT_POSITION, &positionServo0, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS) 
        {
            std::cerr << "Failed to read position of servo ID 0: " << packetHandler->getTxRxResult(dxl_comm_result) << std::endl;
        } 
        else if (dxl_error != 0) 
        {
            std::cerr << "Servo ID 0 error: " << packetHandler->getRxPacketError(dxl_error) << std::endl;
        } 
        else 
        {
            //std::cout << "Servo ID 0 Position: " << positionServo0 << std::endl;
        }

        // Read position of servo ID 1
        dxl_comm_result = packetHandler->read2ByteTxRx(portHandler, 1, ADDR_MX_PRESENT_POSITION, &positionServo1, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS) 
        {
            // std::cerr << "Failed to read position of servo ID 1: " << packetHandler->getTxRxResult(dxl_comm_result) << std::endl;
        } 
        else if (dxl_error != 0) 
        {
            std::cerr << "Servo ID 1 error: " << packetHandler->getRxPacketError(dxl_error) << std::endl;
        } 
        else 
        {
            //std::cout << "Servo ID 1 Position: " << positionServo1 << std::endl;
        }

        present_position[0] = float(512-positionServo0)/1024.0;
        present_position[1] = float(512-positionServo1)/1024.0;

       //  std::cout << present_position(0) << " " << present_position(1) << std::endl;
    }



    void Tick()
    {
        if(!connected)
            return;

        //SendPositionCommands(512, 512); // Set both servos to position 512
        ReadPositions(); // Read and print the positions of both servos
    }
};

INSTALL_CLASS(HeadServos)