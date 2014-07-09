/**
 * @file StoreInstrumenter.cpp
 * @brief
 */

#include "StoreInstrumenter.h"

bool StoreInstrumenter::CheckAndInstrument(Instruction* inst) {
  StoreInst* storeInst; 
  
  storeInst = dyn_cast<StoreInst>(inst);
  if (storeInst != NULL) {
    safe_assert(parent_ != NULL);

    InstrPtrVector instrs;
    Value *valueOp, *pointerOp;
    Type *valueOpType;
    Constant *cPointerInx, *cScope, *cInx, *cSrcType, *cSrcScope, *cSrcInx; 
    Instruction* cSrcValue;
    Instruction *call;

    count_++;

    // skipping stores in main if operands are arguments
    if (BasicBlock *basicBlock = storeInst->getParent()) {
      if (Function *function = basicBlock->getParent()) {
        if (function->getName() == "main") {
          if (dyn_cast<Argument>(storeInst->getValueOperand())) {
            return false;
          }
        }
      }
    }

    parent_->AS_ = storeInst->getPointerAddressSpace();

    valueOp = storeInst->getValueOperand();
    pointerOp = storeInst->getPointerOperand();
    valueOpType = valueOp->getType();

    cSrcType = KIND_CONSTANT(TypeToKind(valueOpType));
    cSrcScope = INT32_CONSTANT(getScope(valueOp), SIGNED);
    cSrcInx = computeIndex(valueOp);
    cPointerInx = computeIndex(pointerOp);
    cScope = INT32_CONSTANT(getScope(pointerOp), SIGNED);
    cInx = computeIndex(storeInst);

    // retrieving source value
    cSrcValue = NULL;
    if (valueOpType->isIntegerTy()) {

      cSrcValue = INTMAX_CAST_INSTR(valueOp, true);
      instrs.push_back(cSrcValue);

    } else if (valueOpType->isFloatingPointTy()) {

      Instruction *flpCast;

      flpCast = FLPMAX_CAST_INSTR(valueOp);
      cSrcValue = VALUE_CAST_INSTR(flpCast);

      instrs.push_back(flpCast);
      instrs.push_back(cSrcValue);
      
    } else if (valueOpType->isPointerTy()) {
      cSrcValue = PTRTOINT_CAST_INSTR(valueOp);

      instrs.push_back(cSrcValue);
    }


    // debugging info
    string filename = getFileName(storeInst);
    int line = getLineNumber(storeInst);

    DebugInfo *debug = new DebugInfo;
    debug->file = strdup(filename.c_str());
    debug->line = line;
    IID address = static_cast<IID>(reinterpret_cast<ADDRINT>(storeInst));  
    parent_->debugMap[address] = debug;
    // end of debugging info

    if (parent_->fileNames.insert(std::make_pair(filename, parent_->fileCount)).second) {
      // element was inserted
      parent_->fileCount++;
    }

    call = CALL_INT_INT_KIND_INT_INT_INT64_INT("llvm_store", cPointerInx, cScope, cSrcType, cSrcScope, cSrcInx, cSrcValue, cInx);
    instrs.push_back(call);

    // instrument
    InsertAllBefore(instrs, storeInst);

    return true;

  } else {

    return false;

  }
}

SCOPE StoreInstrumenter::getScope(Value *value) {
  if (isa<GlobalVariable>(value)) {
    return GLOBAL;
  } else if (isa<Constant>(value)) {
    return CONSTANT;
  } else {
    safe_assert(isa<Instruction>(value) || isa<Argument>(value));
    return LOCAL;
  }
}
