#include "Common.h"

/*******************************************************************************************/

std::string IID_ToString(IID& iid) {
	std::stringstream s;
	s << "[IID: " << iid << "]";
	return s.str();
}

std::string PTR_ToString(PTR& ptr) {
	std::stringstream s;
	s << "[PTR: " << ptr << "]";
	return s.str();
}

std::string KVALUE_ToString(KVALUE& kv) {
	std::stringstream s;
	s << "[IID: " << kv.iid << " ";
	switch(kv.kind) {
		case PTR_KIND:
			s << "PTR: " << kv.value.as_ptr << "]";
			break;
		case INT1_KIND:
			s << "INT1: " << kv.value.as_int << "]";
			break;
		case INT8_KIND:
			s << "INT8: " << kv.value.as_int << "]";
			break;
		case INT16_KIND:
			s << "INT16: " << kv.value.as_int << "]";
			break;
		case INT32_KIND:
			s << "INT32: " << kv.value.as_int << "]";
			break;
		case INT64_KIND:
			s << "INT64: " << kv.value.as_int << "]";
			break;
		case FLP32_KIND:
			s << "FLP32: " << kv.value.as_flp << "]";
			break;
		case FLP64_KIND:
			s << "FLP64: " << kv.value.as_flp << "]";
			break;
		default: //safe_assert(false);
			break;
	}
	return s.str();
}

/*******************************************************************************************/

