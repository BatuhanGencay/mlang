// ast.h
#pragma once

#include <string>
#include <memory>
#include <vector>

namespace llvm {
    class Value;
    class Function;
}

//===----------------------------------------------------------------------===//
// Base class for all AST nodes
//===----------------------------------------------------------------------===//
class ExprAST {
public:
    virtual ~ExprAST() = default;
    virtual llvm::Value* codegen() = 0;
};

//===----------------------------------------------------------------------===//
// Numeric literal
//===----------------------------------------------------------------------===//
class NumberExprAST : public ExprAST {
    double Val;
public:
    NumberExprAST(double Val) : Val(Val) {}
    llvm::Value* codegen() override;
};

//===----------------------------------------------------------------------===//
// Variable reference
//===----------------------------------------------------------------------===//
class VariableExprAST : public ExprAST {
    std::string Name;
public:

    VariableExprAST(const std::string &Name) : Name(Name) {}
    const std::string& getName() const { return Name; }
    llvm::Value* codegen() override;
};

//===----------------------------------------------------------------------===//
// String literal
//===----------------------------------------------------------------------===//
class StringExprAST : public ExprAST {
    std::string Val;
public:
    StringExprAST(const std::string &Val) : Val(Val) {}
    llvm::Value* codegen() override;
};

//===----------------------------------------------------------------------===//
// Unary operator
//===----------------------------------------------------------------------===//
class UnaryExprAST : public ExprAST {
    char Opcode;
    std::unique_ptr<ExprAST> Operand;
public:
    UnaryExprAST(char Opcode, std::unique_ptr<ExprAST> Operand)
        : Opcode(Opcode), Operand(std::move(Operand)) {}
    llvm::Value* codegen() override;
};

//===----------------------------------------------------------------------===//
// Binary operator
//===----------------------------------------------------------------------===//
class BinaryExprAST : public ExprAST {
    int Op;
    std::unique_ptr<ExprAST> LHS, RHS;
public:
    BinaryExprAST(int Op,
                  std::unique_ptr<ExprAST> LHS,
                  std::unique_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
    llvm::Value* codegen() override;
};

//===----------------------------------------------------------------------===//
// Assignment
//===----------------------------------------------------------------------===//
class AssignExprAST : public ExprAST {
    std::string VarName;
    std::unique_ptr<ExprAST> ValueExpr;
public:
    AssignExprAST(const std::string &VarName,
                  std::unique_ptr<ExprAST> ValueExpr)
        : VarName(VarName), ValueExpr(std::move(ValueExpr)) {}
    llvm::Value* codegen() override;
};

//===----------------------------------------------------------------------===//
// Function call
//===----------------------------------------------------------------------===//
class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;
public:
    CallExprAST(const std::string &Callee,
                std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}
    llvm::Value* codegen() override;
};

//===----------------------------------------------------------------------===//
// Block: a sequence of statements
//===----------------------------------------------------------------------===//
class BlockExprAST : public ExprAST {
    std::vector<std::unique_ptr<ExprAST>> Statements;
public:
    BlockExprAST(std::vector<std::unique_ptr<ExprAST>> Stmts)
        : Statements(std::move(Stmts)) {}
    llvm::Value* codegen() override;
};

//===----------------------------------------------------------------------===//
// Function prototype
//===----------------------------------------------------------------------===//
class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;
public:
    PrototypeAST(const std::string &Name,
                 std::vector<std::string> Args)
        : Name(Name), Args(std::move(Args)) {}
    const std::string &getName() const { return Name; }
    llvm::Function* codegen();
};

//===----------------------------------------------------------------------===//
// Function definition
//===----------------------------------------------------------------------===//
class FunctionAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;
public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                std::unique_ptr<ExprAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}
    llvm::Function* codegen();
};

class IfExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Cond;
    std::unique_ptr<ExprAST> Then;
    std::unique_ptr<ExprAST> Else;
public:
    IfExprAST(std::unique_ptr<ExprAST> Cond,
              std::unique_ptr<ExprAST> Then,
              std::unique_ptr<ExprAST> Else)
        : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

    llvm::Value* codegen() override;
};

class WhileExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Cond;
    std::unique_ptr<ExprAST> Body;
public:
    WhileExprAST(std::unique_ptr<ExprAST> Cond,
                 std::unique_ptr<ExprAST> Body)
        : Cond(std::move(Cond)), Body(std::move(Body)) {}

    llvm::Value* codegen() override;
};

//===----------------------------------------------------------------------===//
// Return Expression
//===----------------------------------------------------------------------===//
class ReturnExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Expr;
public:
    ReturnExprAST(std::unique_ptr<ExprAST> Expr)
        : Expr(std::move(Expr)) {}

    llvm::Value* codegen() override;
};

class RevealExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Arg;

public:
    RevealExprAST(std::unique_ptr<ExprAST> Arg)
        : Arg(std::move(Arg)) {}

    llvm::Value* codegen() override;
};
