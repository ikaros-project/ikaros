//
//	UM7.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2023 Birger Johansson
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
// Code from manual
// https://www.pololu.com/file/0J1556/UM7%20Datasheet_v1-8_30.07.2018.pdf

#include "ikaros.h"

using namespace ikaros;

class UM7 : public Module
{
    parameter port;
    matrix roll;
    matrix pitch;
    matrix yaw;

    // Structure for holding received packet information
    typedef struct UM7_packet_struct
    {
        uint8_t Address;
        uint8_t PT;
        uint16_t Checksum;
        uint8_t data_length;
        uint8_t data[30];
    } UM7_packet;

    UM7_packet packet;
    bool packet_has_data = false;

    Serial *s;
    int rx_length;
    char rx_data[255];

    void Init()
    {
        Bind(port, "port");
        Bind(roll, "ROLL");
        Bind(pitch, "PITCH");
        Bind(yaw, "YAW");

        // Create a serial port
        s = NULL;

        // s = new Serial("/dev/cu.usbserial-AU04OEIL", 115200); // Hardcoded baudrate.
        s = new Serial(std::string(port).c_str(), 115200); // Hardcoded baudrate.

        // Set parameters for the sensor
        // prevent all broadcast from UM7
        // printf("Disable broadcast for UM7");
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

        // Setting wanted bradcast
        // printf("Enable ikaros related UM7 broadcast");

        uint8_t tx_data[20];
        tx_data[0] = 's';
        tx_data[1] = 'n';
        tx_data[2] = 'p';
        tx_data[3] = 0x80; // Packet Type byte
        tx_data[4] = 0x05; // Address
        tx_data[5] = 0x00; // Data
        tx_data[6] = 0xFF; // Data // Maximum broadcast of angles in hz
        tx_data[7] = 0x00; // Data
        tx_data[8] = 0x00; // Data
        uint16_t computed_checksum = tx_data[0] + tx_data[1] + tx_data[2] + tx_data[3] + tx_data[4] + tx_data[5] + tx_data[6] + tx_data[7] + tx_data[8];
        tx_data[9] = (computed_checksum >> 8) & 0xFF;
        tx_data[10] = computed_checksum & 0xFF;
        s->SendBytes((char *)tx_data, 11);
    }

