/**
 * @file InterpreterObserver.cpp
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
 * 3. All advertising materials mentioning features or use of this software must
 * display the following acknowledgement: This product includes software
 * developed by the UC Berkeley.
 *
 * 4. Neither the name of the UC Berkeley nor the names of its contributors may
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

#include "InterpreterObserver.h"

#include "stdbool.h"
#include <assert.h>
#include <math.h>
#include <stack>
#include <vector>

#include <llvm/IR/InstrTypes.h>
#include <glog/logging.h>

/***************************** Helper Functions *****************************/

unsigned InterpreterObserver::findIndex(IValue* array, unsigned offset, unsigned length) {
  int low, high, index;

  //
  // assert: offset cannot larger than the size of the array itself (int byte)
  //
  // safe_assert(offset <= array[length-1].getFirstByte() + KIND_GetSize(array[length-1].getType()));

  //
  // initializing lowerbound and upperbound of the index
  //
  low = 0;
  high = length - 1;

  DEBUG_STDOUT("\t" << "[findIndex] Offset: " << offset << " Length: " << length);

  //
  // search for the index using binary search
  // the IValue at the index should have the largest byteOffset that is less
  // than or equal to the offset
  //
  index = -1;
  while(low < high){

    unsigned mid, firstByte;
    mid = (low + high) / 2;

    DEBUG_STDOUT("\t" << "[findIndex] Mid index: " << mid);

    firstByte = array[mid].getFirstByte();

    DEBUG_STDOUT("\t" << "[findIndex] Firstbyte: " << firstByte);

    if (firstByte == offset) {
      index = mid;
      break;
    } else if ((mid + 1 <= length -1) && (firstByte < offset) && (offset < array[mid+1].getFirstByte())) {
      index = mid;
      break;
    } else if (offset < firstByte) {
      high = mid - 1;
    } else {
      low = mid + 1;
    }

  }

  index = (index == -1) ? high : index;

  DEBUG_STDOUT("\t" << "[findIndex] Returning index: " << index); 

  return index; 
}

bool InterpreterObserver::checkStore(IValue *dest, KIND srcKind, int64_t srcValue) {
  bool result; 
  double dpValue;
  double *dpPtr;

  result = false;
  dpPtr = (double *) &srcValue; 
  dpValue = *dpPtr;

  switch(srcKind) {
    case PTR_KIND:

      DEBUG_STDOUT("\t Destination value: " << (int64_t) dest->getValue().as_ptr);
      DEBUG_STDOUT("\t Destination value plus offset: " << (int64_t)
          dest->getValue().as_ptr + dest->getOffset());
      DEBUG_STDOUT("\t Concrete value: " << (int64_t) srcValue);

      result = ((int64_t)dest->getValue().as_ptr + dest->getOffset() == srcValue);
      break;
    case INT1_KIND:
      result = ((bool)dest->getValue().as_int == (bool)srcValue);
      break;
    case INT8_KIND: 
      result = ((int8_t)dest->getValue().as_int == (int8_t)srcValue);
      break;
    case INT16_KIND: 
      result = ((int16_t)dest->getValue().as_int == (int16_t)srcValue);
      break;
    case INT24_KIND:
      result = (dest->getIntValue() == srcValue);
      break;
    case INT32_KIND: 
      result = ((int32_t)dest->getValue().as_int == (int32_t)srcValue);
      break;
    case INT64_KIND:
      result = (dest->getValue().as_int == srcValue);
      break;
    case FLP32_KIND:
      if (isnan((float)dest->getValue().as_flp) && isnan((float)dpValue)) {
        result = true;
      }
      else {
        result = ((float)dest->getValue().as_flp) == ((float)dpValue);
      }
      break;
    case FLP64_KIND:
      if (isnan((double)dest->getValue().as_flp) && isnan((double)dpValue)) {
        result = true;
      }
      else {
        result = ((double)dest->getValue().as_flp) == ((double)dpValue);
      }
      break;
    case FLP80X86_KIND:
      result = dest->getValue().as_flp == dpValue;
      break;
    default: //safe_assert(false);
      break;
  }

  return result;
}

bool InterpreterObserver::checkStore(IValue *dest, KVALUE *kv) {
  bool result = false;

  switch(kv->kind) {
    case PTR_KIND:

      DEBUG_STDOUT("\t Destination value: " << (int64_t) dest->getValue().as_ptr);
      DEBUG_STDOUT("\t Destination value plus offset: " << (int64_t)
          dest->getValue().as_ptr + dest->getOffset());
      DEBUG_STDOUT("\t Concrete value: " << (int64_t) kv->value.as_ptr);

      result = ((int64_t)dest->getValue().as_ptr + dest->getOffset() == (int64_t)kv->value.as_ptr);
      break;
    case INT1_KIND:
      result = ((bool)dest->getValue().as_int == (bool)kv->value.as_int);
      break;
    case INT8_KIND: 
      result = ((int8_t)dest->getValue().as_int == (int8_t)kv->value.as_int);
      break;
    case INT16_KIND: 
      result = ((int16_t)dest->getValue().as_int == (int16_t)kv->value.as_int);
      break;
    case INT24_KIND:
      result = (dest->getIntValue() == KVALUE_ToIntValue(kv));
      break;
    case INT32_KIND: 
      result = ((int32_t)dest->getValue().as_int == (int32_t)kv->value.as_int);
      break;
    case INT64_KIND:
      result = (dest->getValue().as_int == kv->value.as_int);
      break;
    case FLP32_KIND:
      if (isnan((float)dest->getValue().as_flp) && isnan((float)kv->value.as_flp)) {
        result = true;
      }
      else {
        result = ((float)dest->getValue().as_flp) == ((float)kv->value.as_flp);
      }
      break;
    case FLP64_KIND:
      if (isnan((double)dest->getValue().as_flp) && isnan((double)kv->value.as_flp)) {
        result = true;
      }
      else {
        result = ((double)dest->getValue().as_flp) == ((double)kv->value.as_flp);
        if (!result) {
          cout << dest->getValue().as_ptr << endl;
          cout << kv->value.as_ptr << endl;
        }
      }
      break;
    case FLP80X86_KIND:
      result = dest->getValue().as_flp == kv->value.as_flp;
      break;
    default: //safe_assert(false);
      break;
  }

  return result;
}

void InterpreterObserver::copyShadow(IValue* src UNUSED, IValue* dest UNUSED) {
  // done nothing
  // shadow value is not used in the core interpreter
}

std::string InterpreterObserver::BINOP_ToString(int binop) {
  std::stringstream s;
  switch(binop) {
    case ADD:
      s << "ADD";
      break;
    case FADD:
      s << "FADD";
      break;
    case SUB:
      s << "SUB";
      break;
    case FSUB:
      s << "FSUB";
      break;
    case MUL:
      s << "MUL";
      break;
    case FMUL:
      s << "FMUL";
      break;
    case UDIV:
      s << "UDIV";
      break;
    case SDIV:
      s << "SDIV";
      break;
    case FDIV:
      s << "FDIV";
      break;
    case UREM:
      s << "UREM";
      break;
    case SREM:
      s << "SREM";
      break;
    case FREM:
      s << "FREM";
      break;
    default: 
      DEBUG_STDERR("Unsupport binary operator operand: " << binop);
      safe_assert(false);
      break;
  }
  return s.str();
}

std::string InterpreterObserver::BITWISE_ToString(int bitwise) {
  std::stringstream s;
  switch(bitwise) {
    case SHL:
      s << "SHL";
      break;
    case LSHR:
      s << "LSHR";
      break;
    case ASHR:
      s << "ASHR";
      break;
    case AND:
      s << "AND";
      break;
    case OR:
      s << "OR";
      break;
    case XOR:
      s << "XOR";
      break;
    default: 
      DEBUG_STDERR("Unsupport bitwise operator operand: " << bitwise);
      safe_assert(false);
      break;
  }
  return s.str();
}

std::string InterpreterObserver::CASTOP_ToString(int castop) {
  std::stringstream s;
  switch(castop) {
    case TRUNC:
      s << "TRUNC";
      break;
    case ZEXT:
      s << "ZEXT";
      break;
    case SEXT:
      s << "SEXT";
      break;
    case FPTRUNC:
      s << "FPTRUNC";
      break;
    case FPEXT:
      s << "FPEXT";
      break;
    case FPTOUI:
      s << "FPTOUI";
      break;
    case FPTOSI:
      s << "FPTOSI";
      break;
    case UITOFP:
      s << "UITOFP";
      break;
    case SITOFP:
      s << "SITOFP";
      break;
    case PTRTOINT:
      s << "PTRTOINT";
      break;
    case INTTOPTR:
      s << "INTTOPTR";
      break;
    case BITCAST:
      s << "BITCAST";
      break;
    default: 
      DEBUG_STDERR("Unsupport bitwise operator operand: " << castop);
      safe_assert(false);
      break;
  }
  return s.str();
}

/***************************** Interpretation *****************************/

// *** Load and Store Operations *** //

void InterpreterObserver::load_struct(IID iid UNUSED, KIND type UNUSED, KVALUE* src, int file, int line, int inx) {
  int i, structSize;
  IValue* dest;

  LOG(INFO) << "[LOAD STRUCT] Performing load at " << file << ":" << line << endl; 

  structSize = returnStruct.size();
  dest = new IValue [structSize];
    
//    (IValue*) malloc(structSize*sizeof(IValue));

  if (src->inx == -1) {

    //
    // Case 1: struct constant.
    //
    // Create an IValue struct that has all values in structReturn.
    //
    
    i = 0;
    while (!returnStruct.empty()) {
      KVALUE* concreteStructElem; 
      IValue* structElem; 
      
      concreteStructElem = returnStruct.front();

      if (concreteStructElem->inx == -1) {
        structElem = new IValue(concreteStructElem->kind, concreteStructElem->value,
            REGISTER);
      } else {
        structElem = concreteStructElem->isGlobal ?
          globalSymbolTable[concreteStructElem->inx] :
          executionStack.top()[concreteStructElem->inx];
      }

      dest[i] = *structElem;

      i++;
      returnStruct.pop();
    }
    
    safe_assert(false);

  } else {
    
    //
    // Case 2: local or global struct.
    //
    
    IValue* srcPointer;
    IValue* structSrc;

    srcPointer = src->isGlobal ? globalSymbolTable[src->inx] : executionStack.top()[src->inx];
    structSrc = (IValue*) srcPointer->getIPtrValue();

    i = 0;
    while (!returnStruct.empty()) {
      KVALUE *concreteStructElem, *concreteStructElemPtr; 
      IValue *structElem;
      int type;

      //
      // get concrete value in case we need to sync
      //
      concreteStructElem = returnStruct.front();

      structElem = new IValue();
      structSrc[i].copy(structElem);
      type = structElem->getType();

      //
      // sync load
      // first create a KVALUE pointer to concreteStructElem because sync load
      // expect the KVALUE to be a pointer to the concrete value
      //
      concreteStructElemPtr = (KVALUE*) malloc(sizeof(KVALUE));
      concreteStructElemPtr->value.as_ptr = &(concreteStructElem->value);
      if (syncLoad(structElem, concreteStructElemPtr, type)) {
        LOG(INFO) << "[LOAD STRUCT] Syncing load at " << file << ":" << line << endl; 
      }

      dest[i] = *structElem;

      i++;
      returnStruct.pop();
    }
  }

  dest->setLineNumber(line);

  executionStack.top()[inx] = dest;

  DEBUG_STDOUT("Destination result: " << dest->toString());

  return;
}

