// ast.cpp

#include "ast.h"
#include "generator.h"
#include "parser.h"
#include "lexer.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Constants.h"
#include <map>
#include <memory>

using namespace llvm;

// External state from generator.cpp
extern llvm::LLVMContext TheContext;
extern llvm::IRBuilder<> Builder;
extern std::unique_ptr<llvm::Module> TheModule;
extern std::map<std::string, llvm::Value *> NamedValues;

//===----------------------------------------------------------------------===//
// NumberExprAST
//===----------------------------------------------------------------------===//

Value *NumberExprAST::codegen()
{
    return ConstantFP::get(Type::getDoubleTy(TheContext), Val);
}

//===----------------------------------------------------------------------===//
// VariableExprAST
//===----------------------------------------------------------------------===//

llvm::Value *VariableExprAST::codegen()
{
    llvm::Value *V = NamedValues[Name];
    if (!V)
    {
        std::cerr << "Unknown variable name: " << Name << "\n";
        return nullptr;
    }

    // Lookup type
    int VarType = VariableTypes[Name];

    if (VarType == 0)
    {
        // number: load double
        return Builder.CreateLoad(Type::getDoubleTy(TheContext), V, Name.c_str());
    }
    else if (VarType == 1)
    {
        // string: load pointer to string (char array)
        return Builder.CreateLoad(ArrayType::get(Type::getInt8Ty(TheContext), 256), V, Name.c_str());
    }
    else
    {
        std::cerr << "Unknown type for variable: " << Name << "\n";
        return nullptr;
    }
}

//===----------------------------------------------------------------------===//
// StringExprAST
//===----------------------------------------------------------------------===//

llvm::Value* StringExprAST::codegen() {
    return Builder.CreateGlobalStringPtr(Val);
}

//===----------------------------------------------------------------------===//
// UnaryExprAST
//===----------------------------------------------------------------------===//

Value *UnaryExprAST::codegen()
{
    Value *OperandV = Operand->codegen();
    if (!OperandV)
        return nullptr;

    switch (Opcode)
    {
    case '-':
        return Builder.CreateFNeg(OperandV, "negtmp");
    case '!':
    {
        Value *Cmp = Builder.CreateFCmpUEQ(
            OperandV,
            ConstantFP::get(TheContext, APFloat(0.0)),
            "nottmp");
        return Builder.CreateUIToFP(Cmp, Type::getDoubleTy(TheContext));
    }
    default:
        return nullptr;
    }
}

//===----------------------------------------------------------------------===//
// BinaryExprAST
//===----------------------------------------------------------------------===//

Value *BinaryExprAST::codegen()
{
    Value *L = LHS->codegen();
    Value *R = RHS->codegen();
    if (!L || !R)
        return nullptr;

    switch (Op)
    {
    case tok_plus:
        return Builder.CreateFAdd(L, R, "addtmp");
    case tok_minus:
        return Builder.CreateFSub(L, R, "subtmp");
    case tok_mul:
        return Builder.CreateFMul(L, R, "multmp");
    case tok_div:
        return Builder.CreateFDiv(L, R, "divtmp");
    case tok_lt:
    {
        Value *Cmp = Builder.CreateFCmpULT(L, R, "lttmp");
        return Builder.CreateUIToFP(Cmp, Type::getDoubleTy(TheContext));
    }
    case tok_gt:
    {
        Value *Cmp = Builder.CreateFCmpUGT(L, R, "gttmp");
        return Builder.CreateUIToFP(Cmp, Type::getDoubleTy(TheContext));
    }
    case tok_eq:
    {
        Value *Cmp = Builder.CreateFCmpUEQ(L, R, "eqtmp");
        return Builder.CreateUIToFP(Cmp, Type::getDoubleTy(TheContext));
    }
    case tok_ne:
    {
        Value *Cmp = Builder.CreateFCmpUNE(L, R, "netmp");
        return Builder.CreateUIToFP(Cmp, Type::getDoubleTy(TheContext));
    }
    case tok_le:
    {
        Value *Cmp = Builder.CreateFCmpULE(L, R, "letmp");
        return Builder.CreateUIToFP(Cmp, Type::getDoubleTy(TheContext));
    }
    case tok_ge:
    {
        Value *Cmp = Builder.CreateFCmpUGE(L, R, "getmp");
        return Builder.CreateUIToFP(Cmp, Type::getDoubleTy(TheContext));
    }
    default:
        return nullptr;
    }
}

