#include "ikaros.h"

using namespace ikaros;

class DynamicMatrixSource : public Module
{
    matrix rows_;

    void Init() override
    {
        Bind(rows_, "ROWS");
    }

    void Tick() override
    {
        rows_.clear();

        int row_count = static_cast<int>(GetTick()) + 1;
        for(int i = 0; i < row_count; ++i)
        {
            matrix row(2);
            row(0) = static_cast<float>(i);
            row(1) = static_cast<float>(GetTick());
            rows_.append(row);
        }
    }

    void Command(std::string command_name, dictionary & parameters) override
    {
        if(command_name == "fail_next_rotation")
        {
#ifndef NDEBUG
            matrix::set_allocation_failure_countdown_for_testing(0);
            std::cout << path_ << " DYNAMIC ROTATION FAILURE ARMED" << std::endl;
#else
            throw exception("Dynamic rotation failure injection requires a Debug build");
#endif
            return;
        }

        Module::Command(command_name, parameters);
    }
};


class DynamicMatrixSink : public Module
{
    matrix rows_;

    void Init() override
    {
        Bind(rows_, "ROWS");
    }

    void Tick() override
    {
        int expected_rows = static_cast<int>(GetTick()) + 1;
        if(rows_.rows() != expected_rows || rows_.cols() != 2)
            throw exception(
                "DynamicMatrixSink: expected shape " +
                std::to_string(expected_rows) + ",2 got " +
                std::to_string(rows_.rows()) + "," + std::to_string(rows_.cols())
            );

        for(int i = 0; i < expected_rows; ++i)
        {
            if(rows_(i, 0) != static_cast<float>(i) || rows_(i, 1) != static_cast<float>(GetTick()))
                throw exception("DynamicMatrixSink: unexpected dynamic matrix value");
        }

        if(GetTick() >= 2)
        {
            std::cout << "DYNAMIC MATRIX TEST OK" << std::endl;
            Notify(msg_terminate, "Dynamic matrix test complete.");
        }
    }
};


class DynamicDelayedMatrixSink : public Module
{
    matrix rows_;

    void Init() override
    {
        Bind(rows_, "ROWS");
    }

    void Tick() override
    {
        const int source_tick = static_cast<int>(GetTick()) - 2;
        const int expected_rows = source_tick < 1 ? 0 : source_tick + 1;
        if(rows_.rows() != expected_rows || rows_.cols() != 2)
            throw exception(
                "DynamicDelayedMatrixSink: expected shape " +
                std::to_string(expected_rows) + ",2 got " +
                std::to_string(rows_.rows()) + "," + std::to_string(rows_.cols())
            );

        for(int i = 0; i < expected_rows; ++i)
            if(rows_(i, 0) != static_cast<float>(i) ||
               rows_(i, 1) != static_cast<float>(source_tick))
                throw exception("DynamicDelayedMatrixSink: unexpected delayed matrix value");

        if(GetTick() >= 3)
        {
            std::cout << path_ << " DYNAMIC DELAYED MATRIX TEST OK" << std::endl;
            Notify(msg_terminate, "Dynamic delayed matrix test complete.");
        }
    }
};


class DynamicInputCapacityModule : public Module
{
    matrix input_;
    matrix rows_;

    void Init() override
    {
        Bind(input_, "X");
        Bind(rows_, "ROWS");

        if(rows_.capacity() != input_.shape())
            throw exception("DynamicInputCapacityModule: capacity did not resolve from the input shape");
        Notify(msg_warning, "DYNAMIC INPUT CAPACITY RESOLVED");
    }
};


class InvalidDynamicCapacityModule : public Module
{
};


class WholeOutputAliasTestModule : public Module
{
    matrix source_;
    matrix alias_;

    void Init() override
    {
        Bind(source_, "SOURCE");
        Bind(alias_, "ALIAS");

        if(source_.get_name("") != "SOURCE")
            throw exception("WholeOutputAliasTestModule: source metadata was renamed");
        source_(0) = 7;
        if(alias_(0) != 7)
            throw exception("WholeOutputAliasTestModule: alias did not share source storage");
        Notify(msg_warning, "WHOLE OUTPUT ALIAS METADATA PRESERVED");
    }
};


INSTALL_CLASS(DynamicMatrixSource)
INSTALL_CLASS(DynamicMatrixSink)
INSTALL_CLASS(DynamicDelayedMatrixSink)
INSTALL_CLASS(DynamicInputCapacityModule)
INSTALL_CLASS(InvalidDynamicCapacityModule)
INSTALL_CLASS(WholeOutputAliasTestModule)
