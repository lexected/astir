#include "SyntacticTree.h"
#include "SemanticTree.h"

#include <set>

std::shared_ptr<SemanticTree> SyntacticTree::makeSemanticEntity(const std::shared_ptr<ISemanticallyProcessable<SemanticTree>>& ownershipPtr) const {
	std::shared_ptr<SemanticTree> semanticTree = std::make_shared<SemanticTree>(std::dynamic_pointer_cast<const SyntacticTree>(ownershipPtr));

	// Future TODO: parse and load other files here

	// build up the name database
	std::set<std::string> usedMachineNames;
	for (const auto& machineDefinitionPtr : machineDefinitions) {
		if (usedMachineNames.find(machineDefinitionPtr->name) != usedMachineNames.end()) {
			throw SemanticAnalysisException("A machine with the name '" + machineDefinitionPtr->name + "' has already been defined in the current context", *machineDefinitionPtr);
		}

		semanticTree->machines.emplace(machineDefinitionPtr->name, machineDefinitionPtr->makeSemanticEntity(machineDefinitionPtr));
		usedMachineNames.insert(machineDefinitionPtr->name);
	}

	// check for recursion
	std::list<std::string> namesEncountered;
	for (const auto& definitionPtr : machineDefinitions) {
		semanticTree->checkForMachineHierarchyRecursion(namesEncountered, definitionPtr->name);
	}

	// now add links between machines
	for (const auto& machineDefinitionPtr : this->machineDefinitions) {
		auto machineConsidered = semanticTree->machines[machineDefinitionPtr->name];
		if (!machineDefinitionPtr->on.empty()) {
			machineConsidered->on = semanticTree->machines[machineDefinitionPtr->on];
		} else {
			machineConsidered->on = nullptr;
		}
		
		for (const auto& used : machineDefinitionPtr->uses) {
			machineConsidered->uses.push_back(semanticTree->machines[used]);
		}
	}

	// and, finally, initialize all machines from the file, internally
	semanticTree->initialize();

	return semanticTree;
}

std::shared_ptr<Machine> FiniteAutomatonDefinition::makeSemanticEntity(const std::shared_ptr<ISemanticallyProcessable<Machine>>& ownershipPtr) const {
	return std::make_shared<FiniteAutomatonMachine>(std::dynamic_pointer_cast<const FiniteAutomatonDefinition>(ownershipPtr), name);
}
