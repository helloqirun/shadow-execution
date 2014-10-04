// Author: Cuong Nguyen

#include "BlameAnalysis.h"
#include "../../src/InstructionMonitor.h"

using namespace std;

/*** HELPER FUNCTIONS ***/

unordered_map<IID, DebugInfo> BlameAnalysis::readDebugInfo() {
	std::stringstream debugFileName;
	debugFileName << getenv("GLOG_log_dir") << separator() << "debug.bin";
	FILE* debugFile = fopen(debugFileName.str().c_str(), "rb");
	IID iid;
	struct DebugInfo debugInfo;
	unordered_map<IID, DebugInfo> debugInfoMap;

	while (fread(&iid, sizeof(uint64_t), 1, debugFile) &&
			fread(&debugInfo, sizeof(struct DebugInfo), 1, debugFile)) {
		debugInfoMap[iid] = debugInfo;
	}
	fclose(debugFile);

	return debugInfoMap;
}

void* BlameAnalysis::copyShadow(void* oldShadow) {
	if (oldShadow != NULL) {
		BlameShadowObject* bsoSrc = (BlameShadowObject*)oldShadow;
		BlameShadowObject* bsoDest =
			new BlameShadowObject(bsoSrc->id, bsoSrc->highValue, bsoSrc->lowValue);
		return bsoDest;
	}
	return NULL;
}

const BlameShadowObject BlameAnalysis::getShadowObject(IID iid, SCOPE scope,
		int64_t value) {
	IValue* iv = nullptr;
	switch (scope) {
		case CONSTANT: {
			double* ptr = (double*)&value;
			return BlameShadowObject(iid, (HIGHPRECISION) * ptr, (LOWPRECISION) * ptr);
		}
		case GLOBAL:
			iv = globalSymbolTable[value];
			break;
		case LOCAL:
			iv = executionStack.top()[value];
			break;
		default:
			DEBUG_STDERR("Unknown scope " << scope);
			safe_assert(false);
	}

	if (iv->getShadow() == NULL) {
		double flpValue = iv->getFlpValue();
		return BlameShadowObject(iid, (HIGHPRECISION) flpValue,
								 (LOWPRECISION) flpValue);
	} else {
		return *((BlameShadowObject*)iv->getShadow());
	}
}

void BlameAnalysis::post_fbinop(IID iid, IID liid, IID riid, SCOPE lScope,
								SCOPE rScope, int64_t lValue, int64_t rValue,
								KIND type UNUSED, int inx, BINOP op) {
	// Compute the resulting shadow object.
	//
	// The resulting shadow object is computed using the shadow object of the
	// left and right operand. Update the shadow object for this instruction. At
	// each time, only the most recent shadow object instant is kept.
	const BlameShadowObject lBSO = getShadowObject(liid, lScope, lValue);
	const BlameShadowObject rBSO = getShadowObject(riid, rScope, rValue);

	HIGHPRECISION hResult =
		feval<HIGHPRECISION>(lBSO.highValue, rBSO.highValue, op);
	LOWPRECISION lResult = feval<LOWPRECISION>(lBSO.lowValue, rBSO.lowValue, op);

	/*
	 * Printing shadow execution trace for debugging.
	DebugInfo debugInfo = debugInfoMap.at(iid);
	printf("[RESULT] File: %s, Line: %d, Column: %d, High precision result: %.10f,
	Low precision result: %.10f\n", debugInfo.file,
	    debugInfo.line, debugInfo.column, hResult, lResult);
	printf("[LOP] High precision result: %.10f, Low precision result: %.10f\n",
	lBSO.highValue, lBSO.lowValue);
	printf("[ROP] High precision result: %.10f, Low precision result: %.10f\n",
	rBSO.highValue, rBSO.lowValue);
	*/

	if (executionStack.top()[inx]->getShadow() == NULL) {
		BlameShadowObject* BSO = new BlameShadowObject(iid, hResult, lResult);
		executionStack.top()[inx]->setShadow(BSO);
	} else {
		*((BlameShadowObject*)executionStack.top()[inx]->getShadow()) =
			BlameShadowObject(iid, hResult, lResult);
	}

	// Compute blame summary for the resulting shadow object.
	BlameShadowObject* BSO =
		(BlameShadowObject*)executionStack.top()[inx]->getShadow();
	computeBlameSummary(*BSO, lBSO, rBSO, op);

	// Update iid.
	_iid = iid;
}

