//
//	Dynamixel.h		This file is a part of the IKAROS project
//
//    Copyright (C) 2010-2011  Christian Balkenius
//
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

#ifndef DynamixelConfigure_
#define DynamixelConfigure_

#include "IKAROS.h"
#include "DynamixelComm.h"
#include "DynamixelServo.h"

class DynamixelConfigure: public Module
{
public:
    static Module * Create(Parameter * p) { return new DynamixelConfigure(p); }
    
	DynamixelConfigure(Parameter * p);
	virtual ~DynamixelConfigure();
    
    void        SetSizes();
    
	void		Print();
    void        PrintAll();
    
	void		Init();
	void		Tick();
    
private:
    int         size;
    int         servos;
    int         max_servo_id;
    int         init_print;
    int         index_mode;
    int         angle_unit;
    float       position_speed; // global speed in fraction of max
    
    float       timebase;
    bool        use_feedback;
    int         baud_rate;
    int         start_up_delay;
    bool        list_servos;
    
    
    // Parameters
    int set_id;
    int set_baud_rate;              // mbps
    int set_return_delay_time;      // ms
    float set_cw_angle_limit;       // Angle unit
  	float set_ccw_angle_limit;      // Angle unit
    int set_drive_mode;             // 0-1
    float set_limit_temperature;    // Celsius
    float set_lowest_limit_voltage; // Volt
    float set_highest_limit_voltage;// Volt
    float set_max_torque;           // 100 %
    int set_status_return_level;    // 0,1,2
    int set_alarm_led;              // 0-255
    int set_alarm_shutdown;         // 0-255
    bool set_torque_enable;         // True/false
    bool set_led;                   // On/Off
    int set_d_gain;                 // 0-254
    int set_i_gain;                 // 0-254
    int set_p_gain;                 // 0-254
    float set_goal_position;        // Angle unit
    float set_moving_speed;         // 0-1
    float set_torque_limit;         // 0-1
    bool set_lock;                  // On/Off
    float set_punch;                // 0.03235-1
    
    // Set in ikc file?
    bool set_id_change;
    bool set_baud_rate_change;              // mbps
    bool set_return_delay_time_change;      // ms
    bool set_cw_angle_limit_change;       // Angle unit
  	bool set_ccw_angle_limit_change;      // Angle unit
    bool set_drive_mode_change;             // 0-1
    bool set_limit_temperature_change;    // Celsius
    bool set_lowest_limit_voltage_change; // Volt
    bool set_highest_limit_voltage_change;// Volt
    bool set_max_torque_change;           // 100 %
    bool set_status_return_level_change;    // 0,1,2
    bool set_alarm_led_change;              // 0-255
    bool set_alarm_shutdown_change;         // 0-255
    bool set_torque_enable_change;         // True/false
    bool set_led_change;                   // On/Off
    bool set_d_gain_change;                 // 0-254
    bool set_i_gain_change;                 // 0-254
    bool set_p_gain_change;                 // 0-254
    bool set_goal_position_change;        // Angle unit
    bool set_moving_speed_change;         // 0-1
    bool set_torque_limit_change;         // 0-1
    bool set_lock_change;                  // On/Off
    bool set_punch_change;                // 0.03235-1
    
    // Continuous rotation parameters
    
    float       gain_alpha;
    float       gain_initial;
    float       deadband_initial;
    
    // Inputs and outputs
    // Inputs
    float *     torqueEnable;
    bool        allocated_torqueEnable;
    float *     LED;
    bool        allocated_LED;
    float *     dGain;
    bool        allocated_dGain;
    float *     iGain;
    bool        allocated_iGain;
    float *     pGain;
    bool        allocated_pGain;
    float *     goalPosition;
    bool        allocated_goalPosition;
    float *     movingSpeed;
    bool        allocated_movingSpeed;
    float *     torqueLimit;
    bool        allocated_torqueLimit;
    
    // Outputs
    float * feedbackTorqueEnable;
    float * feedbackLED;
    float * feedbackDGain;
    float * feedbackIGain;
    float * feedbackPGain;
    float * feedbackGoalPosition;
    float * feedbackMovingPostion;
    float * feedbackTorqueLimit;
    float * feedbackPresentPosition;
    float * feedbackPresentSpeed;
    float * feedbackPresentLoad;
    float * feedbackPresentVoltage;
    float * feedbackPresentTemperature;
    float * feedbackPresentCurrent;
    
    
    float * resetModeOut;
    float * changeModeOut;
    
    float * set;
    float * active;
    // Array of servo data
    
    bool resetMode;
    bool scan_mode;
    DynamixelServo **    servo;
    
    // Arrays used to send commands to servos // FIXME: USE separat list for continous servos
    
    int *       servo_index;
    int *       servo_id;
    
    unsigned char ** DynamixelMemoeries;
    
    
    
    const char *    device;
    DynamixelComm * com;
    
    void parseControlTable(Dictionary * dic, unsigned char* buf, int from, int to);
    int checkBaudRate(int br);
    void PrintChange(int active);
    
    int blink;
    
};

#endif

