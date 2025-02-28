//
//	FadeCandy.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2025 Birger Johansson
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

// Using fadecandy driver from https://github.com/iron-ox/fadecandy_ros/tree/master/fadecandy_driver

#include "fadecandy_driver.h" // Include the fadecandy driver

#include "ikaros.h"

using namespace ikaros;
using namespace fadecandy_driver;

class FadeCandy : public Module
{
    matrix RightEye, LeftEye, MouthLow, MouthHigh;
    fadecandy_driver::FadecandyDriver fd_driver;
    parameter simulate;

    // Initialize the led array colors
    std::vector<std::vector<Color>> led_array_colors = {
        std::vector<Color>(8, Color(0, 0, 0)),  // (Mouth high)
        std::vector<Color>(8, Color(0, 0, 0)),  // (Mouth low)
        std::vector<Color>(12, Color(0, 0, 0)), // (Right eye)
        std::vector<Color>(12, Color(0, 0, 0))  // (Left eye)
    };

    void Init()
    {

        Bind(LeftEye, "LEFT_EYE");
        Bind(RightEye, "RIGHT_EYE");
        Bind(MouthHigh, "MOUTH_HIGH");
        Bind(MouthLow, "MOUTH_LOW");

        Bind(simulate, "simulate");

        if (simulate)
        {
            Notify(msg_warning, "Simulate fadecandy");
            return;
        }
        if ((LeftEye.size_x() <= 12 && LeftEye.size_y() != 3))
            Notify(msg_warning, "Input LEFT_EYE size is not 3x12");
        if ((RightEye.size() <= 12 && RightEye.size_y() != 3))
            Notify(msg_warning, "Input RIGHT_EYE size is not 3x12");
        if ((MouthHigh.size() <= 8 && MouthHigh.size_y() != 3))
            Notify(msg_warning, "Input MOUTH_HIGH size is not 3x8");
        if ((MouthLow.size() <= 8 && MouthLow.size_y() != 3))
            Notify(msg_warning, "Input MOUTH_LOW size is not 3x8");

        try
        {
            auto serial_number = fd_driver.connect();
            Notify(msg_debug, "Connected to Fadecandy board: " + serial_number);
        }
        catch (const std::exception &e)
        {
            Notify(msg_warning, "Could not connect to Fadecandy board"); // Should this be fatal?
            return;
        }
    }

    void
    Tick()
    {
        if (simulate)
            return;

        if (!fd_driver.isConnected())
        {
            try
            {
                Notify(msg_debug, "Reconnecting to Fadecandy board: ");
                auto serial_number = fd_driver.connect();
                Notify(msg_warning, "Reconnecting to Fadecandy board: " + serial_number);
            }
            catch (const std::exception &e)
            {
                Notify(msg_debug, "Could not connect to Fadecandy board");
                return;
            }
        }

        // Fill color from input
        for (size_t i = 0; i < 8; ++i) // 8 Leds in each row of the mouth
        {
            led_array_colors[1][i] = Color(MouthHigh[0][i] * 255, MouthHigh[1][i] * 255, MouthHigh[2][i] * 255);
            led_array_colors[0][i] = Color(MouthLow[0][i] * 255, MouthLow[1][i] * 255, MouthLow[2][i] * 255);
        }
        for (size_t i = 0; i < 12; ++i) // 12 Leds in each eye
        {
            led_array_colors[2][i] = Color(RightEye[0][i] * 255, RightEye[1][i] * 255, RightEye[2][i] * 255);
            led_array_colors[3][i] = Color(LeftEye[0][i] * 255, LeftEye[1][i] * 255, LeftEye[2][i] * 255);
        }

        try
        {
            fd_driver.setColors(led_array_colors); // Send the colors to the driver. Set color is checking that isConnected is true. However,if the device is unpluged this is not detected by the driver.
        }
        catch (const std::exception &e)
        {
            Notify(msg_debug, "Could not set colors of the eyes");
        }
    }
};
INSTALL_CLASS(FadeCandy)
