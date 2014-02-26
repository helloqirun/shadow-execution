
#include <math.h>
#include <glog/logging.h>

#include "NaNPropagationAnalysis.h"

void NaNPropagationAnalysis::load(IID iid UNUSED, KIND type, SCOPE opScope, int opInx, uint64_t opAddr, bool loadGlobal, int loadInx, int file, int line, int inx) {
  InterpreterObserver::load(iid, type, opScope, opInx, opAddr, loadGlobal, loadInx, file, line, inx);
  IValue* loadValue = executionStack.top()[inx];
  
  if (type == FLP32_KIND || type == FLP64_KIND || type == FLP128_KIND) {
    if (isnan(loadValue->getFlpValue())) {
      LOG(INFO) << file << ":" << line << ": WARNING Loading a NAN value " << endl;
    }
  }
  return;
}

void NaNPropagationAnalysis::store(int pInx, SCOPE pScope, KIND srcKind, SCOPE srcScope, int srcInx, int64_t srcValue, int file, int line, int inx) {
  InterpreterObserver::store(pInx, pScope, srcKind, srcScope, srcInx, srcValue, file, line, inx);

  // TODO: storing nan constant
  if (srcScope != CONSTANT) {
    IValue* srcValue = executionStack.top()[srcInx];
    
    if (srcValue->getType() == FLP32_KIND || srcValue->getType() == FLP64_KIND || srcValue->getType() == FLP128_KIND) {
      if (isnan(srcValue->getFlpValue())) {
	LOG(INFO) << file << ":" << line << ": WARNING Storing a NAN Value " << endl;
      }
    }
  }

  // case overwriting a NaN

  return;
}

