#include "ikaros.h"
#include <iostream>
#include <cstring> // for memcpy
#include <memory>  // for smart pointers

using namespace ikaros;

class UM7 : public Module
{
    parameter port;
    matrix roll;
    matrix pitch;
    matrix yaw;
    matrix gyroProc;
    matrix accelProc;
    matrix eulerAngles;

    int baudrate;

    // Structure for holding received packet information
    typedef struct UM7_packet_struct
    {
        uint8_t Address;
        uint8_t PT;
        uint16_t Checksum;
        uint8_t data_length;
        uint8_t data[30] = {}; // Ensure `data` is zero-initialized
    } UM7_packet;

    UM7_packet packet;
    Serial *s;
    int rx_length;
    char rx_data[255] = {};              // Ensure `rx_data` is zero-initialized
    std::string rx_buffer;               // Buffer for handling partial packets
    const size_t MAX_BUFFER_SIZE = 1024; // Define a max buffer size

    // Function to convert 4 bytes to float (assuming little-endian format)
    float bytesToFloat(const uint8_t *bytes)
    {
        float result;
        uint32_t asInt = static_cast<uint32_t>(bytes[0]) |
                         (static_cast<uint32_t>(bytes[1]) << 8) |
                         (static_cast<uint32_t>(bytes[2]) << 16) |
                         (static_cast<uint32_t>(bytes[3]) << 24);
        memcpy(&result, &asInt, sizeof(result));
        return result;
    }

    // Function to convert 4 bytes to float (assuming big-endian format)
    float bytesToFloatBigEndian(const uint8_t *bytes)
    {
        uint32_t asInt = (uint32_t(bytes[3])) |
                         (uint32_t(bytes[2]) << 8) |
                         (uint32_t(bytes[1]) << 16) |
                         (uint32_t(bytes[0]) << 24);
        float result;
        memcpy(&result, &asInt, sizeof(float));
        return result;
    }

    void sendConfig(uint8_t address)
    {
        int acceldata = 0x00;
        if (address == 0x03)
        {
            uint8_t tx_data[11];
            tx_data[0] = 's';
            tx_data[1] = 'n';
            tx_data[2] = 'p';
            tx_data[3] = 0x80;      // Packet Type byte
            tx_data[4] = address;   // Address in hex
            tx_data[5] = 100; // Data
            tx_data[6] = 100;      // Data // Maximum broadcast of 255 hz
            tx_data[7] = 0x00;      // Data
            tx_data[8] = 0x00;      // Data
            uint16_t computed_checksum = tx_data[0] + tx_data[1] + tx_data[2] + tx_data[3] + tx_data[4] + tx_data[5] + tx_data[6] + tx_data[7] + tx_data[8];
            tx_data[9] = (computed_checksum >> 8) & 0xFF;
            tx_data[10] = computed_checksum & 0xFF;
            s->SendBytes((char *)tx_data, 11);
        }
        else{
            uint8_t tx_data[11];
            tx_data[0] = 's';
            tx_data[1] = 'n';
            tx_data[2] = 'p';
            tx_data[3] = 0x80;    // Packet Type byte
            tx_data[4] = address; // Address in hex
            tx_data[5] = 0x00;    // Data
            tx_data[6] = 100;    // Data // Maximum broadcast of 255 hz
            tx_data[7] = 0x00;    // Data
            tx_data[8] = 0x00;    // Data
            uint16_t computed_checksum = tx_data[0] + tx_data[1] + tx_data[2] + tx_data[3] + tx_data[4] + tx_data[5] + tx_data[6] + tx_data[7] + tx_data[8];
            tx_data[9] = (computed_checksum >> 8) & 0xFF;
            tx_data[10] = computed_checksum & 0xFF;
            s->SendBytes((char *)tx_data, 11);
        }
    }

