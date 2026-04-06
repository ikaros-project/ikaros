#include <cassert>
#include <functional>
#include <iostream>
#include <string>

#include "../../../ikaros.h"

using namespace ikaros;

namespace
{
    Group * make_group(const std::string & path, const std::string & name)
    {
        Kernel & k = kernel();
        dictionary info;
        info["name"] = name;
        info["groups"] = list();
        info["modules"] = list();
        info["inputs"] = list();
        info["outputs"] = list();
        info["parameters"] = list();

        k.current_component_info = info;
        k.current_component_path = path;

        auto g = std::make_unique<Group>();
        Group * raw = g.get();
        k.components[path] = std::move(g);
        return raw;
    }

    bool throws_compute(const std::function<void()> & fn)
    {
        try
        {
            fn();
            return false;
        }
        catch(const std::exception &)
        {
            return true;
        }
    }
}

int
main()
{
    Kernel & k = kernel();
    k.Clear();

    Group * root = make_group("root", "root");
    Group * child = make_group("root.child", "child");
    Group * child1 = make_group("root.child1", "child1");
    Group * outer = make_group("root.outer", "outer");
    Group * inner = make_group("root.outer.inner", "inner");
    Group * epi = make_group("root.Epi", "Epi");
    Group * epi_blue = make_group("root.Epi.EpiBlue", "EpiBlue");
    Group * settings = make_group("root.Epi.Settings", "Settings");
    Group * fullx = make_group("root.Epi.Settings.FullX", "FullX");

    root->info_["target"] = "child";
    root->info_["base"] = "child";
    root->info_["i"] = "1";
    root->info_["EpiName"] = "EpiBlue";

    child->info_["value"] = "7";
    child->info_["expr"] = "1+2*3";
    child1->info_["value"] = "11";

    outer->info_["which"] = "inner";
    inner->info_["value"] = "9";
    epi_blue->info_["robotType"] = "FullX";
    settings->info_["type"] = ".Epi.@EpiName.@robotType";
    fullx->info_["Body_L1_T1_data"] = "88";

    k.buffers["root.child.input"] = matrix().set_name("input");
    k.buffers["root.child.input"].realloc(2, 3);

    assert(root->ComputeValue("value") == "value");
    assert(root->ComputeValue("child.value") == "value");
    assert(root->ComputeValue("child.@value") == "7");
    assert(root->ComputeValue("@target.@value") == "7");
    assert(root->ComputeValue("outer.@which.@value") == "9");
    assert(root->ComputeValue("{target}.@value") == "7");
    assert(root->ComputeValue("{base}{i}.@value") == "11");
    assert(child->ComputeValue("{expr}") == "7");
    assert(root->ComputeValue("child.input.rows") == "2");
    assert(root->ComputeValue("child.input.rows*2") == "4");
    assert(root->ComputeValue("child.@value,@target.@value,child.input.rows*2") == "7,7,4");
    assert(root->ComputeValue("child.@value,@target.@value;child.input.rows*2,{child.expr};") == "7,7;4,7");
    assert(settings->ComputeValue("type") == "type");
    assert(settings->ComputeValue("{type}") == "FullX");
    assert(settings->ComputeValue("FullX.{Body_L1_T1_data}") == "88");
    assert(root->ComputeValue(".Epi.Settings.type") == "type");
    assert(root->ComputeValue(".Epi.Settings.{type}") == "FullX");
    assert(root->ComputeValue(".Epi.Settings.{type}.{Body_L1_T1_data}") == "88");
    assert(root->ComputeValue("@i+1") == "2");
    assert(throws_compute([&](){ root->ComputeValue("i+1"); }));

    std::cout << "test_compute passed\n";
    return 0;
}
