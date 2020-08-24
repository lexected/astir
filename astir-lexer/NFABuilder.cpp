#include "NFABuilder.h"
#include "Exception.h"

#include <stack>

#include "SemanticTree.h"

NFA NFABuilder::visit(const Category* category) const {
    NFA base;
    base.finalStates.insert(0);

    for (const auto referencePair : category->references) {
        NFABuilder contextualizedBuilder(this->m_context, referencePair.second);
        base |= referencePair.second->accept(contextualizedBuilder);
    }

    return base;
}

NFA NFABuilder::visit(const Rule* rule) const {
    NFABuilder contextualizedBuilder(this->m_context, rule);
    return rule->accept(contextualizedBuilder);
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

    NFA theMachine = regex->accept(*this);

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

    auto actionRegister = computeActionRegisterEntries(regex->actionTargetPairs);

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

    auto actionRegister = computeActionRegisterEntries(regex->actionTargetPairs);

    for (auto& literalSymbolGroup : negatedGroups) {
        base.addTransition(0, Transition(newState, std::make_shared<LiteralSymbolGroup>(literalSymbolGroup, actionRegister)));
    }

    base.finalStates.insert(newState);

    return base;
}

NFA NFABuilder::visit(const LiteralRegex* regex) const {
    NFA base;
    
    auto actionRegister = computeActionRegisterEntries(regex->actionTargetPairs);

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

    auto actionRegister = computeActionRegisterEntries(regex->actionTargetPairs);

    auto newState = base.addState();
    base.addTransition(0, Transition(newState, std::make_shared<ArbitrarySymbolGroup>(actionRegister)));
    base.finalStates.insert(newState);

    return base;
}


NFA NFABuilder::visit(const ReferenceRegex* regex) const {
    NFA base;

    auto actionRegister = computeActionRegisterEntries(regex->actionTargetPairs);

    auto newState = base.addState();
    bool follows;
    auto component = this->m_context.findMachineComponent(regex->referenceName, &follows);
    if (follows) {
        base.addTransition(0, Transition(newState, std::make_shared<ProductionSymbolGroup>(component, actionRegister)));
    } else {
        NFABuilder contextualizedBuilder(m_context, component);
        NFA ret = component->accept(contextualizedBuilder);
        ret.actionize(actionRegister);
        return ret;
    }
   
    base.finalStates.insert(newState);

    return base;
}

NFA NFABuilder::visit(const LineEndRegex* regex) const {
    NFA base;

    auto actionRegister = computeActionRegisterEntries(regex->actionTargetPairs);

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

ActionRegister NFABuilder::computeActionRegisterEntries(const std::list<ActionTargetPair>& actionTargetPairs) const {
    ActionRegister ret;

    for (const ActionTargetPair& atp : actionTargetPairs) {
        const Field* targetField = this->m_component->findField(atp.target);
        ret.emplace_back(atp.action, targetField);
    }

    return ret;
}
