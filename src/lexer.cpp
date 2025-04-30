// lexer.cpp

#include "lexer.h"
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <string>

// Global lexer state
std::string IdentifierStr; // If tok_identifier, filled with the identifier
double NumVal;             // If tok_number, filled with number
std::string StringVal;     // If tok_string, filled with string
int CurTok;

static int LastChar = ' ';

static std::string InputBuffer;
static size_t InputPos = 0;

void setInput(const std::string &Src)
{
    InputBuffer = Src;
    InputPos = 0;
}

int getNextChar()
{
    if (InputPos >= InputBuffer.size())
    {
        return EOF;
    }
    return InputBuffer[InputPos++];
}

// gettok - Return the next token from standard input
void gettok()
{
    // Skip whitespace
    while (isspace(LastChar))
    {
        LastChar = getNextChar();
    }

    // Identifier: [a-zA-Z][a-zA-Z0-9_]*
    if (isalpha(LastChar))
    {
        IdentifierStr = LastChar;
        while (isalnum((LastChar = getNextChar())) || LastChar == '_')
        {
            IdentifierStr += LastChar;
        }


        if (IdentifierStr == "attempt")
        {
            CurTok = tok_attempt;
            return;
        }
        if (IdentifierStr == "for_some")
        {
            CurTok = tok_forsome;
            return;
        }
        if (IdentifierStr == "awhile")
        {
            CurTok = tok_awhile;
            return;
        }
        if (IdentifierStr == "what_if")
        {
            CurTok = tok_whatif;
            return;
        }
        if (IdentifierStr == "what_else")
        {
            CurTok = tok_whatelse;
            return;
        }
        if (IdentifierStr == "return")
        {
            CurTok = tok_return;
            return;
        }
        if (IdentifierStr == "true")
        {
            CurTok = tok_true;
            return;
        }
        if (IdentifierStr == "false")
        {
            CurTok = tok_false;
            return;
        }
        if (IdentifierStr == "list")
        {
            CurTok = tok_list;
            return;
        }
        if (IdentifierStr == "gimme")
        {
            CurTok = tok_gimme;
            return;
        }
        if (IdentifierStr == "reveal")
        {
            CurTok = tok_reveal;
            return;
        }
        if (IdentifierStr == "number")
        {
            CurTok = tok_numbertype;
            return;
        }

        if (IdentifierStr == "string")
        {
            CurTok = tok_stringtype;
            return;
        }

        CurTok = tok_identifier;
        return;
    }

    // Number: [0-9.]+
    if (isdigit(LastChar) || LastChar == '.')
    {

        std::string NumStr;
        do
        {
            NumStr += LastChar;
            LastChar = getNextChar();
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), nullptr);
        CurTok = tok_number;
        return;
    }

    // String literal
    if (LastChar == '"')
    {
        StringVal = "";

        // Read characters inside the string
        while (true)
        {
            LastChar = getNextChar();

            if (LastChar == EOF)
            {
                CurTok = tok_eof;
                return;
            }

            if (LastChar == '"')
            {
                // Found end of string
                break;
            }

            StringVal += LastChar;
        }

        // After closing '"', move to next character
        LastChar = getNextChar();
        CurTok = tok_string;
        return;
    }

    // Operators
    if (LastChar == '=')
    {
        LastChar = getNextChar();
        if (LastChar == '?')
        {
            LastChar = getNextChar();
            CurTok = tok_eqassign;
            return;
        }
        else if (LastChar == '=')
        {
            LastChar = getNextChar();
            CurTok = tok_eq;
            return;
        }
        else
        {
            CurTok = '=';
            return;
        }
    }

    if (LastChar == '!')
    {
        LastChar = getNextChar();
        if (LastChar == '=')
        {
            LastChar = getNextChar();
            CurTok = tok_ne;
            return;
        }
        else
        {
            CurTok = '!';
            return;
        }
    }

    if (LastChar == '<')
    {
        LastChar = getNextChar();
        if (LastChar == '=')
        {
            LastChar = getNextChar();
            CurTok = tok_le;
            return;
        }
        else
        {
            CurTok = tok_lt;
            return;
        }
    }

    if (LastChar == '>')
    {
        LastChar = getNextChar();
        if (LastChar == '=')
        {
            LastChar = getNextChar();
            CurTok = tok_ge;
            return;
        }
        else
        {
            CurTok = tok_gt;
            return;
        }
    }

    // Handle simple single-character tokens
    // Handle single-character tokens
    if (LastChar == '(')
    {
        LastChar = getNextChar();
        CurTok = tok_lparen;
        return;
    }
    if (LastChar == ')')
    {
        LastChar = getNextChar();
        CurTok = tok_rparen;
        return;
    }
    if (LastChar == '{')
    {
        LastChar = getNextChar();
        CurTok = tok_lbrace;
        return;
    }
    if (LastChar == '}')
    {
        LastChar = getNextChar();
        CurTok = tok_rbrace;
        return;
    }
    if (LastChar == ';')
    {
        LastChar = getNextChar();
        CurTok = tok_semi;
        return;
    }
    if (LastChar == ',')
    {
        LastChar = getNextChar();
        CurTok = tok_comma;
        return;
    }
}
