// this class includes all instrumenters


#ifndef INSTRUMENTERS_H_
#define INSTRUMENTERS_H_

#include "Common.h"
#include "Instrumenter.h"
#include "BinaryOperatorInstrumenter.h"
#include <llvm/Analysis/Verifier.h>

/*******************************************************************************************/

#define INSTR_TO_CALLBACK(inst)		("llvm_" inst)

/*******************************************************************************************/

// ***** Memory Access and Addressing Operations ***** //

// Callback: void alloca()
class AllocaInstrumenter : public Instrumenter {
  public:
    AllocaInstrumenter(std::string name, Instrumentation* instrumentation) :
      Instrumenter(name, instrumentation) {};

    bool CheckAndInstrument(Instruction* I) {
      CAST_OR_RETURN(AllocaInst, SI, I);

      safe_assert(parent_ != NULL);

      count_++;

      InstrPtrVector Instrs;

      Constant* C_iid = IID_CONSTANT(SI);

      Type *T = SI->getAllocatedType();
      if (!T) return false;
      KIND kind = TypeToKind(T);

      // if unsupported kind, return false
      if (kind == INV_KIND) return false;

      Constant* size;
      if (T->isArrayTy()) {
        ArrayType* aType = (ArrayType*) T;
        size = INT64_CONSTANT(aType->getNumElements(), UNSIGNED);
      } else {
        size = INT64_CONSTANT(0, UNSIGNED);
      }

      Constant* C_kind = KIND_CONSTANT(kind);

      Instruction* call = CALL_IID_KIND_INT64_INT(INSTR_TO_CALLBACK("allocax"), C_iid, C_kind, size, computeIndex(SI));
      Instrs.push_back(call);

      // instrument
      InsertAllBefore(Instrs, SI);

      return true;
    }
};


// Callback: void store(IID iid, PTR addr, KVALUE value)
class StoreInstrumenter : public Instrumenter {
  public:
    StoreInstrumenter(std::string name, Instrumentation* instrumentation) :
      Instrumenter(name, instrumentation) {};

    bool CheckAndInstrument(Instruction* I) {
      CAST_OR_RETURN(StoreInst, SI, I);

      safe_assert(parent_ != NULL);

      count_++;

      parent_->AS_ = SI->getPointerAddressSpace();

      InstrPtrVector Instrs;
      Value* kvalue = KVALUE_VALUE(SI->getValueOperand(), Instrs, NOSIGN);
      if(kvalue == NULL) {
        return false; 
      }

      Constant* C_iid = IID_CONSTANT(SI);
      Instruction* I_cast_ptr = PTR_CAST_INSTR(SI->getPointerOperand());
      Instrs.push_back(I_cast_ptr);

      // cuong: pass a kvalue instead of pointer value
      Value* op = KVALUE_VALUE(SI->getPointerOperand(), Instrs, NOSIGN);
      
      // new: iid for ptr
      // Instruction *ptr_iid = (Instruction*)(SI->getPointerOperand());
      // Constant* C_ptr_iid = IID_CONSTANT(ptr_iid);

      Instruction* call = CALL_IID_KVALUE_KVALUE_INT(INSTR_TO_CALLBACK("store"), C_iid, op, kvalue, computeIndex(SI));
      Instrs.push_back(call);

      // instrument
      InsertAllBefore(Instrs, I);

      return true;
    }
};

// Callback: void fence()
class FenceInstrumenter : public Instrumenter {
  public:
    FenceInstrumenter(std::string name, Instrumentation* instrumentation) :
      Instrumenter(name, instrumentation) {};

    bool CheckAndInstrument(Instruction* I) {
      CAST_OR_RETURN(FenceInst, SI, I);

      safe_assert(parent_ != NULL);

      count_++;

      Instruction *call = CallInst::Create(parent_->M_->getOrInsertFunction(StringRef("llvm_fence"), FunctionType::get(VOID_TYPE(), false)));
      call->insertBefore(I);

      return true;
    }
};


