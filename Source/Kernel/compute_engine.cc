// compute_engine.cc

#include "compute_engine.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <utility>

namespace ikaros
{
namespace
{
bool LooksLikeMatrixLiteralValue(const std::string & value)
{
    const std::string trimmed = trim(value);
    if(trimmed.find(',') == std::string::npos &&
       trimmed.find(';') == std::string::npos &&
       (trimmed.empty() || trimmed.front() != '['))
        return false;

    try
    {
        matrix literal(trimmed);
        return true;
    }
    catch(...)
    {
        return false;
    }
}

bool ParseShapeSelector(const std::string & function_name, std::string & base_name, std::string & selector)
{
    const std::size_t bracket = function_name.find('[');
    if(bracket == std::string::npos || function_name.back() != ']')
        return false;

    base_name = function_name.substr(0, bracket);
    selector = function_name.substr(bracket + 1, function_name.size() - bracket - 2);
    return base_name == "shape" || base_name == "size";
}

bool ParseNonNegativeIndex(const std::string & text, std::size_t & value)
{
    const std::string trimmed = trim(text);
    if(trimmed.empty())
        return false;

    value = 0;
    for(char c : trimmed)
    {
        if(!std::isdigit(static_cast<unsigned char>(c)))
            return false;
        const std::size_t digit = static_cast<std::size_t>(c - '0');
        if(value > (std::numeric_limits<std::size_t>::max() - digit) / 10)
            return false;
        value = value * 10 + digit;
    }
    return true;
}


bool IsExpressionIdentifierStart(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == '@';
}


bool IsExpressionVariableStart(char c)
{
    return IsExpressionIdentifierStart(c) || c == '.';
}


struct ArithmeticNode
{
    char op = ' ';
    std::string value;
    std::unique_ptr<ArithmeticNode> left;
    std::unique_ptr<ArithmeticNode> right;
};


class ArithmeticParser
{
public:
    explicit ArithmeticParser(const std::string & source);
    std::unique_ptr<ArithmeticNode> parse();

private:
    const std::string & source_;
    std::size_t position_;