//===----------------------------------------------------------------------===//
// AssignExprAST
//===----------------------------------------------------------------------===//

llvm::Value *AssignExprAST::codegen()
{
    // Generate the value to assign
    llvm::Value *Val = ValueExpr->codegen();
    if (!Val)
        return nullptr;

    // Lookup variable type
    auto It = VariableTypes.find(VarName);
    if (It == VariableTypes.end())
    {
        std::cerr << "Unknown variable type for " << VarName << "\n";
        return nullptr;
    }
    int VarType = It->second;

    llvm::Value *Variable = NamedValues[VarName];

    if (!Variable)
    {
        // Allocate space based on variable type
        if (VarType == 0)
        { // number
            Variable = Builder.CreateAlloca(Type::getDoubleTy(TheContext), nullptr, VarName.c_str());
        }
        else if (VarType == 1)
        { // string
            Variable = Builder.CreateAlloca(ArrayType::get(Type::getInt8Ty(TheContext), 256), nullptr, VarName.c_str());
        }
        NamedValues[VarName] = Variable;
    }

    if (VarType == 0)
    {
        // Store double
        Builder.CreateStore(Val, Variable);
    }
    else if (VarType == 1)
    {
        // Val should be a pointer to a string (for now assume string literals only)
        Builder.CreateStore(Val, Variable);
    }

    return Val;
}

//===----------------------------------------------------------------------===//
// CallExprAST
//===----------------------------------------------------------------------===//

Value *CallExprAST::codegen()
{
    Function *CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF)
        return nullptr;
    if (Callee == "gimme")
    {
        if (Args.size() != 1)
            return LogErrorV("gimme expects exactly one argument");

        auto &ArgName = dynamic_cast<VariableExprAST &>(*Args[0]).getName();

        llvm::Value *Var = NamedValues[ArgName];
        if (!Var)
            return LogErrorV("Unknown variable name passed to gimme");

        int VarType = VariableTypes[ArgName];

        // Cast the pointer to i8*
        llvm::Value* CastedVar = Builder.CreateBitCast(Var, llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(TheContext)));

        llvm::Function *GimmeFunc = TheModule->getFunction("gimme");
        if (!GimmeFunc)
            return LogErrorV("gimme function not found");

        return Builder.CreateCall(GimmeFunc, {CastedVar, Builder.getInt32(VarType)});
    }
    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i)
    {
        Value *ArgV = Args[i]->codegen();
        if (!ArgV)
            return nullptr;
        ArgsV.push_back(ArgV);
    }

    return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

//===----------------------------------------------------------------------===//
// BlockExprAST
//===----------------------------------------------------------------------===//

llvm::Value* BlockExprAST::codegen() {
    llvm::Value* Last = nullptr;
    for (auto& Stmt : Statements) {
        Last = Stmt->codegen();
        if (!Last)
            return nullptr; // if any statement fails
    }
    return Last; // return the last statement's result
}


//===----------------------------------------------------------------------===//
// PrototypeAST
//===----------------------------------------------------------------------===//

Function *PrototypeAST::codegen()
{
    std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(TheContext));
    FunctionType *FT = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);
    Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());
    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName(Args[Idx++]);
    return F;
}

//===----------------------------------------------------------------------===//
// FunctionAST
//===----------------------------------------------------------------------===//