void InterpreterObserver::load(IID iid UNUSED, KIND type, SCOPE opScope, int opInx, uint64_t opAddr, bool loadGlobal, int loadInx, int file, int line, int inx) {

  bool isPointerConstant = false;
  bool sync = false;
  IValue* srcPtrLocation;

  LOG(INFO) << "[LOAD] Performing load at " << file << ":" << line << endl; 

  // obtain source pointer value
  if (opScope == CONSTANT) {
    isPointerConstant = true;
  } else if (opScope == GLOBAL) {
    srcPtrLocation = globalSymbolTable[opInx];
  } else {
    srcPtrLocation = executionStack.top()[opInx];
  }

  // perform load
  if (!isPointerConstant) {

    DEBUG_STDOUT("\tsrcPtrLocation: " << srcPtrLocation->toString());

    // creating new value
    IValue *destLocation = new IValue();    
    if (srcPtrLocation->isInitialized()) {
      IValue *srcLocation;

      // retrieving source
      IValue *values = (IValue*)srcPtrLocation->getIPtrValue();
      unsigned valueIndex = srcPtrLocation->getIndex();
      unsigned currOffset = values[valueIndex].getFirstByte();
      srcLocation = values + valueIndex;

      // calculating internal offset
      unsigned srcOffset = srcPtrLocation->getOffset();
      int internalOffset = srcOffset - currOffset;
      VALUE value = srcPtrLocation->readValue(internalOffset, type);

      DEBUG_STDOUT("\t\tvalueIndex: " << valueIndex);
      DEBUG_STDOUT("\t\tsrcOffset: " << srcOffset);
      DEBUG_STDOUT("\t\tcurrOffset: " << currOffset);
      DEBUG_STDOUT("\t\tsrcOffset" << srcOffset);
      DEBUG_STDOUT("\t\tinternal offset: " << internalOffset);
      DEBUG_STDOUT("\tsrcLocation: " << srcLocation->toString());
      DEBUG_STDOUT("\tCalling readValue with internal offset: " << internalOffset << " and size: " << KIND_GetSize(type)); 
      DEBUG_STDOUT("\t\tVALUE returned (float): " << value.as_flp);
      DEBUG_STDOUT("\t\tVALUE returned (int): " << value.as_int);

      // copying src into dest
      srcLocation->copy(destLocation);
      destLocation->setValue(value);
      destLocation->setType(type);

      // sync load
      sync = syncLoad(destLocation, opAddr, type);

      // if sync happens, update srcPtrLocation if possible
      if (sync) {
        IValue *lastElement;

        lastElement = values + srcPtrLocation->getLength() - 1;

        if (srcOffset + KIND_GetSize(type) <= lastElement->getFirstByte() + KIND_GetSize(lastElement->getType())) {
          srcPtrLocation->writeValue(internalOffset, KIND_GetSize(type), destLocation);
        }
      }
      
    } else {

      //
      // Source pointer is not initialized.
      //

      DEBUG_STDOUT("\tSource pointer is not initialized!");

      VALUE zeroValue;
      IValue* elem;
      zeroValue.as_int = 0;

      destLocation->setType(type);
      destLocation->setValue(zeroValue);

      // sync load
      sync = syncLoad(destLocation, opAddr, type);

      //
      // initialized source pointer
      //
      DEBUG_STDOUT("\tInitializing source pointer.");
      DEBUG_STDOUT("\tSource pointer location: " << srcPtrLocation->toString());
      elem = new IValue();
      destLocation->copy(elem);
      srcPtrLocation->setLength(1);
      srcPtrLocation->setSize(KIND_GetSize(type));
      srcPtrLocation->setValueOffset((int64_t) elem - srcPtrLocation->getValue().as_int);
      DEBUG_STDOUT("\tSource pointer location: " << srcPtrLocation->toString());

      //
      // update load variable
      //
      if (loadInx != -1) {
        IValue *elem, *values, *loadInst;

        loadInst = loadGlobal ? globalSymbolTable[loadInx] : executionStack.top()[loadInx];

        // retrieving source
        values = (IValue*)loadInst->getIPtrValue();
        elem = values + loadInst->getIndex();
        elem->setLength(srcPtrLocation->getLength());
        elem->setSize(srcPtrLocation->getSize());
        elem->setValueOffset(srcPtrLocation->getValueOffset());
      }

    }

    destLocation->setLineNumber(line);

    executionStack.top()[inx] = destLocation;
    DEBUG_STDOUT(destLocation->toString());

  }
  else {
    // NEW case for pointer constants
    // TODO: revise again
    DEBUG_STDOUT("[Load] => pointer constant.");

    IValue* destLocation; 
    VALUE zeroValue;

    destLocation = new IValue();

    zeroValue.as_int = 0;

    destLocation->setType(type);
    destLocation->setValue(zeroValue);
    destLocation->setLineNumber(line);

    // sync load
    sync = syncLoad(destLocation, opAddr, type);

    executionStack.top()[inx] = destLocation;

    DEBUG_STDOUT(destLocation->toString());
  }

  if (sync) {
    LOG(INFO) << "[LOAD] Syncing load at " << file << ":" << line << endl;
  }

  return;
}

void InterpreterObserver::store(int destInx, SCOPE destScope, KIND srcKind, SCOPE srcScope, int srcInx, int64_t srcValue, int file UNUSED, int line UNUSED, int inx UNUSED) {

  if (srcKind == INT80_KIND) {
    cout << "[store] Unsupported INT80_KIND" << endl;
    safe_assert(false);
    return; // otherwise compiler warning
  }

  //
  // pointer constant; we simply ignore this case
  //
  if (destScope == CONSTANT) {
    DEBUG_STDOUT("\tIgnoring pointer constant.");
    return; 
  }

  //
  // retrieve destination pointer operand
  //
  IValue* destPtrLocation = (destScope == GLOBAL) ? globalSymbolTable[destInx] :
    executionStack.top()[destInx];

  DEBUG_STDOUT("\tDestPtr: " << destPtrLocation->toString());

  // the destination pointer is not initialized
  // initialize with an empty IValue object
  if (!destPtrLocation->isInitialized()) {
    DEBUG_STDOUT("\tDestination pointer location is not initialized");
    IValue* iValue = new IValue(srcKind);
    iValue->setLength(0);
    destPtrLocation->setValueOffset( (int64_t)iValue - (int64_t)destPtrLocation->getPtrValue() ); 
    destPtrLocation->setInitialized();
    DEBUG_STDOUT("\tInitialized destPtr: " << destPtrLocation->toString());
  }

  unsigned destPtrOffset = destPtrLocation->getOffset();
  IValue *destLocation = NULL;
  IValue *srcLocation = NULL;
  int internalOffset = 0;

  // retrieve source
  if (srcScope == CONSTANT) {
    VALUE value;
    value.as_int = srcValue;

    srcLocation = new IValue(srcKind, value);
    srcLocation->setLength(0); // uninitialized constant pointer 
    if (srcKind == INT1_KIND) {
      srcLocation->setBitOffset(1);
    }
  } else if (srcScope == GLOBAL) {
      srcLocation = globalSymbolTable[srcInx];
  } else {
    srcLocation = executionStack.top()[srcInx];
  }

  DEBUG_STDOUT("\tSrc: " << srcLocation->toString());

  // retrieve actual destination
  IValue* values = (IValue*)destPtrLocation->getIPtrValue();
  unsigned valueIndex = destPtrLocation->getIndex();
  unsigned currOffset = values[valueIndex].getFirstByte();
  destLocation = values + valueIndex;
  internalOffset = destPtrOffset - currOffset;

  DEBUG_STDOUT("\tdestPtrOffset: " << destPtrOffset);
  DEBUG_STDOUT("\tvalueIndex: " << valueIndex << " currOffset: " << currOffset << " Other offset: "  << destPtrOffset);
  DEBUG_STDOUT("\tinternalOffset: " << internalOffset <<  " Size: " << destPtrLocation->getSize());
  DEBUG_STDOUT("\tDest: " << destLocation->toString());
  DEBUG_STDOUT("\tCalling writeValue with offset: " << internalOffset << ", size: " << destPtrLocation->getSize());

  // writing src into dest
  if (destPtrLocation->writeValue(internalOffset, KIND_GetSize(srcKind), srcLocation)) {
    srcLocation->copy(destLocation);
  }
  destPtrLocation->setInitialized();

  DEBUG_STDOUT("\tUpdated Dest: " << destLocation->toString());

  // just read again to check store
  DEBUG_STDOUT("\tCalling readValue with internal offset: " << internalOffset << " size: " << destPtrLocation->getSize());

  // NOTE: destLocation->getType() before
  IValue* writtenValue = new IValue(srcLocation->getType(),
      destPtrLocation->readValue(internalOffset, srcKind)); 
  writtenValue->setSize(destLocation->getSize());
  writtenValue->setIndex(destLocation->getIndex());
  writtenValue->setOffset(destLocation->getOffset());
  writtenValue->setBitOffset(destLocation->getBitOffset());

  DEBUG_STDOUT("\twrittenValue: " << writtenValue->toString());

  if (!checkStore(writtenValue, srcKind, srcValue)) { // destLocation
    DEBUG_STDERR("\twrittenValue: " << writtenValue->toString());
    DEBUG_STDERR("\tconcreteType: " << KIND_ToString(srcKind));
    DEBUG_STDERR("\tconcreteValue: " << srcValue);
    DEBUG_STDERR("\tMismatched values found in Store");
    safe_assert(false);
  }

  DEBUG_STDOUT("\tsrcLocation: " << srcLocation->toString());

  return;
}

// **** Binary Operations *** //
void InterpreterObserver::binop(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx, BINOP op) {

  if (type == INT80_KIND) {
    DEBUG_STDERR("Unsupported INT80_KIND");
    safe_assert(false);
    return; 
  }

  int64_t v1, v2;
  double d1, d2;
  VALUE result;
  IValue* iResult;

  //
  // Get values from two operands. They can be either integer or double so we
  // need 4 variables.
  //
  if (lScope == CONSTANT) { // constant
    double *ptr;

    v1 = lValue;
    ptr = (double *) &lValue;
    d1 = *ptr;
  } else { // register
    IValue *loc1; 

    loc1 = (lScope == GLOBAL) ? globalSymbolTable[lValue] : executionStack.top()[lValue];
    v1 = loc1->getIntValue();
    d1 = loc1->getFlpValue();

    DEBUG_STDOUT("\tOperand 01: " << loc1->toString());
  }

  if (rScope == CONSTANT) { // constant
    double *ptr;

    v2 = rValue;
    ptr = (double *) &rValue;
    d2 = *ptr;
  } else { // register
    IValue *loc2; 

    loc2 = (rScope == GLOBAL) ? globalSymbolTable[rValue] : executionStack.top()[rValue];
    v2 = loc2->getIntValue();
    d2 = loc2->getFlpValue();

    DEBUG_STDOUT("\tOperand 02: " << loc2->toString());
  }

  switch (op) {
    case ADD:
      result.as_int = v1 + v2;
      break;
    case SUB:
      result.as_int = v1 - v2;
      break;
    case MUL:
      result.as_int = v1 * v2;
      break;
    case SDIV:
      result.as_int = v1 / v2;
      break;
    case SREM:
      result.as_int = v1 % v2;
      break;
    case UDIV:
      result.as_int = (uint64_t) v1 / (uint64_t) v2;
      break;
    case UREM:
      result.as_int = (uint64_t) v1 % (uint64_t) v2;
      break;
    case FADD:
      result.as_flp = d1 + d2;
      break;
    case FSUB:
      result.as_flp = d1 - d2;
      break;
    case FMUL:
      result.as_flp = d1 * d2;
      break;
    case FDIV:
      result.as_flp = d1 / d2;
      break;
    default:
      DEBUG_STDERR("Unsupported binary operator: " << BINOP_ToString(op)); 
      safe_assert(false);
  }

  iResult = new IValue(type, result);
  iResult->setLineNumber(line);

  executionStack.top()[inx] = iResult;

  DEBUG_STDOUT(iResult->toString());

  return;
}

void InterpreterObserver::add(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx) {
  binop(lScope, rScope, lValue, rValue, type, line, inx, ADD);
}

void InterpreterObserver::fadd(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx) {
  binop(lScope, rScope, lValue, rValue, type, line, inx, FADD);
}

void InterpreterObserver::sub(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx) {
  binop(lScope, rScope, lValue, rValue, type, line, inx, SUB);
}

void InterpreterObserver::fsub(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx) {
  binop(lScope, rScope, lValue, rValue, type, line, inx, FSUB);
}

void InterpreterObserver::mul(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx) {
  binop(lScope, rScope, lValue, rValue, type, line, inx, MUL);
}

void InterpreterObserver::fmul(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx) {
  binop(lScope, rScope, lValue, rValue, type, line, inx, FMUL);
}

void InterpreterObserver::udiv(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx) {
  binop(lScope, rScope, lValue, rValue, type, line, inx, UDIV);
}

void InterpreterObserver::sdiv(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx) {
  binop(lScope, rScope, lValue, rValue, type, line, inx, SDIV);
}

void InterpreterObserver::fdiv(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx) {
  binop(lScope, rScope, lValue, rValue, type, line, inx, FDIV);
}

void InterpreterObserver::urem(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx) {
  binop(lScope, rScope, lValue, rValue, type, line, inx, UREM);
}

void InterpreterObserver::srem(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx) {
  binop(lScope, rScope, lValue, rValue, type, line, inx, SREM);
}

