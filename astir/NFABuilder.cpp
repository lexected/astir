#include "NFABuilder.h"
#include "Exception.h"

#include <stack>
#include <algorithm>

#include "SyntacticTree.h"
#include "MachineDefinition.h"
#include "Regex.h"
#include "SemanticAnalysisException.h"

NFA NFABuilder::visit(const CategoryStatement* category) const {
	NFA alternationPoint;
	const std::string parentContextPath = m_generationContextPath + "__" + category->name;
	const std::string generationContextPathPrefix = parentContextPath + "__";
	for (const auto referencePair : category->references) {
		const std::string& newSubcontextName = referencePair.first;
		const INFABuildable* componentCastIntoBuildable = dynamic_cast<const INFABuildable*>(referencePair.second.statement);
		NFA alternativeNfa = componentCastIntoBuildable->accept(*this);

		NFAActionRegister elevateContextActionRegister;
		
		auto typeFormingStatement = dynamic_cast<const TypeFormingStatement*>(referencePair.second.statement);
		if(typeFormingStatement) {
			// if the component is type-forming, a new context has been created in alternativeNfa and it needs to be elevated to the category level
			// but, if it is also terminal, we need to associate the raw capture with the context before elevating
			auto productionStatement = dynamic_cast<const ProductionStatement*>(typeFormingStatement);
			if (productionStatement && productionStatement->terminality == Terminality::Terminal) {
				elevateContextActionRegister.emplace_back(NFAActionType::TerminalizeContext, parentContextPath, newSubcontextName);
			}

			elevateContextActionRegister.emplace_back(NFAActionType::ElevateContext, parentContextPath, newSubcontextName);
			alternativeNfa.concentrateFinalStates(elevateContextActionRegister);
		}
		// if the component isn't type-forming, all the actions of the component (as of now just pattern) act on the category field
		// category context creation can not be omitted (although one could be tempted to say that it is not strictly necessary if the category referencess are all type-forming) since we do not know upfront whether there really are no patterns referring to the category
		// but yes, there is some space for optimisation here, as that can easily be figured out with a loop or during the reference building process

		alternationPoint |= alternativeNfa;
	}

	// add a transition from the first state to the second one, actioned by context creation
	NFAActionRegister createContextActionRegister;
	createContextActionRegister.emplace_back(NFAActionType::CreateContext, m_generationContextPath, category->name);
	alternationPoint.addInitialActions(createContextActionRegister);
	alternationPoint.registerContext(m_generationContextPath, category->name);

	return alternationPoint;
}

NFA NFABuilder::visit(const PatternStatement* patternStatement) const {
	const std::string newContextPath = m_generationContextPath + "__" + patternStatement->name;

	NFABuilder contextualizedBuilder(this->m_contextMachine, patternStatement, newContextPath);
	NFA regexNfa = patternStatement->regex->accept(contextualizedBuilder);

	return regexNfa;
}

NFA NFABuilder::visit(const ProductionStatement* productionStatement) const {
	const std::string newContextPath = m_generationContextPath + "__" + productionStatement->name;

	// compute the NFA of the underlying regex
	NFABuilder contextualizedBuilder(this->m_contextMachine, productionStatement, newContextPath);
	NFA regexNfa = productionStatement->regex->accept(contextualizedBuilder);
	
	// add context creation actions
	NFAActionRegister createContextActionRegister;
	createContextActionRegister.emplace_back(NFAActionType::CreateContext, m_generationContextPath, productionStatement->name);
	regexNfa.addInitialActions(createContextActionRegister);
	regexNfa.registerContext(m_generationContextPath, productionStatement->name);

	// return the contexted result
	return regexNfa;
}

NFA NFABuilder::visit(const RegexStatement* regexStatement) const {
	// we are certain that there will be no type forming nor any actions in this NFA, so let's just use this builder to build the NFA
	return regexStatement->regex->accept(*this);
}

NFA NFABuilder::visit(const DisjunctiveRegex* regex) const {
	NFA base;

	for (const auto& conjunctiveRegex : regex->disjunction) {
		base |= conjunctiveRegex->accept(*this);
	}

	NFAActionRegister initial, final;
	std::tie(initial, final) = computeActionRegisterEntries(regex->actions);
	base.addInitialActions(initial);
	base.addFinalActions(final);

	return base;
}

NFA NFABuilder::visit(const ConjunctiveRegex* regex) const {
	NFA base;
	base.finalStates.insert(0);

	for (const auto& rootRegex : regex->conjunction) {
		const INFABuildable* rootAsBuildable = rootRegex.get();
		base &= rootAsBuildable->accept(*this);
	}
	return base;
}

