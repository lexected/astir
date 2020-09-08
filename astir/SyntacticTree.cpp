#include "SyntacticTree.h"
#include "GenerationVisitor.h"
#include "NFA.h"
#include "NFABuilder.h"

#include <set>

#include "SemanticAnalysisException.h"

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


std::shared_ptr<Field> AttributedStatement::findField(const std::string& name, std::shared_ptr<CategoryStatement>& categoryFoundIn) const {
	auto fit = std::find_if(fields.cbegin(), fields.cend(), [&name](const auto& fieldPtr) {
		return fieldPtr->name == name;
	});

	if (fit != fields.cend()) {
		categoryFoundIn = nullptr;
		return *fit;
	} else {
		return findCategoryField(name, categoryFoundIn);
	}
}

std::shared_ptr<Field> AttributedStatement::findCategoryField(const std::string& name, std::shared_ptr<CategoryStatement>& categoryFoundIn) const {
	for (const auto& categoryPair : categories) {
		auto foundPtr = categoryPair.second->findField(name, categoryFoundIn);
		if (foundPtr) {
			if (!categoryFoundIn) {
				categoryFoundIn = categoryPair.second;
			}
			return foundPtr;
		}
	}
}

void AttributedStatement::completeFieldDeclarations(MachineDefinition& context) const {
	for (auto it = fields.cbegin(); it != fields.cend(); ++it) {
		const auto& currentField = *it;

		auto previousOccurenceOfTheName = std::find_if(fields.cbegin(), it, [&currentField](const auto& fieldPtr) {
			return fieldPtr->name == currentField->name;
			});

		if (previousOccurenceOfTheName != it) {
			throw SemanticAnalysisException("The field '" + currentField->name + "' declared at " + currentField->locationString() + " uses the name already taken by the field declared at " + (*it)->locationString());
		}

		std::shared_ptr<CategoryStatement> categoryFoundIn;
		auto foundFieldPtr = findCategoryField(currentField->name, categoryFoundIn);
		if (foundFieldPtr) {
			throw SemanticAnalysisException("The field '" + currentField->name + "' declared at " + currentField->locationString() + " uses the name already taken by the field declared at " + foundFieldPtr->locationString() + "' in category '" + categoryFoundIn->name);
		}

		VariablyTypedField* vtf = dynamic_cast<VariablyTypedField*>(currentField.get());
		if (vtf) {
			const MachineDefinition* mcm;
			std::shared_ptr<MachineStatement> mc = context.findMachineStatement(vtf->type, &mcm);
			if (!mc) {
				throw SemanticAnalysisException("The variably typed currentField '" + vtf->name + "' references type '" + vtf->type + "' at " + currentField->locationString() + ", but no such type (i.e. a category or production rule) could be found in the present context of the machine '" + context.name + "'");
			}

			vtf->machineOfTheType = mcm;
		}
	}
}