    void DisableBroadcast()
    {
        for (int8_t i = 1; i <= 7; i++)
        {
            uint8_t tx_data[20];
            tx_data[0] = 's';
            tx_data[1] = 'n';
            tx_data[2] = 'p';
            tx_data[3] = 0x80; // Packet Type byte
            tx_data[4] = i;    // Address
            tx_data[5] = 0x00; // Data
            tx_data[6] = 0x00; // Data
            tx_data[7] = 0x00; // Data
            tx_data[8] = 0x00; // Data
            uint16_t computed_checksum = tx_data[0] + tx_data[1] + tx_data[2] + tx_data[3] + tx_data[4] + tx_data[5] + tx_data[6] + tx_data[7] + tx_data[8];
            tx_data[9] = (computed_checksum >> 8) & 0xFF;
            tx_data[10] = computed_checksum & 0xFF;
            s->SendBytes((char *)tx_data, 11);
        }
    }

    void Init()
    {
        Bind(port, "port");
        Bind(roll, "ROLL");
        Bind(pitch, "PITCH");
        Bind(yaw, "YAW");
        Bind(gyroProc, "ProcessedGyro");
        Bind(accelProc, "ProcessedAccel");
        Bind(eulerAngles, "EulerAngles");
    
        baudrate = 115200; // Default baudrate

        s = new Serial(std::string(port).c_str(), baudrate); // Use configurable baudrate
        if (!s)
        {
            std::cerr << "Failed to initialize serial port." << std::endl;
            return;
        }
        Notify(msg_print, std::string("Serial port opened on ") + std::string(port).c_str() + " with baudrate " + std::to_string(baudrate) + "\n");

        DisableBroadcast();
        sendConfig(0x05); // Enable Euler angles at 255 Hz
        sendConfig(0x03); // Enable processed accelerometer and gyro data at 255 Hz
        Notify(msg_debug, "UM7 configured for gyro, accelerometer, and Euler angle data broadcasting.");
    }

    void Tick()
    {
        rx_length = s->ReceiveBytes(rx_data, 255, 1);
        if (rx_length <= 0)
        {
            Notify(msg_warning, "No data received from UM7.");
            return;
        }

        rx_buffer.append(rx_data, rx_length);

        int parse_result = parsePacket();
        while (parse_result == 0)
        { // Keep parsing while packets are available
            if (parse_result == 0)
            {
                // Process the valid packet
                switch (packet.Address)
                {
                case 0x61:
                    ProcessGyroData();
                    break;
                case 0x65:
                    ProcessAccelData();
                    break;
                case 0x70:
                case 0x71:
                    ProcessEulerAngles();
                    break;
                }
            }
            parse_result = parsePacket(); // Continue parsing if more packets are present
        }

        if (parse_result == 2)
        {
            Notify(msg_debug, "No header found in buffer.");
        }
        if (parse_result == 3)
        {

            Notify(msg_debug, "Not enough data for a full packet.");
        }
        if (parse_result == 4)
        {

            Notify(msg_debug, "Checksum mismatch, invalid packet discarded.");
        }

        // Manage buffer size
        if (rx_buffer.size() > MAX_BUFFER_SIZE)
        {
            rx_buffer.clear();
            std::cerr << "Buffer size exceeded. Clearing buffer." << std::endl;
        }
    }

    void ProcessGyroData()
    {
        if (packet.data_length < 12)
        {
            std::cerr << "Error: Not enough data for gyro values. Expected 12 bytes, got " << (int)packet.data_length << std::endl;
            return;
        }

        // Extract 4 bytes for each gyro axis and convert to float (IEEE 754 format)
        float gyroX = bytesToFloatBigEndian(packet.data);
        float gyroY = bytesToFloatBigEndian(packet.data + 4);
        float gyroZ = bytesToFloatBigEndian(packet.data + 8);

        gyroProc[0] = gyroX;
        gyroProc[1] = gyroY;
        gyroProc[2] = gyroZ;

        //std::cout << "Gyro X: " << (float)gyroProc[0] << " deg/s, Y: " << (float)gyroProc[1] << " deg/s, Z: " << (float)gyroProc[2] << " deg/s" << std::endl;
    }

    void ProcessAccelData()
    {
        if (packet.data_length < 12)
        {
            std::cerr << "Error: Not enough data for accelerometer values. Expected 12 bytes, got " << (int)packet.data_length << std::endl;
            return;
        }

        // Extract 4 bytes for each accelerometer axis and convert to float (IEEE 754 format)
        float accelX = bytesToFloatBigEndian(packet.data);
        float accelY = bytesToFloatBigEndian(packet.data + 4);
        float accelZ = bytesToFloatBigEndian(packet.data + 8);

        accelProc[0] = accelX;
        accelProc[1] = accelY;
        accelProc[2] = accelZ;

        //std::cout << "Accel X: " << (float)accelProc[0] << " m/s², Y: " << (float)accelProc[1] << " m/s², Z: " << (float)accelProc[2] << " m/s²" << std::endl;
    }