    void Tick()
    {
        // Notify(log_level_debug, "Look for UM7 broadcast message from serial");

        // Check new data
        packet_has_data = false;

        while (1) // Read data until buffer is empty
        {
            printf("*");
            rx_length = s->ReceiveBytes(rx_data, 255, 0);
            // Notify(log_level_debug, "Data length %i\n", rx_length);

            if (rx_length <= 11 and !packet_has_data) // No data this tick. I think the minimal lenght
            {
                // Notify(log_level_trace, "No data or incomplete data recived from UM7.");
                return;
            }

            if (rx_length <= 0 and packet_has_data) // Read all buffer and have data
            {
                // Notify(log_level_trace, "Recived some data.");
                break;
            }

            packet_has_data = true;
            while (1)
            {

                if (parsePacket() != 0) // it can be a long message and we want the last message.
                {
                    packet_has_data = false;
                    // Notify(log_level_trace, "Data not parsed ");
                    break;
                }
                else
                {
                    packet_has_data = true;
                    // Notify(log_level_trace, "Data parsed");
                }

                // if (packet.Address > 0)
                //     printf("Packet Adress %i\n", packet.Address);

                // Extracting the data used by ikaros
                if (packet.Address == 0x70) // 112
                {
                    // printf("HZ: %f Time %f\n", 1000.0/T.GetTime(),T.GetTime()); // This is not
                    // printf("Got data for ikaros\n");
                    // T.Reset();

                    int16_t estRoll;
                    int16_t estPitch;
                    int16_t estYaw;

                    estRoll = (packet.data[0] << 8) | packet.data[1];
                    estPitch = (packet.data[2] << 8) | packet.data[3];
                    estYaw = (packet.data[4] << 8) | packet.data[5];

                    estRoll = estRoll / 91.02222;
                    estPitch = estPitch / 91.02222;
                    estYaw = estYaw / 91.02222;

                    roll = estRoll;
                    pitch = estPitch;
                    yaw = estYaw;
                    // printf("%i\n", estYaw);
                }
                /* code */
            }
        }
    }
    int parsePacket()
    {

        uint8_t data_length = 0;
        uint8_t index;
        uint8_t packet_index;
        // parse_serial_data
        // This function parses the data in 'rx_data' with length 'rx_length' and attempts to find a packet // in the data. If a packet is found, the structure 'packet' is filled with the packet data.
        // If there is not enough data for a full packet in the provided array, parse_serial_data returns 1. // If there is enough data, but no packet header was found, parse_serial_data returns 2.
        // If a packet header was found, but there was insufficient data to parse the whole packet,
        // then parse_serial_data returns 3. This could happen if not all of the serial data has been
        // received when parse_serial_data is called.
        // If a packet was received, but the checksum was bad, parse_serial_data returns 4.
        // If a good packet was received, parse_serial_data fills the UM7_packet structure and returns 0. uint8_t parse_serial_data( uint8_t* rx_data, uint8_t rx_length, UM7_packet* packet )
        // if (packet_has_data)
        // {
        // Try to find the 'snp' start sequence for the packet
        for (index = 0; index < (rx_length - 2); index++)
        {
            // Check for 'snp'. If found, immediately exit the loop
            if (rx_data[index] == 's' && rx_data[index + 1] == 'n' && rx_data[index + 2] == 'p')
            {
                break;
            }
        }
        // uint8_t packet_index = index;
        packet_index = index;
        // Check to see if the variable 'packet_index' is equal to (rx_length - 2). If it is, then the above // loop executed to completion and never found a packet header.
        if (packet_index == (rx_length - 2))
            return 2;
        // If we get here, a packet header was found. Now check to see if we have enough room
        // left in the buffer to contain a full packet. Note that at this point, the variable 'packet_index' // contains the location of the 's' character in the buffer (the first byte in the header)
        if ((rx_length - packet_index) < 7)
            return 3;
        // We've found a packet header, and there is enough space left in the buffer for at least
        // the smallest allowable packet length (7 bytes). Pull out the packet type byte to determine // the actual length of this packet
        uint8_t PT = rx_data[packet_index + 3];
        uint8_t packet_is_batch = (PT >> 6) & 0x01;
        uint8_t batch_length = (PT >> 2) & 0x0F;
        // Now finally figure out the actual packet length

        if (packet_has_data)
        {
            if (packet_is_batch)
            {
                // Packet has data and is a batch. This means it contains 'batch_length' registers, each // of which has a length of 4 bytes
                data_length = 4 * batch_length;
            }
            else // Packet has data but is not a batch. This means it contains one register (4 bytes) {
                data_length = 4;
        }
        // Do some bit-level manipulation to determine if the packet contains data and if it is a batch // We have to do this because the individual bits in the PT byte specify the contents of the
        // packet.
        uint8_t packet_has_data = (PT >> 7) & 0x01;
        // }
        // else
        // {
        //     // Packet has no data
        //     data_length = 0;
        // }

        // At this point, we know exactly how long the packet is. Now we can check to make sure // we have enough data for the full packet.
        if ((rx_length - packet_index) < (data_length + 5))
            return 3;

        // If we get here, we know that we have a full packet in the buffer. All that remains is to pull // out the data and make sure the checksum is good.
        // Start by extracting all the data

        // UM7_packet packet;
        packet.Address = rx_data[packet_index + 4];
        packet.PT = rx_data[packet_index + 3];
        // Get the data bytes and compute the checksum all in one step
        packet.data_length = data_length;
        uint16_t computed_checksum = 's' + 'n' + 'p' + packet.PT + packet.Address;
        for (index = 0; index < data_length; index++)
        {
            // Copy the data into the packet structure's data array
            packet.data[index] = rx_data[packet_index + 5 + index];
            // Add the new byte to the checksum
            computed_checksum += packet.data[index];
        }
        // Now see if our computed checksum matches the received checksum
        // First extract the checksum from the packet
        uint16_t received_checksum = (rx_data[packet_index + 5 + data_length] << 8);
        received_checksum |= rx_data[packet_index + 6 + data_length];
        // Now check to see if they don't match
        if (received_checksum != computed_checksum)
            return 4;
        // At this point, we've received a full packet with a good checksum. It is already
        // fully parsed and copied to the 'packet' structure, so return 0 to indicate that a packet was // processed.

        // printf("Everything is fine!\n");

        // Zero out the message just parsed. To be able to run the function again until all messages is parsed.
        for (index = 0; index < data_length; index++)
            rx_data[packet_index + index] = 0;

        return 0;
    }
};

INSTALL_CLASS(UM7)
