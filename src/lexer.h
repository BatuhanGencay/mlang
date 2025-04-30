#pragma once
#include <iostream>
#include <string>

// Token types
enum Token {
    tok_eof = -1,

    // Keywords
    tok_attempt = -2,
    tok_forsome = -3,
    tok_awhile = -4,
    tok_whatif = -5,
    tok_whatelse = -6,
    tok_return = -7,
    tok_true = -8,
    tok_false = -9,
    tok_list = -10,
    tok_gimme = -11,
    tok_reveal = -12,

    // Identifiers and constants
    tok_identifier = -20,
    tok_number = -21,
    tok_string = -22,

    //variables
    tok_numbertype = -40, // 'number' keyword (variable declaration)
    tok_stringtype = -41, // 'string' keyword (variable declaration)

    // Operators
    tok_eqassign = -30,
    tok_eq = -31,
    tok_ne = -32,
    tok_lt = '<',
    tok_gt = '>',
    tok_le = -33,
    tok_ge = -34,
    tok_plus = '+',
    tok_minus = '-',
    tok_mul = '*',
    tok_div = '/',
    tok_lparen = '(',
    tok_rparen = ')',
    tok_lbrace = '{',
    tok_rbrace = '}',
    tok_comma = ',',
    tok_semi = ';'
};

// Lexer functions
extern std::string IdentifierStr;
extern double NumVal;
extern std::string StringVal;

void setInput(const std::string &Src);
int getNextChar();
void gettok();

extern int CurTok;
