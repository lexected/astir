#include "NFABuilder.h"
#include "Exception.h"

#include <stack>

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

NFA NFABuilder::visit(const ActionAtomicRegex* regex) const {
    ActionRegister reg;
    for (const auto& atp : regex->actionTargetPairs) {
        reg.emplace_back(ActionRegisterEntryType::Add, atp);
    }

    auto atomicNfa = regex->accept(*this);
    for (auto finalState : atomicNfa.finalStates) {
        atomicNfa.states[finalState].actions = reg;
    }

    return atomicNfa;
}


NFA NFABuilder::visit(const PrimitiveRegex* regex) const {
    NFA base;
    auto newState = base.addState();
    base.addTransition(0, Transition(newState, regex));
    base.finalStates.insert(newState);

    return base;
}
