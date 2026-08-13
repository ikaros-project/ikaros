#include "dynamixel_sdk.h" 
#include <string>

#include "ikaros.h"

// Dynamixel settings

// install_name_tool -add_rpath /usr/local/lib your_executable


constexpr int baudRate1M = 1000000; // XL-320 is limited to 1 Mbit/s
constexpr int mxGoalPositionAddress = 30;
constexpr int mxPresentPositionAddress = 36;
constexpr double protocolVersion = 1.0;



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
        int baudRate = baudRate1M; // Set your desired baud rate here
        
        // Initialize PortHandler and PacketHandler
        portHandler = dynamixel::PortHandler::getPortHandler(serialPort.c_str());
        packetHandler = dynamixel::PacketHandler::getPacketHandler(protocolVersion);

        // Open port
        if (!portHandler->openPort())
        {
            throw exception("HeadServos could not open serial port \"" + serialPort + "\".", path_);
        }

        // Set baud rate
        if (!portHandler->setBaudRate(baudRate)) 
        {
            throw exception("HeadServos could not set baud rate " + std::to_string(baudRate) +
                            " on \"" + serialPort + "\".", path_);
        }

        Print("HeadServos connected to \"" + serialPort + "\" at " + std::to_string(baudRate) + " baud.");

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
        dxl_comm_result = packetHandler->write2ByteTxRx(portHandler, 0, mxGoalPositionAddress, positionServo0, &dxl_error);

        if (dxl_comm_result != COMM_SUCCESS) {
            Warning("HeadServos failed to send position to servo 0: " +
                    std::string(packetHandler->getTxRxResult(dxl_comm_result)));
        } else if (dxl_error != 0) {
            Warning("HeadServos servo 0 error: " + std::string(packetHandler->getRxPacketError(dxl_error)));
        }

        // Send position command to servo ID 1
        dxl_comm_result = packetHandler->write2ByteTxRx(portHandler, 1, mxGoalPositionAddress, positionServo1, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS) {
            Warning("HeadServos failed to send position to servo 1: " +
                    std::string(packetHandler->getTxRxResult(dxl_comm_result)));
        } else if (dxl_error != 0) 
        {
            Warning("HeadServos servo 1 error: " + std::string(packetHandler->getRxPacketError(dxl_error)));
        }
    }

    void SetServoPosition(int servoID, uint16_t position)
    {
        uint8_t dxl_error = 0;
        int dxl_comm_result;

        // Send position command to the specified servo
        dxl_comm_result = packetHandler->write2ByteTxRx(portHandler, servoID, mxGoalPositionAddress, position, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS) {
            Warning("HeadServos failed to send position to servo " + std::to_string(servoID) + ": " +
                    packetHandler->getTxRxResult(dxl_comm_result));
        } else if (dxl_error != 0) {
            Warning("HeadServos servo " + std::to_string(servoID) + " error: " +
                    packetHandler->getRxPacketError(dxl_error));
        }
    }

    void ReadPositions() 
    {
        uint8_t dxl_error = 0;
        int dxl_comm_result;
        uint16_t positionServo0 = 0;
        uint16_t positionServo1 = 0;

        // Read position of servo ID 0
        dxl_comm_result = packetHandler->read2ByteTxRx(portHandler, 0, mxPresentPositionAddress, &positionServo0, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS) 
        {
            Warning("HeadServos failed to read position from servo 0: " +
                    std::string(packetHandler->getTxRxResult(dxl_comm_result)));
        } 
        else if (dxl_error != 0) 
        {
            Warning("HeadServos servo 0 error: " + std::string(packetHandler->getRxPacketError(dxl_error)));
        } 
        else 
        {
            //std::cout << "Servo ID 0 Position: " << positionServo0 << std::endl;
        }

        // Read position of servo ID 1
        dxl_comm_result = packetHandler->read2ByteTxRx(portHandler, 1, mxPresentPositionAddress, &positionServo1, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS) 
        {
            Warning("HeadServos failed to read position from servo 1: " +
                    std::string(packetHandler->getTxRxResult(dxl_comm_result)));
        } 
        else if (dxl_error != 0) 
        {
            Warning("HeadServos servo 1 error: " + std::string(packetHandler->getRxPacketError(dxl_error)));
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
