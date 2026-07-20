#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "ikaros.h"

using namespace ikaros;

namespace
{
void
require_true(bool condition, const std::string & message)
{
    if(!condition)
        throw exception("ComputeEngineTestModule: " + message);
}


void
require_throws(const std::function<void()> & function, const std::string & message)
{
    try
    {
        function();
    }
    catch(const std::exception &)
    {
        return;
    }

    throw exception("ComputeEngineTestModule: " + message + " (expected exception)");
}
}


class ComputeEngineTestModule: public Module
{
    void Init() override
    {
        Component & root = *Parent();
        Component * child = root.GetComponent("child");
        Component * settings = root.GetComponent("Epi.Settings");

        require_true(root.ComputeValue("value") == "value",
                     "plain names should remain literal without explicit evaluation");
        require_true(root.ComputeValue("child.value") == "value",
                     "plain final path attributes should remain literal");
        require_true(root.ComputeValue("child.@value") == "7",
                     "relative path indirection should resolve values");
        require_true(root.ComputeValue("@target.@value") == "7",
                     "computed component paths should resolve values");
        require_true(root.ComputeValue("outer.@which.@value") == "9",
                     "nested computed component paths should resolve values");
        require_true(root.ComputeValue("{target}.@value") == "7",
                     "curly expansion should construct component paths");
        require_true(root.ComputeValue("{base}{i}.@value") == "11",
                     "adjacent curly expansions should compose path segments");
        require_true(child->ComputeValue("{expr}") == "7",
                     "curly expansion should evaluate arithmetic attributes");

        require_true(root.ComputeValue("MatrixSource.OUTPUT.rows") == "2",
                     "rows should report the matrix row count");
        require_true(root.ComputeValue("MatrixSource.OUTPUT.rows*2") == "4",
                     "matrix functions should participate in arithmetic");
        require_true(root.ComputeValue("MatrixSource.OUTPUT.shape") == "2,3",
                     "shape should report every matrix dimension");
        require_true(root.ComputeValue("MatrixSource.OUTPUT.rank") == "2",
                     "rank should report the number of dimensions");
        require_true(root.ComputeValue("MatrixSource.OUTPUT.shape[0]") == "2",
                     "shape indexing should return one dimension");
        require_true(root.ComputeValue("MatrixSource.OUTPUT.shape[1:]") == "3",
                     "shape slicing should return the selected dimensions");
        require_true(root.ComputeValue("MatrixSource.OUTPUT.shape[0]*2") == "4",
                     "shape indexing should participate in arithmetic");
        require_true(root.ComputeValue("MatrixSource.OUTPUT.rank+1") == "3",
                     "rank should participate in arithmetic");
        require_true(root.ComputeValue("MatrixSource.OUTPUT.size[0]") == "2" &&
                     root.ComputeValue("MatrixSource.OUTPUT.size[1:]") == "3",
                     "size selectors should match shape selectors");

        require_true(ComputeValue("@padding") == "same",
                     "numeric options should retain their label for general evaluation");
        require_true(root.ComputeValue("child.@value,@target.@value,MatrixSource.OUTPUT.rows*2") == "7,7,4",
                     "list expressions should evaluate each item");
        require_true(root.ComputeValue("child.@value,@target.@value;MatrixSource.OUTPUT.rows*2,{child.expr};") ==
                     "7,7;4,7", "matrix expressions should evaluate rows and retain trailing separators");

        require_true(settings->ComputeValue("type") == "type",
                     "plain inherited names should remain literal");
        require_true(settings->ComputeValue("{type}") == "FullX",
                     "curly expansion should evaluate inherited path expressions");
        require_true(settings->ComputeValue("FullX.{Body_L1_T1_data}") == "88",
                     "curly expansion should resolve values in a child component");
        require_true(root.ComputeValue(".Epi.Settings.type") == "type",
                     "absolute paths should preserve a plain final attribute");
        require_true(root.ComputeValue(".Epi.Settings.{type}") == "FullX",
                     "absolute paths should evaluate an explicit final attribute");
        require_true(root.ComputeValue(".Epi.Settings.{type}.{Body_L1_T1_data}") == "88",
                     "absolute paths should support multiple computed segments");
        require_true(root.ComputeValueOf("agent") == "EpiBlue",
                     "ComputeValueOf should resolve an expression-valued attribute");
        require_true(root.ComputeValueOf("literal_agent") == "X",
                     "ComputeValueOf should resolve a literal attribute");
        require_true(root.ComputeValue("@i+1") == "2",
                     "indirected parameters should participate in arithmetic");
        require_throws([&]() { static_cast<void>(root.ComputeValue("i+1")); },
                       "bare arithmetic variables should be rejected");

        std::string firstDimensionSource = "MatrixSource.OUTPUT.shape[0]";
        require_true(root.EvaluateShapeList(firstDimensionSource) == std::vector<int>({2}),
                     "shape lists should accept indexed matrix dimensions");
        std::string tailDimensionsSource = "MatrixSource.OUTPUT.shape[1:]";
        require_true(root.EvaluateShapeList(tailDimensionsSource) == std::vector<int>({3}),
                     "shape lists should accept sliced matrix dimensions");
        std::string rankSource = "MatrixSource.OUTPUT.rank";
        require_true(root.EvaluateShapeList(rankSource) == std::vector<int>({2}),
                     "shape lists should accept matrix ranks");

        std::string numericOptionSource = ".MatrixSource.OUTPUT.rows+@padding";
        require_true(EvaluateShapeList(numericOptionSource) == std::vector<int>({3}),
                     "shape arithmetic should use numeric option indices");
        std::string optionalMissingSource =
            "optional(MatrixSource.OUTPUT.size_z),MatrixSource.OUTPUT.rows,MatrixSource.OUTPUT.cols";
        require_true(root.EvaluateShapeList(optionalMissingSource) == std::vector<int>({2, 3}),
                     "optional zero dimensions should be omitted");
        std::string optionalPresentSource =
            "optional(MatrixSource.OUTPUT.rows),MatrixSource.OUTPUT.cols";
        require_true(root.EvaluateShapeList(optionalPresentSource) == std::vector<int>({2, 3}),
                     "present optional dimensions should be retained");
        std::string requiredZeroSource =
            "MatrixSource.OUTPUT.size_z,MatrixSource.OUTPUT.rows,MatrixSource.OUTPUT.cols";
        require_true(root.EvaluateShapeList(requiredZeroSource).empty(),
                     "required zero dimensions should make the shape unresolved");

        require_true(root.xml().find("shape_expr=\"MatrixSource.OUTPUT.shape[1:]\"") != std::string::npos,
                     "component XML should preserve shape expressions");

        std::cout << "COMPUTE ENGINE TEST OK" << std::endl;
    }
};

INSTALL_CLASS(ComputeEngineTestModule)