// Callback: void cmpxchg()
class AtomicCmpXchgInstrumenter : public Instrumenter {
  public:
    AtomicCmpXchgInstrumenter(std::string name, Instrumentation* instrumentation) :
      Instrumenter(name, instrumentation) {};

    bool CheckAndInstrument(Instruction* I) {
      CAST_OR_RETURN(AtomicCmpXchgInst, SI, I);

      safe_assert(parent_ != NULL);

      count_++;

      InstrPtrVector Instrs;
      Value* kvalue1 = KVALUE_VALUE(SI->getCompareOperand(), Instrs, NOSIGN);
      if(kvalue1 == NULL) return false;

      Value* kvalue2 = KVALUE_VALUE(SI->getNewValOperand(), Instrs, NOSIGN);
      if(kvalue2 == NULL) return false;

      Constant* C_iid = IID_CONSTANT(SI);
      Instruction* I_cast_ptr = PTR_CAST_INSTR(SI->getPointerOperand());
      Instrs.push_back(I_cast_ptr);

      Instruction* call = CALL_IID_PTR_KVALUE_KVALUE_INT(INSTR_TO_CALLBACK("cmpxchg"), C_iid, I_cast_ptr, kvalue1, kvalue2, computeIndex(SI));
      Instrs.push_back(call);

      // instrument
      InsertAllBefore(Instrs, I);

      return true;
    }
};


// Callback: void atomicrmw()
class AtomicRMWInstrumenter : public Instrumenter {
  public:
    AtomicRMWInstrumenter(std::string name, Instrumentation* instrumentation) :
      Instrumenter(name, instrumentation) {};

    bool CheckAndInstrument(Instruction* I) {
      CAST_OR_RETURN(AtomicRMWInst, SI, I);

      safe_assert(parent_ != NULL);

      count_++;

      Instruction *call = CallInst::Create(parent_->M_->getOrInsertFunction(StringRef("llvm_atomicrmw"), FunctionType::get(VOID_TYPE(), false)));
      call->insertBefore(I);

      return true;
    }
};

// ***** TerminatorInst ***** //

// Callback: void branch()
class BranchInstrumenter : public Instrumenter {
  public:
    BranchInstrumenter(std::string name, Instrumentation* instrumentation) :
      Instrumenter(name, instrumentation) {};

    bool CheckAndInstrument(Instruction* I) {
      CAST_OR_RETURN(BranchInst, SI, I);

      safe_assert(parent_ != NULL);

      count_++;

      InstrPtrVector Instrs;

      Constant* conditional = BOOL_CONSTANT(SI->isConditional());
      Constant* C_iid = IID_CONSTANT(SI);

      if (SI->isConditional()) {
        Value* op1 = KVALUE_VALUE(SI->getCondition(), Instrs, NOSIGN);
        if(op1 == NULL) return false;
        Instruction* call = CALL_IID_BOOL_KVALUE_INT(INSTR_TO_CALLBACK("branch"), C_iid, conditional, op1, computeIndex(SI));
        Instrs.push_back(call);
      } else {
        Instruction* call = CALL_IID_BOOL_INT(INSTR_TO_CALLBACK("branch2"), C_iid, conditional, computeIndex(SI));
        Instrs.push_back(call);
      }

      // instrument
      InsertAllBefore(Instrs, SI);

      return true;
    }
};


// Callback: void indirectbr()
class IndirectBrInstrumenter : public Instrumenter {
  public:
    IndirectBrInstrumenter(std::string name, Instrumentation* instrumentation) :
      Instrumenter(name, instrumentation) {};

