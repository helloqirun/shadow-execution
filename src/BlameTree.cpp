/**
 * @file BlameTree.cpp
 * @brief 
 */

/*
 * Copyright (c) 2013, UC Berkeley All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 *
 * 1.  Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the UC Berkeley nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY UC BERKELEY ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL UC BERKELEY BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

// Author: Cuong Nguyen and Cindy Rubio-Gonzalez

#include "BlameTree.h"

/******* DEFINE ANALYSIS PARAMETERS *******/
int BlameTree::outputPC = 0;
BlameTree::HIGHPRECISION BlameTree::errorThreshold = 2e-26; 
BlameTree::HIGHPRECISION BlameTree::machineEpsilon = 2e-52;

/******* HELPER FUNCTIONS *******/

void BlameTree::copyShadow(IValue *src, IValue *dest) {
  //
  // copy shadow object from source to destination, only if they are
  // floating-point value
  //
  if (src->isFlpValue() && dest->isFlpValue()) {
    if (src->getShadow() != NULL) {
      BlameTreeShadowObject<HIGHPRECISION> *btmSOSrc, *btmSODest;

      btmSOSrc = (BlameTreeShadowObject<HIGHPRECISION> *) src->getShadow();
      btmSODest = new BlameTreeShadowObject<HIGHPRECISION>(*btmSOSrc);

      dest->setShadow(btmSODest);
    } else {
      dest->setShadow(NULL); 
    }
  }
}

BlameTree::HIGHPRECISION BlameTree::getShadowValue(SCOPE scope, int64_t value) {
  HIGHPRECISION result;

  if (scope == CONSTANT) {
    double *ptr;

    ptr = (double *) &value;
    result = *ptr;
  } else {
    IValue *iv; 
    
    iv = (scope == GLOBAL) ? globalSymbolTable[value] :
      executionStack.top()[value];
    result = iv->getShadow() == NULL ? iv->getFlpValue() :
      ((BlameTreeShadowObject<HIGHPRECISION>*) iv->getShadow())->getValue();
  }

  return result;
}

BlameTree::LOWPRECISION BlameTree::getActualValue(SCOPE scope, int64_t value) {
  LOWPRECISION actualValue;

  if (scope == CONSTANT) {
    double *ptr;

    ptr = (double *) &value;
    actualValue = *ptr;
  } else {
    IValue *iv;

    iv = (scope == GLOBAL) ? globalSymbolTable[value] :
      executionStack.top()[value];
    actualValue = iv->getFlpValue();
  }

  return actualValue;
}

int BlameTree::getPC(SCOPE scope, int64_t value) {
  int pc;

  if (scope == CONSTANT) {
    pc = -1;
  } else {
    IValue *iv;

    iv = (scope == GLOBAL) ? globalSymbolTable[value] : executionStack.top()[value];
    if (iv->getShadow() != NULL) {
      pc = ((BlameTreeShadowObject<HIGHPRECISION>*) iv->getShadow())->getPC();
    } else {
      pc = iv->getLineNumber();
    }
  }

  return pc;
}

BlameTree::HIGHPRECISION BlameTree::computeRelativeError(HIGHPRECISION
                 highValue, LOWPRECISION lowValue) {
  HIGHPRECISION d; 
  
  d = highValue != 0 ? highValue : machineEpsilon;

  return abs((HIGHPRECISION)((highValue - lowValue)/d));
}

/******* ANALYSIS FUNCTIONS *******/

void BlameTree::pre_fpbinop(int inx) {
  if (executionStack.top()[inx]->getShadow() != NULL) {
    preBtmSO = *((BlameTreeShadowObject<HIGHPRECISION>*)
      executionStack.top()[inx]->getShadow());
  } else {
    preBtmSO = BlameTreeShadowObject<HIGHPRECISION>();
  }
} 

void BlameTree::post_fbinop(SCOPE lScope, SCOPE rScope, int64_t lValue,
    int64_t rValue, KIND type, int line, int inx, BINOP op) {

  HIGHPRECISION sv1, sv2, sresult;
  LOWPRECISION v1, v2, result;
  int pc1, pc2;

  //
  // assert: type is a floating-point type
  //
  safe_assert(type == FLP32_KIND || type == FLP64_KIND || type == FLP80X86_KIND
      || type == FLP128_KIND || type == FLP128PPC_KIND);

  //
  // Obtain actual value, shadow value and pc of two operands
  //
  v1 = getActualValue(lScope, lValue);
  v2 = getActualValue(rScope, rValue);
  sv1 = getShadowValue(lScope, lValue);
  sv2 = getShadowValue(rScope, rValue);
  pc1 = (lScope == CONSTANT) ? line : getPC(lScope, lValue); 
  pc2 = (rScope == CONSTANT) ? line : getPC(rScope, rValue);

  //
  // Perform binary operation for shadow values
  //
  switch (op) {
    case FADD:
      sresult = sv1 + sv2;
      break;
    case FSUB:
      sresult = sv1 - sv2;
      break;
    case FMUL:
      sresult = sv1 * sv2;
      break;
    case FDIV:
      sresult = sv1 / sv2;
      break;
    default:
      DEBUG_STDERR("Unsupported floating-point binary operator: " << BINOP_ToString(op)); 
      safe_assert(false);
  }

  //
  // Compute other analysis information such as relative error, sources of
  // relative error
  //
}