#pragma once

#include <vector>
#include <string>
#include <cstdint>

using namespace std;

enum class TokenType { Number, Variable, Input, Operator, Function };

struct Token
{
    TokenType type;
    int        value;
    char       op;
    string func;
};

vector<Token> shuntingYard(const string& expr);
int evaluateExpr(const vector<Token>& tokens, uint32_t t, int x);