void MachineDefinition::initialize() {
	if (initialized()) {
		return;
	}

	ISemanticEntity::initialize();

	// here we simply make sure that the dependencies are initialized before the machine itself.
	// Note that since at this point we are guaranteed that there are no recursions assumed the process is consequently also guaranteed to terminated
	if (on.second) {
		on.second->initialize();
	}

	for (const auto& usedPair : uses) {
		usedPair.second->initialize();
	}

	for (const auto& statementPair : statements) {
		const auto& statementPtr = statementPair.second;

		auto typeFormingStatementPtr = std::dynamic_pointer_cast<TypeFormingStatement>(statementPtr);
		if (typeFormingStatementPtr) {
			if (typeFormingStatementPtr->rootness == Rootness::Unspecified) {
				auto categoryStatementPtr = std::dynamic_pointer_cast<CategoryStatement>(typeFormingStatementPtr);
				if (categoryStatementPtr) {
					const auto machineDefinitionAttributeIterator = attributes.find(MachineFlag::CategoriesRootByDefault);
					categoryStatementPtr->rootness = machineDefinitionAttributeIterator->second.value ? Rootness::AcceptRoot : Rootness::Unspecified;
				} else {
					auto productionStatementPtr = std::dynamic_pointer_cast<ProductionStatement>(typeFormingStatementPtr);
					if(productionStatementPtr) {
						const auto machineDefinitionAttributeIterator = attributes.find(MachineFlag::ProductionsRootByDefault);
						productionStatementPtr->rootness = machineDefinitionAttributeIterator->second.value ? Rootness::AcceptRoot : Rootness::Unspecified;
					}
				}
			}

			auto productionStatementPtr = std::dynamic_pointer_cast<ProductionStatement>(typeFormingStatementPtr);
			if (productionStatementPtr) {
				TerminalTypeIndex terminalityDecision = (TerminalTypeIndex)0;
				if (productionStatementPtr->terminality == Terminality::Unspecified) {
					const auto machineDefinitionAttributeIterator = attributes.find(MachineFlag::ProductionsTerminalByDefault);
					productionStatementPtr->terminality = machineDefinitionAttributeIterator->second.value ? Terminality::Terminal : Terminality::Nonterminal;

					productionStatementPtr->terminalTypeIndex = machineDefinitionAttributeIterator->second.value ? ++m_terminalCount : (TerminalTypeIndex)0;
				}
			}
		}
	}

	// check for possible category reference recursion (and obviously whether the referenced category is indeed a category)
	std::list<std::string> namesEncountered;
	for (const auto& statementPair : statements) {
		auto attributedStatementPtr = std::dynamic_pointer_cast<AttributedStatement>(statementPair.second);
		if(attributedStatementPtr) {
			completeCategoryReferences(namesEncountered, attributedStatementPtr);
		}
	}

	// at this point we can proceed to initializing the MachineStatements internally (e.g. initializing their fields)
	for (const auto& statementPair : statements) {
		statementPair.second->initialize();
	}

	// now that the fields are initialized, we can proceed to checking that there are no name collisions and that all types used are valid
	for (const auto& statementPair : statements) {
		auto attributedStatementPtr = std::dynamic_pointer_cast<AttributedStatement>(statementPair.second);
		if(attributedStatementPtr) {
			attributedStatementPtr->completeFieldDeclarations(*this);
		}
	}

	// then, once the basic field and rule/category reference verifications have been conducted, we need to check whether the actions of individual rules are contextually valid
	// wasFoundInUnderlyingMachine the rule and category level we need to check that there is no disallowed recursion within the productions themselves
	for (const auto& statementPair : statements) {
		auto ruleStatement = std::dynamic_pointer_cast<RuleStatement>(statementPair.second);
		if(ruleStatement) {
			ruleStatement->verifyContextualValidity(*this);
		}

		std::list<std::string> relevantNamesEncountered;
		auto recursiveReferenceLocalizableInstance = statementPair.second->findRecursiveReference(*this, relevantNamesEncountered, statementPair.first);
		if (recursiveReferenceLocalizableInstance != nullptr) {
			std::string hierarchyPath = relevantNamesEncountered.front();
			relevantNamesEncountered.pop_front();
			for (const auto& nameEncountered : relevantNamesEncountered) {
				hierarchyPath += "-" + nameEncountered;
			}
			throw SemanticAnalysisException("Rule/category reference recursion found in the path " + hierarchyPath + "; start at " + statementPair.second->locationString() + ", end at " + recursiveReferenceLocalizableInstance->locationString() + " - no recursion is allowed in finite automata");
		}
	}
}

std::shared_ptr<MachineStatement> MachineDefinition::findMachineStatement(const std::string& name, const MachineDefinition** sourceMachine) const {
	std::shared_ptr<MachineStatement> ret;
	for (const auto& used : uses) {
		if (ret = used.second->findMachineStatement(name)) {
			return ret;
		}
	}

	if (this->on.second && (ret = this->on.second->findMachineStatement(name, sourceMachine))) {
		return ret;
	}

	auto it = statements.find(name);
	if (it != statements.cend()) {
		if (sourceMachine) {
			*sourceMachine = this;
		}
		return it->second;
	}

	return nullptr;
}

std::list<std::shared_ptr<ProductionStatement>> MachineDefinition::getTerminalProductions() const {
	std::list<std::shared_ptr<ProductionStatement>> ret;
	
	for (const auto& statementPair : statements) {
		auto productionPtr = std::dynamic_pointer_cast<ProductionStatement>(statementPair.second);
		if (productionPtr) {
			ret.push_back(productionPtr);
		}
	}

	return ret;
}

std::list<std::shared_ptr<TypeFormingStatement>> MachineDefinition::getTypeFormingStatements() const {
	std::list<std::shared_ptr<TypeFormingStatement>> ret;

	for (const auto& statementPair : statements) {
		auto typeFormingStatementPtr = std::dynamic_pointer_cast<TypeFormingStatement>(statementPair.second);
		if (typeFormingStatementPtr) {
			ret.push_back(typeFormingStatementPtr);
		}
	}

	return ret;
}