    bool at_end() const;
    char current() const;
    void skip_whitespace();
    bool consume(char token);
    [[noreturn]] void fail(const std::string & message) const;
    bool identifier_char(char c) const;
    std::unique_ptr<ArithmeticNode> make_terminal(const std::string & value) const;
    std::unique_ptr<ArithmeticNode> make_unary(char op, std::unique_ptr<ArithmeticNode> operand) const;
    std::unique_ptr<ArithmeticNode> make_binary(char op,
                                                std::unique_ptr<ArithmeticNode> left,
                                                std::unique_ptr<ArithmeticNode> right) const;
    std::unique_ptr<ArithmeticNode> parse_additive();
    std::unique_ptr<ArithmeticNode> parse_multiplicative();
    std::unique_ptr<ArithmeticNode> parse_unary();
    std::unique_ptr<ArithmeticNode> parse_primary();
    std::unique_ptr<ArithmeticNode> parse_number();
    std::unique_ptr<ArithmeticNode> parse_identifier();
};


ArithmeticParser::ArithmeticParser(const std::string & source):
    source_(source),
    position_(0)
{}


std::unique_ptr<ArithmeticNode>
ArithmeticParser::parse()
{
    skip_whitespace();
    if(at_end())
        fail("Expression cannot be empty");

    std::unique_ptr<ArithmeticNode> result = parse_additive();
    skip_whitespace();
    if(!at_end())
        fail("Unexpected token");
    return result;
}


bool
ArithmeticParser::at_end() const
{
    return position_ >= source_.size();
}


char
ArithmeticParser::current() const
{
    return at_end() ? '\0' : source_[position_];
}


void
ArithmeticParser::skip_whitespace()
{
    while(!at_end() && std::isspace(static_cast<unsigned char>(source_[position_])))
        ++position_;
}


bool
ArithmeticParser::consume(char token)
{
    skip_whitespace();
    if(current() != token)
        return false;
    ++position_;
    return true;
}


void
ArithmeticParser::fail(const std::string & message) const
{
    throw std::invalid_argument(message + " at position " + std::to_string(position_) + ".");
}


bool
ArithmeticParser::identifier_char(char c) const
{
    return IsExpressionIdentifierStart(c) || c == '.' || c == '[' || c == ']' || c == ':' ||
           (c >= '0' && c <= '9');
}


std::unique_ptr<ArithmeticNode>
ArithmeticParser::make_terminal(const std::string & value) const
{
    auto result = std::make_unique<ArithmeticNode>();
    result->value = value;
    return result;
}


std::unique_ptr<ArithmeticNode>
ArithmeticParser::make_unary(char op, std::unique_ptr<ArithmeticNode> operand) const
{
    auto result = std::make_unique<ArithmeticNode>();
    result->op = op;
    result->right = std::move(operand);
    return result;
}


std::unique_ptr<ArithmeticNode>
ArithmeticParser::make_binary(char op,
                              std::unique_ptr<ArithmeticNode> left,
                              std::unique_ptr<ArithmeticNode> right) const
{
    auto result = std::make_unique<ArithmeticNode>();
    result->op = op;
    result->left = std::move(left);
    result->right = std::move(right);
    return result;
}


std::unique_ptr<ArithmeticNode>
ArithmeticParser::parse_additive()
{
    std::unique_ptr<ArithmeticNode> result = parse_multiplicative();
    while(true)
    {
        if(consume('+'))
            result = make_binary('+', std::move(result), parse_multiplicative());
        else if(consume('-'))
            result = make_binary('-', std::move(result), parse_multiplicative());
        else
            return result;
    }
}


std::unique_ptr<ArithmeticNode>
ArithmeticParser::parse_multiplicative()
{
    std::unique_ptr<ArithmeticNode> result = parse_unary();
    while(true)
    {
        if(consume('*'))
            result = make_binary('*', std::move(result), parse_unary());
        else if(consume('/'))
            result = make_binary('/', std::move(result), parse_unary());
        else
            return result;
    }
}


std::unique_ptr<ArithmeticNode>
ArithmeticParser::parse_unary()
{
    if(consume('+'))
        return make_unary('+', parse_unary());
    if(consume('-'))
        return make_unary('-', parse_unary());
    return parse_primary();
}


std::unique_ptr<ArithmeticNode>
ArithmeticParser::parse_primary()
{
    skip_whitespace();
    if(at_end())
        fail("Missing operand");

    if(consume('('))
    {
        std::unique_ptr<ArithmeticNode> result = parse_additive();
        if(!consume(')'))
            fail("Missing closing parenthesis");
        return result;
    }

    const char c = current();
    if(std::isdigit(static_cast<unsigned char>(c)) ||
       (c == '.' && position_ + 1 < source_.size() &&
        std::isdigit(static_cast<unsigned char>(source_[position_ + 1]))))
        return parse_number();

    if(IsExpressionIdentifierStart(c) ||
       (c == '.' && position_ + 1 < source_.size() &&
        IsExpressionIdentifierStart(source_[position_ + 1])))
        return parse_identifier();

    fail("Expected a number, variable, or parenthesized expression");
}


std::unique_ptr<ArithmeticNode>
ArithmeticParser::parse_number()
{
    const std::size_t start = position_;
    bool digits = false;
    while(!at_end() && std::isdigit(static_cast<unsigned char>(current())))
    {
        digits = true;
        ++position_;
    }

    if(current() == '.')
    {
        ++position_;
        while(!at_end() && std::isdigit(static_cast<unsigned char>(current())))
        {
            digits = true;
            ++position_;
        }
    }

    if(!digits)
        fail("Invalid decimal number");

    if(current() == 'e' || current() == 'E')
    {
        ++position_;
        if(current() == '+' || current() == '-')
            ++position_;

        const std::size_t exponent_start = position_;
        while(!at_end() && std::isdigit(static_cast<unsigned char>(current())))
            ++position_;
        if(position_ == exponent_start)
            fail("Invalid exponent");
    }

    const std::string value = source_.substr(start, position_ - start);
    static_cast<void>(parse_double(value));
    return make_terminal(value);
}


std::unique_ptr<ArithmeticNode>
ArithmeticParser::parse_identifier()
{
    const std::size_t start = position_;
    if(current() == '.')
        ++position_;
    while(!at_end() && identifier_char(current()))
        ++position_;
    return make_terminal(source_.substr(start, position_ - start));
}


class ArithmeticExpression
{
public:
    using Values = std::map<std::string, std::string>;