void InterpreterObserver::frem(SCOPE lScope UNUSED, SCOPE rScope UNUSED, int64_t lValue UNUSED, int64_t rValue UNUSED, KIND type UNUSED, int line UNUSED, int inx UNUSED) {
  DEBUG_STDERR("UNSUPPORTED IN C???");
  safe_assert(false);
}

// **** Bitwise Operations *** //

void InterpreterObserver::bitwise(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx, BITWISE op) {
  int64_t v64_1, v64_2;
  uint64_t uv64_1, uv64_2;
  int32_t v32_1, v32_2;
  uint32_t uv32_1, uv32_2;
  int16_t v16_1, v16_2; 
  uint16_t uv16_1, uv16_2;
  int8_t v8_1, v8_2;
  uint8_t uv8_1, uv8_2; 

  VALUE result;
  IValue* iResult;

  //
  // Unsupport for INT80_KIND right now
  //
  if (type == INT80_KIND) {
    DEBUG_STDERR("Unsupported INT80.");
    safe_assert(false);
    return;
  }

  //
  // Get values of two operands
  //
  if (lScope == CONSTANT) {
    v64_1 = lValue;
  } else {
    IValue* iOp1; 

    iOp1 = (lScope == GLOBAL) ? globalSymbolTable[lValue] : executionStack.top()[lValue];
    v64_1 = iOp1->getIntValue();
  }

  if (rScope == CONSTANT) {
    v64_2 = rValue;
  } else {
    IValue* iOp2; 

    iOp2 = (rScope == GLOBAL) ? globalSymbolTable[rValue] : executionStack.top()[rValue];
    v64_2 = iOp2->getIntValue();
  }


  //
  // Initialize values for other integer variables depending on type
  //
  v8_1 = (int8_t) v64_1;
  uv8_1 = (uint8_t) v8_1;
  v8_2 = (int8_t) v64_2;
  uv8_2 = (uint8_t) v8_2;
  v16_1 = (int16_t) v64_1;
  uv16_1 = (uint16_t) v16_1;
  v16_2 = (int16_t) v64_2;
  uv16_2 = (uint16_t) v16_2;
  v32_1 = (int32_t) v64_1;
  uv32_1 = (uint32_t) v32_1;
  v32_2 = (int32_t) v64_2;
  uv32_2 = (uint32_t) v32_2;
  uv64_1 = (uint64_t) v64_1;
  uv64_2 = (uint64_t) v64_2;

  //
  // Compute the result of the bitwise operator
  //
  switch (op) {
    case SHL:
      switch (type) {
        case INT1_KIND:
        case INT8_KIND:
          result.as_int = uv8_1 << uv8_2;
          break;
        case INT16_KIND:
          result.as_int = uv16_1 << uv16_2;
          break;
        case INT24_KIND:
        case INT32_KIND:
          result.as_int = uv32_1 << uv32_2;
          break;
        case INT64_KIND:
          result.as_int = uv64_1 << uv64_2;
          break;
        default:
          DEBUG_STDERR("Unsupport integer type: " << type);
          safe_assert(false);
          return;
      }
      break;

    case LSHR:
      switch (type) {
        case INT1_KIND:
        case INT8_KIND:
          result.as_int = uv8_1 >> uv8_2;
          break;
        case INT16_KIND:
          result.as_int = uv16_1 >> uv16_2;
          break;
        case INT24_KIND:
        case INT32_KIND:
          result.as_int = uv32_1 >> uv32_2;
          break;
        case INT64_KIND:
          result.as_int = uv64_1 >> uv64_2;
          break;
        default:
          DEBUG_STDERR("Unsupport integer type: " << type);
          safe_assert(false);
          return;
      }
      break;

    case ASHR:
      switch (type) {
        case INT1_KIND:
        case INT8_KIND:
          result.as_int = uv8_1 >> uv8_2;
          break;
        case INT16_KIND:
          result.as_int = uv16_1 >> uv16_2;
          break;
        case INT24_KIND:
        case INT32_KIND:
          result.as_int = uv32_1 >> uv32_2;
          break;
        case INT64_KIND:
          result.as_int = uv64_1 >> uv64_2;
          break;
        default:
          DEBUG_STDERR("Unsupport integer type: " << type);
          safe_assert(false);
          return;
      }
      break;

    case AND:
      switch (type) {
        case INT1_KIND:
        case INT8_KIND:
          result.as_int = v8_1 & v8_2;
          break;
        case INT16_KIND:
          result.as_int = v16_1 & v16_2;
          break;
        case INT24_KIND:
        case INT32_KIND:
          result.as_int = v32_1 & v32_2;
          break;
        case INT64_KIND:
          result.as_int = v64_1 & v64_2;
          break;
        default:
          DEBUG_STDERR("Unsupport integer type: " << type);
          safe_assert(false);
          return;
      }
      break;

    case OR:
      switch (type) {
        case INT1_KIND:
        case INT8_KIND:
          result.as_int = v8_1 | v8_2;
          break;
        case INT16_KIND:
          result.as_int = v16_1 | v16_2;
          break;
        case INT24_KIND:
        case INT32_KIND:
          result.as_int = v32_1 | v32_2;
          break;
        case INT64_KIND:
          result.as_int = v64_1 | v64_2;
          break;
        default:
          DEBUG_STDERR("Unsupport integer type: " << type);
          safe_assert(false);
          return;
      }
      break;

    case XOR:
      switch (type) {
        case INT1_KIND:
        case INT8_KIND:
          result.as_int = v8_1 ^ v8_2;
          break;
        case INT16_KIND:
          result.as_int = v16_1 ^ v16_2;
          break;
        case INT24_KIND:
        case INT32_KIND:
          result.as_int = v32_1 ^ v32_2;
          break;
        case INT64_KIND:
          result.as_int = v64_1 ^ v64_2;
          break;
        default:
          DEBUG_STDERR("Unsupport integer type: " << type);
          safe_assert(false);
          return;
      }
      break;


    default:
      DEBUG_STDERR("Unsupport bitwise operator: " << BITWISE_ToString(op));
      safe_assert(false);
      return;
  }

  iResult = new IValue(type, result);
  iResult->setLineNumber(line);
  executionStack.top()[inx] = iResult;

  DEBUG_STDOUT(iResult->toString());

  return;
} 

void InterpreterObserver::shl(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx) {
  bitwise(lScope, rScope, lValue, rValue, type, line, inx, SHL);
}

void InterpreterObserver::lshr(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx) {
  bitwise(lScope, rScope, lValue, rValue, type, line, inx, LSHR);
}

void InterpreterObserver::ashr(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx) {
  bitwise(lScope, rScope, lValue, rValue, type, line, inx, ASHR);
}

void InterpreterObserver::and_(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx) {
  bitwise(lScope, rScope, lValue, rValue, type, line, inx, AND);
}

void InterpreterObserver::or_(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx) {
  bitwise(lScope, rScope, lValue, rValue, type, line, inx, OR);
}

void InterpreterObserver::xor_(SCOPE lScope, SCOPE rScope, int64_t lValue, int64_t rValue, KIND type, int line, int inx) {
  bitwise(lScope, rScope, lValue, rValue, type, line, inx, XOR);
}

// ***** Vector Operations ***** //

void InterpreterObserver::extractelement(IID iid UNUSED, KVALUE* op1 UNUSED, KVALUE* op2 UNUSED, int inx UNUSED) {
  DEBUG_STDOUT("Unimplemented function.");
  safe_assert(false);
}

void InterpreterObserver::insertelement() {
  DEBUG_STDOUT("Unimplemented function.");
  safe_assert(false);
}

void InterpreterObserver::shufflevector() {
  DEBUG_STDOUT("Unimplemented function.");
  safe_assert(false);
}


// ***** AGGREGATE OPERATIONS ***** //

void InterpreterObserver::extractvalue(IID iid UNUSED, int inx, int opinx) {
  int index, count; 
  IValue *aggIValue, *iResult;
  KVALUE *aggKValue;

  //
  // We expect only one index in the getElementPtrIndexList.
  //
  index = getElementPtrIndexList.front();
  getElementPtrIndexList.pop();
  safe_assert(getElementPtrIndexList.empty());

  //
  // Obtain KVALUE and IValue objects.
  //
  aggKValue = returnStruct.front();

  if (opinx == -1) {
    aggIValue = NULL;
  } else {
    aggIValue = aggKValue->isGlobal ? globalSymbolTable[opinx] :
      executionStack.top()[opinx];
  }

  count = 0;
  while (!returnStruct.empty()) {
    count++;
    returnStruct.pop();
    // obtain the KVALUE corresponding to the index
    if (count == index) {
      aggKValue = returnStruct.front(); 
    }
  }

  DEBUG_STDOUT("KVALUE: " << KVALUE_ToString(aggKValue));

  //
  // Compute the result
  //
  iResult = new IValue();
  if (aggIValue != NULL) { 
    aggIValue += index;
    aggIValue->copy(iResult);
  } else { // constant struct, use KVALUE to create iResult
    iResult->setType(aggKValue->kind);
    iResult->setValue(aggKValue->value);
  }

  executionStack.top()[inx] = iResult;

  DEBUG_STDOUT(iResult->toString());

  return;
}

void InterpreterObserver::insertvalue(IID iid UNUSED, KVALUE* op1 UNUSED, KVALUE* op2 UNUSED, int inx UNUSED) {
  DEBUG_STDOUT("Unimplemented function.");
  safe_assert(false);
}

// ***** Memory Access and Addressing Operations ***** //

void InterpreterObserver::allocax(IID iid UNUSED, KIND type, uint64_t size UNUSED, int inx, int line, bool arg UNUSED, KVALUE* actualAddress) {

  IValue *ptrLocation, *location;

  DEBUG_STDOUT("LOCAL alloca");

  // alloca for non-argument variables
  location = new IValue(type); // should we count it as LOCAL?
  location->setLength(0);

  VALUE value;
  value.as_ptr = actualAddress->value.as_ptr;
  ptrLocation = new IValue(PTR_KIND, value, LOCAL);
  ptrLocation->setValueOffset((int64_t)location - (int64_t)value.as_ptr);
  DEBUG_STDOUT("actual address: " << actualAddress->value.as_ptr);
  DEBUG_STDOUT("location" << location);

  ptrLocation->setSize(KIND_GetSize(type)); // put in constructor
  ptrLocation->setLength(1);
  ptrLocation->setLineNumber(line);
  executionStack.top()[inx] = ptrLocation;

  DEBUG_STDOUT("Location: " << location->toString());
  DEBUG_STDOUT(ptrLocation->toString());

  safe_assert(ptrLocation->getValueOffset() != -1);
  return;
}

void InterpreterObserver::allocax_array(IID iid UNUSED, KIND type, uint64_t size, int inx, int line, bool arg UNUSED, KVALUE* addr) {

  unsigned firstByte, bitOffset, length;

  firstByte = 0;
  bitOffset = 0;
  length = 0; 

  //
  // if array element is struct, get list of primitive types for each struct
  // element
  //
  uint64_t structSize = 1;
  if (type == STRUCT_KIND) {
    structSize = structType.size(); 
  }
  KIND* structKind = (KIND*) malloc(structSize*sizeof(KIND));
  if (type == STRUCT_KIND) {
    for (uint64_t i = 0; i < structSize; i++) {
      structKind[i] = structType.front();
      structType.pop();
    }
  }

  IValue* locArr = (IValue*) malloc(size*structSize*sizeof(IValue));
  for (uint64_t i = 0; i < size; i++) {
    if (type == STRUCT_KIND) {
      for (uint64_t j = 0; j < structSize; j++) {
        IValue* var = new IValue(structKind[j]);
        length++;
        var->setFirstByte(firstByte + bitOffset/8);
        var->setBitOffset(bitOffset%8);
        var->setLength(0);
        unsigned structType = structKind[j];
        firstByte += KIND_GetSize(structType);
        bitOffset = (structType == INT1_KIND) ? bitOffset + 1 : bitOffset;
        locArr[i*structSize+j] = *var;
      }
    } else {
      IValue* var = new IValue(type);
      length++;
      var->setFirstByte(firstByte + bitOffset/8);
      var->setBitOffset(bitOffset%8);
      var->setLength(0);
      firstByte += KIND_GetSize(type);
      bitOffset = (type == INT1_KIND) ? bitOffset + 1 : bitOffset;
      if (type == INT1_KIND) {
        bitOffset++;
      }
      locArr[i] = *var;
    }
  }

  IValue* locArrPtr = new IValue(PTR_KIND, addr->value, LOCAL);
  locArrPtr->setValueOffset((int64_t)locArr - (int64_t)locArrPtr->getPtrValue());
  locArrPtr->setSize(KIND_GetSize(locArr[0].getType()));
  locArrPtr->setLength(length);
  locArrPtr->setLineNumber(line);
  executionStack.top()[inx] = locArrPtr;

  DEBUG_STDOUT(executionStack.top()[inx]->toString());

  safe_assert(locArrPtr->getValueOffset() != -1);
  return;
}

