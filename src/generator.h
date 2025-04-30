// generator.h (updated)
#pragma once
#include <iostream>
#include <memory>
#include <map>
#include "llvm/IR/IRBuilder.h"

namespace llvm {
    class Module;
}

// Expose the global IRBuilder from generator.cpp
extern llvm::LLVMContext             TheContext;
extern llvm::IRBuilder<>             Builder;
extern std::unique_ptr<llvm::Module> TheModule;
extern std::map<std::string, llvm::Value*> NamedValues;
// Initialize built-in runtime (e.g., printf declaration)
void InitializeRuntime(llvm::Module *M);