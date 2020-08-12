#include "SyntacticTree.h"
#include "SemanticTree.h"

void SemanticTree::checkForMachineHierarchyRecursion(std::list<std::string>& namesEncountered, const std::string& nameConsidered) const {
	bool collision = std::find(namesEncountered.cbegin(), namesEncountered.cend(), nameConsidered) == namesEncountered.cend();
	namesEncountered.push_back(nameConsidered);
	if (collision) {
		std::string hierarchyPath = namesEncountered.front();
		namesEncountered.pop_front();
		for (const auto& nameEncountered : namesEncountered) {
			hierarchyPath += "-" + nameEncountered;
		}
		throw SemanticAnalysisException("Definition recursion found in the mixed follow/extends hierarchy path " + hierarchyPath);
	}

	const MachineDefinition* definitionConsidered = dynamic_cast<const MachineDefinition*>(this->machines.find(nameConsidered)->second->underlyingSyntacticEntity().get());
	checkForMachineHierarchyRecursion(namesEncountered, definitionConsidered->follows);
	for (const auto& used : definitionConsidered->uses) {
		checkForMachineHierarchyRecursion(namesEncountered, used);
	}

	namesEncountered.pop_back();
}

const std::shared_ptr<const ISyntacticEntity>& SemanticTree::underlyingSyntacticEntity() const {
	return m_syntacticTree;
}

void SemanticTree::initialize() {
	for (const auto& machinePair : machines) {
		machinePair.second->initialize();
	}
}

void Machine::initialize() {
	// here we simply make sure that the dependencies are initialized before the machine itself.
	// Note that since there are no recursions assumed the process is guaranteed to terminate.
	if (initialized()) {
		return;
	}

	ISemanticEntity::initialize();

	for (const auto& usedPtr : uses) {
		usedPtr->initialize();
	}

	if (follows) {
		follows->initialize();
	}

	const MachineDefinition* machineDefinition = dynamic_cast<const MachineDefinition*>(underlyingSyntacticEntity().get());
	// build up the name database
	for (const auto& statementPtr : machineDefinition->categoryStatements) {
		if (contextFindMachineComponent(statementPtr->name)) {
			throw SemanticAnalysisException("Category name '" + statementPtr->name + "' encountered in machine '" + name + "' already defined", *this); // TODO: could use an improvement; already defined where?
		}

		categories[statementPtr->name] = statementPtr->makeSemanticEntity();
	}

	for (const auto& statementPtr : machineDefinition->ruleStatements) {
		if (contextFindMachineComponent(statementPtr->name)) {
			throw SemanticAnalysisException("Rule name '" + statementPtr->name + "' encountered in machine '" + name + "' already defined", *this); // TODO: could use an improvement; already defined where?
		}

		rules[statementPtr->name] = statementPtr->makeSemanticEntity();
	}

	// check for possible category reference recursion (and obviously whether the referenced category is indeed a category)
	std::list<std::string> namesEncountered;
	for (const auto& rulePair : this->rules) {
		checkForDeclarationCategoryRecursion(namesEncountered, rulePair.first, *rulePair.second, false);
	}

	// since there are no recursions at this point, we can create category links and references here (the same as with machines, and it all becomes messy if you try to move this chore down the hierarchy
	for (const auto& categoryPair : categories) {
		const CategoryStatement* categoryStatement = dynamic_cast<const CategoryStatement*>(categoryPair.second->underlyingSyntacticEntity().get());
		for (const auto& categoryUsedName : categoryStatement->categories) {
			auto mc = contextFindMachineComponent(categoryUsedName);
			Category* cat = dynamic_cast<Category*>(mc);
			categoryPair.second->categories.push_back(cat);
			cat->references[categoryPair.first] = categoryPair.second.get();
		}
	}

	for (const auto& rulePair : rules) {
		const RuleStatement* ruleStatement = dynamic_cast<const RuleStatement*>(rulePair.second->underlyingSyntacticEntity().get());
		for (const auto& categoryUsedName : ruleStatement->categories) {
			auto mc = contextFindMachineComponent(categoryUsedName);
			Category* cat = dynamic_cast<Category*>(mc);
			rulePair.second->categories.push_back(cat);
			cat->references[rulePair.first] = rulePair.second.get();
		}
	}

	// at this point we can proceed to initializing the MachineComponents internally (e.g. initializing their fields)
	for (const auto& componentPair : categories) {
		componentPair.second->initialize();
	}

	for (const auto& componentPair : rules) {
		componentPair.second->initialize();
	}

	// the last bit is to check that there is no disallowed recursion within the productions themselves
	checkForComponentRecursion();
}

MachineComponent* Machine::contextFindMachineComponent(const std::string& name) const {
	MachineComponent* ret;
	for (const auto& used : uses) {
		if (ret = used->contextFindMachineComponent(name)) {
			return ret;
		}
	}
	
	if (follows && (ret = follows->contextFindMachineComponent(name))) {
		return ret;
	}

	auto it = categories.find(name);
	if (it != categories.cend()) {
		return it->second.get();
	}

	auto iit = rules.find(name);
	if (iit != rules.cend()) {
		return iit->second.get();
	}

	return nullptr;
}

