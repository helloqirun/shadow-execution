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

#endif // INSTRUMENTERS_H_