void InterpreterObserver::allocax_struct(IID iid UNUSED, uint64_t size, int inx, int line, bool arg UNUSED, KVALUE* addr) {

  safe_assert(structType.size() == size);

  unsigned firstByte = 0;
  unsigned bitOffset = 0;
  unsigned length = 0;
  IValue* ptrToStructVar = (IValue*) malloc(size*sizeof(IValue));
  for (uint64_t i = 0; i < size; i++) {
    KIND type = structType.front();
    IValue* var = new IValue(type);
    var->setFirstByte(firstByte + bitOffset/8);
    var->setBitOffset(bitOffset%8);
    var->setLength(0);
    firstByte += KIND_GetSize(type);
    bitOffset = (type == INT1_KIND) ? bitOffset + 1 : bitOffset;
    length++;
    ptrToStructVar[i] = *var;
    structType.pop();
  }
  safe_assert(structType.empty());

  IValue* structPtrVar = new IValue(PTR_KIND, addr->value);
  structPtrVar->setValueOffset((int64_t) ptrToStructVar - (int64_t) structPtrVar->getPtrValue());
  structPtrVar->setSize(KIND_GetSize(ptrToStructVar[0].getType()));
  structPtrVar->setLength(length);
  structPtrVar->setLineNumber(line);

  executionStack.top()[inx] = structPtrVar;

  DEBUG_STDOUT(executionStack.top()[inx]->toString());

  safe_assert(structPtrVar->getValueOffset() != -1);
  return;
}

void InterpreterObserver::fence() {

  cerr << "[InterpreterObserver::fence] => Unimplemented\n";
  safe_assert(false);
}

void InterpreterObserver::cmpxchg(IID iid UNUSED, PTR addr UNUSED, KVALUE* kv1 UNUSED, KVALUE* kv2 UNUSED, int inx UNUSED) {

  cerr << "[InterpreterObserver::cmpxchg] => Unimplemented\n";
  safe_assert(false);
}

void InterpreterObserver::atomicrmw() {

  cerr << "[InterpreterObserver::atomicrmw] => Unimplemented\n";
  safe_assert(false);
}

void InterpreterObserver::getelementptr(IID iid UNUSED, bool inbound UNUSED, KVALUE* base, KVALUE* offset, KIND type, uint64_t size, bool loadGlobal, int loadInx, int line, int inx) {

  if (type == INT80_KIND) {
    DEBUG_STDERR("[getelementptr] Unsupported INT80_KIND");
    safe_assert(false);
    return; // otherwise compiler warning
  }


  IValue *basePtrLocation, *ptrLocation; 
  int index;
  int newOffset;
  bool reInit;

  //
  // get base pointer operand
  //
  if (base->inx == -1) {
    // constant base pointer
    basePtrLocation = new IValue(PTR_KIND, base->value, 0, 0, 0, 0);
  } else {
    basePtrLocation = base->isGlobal ? globalSymbolTable[base->inx] :
      executionStack.top()[base->inx];
  }

  DEBUG_STDOUT("\tPointer operand " << basePtrLocation->toString());

  //
  // get index operand
  // 
  index = offset->inx == -1 ? offset->value.as_int :
    executionStack.top()[offset->inx]->getValue().as_int; 

  //
  // compute new offset from base pointer in bytes 
  // 
  newOffset = (index * (size/8)) + basePtrLocation->getOffset();

  DEBUG_STDOUT("\tSize: " << size);
  DEBUG_STDOUT("\tBase Offset: " << basePtrLocation->getOffset());
  DEBUG_STDOUT("\tIndex: " << index);
  DEBUG_STDOUT("\tnewOffset: " << newOffset);

  //
  // check whether the pointer need to be (re)initialized.
  //
  reInit = false;
  if (basePtrLocation->isInitialized()) {
    IValue *array; 
    int length;

    array = (IValue*) basePtrLocation->getIPtrValue();
    length = basePtrLocation->getLength();
    // newOffset can be negative in case of negative index
    if (newOffset < 0 || newOffset + size/8 > array[length-1].getFirstByte() +
        KIND_GetSize(array[length-1].getType())) {
      reInit = true;
    }
  } else {
    reInit = true;
  }

  //
  // compute index, (re)initialized the pointer if neccessary
  //
  if (reInit) {
    DEBUG_STDOUT("\tPointer is re-initialized!");
    IValue *array, *newArray, *loadInst;
    int length, newLength, i, extraBytes;

    length = basePtrLocation->getLength();
    array = (IValue*) basePtrLocation->getIPtrValue();
    if (newOffset >= 0) { // newOffset is positive
      if (basePtrLocation->isInitialized()) {
        extraBytes = newOffset + size/8 -
          array[length-1].getFirstByte() + KIND_GetSize(array[length-1].getType()); 
      } else {
        extraBytes = newOffset + size/8;
      }
    } else { // newOffset is negative
      DEBUG_STDOUT("New offset is negative.");
      extraBytes = abs(newOffset);
    }

    DEBUG_STDOUT("Extra bytes: " << extraBytes);

    newLength = length + ceil((double)extraBytes/(double)(size/8));

    DEBUG_STDOUT("Old length: " << length);
    DEBUG_STDOUT("New length: " << newLength);

    newArray = (IValue *) malloc(newLength * sizeof(IValue));
    array = (IValue*) basePtrLocation->getIPtrValue();

    for (i = 0; i < newLength; i++) {
      IValue oldElement; 
      IValue *newElement; 
      VALUE value;

      value.as_int = 0;

      if (newOffset < 0) { // newOffset is negative, append at the beginning at the array new elements
        if (i < newLength - length) {
          newElement = new IValue(type, value);
          if (i == 0) {
            newElement->setFirstByte(0);
          } else {
            newElement->setFirstByte(newArray[i-1].getFirstByte() +
                KIND_GetSize(newArray[i-1].getType()));
          }
        } else {
          newElement = new IValue();
          oldElement = array[i];
          oldElement.copy(newElement);
          if (i == 0) {
            newElement->setFirstByte(0);
          } else {
            newElement->setFirstByte(newArray[i-1].getFirstByte() +
                KIND_GetSize(newArray[i-1].getType()));
          }
        }
      } else { // newOffset is positive, append at the end of the array new element
        if (i < length) {
          newElement = new IValue();
          oldElement = array[i];
          oldElement.copy(newElement);
          newElement->setFirstByte(oldElement.getFirstByte());
        } else {
          newElement = new IValue(type, value);
          if (i == 0) {
            newElement->setFirstByte(0);
          } else {
            newElement->setFirstByte(newArray[i-1].getFirstByte() +
                KIND_GetSize(newArray[i-1].getType()));
          }
        }
      }

      DEBUG_STDOUT("\tNew element at index " << i << " is: " <<
          newElement->toString());

      newArray[i] = *newElement;
    } 

    newOffset = newOffset < 0 ? 0 : newOffset;

    basePtrLocation->setLength(newLength);
    basePtrLocation->setSize(size/8);
    basePtrLocation->setValueOffset((int64_t) newArray - basePtrLocation->getValue().as_int);

    //
    // update load variable
    //
    if (loadInx != -1) {
      IValue *elem, *values;

      // TODO: load can also be a global variable
      loadInst = loadGlobal ? globalSymbolTable[loadInx] : executionStack.top()[loadInx];

      // retrieving source
      values = (IValue*)loadInst->getIPtrValue();
      elem = values + loadInst->getIndex();
      elem->setLength(basePtrLocation->getLength());
      elem->setSize(basePtrLocation->getSize());
      elem->setValueOffset(basePtrLocation->getValueOffset());
    }
  } 

  index = findIndex((IValue*) basePtrLocation->getIPtrValue(), newOffset,
      basePtrLocation->getLength()); 

  ptrLocation = new IValue(PTR_KIND, basePtrLocation->getValue(), size/8, newOffset, index, basePtrLocation->getLength());

  ptrLocation->setValueOffset(basePtrLocation->getValueOffset());
  ptrLocation->setLineNumber(line);

  executionStack.top()[inx] = ptrLocation;
  DEBUG_STDOUT(executionStack.top()[inx]->toString());
  return;
}

void InterpreterObserver::getelementptr_array(KVALUE* op, KIND kind UNUSED, int elementSize, int scopeInx01, int scopeInx02, int scopeInx03, int64_t valOrInx01, int64_t valOrInx02, int64_t valOrInx03, int size01 UNUSED, int size02,  int inx) {

  IValue* arrayElemPtr;
  int newOffset;

  if (op->inx == -1) {
    // TODO: review this
    // constant pointer
    // return a dummy object
    arrayElemPtr = new IValue(PTR_KIND, op->value, 0, 0, 0, 0);
    while (!getElementPtrIndexList.empty()) {
      getElementPtrIndexList.pop();
    }
    while (!arraySize.empty()) {
      arraySize.pop();
    }
  } else {
    IValue *ptrArray, *array;
    int *arraySizeVec, *indexVec;
    int index, arrayDim, getIndexNo, i, j;

    ptrArray = op->isGlobal ? globalSymbolTable[op->inx] : executionStack.top()[op->inx];
    array = static_cast<IValue*>(ptrArray->getIPtrValue());

    DEBUG_STDOUT("\tPointer operand: " << ptrArray->toString());

    //
    // compute the index for flatten array representation of
    // the program's multi-dimensional array
    //
    arrayDim = (size02 != -1) ? arraySize.size() + 2 : 1;

    if (scopeInx02 == SCOPE_INVALID) {
      scopeInx02 = CONSTANT;
      valOrInx02 = 0;
    } 
    
    if (scopeInx03 == SCOPE_INVALID) {
      getIndexNo = 1;
    } else {
      getIndexNo = getElementPtrIndexList.size() + 2;
    }

    DEBUG_STDOUT("arrayDim " << arrayDim);

    DEBUG_STDOUT("getIndexNo " << getIndexNo);

    safe_assert(getIndexNo != 0);

    arraySizeVec = (int*) malloc(getIndexNo * sizeof(int));

    indexVec = (int*) malloc(getIndexNo * sizeof(int));

    // the size of out-most dimension; 
    // we do not need this to compute the index
    if (size02 != -1) {
      arraySizeVec[0] = size02;
      i = 1;
      while (!arraySize.empty()) {
        if (i < getIndexNo) {
          arraySizeVec[i] = arraySize.front();
        }
        arraySize.pop();
        i++;
      }
      safe_assert(arraySize.empty());
    }

    arraySizeVec[getIndexNo-1] = 1; 

    for (i = 0; i < getIndexNo; i++) {
      for (j = i+1; j < getIndexNo; j++) {
        arraySizeVec[i] *= arraySizeVec[j]; 
      }
    }

    // the first index is for the pointer operand;
    array = array + actualValueToIntValue(scopeInx01, valOrInx01);
    safe_assert(scopeInx01 != SCOPE_INVALID);
    indexVec[0] = actualValueToIntValue(scopeInx02, valOrInx02);

    if (scopeInx03 != SCOPE_INVALID) {
      indexVec[1] = actualValueToIntValue(scopeInx03, valOrInx03);
      i = 2;
      while (!getElementPtrIndexList.empty()) {
        indexVec[i] = getElementPtrIndexList.front();
        getElementPtrIndexList.pop();
        i++;
      }
      safe_assert(getElementPtrIndexList.empty());
    }

    index = 0;
    for (i = 0; i < getIndexNo; i++) {
      index += indexVec[i] * arraySizeVec[i];
    }

    DEBUG_STDOUT("\tIndex: " << index);

    //
    // compute new offset for flatten array
    //
    newOffset = ptrArray->getOffset() + elementSize*index; 

    //
    // compute the index for the casted fatten array
    //

    if (ptrArray->isInitialized()) {
      index = findIndex((IValue*) ptrArray->getIPtrValue(), newOffset, ptrArray->getLength()); 
    }

    DEBUG_STDOUT("\tIndex: " << index);

    // TODO: revisit this
    if (index < (int) ptrArray->getLength()) {
      IValue* arrayElem = array + index;
      arrayElemPtr = new IValue(PTR_KIND, ptrArray->getValue());
      arrayElemPtr->setValueOffset(ptrArray->getValueOffset());
      arrayElemPtr->setIndex(index);
      arrayElemPtr->setLength(ptrArray->getLength());
      arrayElemPtr->setSize(KIND_GetSize(arrayElem[0].getType()));
      arrayElemPtr->setOffset(arrayElem[0].getFirstByte());
    } else {
      VALUE arrayElemPtrValue;
      arrayElemPtrValue.as_int = ptrArray->getValue().as_int + newOffset;
      arrayElemPtr = new IValue(PTR_KIND, arrayElemPtrValue, ptrArray->getSize(), 0, 0, 0);
      arrayElemPtr->setValueOffset((int64_t)arrayElemPtr - arrayElemPtr->getValue().as_int);
    }
  }

  safe_assert(getElementPtrIndexList.empty());
  executionStack.top()[inx] = arrayElemPtr;
  DEBUG_STDOUT(executionStack.top()[inx]->toString());
}

