#include "MachineDefinition.h"

#include "SemanticAnalysisException.h"

#include <algorithm>

void MachineDefinition::initialize() {
	if (initialized()) {
		return;
	}

	ISemanticEntity::initialize();

	// here we simply make sure that the dependencies are initialized before the machine itself.
	// Note that since at this point we are guaranteed that there are no recursions assumed the process is consequently also guaranteed to terminated
	if (on.second) {
		if (on.first == this->name) {
			throw SemanticAnalysisException("The machine '" + name + "' referenced itself in the `on` clause (can not use itself for input)", *this);
		}

		on.second->initialize();
		if (on.second->hasPurelyTerminalRoots()) {
			this->m_isOnTerminalInput = true;
		}
	} else {
		this->m_isOnTerminalInput = true;
	}
	

	for (const auto& usedPair : uses) {
		if (usedPair.first == this->name) {
			throw SemanticAnalysisException("The machine '" + name + "' referenced itself in the `uses` clause (can not use itself as a dependency machine)", *this);
		}
		if (usedPair.second->on.first != on.first) {
			throw SemanticAnalysisException("The machine '" + usedPair.first + "' declared at " + usedPair.second->locationString() + " referenced in the `uses` clause of '" + name + "' accepts input (specified in the `on` clause if present) different from the one of '" + name + "' -- inputs of the current machine and all of its dependencies have to be identical", *this);
		}
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
					if (productionStatementPtr) {
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

					terminalityDecision = machineDefinitionAttributeIterator->second.value ? ++m_terminalCount : (TerminalTypeIndex)0;
				} else if(productionStatementPtr->terminality == Terminality::Terminal) {
					terminalityDecision = ++m_terminalCount;
				}
				productionStatementPtr->terminalTypeIndex = terminalityDecision;
			}
		}
	}

	// check for possible category reference recursion (and obviously whether the referenced category is indeed a category)
	std::list<std::string> namesEncountered;
	for (const auto& statementPair : statements) {
		auto attributedStatementPtr = std::dynamic_pointer_cast<AttributedStatement>(statementPair.second);
		if (attributedStatementPtr) {
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
		if (attributedStatementPtr) {
			attributedStatementPtr->completeFieldDeclarations(*this);
		}
	}

	// then, once the basic field and rule/category reference verifications have been conducted, we need to check whether the actions of individual rules are contextually valid
	// wasFoundInUnderlyingMachine the rule and category level we need to check that there is no disallowed recursion within the productions themselves
	for (const auto& statementPair : statements) {
		auto ruleStatement = std::dynamic_pointer_cast<RuleStatement>(statementPair.second);
		if (ruleStatement) {
			ruleStatement->completeReferences(*this);
			ruleStatement->verifyContextualValidity(*this);
		}
	}
}

std::shared_ptr<MachineStatement> MachineDefinition::findMachineStatement(const std::string& name, const MachineDefinition** sourceMachine) const {
	std::shared_ptr<MachineStatement> ret;
	for (const auto& used : uses) {
		if ((ret = used.second->findMachineStatement(name, sourceMachine))) {
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
		if (productionPtr && productionPtr->terminality == Terminality::Terminal) {
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
		if (typeFormingStatementPtr && typeFormingStatementPtr->rootness != Rootness::Unspecified) {
			auto productionPtr = std::dynamic_pointer_cast<ProductionStatement>(typeFormingStatementPtr);
			if (productionPtr && productionPtr->terminality == Terminality::Terminal) {
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

	for (auto& categoryReferencePair : attributedStatement->categories) {
		if (!categoryReferencePair.second) {
			auto soNamedStatement = this->findMachineStatement(categoryReferencePair.first);
			if (!soNamedStatement) {
				throw SemanticAnalysisException("'" + nameConsidered + "' was not found in the present context, though it was referenced as a category in the declaration of '" + attributedStatement->name + "'", *attributedStatement);
			}

			auto soNamedCategory = std::dynamic_pointer_cast<CategoryStatement>(soNamedStatement);
			if (!soNamedCategory) {
				throw SemanticAnalysisException("'" + nameConsidered + "' was found in the present context at " + soNamedStatement->locationString() + " but not as a category, though it was referenced as such in the declaration of '" + attributedStatement->name + "' at " + attributedStatement->locationString());
			}

			categoryReferencePair.second = soNamedCategory;
		}
		categoryReferencePair.second->references.emplace(attributedStatement->name, CategoryReference(attributedStatement.get(), false));
		completeCategoryReferences(namesEncountered, categoryReferencePair.second, true);
	}

	namesEncountered.pop_back();
}

SymbolGroupList MachineDefinition::computeArbitrarySymbolGroupList() const {
	SymbolGroupList ret;
	if (on.second) {
		for (const auto& root : this->getRoots()) {
			ret.push_back(std::make_shared<StatementSymbolGroup>(root.get(), on.second.get()));
		}
	} else {
		ret.push_back(std::make_shared<ByteSymbolGroup>((CharType)0, (CharType)255));
	}
	return ret;
}

MachineDefinition::MachineDefinition(const std::map<MachineFlag, MachineDefinitionAttribute>& attributes)
	: MachineDefinition() {
	mergeInAttributes(attributes);
}

void MachineDefinition::mergeInAttributes(const std::map<MachineFlag, MachineDefinitionAttribute>& attributes) {
	for (const auto& incomingAttributePair : attributes) {
		this->attributes[incomingAttributePair.first] = incomingAttributePair.second;
	}
}
