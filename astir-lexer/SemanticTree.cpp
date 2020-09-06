#include "SyntacticTree.h"
#include "SemanticTree.h"
#include "NFABuilder.h"

#include "GenerationVisitor.h"

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
	if (!definitionConsidered->on.empty()) {
		checkForMachineHierarchyRecursion(namesEncountered, definitionConsidered->on);
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

void SemanticTree::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
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

	if (on) {
		on->initialize();
	}

	const MachineDefinition* machineDefinition = dynamic_cast<const MachineDefinition*>(underlyingSyntacticEntity().get());
	// build up the name database
	for (const auto& statementPtr : machineDefinition->categoryStatements) {
		if (findMachineComponent(statementPtr->name)) {
			throw SemanticAnalysisException("Category name '" + statementPtr->name + "' encountered in machine '" + name + "' already defined", *this); // TODO: could use an improvement; already defined where?
		}

		components[statementPtr->name] = std::make_shared<Category>(statementPtr, statementPtr->name);
	}

	for (const auto& statementPtr : machineDefinition->ruleStatements) {
		if (findMachineComponent(statementPtr->name)) {
			throw SemanticAnalysisException("Rule name '" + statementPtr->name + "' encountered in machine '" + name + "' already defined", *this); // TODO: could use an improvement; already defined where?
		}

		std::shared_ptr<Rule> rulePtr;

		// decide on the type
		RuleStatementType typeDecision;
		if (statementPtr->typeSpecified) {
			typeDecision = statementPtr->type;
		} else {
			const auto machineDefinitionAttributeIterator = machineDefinition->attributes.find(MachineFlag::RulesProductionsByDefault);
			typeDecision = machineDefinitionAttributeIterator->second.value ? RuleStatementType::Production : RuleStatementType::Pattern;
		}

		// then, if applicable, also decide on the terminality of the rule
		if (typeDecision == RuleStatementType::Pattern) {
			if (statementPtr->terminalitySpecified) {
				throw SemanticAnalysisException("Pattern '" + statementPtr->name + "' was specified as '" + (statementPtr->terminality ? "terminal" : "nonterminal") + "' but patterns cannot have terminality", *statementPtr);
			}

			/*
			THIS IS ACTUALLY LEGAL
			if (!statementPtr->categories.empty()) {
				throw SemanticAnalysisException("Pattern '" + statementPtr->name + "' refers to at least one category, but patterns cannot inherit from categories as they are not type-forming", *statementPtr);
			}*/

			rulePtr = std::make_shared<Pattern>(statementPtr, statementPtr->name, statementPtr->disjunction);
		} else if (typeDecision == RuleStatementType::Production) {
			TerminalTypeIndex terminalityDecision = (TerminalTypeIndex)0;
			if (!statementPtr->terminalitySpecified) {
				const auto machineDefinitionAttributeIterator = machineDefinition->attributes.find(MachineFlag::ProductionsTerminalByDefault);
				terminalityDecision = machineDefinitionAttributeIterator->second.value ? ++m_terminalCount : (TerminalTypeIndex)0;
			} else {
				terminalityDecision = statementPtr->terminality ? ++m_terminalCount : (TerminalTypeIndex)0;
			}
			rulePtr = std::make_shared<Production>(statementPtr, statementPtr->name, statementPtr->disjunction, terminalityDecision);
		}

		// register the newly build semantic object
		components[statementPtr->name] = rulePtr;
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
			const Machine* mcm;
			auto mc = findMachineComponent(categoryUsedName, &mcm);
			Category* cat = dynamic_cast<Category*>(mc);
			componentPair.second->categories.push_back(cat);
			cat->references[componentPair.first] = CategoryReference(componentPair.second.get(), mcm->name != name);
		}
	}

	// at this point we can proceed to initializing the MachineComponents internally (e.g. initializing their fields)
	for (const auto& componentPair : components) {
		componentPair.second->initialize();
	}

	// now that the fields are initialized, we can proceed to checking that there are no name collisions and that all types used are valid
	for (const auto& componentPair : components) {
		componentPair.second->checkAndTypeformFieldDeclarations(*this);
	}

	// wasFoundInUnderlyingMachine the rule and category level we need to check that there is no disallowed recursion within the productions themselves
	checkForComponentRecursion();

	// once the basic field and rule/category reference verifications have been conducted, we need to check whether the actions of individual rules are contextually valid
	for (const auto& componentPair : components) {
		componentPair.second->verifyContextualValidity(*this);
	}
}

MachineComponent* Machine::findMachineComponent(const std::string& name, const Machine** sourceMachine) const {
	MachineComponent* ret;
	for (const auto& used : uses) {
		if (ret = used->findMachineComponent(name)) {
			return ret;
		}
	}
	
	if (this->on && (ret = this->on->findMachineComponent(name, sourceMachine))) {
		return ret;
	}

	auto it = components.find(name);
	if (it != components.cend()) {
		if (sourceMachine) {
			*sourceMachine = this;
		}
		return it->second.get();
	}

	return nullptr;
}

std::list<const MachineComponent*> Machine::getTerminalComponents() const {
	std::list<const MachineComponent*> terminals;

	for (const auto& machineComponentPair : this->components) {
		if (machineComponentPair.second->isTerminal()) {
			terminals.push_back(machineComponentPair.second.get());
		}
	}

	return terminals;
}

std::list<const MachineComponent*> Machine::getTerminalTypeComponents() const {
	std::list<const MachineComponent*> terminals;

	for (const auto& machineComponentPair : this->components) {
		if (machineComponentPair.second->isTerminal() && machineComponentPair.second->isTypeForming()) {
			terminals.push_back(machineComponentPair.second.get());
		}
	}

	return terminals;
}

