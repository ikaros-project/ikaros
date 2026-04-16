// compute_engine.h

#pragma once

#include "ikaros.h"

namespace ikaros
{

class ComputeEngine
{
public:
    explicit ComputeEngine(Component & component);

    std::string ComputeValue(const std::string & s);
    double ComputeDouble(const std::string & s);
    int ComputeInt(const std::string & s);
    bool ComputeBool(const std::string & s);

    static std::vector<std::string> SplitTopLevel(const std::string & s, char separator);

private:
    struct LookupResult
    {
        enum class Source
        {
            none,
            local_attribute,
            resolved_parameter,
            inherited_value,
        };

        Source source = Source::none;
        std::string value;

        bool found() const
        {
            return source != Source::none;
        }
    };

    struct ParsedPath
    {
        bool absolute = false;
        std::vector<std::string> segments;
    };

    struct EvalContext
    {
        std::unordered_map<std::string, bool> explicit_syntax_cache;
        std::unordered_map<std::string, bool> path_like_cache;
        std::unordered_map<std::string, bool> top_level_math_cache;
        std::unordered_map<std::string, bool> number_cache;
        std::unordered_map<std::string, std::vector<std::string>> comma_split_cache;
        std::unordered_map<std::string, std::vector<std::string>> semicolon_split_cache;
        std::unordered_map<std::string, std::vector<std::string>> dot_split_cache;
        std::unordered_map<std::string, ParsedPath> parsed_path_cache;
        std::unordered_map<std::string, LookupResult> lookup_cache;
    };

    Component & component_;

    std::string EvalMatrix(EvalContext & context, const std::string & s, int depth=0);
    std::string EvalList(EvalContext & context, const std::string & s, int depth=0);
    std::string EvalScalar(EvalContext & context, const std::string & s, int depth=0, bool evaluate_final=false);
    std::string ExpandCurly(EvalContext & context, const std::string & s, int depth=0);
    std::string EvalPath(EvalContext & context, const std::string & s, int depth=0, bool evaluate_final=false);
    std::string EvalFinalSegment(EvalContext & context, const std::string & s, int depth=0, bool evaluate_final=false);
    std::string EvalFunction(EvalContext & context, const std::string & s, int depth=0);
    std::string EvalMath(EvalContext & context, const std::string & s, int depth=0);
    std::string ExpandSegment(EvalContext & context, const std::string & s, int depth=0);
    LookupResult LookupLocal(EvalContext & context, const std::string & name) const;
    std::optional<matrix> LookupMatrixValue(const std::string & name) const;
    std::string MatrixSizeFunctionValue(const matrix & m, const std::string & function_name) const;

    bool HasExplicitSyntax(EvalContext & context, const std::string & s) const;
    bool IsPathLike(EvalContext & context, const std::string & s) const;
    bool ShouldReturnLiteral(EvalContext & context, const std::string & s, bool evaluate_final) const;
    bool HasTopLevelMath(EvalContext & context, const std::string & s) const;
    bool IsFunction(const std::string & s) const;
    bool LooksLikeNumber(EvalContext & context, const std::string & s) const;
    const std::vector<std::string> & SplitTopLevel(EvalContext & context, const std::string & s, char separator) const;
    const ParsedPath & ParsePath(EvalContext & context, const std::string & s) const;
    Component * ResolvePathComponent(Component * current, const std::string & next_component) const;
    void CheckDepth(int depth) const;
};

}
