#include "SyntacticTree.h"

#include "GenerationVisitor.h"
#include "NFABuilder.h"
#include "SemanticAnalysisException.h"

#include <set>
#include <algorithm>

void SyntacticTree::initialize() {
	if (initialized()) {
		return;
	}

	ISemanticEntity::initialize();

	// Future TODO: parse and load other files here

	// check for recursion
	std::list<std::string> namesEncountered;
	for (const auto& definitionPair : machineDefinitions) {
		completeMachineHierarchy(namesEncountered, definitionPair.second);
	}

	// and, finally, internally initialize all machines
	for (const auto& machinePair : machineDefinitions) {
		machinePair.second->initialize();
	}
}

void SyntacticTree::completeMachineHierarchy(std::list<std::string>& namesEncountered, const std::shared_ptr<MachineDefinition>& machineDefinitionToComplete) const {
	const std::string& nameConsidered = machineDefinitionToComplete->name;
	bool collision = std::find(namesEncountered.cbegin(), namesEncountered.cend(), nameConsidered) != namesEncountered.cend();
	namesEncountered.push_back(nameConsidered);

	if (collision) {
		std::string hierarchyPath = namesEncountered.front();
		namesEncountered.pop_front();
		for (const auto& nameEncountered : namesEncountered) {
			hierarchyPath += "-" + nameEncountered;
		}
		throw SemanticAnalysisException("Definition recursion found in the mixed follow/extends hierarchy path " + hierarchyPath, *this);
	}

	const std::string& onName = machineDefinitionToComplete->on.first;
	if (onName.empty()) {
		machineDefinitionToComplete->on.second = nullptr;
	} else {
		auto onIt = machineDefinitions.find(onName);
		if (onIt == machineDefinitions.end()) {
			throw SemanticAnalysisException("Unknown machine name '" + onName + "' referenced as 'on' dependency by machine '" + machineDefinitionToComplete->name + "', declared at " + machineDefinitionToComplete->locationString());
		}

		machineDefinitionToComplete->on.second = onIt->second;
		completeMachineHierarchy(namesEncountered, onIt->second);
	}

	for (auto& usesPair : machineDefinitionToComplete->uses) {
		auto usesIt = machineDefinitions.find(usesPair.first);
		if (usesIt == machineDefinitions.end()) {
			throw SemanticAnalysisException("Unknown machine name '" + onName + "' referenced as 'uses' dependency by machine '" + machineDefinitionToComplete->name + "', declared at " + machineDefinitionToComplete->locationString());
		}

		usesPair.second = usesIt->second;
		completeMachineHierarchy(namesEncountered, usesIt->second);
	}

	namesEncountered.pop_back();
}

void SyntacticTree::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
}
