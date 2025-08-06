#include "ExprParser.h"
#include <vector>
#include <string>
#include <map>
#include <stack>
#include <algorithm>
#include <stdexcept>
#include <cstdint>
#include <cmath>

using namespace std;

vector<Token> shuntingYard (const string& expr)
{
    static map<char,int> prec = {
        // L = <<, R = >>
        // F = function (sin,cos)
        {'F', 6},
        {'*', 5}, {'/', 5},{ '%', 5},
        {'+', 4}, {'-', 4},
        {'L', 3}, {'R', 3},
        {'&', 2},
        {'^', 1},
        {'|', 0}
    };
    vector<Token>      output;
    stack<char>        opstack;

    for (size_t i=0; i<expr.size(); ++i)
    {
        if (expr.substr(i, 3) == "sin")
        {
            output.push_back({TokenType::Function, 0, 0, "sin"});
            i += 2;
            continue;
        }
        else if (expr.substr(i, 3) == "cos")
        {
            output.push_back({TokenType::Function, 0, 0, "cos"});
            i += 2;
            continue;
        }

        char c = expr[i];

        // bit shifts:
        if (i+1 < expr.size() && expr[i+1] == c && (c == '<' || c == '>'))
        {
            char shiftOp;
            if (c == '<')
                shiftOp = 'L';
            else
                shiftOp = 'R'; 
            while (!opstack.empty() && opstack.top() != '(' && prec.at(opstack.top()) >= prec.at(shiftOp))
            {
                output.push_back({TokenType::Operator,0,opstack.top()});
                opstack.pop();
            }
            opstack.push(shiftOp);
            ++i;
            continue;
        }

        if (c == 't')
        {
            output.push_back({TokenType::Variable, 0, 't'});
        }
        else if (c == 'x')
        {
            output.push_back({TokenType::Input, 0, 'x'});
        }
        else if (isdigit(c))
        {
            // handle multi-digit integers
            size_t start = i;
            while (i < expr.size() && isdigit(expr[i]))
                ++i;
            std::string numStr = expr.substr(start, i - start);
            int v = std::stoi(numStr);
            output.push_back({TokenType::Number, v, 0});
            --i;
        }
        else if (prec.count(c))
        {
            while (!opstack.empty() && opstack.top() != '(' && prec.at(opstack.top()) >= prec.at(c))
            {
                output.push_back({TokenType::Operator,0,opstack.top()});
                opstack.pop();
            }
            opstack.push(c);
        }
        else if (c == '(')
        {
            opstack.push(c);
        }
        else if (c == ')')
        {
            while (!opstack.empty() && opstack.top() != '(')
            {
                output.push_back({TokenType::Operator,0,opstack.top()});
                opstack.pop();
            }
            if (opstack.empty())
                throw runtime_error("Mismatched ')'");
            opstack.pop();
        }
        else if (c == ' ')
        {
            continue;
        }
        else
        {
            throw runtime_error(string("Invalid char: ")+c);
        }
    }

    while (!opstack.empty())
    {
        if (opstack.top()=='(' || opstack.top()==')')
            throw runtime_error("Mismatched brackets");
        output.push_back({TokenType::Operator,0,opstack.top()});
        opstack.pop();
    }

    return output;
}

int evaluateExpr(const vector<Token>& tokens, uint32_t t, int x)
{
    vector<int> stack;
    for (const auto& token : tokens)
    {
        switch (token.type)
        {
            case TokenType::Number:
                stack.push_back(token.value);
                break;
            case TokenType::Variable:
                stack.push_back(static_cast<int>(t));
                break;
            case TokenType::Input:
                stack.push_back(x);
                break;
            case TokenType::Function: {
                if (stack.empty()) return 0;
                int arg = stack.back(); stack.pop_back();
                if (token.func == "sin")
                    stack.push_back(static_cast<int>(127.5f * (std::sin(arg) + 1.0f)));
                else if (token.func == "cos")
                    stack.push_back(static_cast<int>(127.5f * (std::cos(arg) + 1.0f)));
                break;
            }
            case TokenType::Operator: {
                if (stack.size() < 2) return 0;
                int b = stack.back(); stack.pop_back();
                int a = stack.back(); stack.pop_back();
                switch (token.op) {
                    case '+': stack.push_back(a + b); break;
                    case '-': stack.push_back(a - b); break;
                    case '*': stack.push_back(a * b); break;
                    case '/':
                        if (b != 0)
                            stack.push_back(a / b);
                        else
                            stack.push_back(0);
                        break;
                    case '%':
                        if (b != 0)
                            stack.push_back(a % b);
                        else
                            stack.push_back(0);
                        break;
                    case '&': stack.push_back(a & b); break;
                    case '|': stack.push_back(a | b); break;
                    case '^': stack.push_back(a ^ b); break;
                    case 'L': stack.push_back(a << b); break;
                    case 'R': stack.push_back(a >> b); break;
                }
                break;
            }
            default: break;
        }
    }
    if (stack.empty())
        return 0;
    else
        return stack.back();
}
