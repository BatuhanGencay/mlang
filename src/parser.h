#pragma once
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <map>

#include "ast.h"

extern std::map<std::string, int> VariableTypes;
// Operator precedence map
extern std::map<int, int> BinopPrecedence;

// Initialize binary operator precedences
void InitBinopPrecedence();

llvm::Value* LogErrorV(const char* Str);

// Primary parsing routines
std::unique_ptr<ExprAST> ParseExpression();
std::unique_ptr<ExprAST> ParsePrimary();
std::unique_ptr<ExprAST> ParseUnary();
std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS);
std::unique_ptr<ExprAST> ParseIfExpr();
std::unique_ptr<ExprAST> ParseWhileExpr();
std::unique_ptr<ExprAST> ParseForExpr();
std::unique_ptr<ExprAST> ParseReturnExpr();
std::unique_ptr<ExprAST> ParseVariableDeclaration();
std::unique_ptr<ExprAST> ParseRevealExpr();
// Block and definition parsing
std::unique_ptr<ExprAST> ParseBlock();
std::unique_ptr<FunctionAST> ParseDefinition();
std::unique_ptr<ExprAST> ParseTopLevelExpr();