void InterpreterObserver::getelementptr_struct(IID iid UNUSED, bool inbound UNUSED, KVALUE* op, KIND kind UNUSED, KIND arrayKind UNUSED, int inx) {

  DEBUG_STDOUT("\tstructType size " << structType.size());

  IValue *structPtr, *structElemPtr; 
  int structElemNo, structSize, index, size, i, newOffset;
  int* structElemSize, *structElem;

  //
  // get the struct operand
  //
  structPtr = executionStack.top()[op->inx];
  structElemNo = structType.size();
  structElemSize = (int*) malloc(sizeof(int)*structElemNo);
  structElem = (int*) malloc(sizeof(int)*structElemNo);

  //
  // record struct element size
  // compute struct size
  //
  structSize = 0;
  i = 0;
  while (!structType.empty()) {
    structElemSize[i] = KIND_GetSize(structType.front());
    structElem[i] = structType.front();
    structSize += structElemSize[i];
    i++;
    structType.pop();
  }

  DEBUG_STDOUT("\tstructSize is " << structSize);

  DEBUG_STDOUT("\t" << structPtr->toString());

  // compute struct index
  DEBUG_STDOUT("\tsize of getElementPtrIndexList: " << getElementPtrIndexList.size());
  index = getElementPtrIndexList.front()*structElemNo;
  getElementPtrIndexList.pop();
  if (!getElementPtrIndexList.empty()) {
    unsigned i;
    for (i = 0; i < getElementPtrIndexList.front(); i++) {
      index = index + structElementSize.front();
      safe_assert(!structElementSize.empty());
      structElementSize.pop();
    }
  }
  if (!getElementPtrIndexList.empty()) {
    getElementPtrIndexList.pop();
  }
  while (!structElementSize.empty()) {
    structElementSize.pop();
  }
  safe_assert(getElementPtrIndexList.empty());

  DEBUG_STDOUT("\tIndex is " << index);

  newOffset = structSize * (index/structElemNo);
  for (i = 0; i < index % structElemNo; i++) {
    newOffset = newOffset + KIND_GetSize(structElem[i]);
  }

  newOffset = newOffset + structPtr->getOffset();

  size = KIND_GetSize(structElem[index % structElemNo]);


  DEBUG_STDOUT("\tNew offset is: " << newOffset);

  //
  // compute the result; consider two cases: the struct pointer operand is
  // initialized and is not initialized
  //
  if (structPtr->isInitialized()) {
    IValue* structBase = static_cast<IValue*>(structPtr->getIPtrValue());


    index = findIndex((IValue*) structPtr->getIPtrValue(), newOffset, structPtr->getLength()); // TODO: revise offset, getValue().as_ptr

    DEBUG_STDOUT("\tNew index is: " << index);

    // TODO: revisit this
    if (index < (int) structPtr->getLength()) {
      DEBUG_STDOUT("\tstructBase = " << structBase->toString());
      IValue* structElem = structBase + index;
      DEBUG_STDOUT("\tstructElem = " << structElem->toString());
      structElemPtr = new IValue(PTR_KIND, structPtr->getValue());
      structElemPtr->setValueOffset(structPtr->getValueOffset());
      structElemPtr->setIndex(index);
      structElemPtr->setLength(structPtr->getLength());
      structElemPtr->setSize(size);
      structElemPtr->setOffset(newOffset);
    } else {
      structElemPtr = new IValue(PTR_KIND, structPtr->getValue(), structPtr->getSize(), 0, 0, 0);
      structElemPtr->setValueOffset(structPtr->getValueOffset());
    }
  } else {
    DEBUG_STDOUT("\tPointer is not initialized");
    VALUE structElemPtrValue;

    // compute the value for the element pointer
    structElemPtrValue = structPtr->getValue();
    structElemPtrValue.as_int = structElemPtrValue.as_int + newOffset;

    structElemPtr = new IValue(PTR_KIND, structElemPtrValue, size, 0, 0, 0);
    structElemPtr->setValueOffset((int64_t)structElemPtr - structElemPtr->getValue().as_int);
  }

  executionStack.top()[inx] = structElemPtr;

  DEBUG_STDOUT(executionStack.top()[inx]->toString());
}

// ***** Conversion Operations ***** //

void InterpreterObserver::castop(int64_t opVal, SCOPE opScope, KIND opType, KIND type, int size, int inx, CASTOP op) {
  VALUE result;
  IValue *iOp, *iResult;
  int64_t v64, opIntValue, opPtrValue;
  uint64_t opUIntValue;
  int64_t *v64Ptr;
  int32_t v32, sign;
  double opFlpValue;

  //
  // assert: we do not support INT80 yet.
  //
  if (type == INT80_KIND || opType == INT80_KIND) {
    DEBUG_STDERR("Do not support INT80 type yet.");
    safe_assert(false);
  }

  //
  // Obtain value and type of the operand.
  //
  if (opScope == CONSTANT) {
    double *ptr;

    iOp = NULL; // compiler warning without this
    ptr = (double *)&op;

    opIntValue = op;
    opUIntValue = op;
    opFlpValue = *ptr;
    opPtrValue = op;

  } else {

    iOp = (opScope == GLOBAL) ? globalSymbolTable[opVal] :
      executionStack.top()[opVal];
    opIntValue = iOp->getIntValue();
    opUIntValue = iOp->getUIntValue();
    opFlpValue = iOp->getFlpValue();
    opPtrValue = iOp->getValue().as_int + iOp->getOffset();

  }

  //
  // Compute 64-bit, 32-bit and sign representation of value. 
  //
  v64 = opIntValue;
  v64Ptr = &v64;
  v32 = *((int32_t *) v64Ptr);
  sign = v32 & 0x1;

  //
  // Compute the result based on castop and type
  //
  switch (op) {
    case TRUNC:
      //
      // assert: size of opType is larger than or equal to (result) type.
      //
      safe_assert(KIND_GetSize(opType) >= KIND_GetSize(type));
      switch (type) {
        case INT1_KIND:
          result.as_int = sign;
          break;
        case INT8_KIND:
          result.as_int = v32 & 0x000000FF;
          break;
        case INT16_KIND:
          result.as_int = v32 & 0x0000FFFF;
          break;
        case INT24_KIND:
        case INT32_KIND:
          result.as_int = v32;
          break;
        case INT64_KIND:
          result.as_int = v64;
          break;
        default:
          DEBUG_STDERR("Unsupported integer type: " << KIND_ToString(type));
          safe_assert(false);
      }
      break;

    case ZEXT:
      //
      // assert: size of opType is smaller than or equal to (result) type.
      //
      safe_assert(KIND_GetSize(opType) <= KIND_GetSize(type));
      switch (type) {
        case INT1_KIND:
        case INT8_KIND:
        case INT16_KIND:
        case INT24_KIND:
        case INT32_KIND:
        case INT64_KIND:
          result.as_int = v64;
          break;
        default:
          DEBUG_STDERR("Unsupported integer type: " << KIND_ToString(type));
      }
      break;

    case SEXT:
      //
      // assert: size of opType is smaller than or equal to (result) type.
      //
      safe_assert(KIND_GetSize(opType) <= KIND_GetSize(type));
      switch (type) {
        case INT1_KIND:
        case INT8_KIND:
        case INT16_KIND:
        case INT24_KIND:
        case INT32_KIND:
        case INT64_KIND:
          DEBUG_STDERR("Sign extension is not completely implemented yet!");
          result.as_int = v64;
          break;
        default:
          DEBUG_STDERR("Unsupported integer type: " << KIND_ToString(type));
          safe_assert(false);
      }
      break;

    case FPTRUNC:
      //
      // assert: size of opType is larger than or equal to (result) type.
      //
      safe_assert(KIND_GetSize(opType) >= KIND_GetSize(type));
      switch (type) {
        case FLP32_KIND:
          result.as_flp = (float) opFlpValue;
          break;
        case FLP64_KIND:
          result.as_flp = (double) opFlpValue;
          break;
        case FLP80X86_KIND:
          result.as_flp = (double) opFlpValue;
          break;
        default:
          DEBUG_STDERR("Unsupported float type: " << KIND_ToString(type));
          safe_assert(false);
      }
      break;

    case FPEXT:
      //
      // assert: size of opType is smaller than or equal to (result) type.
      //
      safe_assert(KIND_GetSize(opType) <= KIND_GetSize(type));
      switch (type) {
        case FLP32_KIND:
          result.as_flp = (float) opFlpValue;
          break;
        case FLP64_KIND:
          result.as_flp = (double) opFlpValue;
          break;
        case FLP80X86_KIND:
          result.as_flp = (double) opFlpValue;
          break;
        default:
          DEBUG_STDERR("Unsupported float type: " << KIND_ToString(type));
          safe_assert(false);
      }
      break;
    case FPTOUI:
      switch (type) {
        case INT1_KIND:
          result.as_int = (bool) opFlpValue;
          break;
        case INT8_KIND:
          result.as_int = (uint8_t) opFlpValue;
          break;
        case INT16_KIND:
          result.as_int = (uint16_t) opFlpValue;
          break;
        case INT24_KIND:
        case INT32_KIND:
          result.as_int = (uint32_t) opFlpValue;
          break;
        case INT64_KIND:
          result.as_int = (uint64_t) opFlpValue;
          break;
        default:
          DEBUG_STDERR("Unsupported integer type: " << KIND_ToString(type));
          safe_assert(false);
      }
      break;

    case FPTOSI:
      switch (type) {
        case INT1_KIND:
          result.as_int = (bool) opFlpValue;
          break;
        case INT8_KIND:
          result.as_int = (int8_t) opFlpValue;
          break;
        case INT16_KIND:
          result.as_int = (int16_t) opFlpValue;
          break;
        case INT24_KIND:
        case INT32_KIND:
          result.as_int = (int32_t) opFlpValue;
          break;
        case INT64_KIND:
          result.as_int = (int64_t) opFlpValue;
          break;
        default:
          DEBUG_STDERR("Unsupported integer type: " << KIND_ToString(type));
          safe_assert(false);
      }
      break;

    case UITOFP:
      switch (type) {
        case FLP32_KIND:
          result.as_flp = (float) opUIntValue;
          break;
        case FLP64_KIND:
          result.as_flp = (double) opUIntValue;
          break;
        case FLP80X86_KIND:
          result.as_flp = (double) opUIntValue;
          break;
        default:
          DEBUG_STDERR("Unsupported float type: " << KIND_ToString(type));
          safe_assert(false);

      }
      break;

    case SITOFP:
      switch (type) {
        case FLP32_KIND:
          result.as_flp = (float) opIntValue;
          break;
        case FLP64_KIND:
          result.as_flp = (double) opIntValue;
          break;
        case FLP80X86_KIND:
          result.as_flp = (double) opIntValue;
          break;
        default:
          DEBUG_STDERR("Unsupported float type: " << KIND_ToString(type));
          safe_assert(false);

      }
      break;

    case PTRTOINT:
      switch (type) {
        case INT1_KIND:
          result.as_int = (bool) opPtrValue;
          break;
        case INT8_KIND:
          result.as_int = (int8_t) opPtrValue;
          break;
        case INT16_KIND:
          result.as_int = (int16_t) opPtrValue;
          break;
        case INT24_KIND:
        case INT32_KIND:
          result.as_int = (int32_t) opPtrValue;
          break;
        case INT64_KIND:
          result.as_int = opPtrValue;
          break;
        default:
          DEBUG_STDERR("Unsupported integer type: " << KIND_ToString(type));
          safe_assert(false);
      }
      break;

    case INTTOPTR:
      result.as_int = opIntValue;
      break;

    case BITCAST:
      break;

    default:
      DEBUG_STDERR("Unsupported conversion operator: " << CASTOP_ToString(op));
      safe_assert(false);
  } 

  if (op == BITCAST) {
    if (opScope == CONSTANT) {
      VALUE val;

      iResult = new IValue();
      val.as_int = op;
      iResult->setValue(val);
      iResult->setSize(size/8);
      iResult->setType(type);
    } else {
      iResult = new IValue();
      iOp->copy(iResult);
      iResult->setSize(size/8);
      iResult->setType(type);
    }
  } else {
    iResult = new IValue(type, result);
  }
  executionStack.top()[inx] = iResult;

  DEBUG_STDOUT(iResult->toString());

  return;
}

