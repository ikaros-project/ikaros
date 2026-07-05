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

INSTALL_CLASS(DynamicMatrixSource)
INSTALL_CLASS(DynamicMatrixSink)