    explicit ArithmeticExpression(const std::string & source);
    std::set<std::string> variables() const;
    double evaluate(const Values & values = {}) const;
    bool has_operators() const noexcept;
    std::string substitute(const Values & replacements) const;

private:
    std::unique_ptr<ArithmeticNode> root_;

    static void collect_variables(const ArithmeticNode * current, std::set<std::string> & result);
    static double evaluate_node(const ArithmeticNode * current, const Values & values);
    static std::string substitute_node(const ArithmeticNode * current, const Values & replacements);
};


ArithmeticExpression::ArithmeticExpression(const std::string & source):
    root_(ArithmeticParser(source).parse())
{}


std::set<std::string>
ArithmeticExpression::variables() const
{
    std::set<std::string> result;
    collect_variables(root_.get(), result);
    return result;
}


double
ArithmeticExpression::evaluate(const Values & values) const
{
    try
    {
        return evaluate_node(root_.get(), values);
    }
    catch(const std::exception & e)
    {
        throw std::invalid_argument(e.what());
    }
    catch(...)
    {
        throw std::invalid_argument("Invalid expression.");
    }
}


bool
ArithmeticExpression::has_operators() const noexcept
{
    return root_->op != ' ';
}


std::string
ArithmeticExpression::substitute(const Values & replacements) const
{
    return substitute_node(root_.get(), replacements);
}


void
ArithmeticExpression::collect_variables(const ArithmeticNode * current, std::set<std::string> & result)
{
    if(current->op == ' ')
    {
        if(!current->value.empty() && IsExpressionVariableStart(current->value[0]))
            result.insert(current->value);
        return;
    }

    if(current->left)
        collect_variables(current->left.get(), result);
    collect_variables(current->right.get(), result);
}


double
ArithmeticExpression::evaluate_node(const ArithmeticNode * current, const Values & values)
{
    if(current->op == ' ')
    {
        auto variable = values.find(current->value);
        if(variable != values.end())
            return parse_double(variable->second);
        if(!current->value.empty() && IsExpressionVariableStart(current->value[0]))
            throw std::invalid_argument("Variable \"" + current->value + "\" not defined.");
        return parse_double(current->value);
    }

    const double right = evaluate_node(current->right.get(), values);
    if(!current->left)
        return current->op == '-' ? -right : right;

    const double left = evaluate_node(current->left.get(), values);
    switch(current->op)
    {
        case '+': return left + right;
        case '-': return left - right;
        case '*': return left * right;
        case '/':
            if(right == 0)
                throw std::runtime_error("Division by zero in expression.");
            return left / right;
        default:
            throw std::invalid_argument("Invalid expression operator.");
    }
}


std::string
ArithmeticExpression::substitute_node(const ArithmeticNode * current, const Values & replacements)
{
    if(current->op == ' ')
    {
        auto replacement = replacements.find(current->value);
        return replacement == replacements.end() ? current->value : replacement->second;
    }

    const std::string right = substitute_node(current->right.get(), replacements);
    if(!current->left)
        return "(" + std::string(1, current->op) + right + ")";

    return "(" + substitute_node(current->left.get(), replacements) +
           std::string(1, current->op) + right + ")";
}
}

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
    return parse_double(value);
}


int
ComputeEngine::ComputeInt(const std::string & s)
{
    return checked_truncating_int(ComputeDouble(s), "int");
}


bool
ComputeEngine::ComputeBool(const std::string & s)
{
    EvalContext context;
    std::string value = trim(EvalMatrix(context, s, 0));
    bool result = false;
    if(parse_bool(value, result))
        return result;

    throw exception("ComputeBool could not convert \""+value+"\" to bool.", component_.path_);
}


