#include "SyntacticTree.h"
#include "SemanticTree.h"

#include <set>

std::shared_ptr<Rule> RuleStatement::makeSemanticEntity(const std::shared_ptr<ISemanticallyProcessable<Rule>>& ownershipPtr) const {
	bool terminal;
	bool typeForming;

	terminal = this->terminalitySpecified ? this->terminality : false;

	if (type == RuleStatementType::Production) {
		typeForming = true;
	} else if(type == RuleStatementType::Pattern) {
		typeForming = false;
	} else {
		throw SemanticAnalysisException("Unrecognized RuleStatement type. This should never happen in practice.");
	}

	return std::make_shared<Rule>(std::dynamic_pointer_cast<const RuleStatement>(ownershipPtr), this->name, terminal, typeForming, this->disjunction);
}

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
		if (!machineDefinitionPtr->follows.empty()) {
			machineConsidered->follows = semanticTree->machines[machineDefinitionPtr->follows];
		} else {
			machineConsidered->follows = nullptr;
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
	return std::make_shared<FiniteAutomaton>(std::dynamic_pointer_cast<const FiniteAutomatonDefinition>(ownershipPtr), name, type);
}

std::shared_ptr<Category> CategoryStatement::makeSemanticEntity(const std::shared_ptr<ISemanticallyProcessable<Category>>& ownershipPtr) const {
	return std::make_shared<Category>(std::dynamic_pointer_cast<const CategoryStatement>(ownershipPtr), this->name);
}

bool FlagField::flaggable() const {
	return true;
}

bool FlagField::settable() const {
	return false;
}

bool FlagField::listable() const {
	return false;
}

bool RawField::flaggable() const {
	return false;
}

bool RawField::settable() const {
	return true;
}

bool RawField::listable() const {
	return true;
}

bool ItemField::flaggable() const {
	return false;
}

bool ItemField::settable() const {
	return true;
}

bool ItemField::listable() const {
	return false;
}

bool ListField::flaggable() const {
	return false;
}

bool ListField::settable() const {
	return false;
}

bool ListField::listable() const {
	return true;
}
