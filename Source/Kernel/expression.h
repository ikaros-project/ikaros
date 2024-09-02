// expression.h - -(c) Christian Balkenius 2023
//
// evaluates a numerical expression with optional variables
//
// variables must start with a letter or _ or @  and may also include dots and numbers
//
// expression e = expression(string) creates the expression
// e.variables() retusn a set of strings that contains the variables in the expression
// e.evaluat(vars) evaluate the expression with the variables in vars (map of strings)
//
// expressions are evaluated as floats

#ifndef EXPRESSION
#define EXPRESSION

#include <exception>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "utilities.h"

typedef std::map<std::string, std::string> variables;

class expression
{
public:
    static bool is_expression(const std::string & s)
    {
        if(s.empty())
            return false;

        if(s.find(',') != std::string::npos)
            return false;

    if(s.find(';') != std::string::npos)
            return false;
            
        for(char c : "+-*/()")
            if(s.find(c) != std::string::npos)
            return true;
        //if(s[0] >= '0' && s[0] <='9')
        //    return true;
        return false;
    }

    bool split(std::string s, char bin_op, bool unary=false)
    {
        std::string left_string, right_string;
        if(!split_expression(left_string, right_string, s, bin_op, unary))
            return false;

    op = bin_op;
    left = std::make_unique<expression>(left_string);
    right = std::make_unique<expression>(right_string);
    return true;
    }

    expression(std::string s)
    {
        int left_p_count = std::count(s.begin(), s.end(), '(');
        int right_p_count = std::count(s.begin(), s.end(), ')');

        if(left_p_count != right_p_count)
            throw std::invalid_argument("Unmatched parantheses");

        op = ' ';
        erase_whitespace(s);
        str = s; 

        if(s.empty())
            return;

        while(s.size()>1 && s.front()=='(' && s.back() == ')' && find_closing(s) == s.size()-1)
            str = s = s.substr(1, s.size()-2);

        if(split(s, '+'))
            return;

        if(split(s, '-', true))
            return;

        if(split(s, '/'))
            return;

        if(split(s, '*'))
            return;

        if(split(s, '-')) // unary minus
            return;

        // terminal - do nothing
    }

    std::set<std::string> variables()
    {
        std::set<std::string> vars;
        std::string s;
        bool in_id = false;

        for(char c : str)
        {
            if(initial_identifier_char(c))
                in_id = true;
            else if(!identifier_char(c))
                in_id = false;

            if(in_id)       
                s.push_back(c);


            if(!in_id && !s.empty())
            {
                vars.insert(s);
                s.clear();
            }
        }

            if(!s.empty())
                vars.insert(s);

        return vars;
    }

    double evaluate(::variables vars = {})
    {
        try
        {
            switch(op)
            {
                case ' ':   if(str.empty())
                                return 0;
                            else if(vars.count(str))
                                str = vars[str];
                            if(str.empty()) // FIXME: throw parameter is not set; should get default if possible
                                return 0;
                            return std::stod(str);
                

                case '+':   return left->evaluate(vars) + right->evaluate(vars);
                case '-':   return left->evaluate(vars) - right->evaluate(vars);
                case '*':   return left->evaluate(vars) * right->evaluate(vars);
                case '/':   
                    {
                            double r = right->evaluate(vars);
                            if(r==0) 
                                throw std::runtime_error("Division by zero in expression.");
                            return left->evaluate(vars) / r;
                    }
            default:
                return 0;
            }   
        }
        catch(std::exception & e)
        {
            throw std::invalid_argument(e.what());
        }
        catch(...)
        {
            throw std::invalid_argument("invalid expression");
        }
    }

    void print(int depth=0)
    {
        if(op==' ')
            std::cout << ::ikaros::tab(depth) << "[" << str << "]" << std::endl;
        else
            std::cout << ::ikaros::tab(depth) << op << std::endl;
        if(left)
            left->print(depth+1);
        if(right)
            right->print(depth+1);
        if(depth == 0)
            std::cout << std::endl;
    }

private:

    bool initial_identifier_char(char c)
        {
            return (c>='A' && c<='Z') || (c>='a' && c<='z') || (c=='_') || (c=='@');
        }

    bool identifier_char(char c)
    {
        return initial_identifier_char(c)  || (c=='.')   || (c >= '0' && c <= '9');
    }

    void
    erase_whitespace(std::string & s)
    {
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char x){ return std::isspace(x); }), s.end());
    }

    // split string at token except withing parantheses or after a chacter in not_after-list (for unary minus mainly)

    int find_closing(std::string & s)
    {
        int c = 1;
        for(int i=1; i< s.size(); i++)
        {
            if(s[i]==')')
                c--;
            else if(s[i] == '(')
                c++;
            if(c==0)
                return i;
        }
        return 0;
    }

    bool split_expression(std::string & head, std::string & tail, std::string & s, char token, bool unary=false)
    {
        if(s.empty())
            return false;

        int p_count = 0;
        for(int i=s.size()-1; i>=0 ; i--)
        {
            if(s[i]=='(')
                p_count++;
            if(s[i]==')')
                p_count--;
            if(p_count==0 && s[i]==token)
            {
                bool match = true;
                std::string not_after = "+(*/";
                if(unary)
                    for(int j=0; j<not_after.size(); j++)
                        if(i>0 && s[i-1]==not_after[j])
                        {
                            match = false;
                            break;
                        }
                
                if(match)
                {    
                    int start=0;
                    while(isspace(s[start]))
                        start++;
                        
                        head = s.substr(start, i);
                        tail = s.substr(i+1, s.size()-i);
                    return true;
                }
            }
        }
        return false;
    }

    char op;
    std::string str;
    std::unique_ptr<expression> left;
    std::unique_ptr<expression> right;
};

#endif