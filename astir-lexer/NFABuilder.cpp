#include "NFABuilder.h"
#include "Exception.h"

#include <stack>

#include "SemanticTree.h"

NFA NFABuilder::visit(const Category* category) const {
	NFA base;

	const std::string parentContextPath = m_generationContextPath + "__" + category->name;
	const std::string generationContextPathPrefix = parentContextPath + "__";
	for (const auto referencePair : category->references) {
		const std::string& newSubcontextName = referencePair.first;
		NFA alternativeNfa = referencePair.second.component->accept(*this);

		NFAActionRegister elevateContextActionRegister;
		elevateContextActionRegister.emplace_back(NFAActionType::ElevateContext, parentContextPath, newSubcontextName);
		alternativeNfa.concentrateFinalStates(elevateContextActionRegister);

		base |= alternativeNfa;
	}

	return base;
}

NFA NFABuilder::visit(const Pattern* rule) const {
	const std::string newContextPath = m_generationContextPath + "__" + rule->name;

	NFABuilder contextualizedBuilder(this->m_contextMachine, rule, newContextPath);
	NFA regexNfa = rule->regex->accept(contextualizedBuilder);

	return regexNfa;
}

NFA NFABuilder::visit(const Production* rule) const {
	const std::string newContextPath = m_generationContextPath + "__" + rule->name;
	
	// create an NFA with two states, the second one final
	NFA ret;
	State interimState = ret.addState();
	ret.finalStates.insert(interimState);

	// add a transition from the first one to the second one, actioned by context creation
	NFAActionRegister createContextActionRegister;
	createContextActionRegister.emplace_back(NFAActionType::CreateContext, m_generationContextPath, rule->name);
	ret.addEmptyTransition(0, interimState, createContextActionRegister);
	ret.registerContext(m_generationContextPath, rule->name);

	// chain the regex NFA onto the contexted NFA we prepared above
	NFABuilder contextualizedBuilder(this->m_contextMachine, rule, newContextPath);
	NFA regexNfa = rule->regex->accept(contextualizedBuilder);
	ret &= regexNfa;

	// return the contexted result
	return ret;
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

	auto actionRegister = computeActionRegisterEntries(regex->actions, "");

	auto literalGroups = computeLiteralGroups(regex);
	for (auto& literalSymbolGroup : literalGroups) {
		base.addTransition(0, Transition(newState, std::make_shared<LiteralSymbolGroup>(literalSymbolGroup, actionRegister)));
	}

	base.finalStates.insert(newState);
	return base;
}

NFA NFABuilder::visit(const ExceptAnyRegex* regex) const {
	NFA base;
	auto newState = base.addState();

	auto literalGroups = computeLiteralGroups(regex);
	NFA::calculateDisjointLiteralSymbolGroups(literalGroups);
	auto negatedGroups = NFA::negateLiteralSymbolGroups(literalGroups);

	auto actionRegister = computeActionRegisterEntries(regex->actions, "");

	for (auto& literalSymbolGroup : negatedGroups) {
		base.addTransition(0, Transition(newState, std::make_shared<LiteralSymbolGroup>(literalSymbolGroup, actionRegister)));
	}

	base.finalStates.insert(newState);

	return base;
}

NFA NFABuilder::visit(const LiteralRegex* regex) const {
	NFA base;
	
	auto actionRegister = computeActionRegisterEntries(regex->actions, "");

	State prevState = 0;
	for(unsigned char c : regex->literal) {   
		State newState = base.addState();
		base.addTransition(prevState, Transition(newState, std::make_shared<LiteralSymbolGroup>(c, c, actionRegister)));

		prevState = newState;
	}

	base.finalStates.insert(prevState);

	return base;
}

NFA NFABuilder::visit(const ArbitraryLiteralRegex* regex) const {
	NFA base;

	auto actionRegister = computeActionRegisterEntries(regex->actions, "");

	auto newState = base.addState();
	base.addTransition(0, Transition(newState, std::make_shared<ArbitrarySymbolGroup>(actionRegister)));
	base.finalStates.insert(newState);

	return base;
}

NFA NFABuilder::visit(const ReferenceRegex* regex) const {
	NFA base;
	const State newBaseState = base.addState();
	base.finalStates.insert(newBaseState);

	bool wasFoundInInputMachine;
	auto component = this->m_contextMachine.findMachineComponent(regex->referenceName, &wasFoundInInputMachine);
	if (wasFoundInInputMachine) {
		auto actionRegister = computeActionRegisterEntries(regex->actions, "");
		base.addTransition(0, Transition(newBaseState, std::make_shared<ProductionSymbolGroup>(component, actionRegister)));
	} else {
		const std::string payloadPath = component->isTypeForming() ? m_generationContextPath + "__" + regex->referenceName : "";
		auto actionRegister = computeActionRegisterEntries(regex->actions, payloadPath);

		NFABuilder contextualizedBuilder(m_contextMachine, component, m_generationContextPath);
		base = component->accept(contextualizedBuilder);
		base.addFinalActions(actionRegister);
	}

	return base;
}

NFA NFABuilder::visit(const LineEndRegex* regex) const {
	NFA base;

	auto actionRegister = computeActionRegisterEntries(regex->actions, "");

	auto lineFeedState = base.addState();
	base.addTransition(0, Transition(lineFeedState, std::make_shared<LiteralSymbolGroup>('\n', '\n', actionRegister)));
	base.finalStates.insert(lineFeedState);
	auto carriageReturnState = base.addState();
	base.addTransition(0, Transition(carriageReturnState, std::make_shared<LiteralSymbolGroup>('\r', '\r')));
	base.addTransition(carriageReturnState, Transition(lineFeedState, std::make_shared<LiteralSymbolGroup>('\n', '\n', actionRegister)));

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
		unsigned char beginning = (unsigned char)range.start;
		unsigned char end = (unsigned char)range.end;
		literalGroup.emplace_back(beginning, end);
	}

	return literalGroup;
}

NFAActionRegister NFABuilder::computeActionRegisterEntries(const std::list<RegexAction>& actions, const std::string& payload) const {
	NFAActionRegister ret;

	for (const RegexAction& atp : actions) {
		if (payload.empty()) {
			ret.emplace_back((NFAActionType)atp.type, m_generationContextPath, atp.target);
			// within the context (m_generationContextPath) we modify the target (m_generationContextPath + "__" + atp.target) with the payload of the transition
		} else {
			ret.emplace_back((NFAActionType)atp.type, m_generationContextPath, atp.target, payload);
		}
	}

	return ret;
}
