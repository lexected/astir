#include "SemanticAnalysisException.h"

SemanticAnalysisException::SemanticAnalysisException(const std::string& message, const IFileLocalizable& somethingLocalizableToPinpointLocationBy)
	: Exception(message + " -- at " + somethingLocalizableToPinpointLocationBy.locationString()) { }
