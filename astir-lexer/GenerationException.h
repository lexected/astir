#pragma once

#include "Exception.h"
#include "IFileLocalizable.h"

class GenerationException : public Exception {
public:
	GenerationException()
		: Exception("A generation exception has been encountered") { }
	GenerationException(const std::string& message)
		: Exception(message) { }
	GenerationException(const std::string& message, const IFileLocalizable& element)
		: Exception(message + " -- on " + element.locationString()) { }
};
