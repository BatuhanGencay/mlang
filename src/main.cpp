// main.cpp (FINAL FINAL FIXED for modern LLVM with full token support)

#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "generator.h"

#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <map>
#include <vector>
#include <string>

using namespace llvm;
using namespace llvm::orc;

static ExitOnError ExitOnErr;
static std::unique_ptr<LLJIT> TheJIT;
static std::shared_ptr<LLVMContext> SharedContext;

static void InitializeModule() {
    std::cerr << "[DEBUG] Entered InitializeModule()\n";
    SharedContext = std::make_shared<LLVMContext>();
    new (&Builder) IRBuilder<>(*SharedContext);
    TheModule = std::make_unique<Module>("might", *SharedContext);
    NamedValues.clear();
    InitializeRuntime(TheModule.get());
}

static void HandleTopLevelExpression() {
    std::cerr << "[DEBUG] Entered HandleTopLevelExpression()\n";
    auto AST = ParseExpression();
    if (!AST) {
        std::cerr << "[error] ParseExpression() returned null!\n";
        gettok(); // recover
        return;
    }

    auto Proto = std::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>{});
    auto FnAST = std::make_unique<FunctionAST>(std::move(Proto), std::move(AST));

    auto NewCtx = std::make_unique<LLVMContext>();
    auto M = std::make_unique<Module>("might", *NewCtx);
    InitializeRuntime(M.get());

    auto *OldModule = TheModule.release();
    TheModule.reset(M.get());
    new (&Builder) IRBuilder<>(*NewCtx);

    if (Function *FnIR = FnAST->codegen()) {
        ThreadSafeModule TSM(std::unique_ptr<Module>(M.release()), std::move(NewCtx));
        ExitOnErr(TheJIT->addIRModule(std::move(TSM)));

        if (auto SymOrErr = TheJIT->lookup("__anon_expr")) {
            auto Sym = *SymOrErr;
            auto FP = (double (*)())Sym.toPtr<void *>();
            double Result = FP();
            std::cout << "Result = " << Result << "\n";
        } else {
            std::cerr << "[error] Failed to lookup __anon_expr!\n";
        }
    } else {
        std::cerr << "[error] Codegen failed for top-level expression.\n";
    }

    TheModule.reset(OldModule);
    new (&Builder) IRBuilder<>(TheContext);
    InitializeModule();
}

static void MainLoop() {
    while (true) {
        // std::cerr << "[debug] Token " << CurTok << "\n";
        if (CurTok == tok_eof) break;

        if (CurTok == tok_semi) {
            gettok();
            continue;
        }

        if (CurTok == tok_attempt) {
            if (auto FnAST = ParseDefinition()) {
                if (auto *FnIR = FnAST->codegen()) {
                    llvm::orc::ThreadSafeContext TSCtx(std::make_unique<LLVMContext>());
                    auto TSM = ThreadSafeModule(std::move(TheModule), TSCtx);
                    ExitOnErr(TheJIT->addIRModule(std::move(TSM)));
                    TheModule = std::make_unique<Module>("might", *TSCtx.getContext());
                    NamedValues.clear();
                } else {
                    std::cerr << "[error] Codegen failed, FnIR is null!\n";
                }
            } else {
                std::cerr << "[error] ParseDefinition() returned null!\n";
            }
        }
    }

    // Call 'maybe' entry point
    std::cerr << "[debug] Looking up 'maybe' entry point...\n";
    if (auto SymOrErr = TheJIT->lookup("maybe")) {
        auto Sym = *SymOrErr;
        auto FP = (int (*)())Sym.toPtr<void *>();
        int Result = FP();
        std::cout << "[info] maybe() returned " << Result << "\n";
    } else {
        std::cerr << "[error] Entry point 'maybe' not found!\n";
    }
}

int main(int argc, char **argv) {
    std::cerr << "[DEBUG] Entered main()\n";
    std::string src;

    if (argc > 1) {
        std::ifstream in(argv[1]);
        if (!in) {
            perror(argv[1]);
            return 1;
        }
        if (in.peek() == std::ifstream::traits_type::eof()) {
            std::cerr << "[error] Input file is empty!\n";
        }
        src.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    } else {
        src.assign(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
    }

    setInput(src);
    std::cerr << "[DEBUG] Loaded input file content:\n" << src << "\n";

    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
    InitializeModule();

    TheJIT = ExitOnErr(LLJITBuilder().create());

    InitBinopPrecedence();
    gettok();
    std::cerr << "[DEBUG] First call: " << CurTok << "\n";

    MainLoop();
    return 0;
}