void InterpreterObserver::trunc(int64_t opVal, SCOPE opScope, KIND opKind, KIND kind, int size, int inx) {
  castop(opVal, opScope, opKind, kind, size, inx, TRUNC);
}

void InterpreterObserver::zext(int64_t opVal, SCOPE opScope, KIND opKind, KIND kind, int size, int inx) {
  castop(opVal, opScope, opKind, kind, size, inx, ZEXT);
}

void InterpreterObserver::sext(int64_t opVal, SCOPE opScope, KIND opKind, KIND kind, int size, int inx) {
  castop(opVal, opScope, opKind, kind, size, inx, SEXT);
}

void InterpreterObserver::fptrunc(int64_t opVal, SCOPE opScope, KIND opKind, KIND kind, int size, int inx) {
  castop(opVal, opScope, opKind, kind, size, inx, FPTRUNC);
}

void InterpreterObserver::fpext(int64_t opVal, SCOPE opScope, KIND opKind, KIND kind, int size, int inx) {
  castop(opVal, opScope, opKind, kind, size, inx, FPEXT);
}

void InterpreterObserver::fptoui(int64_t opVal, SCOPE opScope, KIND opKind, KIND kind, int size, int inx) {
  castop(opVal, opScope, opKind, kind, size, inx, FPTOUI);
}

void InterpreterObserver::fptosi(int64_t opVal, SCOPE opScope, KIND opKind, KIND kind, int size, int inx) {
  castop(opVal, opScope, opKind, kind, size, inx, FPTOSI);
}

void InterpreterObserver::uitofp(int64_t opVal, SCOPE opScope, KIND opKind, KIND kind, int size, int inx) {
  castop(opVal, opScope, opKind, kind, size, inx, UITOFP);
}

void InterpreterObserver::sitofp(int64_t opVal, SCOPE opScope, KIND opKind, KIND kind, int size, int inx) {
  castop(opVal, opScope, opKind, kind, size, inx, SITOFP);
}

void InterpreterObserver::ptrtoint(int64_t opVal, SCOPE opScope, KIND opKind, KIND kind, int size, int inx) {
  castop(opVal, opScope, opKind, kind, size, inx, PTRTOINT);
}

void InterpreterObserver::inttoptr(int64_t opVal, SCOPE opScope, KIND opKind, KIND kind, int size, int inx) {
  castop(opVal, opScope, opKind, kind, size, inx, INTTOPTR);
}

void InterpreterObserver::bitcast(int64_t opVal, SCOPE opScope, KIND opKind, KIND kind, int size, int inx) {
  castop(opVal, opScope, opKind, kind, size, inx, BITCAST);
}

// ***** TerminatorInst ***** //
void InterpreterObserver::branch(IID iid UNUSED, bool conditional UNUSED, KVALUE* op1, int inx UNUSED) {

  IValue* cond = (op1->inx == -1) ? NULL : executionStack.top()[op1->inx];

  if (cond != NULL && ((bool) cond->getIntValue() != (bool) op1->value.as_int)) {
    DEBUG_STDERR("\tKVALUE: " << KVALUE_ToString(op1));
    DEBUG_STDERR("\tIVALUE: " << cond->toString());

    DEBUG_STDERR("\tShadow and concrete executions diverge at this branch.");
    safe_assert(false);
  }
}

void InterpreterObserver::branch2(IID iid UNUSED, bool conditional UNUSED, int inx UNUSED) {
}

void InterpreterObserver::indirectbr(IID iid UNUSED, KVALUE* op1 UNUSED, int inx UNUSED) {
}

void InterpreterObserver::invoke(IID iid UNUSED, KVALUE* call_value UNUSED, int inx UNUSED) {
  int count; 

  count = 0;
  while (!myStack.empty()) {
    KVALUE* argument; 

    argument = myStack.top();
    DEBUG_STDOUT("\t Argument " << count << ": " << KVALUE_ToString(argument));
    myStack.pop();
  }

  DEBUG_STDERR("Unimplemented function.");
  safe_assert(false);
}

void InterpreterObserver::resume(IID iid UNUSED, KVALUE* op1 UNUSED, int inx UNUSED) {
  DEBUG_STDERR("Unimplemented function.");
  safe_assert(false);
}

void InterpreterObserver::return_(IID iid UNUSED, KVALUE* op1, int inx UNUSED) {
  safe_assert(!executionStack.empty());

  IValue* returnValue = op1->inx == -1 ? NULL : executionStack.top()[op1->inx];

  executionStack.pop();

  if (!executionStack.empty()) {
    DEBUG_STDOUT("New stack size: " << executionStack.size());
    safe_assert(!callerVarIndex.empty());

    if (returnValue == NULL) {
      executionStack.top()[callerVarIndex.top()]->setValue(op1->value); 
      executionStack.top()[callerVarIndex.top()]->setType(op1->kind); 
    } else {
      returnValue->copy(executionStack.top()[callerVarIndex.top()]);
    }
    DEBUG_STDOUT(executionStack.top()[callerVarIndex.top()]->toString());

    callerVarIndex.pop();
  } else {
    cout << "The execution stack is empty.\n";
  }

  isReturn = true;

  return;
}

void InterpreterObserver::return2_(IID iid UNUSED, int inx UNUSED) {

  safe_assert(!executionStack.empty());
  executionStack.pop();

  if (!executionStack.empty()) {
    DEBUG_STDOUT("New stack size: " << executionStack.size());
  } else {
    cout << "The execution stack is empty.\n";
  }

  isReturn = true;

  return;
}

void InterpreterObserver::return_struct_(IID iid UNUSED, int inx UNUSED, int valInx) {

  safe_assert(!executionStack.empty());

  IValue* returnValue = (valInx == -1) ? NULL : executionStack.top()[valInx];

  executionStack.pop();

  if (!executionStack.empty()) {
    DEBUG_STDOUT("New stack size: " << executionStack.size());
    safe_assert(!callerVarIndex.empty());
    safe_assert(!returnStruct.empty());

    // reconstruct struct value
    unsigned size = returnStruct.size();
    IValue* structValue = (IValue*) malloc(returnStruct.size()*sizeof(IValue));
    unsigned i = 0;
    while (!returnStruct.empty()) {
      KVALUE* value = returnStruct.front();
      IValue* iValue;

      if (returnValue == NULL) {
        iValue = new IValue(value->kind);
        iValue->setValue(value->value);
        iValue->setLength(0);
      } else {
        iValue = new IValue();
        returnValue->copy(iValue);
        returnValue++;
      }

      structValue[i] = *iValue; 
      DEBUG_STDOUT(cout << structValue[i].toString());
      i++;
      returnStruct.pop();
    }

    safe_assert(returnStruct.empty());

    executionStack.top()[callerVarIndex.top()] = structValue;
    for (i = 0; i < size; i++)
      DEBUG_STDOUT(executionStack.top()[callerVarIndex.top()][i].toString());
  } else {
    cout << "The execution stack is empty.\n";
  }

  safe_assert(!callerVarIndex.empty());
  callerVarIndex.pop();

  isReturn = true;

  return;
}

void InterpreterObserver::switch_(IID iid UNUSED, KVALUE* op UNUSED, int inx UNUSED) {
}

void InterpreterObserver::unreachable() {
}

// ***** Other Operations ***** //

void InterpreterObserver::icmp(IID iid UNUSED, KVALUE* op1, KVALUE* op2, PRED pred, int inx) {
  if (op1->kind == INT80_KIND || op2->kind == INT80_KIND) {
    cout << "[icmp] Unsupported INT80_KIND" << endl;
    safe_assert(false);
    return;
  }

  int64_t v1, v2;

  // get value of v1
  if (op1->inx == -1) { // constant
    v1 = op1->value.as_int;
  } else {
    IValue *loc1 = op1->isGlobal ? globalSymbolTable[op1->inx] :
      executionStack.top()[op1->inx];
    v1 = loc1->getType() == PTR_KIND ? loc1->getIntValue() + loc1->getOffset()
      : loc1->getIntValue();
  }

  // get value of v2
  if (op2->inx == -1) { // constant
    v2 = op2->value.as_int;
  } else {
    IValue *loc2 = op2->isGlobal ? globalSymbolTable[op2->inx] :
      executionStack.top()[op2->inx];
    v2 = loc2->getType() == PTR_KIND ? loc2->getIntValue() + loc2->getOffset()
      : loc2->getIntValue();
  } 

  DEBUG_STDOUT("=============" << v1);
  DEBUG_STDOUT("=============" << v2);

  int result = 0;
  switch(pred) {
    case CmpInst::ICMP_EQ:
      DEBUG_STDOUT("PRED = ICMP_EQ");
      result = v1 == v2;
      break;
    case CmpInst::ICMP_NE:
      DEBUG_STDOUT("PRED = ICMP_NE");
      result = v1 != v2;
      break;
    case CmpInst::ICMP_UGT:
      DEBUG_STDOUT("PRED = ICMP_UGT");
      result = (uint64_t)v1 > (uint64_t)v2;
      break;
    case CmpInst::ICMP_UGE:
      DEBUG_STDOUT("PRED = ICMP_UGE");
      result = (uint64_t)v1 >= (uint64_t)v2;
      break;
    case CmpInst::ICMP_ULT:
      DEBUG_STDOUT("PRED = ICMP_ULT");
      result = (uint64_t)v1 < (uint64_t)v2;
      break;
    case CmpInst::ICMP_ULE:
      DEBUG_STDOUT("PRED = ICMP_ULE");
      result = (uint64_t)v1 <= (uint64_t)v2;
      break;
    case CmpInst::ICMP_SGT:
      DEBUG_STDOUT("PRED = ICMP_SGT");
      result = v1 > v2;
      break;
    case CmpInst::ICMP_SGE:
      DEBUG_STDOUT("PRED = ICMP_SGE");
      result = v1 >= v2;
      break;
    case CmpInst::ICMP_SLT:
      DEBUG_STDOUT("PRED = ICMP_SLT");
      result = v1 < v2;
      break;
    case CmpInst::ICMP_SLE:
      DEBUG_STDOUT("PRED = ICMP_SLE");
      result = v1 <= v2;
      break;
    default:
      safe_assert(false);
      break;
  }

  VALUE vresult;
  vresult.as_int = result;

  IValue *nloc = new IValue(INT1_KIND, vresult);
  nloc->setSize(KIND_GetSize(INT1_KIND));

  executionStack.top()[inx] = nloc;
  DEBUG_STDOUT(nloc->toString());
  return;
}

void InterpreterObserver::fcmp(IID iid UNUSED, KVALUE* op1, KVALUE* op2, PRED pred, int inx) {
  double v1, v2;

  // get value of v1
  if (op1->inx == -1) { // constant
    v1 = KVALUE_ToFlpValue(op1);
  } else {
    IValue *loc1 = op1->isGlobal ? globalSymbolTable[op1->inx] :
      executionStack.top()[op1->inx];
    v1 = loc1->getFlpValue();
  } 

  // get value of v2
  if (op2->inx == -1) { // constant
    v2 = KVALUE_ToFlpValue(op2);
  } else {
    IValue *loc2 = op2->isGlobal ? globalSymbolTable[op2->inx] :
      executionStack.top()[op2->inx];
    v2 = loc2->getFlpValue();
  } 

  DEBUG_STDOUT("=============" << v1);
  DEBUG_STDOUT("=============" << v2);

  int result = 0;
  switch(pred) {
    case CmpInst::FCMP_FALSE:
      DEBUG_STDOUT("\tCondition is FALSE");
      result = 0;
      break;
    case CmpInst::FCMP_TRUE:
      DEBUG_STDOUT("\tCondition is TRUE");
      result = 1;
      break;
    case CmpInst::FCMP_UEQ:
      DEBUG_STDOUT("\tCondition is UEQ");
      result = v1 == v2;
      break;
    case CmpInst::FCMP_UNE:
      DEBUG_STDOUT("\tCondition is UNE");
      result = v1 != v2;
      break;
    case CmpInst::FCMP_UGT:
      DEBUG_STDOUT("\tCondition is UGT");
      result = v1 > v2;
      break;
    case CmpInst::FCMP_UGE:
      DEBUG_STDOUT("\tCondition is UGE");
      result = v1 >= v2;
      break;
    case CmpInst::FCMP_ULT:
      DEBUG_STDOUT("\tCondition is ULT");
      result = v1 < v2;
      break;
    case CmpInst::FCMP_ULE:
      DEBUG_STDOUT("\tCondition is ULT");
      result = v1 <= v2;
      break;
    case CmpInst::FCMP_OEQ:
      DEBUG_STDOUT("\tCondition is OEQ");
      result = v1 == v2;
      break;
    case CmpInst::FCMP_ONE:
      DEBUG_STDOUT("\tCondition is ONE"); 
      result = v1 != v2;
      break;
    case CmpInst::FCMP_OGT:
      DEBUG_STDOUT("\tCondition is OGT");
      result = v1 > v2;
      break;
    case CmpInst::FCMP_OGE:
      DEBUG_STDOUT("\tCondition is OGE");
      result = v1 >= v2;
      break;
    case CmpInst::FCMP_OLT:
      DEBUG_STDOUT("\tCondition is OLT");
      result = v1 < v2;
      break;
    case CmpInst::FCMP_OLE:
      DEBUG_STDOUT("\tCondition is OLE");
      result = v1 <= v2;
      break;
    default:
      safe_assert(false);
      break;
  }

  // put result back to VALUE
  // TODO: incomplete?!
  VALUE vresult;
  vresult.as_int = result;

  IValue *nloc = new IValue(INT1_KIND, vresult);
  executionStack.top()[inx] = nloc;
  DEBUG_STDOUT(nloc->toString());

  return;
}