bool MachineDefinition::hasPurelyTerminalRoots() const {
	for (const auto& statementPair : statements) {
		auto typeFormingStatementPtr = std::dynamic_pointer_cast<TypeFormingStatement>(statementPair.second);
		if (typeFormingStatementPtr && typeFormingStatementPtr->rootness == Rootness::AcceptRoot) {
			auto productionPtr = std::dynamic_pointer_cast<ProductionStatement>(typeFormingStatementPtr);
			if (!productionPtr) {
				return false;
			}

			if (productionPtr->terminality != Terminality::Terminal) {
				return false;
			}
		}
	}

	return true;
}

std::list<std::shared_ptr<TypeFormingStatement>> MachineDefinition::getRoots() const {
	std::list<std::shared_ptr<TypeFormingStatement>> ret;

	for (const auto& statementPair : statements) {
		auto typeFormingStatementPtr = std::dynamic_pointer_cast<TypeFormingStatement>(statementPair.second);
		if (typeFormingStatementPtr && typeFormingStatementPtr->rootness == Rootness::AcceptRoot) {
			ret.push_back(typeFormingStatementPtr);
		}
	}

	return ret;
}

std::list<const ProductionStatement*> MachineDefinition::getUnderlyingProductionsOfRoots() const {
	std::list<const ProductionStatement*> ret;

	for (const auto& statementPair : statements) {
		auto typeFormingPtr = std::dynamic_pointer_cast<TypeFormingStatement>(statementPair.second);
		if (typeFormingPtr && typeFormingPtr->rootness == Rootness::AcceptRoot) {
			auto toInsert = typeFormingPtr->calculateInstandingProductions();
			ret.insert(ret.end(), toInsert.cbegin(), toInsert.cend());
		}
	}

	return ret;
}

std::list<std::shared_ptr<ProductionStatement>> MachineDefinition::getTerminalRoots() const {
	std::list<std::shared_ptr<ProductionStatement>> ret;

	for (const auto& statementPair : statements) {
		auto typeFormingStatementPtr = std::dynamic_pointer_cast<TypeFormingStatement>(statementPair.second);
		if (typeFormingStatementPtr && typeFormingStatementPtr->rootness == Rootness::AcceptRoot) {
			auto productionPtr = std::dynamic_pointer_cast<ProductionStatement>(typeFormingStatementPtr);
			if (productionPtr && productionPtr->terminality != Terminality::Terminal) {
				ret.push_back(productionPtr);
			}
		}
	}

	return ret;
}

void MachineDefinition::completeCategoryReferences(std::list<std::string> namesEncountered, const std::shared_ptr<AttributedStatement>& attributedStatement, bool mustBeACategory) const {
	const std::string& nameConsidered = attributedStatement->name;
	auto fit = std::find(namesEncountered.cbegin(), namesEncountered.cend(), nameConsidered);
	const bool collision = fit != namesEncountered.cend();
	namesEncountered.push_back(nameConsidered);

	if (collision) {
		std::string hierarchyPath = namesEncountered.front();
		namesEncountered.pop_front();
		for (const auto& nameEncountered : namesEncountered) {
			hierarchyPath += "-" + nameEncountered;
		}
		throw SemanticAnalysisException("Declaration recursion found in the category-use declaration hierarchy path " + hierarchyPath + " at " + attributedStatement->locationString());
	}

	std::shared_ptr<CategoryStatement> category = std::dynamic_pointer_cast<CategoryStatement>(attributedStatement);
	if (!category && mustBeACategory) {
		throw SemanticAnalysisException(nameConsidered + "' was found in the present context at " + attributedStatement->locationString() + " but not as a category, though it was referenced as such in the declaration of " + namesEncountered.back());
	}

	for (const auto& categoryReferencePair : attributedStatement->categories) {
		categoryReferencePair.second->references.emplace(attributedStatement->name, CategoryReference(attributedStatement.get(), false));
		completeCategoryReferences(namesEncountered, categoryReferencePair.second, true);
	}

	namesEncountered.pop_back();
}

