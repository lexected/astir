#pragma once

#include <list>
#include <string>

#include "IFileLocalizable.h"

struct MachineDefinition;
class IReferencing {
public:
	virtual ~IReferencing() = default;

	virtual const IFileLocalizable* findRecursiveReference(const MachineDefinition& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;
	
protected:
	IReferencing() = default;
};