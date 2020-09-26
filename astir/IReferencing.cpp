#include "IReferencing.h"

std::string IReferencing::referenceName() const {
    return std::string();
}

void IReferencing::completeReferences(const MachineDefinition& machine) { }

IFileLocalizableCPtr IReferencing::findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const {
    return nullptr;
}
