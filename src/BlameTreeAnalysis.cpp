/**
 * @file BlameTreeAnalysis.cpp
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

// Author: Cuong Nguyen

#include "BlameTreeAnalysis.h"

BlameNode BlameTreeAnalysis::constructBlameNode(BlameTreeShadowObject<BlameTree::HIGHPRECISION>
    left, BlameTree::PRECISION precision,
    BlameTreeShadowObject<BlameTree::HIGHPRECISION> right01,
    BlameTreeShadowObject<BlameTree::HIGHPRECISION> right02) {

  int dpc, pc, fid, dpc01, dpc02;
  std::map<BlameNodeID, BlameNode>::iterator it;
  BlameTree::HIGHPRECISION value;
  BINOP bop;

  dpc = left.getDPC();
  BlameNodeID bnID(dpc, precision);
  pc = left.getPC(); 
  fid = left.getFileID();
  value = left.getValue(precision);
  bop = left.getBinOp();
  it = nodes.find(bnID);

  if (it != nodes.end()) {
    int i, j, max_j;
    vector< vector< BlameNodeID > > blameNodeIds;
    BlameNode node(dpc, pc, fid, precision, blameNodeIds);

    dpc01 = right01.getDPC();
    dpc02 = right02.getDPC();

    //
    // try different combination of precision of the two operands
    //
    max_j = 5;
    for (i = 0; i < 5; i++) {
      BlameTree::HIGHPRECISION value01 = right01.getValue(i);

      for (j = 0; j < max_j; j++) {
        BlameTree::HIGHPRECISION value02 = right02.getValue(j);

        if (value == chop(eval(value01, value02, bop), precision)) {
          //
          // Construct edges for each blame
          //
          BlameNodeID bnID01(dpc01, BlameTree::PRECISION(i));
          BlameNodeID bnID02(dpc02, BlameTree::PRECISION(j));
          vector<BlameNodeID> blamedNodes; 

          blamedNodes.push_back(bnID01);
          blamedNodes.push_back(bnID02);
          node.addBlamedNodes(blamedNodes);

          break;
        }
      }

      max_j = j;
    }

    nodes[bnID] = node;

    return node;
  } 

  return it->second;
}

BlameTree::HIGHPRECISION eval(BlameTree::HIGHPRECISION value01,
    BlameTree::HIGHPRECISION value02, BINOP bop) {
  switch (bop) {
    case ADD:
    case FADD:
      return value01 + value02;
    case SUB:
    case FSUB:
      return value01 - value02; 
    case MUL:
    case FMUL:
      return value01 * value02;
    case FDIV:
    case UDIV:
    case SDIV:
      return value01 / value02;
    case UREM:
    case SREM:
    case FREM:
    default:
      safe_assert(false);
      DEBUG_STDERR("Unsupport binary operator " << bop);
      return 0;
  }
}