NFA NFABuilder::visit(const RepetitiveRegex* regex) const {
	if (regex->minRepetitions == regex->INFINITE_REPETITIONS) {
		throw Exception("Can not create a machine for a regex with minimum of infinitely many repetitions");
	}

	NFA base;
	if (regex->minRepetitions == 0) {
		/*
		OLD BEHAVIOUR ASSUMING OLD addInitialActions() BEHAVIOUR
		const State newState = base.addState();
		base.addEmptyTransition(0, newState);
		base.finalStates.insert(newState);
		*/
		base.finalStates.insert(0);
	}

	const INFABuildable* atomicAsBuildable = regex->regex.get();
	NFA atomMachine = atomicAsBuildable->accept(*this);
	if (regex->minRepetitions <= 1 && regex->maxRepetitions > 0) {
		base.orNFA(atomMachine, true);
	}

	NFA theLongBranch;
	theLongBranch.finalStates.insert(0);
	if (regex->maxRepetitions == regex->INFINITE_REPETITIONS) {
		if(regex->minRepetitions <= 2) {
			// must be!
			NFA doubleAtom(atomMachine);
			doubleAtom &= atomMachine;

			base.orNFA(doubleAtom, true);
		}

		unsigned long howManyToPrefix = std::max((unsigned long)2, regex->minRepetitions) - 1;
		for(unsigned long it = 0; it < howManyToPrefix;++it) {
			theLongBranch &= atomMachine;
		}
		const AFAState startLoopBodyFinalState = theLongBranch.concentrateFinalStates();
		theLongBranch &= atomMachine;
		const AFAState endLoopBodyFinalState = theLongBranch.concentrateFinalStates();
		theLongBranch.addEmptyTransition(endLoopBodyFinalState, startLoopBodyFinalState);
		theLongBranch.andNFA(atomMachine, true); // MUST BE TRUE
	} else if (regex->maxRepetitions >= 2) {
		unsigned long howManyToPrefix = std::max((unsigned long)2, regex->minRepetitions) - 1;
		for (unsigned long it = 0; it < howManyToPrefix; ++it) {
			theLongBranch &= atomMachine;
		}

		std::stack<AFAState> interimFinalStates;
		interimFinalStates.push(theLongBranch.concentrateFinalStates());

		for (auto it = regex->minRepetitions; it < regex->maxRepetitions;++it) {
			theLongBranch &= atomMachine;
			interimFinalStates.push(theLongBranch.concentrateFinalStates());
		}
		const AFAState endOfOptionalityState = interimFinalStates.top();
		interimFinalStates.pop();

		while (!interimFinalStates.empty()) {
			const AFAState interimState = interimFinalStates.top();
			theLongBranch.addEmptyTransition(interimState, endOfOptionalityState);
			interimFinalStates.pop();
		}

		theLongBranch.andNFA(atomMachine, true); // MUST BE TRUE
	}

	base.orNFA(theLongBranch, true);

	NFAActionRegister initial, final;
	std::tie(initial, final) = computeActionRegisterEntries(regex->actions);
	base.addInitialActions(initial);
	base.addFinalActions(final);

	return base;
}

NFA NFABuilder::visit(const EmptyRegex* regex) const {
	NFA base;
	const AFAState newState = base.addState();
	base.finalStates.insert(newState);

	NFAActionRegister initial, final;
	std::tie(initial, final) = computeActionRegisterEntries(regex->actions);
	initial += final;
	base.addEmptyTransition(0, newState, initial);
	
	return base;
}

NFA NFABuilder::visit(const AnyRegex* regex) const {
	NFA base;
	auto newState = base.addState();

	NFAActionRegister initial, final;
	std::tie(initial, final) = computeActionRegisterEntries(regex->actions);

	auto literalGroups = regex->makeSymbolGroups();
	for (auto& literalSymbolGroup : literalGroups) {
		base.addTransition(0, Transition(newState, literalSymbolGroup, initial));
	}

	base.finalStates.insert(newState);
	base.addFinalActions(final);

	return base;
}

NFA NFABuilder::visit(const ExceptAnyRegex* regex) const {
	NFA base;
	auto newState = base.addState();

	auto literalGroups = regex->makeSymbolGroups();
	auto complementedGroups = NFA::makeComplementSymbolGroups(literalGroups);

	NFAActionRegister initial, final;
	std::tie(initial, final) = computeActionRegisterEntries(regex->actions);

	for (auto& symbolGroup : complementedGroups) {
		base.addTransition(0, Transition(newState, symbolGroup, initial));
	}

	base.finalStates.insert(newState);
	base.addFinalActions(final);

	return base;
}

