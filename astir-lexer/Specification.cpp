#include "Specification.h"


void Specification::initializeFromFile(const SpecificationFile& specificationFile) {
	// build up the name database
	std::map<std::string, MachineDefinition*> machineDefinitions;
	for (const auto & statementPtr: specificationFile.statements) {
		MachineDefinition* md;
		if ((md = dynamic_cast<MachineDefinition*>(statementPtr.get())) != nullptr) {
			auto prevDefIt = machineDefinitions.find(md->machineName);
			if (prevDefIt != machineDefinitions.cend()) {
				throw SemanticAnalysisException("A machine with name '" + md->machineName + "' already defined", *prevDefIt->second);
			}
			machineDefinitions[md->machineName] = md;

			auto machinePtr = std::make_shared<Machine>(md->machineName);
			machines[md->machineName] = move(machinePtr);
		} else {
			throw SemanticAnalysisException("Unsupported statement type encountered", *statementPtr);
		}
	}

	// check for recursion
	std::list<std::string> namesEncountered;
	for (const auto& pair : machineDefinitions) {
		if (containsDefinitionRecursion(machineDefinitions, namesEncountered, pair.first)) {
			std::string hierarchyPath = namesEncountered.front();
			namesEncountered.pop_front();
			for (const auto& nameEncountered : namesEncountered) {
				hierarchyPath += "-" + nameEncountered;
			}
			throw SemanticAnalysisException("Definition recursion found in the mixed follow/extends hierarchy path " + hierarchyPath);
		}
	}

	// now add links between machines
	for (const auto & pair : machineDefinitions) {
		MachineDefinition* md = pair.second;
		
		machines[pair.first]->extends = machines[md->extends];
		machines[pair.first]->follows = machines[md->follows];
	}

	// and, finally, initialize all machines from the file, internally
	for (const auto& pair : machineDefinitions) {
		MachineDefinition* md = pair.second;

		machines[pair.first]->initializeFromDefinition(md);
	}
}

bool Specification::containsDefinitionRecursion(const std::map<std::string, MachineDefinition*>& definitions, std::list<std::string>& namesEncountered, std::string nameConsidered) const {

	bool retNow = std::find(namesEncountered.cbegin(), namesEncountered.cend(), nameConsidered) == namesEncountered.cend();
	namesEncountered.push_back(nameConsidered);
	if (retNow) {
		return true;
	}

	const MachineDefinition* definitionConsidered = definitions.find(nameConsidered)->second;
	if (containsDefinitionRecursion(definitions, namesEncountered, definitionConsidered->follows)) {
		return true;
	}

	if (containsDefinitionRecursion(definitions, namesEncountered, definitionConsidered->extends)) {
		return true;
	}

	namesEncountered.pop_back();
	return false;
}

void Machine::initializeFromDefinition(const MachineDefinition* definition) {
	// implement
}
