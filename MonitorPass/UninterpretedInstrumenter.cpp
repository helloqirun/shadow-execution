/**
 * @file UninterpretedInstrumenter.cpp
 * @brief
 */

#include "UninterpretedInstrumenter.h"

bool UninterpretedInstrumenter::CheckAndInstrument(Instruction* inst) {
  safe_assert(parent_ != NULL);

  count_++;

  UNINTERPRETEDINST unInst = getUninterpretedInst(inst);

  std::stringstream callback;
  callback << "llvm_";

  switch(unInst) {
    case EXTRACTELEMENT:
      callback << "extractelement";
      break;
    case INSERTELEMENT:
      callback << "insertelement";
      break;
    case SHUFFLEVECTOR:
      callback << "shufflevector";
      break;
    case EXTRACTVALUE:
      callback << "extractvalue";
      break;
    case INSERTVALUE:
      callback << "insertvalue";
      break;
    case UNREACHABLE:
      callback << "unreachable";
      break;
    case PHINODE:
      callback << "phinode";
      break;
    case SELECT:
      callback << "select";
      break;
    case LANDINGPAD:
      callback << "landingpad";
      break;
    case FENCE:
      callback << "fence";
      break;
    case VAARG:
      callback << "vaarg";
      break;
    default:
      return false; 
  }

  Instruction *call =
    CallInst::Create(parent_->M_->getOrInsertFunction(StringRef(callback.str().c_str()),
          FunctionType::get(VOID_TYPE(), false)));
  if (unInst == PHINODE) {
    // TODO: what to do for PHINode?
  } else {
    call->insertBefore(inst);
  }

  return true;
}

UNINTERPRETEDINST UninterpretedInstrumenter::getUninterpretedInst(Instruction* inst) {
  if (dyn_cast<ExtractElementInst>(inst) != NULL) {
    return EXTRACTELEMENT;
  } else if (dyn_cast<InsertElementInst>(inst) != NULL) {
    return INSERTELEMENT;
  } else if (dyn_cast<ShuffleVectorInst>(inst) != NULL) {
    return SHUFFLEVECTOR;
  } else if (dyn_cast<ExtractValueInst>(inst) != NULL) {
    return EXTRACTVALUE;
  } else if (dyn_cast<InsertValueInst>(inst) != NULL) {
    return INSERTVALUE;
  } else if (dyn_cast<UnreachableInst>(inst) != NULL) {
    return UNREACHABLE;
  } else if (dyn_cast<PHINode>(inst) != NULL) {
    return PHINODE;
  } else if (dyn_cast<SelectInst>(inst) != NULL) {
    return SELECT;
  } else if (dyn_cast<LandingPadInst>(inst) != NULL) {
    return LANDINGPAD;
  } else if (dyn_cast<FenceInst>(inst) != NULL) {
    return FENCE;
  } else if (dyn_cast<VAArgInst>(inst) != NULL) {
    return VAARG;
  } else {
    return UNINTERPRETEDINST_INVALID;
  }
}

