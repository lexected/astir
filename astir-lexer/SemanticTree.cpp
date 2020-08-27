#include "SyntacticTree.h"
#include "SemanticTree.h"
#include "NFABuilder.h"

void SemanticTree::checkForMachineHierarchyRecursion(std::list<std::string>& namesEncountered, const std::string& nameConsidered) const {
	bool collision = std::find(namesEncountered.cbegin(), namesEncountered.cend(), nameConsidered) != namesEncountered.cend();
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
	if (!definitionConsidered->follows.empty()) {
		checkForMachineHierarchyRecursion(namesEncountered, definitionConsidered->follows);
	}
	for (const auto& used : definitionConsidered->uses) {
		checkForMachineHierarchyRecursion(namesEncountered, used);
	}

	namesEncountered.pop_back();
}

std::shared_ptr<const ISyntacticEntity> SemanticTree::underlyingSyntacticEntity() const {
	return this->m_syntacticTree;
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
		if (findMachineComponent(statementPtr->name)) {
			throw SemanticAnalysisException("Category name '" + statementPtr->name + "' encountered in machine '" + name + "' already defined", *this); // TODO: could use an improvement; already defined where?
		}

		components[statementPtr->name] = statementPtr->makeSemanticEntity(statementPtr);
	}

	for (const auto& statementPtr : machineDefinition->ruleStatements) {
		if (findMachineComponent(statementPtr->name)) {
			throw SemanticAnalysisException("Rule name '" + statementPtr->name + "' encountered in machine '" + name + "' already defined", *this); // TODO: could use an improvement; already defined where?
		}

		components[statementPtr->name] = statementPtr->makeSemanticEntity(statementPtr);
	}

	// check for possible category reference recursion (and obviously whether the referenced category is indeed a category)
	std::list<std::string> namesEncountered;
	for (const auto& rulePair : this->components) {
		checkForDeclarationCategoryRecursion(namesEncountered, rulePair.first, *rulePair.second, false);
	}

	// since there are no recursions at this point, we can create category links and references here (the same as with machines, and it all becomes messy if you try to move this chore down the hierarchy
	for (const auto& componentPair : components) {
		const MachineStatement* machineStatement = dynamic_cast<const MachineStatement*>(componentPair.second->underlyingSyntacticEntity().get());
		for (const auto& categoryUsedName : machineStatement->categories) {
			bool isFromFollows;
			auto mc = findMachineComponent(categoryUsedName, &isFromFollows);
			Category* cat = dynamic_cast<Category*>(mc);
			componentPair.second->categories.push_back(cat);
			cat->references[componentPair.first] = CategoryReference(componentPair.second.get(), isFromFollows);
		}
	}

	// at this point we can proceed to initializing the MachineComponents internally (e.g. initializing their fields)
	for (const auto& componentPair : components) {
		componentPair.second->initialize();
	}


	// now that the fields are initialized, we can proceed to checking that there are no name collisions and that all types used are valid
	for (const auto& componentPair : components) {
		componentPair.second->checkFieldDeclarations(*this);
	}

	// on the rule and category level we need to check that there is no disallowed recursion within the productions themselves
	checkForComponentRecursion();

	// once the basic field and rule/category reference verifications have been conducted, we need to check whether the actions of individual rules are contextually valid
	for (const auto& componentPair : components) {
		componentPair.second->verifyContextualValidity(*this);
	}
}

MachineComponent* Machine::findMachineComponent(const std::string& name, bool* follows) const {
	MachineComponent* ret;
	for (const auto& used : uses) {
		if (ret = used->findMachineComponent(name)) {
			return ret;
		}
	}
	
	if (follows && (ret = this->follows->findMachineComponent(name))) {
		if (follows) {
			*follows = true;
		}
		return ret;
	}

	auto it = components.find(name);
	if (it != components.cend()) {
		if (follows) {
			*follows = false;
		}
		return it->second.get();
	}

	return nullptr;
}

void Machine::checkForDeclarationCategoryRecursion(std::list<std::string>& namesEncountered, const std::string& nameConsidered, const IFileLocalizable& occurence, bool mustBeACategory) const {
	bool collision = std::find(namesEncountered.cbegin(), namesEncountered.cend(), nameConsidered) != namesEncountered.cend();
	namesEncountered.push_back(nameConsidered);
	if (collision) {
		std::string hierarchyPath = namesEncountered.front();
		namesEncountered.pop_front();
		for (const auto& nameEncountered : namesEncountered) {
			hierarchyPath += "-" + nameEncountered;
		}
		throw SemanticAnalysisException("Declaration recursion found in the category-use declaration hierarchy path " + hierarchyPath + "; start at " + occurence.locationString() + ", end at " + this->locationString());
	}

	MachineComponent* component = findMachineComponent(nameConsidered);
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
	auto component = findMachineComponent(referenceName);
	if (component == nullptr) {
		throw SemanticAnalysisException("The category/rule '" + referenceName + "' is not defined in the context of machine '" + this->name + "' defined at " + this->locationString() + "' even though it has been referenced there");
	}

	return component->findRecursiveReference(*this, namesEncountered, targetName);
}

