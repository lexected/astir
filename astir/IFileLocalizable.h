#pragma once

#include "FileLocation.h"

class IFileLocalizable {
public:
	virtual const FileLocation& location() const = 0;
	std::string locationString() const { return location().toString(); }

protected:
	IFileLocalizable() = default;
	virtual ~IFileLocalizable() = default; 
};

typedef const IFileLocalizable* IFileLocalizableCPtr;