void Machine::checkForDeclarationCategoryRecursion(std::list<std::string>& namesEncountered, const std::string& nameConsidered, const IFileLocalizable& occurence, bool mustBeACategory) const {
	bool collision = std::find(namesEncountered.cbegin(), namesEncountered.cend(), nameConsidered) == namesEncountered.cend();
	namesEncountered.push_back(nameConsidered);
	if (collision) {
		std::string hierarchyPath = namesEncountered.front();
		namesEncountered.pop_front();
		for (const auto& nameEncountered : namesEncountered) {
			hierarchyPath += "-" + nameEncountered;
		}
		throw SemanticAnalysisException("Declaration recursion found in the category-use declaration hierarchy path " + hierarchyPath + "; start at " + occurence.locationString() + ", end at " + this->locationString());
	}

	MachineComponent* component = contextFindMachineComponent(nameConsidered);
	if (component == nullptr) {
		throw SemanticAnalysisException(nameConsidered + "' is not recognized as category name in the present context but was referenced as such in the declaration of " + namesEncountered.back(), occurence);
	}

	const Category* category = dynamic_cast<const Category*>(component);
	if (category == nullptr && mustBeACategory) {
		throw SemanticAnalysisException(nameConsidered + "' was found in the present context at " + component->locationString() + " but not as a category, though it was referenced as such in the declaration of " + namesEncountered.back(), occurence);
	}

	const MachineStatement* machineStatement = dynamic_cast<const MachineStatement*>(component->underlyingSyntacticEntity().get());
	for(const std::string& categoryReferenced : machineStatement->categories) {
		checkForDeclarationCategoryRecursion(namesEncountered, categoryReferenced, occurence, true);
	}

	namesEncountered.pop_back();
}

const IFileLocalizable* Machine::findRecursiveReferenceThroughName(const std::string& referenceName, std::list<std::string>& namesEncountered, const std::string& targetName) const {
	auto component = contextFindMachineComponent(referenceName);
	if (component == nullptr) {
		throw SemanticAnalysisException("The category/rule '" + referenceName + "' is not defined in the context of machine '" + this->name + "' defined at " + this->locationString() + "' even though it has been referenced there");
	}

	return component->findRecursiveReference(*this, namesEncountered, targetName);
}

SemanticAnalysisException::SemanticAnalysisException(const std::string& message, const IFileLocalizable& somethingLocalizableToPinpointLocationBy)
	: Exception(message + " -- on " + somethingLocalizableToPinpointLocationBy.locationString()) { }


void FiniteAutomaton::checkForComponentRecursion() const {
	std::list<std::string> relevantNamesEncountered;
	for (const auto& categoryPair : categories) {
		auto recursiveReferenceLocalizableInstance = categoryPair.second->findRecursiveReference(*this, relevantNamesEncountered, categoryPair.first);
		if (recursiveReferenceLocalizableInstance != nullptr) {
			std::string hierarchyPath = relevantNamesEncountered.front();
			relevantNamesEncountered.pop_front();
			for (const auto& nameEncountered : relevantNamesEncountered) {
				hierarchyPath += "-" + nameEncountered;
			}
			throw SemanticAnalysisException("Category reference recursion found in the path " + hierarchyPath + "; start at " + categoryPair.second->locationString() + ", end at " + recursiveReferenceLocalizableInstance->locationString() + " - no recursion is allowed in finite automata");
		}
	}

	for (const auto& rulePair : rules) {
		auto recursiveReferenceLocalizableInstance = rulePair.second->findRecursiveReference(*this, relevantNamesEncountered, rulePair.first);
		if (recursiveReferenceLocalizableInstance != nullptr) {
			std::string hierarchyPath = relevantNamesEncountered.front();
			relevantNamesEncountered.pop_front();
			for (const auto& nameEncountered : relevantNamesEncountered) {
				hierarchyPath += "-" + nameEncountered;
			}
			throw SemanticAnalysisException("Rule reference recursion found in the path " + hierarchyPath + "; start at " + rulePair.second->locationString() + ", end at " + recursiveReferenceLocalizableInstance->locationString() + " - no recursion is allowed in finite automata");
		}
	}
}

const std::shared_ptr<const ISyntacticEntity>& FiniteAutomaton::underlyingSyntacticEntity() const {
	return m_finiteAutomatonDefinition;
}

void MachineComponent::initialize() {
	if (initialized()) {
		return;
	}

	ISemanticEntity::initialize();

	const MachineStatement* statement = dynamic_cast<const MachineStatement*>(underlyingSyntacticEntity().get());
	for (const auto& fieldPtr : statement->fields) {
		fields.push_back(fieldPtr);
	}
}

const IFileLocalizable* Category::findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
	auto nEit = std::find(namesEncountered.cbegin(), namesEncountered.cend(), targetName);
	if (nEit != namesEncountered.cend()) {
		return nullptr;
	}

	namesEncountered.push_back(this->name);

	for (const auto& referencePair : this->references) {
		const IFileLocalizable* ret;
		if ((ret = referencePair.second->findRecursiveReference(machine, namesEncountered, targetName)) != nullptr) {
			return ret;
		}
	}

	namesEncountered.pop_back();
	return nullptr;
}

const std::shared_ptr<const ISyntacticEntity>& Category::underlyingSyntacticEntity() const {
	return m_categoryStatement;
}

const IFileLocalizable* Rule::findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
	auto nEit = std::find(namesEncountered.cbegin(), namesEncountered.cend(), targetName);
	if (nEit != namesEncountered.cend()) {
		return nullptr;
	}

	namesEncountered.push_back(this->name);

	auto ret = regex->findRecursiveReference(machine, namesEncountered, targetName);
	if (ret) {
		return ret;
	}

	namesEncountered.pop_back();
	return nullptr;
}

const std::shared_ptr<const ISyntacticEntity>& Rule::underlyingSyntacticEntity() const {
	return m_ruleStatement;
}