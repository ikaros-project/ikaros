#include "ikaros.h"

#include <type_traits>
#include <utility>


using namespace ikaros;

namespace
{
template <typename T, typename = void>
struct exposes_component_scheduler_state: std::false_type
{};

template <typename T>
struct exposes_component_scheduler_state<T, std::void_t<
    decltype(std::declval<T &>().async_mode),
    decltype(std::declval<T &>().async_future),
    decltype(std::declval<T &>().profiler_),
    decltype(std::declval<T &>().parent_)>>: std::true_type
{};

static_assert(!exposes_component_scheduler_state<Component>::value,
              "Component scheduler and identity state must not be public");

template <typename T, typename = void>
struct exposes_class_registry_state: std::false_type
{};

template <typename T>
struct exposes_class_registry_state<T, std::void_t<
    decltype(std::declval<T &>().info_),
    decltype(std::declval<T &>().module_creator),
    decltype(std::declval<T &>().name),
    decltype(std::declval<T &>().path)>>: std::true_type
{};

static_assert(!exposes_class_registry_state<Class>::value,
              "Class registry state must not be public");
}

class PersistentStateTestModule: public Module
{
    matrix data;
    parameter write;
    parameter use_private;
    parameter use_scalar;
    matrix memory;
    matrix output;
    float bias;
    double gain;
    int steps;
    bool active;
    std::string label;

    void Init()
    {
        Bind(data, "data");
        Bind(write, "write");
        Bind(use_private, "use_private");
        Bind(use_scalar, "use_scalar");
        Bind(memory, "MEMORY");
        Bind(output, "STATE");
        Bind(bias, "bias");
        Bind(gain, "gain");
        Bind(steps, "steps");
        Bind(active, "active");
        Bind(label, "label");
    }

    void Tick()
    {
        if(use_scalar.as_bool())
        {
            if(write.as_bool())
            {
                bias = 0.25f;
                gain = 1.5;
                steps = 42;
                active = false;
                label = "trained";
            }
            else
            {
                std::cout << "SCALAR_STATE: " << bias << " " << gain << " " << steps << " " << active << " " << label << std::endl;
            }
            return;
        }

        matrix & state = use_private.as_bool() ? memory : output;
        if(write.as_bool())
            state.copy(data);
        else
            state.print("STATE");
    }
};


class InvalidIntStateDefaultModule : public Module
{
};


class InvalidBoolStateDefaultModule : public Module
{
};


class ComponentHardeningTestModule : public Module
{
    int firstBinding = 0;
    int secondBinding = 0;

    void Init() override
    {
        if(KeyExists("integer_attribute"))
            GetIntValue("integer_attribute");

        if(KeyExists("class_scan_path"))
            kernel().ScanClasses(GetValue("class_scan_path"));

        Bind(firstBinding, "value");
        if(KeyExists("bind_state_twice") && ComputeBool(GetValue("bind_state_twice")))
            Bind(secondBinding, "value");
    }
};


INSTALL_CLASS(PersistentStateTestModule)
INSTALL_CLASS(InvalidIntStateDefaultModule)
INSTALL_CLASS(InvalidBoolStateDefaultModule)
INSTALL_CLASS(ComponentHardeningTestModule)