Function *FunctionAST::codegen()
{
    Function *F = TheModule->getFunction(Proto->getName());
    if (!F)
        F = Proto->codegen();
    if (!F)
        return nullptr;

    BasicBlock *BB = BasicBlock::Create(TheContext, "entry", F);
    Builder.SetInsertPoint(BB);

    NamedValues.clear();
    for (auto &Arg : F->args())
    {
        AllocaInst *Alloca = Builder.CreateAlloca(Type::getDoubleTy(TheContext), nullptr, Arg.getName());
        Builder.CreateStore(&Arg, Alloca);
        NamedValues[Arg.getName().str()] = Alloca;
    }

    if (Value *RetVal = Body->codegen())
    {
        Builder.CreateRet(RetVal);
        verifyFunction(*F);
        return F;
    }

    F->eraseFromParent();
    return nullptr;
}

llvm::Value *IfExprAST::codegen()
{
    llvm::Value *CondV = Cond->codegen();
    if (!CondV)
        return nullptr;

    // Convert condition to bool
    CondV = Builder.CreateFCmpONE(
        CondV, llvm::ConstantFP::get(TheContext, llvm::APFloat(0.0)), "ifcond");

    llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

    llvm::BasicBlock *ThenBB = llvm::BasicBlock::Create(TheContext, "then", TheFunction);
    llvm::BasicBlock* ElseBB = llvm::BasicBlock::Create(TheContext, "else", TheFunction);
    llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(TheContext, "ifcont", TheFunction);


    Builder.CreateCondBr(CondV, ThenBB, ElseBB);

    // Emit Then block
    Builder.SetInsertPoint(ThenBB);
    llvm::Value *ThenV = Then->codegen();
    if (!ThenV)
        return nullptr;
    Builder.CreateBr(MergeBB);

    // Codegen of 'Then' can change the current block, update ThenBB
    ThenBB = Builder.GetInsertBlock();

    // Emit Else block
    Builder.SetInsertPoint(ElseBB);
    llvm::Value *ElseV = Else->codegen();
    if (!ElseV)
        return nullptr;
    Builder.CreateBr(MergeBB);

    // Codegen of 'Else' can change the current block, update ElseBB
    ElseBB = Builder.GetInsertBlock();

    // Emit Merge block
    Builder.SetInsertPoint(MergeBB);

    return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(TheContext));
}

llvm::Value *WhileExprAST::codegen()
{
    llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

    llvm::BasicBlock *CondBB = llvm::BasicBlock::Create(TheContext, "cond", TheFunction);
    llvm::BasicBlock* LoopBB = llvm::BasicBlock::Create(TheContext, "loop", TheFunction);
    llvm::BasicBlock *AfterBB = llvm::BasicBlock::Create(TheContext, "afterloop",TheFunction);

    // Branch to condition check first
    Builder.CreateBr(CondBB);

    Builder.SetInsertPoint(CondBB);

    llvm::Value *CondV = Cond->codegen();
    if (!CondV)
        return nullptr;

    CondV = Builder.CreateFCmpONE(
        CondV, llvm::ConstantFP::get(TheContext, llvm::APFloat(0.0)), "loopcond");

    Builder.CreateCondBr(CondV, LoopBB, AfterBB);

    // Emit loop body
    Builder.SetInsertPoint(LoopBB);

    if (!Body->codegen())
        return nullptr;

    Builder.CreateBr(CondBB);

    // Emit after-loop block
    Builder.SetInsertPoint(AfterBB);

    return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(TheContext));
}

llvm::Value *ReturnExprAST::codegen()
{
    llvm::Value *RetVal = Expr->codegen();
    if (!RetVal)
        return nullptr;
    Builder.CreateRet(RetVal);
    return RetVal;
}

llvm::Value* RevealExprAST::codegen() {
    llvm::Value* ArgV = Arg->codegen();
    if (!ArgV)
        return nullptr;

    llvm::Function* PrintfFunc = TheModule->getFunction("printf");
    if (!PrintfFunc)
        return LogErrorV("printf function not found!");

    // Create a format string: "%s\n"
    llvm::Value* FormatStr = Builder.CreateGlobalStringPtr("%s\n");

    return Builder.CreateCall(PrintfFunc, { FormatStr, ArgV });
}
