// Ikaros 3.0

#include "ikaros.h"

namespace ikaros
{
    tick_count Module::GetTick() const        { return kernel().GetTick(); }
    double Module::GetTickDuration() const    { return kernel().GetTickDuration(); } // Time for each tick in seconds (s)
    double Module::GetTime() const            { return kernel().GetTime(); }
    double Module::GetRealTime() const        { return kernel().GetRealTime(); }
    double Module::GetNominalTime() const     { return kernel().GetNominalTime(); }
    double Module::GetRunTime() const         { return kernel().GetRunTime(); }
    double Module::GetTimeOfDay() const       { return kernel().GetTimeOfDay(); }
    double Module::GetLag() const             { return kernel().GetLag(); }
    double Module::GetUptime() const          { return kernel().GetUptime(); }
    double Module::GetActualTickDuration() const { return kernel().GetActualTickDuration(); }
    double Module::GetTickTimeUsage() const   { return kernel().GetTickTimeUsage(); }
    double Module::GetCPUUsage() const        { return kernel().GetCPUUsage(); }
    double Module::GetIdleTime() const        { return kernel().GetIdleTime(); }
    int Module::GetRunMode() const            { return kernel().GetRunMode(); }
    int Module::GetCPUCoreCount() const       { return kernel().GetCPUCoreCount(); }
    int Module::GetModuleCount() const        { return kernel().GetModuleCount(); }
    int Module::GetClassCount() const         { return kernel().GetClassCount(); }
    tick_count Module::GetStopAfter() const   { return kernel().GetStopAfter(); }


    Module::Module()
    {

    }

    bool
    Module::TryProfilingBegin()
    {
        if(!kernel().ProfilingEnabled())
            return false;

        ProfilingBegin();
        return true;
    }

    void
    Module::ProfilingBegin()
    {
        profiler_.begin();
    }

    void
    Module::ProfilingEnd()
    {
        profiler_.end();
    }

    INSTALL_CLASS(Module)

// ****************************** MODULE Sizes ******************************

    int 
    Module::SetOutputShape(dictionary d, input_map)
    {
        try
        {
            if(d.contains_non_null("alias"))
                return 0;

            bool dynamic_output = d.is_set("dynamic");
            if(dynamic_output && !d.contains("capacity"))
                throw setup_failed("Dynamic output \""+std::string(d.at("name")) +"\" must have a capacity attribute.", path_);

            std::string shape_expr;
            if(dynamic_output)
                shape_expr = std::string(d.at("capacity"));
            else if(d.contains("size"))
                shape_expr = std::string(d.at("size"));
            else if(d.contains("shape"))
                shape_expr = std::string(d.at("shape"));
            else if(info_.contains("size"))
                shape_expr = std::string(info_.at("size"));
            else
                throw setup_failed("Output \""+std::string(d.at("name")) +"\" must have a value for \"size\" or \"shape\".", path_);
            
            if(shape_expr.empty())
                throw setup_failed("Output \""+std::string(d.at("name")) +"\" must have a value for \"size\" or \"shape\".", path_);
            std::vector<int> shape = EvaluateShapeList(shape_expr);
            matrix o;
            Bind(o, d.at("name"));
            if(dynamic_output)
            {
                if(shape.empty())
                    return 0;
                o.reserve(shape);
                o.set_dynamic().set_fixed_capacity();
            }
            else
                o.realloc(shape);
            return 0;
        }
        catch(const std::invalid_argument & e)
        {
            throw setup_failed("Size expression for output \""+std::string(d.at("name")) +"\" is invalid. "+e.what(), path_);
        }
        catch(const std::exception & e)
        {
            throw setup_failed("Size expression for output \""+std::string(d.at("name")) +"\" is invalid. "+std::string(e.what()), path_);
        }
        catch(...)
        {
            throw setup_failed("Size expression for output \""+std::string(d.at("name")) +"\" is invalid.", path_);
        }
    }


    int 
    Module::SetOutputShapes(input_map ingoing_connections)
    {
        if(!InputsReady(info_, ingoing_connections))
            return 0; // Cannot set size yet

        for(auto & d : info_["outputs"])
            SetOutputShape(d, ingoing_connections);
        ApplyOutputAliases();

        return 0;
    }


    int
    Module::SetStateShapes(input_map ingoing_connections)
    {
        if(!InputsReady(info_, ingoing_connections))
            return 0;

        for(auto & d : info_["states"])
            SetStateShape(d);

        return 0;
    }


    int 
    Module::SetSizes(input_map ingoing_connections)
    {
        SetInputSizes(ingoing_connections);
        SetOutputShapes(ingoing_connections);
        SetStateShapes(ingoing_connections);
        return 0;
    }   


}
