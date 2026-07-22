//
//	UM7.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2026 Birger Johansson and Pierre Klintefors
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    See http://www.ikaros-project.org/ for more information.
//

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "ikaros.h"

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
    std::unique_ptr<Serial> serial;
    int rx_length = 0;
    std::array<char, 256> rx_data{};
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

    void
    SendPacket(const std::array<std::uint8_t, 11> & packet_data)
    {
        const int sent = serial->SendBytes(
            reinterpret_cast<const char *>(packet_data.data()),
            static_cast<int>(packet_data.size()));
        if(sent != static_cast<int>(packet_data.size()))
            throw std::runtime_error("Could not send the complete UM7 configuration packet");
    }


    void
    SendConfig(std::uint8_t address, std::array<std::uint8_t, 4> data)
    {
        std::array<std::uint8_t, 11> packet_data
        {
            's', 'n', 'p', 0x80, address,
            data[0], data[1], data[2], data[3], 0, 0,
        };
        std::uint16_t checksum = 0;
        for(std::size_t i = 0; i < 9; ++i)
            checksum += packet_data[i];
        packet_data[9] = static_cast<std::uint8_t>(checksum >> 8);
        packet_data[10] = static_cast<std::uint8_t>(checksum);
        SendPacket(packet_data);
    }


    void
    DisableBroadcast()
    {
        for(std::uint8_t address = 1; address <= 7; ++address)
            SendConfig(address, {0, 0, 0, 0});
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

        serial = std::make_unique<Serial>(std::string(port), baudrate);
        Debug(std::string("Serial port for UM7 opened on ") + std::string(port).c_str() + " with baudrate " + std::to_string(baudrate) + "\n");

        // Reserve buffer space to avoid frequent reallocations
        rx_buffer.reserve(MAX_BUFFER_SIZE);

        DisableBroadcast();
        SendConfig(0x05, {0, 100, 0, 0}); // Enable Euler angles at 255 Hz
        SendConfig(0x03, {100, 100, 0, 0}); // Enable accelerometer and gyro at 255 Hz
        Debug( "UM7 configured for gyro, accelerometer, and Euler angle data broadcasting.");
    }

    void Tick()
    {
        // Tick
        
        // Read available data with short timeout for responsive operation
        rx_length = serial->ReceiveBytes(rx_data.data(),
                                         static_cast<int>(rx_data.size()), 5);
        if (rx_length <= 0)
        {
            return;
        }

        rx_buffer.append(rx_data.data(), rx_length);

        // Parse all available packets
        int parse_result;
        int packets_processed = 0;
        do {
            parse_result = parsePacket();
            if (parse_result == 0)
            {
                packets_processed++;
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
        } while (parse_result == 0 && packets_processed < 10); // Limit to prevent infinite loops

        // Only log errors occasionally to prevent spam
        static int error_count = 0;
        if (parse_result > 1 && ++error_count % 100 == 0)
        {
            switch(parse_result)
            {
            case 2: Debug("No header found (logged every 100 errors)"); break;
            case 3: Debug("Incomplete packets (logged every 100 errors)"); break;
            case 4: Debug("Checksum errors (logged every 100 errors)"); break;
            }
        }

        // Manage buffer size to prevent memory issues
        if (rx_buffer.size() > MAX_BUFFER_SIZE)
        {
            rx_buffer.clear();
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


        // std::cout << "Gyro X: " << (float)gyroProc[0] << " deg/s, Y: " << (float)gyroProc[1] << " deg/s, Z: " << (float)gyroProc[2] << " deg/s" << std::endl;
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
        // Total packet length = 3(header) + 1(PT) + 1(addr) + data_length + 2(checksum) = 7 + data_length
        if ((rx_buffer.size() - packet_index) < (data_length + 7))
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
            // Enhanced debugging for checksum mismatch
            std::string debug_msg = "Checksum mismatch at pos " + std::to_string(packet_index) + 
                                  ": computed=0x" + std::to_string(computed_checksum) +
                                  ", received=0x" + std::to_string(received_checksum) +
                                  ", PT=0x" + std::to_string(PT) +
                                  ", Addr=0x" + std::to_string(packet.Address) +
                                  ", DataLen=" + std::to_string(data_length);
            
            // Show some bytes around the packet for context
            debug_msg += ", Bytes: ";
            int bytes_to_show = 10;
            if (bytes_to_show > (int)rx_buffer.size() - packet_index)
                bytes_to_show = (int)rx_buffer.size() - packet_index;
            for (int i = 0; i < bytes_to_show; i++)
            {
                char hex[4];
                snprintf(hex, sizeof(hex), "%02X ", (unsigned char)rx_buffer[packet_index + i]);
                debug_msg += hex;
            }
            Debug(debug_msg);
            
            // Remove packet from buffer and try to find next valid packet
            rx_buffer.erase(0, packet_index + 1); // Remove just the bad start, not the whole packet
            return 4;                             // Bad checksum
        }

        // Successfully parsed packet, remove it from the buffer              
        size_t erase_length = packet_index + 7 + data_length; // 3(header) + 1(PT) + 1(addr) + data + 2(checksum)
        if (erase_length > rx_buffer.size())
        {
            Warning( "Error: Attempt to erase beyond buffer size. Buffer size: " + std::to_string(rx_buffer.size())
                      + ", Requested erase length: " + std::to_string(erase_length));
            return 3; // Not enough data yet
        }

        rx_buffer.erase(0, erase_length);
        return 0; // Successfully parsed

    }

};

INSTALL_CLASS(UM7)