NFA NFABuilder::visit(const LiteralRegex* regex) const {
	NFA base;

	if (m_contextMachine.on.second) {
		throw SemanticAnalysisException("Encountered literal regex '" + regex->literal + "' at "+regex->locationString() + " within the finite automaton '" + m_contextMachine.name +"' (declared at " + m_contextMachine.locationString() + ") that refers to '" + m_contextMachine.on.first + "' for input (whereas literal regexes may only be used for finite automata on raw input)");
	}
	
	NFAActionRegister initial, final;
	std::tie(initial, final) = computeActionRegisterEntries(regex->actions);

	AFAState newState = base.addState();
	if (regex->literal.length() == 1) {
		base.addTransition(0, Transition(newState, std::make_shared<ByteSymbolGroup>(regex->literal[0], regex->literal[0]), initial));
	} else {
		if (!m_contextMachine.on.second) {
			// TODO: could actually be made into a warning and proceed just with the first character of regex->literal
			throw SemanticAnalysisException("Encountered multi-byte literal regex '" + regex->literal + "' at " + regex->locationString() + " within the finite automaton '" + m_contextMachine.name + "' (declared at " + m_contextMachine.locationString() + ") accepts raw input -- multibyte strings can not be recognized by other finite automata"); // TODO: add the words "will proceed with the first character ('X') instead"
			base.addTransition(0, Transition(newState, std::make_shared<ByteSymbolGroup>(regex->literal[0], regex->literal[0]), initial));
		} else {
			base.addTransition(0, Transition(newState, std::make_shared<LiteralSymbolGroup>(regex->literal), initial));
		}
	}

	base.finalStates.insert(newState);
	base.addFinalActions(final);

	return base;
}

NFA NFABuilder::visit(const ArbitrarySymbolRegex* regex) const {
	NFA base;

	NFAActionRegister initial, final;
	std::tie(initial, final) = computeActionRegisterEntries(regex->actions);

	auto newState = base.addState();
	for (const auto& sg : m_contextMachine.computeArbitrarySymbolGroupList()) {
		base.addTransition(0, Transition(newState, sg, initial));
	}
	base.finalStates.insert(newState);
	base.addFinalActions(final);

	return base;
}

NFA NFABuilder::visit(const ReferenceRegex* regex) const {
	NFA base;
	const AFAState newBaseState = base.addState();
	base.finalStates.insert(newBaseState);

	const MachineDefinition* statementMachine;
	auto statement = this->m_contextMachine.findMachineStatement(regex->referenceName, &statementMachine);
	auto statementCastIntoBuildable = std::dynamic_pointer_cast<INFABuildable>(statement);
	if (statementMachine->name == m_contextMachine.name) {
		auto typeFormingComponent = std::dynamic_pointer_cast<TypeFormingStatement>(statement);
		const std::string payloadPath = typeFormingComponent ? m_generationContextPath + "__" + regex->referenceName : "";
		NFAActionRegister initial, final;
		std::tie(initial, final) = computeActionRegisterEntries(regex->actions, payloadPath);

		NFABuilder contextualizedBuilder(m_contextMachine, statement.get(), m_generationContextPath);
		base = statementCastIntoBuildable->accept(contextualizedBuilder);
		base.addInitialActions(initial);
		base.addFinalActions(final);
	} else {
		NFAActionRegister initial, final;
		std::tie(initial, final) = computeActionRegisterEntries(regex->actions);
		auto typeFormingStatement = std::dynamic_pointer_cast<TypeFormingStatement>(statement);
		base.addTransition(0, Transition(newBaseState, std::make_shared<StatementSymbolGroup>(typeFormingStatement.get(), regex->referenceStatementMachine), initial));
		base.addFinalActions(final);
	}

	return base;
}

std::pair<NFAActionRegister, NFAActionRegister> NFABuilder::computeActionRegisterEntries(const std::list<RegexAction>& actions) const {
	return computeActionRegisterEntries(actions, "");
}

std::pair<NFAActionRegister, NFAActionRegister>  NFABuilder::computeActionRegisterEntries(const std::list<RegexAction>& actions, const std::string& payload) const {
	NFAActionRegister initial, final;

	for (const RegexAction& atp : actions) {
		if (payload.empty()) {
			final.emplace_back((NFAActionType)atp.type, m_generationContextPath, atp.target, atp.targetField);
		} else {
			final.emplace_back((NFAActionType)atp.type, m_generationContextPath, atp.target, atp.targetField, payload);
		}

		if (atp.type == RegexActionType::Capture
			|| atp.type == RegexActionType::Append
			|| atp.type == RegexActionType::Prepend) {
			initial.emplace_back(NFAActionType::InitiateCapture, m_generationContextPath, atp.target);
		}
	}

	return std::pair<NFAActionRegister, NFAActionRegister>(initial, final);
}