std::vector<int>
ComputeEngine::EvaluateShapeList(const std::string & s)
{
    std::vector<int> shape;
    auto resolve_matrix_size_function = [&](const std::string & token) -> std::optional<std::string>
    {
        static const std::vector<std::string> size_functions = {
            "size_x", "size_y", "size_z", "rows", "cols", "size", "shape", "rank"
        };

        const std::size_t dot = token.rfind('.');
        if(dot == std::string::npos || dot == token.size() - 1)
            return std::nullopt;

        const std::string function_name = token.substr(dot + 1);
        const std::string matrix_path = token.substr(0, dot);
        bool recognized = std::find(size_functions.begin(), size_functions.end(), function_name) !=
                          size_functions.end();
        if(!recognized)
        {
            std::string base_name;
            std::string selector;
            recognized = ParseShapeSelector(function_name, base_name, selector);
        }

        if(!recognized || matrix_path.empty())
            return std::nullopt;

        Component * current = &component_;
        std::string local_path = matrix_path;
        if(local_path[0] == '.')
        {
            const std::string root_path = peek_head(component_.path_, ".");
            current = kernel().components.at(root_path).get();
            local_path = local_path.substr(1);
        }

        const auto & segments = SplitTopLevel(local_path, '.');
        if(segments.empty() || std::any_of(segments.begin(), segments.end(), [](const std::string & segment)
        {
            return segment.empty();
        }))
            throw exception("Matrix path \"" + matrix_path + "\" contains an empty path segment.",
                            component_.path_);

        for(std::size_t i = 0; i + 1 < segments.size(); ++i)
            current = current->GetComponent(segments[i]);

        matrix m;
        current->Bind(m, segments.back());
        if(m.is_uninitialized())
            return std::nullopt;

        std::string base_name;
        std::string selector;
        if(ParseShapeSelector(function_name, base_name, selector) &&
           selector.find(':') == std::string::npos && m.shape().empty())
            return "0";

        return MatrixShapeFunctionValue(m, function_name);
    };

    auto resolve_parameter_for_shape = [&](const std::string & token) -> std::optional<std::string>
    {
        if(token.empty() || token[0] != '@')
            return std::nullopt;

        const std::string parameter_name = token.substr(1);
        if(parameter_name.empty() || parameter_name.find('.') != std::string::npos ||
           parameter_name.find('@') != std::string::npos)
            return std::nullopt;

        parameter p;
        if(!component_.LookupParameter(p, parameter_name))
            return std::nullopt;

        if(!p.is_resolved())
        {
            std::string local_parameter_name = parameter_name;
            if(!component_.ResolveParameter(p, local_parameter_name))
                return std::nullopt;
        }

        if(p.has_options() && (p.get_type() == number_type || p.get_type() == rate_type))
            return p.as_int_string();
        if(p.get_type() == number_type || p.get_type() == rate_type)
            return formatNumber(p.as_double());
        if(p.get_type() == bool_type)
            return p.as_bool() ? "1" : "0";

        return std::nullopt;
    };

    auto unwrap_optional_dimension = [](std::string & item)
    {
        const std::string optional_prefix = "optional(";
        if(item.size() <= optional_prefix.size() || !starts_with(item, optional_prefix) || item.back() != ')')
            return false;

        int depth = 0;
        for(std::size_t i = 0; i < item.size(); ++i)
        {
            if(item[i] == '(')
                ++depth;
            else if(item[i] == ')')
            {
                --depth;
                if(depth == 0 && i != item.size() - 1)
                    return false;
            }
        }

        item = trim(item.substr(optional_prefix.size(), item.size() - optional_prefix.size() - 1));
        return true;
    };

    EvalContext context;
    for(std::string source : SplitTopLevel(context, s, ','))
    {
        source = trim(source);
        if(source.empty())
            continue;

        const bool optional_dimension = unwrap_optional_dimension(source);
        if(optional_dimension && source.empty())
            throw std::invalid_argument("optional() requires a size expression.");

        bool unresolved_variable = false;
        ArithmeticExpression expression(source);
        ArithmeticExpression::Values replacements;
        for(const auto & variable : expression.variables())
        {
            if(auto replacement = resolve_matrix_size_function(variable))
                replacements[variable] = *replacement;
            else if(!variable.empty() && variable[0] == '@')
            {
                std::optional<std::string> shape_parameter = resolve_parameter_for_shape(variable);
                std::string replacement = shape_parameter ? *shape_parameter : ComputeValue(variable);
                if(replacement == "true")
                    replacement = "1";
                else if(replacement == "false")
                    replacement = "0";
                replacements[variable] = replacement;
            }
            else
                unresolved_variable = true;
        }
        if(unresolved_variable)
            return {};

        const std::string rewritten = expression.substitute(replacements);
        const bool purely_numeric_expression = std::none_of(rewritten.begin(), rewritten.end(), [](unsigned char c)
        {
            return std::isalpha(c) || c == '@' || c == '_';
        });
        const bool contains_top_level_comma = SplitTopLevel(context, rewritten, ',').size() > 1;

        std::string computed;
        if(purely_numeric_expression && !contains_top_level_comma)
            computed = formatNumber(ArithmeticExpression(rewritten).evaluate());
        else if(purely_numeric_expression)
            computed = rewritten;
        else
            computed = ComputeValue(rewritten);

        if(computed.find(';') != std::string::npos)
            throw std::invalid_argument("Size expression \"" + source + "\" evaluated to a matrix.");

        bool had_dimension = false;
        for(std::string item : SplitTopLevel(context, computed, ','))
        {
            item = trim(item);
            if(item.empty())
                continue;
            if(optional_dimension && had_dimension)
                throw std::invalid_argument("optional() must resolve to a single dimension.");
            had_dimension = true;

            const int dimension = ComputeInt(item);
            if(dimension < 0)
                return {};
            if(dimension == 0 && optional_dimension)
                continue;
            if(dimension == 0)
                return {};
            shape.push_back(dimension);
        }
        if(optional_dimension && !had_dimension)
            return {};
    }

    return shape;
}


