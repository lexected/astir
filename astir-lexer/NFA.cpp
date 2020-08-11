#include "NFA.h"

#include <stack>

NFA::NFA()
    : finalStates(), transitionsByState() {
    transitionsByState.emplace_back();
}

void NFA::operator|=(const NFA& rhs) {
    State stateIndexShift = this->transitionsByState.size();
    std::vector<TransitionList> stateCopy = rhs.transitionsByState;
    for (auto& transitionList : stateCopy) {
        for (auto& transition : transitionList) {
            transition.target += stateIndexShift;
        }
    }

    std::set<State> finalStatesCopy;
    for (const auto& finalState : rhs.finalStates) {
        finalStatesCopy.insert(stateIndexShift+finalState);
    }

    transitionsByState.insert(transitionsByState.end(), stateCopy.begin(), stateCopy.end());
    finalStates.insert(finalStatesCopy.begin(), finalStatesCopy.end());

    addEmptyTransition(0, stateIndexShift);
}

void NFA::operator&=(const NFA& rhs) {
    State rhsStartState = this->transitionsByState.size();
    std::vector<TransitionList> stateCopy = rhs.transitionsByState;
    for (auto& transitionList : stateCopy) {
        for (auto& transition : transitionList) {
            transition.target += rhsStartState;
        }
    }

    std::set<State> finalStatesCopy;
    for (const auto& finalState : rhs.finalStates) {
        finalStatesCopy.insert(rhsStartState + finalState);
    }

    transitionsByState.insert(transitionsByState.end(), stateCopy.begin(), stateCopy.end());
    for (const auto& finalState : this->finalStates) {
        addEmptyTransition(finalState, rhsStartState);
    }
    this->finalStates = finalStatesCopy;
}

State NFA::addState() {
    auto ret = (State)transitionsByState.size();
    transitionsByState.emplace_back();
    return ret;
}

void NFA::addTransition(State state, const Transition& transition) {
    transitionsByState[state].push_back(transition);
}

Transition& NFA::addEmptyTransition(State state, State target) {
    addTransition(state, Transition(target, nullptr));
    return transitionsByState[state].back();
}

State NFA::concentrateFinalStates() {
    auto newFinalState = addState();
    for (auto finalState : finalStates) {
        addEmptyTransition(finalState, newFinalState);
    }
    finalStates.clear();
    finalStates.insert(newFinalState);

    return newFinalState;
}

NFA NFA::buildFromRegex(const DisjunctiveRegex* regex) {
    NFA base;
    for (const auto& conjunctiveRegex : regex->disjunction) {
        base |= NFA::buildFromRegex(conjunctiveRegex.get());
    }
    return base;
}

NFA NFA::buildFromRegex(const ConjunctiveRegex* regex) {
    NFA base;
    base.finalStates.insert(0);

    for (const auto& rootRegex : regex->conjunction) {
        base &= NFA::buildFromRegex(rootRegex.get());
    }
    return base;
}

NFA NFA::buildFromRegex(const RootRegex* regex) {
    const RepetitiveRegex* rr = dynamic_cast<const RepetitiveRegex*>(regex);
    if (rr) {
        return buildFromRegex(rr);
    }

    const LookaheadRegex* lr = dynamic_cast<const LookaheadRegex*>(regex);
    if (lr) {
        return buildFromRegex(lr);
    }

    const ActionAtomicRegex* aar = dynamic_cast<const ActionAtomicRegex*>(regex);
    if (aar) {
        return buildFromRegex(aar);
    }

    throw Exception("Unknown type of RootRegex encountered");
}

NFA NFA::buildFromRegex(const RepetitiveRegex* regex) {
    if (regex->minRepetitions == regex->INFINITE_REPETITIONS) {
        throw Exception("Can not create a machine for a regex with minimum of infinitely many repetitions");
    }

    NFA theMachine = NFA::buildFromRegex(regex->actionAtomicRegex.get());

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
        
        std::stack<State> interimConcentratedFinalStates({ lastBaseFinalState  });
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

NFA NFA::buildFromRegex(const LookaheadRegex* regex) {
    //TODO: implement LookaheadRegex handling
    throw Exception("LookaheadRegexes not supported at the moment");
}

NFA NFA::buildFromRegex(const ActionAtomicRegex* regex) {
    // TODO: implement action handling

    return NFA::buildFromRegex(regex->regex.get());
}

NFA NFA::buildFromRegex(const AtomicRegex* regex) {
    const DisjunctiveRegex* dr = dynamic_cast<const DisjunctiveRegex*>(regex);
    if (dr) {
        return NFA::buildFromRegex(dr);
    }

    const PrimitiveRegex* pr = dynamic_cast<const PrimitiveRegex*>(regex);
    if (pr) {
        return NFA::buildFromRegex(pr);
    }

    throw Exception("Unknown type of atomic regex encountered");
}

NFA NFA::buildFromRegex(const PrimitiveRegex* regex) {
    NFA base;
    auto newState = base.addState();
    base.addTransition(0, Transition(newState, regex));
    base.finalStates.insert(newState);

    return base;
}
