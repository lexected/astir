#pragma once

#include <list>
#include <string>

#include "IFileLocalizable.h"

class IReferencing;
typedef const IReferencing* IReferencingCPtr;

struct MachineDefinition;
class IReferencing {
public:
	virtual ~IReferencing() = default;

	virtual std::string referenceName() const;
	virtual void completeReferences(const MachineDefinition& machine);
	virtual IFileLocalizableCPtr findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const;
	
protected:
	IReferencing() = default;
};