std::string
ComputeEngine::EvalMatrix(EvalContext & context, const std::string & s, int depth)
{
    CheckDepth(depth);

    if(s.find(';') == std::string::npos)
        return EvalList(context, trim(s), depth+1);

    const auto & rows = SplitTopLevel(context, s, ';');
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
    std::vector<std::pair<char, std::size_t>> delimiters;

    for(std::size_t i = 0; i < s.size(); ++i)
    {
        const char c = s[i];
        if(c == '(' || c == '{' || c == '[')
            delimiters.emplace_back(c, i);
        else if(c == ')' || c == '}' || c == ']')
        {
            if(delimiters.empty())
                throw std::invalid_argument("Unmatched closing delimiter \"" + std::string(1, c) +
                                            "\" at position " + std::to_string(i) + ".");

            const char expected = c == ')' ? '(' : c == '}' ? '{' : '[';
            if(delimiters.back().first != expected)
                throw std::invalid_argument("Mismatched closing delimiter \"" + std::string(1, c) +
                                            "\" at position " + std::to_string(i) +
                                            "; expected the delimiter opened by \"" +
                                            std::string(1, delimiters.back().first) + "\" at position " +
                                            std::to_string(delimiters.back().second) + ".");
            delimiters.pop_back();
        }

        if(c == separator && delimiters.empty())
        {
            items.push_back(trim(current));
            current.clear();
        }
        else
            current.push_back(c);
    }

    if(!delimiters.empty())
        throw std::invalid_argument("Unclosed delimiter \"" + std::string(1, delimiters.back().first) +
                                    "\" at position " + std::to_string(delimiters.back().second) + ".");

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
    {
        try
        {
            it = cache->emplace(s, SplitTopLevel(s, separator)).first;
        }
        catch(const std::invalid_argument & e)
        {
            throw exception(e.what(), component_.path_);
        }
    }
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

    if(path.empty())
        throw exception("Compute path \"" + s + "\" does not contain a path segment.", component_.path_);

    for(const auto & part : SplitTopLevel(context, path, '.'))
    {
        if(part.empty())
            throw exception("Compute path \"" + s + "\" contains an empty path segment.", component_.path_);
        parsed.segments.push_back(part);
    }

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
        else if(IsFunction(s))
            is_path_like = true;
        else if(s.find('.') != std::string::npos)
            is_path_like = SplitTopLevel(context, s, '.').size() > 1;
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
    if(ends_with(s, ".shape"))
        return true;
    if(ends_with(s, ".size_x") || ends_with(s, ".size_y") || ends_with(s, ".size_z")
        || ends_with(s, ".rows") || ends_with(s, ".cols") || ends_with(s, ".size")
        || ends_with(s, ".rank"))
        return true;

    std::string function_path = s;
    std::string function_name = rtail(function_path, ".");
    std::string base_name;
    std::string selector;
    if(ParseShapeSelector(function_name, base_name, selector))
        return true;

    return false;
}


