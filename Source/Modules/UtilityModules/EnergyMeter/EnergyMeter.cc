#include "ikaros.h"

using namespace ikaros;
#include <iostream>

class EnergyMeter: public Module
{
    Socket      socket;

    parameter address;
    matrix measured_power;
    matrix energy;

    void Init()
    {
        Bind(address, "address");
        Bind(measured_power, "MEASURED_POWER");
        Bind(energy, "ENERGY");
    }


    void Tick()
    {
        try
        {
            // auto data = socket.HTTPGet("192.168.50.59/status");
            //std::cout << "address: " << address << "\n";
            auto data = socket.HTTPGet(address);
            measured_power[0] = std::stof(split(data, "\"power\":", 1).at(1)); // Not exactly JSON parsing but it will do.
            //std::cout << "measured power: \n" << split(data, "\"power\":", 1).at(1);
            //std::cout << "1\n";
        }
        catch(const std::exception& e)
        {
            energy[0] = 0;
            //std::cout << "2\n";
        }

        energy[0] += GetTickDuration()*(measured_power[0]) /(1000.0*1000.0*3600.0); // Convert time interval to hours and integrate to Wh
        //printf("ENERGY: %d, POWER: %d\n", energy[0], measured_power[0]);
        // std::cout << "Energy: " << (float)energy[0] << "; Power: " << (float)measured_power[0] << "\n";
    
    }
};

INSTALL_CLASS(EnergyMeter)

