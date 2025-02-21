//
//	UM7.h		This file is a part of the IKAROS project
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

#ifndef UM7_
#define UM7_

#include "IKAROS.h"

// Structure for holding received packet information
typedef struct UM7_packet_struct
{
    uint8_t Address;
    uint8_t PT;
    uint16_t Checksum;
    uint8_t data_length;
    uint8_t data[30];
} UM7_packet;

class UM7 : public Module
{
public:
    static Module *Create(Parameter *p) { return new UM7(p); }

    UM7(Parameter *p) : Module(p) {}
    virtual ~UM7() {}

    void Init();
    void Tick();

    Serial *s;
    int rx_length;
    char rx_data[255];

    int parsePacket();

    UM7_packet packet;
    bool packet_has_data = false;

    float *Roll_OUTPUT;
    float *Pitch_OUTPUT;
    float *Yaw_OUTPUT;

    // Timer T; // debug
};

#endif
