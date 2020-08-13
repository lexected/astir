#include "NFA.h"

#include <stack>
#include "Exception.h"

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
