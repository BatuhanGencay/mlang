// parser.cpp

#include "lexer.h"
#include "parser.h"
#include "ast.h"

#include <map>
#include <memory>
#include <vector>
#include <string>
#include <iostream>

std::map<std::string, int> VariableTypes;

// Operator precedence map (defined in parser.h)
std::map<int, int> BinopPrecedence;

void InitBinopPrecedence()
{
    BinopPrecedence[tok_lt] = 10;
    BinopPrecedence[tok_gt] = 10;
    BinopPrecedence[tok_eq] = 10;
    BinopPrecedence[tok_ne] = 10;
    BinopPrecedence[tok_le] = 10;
    BinopPrecedence[tok_ge] = 10;
    BinopPrecedence[tok_plus] = 20;
    BinopPrecedence[tok_minus] = 20;
    BinopPrecedence[tok_mul] = 40;
    BinopPrecedence[tok_div] = 40;
}

// Error handling helpers
static std::unique_ptr<ExprAST> LogError(const char *Str)
{
    std::cerr << "Parser Error: " << Str << std::endl;
    return nullptr;
}

static std::unique_ptr<FunctionAST> LogErrorP(const char *Str)
{
    std::cerr << "Parser Error: " << Str << std::endl;
    return nullptr;
}
llvm::Value *LogErrorV(const char *Str)
{
    fprintf(stderr, "Codegen Error: %s\n", Str);
    return nullptr;
}
// number-expression: [0-9.]+
static std::unique_ptr<ExprAST> ParseNumberExpr()
{
    std::cerr << "ParseNumberExpr " << std::endl;

    auto Result = std::make_unique<NumberExprAST>(NumVal);
    gettok(); // consume the number
    return Result;
}

// string-expression: "..."
static std::unique_ptr<ExprAST> ParseStringExpr()
{
    std::cerr << "[DEBUG] ParseStringExpr sees StringVal: " << StringVal << std::endl;
    auto Result = std::make_unique<StringExprAST>(StringVal);
    gettok(); // consume the string
    return Result;
}

// parenthesized-expression: '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr()
{
    gettok(); // consume '('
    std::cerr << "[DEBUG] After eating '(', CurTok = " << CurTok << std::endl;

    auto V = ParseExpression();
    if (!V)
        return nullptr;
    if (CurTok != tok_rparen)
        return LogError("expected ')'");
    gettok(); // consume ')'
    return V;
}

