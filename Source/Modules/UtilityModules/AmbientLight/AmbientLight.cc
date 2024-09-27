#include "ikaros.h"

#include <math.h>
#include <iostream>
#include <cmath>
#include <ctime>

using namespace ikaros;

static void calculateIrradianceAndDiffuse(double latitude, double longitude, int start_year, int start_month, int start_day, int total_seconds, double &direct_normal_irradiance, double &diffuse_sky_radiation) {
    int days_passed = total_seconds / 86400; // Calculate the number of complete days passed
    int seconds_since_midnight = total_seconds % 86400; // Calculate the seconds since midnight for the current day
    std::tm tm = {}; 
    tm.tm_year = start_year - 1900; tm.tm_mon = start_month - 1; tm.tm_mday = start_day + days_passed; // Set the start date and adjust based on days passed
    std::mktime(&tm); // Use std::mktime to adjust the date correctly, considering month/year overflow and leap years
    std::tm start_of_year_tm = tm; 
    start_of_year_tm.tm_mon = 0; start_of_year_tm.tm_mday = 1; // Set to the first day of the year
    std::time_t start_of_year_time = std::mktime(&start_of_year_tm); // Convert to time_t to get start of year time
    int day_of_year = std::difftime(std::mktime(&tm), start_of_year_time) / (60 * 60 * 24) + 1; // Calculate the day of the year
    double solar_declination = 23.44 * std::sin((360.0 / 365.24) * (day_of_year - 81) * M_PI / 180.0); // Calculate solar declination
    double B = 2 * M_PI * (day_of_year - 81) / 364; // Calculate B for equation of time
    double equation_of_time = 9.87 * std::sin(2 * B) - 7.53 * std::cos(B) - 1.5 * std::sin(B); // Calculate the equation of time
    double time_correction = 4 * (longitude - 15 * std::floor(longitude / 15)) + equation_of_time; // Calculate the time correction factor
    double solar_time = (seconds_since_midnight / 3600.0) + time_correction / 60.0; // Convert seconds to hours and calculate solar time
    double solar_hour_angle = (solar_time - 12) * 15; // Calculate solar hour angle
    double latitude_rad = latitude * M_PI / 180.0; // Convert latitude to radians
    double solar_declination_rad = solar_declination * M_PI / 180.0; // Convert solar declination to radians
    double solar_hour_angle_rad = solar_hour_angle * M_PI / 180.0; // Convert solar hour angle to radians
    double solar_altitude = std::asin(std::sin(latitude_rad) * std::sin(solar_declination_rad) + std::cos(latitude_rad) * std::cos(solar_declination_rad) * std::cos(solar_hour_angle_rad)) * 180.0 / M_PI; // Calculate solar altitude
    direct_normal_irradiance = 0.0;
    if (solar_altitude > 0) 
        direct_normal_irradiance = 1361 * std::sin(solar_altitude * M_PI / 180.0); // Calculate direct normal irradiance
    double solar_zenith_angle = 90.0 - solar_altitude; // Calculate solar zenith angle
    double diffuse_fraction; // Declare variable for diffuse fraction
    double solar_zenith_angle_rad = solar_zenith_angle * M_PI / 180.0; // Convert the solar zenith angle from degrees to radians
    if (solar_zenith_angle <= 80.0) 
        diffuse_fraction = 0.3 + 0.2 * std::sin(solar_zenith_angle_rad); // Calculate diffuse fraction for low zenith angles
    else 
        diffuse_fraction = 0.7; // Higher zenith angle results in more scattering, hence higher diffuse fraction
    diffuse_sky_radiation = diffuse_fraction * direct_normal_irradiance * std::cos(solar_zenith_angle_rad); // Calculate diffuse sky radiation
}




class AmbientLight: public Module
{
    parameter   longitude;
    parameter   latitude;

    parameter   radiation;
    parameter   method;
    parameter   use_system_time;

    parameter   year;
    parameter   month;
    parameter   day;


    matrix      output;

    void Init()
    {
        Bind(radiation, "radiation");
        Bind(method, "method");
        Bind(use_system_time, "use_system_time");

        Bind(longitude, "longitude");
        Bind(latitude, "latitude");

        Bind(year, "year");
        Bind(month, "month");
        Bind(day, "day");


        Bind(output, "OUTPUT");
    }



    void Tick()
    {
        float time = GetTime();

        double direct_normal_irradiance;
        double diffuse_sky_radiation;

        calculateIrradianceAndDiffuse(latitude, longitude, year, month, day, time, direct_normal_irradiance, diffuse_sky_radiation);

        output[0] = diffuse_sky_radiation;
    }
};

INSTALL_CLASS(AmbientLight)

