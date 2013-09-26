/**
 * @file Instrumentation.h
 * @brief Instrumentation Declarations.
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
 */

#ifndef INSTRUMENTATION_H_
#define INSTRUMENTATION_H_

#include "Common.h"
#include "InstrumenterBase.h"
#include <map>

class InstrumenterBase;

class Instrumentation {
public:
	typedef std::vector<InstrumenterBase*> InstrumenterPtrList;

  /**
   * Deconstructor of Instrumentation class.
   */
	~Instrumentation();

  /**
   * Initialize the instrumentation process.
   *
   * @param M the LLVM module.
   *
   * @return true if the initialization succeeds; false otherwise.
   */
  bool Initialize(Module &M);

  /**
   * Finalize the instrumentation process.
   *
   * @param M the LLVM module.
   *
   * @return true if the finalization succeeds; false otherwise.
   */
  bool Finalize(Module &M);

  /**
   * Assign basic block, function and module being instrumented to fields. 
   *
   * @param BB the LLVM basic block being instrumented.
   * @param F the LLVM function being instrumented.
   * @param M the LLVM module being instrumented.
   */
  void BeginBasicBlock(BasicBlock* BB, Function* F, Module* M);

  /**
   * Reassign the number of variables and their indices to a clean state.
   *
   * @note VarCount counts the number of local variables and registers created in
   * the function. Indices map each variables/registers to a unique index. This
   * index is used for fast access to the variable during execution.
   */
  void BeginFunction(); 

  /**
   * Iterate through all registered instrumenter and instrument the given
   * instruction.
   *
   * @param I the LLVM instruction.
   *
   * @return true if at least one instrumenter can be used to instrument the
   * given instruction; false otherwise
   */
  bool CheckAndInstrument(Instruction* I);

  /**
   * Create a unique index for variable with id iid.
   *
   * @note This function also keep the counter of variables/registers updated.
   *
   * @param iid the unique id of a variable
   */
  void createIndex(uint64_t iid); 

  /**
   * Get the index of an instruction.
   *
   * @param inst the LLVM instruction.
   *
   * @return the index of the instruction.
   */
  int getIndex(Instruction* inst);

  /**
   * Get the number of variables/registers in the current function.
   *
   * @return number of variables/registers in the current function.
   */
  int getFrameSize();

  inline static Instrumentation* GetInstance() {
    if(instance_ == NULL) {
      instance_ = new Instrumentation();
		}
		return instance_;
	}

  /**
   * Register an instrumenter.
   *
   * @note All the instrumenters being registered will be invoked in the order of
   * registration during instrumentation.
   */
  void RegisterInstrumenter(InstrumenterBase* instrumenter);

public:
  BasicBlock* BB_;
	Function* F_;
	Module* M_;
	unsigned AS_;
  int varCount;
  std::map<uint64_t, int> indices;

private:
	InstrumenterPtrList instrumenters_;

	// singleton!!!
	Instrumentation() : BB_(NULL), F_(NULL), M_(NULL), AS_(0U) {}
	static Instrumentation* instance_;
};

#endif /* INSTRUMENTATION_H_ */