const IFileLocalizable* MachineDefinition::findRecursiveReferenceThroughName(const std::string& referenceName, std::list<std::string>& namesEncountered, const std::string& targetName) const {
	auto statement = findMachineStatement(referenceName);
	if (!statement) {
		throw SemanticAnalysisException("The name '" + referenceName + "' is not defined in the context of machine '" + this->name + "' defined at " + this->locationString() + "' even though it has been referenced there");
	}

	return statement->findRecursiveReference(*this, namesEncountered, targetName);
}

void FiniteAutomatonDefinition::initialize() {
	if (initialized()) { // really necessary
		return;
	}

	this->MachineDefinition::initialize();

	if (this->on.second) {
		if (!on.second->hasPurelyTerminalRoots()) {
			throw SemanticAnalysisException("The finite automaton '" + this->name + "' declared at " + this->locationString() + "' references the machine '" + this->on.first + "' that does not have purely terminal roots - such a machine can not serve as input for a finite automaton, and '" + this->name + "' is no exception");
		}
	}

	NFA base;
	NFABuilder builder(*this, nullptr, "m_token");
	auto typeFormingStatement = this->getTypeFormingStatements();
	for (const auto& statement : typeFormingStatement) {
		if (statement->rootness != Rootness::AcceptRoot) {
			continue;
		}

		const std::string& newSubcontextName = statement->name;
		std::shared_ptr<INFABuildable> componentCastPtr = std::dynamic_pointer_cast<INFABuildable>(statement);
		NFA alternativeNfa = componentCastPtr->accept(builder);

		NFAActionRegister elevateContextActionRegister;
		// if the component is type-forming, a new context has been created in alternativeNfa and it needs to be elevated to the category level
		// but, if it is also terminal, we need to associate the raw capture with the context before elevating
		auto productionStatement = std::dynamic_pointer_cast<ProductionStatement>(statement);
		if (productionStatement && productionStatement->terminality == Terminality::Terminal) {
			elevateContextActionRegister.emplace_back(NFAActionType::TerminalizeContext, "m_token", newSubcontextName);
		}
		elevateContextActionRegister.emplace_back(NFAActionType::ElevateContext, "m_token", newSubcontextName);
		alternativeNfa.concentrateFinalStates(elevateContextActionRegister);
		alternativeNfa.concentrateFinalStates(elevateContextActionRegister);

		base |= alternativeNfa;
	}

	m_nfa = base.buildPseudoDFA();
}

void FiniteAutomatonDefinition::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
}

const IFileLocalizable* RuleStatement::findRecursiveReference(const MachineDefinition& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
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

const IFileLocalizable* CategoryStatement::findRecursiveReference(const MachineDefinition& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
	auto nEit = std::find(namesEncountered.cbegin(), namesEncountered.cend(), targetName);
	if (nEit != namesEncountered.cend()) {
		return nullptr;
	}

	namesEncountered.push_back(this->name);

	for (const auto& referencePair : this->references) {
		const IFileLocalizable* ret;
		if ((ret = referencePair.second.statement->findRecursiveReference(machine, namesEncountered, targetName)) != nullptr) {
			return ret;
		}
	}

	namesEncountered.pop_back();
	return nullptr;
}

std::list<const ProductionStatement*> CategoryStatement::calculateInstandingProductions() const {
	std::list<const ProductionStatement*> ret;

	for (const auto& referencePair : references) {
		auto instandingProductions = referencePair.second.statement->calculateInstandingProductions();
		ret.insert(ret.end(), instandingProductions.cbegin(), instandingProductions.cend());
	}

	return ret;
}

NFA CategoryStatement::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

void ProductionStatement::verifyContextualValidity(const MachineDefinition& machine) const {
	regex->checkAndTypeformActionUsage(machine, this, true);
}

std::list<const ProductionStatement*> ProductionStatement::calculateInstandingProductions() const {
	return std::list<const ProductionStatement*>({ this });
}

NFA ProductionStatement::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

void PatternStatement::verifyContextualValidity(const MachineDefinition& machine) const {
	regex->checkAndTypeformActionUsage(machine, this, true);
}

std::list<const ProductionStatement*> PatternStatement::calculateInstandingProductions() const {
	return std::list<const ProductionStatement*>(); // exactly what I want to return here - nothing
}

NFA PatternStatement::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

void RegexStatement::verifyContextualValidity(const MachineDefinition& machine) const {
	regex->checkAndTypeformActionUsage(machine, this, false);
}

NFA RegexStatement::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

void TypeFormingStatement::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
}
