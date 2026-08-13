#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>

#include "ikaros.h"


using namespace ikaros;


class OutputImageRecoveryFixture : public Module
{
    matrix write;
    parameter filename;
    std::filesystem::path obstructionPath;
    int tickCount = 0;

public:
    void
    Init() override
    {
        Bind(write, "WRITE");
        Bind(filename, "filename");

        if(!kernel().SanitizeWritePath(
               filename.as_string(), obstructionPath))
            throw std::invalid_argument(
                "OutputImageRecoveryFixture filename must be inside UserData");

        std::error_code error;
        std::filesystem::remove(obstructionPath, error);
        if(error)
            throw std::runtime_error(
                "Could not clear OutputImage recovery fixture: " +
                error.message());
        if(!std::filesystem::create_directory(obstructionPath, error))
            throw std::runtime_error(
                "Could not create OutputImage recovery fixture: " +
                error.message());
        write(0) = 1;
    }


    void
    Tick() override
    {
        if(tickCount == 2)
        {
            std::error_code error;
            if(!std::filesystem::remove(obstructionPath, error) || error)
                throw std::runtime_error(
                    "Could not remove OutputImage recovery fixture: " +
                    error.message());
            write(0) = 0;
        }
        else
            write(0) = 1;
        ++tickCount;
    }


    void
    Stop() override
    {
        std::error_code error;
        if(std::filesystem::is_directory(obstructionPath, error))
            std::filesystem::remove(obstructionPath, error);
    }
};


INSTALL_CLASS(OutputImageRecoveryFixture)