void BlameAnalysis::computeBlameSummary(const BlameShadowObject& bso,
										const BlameShadowObject& lbso,
										const BlameShadowObject& rbso,
										BINOP op) {
	IID id = bso.id;

	// Initialize the blame summary for the two operands if not exist. They can be
	// non-exists if their values are
	// constant.
	initSummaryIfNotExist(lbso.id);
	initSummaryIfNotExist(rbso.id);

	// Reuse the roots from summary or initialize a new one if not exists.
	std::vector<BlameNode*> roots;
	if (blameSummary.find(id) != blameSummary.end()) {
		roots = blameSummary[id];
	} else {
		roots.push_back(new BlameNode(id, BITS_FLOAT, false, false,
									  std::vector<BlameNode*>()));
		for (PRECISION p = PRECISION(BITS_FLOAT + 1); p < PRECISION_NO;
				p = PRECISION(p + 1)) {
			std::vector<BlameNode*> blames = { blameSummary[lbso.id][BITS_FLOAT],
											   blameSummary[rbso.id][BITS_FLOAT]
											 };
			roots.push_back(new BlameNode(id, p, false, false, blames));
		}
		blameSummary[id] = roots;
	}

	// Compute blame information for all remained precisions and merge with the
	// current blame summary.
	for (PRECISION p = PRECISION(BITS_FLOAT + 1); p < PRECISION_NO;
			p = PRECISION(p + 1)) {
		const BlameNode& blameInfo =
			computeBlameInformation(bso, lbso, rbso, op, p);
		mergeBlame(roots[p], blameInfo);
	}
}

