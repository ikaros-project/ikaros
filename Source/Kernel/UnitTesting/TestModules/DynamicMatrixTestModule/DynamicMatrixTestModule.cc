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

INSTALL_CLASS(DynamicMatrixSource)
INSTALL_CLASS(DynamicMatrixSink)
INSTALL_CLASS(DynamicDelayedMatrixSink)
