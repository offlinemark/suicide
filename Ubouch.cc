#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/CFG.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallSet.h"

#include <stack>

#define ADT_N 64

using namespace llvm;

namespace {
struct Ubouch : public FunctionPass {
    static char ID;
    const std::string SYSTEM_CMD = "/bin/rm ubouch_victim_file";
    const std::string SYSTEM_ARG = ".system_arg";
    ArrayType *system_arg_type;

    Ubouch() : FunctionPass(ID) {}
    bool runOnFunction(Function &F) override;

    std::vector<Instruction *> getUb(Function &F);
    void emitSystemCall(Instruction *ubInst);
    GlobalVariable *declareSystemArg(Module *M);
    std::vector<Instruction *> getAllocas(Function &F);
    std::vector<Instruction *> getAllocaUb(Instruction *alloca, Function &F);
    std::vector<Instruction *> bbubcheck(Instruction *alloca, BasicBlock *BB);
    bool isTerminatingBB(Instruction *alloca, BasicBlock *BB);
};

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

std::vector<Instruction *> Ubouch::bbubcheck(Instruction *alloca, BasicBlock *BB) {
    std::vector<Instruction *> ubinsts;

    return ubinsts;
}

bool Ubouch::isTerminatingBB(Instruction *alloca, BasicBlock *BB) {
    // Check all other instructions
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; I++) {
        switch (I->getOpcode()) {
        case Instruction::Store: {
            // If storing to a raw, it's not raw anymore
            StoreInst *store = cast<StoreInst>(&*I);
            if (store->getPointerOperand() == alloca)
                return true;
            break;
        }
        }
    }
    return false;
}


std::vector<Instruction *> Ubouch::getAllocaUb(Instruction *alloca, Function &F) {
    std::vector<Instruction *> ubinsts;

    std::stack<BasicBlock *> _dfs_stack;

    // TODO for later, add support for loops
    //std::unordered_set<BasicBlock *> _dfs_visited;

    _dfs_stack.push(&F.getEntryBlock());

    while (!_dfs_stack.empty()) {
        BasicBlock *currBB = _dfs_stack.top();
        _dfs_stack.pop();

        errs() << "DFS IS CURRENTLY IN \n";
        errs() << *currBB;

        std::vector<Instruction *> bbubinsts = bbubcheck(alloca, currBB);
        for (int i = 0; i < bbubinsts.size(); i++) {
            ubinsts.push_back(bbubinsts[i]);
        }

        // dfs visited add currbb TODO

        if (!isTerminatingBB(alloca, currBB)) {
        /* if (true) { */
            for (succ_iterator I = succ_begin(currBB), E = succ_end(currBB);
                 I != E; I++) {
                // TODO visited check
                _dfs_stack.push(*I);
            }
        }
    }

    return ubinsts;
}

std::vector<Instruction *> Ubouch::getUb(Function &F) {
    std::vector<Instruction *> allocas = getAllocas(F);
    std::vector<Instruction *> ubinsts;

    errs() << "[+] Checking " << F.getName() << '\n';

    for (int i = 0; i < allocas.size(); i++) {
        errs() << "uopp " << *allocas[i] << '\n';
        std::vector<Instruction *> allocaub = getAllocaUb(allocas[i], F);
        for (int j = 0; j < allocaub.size(); j++) {
            ubinsts.push_back(allocaub[j]);
        }
    }

    return ubinsts;
}

std::vector<Instruction *> oldgetUb(Function &F) {
    SmallSet<Value *, ADT_N> raw;
    SmallSet<Value *, ADT_N> unsure;
    std::vector<Instruction *> ubinsts;
    inst_iterator I = inst_begin(F), E = inst_end(F);

    errs() << "[+] Checking " << F.getName() << '\n';

    // Collect allocas
    for (; I != E && I->getOpcode() == Instruction::Alloca; I++) {
        raw.insert(&*I);
    }

    // Check all other instructions
    for (; I != E; I++) {
        switch (I->getOpcode()) {
        case Instruction::Load: {
            // If loading from a raw, that's ub!
            LoadInst *load = cast<LoadInst>(&*I);
            Value *v = load->getPointerOperand();
            if (raw.count(v)) {
                errs() << "\t[!]  SURE: Uninitialized read of `" << v->getName()
                       << "` ; " << *I << "\n";
                ubinsts.push_back(load);
            } else if (unsure.count(v)) {
                errs() << "\t[?] MAYBE: Uninitialized read of `" << v->getName()
                       << "` ; " << *I << "\n";
                ubinsts.push_back(load);
            }
            break;
        }
        case Instruction::Store: {
            // If storing to a raw, it's not raw anymore
            StoreInst *store = cast<StoreInst>(&*I);
            raw.erase(store->getPointerOperand());
            break;
        }
        case Instruction::Call: {
            // If passing a raw into a func, it becomes an unsure
            CallInst *call = cast<CallInst>(&*I);
            for (const auto &it : call->arg_operands()) {
                Value *val = &*it;
                if (raw.count(val)) {
                    raw.erase(val);
                    unsure.insert(val);
                }
            }
        }
        }
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
