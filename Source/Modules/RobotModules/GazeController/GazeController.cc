//
//	  GazeController.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2014 Christian Balkenius
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
//  This example is intended as a starting point for writing new Ikaros modules
//  The example includes most of the calls that you may want to use in a module.
//  If you prefer to start with a clean example, use he module MinimalModule instead.
//

#include "GazeController.h"

#include <unistd.h>

using namespace ikaros;

enum {neckTilt, neckPan, camL, camR};



void
GazeController::Init()
{
    Bind(A, "A");
    Bind(B, "B");
    Bind(C, "C");
    Bind(D, "D");
    Bind(E, "E");

    Bind(offset, 4, "offset");
    Bind(center_override, "center_override");

    Bind(target, 3, "target");
    Bind(target_override, "target_override");

    Bind(gamma, "gamma");

    input = GetInputArray("INPUT"); // Assume 4x4
    output = GetOutputArray("OUTPUT");
    
    output[neckPan]     = 0.5;
    output[neckTilt]    = 0.5;
    output[camL]        = 0.5;
    output[camR]        = 0.5;
    
    view_side = GetOutputMatrix("VIEW_SIDE");
    view_top  = GetOutputMatrix("VIEW_TOP");
}



GazeController::~GazeController()
{
}



static void
line(float ** o, int & c, float x0, float y0, float x1, float y1)
{
    float offset = 0.2;
    float scale = 1-2*offset;
    o[c][0] = offset+scale*x0;
    o[c][1] = offset+scale*y0;
    o[c][2] = offset+scale*x1;
    o[c][3] = offset+scale*y1;
    c++;
}



void
GazeController::Tick()
{

	
	
    if(center_override)
    {
        set_array(output, 0.0, 4);
        add(output, offset,4);
        print_array("offset", offset, 4);
        return;
    }
    
    
    // Get target position
    
	float x = h_get_x(input);
    float y = h_get_y(input);
    float z = h_get_z(input);

    if(target_override)
    {
        x = target[0];
        y = target[1];
        z = target[2];
    }
	
	if (x == 0 && y == 0 && z == 0)
	{
		return;
	}
	
    // Calculate angles to target
    
    float e     = A - z;
    float f     = hypot(x, e);
    
    if(f < B)
    {
        printf("WARNING: target cannot be reached\n");
        f = B;
    }
    
    float alpha = atan2(x, e);
    
    float beta  = acos(B/f);
    float theta = pi - alpha - beta;
    float targetTilt    = theta;
    
    
    float d = sqrt(f*f-B*B);
    float omega = atan2(y,d);   // d == 0 should never happen
    float omega_prime = gamma*omega;    // angle for head turn

    float targetPan     = omega_prime;
  
    // printf("%f %f %f  =>  %f %f\n", x, y, z, targetPan,  targetTilt);
	if (targetPan > pi)
		targetPan  = targetPan - pi;
	
	if (targetTilt > pi)
		targetTilt  = targetTilt - pi;
	
	
    float Fx = C*cos(omega_prime);
    float Fy = C*sin(omega_prime);

    float ELx = Fx + (D/2)*sin(omega_prime);
    float ELy = Fy - (D/2)*cos(omega_prime);

    float ERx = Fx - (D/2)*sin(omega_prime);
    float ERy = Fy + (D/2)*cos(omega_prime);

    float TLy = (y-ELy);
    float TLx = (d-ELx);

    float TRy = (y-ERy);
    float TRx = (d-ERx);
    
    float phi_L = atan2(TLy, TLx) - omega_prime;
    float phi_R = atan2(TRy, TRx) - omega_prime;
   
    // Set view outputs
    
    // view_side
    
    int c = 0;
    line(view_side, c, 0, z, x, z);
    line(view_side, c, 0, 0, 0, z);
    line(view_side, c, 0, 0, x, 0);
    line(view_side, c, x, 0, x, z);
    line(view_side, c, 0, A, x, z);

    line(view_side, c, 0, 0, 0, A);
    line(view_side, c, 0, A, B*sin(theta), A+B*cos(theta));
    line(view_side, c, B*sin(theta), A+B*cos(theta), B*sin(theta)+C*cos(theta), A+B*cos(theta)-C*sin(theta));
    line(view_side, c, B*sin(theta)+C*cos(theta), A+B*cos(theta)-C*sin(theta), x, z);
   
    // view_top
    
    c = 0;
    line(view_top, c,   0.5,    0,   0.5,     d);
    line(view_top, c,   0.5,    d,   0.5+y,   d);
    line(view_top, c,   0.5+y,  d,   0.5+y,   0);
    line(view_top, c,   0.5+y,  0,   0.5,     0);
    
    line(view_top, c,   0.5+y,  d,   0.5,     0);   // target direction

    line(view_top, c,   0.5,     0,   0.5+Fy, Fx);  // head kinematics
    line(view_top, c,   0.5+ELy, ELx, 0.5+ERy, ERx);

    line(view_top, c,   0.5+ELy, ELx, 0.5+ELy+E*sin(omega_prime+phi_L), ELx+E*cos(omega_prime+phi_L)); // left eye rotation
    line(view_top, c,   0.5+ERy, ERx, 0.5+ERy+E*sin(omega_prime+phi_R), ERx+E*cos(omega_prime+phi_R)); // right eye rotation
    
    line(view_top, c,   0.5+ELy+E*sin(omega_prime+phi_L), ELx+E*cos(omega_prime+phi_L), 0.5+y, d);    // left gaze direction
    line(view_top, c,   0.5+ERy+E*sin(omega_prime+phi_R), ERx+E*cos(omega_prime+phi_R), 0.5+y, d);    // right gaze direction
    
    // Set control output

	output[neckPan]     = targetPan ;
	output[neckTilt]    = targetTilt;
	output[camL]        = phi_L;
	output[camR]        = phi_R;
	
	// Setting the angle back to angle_unit
	output[neckPan] = angle_to_angle(output[neckPan],radians,angle_unit);
	output[neckTilt] = angle_to_angle(output[neckTilt],radians,angle_unit);
	output[camL] = angle_to_angle(output[camL],radians,angle_unit);
	output[camR] = angle_to_angle(output[camR],radians,angle_unit);
	
	
    add(output, offset, 4);
}



// Install the module. This code is executed during start-up.

static InitClass init("GazeController", &GazeController::Create, "Source/Modules/RobotModules/GazeController/");


