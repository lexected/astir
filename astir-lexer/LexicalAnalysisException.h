#pragma once

#include "Exception.h"

class LexicalAnalysisException : public Exception {
public:
	LexicalAnalysisException(const std::string& errmsg)
		: Exception(errmsg) { }
};