//
//    KalmanFilter.cc		This file is a part of the IKAROS project
//                          The module implements a standard Kalman filter
//
//    Copyright (C) 2009 Christian Balkenius
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
//	Created: 2009-05-27
//
//	<Additional description of the module>


#include "KalmanFilter.h"

using namespace ikaros;



void
KalmanFilter::Init()
{
    Bind(process_noise, "process_noise");
    Bind(observation_noise, "observation_noise");
    
    u_size = GetInputSize("INPUT");
    z_size = GetInputSize("OBSERVATION");
    x_size = GetOutputSize("STATE");
    
    u = GetInputArray("INPUT");
    z = GetInputArray("OBSERVATION");
    x = GetOutputArray("STATE");
    y = GetOutputArray("INNOVATION");
    
    A = GetMatrix("A", x_size, x_size);
    B = GetMatrix("B", u_size, x_size);
    H = GetMatrix("H", x_size, z_size);

    A_T = transpose(create_matrix(x_size, x_size), A, x_size, x_size);
    B_T = transpose(create_matrix(x_size, u_size), B, x_size, u_size);
    H_T = transpose(create_matrix(z_size, x_size), H, z_size, x_size);

    P = eye(create_matrix(x_size, x_size), x_size);
    Q = multiply(eye(create_matrix(x_size, x_size), x_size), process_noise, x_size, x_size); // Process noise
    R = multiply(eye(create_matrix(z_size, z_size), z_size), observation_noise, z_size, z_size); // Observation (measurement) noise
    
//    K = create_matrix(z_size, x_size);
    K = GetOutputMatrix("KALMAN_GAIN");
//    int ksx = GetOutputSizeX("KALMAN_GAIN");
//    int ksy = GetOutputSizeY("KALMAN_GAIN");
    
    I = eye(create_matrix(x_size, x_size), x_size);
}



KalmanFilter::~KalmanFilter()
{
    destroy_matrix(A);
    destroy_matrix(B);
    destroy_matrix(H);

    destroy_matrix(A_T);
    destroy_matrix(B_T);
    destroy_matrix(H_T);

    destroy_matrix(P);
    destroy_matrix(Q);
    destroy_matrix(R);

    destroy_matrix(K);
    destroy_matrix(I);
}



void
KalmanFilter::Tick()
{
    // Recalculate variances for interactive demo

    multiply(eye(Q, x_size), process_noise, x_size, x_size);
    multiply(eye(R, z_size), observation_noise, z_size, z_size);

    // Allocate temporary matrices

    float *     x_p = create_array(x_size);
    float **    P_p = create_matrix(x_size, x_size);
    float **    tmp_zz = create_matrix(z_size, z_size);
    float **    tmp_xz = create_matrix(x_size, z_size);
    float **    tmp_zx = create_matrix(z_size, x_size);
    float **    tmp_xx = create_matrix(x_size, x_size);
    float **    tmp_xx2 = create_matrix(x_size, x_size);
    float *     tmp_z = create_array(z_size);
    float **    S = create_matrix(z_size, z_size);
    
    float *     Ax = create_array(x_size);
    float *     Bu = create_array(x_size);

    // Prediction: x_p = A*x + B*u
    
    multiply(Ax, A, x, x_size, x_size);
    multiply(Bu, B, u, u_size, x_size);
    add(x_p, Ax, Bu, x_size);
    
//    print_array("  x", x, x_size, x_size);
//    print_array("x_p", x_p, x_size, x_size);
    
    // Prediction: P_p = A*P*T(A) + Q

    multiply(P_p, A, P, x_size, x_size, x_size);
    multiply(tmp_xx, P_p, A_T, x_size, x_size, x_size);
    add(P_p, tmp_xx, Q, x_size, x_size);
    
    // Measurement residual: y = z - H * x_p (innovation)

    multiply(tmp_z, H, x_p, x_size, z_size);
    subtract(y, z, tmp_z, z_size);
//    print_array("  y", y, z_size);

    // Residual covariance: S = H * P_p * H_T + R
    
    multiply(tmp_xz, H, P_p, x_size, z_size, x_size);
    multiply(tmp_zz, tmp_xz, H_T, z_size, z_size, x_size);
    add(S, tmp_zz, R, z_size, z_size);
    
    // Optimal Kalman gain: K = P_p * H_T * Inv(S)
    
    multiply(K, P_p, H_T, z_size, x_size, x_size);
    bool r = inv(tmp_zz, S, z_size);
    if(!r)
    {
        Notify(msg_warning, "KalmanFilter: Matrix S is singular.\n");
        return;
    }
    
    multiply(tmp_zx, K, tmp_zz, z_size, x_size, z_size);
    copy_matrix(K, tmp_zx, z_size, x_size);
//    print_matrix("  K", K, z_size, x_size);
    
    // Posteriori update: x = x_p + K * y

    multiply(x, K, y, z_size, x_size);
    add(x, x_p, x_size);

    // Posteriori update: P = (I - K * H) * P_p

    multiply(tmp_xx, K, H, x_size, x_size, z_size);
    subtract(tmp_xx2, I, tmp_xx, x_size, x_size);
    multiply(P, tmp_xx2, P_p, x_size, x_size, x_size);
//    print_matrix("  P", P, x_size, x_size);

    // Clean up

    // FIXME: Clean upp the temporary matrices, or better still make them permanent
}


