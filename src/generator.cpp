// generator.cpp

#include "generator.h"
#include "parser.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include <iostream>
#include <cstring>
using namespace llvm;

// Global LLVM state
LLVMContext TheContext;

// Builder bound to that context
IRBuilder<> Builder(TheContext);

// A single module pointer and symbol table
std::unique_ptr<Module> TheModule;
std::map<std::string, Value *> NamedValues;

// Create printf function for reveal()
Function *CreatePrintfFunction(Module *M)
{
    std::vector<Type *> printf_arg_types;
    printf_arg_types.push_back(PointerType::getUnqual(Type::getInt8Ty(TheContext))); // char*

    FunctionType *printf_type = FunctionType::get(
        Type::getInt32Ty(TheContext), printf_arg_types, true);

    Function *func = Function::Create(
        printf_type, Function::ExternalLinkage, "printf", M);

    return func;
}

// 0 = number, 1 = string
extern "C" void gimme(void *ptr, int type)
{
    if (type == 0)
    {
        // number
        std::cin >> *reinterpret_cast<double *>(ptr);
    }
    else if (type == 1)
    {
        // string
        std::cin.ignore(); // ignore leftover newline from previous input
        std::cin.getline(reinterpret_cast<char *>(ptr), 256);
    }
    else
    {
        std::cerr << "gimme: Unknown type!\n";
    }
}

// Initialize built-in runtime
void InitializeRuntime(Module *M)
{
    CreatePrintfFunction(M);

    std::vector<llvm::Type *> gimme_args = {
        llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(TheContext)),
        llvm::Type::getInt32Ty(TheContext)    // int (type)
    };

    llvm::FunctionType *gimme_type = llvm::FunctionType::get(
        llvm::Type::getVoidTy(TheContext),
        gimme_args,
        false);

    llvm::Function::Create(
        gimme_type,
        llvm::Function::ExternalLinkage,
        "gimme",
        M);
}
