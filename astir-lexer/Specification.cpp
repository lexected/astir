#include "Specification.h"

#include "SpecificationFile.h"

bool Specification::containsDeclarationCategoryRecursion(const std::map<std::string, MachineDefinition*>& definitions, std::list<std::string>& namesEncountered, std::string nameConsidered) const {

	bool retNow = std::find(namesEncountered.cbegin(), namesEncountered.cend(), nameConsidered) == namesEncountered.cend();
	namesEncountered.push_back(nameConsidered);
	if (retNow) {
		return true;
	}

	const MachineDefinition* definitionConsidered = definitions.find(nameConsidered)->second;
	if (containsDeclarationCategoryRecursion(definitions, namesEncountered, definitionConsidered->follows)) {
		return true;
	}

	if (containsDeclarationCategoryRecursion(definitions, namesEncountered, definitionConsidered->extends)) {
		return true;
	}

	namesEncountered.pop_back();
	return false;
}

std::shared_ptr<MachineComponent> Machine::contextFindMachineEntity(const std::string& name) const {
	std::shared_ptr<MachineComponent> ret;
	if (extends && (ret = extends->contextFindMachineEntity(name))) {
		return ret;
	}
	
	if (follows && (ret = follows->contextFindMachineEntity(name))) {
		return ret;
	}

	auto it = components.find(name);
	if (it != components.cend()) {
		return it->second;
	}

	return nullptr;
}

bool Machine::containsDeclarationCategoryRecursion(const std::map<std::string, MachineStatement*>& statements, std::list<std::string>& namesEncountered, std::string nameConsidered, bool mustBeACategory) const {
	bool retNow = std::find(namesEncountered.cbegin(), namesEncountered.cend(), nameConsidered) == namesEncountered.cend();
	namesEncountered.push_back(nameConsidered);
	if (retNow) {
		return true;
	}

	const MachineStatement* statementConsidered = statements.find(nameConsidered)->second;
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

SemanticAnalysisException::SemanticAnalysisException(const std::string& message, const ParsedStructure& parsedStructureToInferLocationFrom)
	: Exception(message + parsedStructureToInferLocationFrom.locationString()) { }
