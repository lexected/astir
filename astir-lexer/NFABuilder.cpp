#include "NFABuilder.h"
#include "Exception.h"

#include <stack>
#include <algorithm>

#include "SemanticTree.h"

NFA NFABuilder::visit(const Category* category) const {
	NFA alternationPoint;
	const std::string parentContextPath = m_generationContextPath + "__" + category->name;
	const std::string generationContextPathPrefix = parentContextPath + "__";
	for (const auto referencePair : category->references) {
		const std::string& newSubcontextName = referencePair.first;
		const INFABuildable* componentCastIntoBuildable = dynamic_cast<const INFABuildable*>(referencePair.second.component);
		NFA alternativeNfa = componentCastIntoBuildable->accept(*this);

		NFAActionRegister elevateContextActionRegister;
		
		if(referencePair.second.component->isTypeForming()) {
			// if the component is type-forming, a new context has been created in alternativeNfa and it needs to be elevated to the category level
			// but, if it is also terminal, we need to associate the raw capture with the context before elevating
			if (referencePair.second.component->isTerminal()) {
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

NFA NFABuilder::visit(const Pattern* rule) const {
	const std::string newContextPath = m_generationContextPath + "__" + rule->name;

	NFABuilder contextualizedBuilder(this->m_contextMachine, rule, newContextPath);
	NFA regexNfa = rule->regex->accept(contextualizedBuilder);

	return regexNfa;
}

NFA NFABuilder::visit(const Production* rule) const {
	const std::string newContextPath = m_generationContextPath + "__" + rule->name;

	// compute the NFA of the underlying regex
	NFABuilder contextualizedBuilder(this->m_contextMachine, rule, newContextPath);
	NFA regexNfa = rule->regex->accept(contextualizedBuilder);
	
	// add context creation actions
	NFAActionRegister createContextActionRegister;
	createContextActionRegister.emplace_back(NFAActionType::CreateContext, m_generationContextPath, rule->name);
	regexNfa.addInitialActions(createContextActionRegister);
	regexNfa.registerContext(m_generationContextPath, rule->name);

	// return the contexted result
	return regexNfa;
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
		base &= rootRegex->accept(*this);
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

	NFA atomMachine = regex->regex->accept(*this);
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
		const State startLoopBodyFinalState = theLongBranch.concentrateFinalStates();
		theLongBranch &= atomMachine;
		const State endLoopBodyFinalState = theLongBranch.concentrateFinalStates();
		theLongBranch.addEmptyTransition(endLoopBodyFinalState, startLoopBodyFinalState);
		theLongBranch.andNFA(atomMachine, true); // MUST BE TRUE
	} else if (regex->maxRepetitions >= 2) {
		unsigned long howManyToPrefix = std::max((unsigned long)2, regex->minRepetitions) - 1;
		for (unsigned long it = 0; it < howManyToPrefix; ++it) {
			theLongBranch &= atomMachine;
		}

		std::stack<State> interimFinalStates;
		interimFinalStates.push(theLongBranch.concentrateFinalStates());

		for (auto it = regex->minRepetitions; it < regex->maxRepetitions;++it) {
			theLongBranch &= atomMachine;
			interimFinalStates.push(theLongBranch.concentrateFinalStates());
		}
		const State endOfOptionalityState = interimFinalStates.top();
		interimFinalStates.pop();

		while (!interimFinalStates.empty()) {
			const State interimState = interimFinalStates.top();
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
	const State newState = base.addState();
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

	auto literalGroups = makeLiteralGroups(regex);
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

	auto literalGroups = makeLiteralGroups(regex);
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
	
	NFAActionRegister initial, final;
	std::tie(initial, final) = computeActionRegisterEntries(regex->actions);

	State prevState = 0;
	for(CharType c : regex->literal) {
		State newState = base.addState();
		base.addTransition(prevState, Transition(newState, std::make_shared<LiteralSymbolGroup>(c, c), initial));

		prevState = newState;
	}

	base.finalStates.insert(prevState);
	base.addFinalActions(final);

	return base;
}

NFA NFABuilder::visit(const ArbitrarySymbolRegex* regex) const {
	NFA base;

	NFAActionRegister initial, final;
	std::tie(initial, final) = computeActionRegisterEntries(regex->actions);

	auto newState = base.addState();
	base.addTransition(0, Transition(newState, createArbitrarySymbolGroup(), initial));
	base.finalStates.insert(newState);
	base.addFinalActions(final);

	return base;
}

NFA NFABuilder::visit(const ReferenceRegex* regex) const {
	NFA base;
	const State newBaseState = base.addState();
	base.finalStates.insert(newBaseState);

	const Machine* componentMachine;
	const MachineComponent* component = this->m_contextMachine.findMachineComponent(regex->referenceName, &componentMachine);
	const INFABuildable* componentCastIntoBuildable = dynamic_cast<const INFABuildable*>(component);
	if (componentMachine->name == m_contextMachine.name) {
		const std::string payloadPath = component->isTypeForming() ? m_generationContextPath + "__" + regex->referenceName : "";
		NFAActionRegister initial, final;
		std::tie(initial, final) = computeActionRegisterEntries(regex->actions, payloadPath);

		NFABuilder contextualizedBuilder(m_contextMachine, component, m_generationContextPath);
		base = componentCastIntoBuildable->accept(contextualizedBuilder);
		base.addInitialActions(initial);
		base.addFinalActions(final);
	} else {
		NFAActionRegister initial, final;
		std::tie(initial, final) = computeActionRegisterEntries(regex->actions);
		base.addTransition(0, Transition(newBaseState, std::make_shared<TerminalSymbolGroup>(component->calculateInstandingProductions()), initial));
		base.addFinalActions(final);
	}

	return base;
}

std::list<std::shared_ptr<SymbolGroup>> NFABuilder::makeLiteralGroups(const AnyRegex* regex) const {
	std::list<std::shared_ptr<SymbolGroup>> literalGroups;

	for (const auto& literal : regex->literals) {
		for (const auto& c : literal) {
			literalGroups.push_back(std::make_shared<LiteralSymbolGroup>(c, c));
		}
	}
	for (const auto& range : regex->ranges) {
		CharType beginning = (CharType)range.start;
		CharType end = (CharType)range.end;
		literalGroups.push_back(std::make_shared<LiteralSymbolGroup>(beginning, end));
	}

	return literalGroups;
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

std::shared_ptr<SymbolGroup> NFABuilder::createArbitrarySymbolGroup() const {
	if (m_contextMachine.on) {
		return std::make_shared<TerminalSymbolGroup>(m_contextMachine.on->getProductionRoots());
	} else {
		return std::make_shared<LiteralSymbolGroup>((CharType)0, (CharType)255);
	}
}