    bool CheckAndInstrument(Instruction* I) {
      CAST_OR_RETURN(IndirectBrInst, SI, I);

      safe_assert(parent_ != NULL);

      count_++;

      InstrPtrVector Instrs;

      Constant* C_iid = IID_CONSTANT(SI);
      Value* op1 = KVALUE_VALUE(SI->getAddress(), Instrs, NOSIGN);
      if (op1 == NULL) return false;
      Instruction* call = CALL_IID_KVALUE_INT(INSTR_TO_CALLBACK("indirectbr"), C_iid, op1, computeIndex(SI));
      Instrs.push_back(call);

      // instrument
      InsertAllBefore(Instrs, SI);

      return true;
    }
};


// Callback: void invoke()
class InvokeInstrumenter : public Instrumenter {
  public:
    InvokeInstrumenter(std::string name, Instrumentation* instrumentation) :
      Instrumenter(name, instrumentation) {};

    bool CheckAndInstrument(Instruction* I) {
      CAST_OR_RETURN(InvokeInst, SI, I);

      safe_assert(parent_ != NULL);

      count_++;

      InstrPtrVector Instrs;

      Constant* C_iid = IID_CONSTANT(SI);

      // get call arguments
      unsigned numArgs = SI->getNumArgOperands();
      unsigned i;

      for (i = 0; i < numArgs; i++)
      {
        Value* arg = KVALUE_VALUE(SI->getArgOperand(i), Instrs, NOSIGN);
        Instruction* call = CALL_KVALUE(INSTR_TO_CALLBACK("push_stack"), arg);
        Instrs.push_back(call);
      }

      Value* call_value = KVALUE_VALUE(SI->getCalledValue(), Instrs, NOSIGN);

      Instruction* call = CALL_IID_KVALUE_INT(INSTR_TO_CALLBACK("invoke"), C_iid, call_value, computeIndex(SI));
      Instrs.push_back(call);

      // instrument
      InsertAllBefore(Instrs, SI);

      return true;

    }
};


// Callback: void resume()
class ResumeInstrumenter : public Instrumenter {
  public:
    ResumeInstrumenter(std::string name, Instrumentation* instrumentation) :
      Instrumenter(name, instrumentation) {};

    bool CheckAndInstrument(Instruction* I) {
      CAST_OR_RETURN(ResumeInst, SI, I);

      safe_assert(parent_ != NULL);

      count_++;

      InstrPtrVector Instrs;

      Constant* C_iid = IID_CONSTANT(SI);
      Value* op1 = KVALUE_VALUE(SI->getValue(), Instrs, NOSIGN);
      if (op1 == NULL) return false;
      Instruction* call = CALL_IID_KVALUE_INT(INSTR_TO_CALLBACK("resume"), C_iid, op1, computeIndex(SI));
      Instrs.push_back(call);

      // instrument
      InsertAllBefore(Instrs, SI);


      return true;
    }
};


// Callback: void return()
class ReturnInstrumenter : public Instrumenter {
  public:
    ReturnInstrumenter(std::string name, Instrumentation* instrumentation) :
      Instrumenter(name, instrumentation) {};

    bool CheckAndInstrument(Instruction* I) {
      CAST_OR_RETURN(ReturnInst, SI, I);

      safe_assert(parent_ != NULL);

      count_++;

      InstrPtrVector Instrs;

      Constant* C_iid = IID_CONSTANT(SI);
      Value* retVal = SI->getReturnValue();

      if (retVal == NULL) {
        Instruction* call = CALL_IID_INT(INSTR_TO_CALLBACK("return2_"), C_iid, computeIndex(SI));
        Instrs.push_back(call);
      } else {
        Value* op1 = KVALUE_VALUE(retVal, Instrs, NOSIGN);
        if (op1 == NULL) return false;
        Instruction* call = CALL_IID_KVALUE_INT(INSTR_TO_CALLBACK("return_"), C_iid, op1, computeIndex(SI));
        Instrs.push_back(call);
      }

      // instrument
      InsertAllBefore(Instrs, SI);

      return true;
    }
};


