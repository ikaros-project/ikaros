#include "ikaros.h"
#include <iostream>
#include <cstring> // for memcpy
#include <memory>  // for smart pointers

using namespace ikaros;

class UM7 : public Module
{
    parameter port;

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
        uint8_t data[30] = {};  // Ensure `data` is zero-initialized
    } UM7_packet;

    UM7_packet packet;
    Serial *s;
    int rx_length;
    char rx_data[255] = {};  // Ensure `rx_data` is zero-initialized
    std::string rx_buffer;  // Buffer for handling partial packets
    const size_t MAX_BUFFER_SIZE = 1024;  // Define a max buffer size

    // Function to convert 4 bytes to float
    float bytesToFloat(const uint8_t* bytes) {
        float result;
        memcpy(&result, bytes, sizeof(float));
        return result;
    }

    void sendConfig(uint8_t address)
    {
        uint8_t tx_data[11];
        tx_data[0] = 's';
        tx_data[1] = 'n';
        tx_data[2] = 'p';
        tx_data[3] = 0x80; // Packet Type byte
        tx_data[4] = address; // Address in hex
        tx_data[5] = 0x00; // Data
        tx_data[6] = 0xFF; // Data // Maximum broadcast of 255 hz
        tx_data[7] = 0x00; // Data
        tx_data[8] = 0x00; // Data
        uint16_t computed_checksum = tx_data[0] + tx_data[1] + tx_data[2] + tx_data[3] + tx_data[4] + tx_data[5] + tx_data[6] + tx_data[7] + tx_data[8];
        tx_data[9] = (computed_checksum >> 8) & 0xFF;
        tx_data[10] = computed_checksum & 0xFF;
        s->SendBytes((char *)tx_data, 11);
    }

    void DisableBroadcast(){
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
        Bind(eulerAngles, "EulerAngles");
        Bind(gyroProc, "ProcessedGyro");
        Bind(accelProc, "ProcessedAccel");
        std::cout << "Bind done." << std::endl;
        baudrate = 115200;  // Default baudrate

        s = new Serial(std::string(port).c_str(), baudrate); // Use configurable baudrate
        if (!s) {
            std::cerr << "Failed to initialize serial port." << std::endl;
            return;
        }
        std::cout << "Serial port opened on " << port << " with baudrate " << baudrate << std::endl;

        DisableBroadcast();

        sendConfig(0x05); // Enable Euler angles at 255 Hz
        sendConfig(0x03); // Enable processed accelerometer and gyro data at 255 Hz
        
        std::cout << "UM7 configured for gyro, accelerometer, and Euler angle data broadcasting." << std::endl;
    }

    void Tick()
    {
        rx_length = s->ReceiveBytes(rx_data, 255, 0);
        if (rx_length <= 0) {
            std::cout << "No data received." << std::endl;
            return;
        }
        
        rx_buffer.append(rx_data, rx_length);  

        int parse_result = parsePacket();
        while (parse_result == 0 ) {  // Keep parsing while packets are available
            if (parse_result == 0) {
                // Process the valid packet
                std::cout << "Processing packet with address " << (int)packet.Address << std::endl;
                switch (packet.Address) {
                    case 0x61: 
                        ProcessGyroData();
                        break;
                    case 0x65: 
                        ProcessAccelData();
                        break;
                    case 0x70: case 0x71:
                        ProcessEulerAngles();
                        break;
                    default:
                        break;
                }
            }
            parse_result = parsePacket();  // Continue parsing if more packets are present
        }

        if (parse_result == 2) {
            std::cout << "No valid packets found in buffer (no snp sequence)." << std::endl;
        }
        if (parse_result == 3) {
            std::cout << "Not enough data for a full packet." << std::endl;
        }
        if (parse_result == 4) {
            std::cout << "Checksum mismatch, invalid packet discarded." << std::endl;
        }

        // Manage buffer size
        if (rx_buffer.size() >= MAX_BUFFER_SIZE) {
            rx_buffer.clear();
            std::cerr << "Buffer size exceeded. Clearing buffer." << std::endl;
        }
    }

    void ProcessGyroData()
    {
        int axis = packet.Address - 0x61;
        float gyroX = bytesToFloat(packet.data);
        if (std::isnan(gyroX) || std::isinf(gyroX)) {
            std::cerr << "Invalid gyro data received." << std::endl;
            return;
        }
        gyroProc[0] = gyroX / 0.0610352;  // Convert to degrees/s
        float gyroY = bytesToFloat(packet.data + 4);
        if (std::isnan(gyroY) || std::isinf(gyroY)) {
            std::cerr << "Invalid gyro data received." << std::endl;
            return;
        }
        gyroProc[1] = gyroY / 0.0610352;
        float gyroZ = bytesToFloat(packet.data + 8);
        if (std::isnan(gyroZ) || std::isinf(gyroZ)) {
            std::cerr << "Invalid gyro data received." << std::endl;
            return;
        }
        gyroProc[2] = gyroZ / 0.0610352;

    

        std::cout << "Gyro X: " << gyroX << ", Y: " << gyroY << ", Z: " << gyroZ << " degrees/s" << std::endl;
    }

    void ProcessAccelData()
    {
        int axis = packet.Address - 0x65;  // Corrected axis indexing
        float accelX = bytesToFloat(packet.data);
        if (std::isnan(accelX) || std::isinf(accelX)) {
            std::cerr << "Invalid accelerometer data received." << std::endl;
            return;
        }
        accelProc[0] = accelX / 0.000183105; // Convert to m/s^2
        float accelY = bytesToFloat(packet.data + 4);
        if (std::isnan(accelY) || std::isinf(accelY)) {
            std::cerr << "Invalid accelerometer data received." << std::endl;
            return;
        }
        accelProc[1] = accelY / 0.000183105;
        float accelZ = bytesToFloat(packet.data + 8);
        if (std::isnan(accelZ) || std::isinf(accelZ)) {
            std::cerr << "Invalid accelerometer data received." << std::endl;
            return;
        }
        accelProc[2] = accelZ / 0.000183105;
        
        std::cout << "Accel X: " << accelX << ", Y: " << accelY << ", Z: " << accelZ << " m/s^2" << std::endl;
    }

    void ProcessEulerAngles()
    {
        if (packet.Address == 0x70) {
            if (packet.data_length < 20) {
                std::cerr << "Error: Not enough data for Euler angles. Expected 20 bytes, got " 
                          << (int)packet.data_length << std::endl;
                return;
            }

            float estroll = bytesToFloat(packet.data);
            float estpitch = bytesToFloat(packet.data + 4);
            float estyaw = bytesToFloat(packet.data + 8);
            float estRoll = (packet.data[0] << 8) | packet.data[1];
            float estPitch = (packet.data[2] << 8) | packet.data[3];
            float estYaw = (packet.data[4] << 8) | packet.data[5];
            eulerAngles[0] = estroll/91.02222;
            eulerAngles[1] = estpitch/91.02222;
            eulerAngles[2] = estyaw/91.02222;

            eulerAngles.print();
        }
        else if (packet.Address == 0x71) {
            if (packet.data_length < 4) {
                std::cerr << "Error: Not enough data for Yaw angle. Expected 4 bytes, got " 
                          << (int)packet.data_length << std::endl;
                return;
            }

            float estyaw = bytesToFloat(packet.data);
            eulerAngles[2] = estyaw;

            std::cout << "Yaw: " << estyaw << " degrees" << std::endl;
        }
    }

    int parsePacket()
    {
        size_t index;
        // Minimum length required to contain a full packet header
        if (rx_buffer.size() < 7) {
            return 1;  // Not enough data for even the smallest packet
        }

        // Find the "snp" start sequence
        for (index = 0; index < rx_buffer.size() - 2; ++index) {
            if (rx_buffer[index] == 's' && rx_buffer[index + 1] == 'n' && rx_buffer[index + 2] == 'p') {
                break;  // Found the packet start
            }
        }

        size_t packet_index = index;

        // Check if we found a header or reached the end
        if (packet_index == rx_buffer.size() - 2) {
            return 2;  // No header found
        }

        // Verify that there's enough data for at least the minimum packet length (7 bytes)
        if ((rx_buffer.size() - packet_index) < 7) {
            return 3;  // Not enough data for a complete packet
        }

        // Extract packet type (PT) and determine packet length
        uint8_t PT = rx_buffer[packet_index + 3];
        uint8_t packet_has_data = (PT >> 7) & 0x01;
        uint8_t packet_is_batch = (PT >> 6) & 0x01;
        uint8_t batch_length = (PT >> 2) & 0x0F;


            

        uint8_t data_length = 0;
        if (packet_has_data) {
            if (packet_is_batch) {
                data_length = 4 * batch_length;
            } else {
                data_length = 4;
            }
        } else {
            data_length = 0;
        }

        // Verify that there's enough data for the full packet, including the checksum
        if ((rx_buffer.size() - packet_index) < (data_length + 5)) {
            return 3;  // Not enough data yet for the complete packet
        }

        // Extract the packet details
        packet.Address = rx_buffer[packet_index + 4];
        packet.PT = PT;
        packet.data_length = data_length;

        // Compute checksum while copying data
        uint16_t computed_checksum = 's' + 'n' + 'p' + PT + packet.Address;

        std::cout << "Checksum calculation bytes: ";
        std::cout << "'s' " << (int)'s' << ", 'n' " << (int)'n' << ", 'p' " << (int)'p' << ", PT " << (int)PT
                  << ", Address " << (int)packet.Address << " ";

        for (size_t i = 0; i < data_length; ++i) {
            packet.data[i] = rx_buffer[packet_index + 5 + i];
            computed_checksum += packet.data[i];
            std::cout << (int)packet.data[i] << " ";
        }
        std::cout << "\nComputed checksum: " << computed_checksum << std::endl;

        computed_checksum &= 0xFFFF;

        uint16_t received_checksum = ((uint8_t)rx_buffer[packet_index + 5 + data_length] << 8) |
                                      (uint8_t)rx_buffer[packet_index + 6 + data_length];

        std::cout << "Received checksum: " << received_checksum << std::endl;

        if (received_checksum != computed_checksum) {
            std::cerr << "Checksum mismatch, discarding packet." << std::endl;
            rx_buffer.erase(0, packet_index + 7);  // Remove bad packet
            return 4;  // Bad checksum
        }

        // Successfully parsed packet, remove it from the buffer
        size_t erase_length = packet_index + 5 + data_length + 2;
        if (erase_length > rx_buffer.size()) {
            std::cerr << "Error: Attempt to erase beyond buffer size. Buffer size: " << rx_buffer.size()
                      << ", Requested erase length: " << erase_length << std::endl;
            return 3; // Not enough data yet
        }

        std::cout << "Packet parsed successfully." << std::endl;
        rx_buffer.erase(0, erase_length);
        std::cout << "Erasing " << erase_length << " bytes." << std::endl;

        return 0;  // Successfully parsed
    }
};

INSTALL_CLASS(UM7)
