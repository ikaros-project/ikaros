// expression.h - (c) Christian Balkenius 2023
//
// Small arithmetic-expression parser for numeric expressions.
//
// Supported syntax:
// - binary operators: +, -, *, /
// - unary plus and minus
// - parentheses
// - decimal and scientific-notation numbers
// - variables beginning with a letter, '_', '@', or an absolute-path dot
//
// expression e(s) parses an expression string.
// e.variables() returns the variables referenced by the expression.
// e.evaluate(vars) evaluates the expression using a map of variable values.
//
// Expressions are evaluated as doubles. Invalid syntax, missing variables,
// and invalid numeric conversions throw std::invalid_argument.

#ifndef EXPRESSION
#define EXPRESSION

#include <cctype>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>

#include "utilities.h"

using variables = std::map<std::string, std::string>;

namespace expression_detail
{
struct node
{
    char op = ' ';
    std::string value;
    std::unique_ptr<node> left;
    std::unique_ptr<node> right;
};


class parser
{
public:
    explicit parser(const std::string & source):
        source_(source),
        position_(0)
    {}

    std::unique_ptr<node> parse()
    {
        skip_whitespace();
        if(at_end())
            fail("Expression cannot be empty");

        std::unique_ptr<node> result = parse_additive();
        skip_whitespace();
        if(!at_end())
            fail("Unexpected token");
        return result;
    }

private:
    const std::string & source_;
    std::size_t position_;

    static bool initial_identifier_char(char c)
    {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == '@';
    }

    static bool identifier_char(char c)
    {
        return initial_identifier_char(c) || c == '.' || c == '[' || c == ']' || c == ':'
            || (c >= '0' && c <= '9');
    }

    bool at_end() const
    {
        return position_ >= source_.size();
    }

    char current() const
    {
        return at_end() ? '\0' : source_[position_];
    }

    void skip_whitespace()
    {
        while(!at_end() && std::isspace(static_cast<unsigned char>(source_[position_])))
            ++position_;
    }

    bool consume(char token)
    {
        skip_whitespace();
        if(current() != token)
            return false;
        ++position_;
        return true;
    }

    [[noreturn]] void fail(const std::string & message) const
    {
        throw std::invalid_argument(message + " at position " + std::to_string(position_) + ".");
    }

    std::unique_ptr<node> make_terminal(const std::string & value) const
    {
        auto result = std::make_unique<node>();
        result->value = value;
        return result;
    }

    std::unique_ptr<node> make_unary(char op, std::unique_ptr<node> operand) const
    {
        auto result = std::make_unique<node>();
        result->op = op;
        result->right = std::move(operand);
        return result;
    }

    std::unique_ptr<node> make_binary(char op, std::unique_ptr<node> left, std::unique_ptr<node> right) const
    {
        auto result = std::make_unique<node>();
        result->op = op;
        result->left = std::move(left);
        result->right = std::move(right);
        return result;
    }

    std::unique_ptr<node> parse_additive()
    {
        std::unique_ptr<node> result = parse_multiplicative();
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

    std::unique_ptr<node> parse_multiplicative()
    {
        std::unique_ptr<node> result = parse_unary();
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

    std::unique_ptr<node> parse_unary()
    {
        if(consume('+'))
            return make_unary('+', parse_unary());
        if(consume('-'))
            return make_unary('-', parse_unary());
        return parse_primary();
    }

    std::unique_ptr<node> parse_primary()
    {
        skip_whitespace();
        if(at_end())
            fail("Missing operand");

        if(consume('('))
        {
            std::unique_ptr<node> result = parse_additive();
            if(!consume(')'))
                fail("Missing closing parenthesis");
            return result;
        }

        const char c = current();
        if(std::isdigit(static_cast<unsigned char>(c)) ||
           (c == '.' && position_ + 1 < source_.size() &&
            std::isdigit(static_cast<unsigned char>(source_[position_ + 1]))))
            return parse_number();

        if(initial_identifier_char(c) ||
           (c == '.' && position_ + 1 < source_.size() &&
            initial_identifier_char(source_[position_ + 1])))
            return parse_identifier();

        fail("Expected a number, variable, or parenthesized expression");
    }

    std::unique_ptr<node> parse_number()
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
        static_cast<void>(::ikaros::parse_double(value));
        return make_terminal(value);
    }

    std::unique_ptr<node> parse_identifier()
    {
        const std::size_t start = position_;
        if(current() == '.')
            ++position_;
        while(!at_end() && identifier_char(current()))
            ++position_;
        return make_terminal(source_.substr(start, position_ - start));
    }
};
}


class expression
{
public:
    explicit expression(const std::string & source):
        root_(expression_detail::parser(source).parse())
    {}

    bool split(const std::string & source, char bin_op, bool unary=false)
    {
        std::unique_ptr<expression_detail::node> parsed = expression_detail::parser(source).parse();
        bool is_unary = parsed->op != ' ' && !parsed->left;
        if(parsed->op != bin_op || (unary && is_unary))
            return false;
        root_ = std::move(parsed);
        return true;
    }

    std::set<std::string> variables() const
    {
        std::set<std::string> result;
        collect_variables(root_.get(), result);
        return result;
    }

    double evaluate(const ::variables & vars = {}) const
    {
        try
        {
            return evaluate(root_.get(), vars);
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

    bool has_operators() const noexcept
    {
        return root_->op != ' ';
    }

    std::string substitute(const std::map<std::string, std::string> & replacements) const
    {
        return substitute(root_.get(), replacements);
    }

    void print(int depth=0) const
    {
        print(root_.get(), depth);
        if(depth == 0)
            std::cout << '\n';
    }

private:
    std::unique_ptr<expression_detail::node> root_;

    static bool initial_identifier_char(char c)
    {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == '@' || c == '.';
    }

    static void collect_variables(const expression_detail::node * current, std::set<std::string> & result)
    {
        if(current->op == ' ')
        {
            if(!current->value.empty() && initial_identifier_char(current->value[0]))
                result.insert(current->value);
            return;
        }

        if(current->left)
            collect_variables(current->left.get(), result);
        collect_variables(current->right.get(), result);
    }

    static double evaluate(const expression_detail::node * current, const ::variables & vars)
    {
        if(current->op == ' ')
        {
            auto variable = vars.find(current->value);
            if(variable != vars.end())
                return ::ikaros::parse_double(variable->second);
            if(!current->value.empty() && initial_identifier_char(current->value[0]))
                throw std::invalid_argument("Variable \"" + current->value + "\" not defined.");
            return ::ikaros::parse_double(current->value);
        }

        const double right = evaluate(current->right.get(), vars);
        if(!current->left)
            return current->op == '-' ? -right : right;

        const double left = evaluate(current->left.get(), vars);
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

    static std::string substitute(const expression_detail::node * current,
                                  const std::map<std::string, std::string> & replacements)
    {
        if(current->op == ' ')
        {
            auto replacement = replacements.find(current->value);
            return replacement == replacements.end() ? current->value : replacement->second;
        }

        const std::string right = substitute(current->right.get(), replacements);
        if(!current->left)
            return "(" + std::string(1, current->op) + right + ")";

        return "(" + substitute(current->left.get(), replacements) +
               std::string(1, current->op) + right + ")";
    }

    static void print(const expression_detail::node * current, int depth)
    {
        if(current->op == ' ')
            std::cout << ::ikaros::tab(depth) << "[" << current->value << "]\n";
        else
            std::cout << ::ikaros::tab(depth) << current->op << '\n';
        if(current->left)
            print(current->left.get(), depth + 1);
        if(current->right)
            print(current->right.get(), depth + 1);
    }
};

#endif