const BlameNode BlameAnalysis::computeBlameInformation(
	const BlameShadowObject& bso, const BlameShadowObject& lbso,
	const BlameShadowObject& rbso, BINOP op, PRECISION p) {
	HIGHPRECISION val =
		clearBits(bso.highValue, DOUBLE_MANTISSA_LENGTH - PRECISION_BITS.at(p));
	bool requireHigherPrecision = val != (LOWPRECISION) val;
	bool requireHigherPrecisionOperator = true;

	// Compute the values of lbso and rbso in different precision.
	std::map<PRECISION, HIGHPRECISION> lbsoVals;
	std::map<PRECISION, HIGHPRECISION> rbsoVals;
	lbsoVals[BITS_FLOAT] = lbso.lowValue;
	rbsoVals[BITS_FLOAT] = rbso.lowValue;
	for (PRECISION i = PRECISION(BITS_FLOAT + 1); i < PRECISION_NO;
			i = PRECISION(i + 1)) {
		lbsoVals[i] = clearBits(lbso.highValue,
								DOUBLE_MANTISSA_LENGTH - PRECISION_BITS.at(i));
		rbsoVals[i] = clearBits(rbso.highValue,
								DOUBLE_MANTISSA_LENGTH - PRECISION_BITS.at(i));
	}

	// Compute the minimal blame information.
	PRECISION i, j;
	bool found = false;
	for (i = BITS_FLOAT; i < PRECISION_NO; i = PRECISION(i + 1)) {
		for (j = BITS_FLOAT; j < PRECISION_NO; j = PRECISION(j + 1)) {
			if (!canBlame(val, lbsoVals[i], rbsoVals[j], op, p)) {
				continue;
			}

			// Found a blame pair. Terminate the loop.
			found = true;
			requireHigherPrecisionOperator = isRequiredHigherPrecisionOperator(
												 val, lbsoVals[i], rbsoVals[j], op, p);
			break;
		}
		// Terminate the loop if found.
		if (found) {
			break;
		}
	}

	/*
	if (!found) {
	  DebugInfo debugInfo = debugInfoMap.at(bso.id);
	  printf("[RESULT] File: %s, Line: %d, Column: %d, High precision result:
	%.10f, Low precision result: %.10f, Current result: %.10f\n", debugInfo.file,
	debugInfo.line, debugInfo.column, bso.highValue, bso.lowValue, val);
	  printf("[LOP] High precision result: %.10f, Low precision result: %.10f\n",
	lbso.highValue, lbso.lowValue);
	  printf("[ROP] High precision result: %.10f, Low precision result: %.10f\n",
	rbso.highValue, rbso.lowValue);
	  printf("High eval result: %.10f, Low eval result: %.10f\n",
	feval<HIGHPRECISION>(lbso.lowValue, rbso.lowValue, op),
	feval<LOWPRECISION>(lbso.highValue, rbso.highValue, op));
	  printf("Can blame: %d\n", canBlame(val, lbsoVals[BITS_FLOAT],
	rbsoVals[BITS_FLOAT], op, p));
	}
	*/

	// Construct the associated blame node and return.
	safe_assert(found);
	BlameNode* lBlameNode = blameSummary[lbso.id][i];
	BlameNode* rBlameNode = blameSummary[rbso.id][j];
	safe_assert(lBlameNode != NULL);
	safe_assert(rBlameNode != NULL);
	std::vector<BlameNode*> blameNodes = { lBlameNode, rBlameNode };

	return BlameNode(bso.id, p, requireHigherPrecision,
					 requireHigherPrecisionOperator, blameNodes);
}

bool BlameAnalysis::canBlame(HIGHPRECISION result, HIGHPRECISION lop,
							 HIGHPRECISION rop, BINOP op, PRECISION p) {
	return equalWithinPrecision(
			   result, clearBits(feval<HIGHPRECISION>(lop, rop, op),
								 DOUBLE_MANTISSA_LENGTH - PRECISION_BITS.at(p)),
			   p);
}

bool BlameAnalysis::isRequiredHigherPrecisionOperator(HIGHPRECISION result,
		HIGHPRECISION lop,
		HIGHPRECISION rop,
		BINOP op, PRECISION p) {
	return !equalWithinPrecision(
			   result, clearBits(feval<LOWPRECISION>(lop, rop, op),
								 DOUBLE_MANTISSA_LENGTH - PRECISION_BITS.at(p)),
			   p);
}

void BlameAnalysis::mergeBlame(BlameNode* summary, const BlameNode& blame) {
	// Summary and blame node needs to have same precision requirement and same
	// numbers of blame children.
	safe_assert(summary->precision == blame.precision &&
				summary->children.size() == blame.children.size());
	std::vector<BlameNode*> merge;
	std::vector<BlameNode*>::iterator sIt;
	std::vector<BlameNode*>::const_iterator bIt;
	for (sIt = summary->children.begin(), bIt = blame.children.begin();
			sIt != summary->children.end(); sIt++, bIt++) {
		BlameNode* sNode = *sIt;
		BlameNode* bNode = *bIt;
		safe_assert(sNode != NULL);
		safe_assert(bNode != NULL);
		if (sNode->precision > bNode->precision) {
			merge.push_back(sNode);
		} else {
			merge.push_back(bNode);
		}
	}
	summary->children = merge;
	summary->requireHigherPrecision =
		summary->requireHigherPrecision || blame.requireHigherPrecision;
	summary->requireHigherPrecisionOperator =
		summary->requireHigherPrecisionOperator ||
		blame.requireHigherPrecisionOperator;
}

