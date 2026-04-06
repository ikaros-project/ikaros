// compute_engine.cc

#include "compute_engine.h"

namespace ikaros
{

ComputeEngine::ComputeEngine(Component & component):
    component_(component)
{}


std::string
ComputeEngine::ComputeValue(const std::string & s)
{
    EvalContext context;
    return EvalMatrix(context, s, 0);
}


double
ComputeEngine::ComputeDouble(const std::string & s)
{
    EvalContext context;
    std::string value = trim(EvalMatrix(context, s, 0));
    if(!LooksLikeNumber(context, value))
        throw exception("ComputeDouble could not convert \""+value+"\" to number.", component_.path_);
    return std::stod(value);
}


int
ComputeEngine::ComputeInt(const std::string & s)
{
    return static_cast<int>(ComputeDouble(s));
}


bool
ComputeEngine::ComputeBool(const std::string & s)
{
    EvalContext context;
    std::string value = trim(EvalMatrix(context, s, 0));
    static std::vector<std::string> false_list = {"false", "False", "no", "NO", "off", "OFF", "0"};

    if(is_true(value))
        return true;
    if(std::find(false_list.begin(), false_list.end(), value) != false_list.end())
        return false;

    throw exception("ComputeBool could not convert \""+value+"\" to bool.", component_.path_);
}


std::string
ComputeEngine::EvalMatrix(EvalContext & context, const std::string & s, int depth)
{
    CheckDepth(depth);

    const auto & rows = SplitTopLevel(context, s, ';');
    if(rows.size() <= 1)
        return EvalList(context, trim(s), depth+1);

    std::vector<std::string> computed_rows;
    for(const auto & row : rows)
    {
        if(row.empty())
            continue;
        computed_rows.push_back(EvalList(context, row, depth+1));
    }

    return join(";", computed_rows, false);
}


std::vector<std::string>
ComputeEngine::SplitTopLevel(const std::string & s, char separator)
{
    std::vector<std::string> items;
    std::string current;
    int paren_depth = 0;
    int brace_depth = 0;

    for(char c : s)
    {
        if(c == '(')
            paren_depth++;
        else if(c == ')')
            paren_depth--;
        else if(c == '{')
            brace_depth++;
        else if(c == '}')
            brace_depth--;

        if(c == separator && paren_depth == 0 && brace_depth == 0)
        {
            items.push_back(trim(current));
            current.clear();
        }
        else
            current.push_back(c);
    }

    items.push_back(trim(current));
    return items;
}


const std::vector<std::string> &
ComputeEngine::SplitTopLevel(EvalContext & context, const std::string & s, char separator) const
{
    auto * cache = &context.dot_split_cache;
    if(separator == ',')
        cache = &context.comma_split_cache;
    else if(separator == ';')
        cache = &context.semicolon_split_cache;

    auto it = cache->find(s);
    if(it == cache->end())
        it = cache->emplace(s, SplitTopLevel(s, separator)).first;
    return it->second;
}


const ComputeEngine::ParsedPath &
ComputeEngine::ParsePath(EvalContext & context, const std::string & s) const
{
    auto it = context.parsed_path_cache.find(s);
    if(it != context.parsed_path_cache.end())
        return it->second;

    ParsedPath parsed;
    std::string path = trim(s);
    parsed.absolute = !path.empty() && path[0] == '.';
    if(parsed.absolute)
        path = path.substr(1);

    for(const auto & part : SplitTopLevel(context, path, '.'))
        if(!part.empty())
            parsed.segments.push_back(part);

    return context.parsed_path_cache.emplace(s, std::move(parsed)).first->second;
}


Component *
ComputeEngine::ResolvePathComponent(Component * current, const std::string & next_component) const
{
    Component * ancestor = current;
    while(ancestor)
    {
        if(std::string(ancestor->info_["name"]) == next_component)
            return ancestor;
        ancestor = ancestor->parent_;
    }

    return current->GetComponent(next_component);
}


void
ComputeEngine::CheckDepth(int depth) const
{
    if(depth > 64)
        throw exception("Maximum compute recursion depth exceeded.", component_.path_);
}


bool
ComputeEngine::LooksLikeNumber(EvalContext & context, const std::string & s) const
{
    auto it = context.number_cache.find(s);
    if(it == context.number_cache.end())
        it = context.number_cache.emplace(s, is_number(trim(s))).first;
    return it->second;
}


bool
ComputeEngine::HasExplicitSyntax(EvalContext & context, const std::string & s) const
{
    auto it = context.explicit_syntax_cache.find(s);
    if(it == context.explicit_syntax_cache.end())
        it = context.explicit_syntax_cache.emplace(s, s.find('@') != std::string::npos || s.find('{') != std::string::npos).first;
    return it->second;
}


bool
ComputeEngine::IsPathLike(EvalContext & context, const std::string & s) const
{
    auto it = context.path_like_cache.find(s);
    if(it != context.path_like_cache.end())
        return it->second;

    bool is_path_like = false;
    if(!s.empty())
    {
        if(s[0] == '.')
            is_path_like = true;
        else
            is_path_like = SplitTopLevel(context, s, '.').size() > 1 || IsFunction(s);
    }

    return context.path_like_cache.emplace(s, is_path_like).first->second;
}


bool
ComputeEngine::ShouldReturnLiteral(EvalContext & context, const std::string & s, bool evaluate_final) const
{
    if(evaluate_final)
        return false;

    if(HasTopLevelMath(context, s))
        return false;

    if(IsPathLike(context, s))
        return false;

    if(HasExplicitSyntax(context, s))
        return false;

    return std::any_of(s.begin(), s.end(), [](unsigned char c)
    {
        return std::isalpha(c);
    });
}


bool
ComputeEngine::IsFunction(const std::string & s) const
{
    return ends_with(s, ".size_x") || ends_with(s, ".size_y") || ends_with(s, ".size_z")
        || ends_with(s, ".rows") || ends_with(s, ".cols");
}


bool
ComputeEngine::HasTopLevelMath(EvalContext & context, const std::string & s) const
{
    auto cached = context.top_level_math_cache.find(s);
    if(cached != context.top_level_math_cache.end())
        return cached->second;

    int paren_depth = 0;
    int brace_depth = 0;
    bool has_top_level_math = false;

    for(size_t i = 0; i < s.size(); i++)
    {
        char c = s[i];
        if(c == '(')
            paren_depth++;
        else if(c == ')')
            paren_depth--;
        else if(c == '{')
            brace_depth++;
        else if(c == '}')
            brace_depth--;

        if(paren_depth != 0 || brace_depth != 0)
            continue;

        if(c == '+' || c == '*' || c == '/')
        {
            has_top_level_math = true;
            break;
        }

        if(c == '-')
        {
            if(i == 0)
            {
                has_top_level_math = true;
                break;
            }
            char prev = s[i-1];
            if(prev != '.' && prev != '_' && !std::isalnum(static_cast<unsigned char>(prev)) && prev != '@' && prev != '}')
            {
                has_top_level_math = true;
                break;
            }
        }
    }

    return context.top_level_math_cache.emplace(s, has_top_level_math).first->second;
}


ComputeEngine::LookupResult
ComputeEngine::LookupLocal(EvalContext & context, const std::string & name) const
{
    if(name.empty())
        return {};

    auto cached = context.lookup_cache.find(name);
    if(cached != context.lookup_cache.end())
        return cached->second;

    if(component_.info_.contains(name))
        return context.lookup_cache.emplace(name, LookupResult{LookupResult::Source::local_attribute, std::string(component_.info_[name])}).first->second;

    if(kernel().parameters.count(component_.path_+'.'+name) && kernel().parameters.at(component_.path_+'.'+name).resolved)
    {
        std::string value = kernel().parameters.at(component_.path_+'.'+name).as_string();
        if(!value.empty())
            return context.lookup_cache.emplace(name, LookupResult{LookupResult::Source::resolved_parameter, value}).first->second;
    }

    if(const Component * owner = component_.GetValueOwner(name))
    {
        if(owner == &component_)
            return context.lookup_cache.emplace(name, LookupResult{LookupResult::Source::local_attribute, std::string(component_.info_[name])}).first->second;

        std::string inherited_value = std::string(owner->info_[name]);
        ComputeEngine owner_engine(*const_cast<Component *>(owner));
        EvalContext owner_context;

        bool has_explicit_syntax = owner_engine.HasExplicitSyntax(owner_context, inherited_value);
        bool is_path_like = owner_engine.IsPathLike(owner_context, inherited_value);
        bool looks_like_number = owner_engine.LooksLikeNumber(owner_context, inherited_value);
        bool has_top_level_math = owner_engine.HasTopLevelMath(owner_context, inherited_value);
        bool has_alpha = std::any_of(inherited_value.begin(), inherited_value.end(), [](unsigned char c)
        {
            return std::isalpha(c);
        });

        if(has_explicit_syntax || is_path_like || looks_like_number || (!has_alpha && has_top_level_math))
            inherited_value = owner_engine.ComputeValue(inherited_value);

        return context.lookup_cache.emplace(name, LookupResult{
            LookupResult::Source::inherited_value,
            inherited_value
        }).first->second;
    }

    return context.lookup_cache.emplace(name, LookupResult{}).first->second;
}


std::string
ComputeEngine::EvalList(EvalContext & context, const std::string & s, int depth)
{
    CheckDepth(depth);

    const auto & items = SplitTopLevel(context, s, ',');
    if(items.size() <= 1)
        return EvalScalar(context, trim(s), depth+1);

    std::vector<std::string> values;
    for(const auto & item : items)
        values.push_back(EvalScalar(context, item, depth+1));

    return join(",", values, false);
}


std::string
ComputeEngine::ExpandCurly(EvalContext & context, const std::string & s, int depth)
{
    CheckDepth(depth);

    std::string out = s;
    while(true)
    {
        int start = -1;
        int end = -1;
        int brace_depth = 0;

        for(size_t i = 0; i < out.size(); i++)
        {
            if(out[i] == '{')
            {
                if(brace_depth == 0)
                    start = static_cast<int>(i);
                brace_depth++;
            }
            else if(out[i] == '}')
            {
                brace_depth--;
                if(brace_depth < 0)
                    throw exception("Unmatched closing brace in compute expression.", component_.path_);
                if(brace_depth == 0)
                {
                    end = static_cast<int>(i);
                    break;
                }
            }
        }

        if(start == -1 && end == -1)
            return out;
        if(start == -1 || end == -1)
            throw exception("Unmatched curly brace in compute expression.", component_.path_);

        std::string inner = out.substr(start+1, end-start-1);
        std::string value = EvalScalar(context, inner, depth+1, true);
        out = out.substr(0, start) + value + out.substr(end+1);
    }
}


std::string
ComputeEngine::ExpandSegment(EvalContext & context, const std::string & s, int depth)
{
    CheckDepth(depth);

    std::string segment = ExpandCurly(context, trim(s), depth+1);
    if(segment.empty())
        return segment;

    if(segment[0] == '@')
    {
        std::string operand = trim(segment.substr(1));
        bool operand_path_like = IsPathLike(context, operand);

        if(!operand_path_like && !HasTopLevelMath(context, operand) && !HasExplicitSyntax(context, operand))
            return EvalFinalSegment(context, operand, depth+1, true);

        return EvalScalar(context, operand, depth+1, true);
    }

    if(HasTopLevelMath(context, segment))
        return EvalMath(context, segment, depth+1);

    return segment;
}


std::string
ComputeEngine::EvalFunction(EvalContext &, const std::string & s, int depth)
{
    CheckDepth(depth);

    std::string ss = s;

    if(ends_with(s, ".size_x"))
        return std::to_string(component_.GetBuffer(rhead(ss, ".")).size_x());

    if(ends_with(s, ".size_y"))
        return std::to_string(component_.GetBuffer(rhead(ss, ".")).size_y());

    if(ends_with(s, ".size_z"))
        return std::to_string(component_.GetBuffer(rhead(ss, ".")).size_z());

    if(ends_with(s, ".rows"))
        return std::to_string(component_.GetBuffer(rhead(ss, ".")).rows());

    if(ends_with(s, ".cols"))
        return std::to_string(component_.GetBuffer(rhead(ss, ".")).cols());

    throw exception("Unknown compute function \""+s+"\".", component_.path_);
}


std::string
ComputeEngine::EvalFinalSegment(EvalContext & context, const std::string & s, int depth, bool evaluate_final)
{
    CheckDepth(depth);

    std::string original = trim(s);
    if(original.empty())
        return "";

    bool explicit_evaluation = HasExplicitSyntax(context, original);

    std::string segment = ExpandSegment(context, original, depth+1);
    if(IsFunction(segment))
        return EvalFunction(context, segment, depth+1);

    if(!explicit_evaluation && !evaluate_final)
        return segment;

    LookupResult lookup = LookupLocal(context, segment);
    if(!lookup.found())
    {
        if(evaluate_final)
            return "";
        if(LooksLikeNumber(context, segment))
            return formatNumber(std::stod(segment));
        return segment;
    }

    const std::string & value = lookup.value;
    bool has_explicit_syntax = HasExplicitSyntax(context, value);

    bool has_alpha = std::any_of(value.begin(), value.end(), [](unsigned char c)
    {
        return std::isalpha(c);
    });

    if(has_explicit_syntax || (!has_alpha && (LooksLikeNumber(context, value) || HasTopLevelMath(context, value))))
        return EvalScalar(context, value, depth+1);

    return value;
}


std::string
ComputeEngine::EvalPath(EvalContext & context, const std::string & s, int depth, bool evaluate_final)
{
    CheckDepth(depth);

    const ParsedPath & parsed = ParsePath(context, s);
    std::vector<std::string> segments = parsed.segments;
    if(segments.empty())
        return "";

    Component * current = &component_;
    if(parsed.absolute)
    {
        std::string root_path = peek_head(component_.path_, ".");
        current = kernel().components.at(root_path).get();
    }

    while(true)
    {
        ComputeEngine current_engine(*current);
        EvalContext current_context;
        current_engine.CheckDepth(depth);

        if(segments.size() == 1)
            return current_engine.EvalFinalSegment(current_context, segments[0], depth+1, evaluate_final);

        if(segments.size() >= 2)
        {
            std::string function_candidate = segments[0] + "." + segments[1];
            if(current_engine.IsFunction(function_candidate))
                return current_engine.EvalFunction(current_context, function_candidate, depth+1);
        }

        std::string expanded = current_engine.ExpandSegment(current_context, segments[0], depth+1);
        const auto & extra = current_engine.SplitTopLevel(current_context, expanded, '.');
        std::vector<std::string> rewritten;
        for(const auto & piece : extra)
            if(!piece.empty())
                rewritten.push_back(piece);
        rewritten.insert(rewritten.end(), segments.begin()+1, segments.end());
        segments = rewritten;

        if(segments.empty())
            return "";

        if(segments.size() == 1)
            return current_engine.EvalFinalSegment(current_context, segments[0], depth+1, evaluate_final);

        current = ResolvePathComponent(current, segments[0]);

        segments.erase(segments.begin());
    }
}


std::string
ComputeEngine::EvalMath(EvalContext & context, const std::string & s, int depth)
{
    CheckDepth(depth);

    expression e(s);
    std::map<std::string, std::string> vars;
    for(const auto & v : e.variables())
    {
        std::string value;
        if(!v.empty() && v[0] == '@')
            value = ExpandSegment(context, v, depth+1);
        else if(IsPathLike(context, v))
            value = EvalScalar(context, v, depth+1, true);
        else
            throw exception("Variables in compute expressions must use @ indirection: \""+v+"\".", component_.path_);

        if(value.empty())
            throw exception("Variable \""+v+"\" not defined.", component_.path_);
        if(!LooksLikeNumber(context, value))
            throw exception("Variable \""+v+"\" resolved to non-numeric value \""+value+"\".", component_.path_);
        vars[v] = value;
    }

    return formatNumber(e.evaluate(vars));
}


std::string
ComputeEngine::EvalScalar(EvalContext & context, const std::string & s, int depth, bool evaluate_final)
{
    CheckDepth(depth);

    std::string current = trim(s);
    if(current.empty())
        return "";

    for(int i = 0; i < 64; i++)
    {
        std::string previous = current;

        if(LooksLikeNumber(context, current))
            return formatNumber(std::stod(current));

        if(ShouldReturnLiteral(context, current, evaluate_final))
            return current;

        if(!IsPathLike(context, current))
            current = ExpandCurly(context, current, depth+1);

        if(HasTopLevelMath(context, current))
            current = EvalMath(context, current, depth+1);
        else
            current = EvalPath(context, current, depth+1, evaluate_final);

        current = trim(current);

        // After explicit expansion or a successful final lookup, preserve a newly
        // resolved literal string rather than re-evaluating punctuation inside it
        // as math or another lookup. This keeps values such as "abc/def:ghi"
        // returned from "@name" intact for string parameters.
        if(current != previous &&
           !HasExplicitSyntax(context, current) &&
           !IsPathLike(context, current) &&
           std::any_of(current.begin(), current.end(), [](unsigned char c) { return std::isalpha(c); }))
            return current;

        if(current == previous)
            return current;
    }

    throw exception("Compute expression did not converge.", component_.path_);
}

}