void InterpreterObserver::phinode(IID iid UNUSED, int inx) {

  DEBUG_STDOUT("Recent block: " << recentBlock.top());

  IValue* phiNode;

  if (phinodeConstantValues.find(recentBlock.top()) != phinodeConstantValues.end()) {
    KVALUE* constant = phinodeConstantValues[recentBlock.top()];
    phiNode = new IValue(constant->kind, constant->value);
    phiNode->setLength(0);
  } else {
    safe_assert(phinodeValues.find(recentBlock.top()) != phinodeValues.end());
    IValue* inValue = executionStack.top()[phinodeValues[recentBlock.top()]];
    phiNode = new IValue();
    inValue->copy(phiNode);
  }

  phinodeConstantValues.clear();
  phinodeValues.clear();

  executionStack.top()[inx] = phiNode;
  DEBUG_STDOUT(phiNode->toString());

  return;
}

void InterpreterObserver::select(IID iid UNUSED, KVALUE* cond, KVALUE* tvalue, KVALUE* fvalue, int inx) {

  int condition;
  IValue *conditionValue, *trueValue, *falseValue, *result;

  if (cond->inx == -1) {
    condition = cond->value.as_int;
  } else {
    conditionValue = cond->isGlobal ? globalSymbolTable[cond->inx] : executionStack.top()[cond->inx];
    condition = conditionValue->getValue().as_int;
  }


  if (condition) {
    if (tvalue->inx == -1) {
      result = new IValue(tvalue->kind, tvalue->value, REGISTER);
    } else {
      result = new IValue();
      trueValue = tvalue->isGlobal ? globalSymbolTable[tvalue->inx] :
        executionStack.top()[tvalue->inx];
      trueValue->copy(result);
    }
  } else {
    if (fvalue->inx == -1) {
      result = new IValue(fvalue->kind, fvalue->value, REGISTER);
    } else {
      result = new IValue();
      falseValue = fvalue->isGlobal ? globalSymbolTable[fvalue->inx] : executionStack.top()[fvalue->inx];
      falseValue->copy(result);
    }
  }

  executionStack.top()[inx] = result;

  DEBUG_STDOUT("Result is " << result->toString());
  return;
}

void InterpreterObserver::push_string(int diff) {
  char c = (char)(((int)'0') + diff);
  logName.push(c);
}

void InterpreterObserver::push_stack(KVALUE* value) {

  myStack.push(value);
}

void InterpreterObserver::push_phinode_constant_value(KVALUE* value, int blockId) {

  phinodeConstantValues[blockId] = value;
}

void InterpreterObserver::push_phinode_value(int valId, int blockId) {

  phinodeValues[blockId] = valId;
}

void InterpreterObserver::push_return_struct(KVALUE* value) {

  returnStruct.push(value);
}

void InterpreterObserver::push_struct_type(KIND kind) {

  structType.push(kind);
}

void InterpreterObserver::push_struct_element_size(uint64_t s) {

  structElementSize.push(s);
}

void InterpreterObserver::push_getelementptr_inx(KVALUE* int_value) {
  int idx = int_value->value.as_int;

  getElementPtrIndexList.push(idx);
}

int InterpreterObserver::actualValueToIntValue(int scope, int64_t vori) {
  switch (scope) {
    case CONSTANT:
      return vori;
    case LOCAL:
      return executionStack.top()[vori]->getIntValue();
    case GLOBAL:
      return globalSymbolTable[vori]->getIntValue();
    default:
      return -1;
  }
}

void InterpreterObserver::push_getelementptr_inx5(int scope01, int scope02, int
    scope03, int scope04, int scope05, int64_t vori01, int64_t vori02, int64_t
    vori03, int64_t vori04, int64_t vori05) {
  int v1, v2, v3, v4, v5;

  v1 = actualValueToIntValue(scope01, vori01);
  v2 = actualValueToIntValue(scope02, vori02);
  v3 = actualValueToIntValue(scope03, vori03);
  v4 = actualValueToIntValue(scope04, vori04);
  v5 = actualValueToIntValue(scope05, vori05);

  if (scope01 != SCOPE_INVALID) {
    getElementPtrIndexList.push(v1);
    if (scope02 != SCOPE_INVALID) {
      getElementPtrIndexList.push(v2);
      if (scope03 != SCOPE_INVALID) {
        getElementPtrIndexList.push(v3);
        if (scope04 != SCOPE_INVALID) {
          getElementPtrIndexList.push(v4);
          if (scope05 != SCOPE_INVALID) {
            getElementPtrIndexList.push(v5);
          }
        }
      }
    }
  }
};

void InterpreterObserver::push_getelementptr_inx2(int int_value) {
  int idx = int_value;

  getElementPtrIndexList.push(idx);
}

void InterpreterObserver::push_array_size(uint64_t size) {

  arraySize.push(size);
}

void InterpreterObserver::push_array_size5(int s1, int s2, int s3, int s4, int s5) {
  if (s1 != -1) {
    arraySize.push(s1);
    if (s2 != -1) {
      arraySize.push(s2);
      if (s3 != -1) {
        arraySize.push(s3);
        if (s4 != -1) {
          arraySize.push(s4);
          if (s5 != -1) {
            arraySize.push(s5);
          }
        }
      }
    }
  }
}

void InterpreterObserver::after_call(KVALUE* kvalue) {

  if (!isReturn) {
    // call is not interpreted
    safe_assert(!callerVarIndex.empty());

    // empty myStack and callArgs
    while (!myStack.empty()) {
      myStack.pop();
    }
    while (!callArgs.empty()) {
      callArgs.pop();
    }

    IValue* reg = executionStack.top()[callerVarIndex.top()];
    reg->setValue(kvalue->value);
    reg->setValueOffset(0); // new
    callerVarIndex.pop();

    DEBUG_STDOUT(reg->toString());
  } else {
    safe_assert(callArgs.empty());
    safe_assert(myStack.empty());
  }

  isReturn = false;

  safe_assert(!recentBlock.empty());
  recentBlock.pop();
}

void InterpreterObserver::after_void_call() {

  isReturn = false;

  safe_assert(!recentBlock.empty());
  recentBlock.pop();

  // empty myStack and callArgs
  while (!myStack.empty()) {
    myStack.pop();
  }
  while (!callArgs.empty()) {
    callArgs.pop();
  }
}

void InterpreterObserver::after_struct_call() {

  if (!isReturn) {
    // call is not interpreted
    safe_assert(!callerVarIndex.empty());

    // empty myStack and callArgs
    while (!myStack.empty()) {
      myStack.pop();
    }
    while (!callArgs.empty()) {
      callArgs.pop();
    }

    safe_assert(!returnStruct.empty());

    // reconstruct struct value
    IValue* structValue = (IValue*) malloc(returnStruct.size()*sizeof(IValue));
    unsigned i = 0;
    while (!returnStruct.empty()) {
      KVALUE* value = returnStruct.front();
      IValue* iValue = new IValue(value->kind);
      iValue->setValue(value->value);
      iValue->setLength(0);

      structValue[i] = *iValue; 
      i++;
      returnStruct.pop();
    }

    safe_assert(returnStruct.empty());


    executionStack.top()[callerVarIndex.top()] = structValue;

    DEBUG_STDOUT(executionStack.top()[callerVarIndex.top()]->toString());

    callerVarIndex.pop();
  } else {
    while (!returnStruct.empty()) {
      returnStruct.pop();
    }
    safe_assert(callArgs.empty());
    safe_assert(myStack.empty());
    safe_assert(returnStruct.empty());
  }

  isReturn = false;

  safe_assert(!recentBlock.empty());
  recentBlock.pop();

}

void InterpreterObserver::create_stack_frame(int size) {

  isReturn = false;

  std::vector<IValue*> frame (size);
  for (int i = 0; i < size; i++) {
    if (!callArgs.empty()) {
      frame[i] = callArgs.top();
      DEBUG_STDOUT("\t Argument " << i << ": " << frame[i]->toString());
      callArgs.pop();
    } else {
      frame[i] = new IValue();
    }
  }
  safe_assert(callArgs.empty());
  executionStack.push(frame);
}

void InterpreterObserver::create_global_symbol_table(int size) {

  //
  // instantiate copyShadow
  //
  IValue::setCopyShadow(&copyShadow);

  //
  // get log name
  //
  int length, i;
  char* log;

  length = logName.size();
  log = (char*) malloc(sizeof(char) * (length+1));

  for (i = 0; i < length; i++) {
    log[length-i-1] = logName.top();
    logName.pop();
  }

  log[length] = '\0';

  // initialize logger
  google::InitGoogleLogging(log);
  LOG(INFO) << "Initialized logger" << endl;

  for (int i = 0; i < size; i++) {
    IValue* value = new IValue();
    globalSymbolTable.push_back(value);
  }
  return;
}

void InterpreterObserver::record_block_id(int id) {

  if (recentBlock.empty()) {
    recentBlock.push(id);
  } else {
    recentBlock.pop();
    recentBlock.push(id);
  }
}

void InterpreterObserver::create_global(KVALUE* kvalue, KVALUE* initializer) {

  // allocate object
  IValue* location;
  location = new IValue(initializer->kind, initializer->value, GLOBAL); // GLOBAL?

  VALUE value;
  value.as_ptr = kvalue->value.as_ptr;
  IValue* ptrLocation = new IValue(PTR_KIND, value, GLOBAL);
  ptrLocation->setSize(KIND_GetSize(initializer->kind)); // put in constructor
  if (location->getType() != PTR_KIND) {
    ptrLocation->setLength(1);
  }
  ptrLocation->setValueOffset((int64_t)location - value.as_int);

  // store it in globalSymbolTable
  globalSymbolTable[kvalue->inx] = ptrLocation;
  DEBUG_STDOUT("\tloc: " << location->toString());
  DEBUG_STDOUT("\tptr: " << ptrLocation->toString());
}

void InterpreterObserver::call(IID iid UNUSED, bool nounwind UNUSED, KIND type, int inx) {

  while (!myStack.empty()) {
    KVALUE* value = myStack.top();
    myStack.pop();

    DEBUG_STDOUT(", arg: " << KVALUE_ToString(value).c_str());

    IValue* argCopy;
    if (value->inx != -1) {
      IValue* arg = value->isGlobal ? globalSymbolTable[value->inx] :
        executionStack.top()[value->inx];
      safe_assert(arg);
      argCopy = new IValue();
      arg->copy(argCopy);
    } else {
      // argument is a constant
      argCopy = new IValue(value->kind, value->value, LOCAL);
      argCopy->setLength(0); // uninitialized pointer
    }
    callArgs.push(argCopy);
  }

  if (type != VOID_KIND) {
    callerVarIndex.push(inx); 
  }

  IValue* callValue = new IValue(type);
  callValue->setLength(0);
  executionStack.top()[inx] = callValue;

  DEBUG_STDOUT(executionStack.top()[inx]->toString());

  // new recentBLock stack frame for the new call
  recentBlock.push(0);
}


