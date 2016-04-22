#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallSet.h"

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

std::vector<Instruction *> Ubouch::getUb(Function &F) {
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
