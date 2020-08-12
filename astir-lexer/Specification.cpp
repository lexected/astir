#include "Specification.h"

#include "SpecificationFile.h"

bool Specification::containsMachineHierarchyRecursion(const std::map<std::string, MachineDefinition*>& definitions, std::list<std::string>& namesEncountered, std::string nameConsidered) const {

	bool retNow = std::find(namesEncountered.cbegin(), namesEncountered.cend(), nameConsidered) == namesEncountered.cend();
	namesEncountered.push_back(nameConsidered);
	if (retNow) {
		return true;
	}

	const MachineDefinition* definitionConsidered = definitions.find(nameConsidered)->second;
	if (containsMachineHierarchyRecursion(definitions, namesEncountered, definitionConsidered->follows)) {
		return true;
	}

	for(const auto& used : definitionConsidered->uses) {
		if (containsMachineHierarchyRecursion(definitions, namesEncountered, used)) {
			return true;
		}
	}

	namesEncountered.pop_back();
	return false;
}

MachineComponent* Machine::contextFindMachineComponent(const std::string& name) const {
	MachineComponent* ret;
	if (extends && (ret = extends->contextFindMachineComponent(name))) {
		return ret;
	}
	
	if (follows && (ret = follows->contextFindMachineComponent(name))) {
		return ret;
	}

	auto it = components.find(name);
	if (it != components.cend()) {
		return it->second.get();
	}

	return nullptr;
}

bool Machine::containsDeclarationCategoryRecursion(const std::map<std::string, MachineStatement*>& statements, std::list<std::string>& namesEncountered, std::string nameConsidered, bool mustBeACategory) const {
	bool retNow = std::find(namesEncountered.cbegin(), namesEncountered.cend(), nameConsidered) == namesEncountered.cend();
	namesEncountered.push_back(nameConsidered);
	if (retNow) {
		return true;
	}

	auto statIt = statements.find(nameConsidered);
	if (statIt == statements.cend()) {
		throw SemanticAnalysisException(nameConsidered + "' is not recognized as category name in the present context but was referenced as such in the declaration of " + namesEncountered.back());
	}

	const MachineStatement* statementConsidered = statIt->second;
	if (mustBeACategory && dynamic_cast<const CategoryStatement*>(statementConsidered) == nullptr) {
		throw SemanticAnalysisException(nameConsidered + "' is not a category but was referenced as a category in the declaration of " + namesEncountered.back());
	}

	for(const std::string& categoryReferenced : statementConsidered->categories) {
		if (containsDeclarationCategoryRecursion(statements, namesEncountered, categoryReferenced, true)) {
			return true;
		}
	}

	namesEncountered.pop_back();
	return false;
}

bool Machine::componentRecursivelyReferenced(std::list<std::string>& namesEncountered, const ReferenceRegex& componentReference) const {
	auto component = contextFindMachineComponent(componentReference.referenceName);
	if (!component) {
		throw SemanticAnalysisException("Unknown component name '" + componentReference.referenceName + "' encountered", componentReference);
	}
	
	return component->componentRecursivelyReferenced(*this, namesEncountered);
}

SemanticAnalysisException::SemanticAnalysisException(const std::string& message, const ParsedStructure& parsedStructureToInferLocationFrom)
	: Exception(message + parsedStructureToInferLocationFrom.locationString()) { }

bool Category::componentRecursivelyReferenced(const Machine& machine, std::list<std::string>& namesEncountered) const {
	bool ret = std::find(namesEncountered.cbegin(), namesEncountered.cend(), name) != namesEncountered.cend();
	namesEncountered.push_back(name);
	if(ret) {
		return true;
	}

	for (auto reference : this->references) {
		if (reference.second->componentRecursivelyReferenced(machine, namesEncountered)) {
			return true;
		}
	}

	namesEncountered.pop_back();
	return false;
}

bool Category::containsDisallowedComponentRecursion(const Machine& machine, std::list<std::string>& namesEncountered, bool isAllRecursionDisallowed) const {
	if (isAllRecursionDisallowed) {
		return componentRecursivelyReferenced(machine, namesEncountered);
	}

	return false;
}

bool Rule::componentRecursivelyReferenced(const Machine& machine, std::list<std::string>& namesEncountered) const {
	bool ret = std::find(namesEncountered.cbegin(), namesEncountered.cend(), name) != namesEncountered.cend();
	namesEncountered.push_back(name);
	if (ret) {
		return true;
	}

	regex->componentRecursivelyReferenced(machine, namesEncountered);

	namesEncountered.pop_back();
	return false;
}

bool Rule::containsDisallowedComponentRecursion(const Machine& machine, std::list<std::string>& namesEncountered, bool isAllRecursionDisallowed) const {
	if (!terminal && !isAllRecursionDisallowed) {
		return false;
	}

	return componentRecursivelyReferenced(machine, namesEncountered);
}

bool FAMachine::containsDisallowedComponentRecursion(std::list<std::string>& namesEncountered) const {
	for (const auto& componentPair : components) {
		if (componentPair.second->containsDisallowedComponentRecursion(*this, namesEncountered, true)) {
			return true;
		}
	}

	return false;
}