bool
ComputeEngine::HasTopLevelMath(EvalContext & context, const std::string & s) const
{
    auto cached = context.top_level_math_cache.find(s);
    if(cached != context.top_level_math_cache.end())
        return cached->second;

    bool has_top_level_math = false;
    try
    {
        const std::string source = trim(s);
        ArithmeticExpression parsed(source);
        bool parenthesized = source.size() >= 2 && source.front() == '(' && source.back() == ')';
        has_top_level_math = parsed.has_operators() || parenthesized;
    }
    catch(const std::invalid_argument &)
    {}

    return context.top_level_math_cache.emplace(s, has_top_level_math).first->second;
}


bool
ComputeEngine::HasResolvableMath(EvalContext & context, const std::string & s) const
{
    if(!HasTopLevelMath(context, s))
        return false;

    try
    {
        ArithmeticExpression parsed(trim(s));
        for(const auto & variable : parsed.variables())
        {
            if(!variable.empty() && variable[0] == '@')
                continue;
            if(IsPathLike(context, variable))
                continue;
            return false;
        }
        return true;
    }
    catch(const std::invalid_argument &)
    {
        return false;
    }
}


ComputeEngine::LookupResult
ComputeEngine::LookupLocal(EvalContext & context, const std::string & name) const
{
    if(name.empty())
        return {};

    auto cached = context.lookup_cache.find(name);
    if(cached != context.lookup_cache.end())
        return cached->second;

    auto parameter_it = kernel().parameters.find(component_.path_ + '.' + name);
    if(parameter_it != kernel().parameters.end() && parameter_it->second.is_resolved())
        return context.lookup_cache.emplace(name, LookupResult{LookupResult::Source::resolved_parameter,
                                                               parameter_it->second.as_string()}).first->second;

    if(component_.info_.contains(name))
        return context.lookup_cache.emplace(name, LookupResult{LookupResult::Source::local_attribute, std::string(component_.info_[name])}).first->second;

    if(const Component * owner = component_.GetValueOwner(name))
    {
        if(owner == &component_)
            return context.lookup_cache.emplace(name, LookupResult{LookupResult::Source::local_attribute, std::string(component_.info_[name])}).first->second;

        std::string inherited_value = std::string(owner->info_[name]);
        ComputeEngine owner_engine(*const_cast<Component *>(owner));
        EvalContext owner_context;

        bool has_explicit_syntax = owner_engine.HasExplicitSyntax(owner_context, inherited_value);
        bool looks_like_number = owner_engine.LooksLikeNumber(owner_context, inherited_value);
        bool has_resolvable_math = owner_engine.HasResolvableMath(owner_context, inherited_value);
        bool is_function = owner_engine.IsFunction(inherited_value);

        if(has_explicit_syntax || looks_like_number || has_resolvable_math || is_function)
            inherited_value = owner_engine.ComputeValue(inherited_value);

        return context.lookup_cache.emplace(name, LookupResult{
            LookupResult::Source::inherited_value,
            inherited_value
        }).first->second;
    }

    std::string default_value = kernel().GetTopLevelDefaultAttribute(name);
    if(!default_value.empty())
        return context.lookup_cache.emplace(name, LookupResult{
            LookupResult::Source::inherited_value,
            default_value
        }).first->second;

    return context.lookup_cache.emplace(name, LookupResult{}).first->second;
}


std::string
ComputeEngine::EvalList(EvalContext & context, const std::string & s, int depth)
{
    CheckDepth(depth);

    if(s.find(',') == std::string::npos)
        return EvalScalar(context, trim(s), depth+1);

    const auto & items = SplitTopLevel(context, s, ',');
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

    std::string function_path = s;
    std::string function_name = rtail(function_path, ".");
    std::string matrix_name = rhead(function_path, ".");

    if(auto m = LookupMatrixValue(matrix_name))
        return MatrixShapeFunctionValue(*m, function_name);

    throw exception("Unknown compute function \""+s+"\".", component_.path_);
}


std::optional<matrix>
ComputeEngine::LookupMatrixValue(const std::string & name) const
{
    const std::string qualified_name = component_.path_ + "." + name;
    Kernel & k = kernel();
    if(k.buffers.find(qualified_name) == k.buffers.end() &&
       k.parameters.find(qualified_name) == k.parameters.end())
        return std::nullopt;

    matrix m;
    const_cast<Component &>(component_).Bind(m, name);
    return m;
}