void BlameAnalysis::initSummaryIfNotExist(IID id) {
	if (blameSummary.find(id) == blameSummary.end()) {
		std::vector<BlameNode*> blames;
		for (PRECISION p = BITS_FLOAT; p < PRECISION_NO; p = PRECISION(p + 1)) {
			blames.push_back(
				new BlameNode(id, p, false, false, std::vector<BlameNode*>()));
		}
		blameSummary[id] = blames;
	}
}

/*** API FUNCTIONS ***/
void BlameAnalysis::pre_analysis() {
	IValue::setShadowHandlers(copyShadow, [](void* a) {
		delete static_cast<BlameShadowObject*>(a);
	});
}

void BlameAnalysis::post_fadd(IID iid, IID liid, IID riid, SCOPE lScope,
							  SCOPE rScope, int64_t lValue, int64_t rValue,
							  KIND type, int inx) {
	post_fbinop(iid, liid, riid, lScope, rScope, lValue, rValue, type, inx, FADD);
}

void BlameAnalysis::post_fsub(IID iid, IID liid, IID riid, SCOPE lScope,
							  SCOPE rScope, int64_t lValue, int64_t rValue,
							  KIND type, int inx) {
	post_fbinop(iid, liid, riid, lScope, rScope, lValue, rValue, type, inx, FSUB);
}

void BlameAnalysis::post_fmul(IID iid, IID liid, IID riid, SCOPE lScope,
							  SCOPE rScope, int64_t lValue, int64_t rValue,
							  KIND type, int inx) {
	post_fbinop(iid, liid, riid, lScope, rScope, lValue, rValue, type, inx, FMUL);
}

void BlameAnalysis::post_fdiv(IID iid, IID liid, IID riid, SCOPE lScope,
							  SCOPE rScope, int64_t lValue, int64_t rValue,
							  KIND type, int inx) {
	post_fbinop(iid, liid, riid, lScope, rScope, lValue, rValue, type, inx, FDIV);
}

// Interpretation of result.
void BlameAnalysis::post_analysis() {
	DebugInfo dbg = debugInfoMap.at(_iid);
	printf("Default starting point: Function %s, Line %d, Column %d, IID %lu\n",
		   dbg.file, dbg.line, dbg.column, _iid);
	printf("Default precision: %d\n", PRECISION_BITS.at(_precision));

	std::set<BlameNode*> visited;
	std::queue<BlameNode*> workList;
	workList.push(blameSummary[_iid][_precision]);

	while (!workList.empty()) {
		// Find more blame node and add to the queue.
		BlameNode* node = workList.front();
		workList.pop();
		std::vector<BlameNode*> blameNodes = node->children;
		for (std::vector<BlameNode*>::iterator it = blameNodes.begin();
				it != blameNodes.end(); it++) {
			BlameNode* blameNode = *it;
			if (visited.find(blameNode) == visited.end()) {
				visited.insert(blameNode);
				workList.push(blameNode);
			}
		}

		// Interpret the result for the current blame node.
		if (debugInfoMap.find(node->iid) == debugInfoMap.end()) {
			continue;
		}
		DebugInfo dbg = debugInfoMap.at(node->iid);
		if (node->requireHigherPrecision || node->requireHigherPrecisionOperator) {
			printf("Function %s, Line %d, Column %d, HigherPrecision: %d, "
				   "HigherPrecisionOperator: %d\n",
				   dbg.file, dbg.line, dbg.column, node->requireHigherPrecision,
				   node->requireHigherPrecisionOperator);
		}
	}

	// Free memory
	for (auto it = blameSummary.begin(); it != blameSummary.end(); it++) {
		vector<BlameNode*> nodes = it->second;
		for (auto it2 = nodes.begin(); it2 != nodes.end(); it2++) {
			delete *it2;
		}
	}
	blameSummary.clear();
}

static RegisterObserver<BlameAnalysis> BlameAnalysisInstance("blameanalysis");