SemanticAnalysisException::SemanticAnalysisException(const std::string& message, const IFileLocalizable& somethingLocalizableToPinpointLocationBy)
	: Exception(message + " -- on " + somethingLocalizableToPinpointLocationBy.locationString()) { }


void FiniteAutomaton::checkForComponentRecursion() const {
	std::list<std::string> relevantNamesEncountered;
	for (const auto& componentPair : components) {
		auto recursiveReferenceLocalizableInstance = componentPair.second->findRecursiveReference(*this, relevantNamesEncountered, componentPair.first);
		if (recursiveReferenceLocalizableInstance != nullptr) {
			std::string hierarchyPath = relevantNamesEncountered.front();
			relevantNamesEncountered.pop_front();
			for (const auto& nameEncountered : relevantNamesEncountered) {
				hierarchyPath += "-" + nameEncountered;
			}
			throw SemanticAnalysisException("Rule/category reference recursion found in the path " + hierarchyPath + "; start at " + componentPair.second->locationString() + ", end at " + recursiveReferenceLocalizableInstance->locationString() + " - no recursion is allowed in finite automata");
		}
	}
}

std::shared_ptr<const ISyntacticEntity> FiniteAutomaton::underlyingSyntacticEntity() const {
	return m_finiteAutomatonDefinition;
}

void FiniteAutomaton::initialize() {
	this->Machine::initialize();

	NFA base;
	for (const auto& componentPair : this->components) {
		const MachineComponent* componentPtr = componentPair.second.get();
		NFABuilder builder(*this, componentPtr, "");
		base.addContextedAlternative(componentPtr->accept(builder), "token", "__" + componentPtr->name, false);
	}

	if (this->type == FiniteAutomatonType::Deterministic) {
		m_nfa = base.buildDFA();
	} else {
		m_nfa = base;
	}
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

void MachineComponent::checkFieldName(Machine& context, const Field* field) const {
	for (const auto& f : fields) {
		if (f->name == field->name && f.get() != field) {
			throw SemanticAnalysisException("The field '" + field->name + "' declared at " + field->locationString() + " uses the name already taken by the field declared at " + f->locationString());
		}
	}

	for (const Category* category : this->categories) {
		category->checkFieldName(context, field);
	}
}

void MachineComponent::checkFieldDeclarations(Machine& context) const {
	for (const auto& field : fields) {
		const VariablyTypedField* vtf = dynamic_cast<const VariablyTypedField*>(field.get());
		if (vtf) {
			MachineComponent* mc = context.findMachineComponent(vtf->type);
			if (!mc) {
				throw SemanticAnalysisException("The variably typed field '" + vtf->name + "' references type '" + vtf->type + "' at " + field->locationString() + ", but no such type (i.e. a category or production rule) could be found in the present context of the machine '" + context.name + "'");
			}
		}

		checkFieldName(context, field.get());
	}
}

const Field* MachineComponent::findField(const std::string& name) const {
	for (const auto& fieldPtr : fields) {
		if (fieldPtr->name == name) {
			return fieldPtr.get();
		}
	}

	for (const Category* category : categories) {
		auto maybeAField = category->findField(name);
		if (maybeAField) {
			return maybeAField;
		}
	}

	return nullptr;
}

void MachineComponent::verifyContextualValidity(const Machine& machine) const { }

const IFileLocalizable* Category::findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
	auto nEit = std::find(namesEncountered.cbegin(), namesEncountered.cend(), targetName);
	if (nEit != namesEncountered.cend()) {
		return nullptr;
	}

	namesEncountered.push_back(this->name);

	for (const auto& referencePair : this->references) {
		const IFileLocalizable* ret;
		if ((ret = referencePair.second.component->findRecursiveReference(machine, namesEncountered, targetName)) != nullptr) {
			return ret;
		}
	}

	namesEncountered.pop_back();
	return nullptr;
}

std::shared_ptr<const ISyntacticEntity> Category::underlyingSyntacticEntity() const {
	return m_categoryStatement;
}

bool Category::entails(const std::string& name) const {
	for (const auto& reference : references) {
		if (reference.second.component->entails(name)) {
			return true;
		}
	}

	return false;
}

bool Category::entails(const std::string& name, std::list<const Category*>& path) const {
	for (const auto& reference : references) {
		if (reference.second.component->entails(name, path)) {
			path.push_back(this);
			return true;
		}
	}

	return false;
}

NFA Category::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

const bool Category::isTypeForming() const {
	return true;
}

void Rule::initialize() {
	if (initialized()) {
		return;
	}

	MachineComponent::initialize();
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

std::shared_ptr<const ISyntacticEntity> Rule::underlyingSyntacticEntity() const {
	return m_ruleStatement;
}

bool Rule::entails(const std::string& name) const {
	return name == this->name;
}

bool Rule::entails(const std::string& name, std::list<const Category*>& path) const {
	return name == this->name;
}

const bool Rule::isTypeForming() const {
	return typeForming;
}

void Rule::verifyContextualValidity(const Machine& machine) const {
	regex->checkActionUsage(machine, this);
}

NFA Rule::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}