std::string
ComputeEngine::MatrixShapeFunctionValue(const matrix & m, const std::string & function_name) const
{
    const auto shape = m.shape();

    if(function_name == "size_x")
        return std::to_string(m.shape_or_zero(-1));

    if(function_name == "size_y")
        return std::to_string(m.shape_or_zero(-2));

    if(function_name == "size_z")
        return std::to_string(m.shape_or_zero(-3));

    if(function_name == "rows")
        return std::to_string(m.rows());

    if(function_name == "cols")
        return std::to_string(m.cols());

    if(function_name == "rank")
        return std::to_string(m.rank());

    if(function_name == "size" || function_name == "shape")
        return ShapeString(shape);

    std::string base_name;
    std::string selector;
    if(ParseShapeSelector(function_name, base_name, selector))
    {
        const std::size_t colon = selector.find(':');
        if(colon == std::string::npos)
        {
            std::size_t index = 0;
            if(!ParseNonNegativeIndex(selector, index))
                throw exception("Invalid shape index \"" + function_name + "\".", component_.path_);
            if(index >= shape.size())
                throw exception("Shape index out of range in \"" + function_name + "\".", component_.path_);
            return std::to_string(shape[index]);
        }

        std::size_t start = 0;
        std::size_t end = shape.size();
        const std::string start_text = selector.substr(0, colon);
        const std::string end_text = selector.substr(colon + 1);
        if(!start_text.empty() && !ParseNonNegativeIndex(start_text, start))
            throw exception("Invalid shape slice \"" + function_name + "\".", component_.path_);
        if(!end_text.empty() && !ParseNonNegativeIndex(end_text, end))
            throw exception("Invalid shape slice \"" + function_name + "\".", component_.path_);

        start = std::min(start, shape.size());
        end = std::min(end, shape.size());
        if(end < start)
            end = start;
        return ShapeString(std::vector<int>(shape.begin() + start, shape.begin() + end));
    }

    throw exception("Unknown matrix size function \""+function_name+"\".", component_.path_);
}


std::string
ComputeEngine::ShapeString(const std::vector<int> & shape) const
{
    std::string result;
    for(size_t i = 0; i < shape.size(); ++i)
    {
        if(i > 0)
            result += ",";
        result += std::to_string(shape[i]);
    }
    return result;
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
            return formatNumber(parse_double(segment));
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
        if(expanded.empty())
            throw exception("Compute path segment \"" + segments[0] + "\" resolved to an empty value.",
                            current->path_);

        const auto & extra = current_engine.SplitTopLevel(current_context, expanded, '.');
        std::vector<std::string> rewritten;
        for(const auto & piece : extra)
        {
            if(piece.empty())
                throw exception("Expanded compute path \"" + expanded + "\" contains an empty path segment.",
                                current->path_);
            rewritten.push_back(piece);
        }
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

    ArithmeticExpression e(s);
    ArithmeticExpression::Values vars;
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
            return formatNumber(parse_double(current));

        if(ShouldReturnLiteral(context, current, evaluate_final))
            return current;

        if(!IsPathLike(context, current))
            current = ExpandCurly(context, current, depth+1);

        if(HasTopLevelMath(context, current))
            current = EvalMath(context, current, depth+1);
        else
            current = EvalPath(context, current, depth+1, evaluate_final);

        current = trim(current);
        if(current.empty())
            return "";

        // After explicit expansion or a successful final lookup, preserve a newly
        // resolved literal string rather than re-evaluating punctuation inside it
        // as math or another lookup. This keeps values such as "abc/def:ghi"
        // returned from "@name" intact for string parameters.
        if(current != previous &&
           !HasExplicitSyntax(context, current) &&
           LooksLikeMatrixLiteralValue(current))
            return current;

        if(current != previous &&
           !HasExplicitSyntax(context, current) &&
           !HasResolvableMath(context, current) &&
           !IsFunction(current) &&
           std::any_of(current.begin(), current.end(), [](unsigned char c) { return std::isalpha(c); }))
            return current;

        if(current == previous)
            return current;
    }

    throw exception("Compute expression did not converge.", component_.path_);
}

}
