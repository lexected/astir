#include "FiniteAutomatonDefinition.h"

#include "SemanticAnalysisException.h"
#include "NFABuilder.h"
#include "GenerationVisitor.h"

void FiniteAutomatonDefinition::initialize() {
	if (initialized()) { // really necessary
		return;
	}

	this->MachineDefinition::initialize();

	if (!this->uses.empty()) {
		throw SemanticAnalysisException("The machine '" + name + "' declared at " + locationString() + " `uses` at least one other machine -- finite automata using other machines is not supported, but the finite automaton can still be `on` input from some other machine");
	}

	// check for any signs of recursion
	for (const auto& statementPair : statements) {
		std::list<IReferencingCPtr> relevantReferencesEncountered;
		auto recursiveReferenceLocalizableInstance = statementPair.second->findRecursiveReference(relevantReferencesEncountered);
		if (recursiveReferenceLocalizableInstance != nullptr) {
			std::string hierarchyPath = relevantReferencesEncountered.front()->referenceName();
			// relevantReferencesEncountered.pop_front();
			for (const auto& referenceEncountered : relevantReferencesEncountered) {
				hierarchyPath += "-" + referenceEncountered->referenceName();
			}
			throw SemanticAnalysisException("Rule/category reference recursion found in the path " + hierarchyPath + "; start at " + statementPair.second->locationString() + ", end at " + recursiveReferenceLocalizableInstance->locationString() + " - no recursion is allowed in finite automata");
		}
	}

	if (this->on.second) {
		if (!on.second->hasPurelyTerminalRoots()) {
			throw SemanticAnalysisException("The finite automaton '" + this->name + "' declared at " + this->locationString() + "' references the machine '" + this->on.first + "' that does not have purely terminal roots - such a machine can not serve as input for a finite automaton, and '" + this->name + "' is no exception");
		}
	}

	NFA base;
	NFABuilder builder(*this, nullptr, "m_token");
	auto typeFormingStatement = this->getTypeFormingStatements();
	for (const auto& typeFormingStatement : typeFormingStatement) {
		if (typeFormingStatement->rootness == Rootness::Unspecified) {
			continue;
		}

		const std::string& newSubcontextName = typeFormingStatement->name;
		std::shared_ptr<INFABuildable> componentCastPtr = std::dynamic_pointer_cast<INFABuildable>(typeFormingStatement);
		NFA alternativeNfa = componentCastPtr->accept(builder);

		NFAActionRegister elevateContextActionRegister;
		// if the component is type-forming, a new context has been created in alternativeNfa and it needs to be elevated to the category level
		// but, if it is also terminal, we need to associate the raw capture with the context before elevating
		auto productionStatement = std::dynamic_pointer_cast<ProductionStatement>(typeFormingStatement);
		if (productionStatement && productionStatement->terminality == Terminality::Terminal) {
			elevateContextActionRegister.emplace_back(NFAActionType::TerminalizeContext, "m_token", newSubcontextName);
		}

		// if the typeformingStatement is an ignored root, create the NFA and the context but do not elevate here
		if (typeFormingStatement->rootness == Rootness::AcceptRoot) {
			elevateContextActionRegister.emplace_back(NFAActionType::ElevateContext, "m_token", newSubcontextName);
		} else if (typeFormingStatement->rootness == Rootness::IgnoreRoot) {
			elevateContextActionRegister.emplace_back(NFAActionType::IgnoreContext, "m_token", newSubcontextName);
		}

		alternativeNfa.concentrateFinalStates(elevateContextActionRegister);

		base |= alternativeNfa;
	}

	m_nfa = base.buildPseudoDFA();
}

void FiniteAutomatonDefinition::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
}