// identifier-expression: variable or function call
static std::unique_ptr<ExprAST> ParseIdentifierExpr()
{
    std::cerr << "ParseIdentifierExpr " << std::endl;
    std::string IdName = IdentifierStr;
    gettok(); // consume identifier

    if (CurTok != tok_lparen)
        return std::make_unique<VariableExprAST>(IdName);

    // function call
    gettok(); // consume '('
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (CurTok != tok_rparen)
    {
        while (true)
        {
            if (auto Arg = ParseExpression())
                Args.push_back(std::move(Arg));
            else
                return nullptr;

            if (CurTok == tok_rparen)
                break;
            if (CurTok != tok_comma)
                return LogError("Expected ',' in argument list");
            gettok(); // consume comma
        }
    }
    gettok(); // consume ')'
    return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

// primary-expression dispatch
std::unique_ptr<ExprAST> ParsePrimary()
{
    std::cerr << "ParsePrimary " <<CurTok<< " " << char(CurTok) << std::endl;
    switch (CurTok)
    {
    case tok_identifier:
        return ParseIdentifierExpr();
    case tok_number:
        return ParseNumberExpr();
    case tok_string:
        return ParseStringExpr();
    case tok_lparen:
        return ParseParenExpr();
    case tok_lbrace:
        return ParseBlock();
    case tok_whatif:
        return ParseIfExpr();
    case tok_awhile:
        return ParseWhileExpr();
    case tok_return:
        return ParseReturnExpr();
    case tok_numbertype:
        return ParseVariableDeclaration();
    case tok_stringtype:
        return ParseVariableDeclaration();
    case tok_reveal:
        return ParseRevealExpr();

    case tok_true:
        gettok();
        return std::make_unique<NumberExprAST>(1.0);
    case tok_false:
        gettok();
        return std::make_unique<NumberExprAST>(0.0);
    default:
        return LogError("unknown token when expecting a primary expression");
    }
}

// unary-expression: ('+'|'!'|'-') unary | primary
std::unique_ptr<ExprAST> ParseUnary() {
    // Primary expressions
    if (!isascii(CurTok) || CurTok == tok_lparen || CurTok == tok_identifier || CurTok == tok_lbrace)
        return ParsePrimary();

    // Invalid tokens like rbrace should be rejected
    if (CurTok == tok_rbrace) {
        std::cerr << "[error] Unexpected '}' (tok_rbrace) in unary expression\n";
        gettok();  // Skip and continue
        return nullptr;
    }

    // Only allow defined unary operators
    if (CurTok != '-' && CurTok != '!' && CurTok != '~') {
        std::cerr << "[error] Invalid unary operator: " << (char)CurTok << " (" << CurTok << ")\n";
        gettok();  // Skip token to avoid infinite loop
        return nullptr;
    }

    int Opc = CurTok;
    gettok(); // consume operator

    if (auto Operand = ParseUnary()) {
        return std::make_unique<UnaryExprAST>(Opc, std::move(Operand));
    }

    std::cerr << "[error] Invalid unary expression after operator: " << (char)Opc << "\n";
    return nullptr;
}


// binary-expression parsing
std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS)
{
    while (true)
    {
        int TokPrec = BinopPrecedence[CurTok];
        if (TokPrec < ExprPrec)
            return LHS;

        int BinOp = CurTok; // Save the operator token
        gettok();           // consume operator

        auto RHS = ParseUnary();
        if (!RHS)
            return nullptr;

        int NextPrec = BinopPrecedence[CurTok];
        if (TokPrec < NextPrec)
        {
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
            if (!RHS)
                return nullptr;
        }

        LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
    }
}

// full-expression parsing entry
std::unique_ptr<ExprAST> ParseExpression()
{
    std::cerr << "[DEBUG] Entering ParseExpression, CurTok = " << CurTok<<" "<<char(CurTok) << std::endl;
    auto LHS = ParseUnary();
    if (!LHS)
        return nullptr;
    return ParseBinOpRHS(0, std::move(LHS));
}

// block parsing: '{' stmt* '}'
std::unique_ptr<ExprAST> ParseBlock()
{
    std::cerr << "[DEBUG] Entering ParseBlock, CurTok = " << CurTok<<" "<<char(CurTok) << std::endl;
    gettok(); // eat '{'

    std::vector<std::unique_ptr<ExprAST>> Statements;

    while (true)
    {
        std::cerr << "[DEBUG] ParseBlock loop, CurTok = " << CurTok<<" "<<char(CurTok) << std::endl;

        if (CurTok == tok_rbrace)
        {
            // Finished the block
            break;
        }
        if (CurTok == tok_eof)
        {
            return LogError("unexpected EOF inside block");
        }

        if (auto Stmt = ParseExpression())
        {
            std::cerr << "[DEBUG] Stmt, CurTok = " << CurTok<<" "<<char(CurTok) << std::endl;

            Statements.push_back(std::move(Stmt));

            // After each statement, expect ';'
            if (CurTok == tok_semi)
            {
                gettok(); // eat ';'
            }
            else
            {
                return LogError("expected ';' after statement inside block");
            }
        }
        else
        {
            return nullptr; // error already reported
        }
    }

    gettok(); // eat '}'

    return std::make_unique<BlockExprAST>(std::move(Statements));
}

// function definition parsing: 'attempt' id '(' args ')' block
std::unique_ptr<FunctionAST> ParseDefinition()
{
    std::cerr << "[DEBUG] ParseDefinition sees token: "<<CurTok<<" " << char(CurTok) << std::endl;
    gettok(); // eat 'attempt'

    if (CurTok != tok_identifier)
        return LogErrorP("expected function name after 'attempt'");

    std::string FuncName = IdentifierStr;
    gettok(); // eat function name

    if (CurTok != tok_lparen)
        return LogErrorP("expected '(' after function name");
    gettok(); // eat '('

    std::vector<std::string> ArgNames;
    if (CurTok != tok_rparen)
    {
        while (true)
        {
            if (CurTok != tok_identifier)
                return LogErrorP("expected argument name");

            ArgNames.push_back(IdentifierStr);
            gettok(); // eat argument name

            if (CurTok == tok_rparen)
                break;

            if (CurTok != tok_comma)
                return LogErrorP("expected ',' between arguments");
            gettok(); // eat ','
        }
    }
    gettok(); // eat ')'

    if (CurTok != tok_lbrace)
        return LogErrorP("expected '{' before function body");
    gettok();
    auto Body = ParseBlock();
    if (!Body)
        return nullptr;

    auto Proto = std::make_unique<PrototypeAST>(FuncName, std::move(ArgNames));
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(Body));
}

