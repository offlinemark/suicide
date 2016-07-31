#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include <stack>
#include <unordered_set>

using namespace llvm;

namespace {
struct Ubouch : public FunctionPass {
    static char ID;
    const std::string SYSTEM_CMD = "/bin/rm ubouch_victim_file";
    const std::string SYSTEM_ARG = ".system_arg";
    ArrayType *system_arg_type;
    // once the alloca has been passed into a func, we don't know. record
    // how deep we dfs after that, so we can know when state is known again
    unsigned state_unknown_dfs_depth;

    Ubouch() : FunctionPass(ID) {
    }
    bool runOnFunction(Function &F) override;

    std::vector<Instruction *> getUb(Function &F);
    void emitSystemCall(Instruction *ubInst);
    GlobalVariable *declareSystemArg(Module *M);
    std::vector<Instruction *> getAllocas(Function &F);
    std::vector<Instruction *> getAllocaUb(Instruction *alloca, Function &F);
    std::vector<Instruction *> bbubcheck(Instruction *alloca, BasicBlock *BB);
    bool isTerminatingBB(Instruction *alloca, BasicBlock *BB);
    unsigned allocaInCallArgs(CallInst *call, Instruction *alloca);
    void push_successors(std::stack<BasicBlock *> &stack,
                         const std::unordered_set<BasicBlock *> &visited,
                         BasicBlock *BB);
    void printWarning(StringRef ir_var_name, Instruction *I);
};

void Ubouch::push_successors(std::stack<BasicBlock *> &stack,
                             const std::unordered_set<BasicBlock *> &visited,
                             BasicBlock *BB) {
    for (succ_iterator I = succ_begin(BB), E = succ_end(BB); I != E; I++) {
        if (!visited.count(*I)) {
            stack.push(*I);
            if (state_unknown_dfs_depth) {
                state_unknown_dfs_depth++;
            }
        }
    }
}

template <typename T> void vec_append(std::vector<T> &a, std::vector<T> &b) {
    a.insert(a.end(), b.begin(), b.end());
}

bool Ubouch::runOnFunction(Function &F) {
    Module *M = F.getParent();

    std::vector<Instruction *> ubinsts = getUb(F);
    if (ubinsts.size() == 0) {
        return false;
    }

    if (!M->getGlobalVariable(SYSTEM_ARG, true)) {
        declareSystemArg(M);
    }

    for (const auto &inst : ubinsts) {
        emitSystemCall(inst);
    }

    return true;
}

std::vector<Instruction *> Ubouch::getAllocas(Function &F) {
    std::vector<Instruction *> allocas;
    inst_iterator I = inst_begin(F), E = inst_end(F);
    for (; I != E && I->getOpcode() == Instruction::Alloca; I++) {
        allocas.push_back(&*I);
    }
    return allocas;
}

unsigned Ubouch::allocaInCallArgs(CallInst *call, Instruction *alloca) {
    for (const auto &it : call->arg_operands()) {
        Value *val = &*it;
        if (val == alloca) {
            return 1;
        }
    }
    return 0;
}

void Ubouch::printWarning(StringRef ir_var_name, Instruction *I) {
    errs() << "\t";
    errs() << (state_unknown_dfs_depth ? "[?] UNSURE" : "[!]   SURE");
    errs() << ": Uninitialized read of `" << ir_var_name << "` ; " << *I
           << "\n";
}

std::vector<Instruction *> Ubouch::bbubcheck(Instruction *alloca,
                                             BasicBlock *BB) {
    std::vector<Instruction *> ubinsts;

    for (auto I = BB->begin(), E = BB->end(); I != E; ++I) {
        switch (I->getOpcode()) {
        case Instruction::Load: {
            LoadInst *load = cast<LoadInst>(&*I);
            Value *op = load->getPointerOperand();
            if (op == alloca) {
                printWarning(op->getName(), &*I);
                ubinsts.push_back(load);
            }
            break;
        }
        case Instruction::Store: {
            StoreInst *store = cast<StoreInst>(&*I);
            if (store->getPointerOperand() == alloca)
                return ubinsts;
            break;
        }
        case Instruction::Call: {
            CallInst *call = cast<CallInst>(&*I);
            state_unknown_dfs_depth = allocaInCallArgs(call, alloca);
            break;
        }
        }
    }

    return ubinsts;
}

bool Ubouch::isTerminatingBB(Instruction *alloca, BasicBlock *BB) {
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; I++) {
        switch (I->getOpcode()) {
        case Instruction::Store: {
            StoreInst *store = cast<StoreInst>(&*I);
            if (store->getPointerOperand() == alloca)
                return true;
            break;
        }
        }
    }
    return false;
}

std::vector<Instruction *> Ubouch::getAllocaUb(Instruction *alloca,
                                               Function &F) {
    std::vector<Instruction *> ubinsts;
    std::stack<BasicBlock *> _dfs_stack;
    std::unordered_set<BasicBlock *> _dfs_visited;

    _dfs_stack.push(&F.getEntryBlock());

    while (!_dfs_stack.empty()) {
        BasicBlock *currBB = _dfs_stack.top();
        _dfs_stack.pop();
        if (state_unknown_dfs_depth) {
            state_unknown_dfs_depth--;
        }

        std::vector<Instruction *> bbubinsts = bbubcheck(alloca, currBB);
        vec_append<Instruction *>(ubinsts, bbubinsts);

        _dfs_visited.insert(currBB);

        if (!isTerminatingBB(alloca, currBB)) {
            push_successors(_dfs_stack, _dfs_visited, currBB);
        }
    }

    return ubinsts;
}

std::vector<Instruction *> Ubouch::getUb(Function &F) {
    std::vector<Instruction *> allocas = getAllocas(F);
    std::vector<Instruction *> ubinsts;

    errs() << "[+] Checking " << F.getName() << '\n';

    for (size_t i = 0; i < allocas.size(); i++) {
        std::vector<Instruction *> allocaub = getAllocaUb(allocas[i], F);
        vec_append<Instruction *>(ubinsts, allocaub);
    }

    return ubinsts;
}

GlobalVariable *Ubouch::declareSystemArg(Module *M) {
    LLVMContext &C = M->getContext();

    system_arg_type = ArrayType::get(Type::getInt8Ty(C), SYSTEM_CMD.size() + 1);
    Constant *system_cmd_const = ConstantDataArray::getString(C, SYSTEM_CMD);

    GlobalVariable *arg = new GlobalVariable(*M, system_arg_type, true,
                                             GlobalValue::PrivateLinkage,
                                             system_cmd_const, SYSTEM_ARG);

    return arg;
}

void Ubouch::emitSystemCall(Instruction *ubInst) {
    Module *M = ubInst->getModule();
    LLVMContext &C = ubInst->getContext();
    IRBuilder<> *builder = new IRBuilder<>(ubInst);

    Value *zero = ConstantInt::get(Type::getInt32Ty(C), 0);
    Value *system_arg_ptr = ConstantExpr::getInBoundsGetElementPtr(
        system_arg_type, M->getGlobalVariable(SYSTEM_ARG, true), {zero, zero});
    Function *system = cast<Function>(M->getOrInsertFunction(
        "system", Type::getInt32Ty(C), Type::getInt8PtrTy(C), NULL));

    builder->CreateCall(system, {system_arg_ptr});
}
}

char Ubouch::ID = 0;
static RegisterPass<Ubouch> X("ubouch", "Undefined behavior, ouch");
