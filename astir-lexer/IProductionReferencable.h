#pragma once

#include <list>
#include <string>

#include "IFileLocalizable.h"

class Machine;
class IProductionReferencable {
public:
	virtual ~IProductionReferencable() = default;

	virtual const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;
	
protected:
	IProductionReferencable() = default;
};