std::unique_ptr<ExprAST> ParseIfExpr()
{
    gettok(); // eat 'what_if'

    if (CurTok != tok_lparen)
        return LogError("expected '(' after 'what_if'");
    gettok(); // eat '('

    auto Cond = ParseExpression();
    if (!Cond)
        return nullptr;

    if (CurTok != tok_rparen)
        return LogError("expected ')' after condition");
    gettok(); // eat ')'

    auto Then = ParseBlock();
    if (!Then)
        return nullptr;

    if (CurTok != tok_whatelse)
        return LogError("expected 'what_else'");
    gettok(); // eat 'what_else'

    auto Else = ParseBlock();
    if (!Else)
        return nullptr;

    return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then), std::move(Else));
}

std::unique_ptr<ExprAST> ParseWhileExpr()
{
    gettok(); // eat 'awhile'

    if (CurTok != tok_lparen)
        return LogError("expected '(' after 'awhile'");
    gettok(); // eat '('

    auto Cond = ParseExpression();
    if (!Cond)
        return nullptr;

    if (CurTok != tok_rparen)
        return LogError("expected ')' after condition");
    gettok(); // eat ')'

    auto Body = ParseBlock();
    if (!Body)
        return nullptr;

    return std::make_unique<WhileExprAST>(std::move(Cond), std::move(Body));
}

std::unique_ptr<ExprAST> ParseReturnExpr()
{
    gettok(); // eat 'return'

    auto RetVal = ParseExpression();
    if (!RetVal)
        return nullptr;

    if (CurTok == tok_semi)
        gettok(); // optional ';' after return expression

    return std::make_unique<ReturnExprAST>(std::move(RetVal));
}

std::unique_ptr<ExprAST> ParseVariableDeclaration()
{
    int VarType = -1;

    if (CurTok == tok_numbertype)
    {
        VarType = 0; // 0 = number
    }
    else if (CurTok == tok_stringtype)
    {
        VarType = 1; // 1 = string
    }
    else
    {
        return LogError("expected 'number' or 'string' type keyword");
    }

    gettok(); // eat type keyword

    if (CurTok != tok_identifier)
        return LogError("expected variable name");

    std::string VarName = IdentifierStr;
    gettok(); // eat variable name

    // Optional assignment
    std::unique_ptr<ExprAST> Init = nullptr;
    if (CurTok == tok_eqassign)
    {             // '=?'
        gettok(); // eat '=?'

        Init = ParseExpression();
        if (!Init)
            return nullptr;
    }

    // Register the variable's type globally
    VariableTypes[VarName] = VarType;

    // If there is an initializer, parse it as an assignment expression
    if (Init)
    {
        return std::make_unique<AssignExprAST>(VarName, std::move(Init));
    }

    // Otherwise, variable declared but uninitialized (just allocate space)
    return std::make_unique<VariableExprAST>(VarName);
}

std::unique_ptr<ExprAST> ParseRevealExpr()
{
    gettok(); // eat 'reveal'

    if (CurTok != tok_lparen)
        return LogError("expected '(' after 'reveal'");
    gettok(); // eat '('

    auto Arg = ParseExpression();
    if (!Arg)
        return nullptr;

    if (CurTok != tok_rparen)
        return LogError("expected ')' after argument to 'reveal'");
    gettok(); // eat ')'

    if (CurTok != tok_semi)
        return LogError("expected ';' after 'reveal' statement");
    gettok(); // eat ';'

    return std::make_unique<RevealExprAST>(std::move(Arg));
}
