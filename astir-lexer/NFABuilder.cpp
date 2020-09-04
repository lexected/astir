#include "NFABuilder.h"
#include "Exception.h"

#include <stack>

#include "SemanticTree.h"

NFA NFABuilder::visit(const Category* category) const {
	NFA alternationPoint;
	const std::string parentContextPath = m_generationContextPath + "__" + category->name;
	const std::string generationContextPathPrefix = parentContextPath + "__";
	for (const auto referencePair : category->references) {
		const std::string& newSubcontextName = referencePair.first;
		NFA alternativeNfa = referencePair.second.component->accept(*this);

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
	alternationPoint.addInitialTransitionActions(createContextActionRegister);
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
	regexNfa.addInitialTransitionActions(createContextActionRegister);
	regexNfa.registerContext(m_generationContextPath, rule->name);

	// return the contexted result
	return regexNfa;
}

NFA NFABuilder::visit(const DisjunctiveRegex* regex) const {
	NFA base;
	for (const auto& conjunctiveRegex : regex->disjunction) {
		base |= conjunctiveRegex->accept(*this);
	}
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

	NFA theMachine = regex->regex->accept(*this);

	NFA base;
	base.finalStates.insert(0);
	for (unsigned long i = 0; i < regex->minRepetitions; ++i) {
		base &= theMachine;
	}

	if (regex->maxRepetitions == regex->INFINITE_REPETITIONS) {
		auto theVeryFinalState = theMachine.addState();
		for (const auto& finalState : theMachine.finalStates) {
			theMachine.addEmptyTransition(finalState, 0);
			theMachine.addEmptyTransition(finalState, theVeryFinalState);
		}
		theMachine.finalStates = { theVeryFinalState };

		NFA theMachineSTAR;
		theMachineSTAR.finalStates.insert(0);
		theMachineSTAR &= theMachine;
		++theVeryFinalState;
		theMachineSTAR.addEmptyTransition(0, theVeryFinalState);

		base &= theMachineSTAR;
	} else {
		theMachine.concentrateFinalStates();
		auto lastBaseFinalState = base.concentrateFinalStates();

		std::stack<State> interimConcentratedFinalStates({ lastBaseFinalState });
		for (unsigned long i = regex->minRepetitions; i < regex->maxRepetitions; ++i) {
			base &= theMachine;
			auto interimFinalState = *base.finalStates.begin();
			interimConcentratedFinalStates.push(interimFinalState);
		}
		auto theVeryFinalState = interimConcentratedFinalStates.top();
		interimConcentratedFinalStates.pop();
		while (!interimConcentratedFinalStates.empty()) {
			auto stateToConnect = interimConcentratedFinalStates.top();
			interimConcentratedFinalStates.pop();

			base.addEmptyTransition(stateToConnect, theVeryFinalState);
		}
	}

	return base;
}

NFA NFABuilder::visit(const LookaheadRegex* regex) const {
	//TODO: implement LookaheadRegex handling
	throw Exception("LookaheadRegexes not supported at the moment");
}

NFA NFABuilder::visit(const AnyRegex* regex) const {
	NFA base;
	auto newState = base.addState();

	NFAActionRegister initial, final;
	std::tie(initial, final) = computeActionRegisterEntries(regex->actions);

	auto literalGroups = computeLiteralGroups(regex);
	for (auto& literalSymbolGroup : literalGroups) {
		base.addTransition(0, Transition(newState, std::make_shared<LiteralSymbolGroup>(literalSymbolGroup, initial)));
	}

	base.finalStates.insert(newState);
	base.addFinalActions(final);

	return base;
}

NFA NFABuilder::visit(const ExceptAnyRegex* regex) const {
	NFA base;
	auto newState = base.addState();

	auto literalGroups = computeLiteralGroups(regex);
	NFA::calculateDisjointLiteralSymbolGroups(literalGroups);
	auto negatedGroups = NFA::negateLiteralSymbolGroups(literalGroups);

	NFAActionRegister initial, final;
	std::tie(initial, final) = computeActionRegisterEntries(regex->actions);

	for (auto& literalSymbolGroup : negatedGroups) {
		base.addTransition(0, Transition(newState, std::make_shared<LiteralSymbolGroup>(literalSymbolGroup, initial)));
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
		base.addTransition(prevState, Transition(newState, std::make_shared<LiteralSymbolGroup>(c, c, initial)));

		prevState = newState;
	}

	base.finalStates.insert(prevState);
	base.addFinalActions(final);

	return base;
}

NFA NFABuilder::visit(const ArbitraryLiteralRegex* regex) const {
	NFA base;

	NFAActionRegister initial, final;
	std::tie(initial, final) = computeActionRegisterEntries(regex->actions);

	auto newState = base.addState();
	base.addTransition(0, Transition(newState, std::make_shared<ArbitrarySymbolGroup>(initial)));
	base.finalStates.insert(newState);
	base.addFinalActions(final);

	return base;
}

NFA NFABuilder::visit(const ReferenceRegex* regex) const {
	NFA base;
	const State newBaseState = base.addState();
	base.finalStates.insert(newBaseState);

	bool wasFoundInInputMachine;
	auto component = this->m_contextMachine.findMachineComponent(regex->referenceName, &wasFoundInInputMachine);
	if (wasFoundInInputMachine) {
		NFAActionRegister initial, final;
		std::tie(initial, final) = computeActionRegisterEntries(regex->actions);
		base.addTransition(0, Transition(newBaseState, std::make_shared<ProductionSymbolGroup>(component, initial)));
		base.addFinalActions(final);
	} else {
		const std::string payloadPath = component->isTypeForming() ? m_generationContextPath + "__" + regex->referenceName : "";
		NFAActionRegister initial, final;
		std::tie(initial, final) = computeActionRegisterEntries(regex->actions, payloadPath);

		NFABuilder contextualizedBuilder(m_contextMachine, component, m_generationContextPath);
		base = component->accept(contextualizedBuilder);
		base.addInitialTransitionActions(initial);
		base.addFinalActions(final);
	}

	return base;
}

NFA NFABuilder::visit(const LineEndRegex* regex) const {
	NFA base;

	NFAActionRegister initial, final;
	std::tie(initial, final) = computeActionRegisterEntries(regex->actions);

	auto lineFeedState = base.addState();
	base.addTransition(0, Transition(lineFeedState, std::make_shared<LiteralSymbolGroup>('\n', '\n', final)));
	base.finalStates.insert(lineFeedState);
	auto carriageReturnState = base.addState();
	base.addTransition(0, Transition(carriageReturnState, std::make_shared<LiteralSymbolGroup>('\r', '\r')));
	base.addTransition(carriageReturnState, Transition(lineFeedState, std::make_shared<LiteralSymbolGroup>('\n', '\n', final)));
	base.addInitialTransitionActions(initial);

	return base;
}

std::list<LiteralSymbolGroup> NFABuilder::computeLiteralGroups(const AnyRegex* regex) const {
	std::list<LiteralSymbolGroup> literalGroup;

	for (const auto& literal : regex->literals) {
		for (const auto& c : literal) {
			literalGroup.emplace_back(c, c);
		}
	}
	for (const auto& range : regex->ranges) {
		CharType beginning = (CharType)range.start;
		CharType end = (CharType)range.end;
		literalGroup.emplace_back(beginning, end);
	}

	return literalGroup;
}

std::pair<NFAActionRegister, NFAActionRegister> NFABuilder::computeActionRegisterEntries(const std::list<RegexAction>& actions) const {
	return computeActionRegisterEntries(actions, "");
}

std::pair<NFAActionRegister, NFAActionRegister>  NFABuilder::computeActionRegisterEntries(const std::list<RegexAction>& actions, const std::string& payload) const {
	NFAActionRegister initial, final;

	for (const RegexAction& atp : actions) {
		if (payload.empty()) {
			final.emplace_back((NFAActionType)atp.type, m_generationContextPath, atp.target);
		} else {
			final.emplace_back((NFAActionType)atp.type, m_generationContextPath, atp.target, payload);
		}

		if (atp.type == RegexActionType::Capture) {
			initial.emplace_back(NFAActionType::InitiateCapture, m_generationContextPath, atp.target);
		}
	}

	return std::pair<NFAActionRegister, NFAActionRegister>(initial, final);
}