std::list<const MachineComponent*> Machine::getTypeComponents() const {
	std::list<const MachineComponent*> terminals;

	for (const auto& machineComponentPair : this->components) {
		if (machineComponentPair.second->isTypeForming()) {
			terminals.push_back(machineComponentPair.second.get());
		}
	}

	return terminals;
}

bool Machine::hasPurelyTerminalRoots() const {
	for (const auto& machineComponentPair : this->components) {
		if (/* machineComponentPair.second->isRoot() && */!machineComponentPair.second->isTerminal()) {
			return false;
		}
	}

	return true;
}

std::list<const MachineComponent*> Machine::getRoots() const {
	std::list<const MachineComponent*> terminalRoots;

	for (const auto& machineComponentPair : this->components) {
		terminalRoots.push_back(machineComponentPair.second.get());
	}

	return terminalRoots;
}

std::list<const Production*> Machine::getProductionRoots() const {
	std::list<const Production*> productionRoots;

	for (const auto& machineComponentPair : this->components) {
		// TODO differentiate between roots and non-roots
		productionRoots.merge(machineComponentPair.second->calculateInstandingProductions()); //WTF? I can't remember why this is here... maybe because of categories? Sort it out in your head!!
	}

	return productionRoots;
}

std::list<const Production*> Machine::getTerminalRoots() const {
	std::list<const Production*> productionRoots;

	for (const auto& machineComponentPair : this->components) {
		if(machineComponentPair.second->isTerminal()) {
			// TODO differentiate between roots and non-roots
			productionRoots.merge(machineComponentPair.second->calculateInstandingProductions()); // the inside call is not strictly needed, but it does help to avoid the dynamic cast in a virtual-method way
			// with merge and move constructor it should also be fairly efficient...
		}
	}

	return productionRoots;
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
	: Exception(message + " -- wasFoundInUnderlyingMachine " + somethingLocalizableToPinpointLocationBy.locationString()) { }


void FiniteAutomatonMachine::checkForComponentRecursion() const {
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

std::shared_ptr<const ISyntacticEntity> FiniteAutomatonMachine::underlyingSyntacticEntity() const {
	return m_finiteAutomatonDefinition;
}

void FiniteAutomatonMachine::initialize() {
	this->Machine::initialize();

	if (this->on) {
		if (!on->hasPurelyTerminalRoots()) {
			throw SemanticAnalysisException("The finite automaton '" + this->name + "' declared at " + this->locationString() + "' references the machine '" + this->on->name + "' that does not have purely terminal roots - such a machine can not serve as input for a finite automaton, and '" + this->name + "' is no exception");
		}
	}

	NFA base;
	NFABuilder builder(*this, nullptr, "m_token");
	for (const auto& componentPair : this->components) {
		const MachineComponent* componentPtr = componentPair.second.get();
		const std::string& newSubcontextName = componentPtr->name;
		const INFABuildable* componentCastPtr = dynamic_cast<const INFABuildable*>(componentPtr);
		NFA alternativeNfa = componentCastPtr->accept(builder);

		NFAActionRegister elevateContextActionRegister;
		if (componentPair.second->isTypeForming()) {
			// if the component is type-forming, a new context has been created in alternativeNfa and it needs to be elevated to the category level
			// but, if it is also terminal, we need to associate the raw capture with the context before elevating
			if (componentPair.second->isTerminal()) {
				elevateContextActionRegister.emplace_back(NFAActionType::TerminalizeContext, "m_token", newSubcontextName);
			}
			elevateContextActionRegister.emplace_back(NFAActionType::ElevateContext, "m_token", newSubcontextName);
			alternativeNfa.concentrateFinalStates(elevateContextActionRegister);
		}
		alternativeNfa.concentrateFinalStates(elevateContextActionRegister);

		base |= alternativeNfa;
	}

	m_nfa = base.buildPseudoDFA();
}

void FiniteAutomatonMachine::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
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

void MachineComponent::checkAndTypeformFieldDeclarations(Machine& context) const {
	for (const auto& field : fields) {
		VariablyTypedField* vtf = dynamic_cast<VariablyTypedField*>(field.get());
		if (vtf) {
			const Machine* mcm;
			MachineComponent* mc = context.findMachineComponent(vtf->type, &mcm);
			if (!mc) {
				throw SemanticAnalysisException("The variably typed field '" + vtf->name + "' references type '" + vtf->type + "' at " + field->locationString() + ", but no such type (i.e. a category or production rule) could be found in the present context of the machine '" + context.name + "'");
			}

			vtf->machineOfTheType = mcm;
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

void MachineComponent::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
}

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

const bool Category::isTerminal() const {
	return false;
}

std::list<const Production*> Category::calculateInstandingProductions() const {
	std::list<const Production*> ret;

	for (const auto& referencePair : references) {
		ret.merge(referencePair.second.component->calculateInstandingProductions());
	}

	return ret;
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

void Rule::verifyContextualValidity(const Machine& machine) const {
	regex->checkAndTypeformActionUsage(machine, this);
}

const bool Pattern::isTypeForming() const {
	return false;
}

const bool Pattern::isTerminal() const {
	return false;
}

NFA Pattern::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

std::list<const Production*> Pattern::calculateInstandingProductions() const {
	throw SemanticAnalysisException("Pattern '" + name + "' asked to calculateInstandingProductions() despite being a pattern - has the semantic check for terminal-purity of input machines failed?", *this);

	// the following no-op would work just fine...
	// but I want an exception to break in case I've done something stupid on the semantic check level
	return std::list<const Production*>();
}

const bool Production::isTypeForming() const {
	return true;
}

const bool Production::isTerminal() const {
	return typeIndex != 0;
}

NFA Production::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

std::list<const Production*> Production::calculateInstandingProductions() const {
	return std::list<const Production*>({ this });
}