void InterpreterObserver::call_malloc(IID iid UNUSED, bool nounwind UNUSED, KIND type, KVALUE* call_value UNUSED, int size, int inx, KVALUE* mallocAddress) {

  // retrieving original number of bytes
  KVALUE* argValue = myStack.top();
  myStack.pop();
  assert(myStack.size() == 0);
  
  if (type != STRUCT_KIND) {
    // allocating space
    int numObjects = argValue->value.as_int*8 / size;
    int actualSize = sizeof(IValue) * numObjects;    
    void *addr = malloc(actualSize);

    // creating pointer object
    VALUE returnValue;
    returnValue.as_ptr = mallocAddress->value.as_ptr;
    IValue* newPointer = new IValue(PTR_KIND, returnValue, size/8, 0, 0, numObjects);
    newPointer->setValueOffset((int64_t)addr - (int64_t)returnValue.as_ptr);
    executionStack.top()[inx] = newPointer;

    // creating locations
    unsigned currOffset = 0;
    for(int i = 0; i < numObjects; i++) {
      // creating object
      VALUE iValue;
      // TODO: check if we need uninitialized value
      IValue *var = new IValue(type, iValue, currOffset);
      ((IValue*)addr)[i] = *var;
      ((IValue*)addr)[i].setValueOffset(newPointer->getValueOffset()); //setting basepointer value offset?

      // updating offset
      currOffset += (size/8);
    }

    DEBUG_STDOUT(executionStack.top()[inx]->toString());
  } else {

    // allocating space
    unsigned numStructs = ceil(argValue->value.as_int*8.0 / size);
    unsigned fields = structType.size();

    int actualSize = sizeof(IValue) * numStructs * fields;    
    void *addr = malloc(actualSize);
    IValue* ptrToStructVar = (IValue*)addr;

    DEBUG_STDOUT("\nTotal size of malloc in bits: " << argValue->value.as_int*8);
    DEBUG_STDOUT("Size: " << size);
    DEBUG_STDOUT("Num Structs: " << numStructs);
    DEBUG_STDOUT("Number of fields: " << fields);

    KIND fieldTypes[fields];
    for(unsigned i = 0; i < fields; i++) {
      fieldTypes[i] = structType.front();
      structType.pop();
    }

    unsigned length = 0;
    unsigned firstByte = 0;
    for(unsigned i = 0; i < numStructs; i++) {
      for (unsigned j = 0; j < fields; j++) {
        KIND type = fieldTypes[j];
        IValue* var = new IValue(type);
        var->setFirstByte(firstByte);
        firstByte = firstByte + KIND_GetSize(type);
        ptrToStructVar[length] = *var;
        DEBUG_STDOUT("Created a field of struct: " << length);
        DEBUG_STDOUT(ptrToStructVar[length].toString());
        length++;
      }
    }

    safe_assert(structType.empty());

    VALUE structPtrVal;
    structPtrVal.as_ptr = mallocAddress->value.as_ptr;
    IValue* structPtrVar = new IValue(PTR_KIND, structPtrVal);
    structPtrVar->setValueOffset((int64_t)ptrToStructVar - (int64_t)mallocAddress->value.as_ptr);  ////////////
    structPtrVar->setSize(KIND_GetSize(ptrToStructVar[0].getType()));
    structPtrVar->setLength(length);

    executionStack.top()[inx] = structPtrVar;
    DEBUG_STDOUT(structPtrVar->toString());
  }

  return;
}

void InterpreterObserver::vaarg() {

  cerr << "[InterpreterObserver::vaarg] => Unimplemented\n";
  safe_assert(false);
}

void InterpreterObserver::landingpad() {

  cerr << "[InterpreterObserver::landingpad] => Unimplemented\n";
  safe_assert(false);
}

void InterpreterObserver::printCurrentFrame() {
  DEBUG_STDOUT("Print current frame.");
}

/**
 *
 * @param iValue the interpreted iValue of the concrete value
 * @param concrete pointer to the concrete value
 */
bool InterpreterObserver::syncLoad(IValue* iValue, KVALUE* concrete, KIND type) { 
  bool sync = false;
  VALUE syncValue;
  int64_t cValueVoid;
  int8_t* cValueInt8Arr;
  int16_t cValueInt16;
  int16_t* cValueInt16Arr;
  int32_t cValueInt32;
  int32_t* cValueInt32Arr;
  int64_t cValueInt64;
  float cValueFloat;
  double cValueDouble;
  double cValueLD;

  switch (type) {
    case PTR_KIND:
      // TODO: we use int64_t to represent a void* here
      // might not work on 32 bit machine
      cValueVoid = *((int64_t*) concrete->value.as_ptr);

      sync = (iValue->getValue().as_int + iValue->getOffset() != cValueVoid);
      if (sync) {
        syncValue.as_int = cValueVoid;
        iValue->setValue(syncValue);
      }
      break;
    case INT1_KIND: 
    case INT8_KIND: 
      cValueInt32 = *((int32_t*) concrete->value.as_ptr);
      cValueInt32 = cValueInt32 & 0x000000FF;

      sync = (((int8_t) iValue->getIntValue()) != cValueInt32);
      if (sync) {
        cValueInt8Arr = (int8_t*) calloc(8, sizeof(int8_t));
        cValueInt8Arr[0] = cValueInt32;
        syncValue.as_int = *((int64_t*) cValueInt8Arr);
        iValue->setValue(syncValue);
      }
      break;
    case INT16_KIND: 
      cValueInt16 = *((int16_t*) concrete->value.as_ptr);
      sync = (((int16_t) iValue->getIntValue()) != cValueInt16);
      if (sync) {
        cValueInt16Arr = (int16_t*) calloc(4, sizeof(int16_t)); 
        cValueInt16Arr[0] = cValueInt16; 
        syncValue.as_int = *((int64_t*) cValueInt16Arr);
        iValue->setValue(syncValue);
      }
      break;
    case INT24_KIND:
      cValueInt32 = *((int32_t*) concrete->value.as_ptr);
      cValueInt32 = cValueInt32 & 0x00FFFFFF;
      sync = (((int32_t) iValue->getIntValue()) != cValueInt32);
      if (sync) {
        cValueInt32Arr = (int32_t*) calloc(2, sizeof(int32_t));
        cValueInt32Arr[0] = cValueInt32; 
        syncValue.as_int = *((int32_t*) cValueInt32Arr);
        iValue->setValue(syncValue);
      }
      break;
    case INT32_KIND: 
      cValueInt32 = *((int32_t*) concrete->value.as_ptr);
      sync = (((int32_t) iValue->getIntValue()) != cValueInt32);
      if (sync) {
        cValueInt32Arr = (int32_t*) calloc(2, sizeof(int32_t));
        cValueInt32Arr[0] = cValueInt32; 
        syncValue.as_int = *((int32_t*) cValueInt32Arr);
        iValue->setValue(syncValue);
      }
      break;
    case INT64_KIND:
      cValueInt64 = *((int64_t*) concrete->value.as_ptr);
      sync = (iValue->getValue().as_int != cValueInt64);
      if (sync) {
        syncValue.as_int = cValueInt64;
        iValue->setValue(syncValue);
      }
      break;
    case INT80_KIND:
      cout << "[syncload] Unsupported INT80_KIND" << endl;
      safe_assert(false);
      break;
    case FLP32_KIND:
      cValueFloat = *((float*) concrete->value.as_ptr);
      if (isnan((float)iValue->getValue().as_flp) && isnan(cValueFloat)) {
	sync = false;
      }
      else {
	sync = ((float)iValue->getValue().as_flp != cValueFloat);
      }
      if (sync) {
        syncValue.as_flp = cValueFloat;
        iValue->setValue(syncValue);
      }
      break;
    case FLP64_KIND:
      cValueDouble = *((double*) concrete->value.as_ptr);
      if (isnan((double)iValue->getValue().as_flp) && isnan(cValueDouble)) {
        sync = false;
      }
      else {
        sync = ((double)iValue->getValue().as_flp != cValueDouble);
      }
      if (sync) {
        syncValue.as_flp = cValueDouble;
        iValue->setValue(syncValue);
      }
      break;
    case FLP80X86_KIND:
      cValueLD = *((long double*) concrete->value.as_ptr);
      if (isnan((double)iValue->getValue().as_flp) && isnan(cValueLD)) {
        sync = false;
      }
      else {
        sync = ((double)iValue->getValue().as_flp != cValueLD);
      }
      if (sync) {
        syncValue.as_flp = cValueLD;
        iValue->setValue(syncValue);
      }
      break;
    default: 
      cout << "Should not reach here!" << endl;
      safe_assert(false);
      break;
  }

  if (sync) {
    DEBUG_STDOUT("\t SYNCING AT LOAD DUE TO MISMATCH");
    DEBUG_STDOUT("\t " << iValue->toString());
  }

  return sync;
}

bool InterpreterObserver::syncLoad(IValue* iValue, uint64_t concreteAddr, KIND type) { 
  bool sync = false;
  VALUE syncValue;
  int cValueInt32;
  int64_t cValueInt64;
  float cValueFloat;
  double cValueDouble;
  double cValueLD;

  void *concreteValue;

  concreteValue = (void *) concreteAddr;

  switch (type) {
    case PTR_KIND:
      // TODO: we use int64_t to represent a void* here
      // might not work on 32 bit machine
      cValueInt64 = *((int64_t*) concreteValue);

      sync = (iValue->getValue().as_int + iValue->getOffset() != cValueInt64);
      if (sync) {
        syncValue.as_int = cValueInt64;
        iValue->setValue(syncValue);
      }
      break;
    case INT1_KIND: 
    case INT8_KIND: 
      cValueInt32 = *((int8_t*) concreteValue);

      sync = (((int8_t) iValue->getIntValue()) != cValueInt32);
      if (sync) {
        syncValue.as_int = cValueInt32;
        iValue->setValue(syncValue);
      }
      break;
    case INT16_KIND: 
      {
        cValueInt32 = *((int16_t*) concreteValue);
        sync = (((int16_t) iValue->getIntValue()) != cValueInt32);
        if (sync) {
          syncValue.as_int = cValueInt32;
          iValue->setValue(syncValue);
        }
        break;
      }
    case INT24_KIND:
      cValueInt32 = *((int32_t*) concreteValue);
      cValueInt32 = cValueInt32 & 0x00FFFFFF;
      sync = (((int32_t) iValue->getIntValue()) != cValueInt32);
      if (sync) {
        syncValue.as_int = cValueInt32;
        iValue->setValue(syncValue);
      }
      break;
    case INT32_KIND: 
      cValueInt32 = *((int32_t*) concreteValue);
      sync = (((int32_t) iValue->getIntValue()) != cValueInt32);
      if (sync) {
        syncValue.as_int = cValueInt32;
        iValue->setValue(syncValue);
      }
      break;
    case INT64_KIND:
      cValueInt64 = *((int64_t*) concreteValue);
      sync = (iValue->getValue().as_int != cValueInt64);
      if (sync) {
        syncValue.as_int = cValueInt64;
        iValue->setValue(syncValue);
      }
      break;
    case INT80_KIND:
      cout << "[syncload] Unsupported INT80_KIND" << endl;
      safe_assert(false);
      break;
    case FLP32_KIND:
      cValueFloat = *((float*) concreteValue);
      if (isnan((float)iValue->getValue().as_flp) && isnan(cValueFloat)) {
        sync = false;
      } else {
        sync = ((float)iValue->getValue().as_flp != cValueFloat);
      }
      if (sync) {
        syncValue.as_flp = cValueFloat;
        iValue->setValue(syncValue);
      }
      break;
    case FLP64_KIND:
      cValueDouble = *((double*) concreteValue);
      if (isnan((double)iValue->getValue().as_flp) && isnan(cValueDouble)) {
        sync = false;
      } else {
        sync = ((double)iValue->getValue().as_flp != cValueDouble);
      }
      if (sync) {
        syncValue.as_flp = cValueDouble;
        iValue->setValue(syncValue);
      }
      break;
    case FLP80X86_KIND:
      {
        cValueLD = *((long double*) concreteValue);
        if (isnan((double)iValue->getValue().as_flp) && isnan(cValueLD)) {
          sync = false;
        } else {
          sync = ((double)iValue->getValue().as_flp != cValueLD);
        }
        if (sync) {
          syncValue.as_flp = cValueLD;
          iValue->setValue(syncValue);
        }
        break;
      }
    default: 
      cout << "Should not reach here!" << endl;
      safe_assert(false);
      break;
  }

  if (sync) {
    DEBUG_STDOUT("\t SYNCING AT LOAD DUE TO MISMATCH");
    DEBUG_STDOUT("\t " << iValue->toString());
  }

  return sync;
}