// Callback: void switch_()
class SwitchInstrumenter : public Instrumenter {
  public:
    SwitchInstrumenter(std::string name, Instrumentation* instrumentation) :
      Instrumenter(name, instrumentation) {};

    bool CheckAndInstrument(Instruction* I) {
      CAST_OR_RETURN(SwitchInst, SI, I);

      safe_assert(parent_ != NULL);

      count_++;

      InstrPtrVector Instrs;

      Constant* C_iid = IID_CONSTANT(SI);

      Value* op = SI->getCondition();

      Instruction* call = CALL_IID_KVALUE_INT(INSTR_TO_CALLBACK("switch_"), C_iid, op, computeIndex(SI));
      Instrs.push_back(call);

      // instrument
      InsertAllBefore(Instrs, SI);

      return true;
    }
};


// ***** Other Operations ***** //

// Callback: void icmp()
class ICmpInstrumenter : public Instrumenter {
  public:
    ICmpInstrumenter(std::string name, Instrumentation* instrumentation) :
      Instrumenter(name, instrumentation) {};

    bool CheckAndInstrument(Instruction* I) {
      CAST_OR_RETURN(ICmpInst, SI, I);

      safe_assert(parent_ != NULL);

      count_++;

      InstrPtrVector Instrs;

      Value* op1 = KVALUE_VALUE(SI->getOperand(0U), Instrs, NOSIGN);
      if(op1 == NULL) return false;

      Value* op2 = KVALUE_VALUE(SI->getOperand(1U), Instrs, NOSIGN);
      if(op2 == NULL) return false;

      Constant* C_iid = IID_CONSTANT(SI);

      PRED pred = SI->getUnsignedPredicate();
      Constant* C_pred = PRED_CONSTANT(pred);

      Instruction* call = CALL_IID_KVALUE_KVALUE_PRED_INT(INSTR_TO_CALLBACK("icmp"), C_iid, op1, op2, C_pred, computeIndex(SI));
      Instrs.push_back(call);

      // instrument
      InsertAllBefore(Instrs, SI);

      return true;
    }
};


// Callback: void fcmp()
class FCmpInstrumenter : public Instrumenter {
  public:
    FCmpInstrumenter(std::string name, Instrumentation* instrumentation) :
      Instrumenter(name, instrumentation) {};

    bool CheckAndInstrument(Instruction* I) {
      CAST_OR_RETURN(FCmpInst, SI, I);

      safe_assert(parent_ != NULL);

      count_++;

      InstrPtrVector Instrs;

      Value* op1 = KVALUE_VALUE(SI->getOperand(0U), Instrs, NOSIGN);
      if(op1 == NULL) return false;

      Value* op2 = KVALUE_VALUE(SI->getOperand(1U), Instrs, NOSIGN);
      if(op2 == NULL) return false;

      Constant* C_iid = IID_CONSTANT(SI);

      PRED pred = SI->getPredicate();
      Constant* C_pred = PRED_CONSTANT(pred);

      Instruction* call = CALL_IID_KVALUE_KVALUE_PRED_INT(INSTR_TO_CALLBACK("icmp"), C_iid, op1, op2, C_pred, computeIndex(SI));
      Instrs.push_back(call);

      // instrument
      InsertAllBefore(Instrs, SI);

      return true;
    }
};

// Callback: void vaarg()
class VAArgInstrumenter : public Instrumenter {
  public:
    VAArgInstrumenter(std::string name, Instrumentation* instrumentation) :
      Instrumenter(name, instrumentation) {};

    bool CheckAndInstrument(Instruction* I) {
      CAST_OR_RETURN(VAArgInst, SI, I);

      safe_assert(parent_ != NULL);

      count_++;

      Instruction *call = CallInst::Create(parent_->M_->getOrInsertFunction(StringRef("llvm_vaarg"), FunctionType::get(VOID_TYPE(), false)));
      call->insertBefore(I);

      return true;
    }
};

#endif // INSTRUMENTERS_H_