    void ProcessEulerAngles()
    {
        if (packet.data_length < 20)
        {
            std::cerr << "Error: Not enough data for Euler angles. Expected 20 bytes, got "
                      << (int)packet.data_length << std::endl;
            return;
        }

        int16_t estRoll;
        int16_t estPitch;
        int16_t estYaw;

        estRoll = (packet.data[0] << 8) | packet.data[1];
        estPitch = (packet.data[2] << 8) | packet.data[3];
        estYaw = (packet.data[4] << 8) | packet.data[5];

        eulerAngles[0] = estRoll / 91.02222;
        eulerAngles[1] = estPitch / 91.02222;
        eulerAngles[2] = estYaw / 91.02222;

       
    }

    int parsePacket()
    {
        size_t index;
        // Minimum length required to contain a full packet header
        if (rx_buffer.size() < 7)
        {
            return 1; // Not enough data for even the smallest packet
        }

        // Find the "snp" start sequence
        for (index = 0; index < rx_buffer.size() - 2; ++index)
        {
            if (rx_buffer[index] == 's' && rx_buffer[index + 1] == 'n' && rx_buffer[index + 2] == 'p')
            {
                break; // Found the packet start
            }
        }

        size_t packet_index = index;

        // Check if we found a header or reached the end
        if (packet_index == rx_buffer.size() - 2)
        {
            return 2; // No header found
        }

        // Verify that there's enough data for at least the minimum packet length (7 bytes)
        if ((rx_buffer.size() - packet_index) < 7)
        {
            return 3; // Not enough data for a complete packet
        }

        // Extract packet type (PT) and determine packet length
        uint8_t PT = rx_buffer[packet_index + 3];
        uint8_t packet_has_data = (PT >> 7) & 0x01;
        uint8_t packet_is_batch = (PT >> 6) & 0x01;
        uint8_t batch_length = (PT >> 2) & 0x0F;

        uint8_t data_length = 0;
        if (packet_has_data)
        {
            if (packet_is_batch)
            {
                data_length = 4 * batch_length;
            }
            else
            {
                data_length = 4;
            }
        }
        else
        {
            data_length = 0;
        }

        // Verify that there's enough data for the full packet, including the checksum
        if ((rx_buffer.size() - packet_index) < (data_length + 5))
        {
            return 3; // Not enough data yet for the complete packet
        }

        // Extract the packet details
        packet.Address = rx_buffer[packet_index + 4];
        packet.PT = PT;
        packet.data_length = data_length;

        // Compute checksum while copying data
        uint16_t computed_checksum = 's' + 'n' + 'p' + PT + packet.Address;

        for (size_t i = 0; i < data_length; ++i)
        {
            packet.data[i] = rx_buffer[packet_index + 5 + i];
            computed_checksum += packet.data[i];
        }

        computed_checksum &= 0xFFFF;

        uint16_t received_checksum = ((uint8_t)rx_buffer[packet_index + 5 + data_length] << 8) |
                                     (uint8_t)rx_buffer[packet_index + 6 + data_length];

        if (received_checksum != computed_checksum)
        {
            Notify(msg_debug, "Checksum mismatch, discarding packet.");
            rx_buffer.erase(0, packet_index + 7); // Remove bad packet
            return 4;                             // Bad checksum
        }

        // Successfully parsed packet, remove it from the buffer
        size_t erase_length = packet_index + 5 + data_length + 2;
        if (erase_length > rx_buffer.size())
        {
            Notify(msg_warning, "Error: Attempt to erase beyond buffer size. Buffer size: " + std::to_string(rx_buffer.size())
                      + ", Requested erase length: " + std::to_string(erase_length));
            return 3; // Not enough data yet
        }

        rx_buffer.erase(0, erase_length);
        return 0; // Successfully parsed
    }
};

INSTALL_CLASS(UM7)
